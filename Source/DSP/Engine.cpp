#include "Engine.h"

using namespace juce;

namespace
{
    constexpr float focusHz = 550.0f, focusQ = 0.8f;       // centre of the 250 - 900 Hz presence band
    constexpr float subCutHz = 28.0f;
    const float hardLimit = Decibels::decibelsToGain (-0.1f);

    // slope of each curve at zero, for level compensation of the drive
    float slopeAtZero (int mode) noexcept { return mode == 0 ? 2.0f / MathConstants<float>::pi : 1.0f; }
}

float Engine::saturate (int mode, float x) noexcept
{
    switch (mode)
    {
        case 0:  // Tape: warm, rounded
            return 2.0f / MathConstants<float>::pi * std::atan (x);
        case 1:  // Tube: asymmetric, compresses the positive half more
            return x >= 0.0f ? x / (1.0f + x) : std::tanh (x);
        default: // Foldback: reflects everything above the threshold back inside
        {
            constexpr float t = 1.0f;
            if (std::abs (x) <= t) return x;
            // triangle fold, stable for any input size
            const auto period = 4.0f * t;
            auto y = std::fmod (x + t, period);
            if (y < 0.0f) y += period;
            return y < 2.0f * t ? y - t : 3.0f * t - y;
        }
    }
}

float Engine::softClip (float x, float t) noexcept
{
    const auto a = std::abs (x);
    if (a < t) return x;
    return std::copysign (t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t)), x);
}

//==============================================================================
void Engine::prepare (double sampleRate, int maxBlockSize)
{
    fs = sampleRate;
    maxBlock = jmax (1, maxBlockSize);

    oversampler = std::make_unique<dsp::Oversampling<float>> (2, oversamplingStages,
                                                              dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing ((size_t) maxBlock);
    osFs = fs * (double) oversampler->getOversamplingFactor();
    latency = (int) std::round (oversampler->getLatencyInSamples());

    dsp::ProcessSpec osSpec { osFs, (uint32) (maxBlock * (int) oversampler->getOversamplingFactor()), 2 };
    crossover.prepare (osSpec);
    crossover.setType (dsp::LinkwitzRileyFilterType::lowpass);

    for (auto& c : ch)
    {
        // 4th-order Butterworth high-pass at 28 Hz (two biquads)
        c.subCut1.setHighPass (osFs, subCutHz, 0.5412);
        c.subCut2.setHighPass (osFs, subCutHz, 1.3066);
        c.phoneHp1.setHighPass (fs, 400.0, 0.7071);
        c.phoneHp2.setHighPass (fs, 400.0, 0.7071);
        c.phoneLp1.setLowPass (fs, 3500.0, 0.7071);
        c.phoneLp2.setLowPass (fs, 3500.0, 0.7071);
        c.dryDelay.prepare (latency);
    }

    dryBuf.setSize (2, maxBlock);
    duckGain.assign ((size_t) maxBlock, 1.0f);

    for (auto* s : { &inGain, &outGain, &clipGain, &ceiling })
        s->reset (fs, 0.02);
    for (auto* s : { &driveSmoothed, &focusSmoothed, &crossoverSmoothed, &bypassAmt })
        s->reset (fs, 0.02);

    inGain.setCurrentAndTargetValue (1.0f);
    outGain.setCurrentAndTargetValue (1.0f);
    clipGain.setCurrentAndTargetValue (1.0f);
    ceiling.setCurrentAndTargetValue (1.0f);
    scopeLength = jmax (1, (int) (fs * 0.005));
    cCrossover = -1.0f;
    cFocus = -999.0f;
    reset();
}

void Engine::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();
    crossover.reset();
    for (auto& c : ch)
    {
        for (auto* b : { &c.subCut1, &c.subCut2, &c.focus, &c.phoneHp1, &c.phoneHp2, &c.phoneLp1, &c.phoneLp2 })
            b->reset();
        c.dcX = c.dcY = 0.0f;
        c.dryDelay.reset();
    }
    scEnv = 0.0f;
    firstBlock = true;
    scopeCount = 0;
    scopeInMax = scopeOutMax = 0.0f;
    inPeak = outPeak = clipDb = duckDb = 0.0f;
}

void Engine::updateFilters (float crossoverHz, float focusDb)
{
    if (std::abs (crossoverHz - cCrossover) > 0.05f)
    {
        cCrossover = crossoverHz;
        crossover.setCutoffFrequency (crossoverHz);
    }
    if (std::abs (focusDb - cFocus) > 0.01f)
    {
        cFocus = focusDb;
        for (auto& c : ch)
            c.focus.setPeak (osFs, focusHz, focusQ, focusDb);
    }
}

//==============================================================================
void Engine::process (AudioBuffer<float>& buffer, int numCh, const AudioBuffer<float>* sidechain, const EngineParams& p)
{
    const auto total = buffer.getNumSamples();
    numCh = jlimit (1, 2, numCh);
    const auto factor = (int) oversampler->getOversamplingFactor();

    inGain.setTargetValue (Decibels::decibelsToGain (p.inputGainDb));
    outGain.setTargetValue (Decibels::decibelsToGain (p.outputDb));
    clipGain.setTargetValue (Decibels::decibelsToGain (p.clipDriveDb));
    ceiling.setTargetValue (Decibels::decibelsToGain (p.ceilingDb));
    driveSmoothed.setTargetValue (jlimit (0.0f, 100.0f, p.drive));
    focusSmoothed.setTargetValue (p.focusDb);
    crossoverSmoothed.setTargetValue (jlimit (80.0f, 200.0f, p.crossoverHz));
    bypassAmt.setTargetValue (p.bypass ? 1.0f : 0.0f);

    if (firstBlock)
    {
        // start on the current settings instead of gliding in from zero
        firstBlock = false;
        for (auto* v : { &inGain, &outGain, &clipGain, &ceiling })
            v->setCurrentAndTargetValue (v->getTargetValue());
        for (auto* v : { &driveSmoothed, &focusSmoothed, &crossoverSmoothed, &bypassAmt })
            v->setCurrentAndTargetValue (v->getTargetValue());
    }

    const auto polarity = p.phaseInvert ? -1.0f : 1.0f;
    const auto knee = jlimit (0.5f, 0.95f, p.clipKnee);
    const auto satMode = jlimit (0, 2, p.satMode);
    const auto scA = 1.0f - std::exp (-1.0f / (float) (fs * 0.002));                         // 2 ms attack
    const auto scR = 1.0f - std::exp (-1.0f / (float) (fs * jmax (10.0f, p.duckReleaseMs) * 0.001));
    const auto scChannels = sidechain != nullptr ? sidechain->getNumChannels() : 0;
    const auto depth = jlimit (0.0f, 1.0f, p.duckDepth * 0.01f);
    const auto dcCoef = 1.0f - (float) (2.0 * MathConstants<double>::pi * 5.0 / osFs);        // DC blocker for the tube curve

    {
        float pk = 0.0f;
        for (int c = 0; c < numCh; ++c)
            pk = jmax (pk, buffer.getMagnitude (c, 0, total));
        if (pk > inPeak.load()) inPeak = pk;
    }

    float scPeak = 0.0f, clipMax = 1.0f, duckMin = 1.0f;

    for (int start = 0; start < total; start += maxBlock)
    {
        const auto n = jmin (maxBlock, total - start);
        float* data[2] = { buffer.getWritePointer (0, start), numCh > 1 ? buffer.getWritePointer (1, start) : nullptr };

        // ---- base rate: input gain, polarity, dry copy, sidechain envelope
        for (int i = 0; i < n; ++i)
        {
            const auto g = inGain.getNextValue() * polarity;
            for (int c = 0; c < numCh; ++c)
            {
                dryBuf.setSample (c, i, data[c][i]);
                data[c][i] *= g;
            }

            float s = 0.0f;
            for (int c = 0; c < jmin (2, scChannels); ++c)
                s = jmax (s, std::abs (sidechain->getSample (c, start + i)));
            scPeak = jmax (scPeak, s);
            scEnv += (s - scEnv) * (s > scEnv ? scA : scR);
            duckGain[(size_t) i] = scChannels > 0 ? 1.0f - depth * jlimit (0.0f, 1.0f, scEnv * 2.0f) : 1.0f;
            duckMin = jmin (duckMin, duckGain[(size_t) i]);
        }

        // ---- 4x: crossover, sub, mid/high saturation, sum, soft clipper
        dsp::AudioBlock<float> block (data, (size_t) numCh, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        const auto upN = (int) up.getNumSamples();
        float* u[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

        focusSmoothed.skip (n);
        crossoverSmoothed.skip (n);
        updateFilters (crossoverSmoothed.getCurrentValue(), focusSmoothed.getCurrentValue());

        for (int j = 0; j < upN; ++j)
        {
            const auto base = j / factor;
            if (j % factor == 0)
            {
                driveSmoothed.getNextValue();
                clipGain.getNextValue();
                ceiling.getNextValue();
            }
            const auto k = 1.0f + driveSmoothed.getCurrentValue() * 0.3f;               // up to +30 dB into the saturator
            const auto makeup = 1.0f / std::sqrt (k * slopeAtZero (satMode));           // keeps the level in the same range
            const auto cg = clipGain.getCurrentValue();
            const auto ceil = ceiling.getCurrentValue();

            float low[2] = { 0.0f, 0.0f }, high[2] = { 0.0f, 0.0f };
            for (int c = 0; c < numCh; ++c)
                crossover.processSample (c, u[c][j], low[c], high[c]);

            // sub band: mono, 28 Hz cut, ducking
            if (p.subMono && numCh == 2)
                low[0] = low[1] = 0.5f * (low[0] + low[1]);

            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                auto sub = low[c];
                if (p.subCut)
                    sub = st.subCut2.process (st.subCut1.process (sub));
                sub *= duckGain[(size_t) base];

                // mid/high band: focus bell, drive, saturation
                auto mid = st.focus.process (high[c]);
                mid = saturate (satMode, mid * k) * makeup;
                const auto dc = mid - st.dcX + dcCoef * st.dcY;       // tube is asymmetric: remove its DC
                st.dcX = mid;
                st.dcY = dc;

                // sum and soft clip to the ceiling
                const auto sum = (sub + dc) * cg;
                const auto y = softClip (sum / ceil, knee) * ceil;
                if (std::abs (sum) > 1.0e-3f && std::abs (y) > 1.0e-6f)
                    clipMax = jmax (clipMax, std::abs (sum) / std::abs (y));
                u[c][j] = y;
            }
        }

        oversampler->processSamplesDown (block);

        // ---- base rate: phone preview, output trim, hard limit, bypass, metering
        for (int i = 0; i < n; ++i)
        {
            const auto og = outGain.getNextValue();
            const auto b = bypassAmt.getNextValue();
            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                auto v = data[c][i];
                if (p.phone)
                    v = st.phoneLp2.process (st.phoneLp1.process (st.phoneHp2.process (st.phoneHp1.process (v))));
                v = jlimit (-hardLimit, hardLimit, v * og);

                const auto dry = st.dryDelay.process (dryBuf.getSample (c, i), latency);
                v += (dry - v) * b;
                data[c][i] = std::isfinite (v) ? v : 0.0f;

                scopeInMax = jmax (scopeInMax, std::abs (dry));
                scopeOutMax = jmax (scopeOutMax, std::abs (data[c][i]));
            }

            if (++scopeCount >= scopeLength)
            {
                const auto w = scopeWrite.load (std::memory_order_relaxed);
                scopeIn[(size_t) w].store (scopeInMax, std::memory_order_relaxed);
                scopeOut[(size_t) w].store (scopeOutMax, std::memory_order_relaxed);
                scopeWrite.store ((w + 1) % scopeSize, std::memory_order_release);
                scopeCount = 0;
                scopeInMax = scopeOutMax = 0.0f;
            }
        }
    }

    scSilentSamples = scPeak > 1.0e-4f ? 0 : jmin (scSilentSamples + total, 1 << 30);
    sidechainActive = scChannels > 0 && scSilentSamples < (int) (fs * 3.0);

    float pk = 0.0f;
    for (int c = 0; c < numCh; ++c)
        pk = jmax (pk, buffer.getMagnitude (c, 0, total));
    if (pk > outPeak.load()) outPeak = pk;

    const auto gr = Decibels::gainToDecibels (clipMax);
    if (gr > clipDb.load()) clipDb = gr;
    const auto dg = -Decibels::gainToDecibels (duckMin, -60.0f);
    if (dg > duckDb.load()) duckDb = dg;
}
