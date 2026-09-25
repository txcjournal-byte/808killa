#include "Engine.h"

using namespace juce;

namespace
{
    enum DirtMode { soft, hardClip, tape, tube, foldback, bitcrush };
    constexpr float maxDriveDb = 36.0f;
}

KillMapping killMappingFor (int style) noexcept
{
    static constexpr KillMapping table[] = {
        { 0.30f, 0.25f, 3.0f, 0.25f },   // Atlanta Clean
        { 0.25f, 0.15f, 2.0f, 0.30f },   // Memphis Phonk
        { 0.20f, 0.15f, 3.0f, 0.20f },   // Rage Underground
        { 0.15f, 0.20f, 2.0f, 0.05f },   // Detroit Clip
        { 0.35f, 0.20f, 2.0f, 0.30f },   // Drill Chicago
        { 0.30f, 0.20f, 2.0f, 0.20f },   // Drill NY
        { 0.35f, 0.15f, 2.0f, 0.30f },   // Drill UK
        { 0.25f, 0.10f, 3.0f, 0.20f },   // Plugg Soft
        { 0.35f, 0.10f, 3.0f, 0.25f },   // Chicago Boom
        { 0.35f, 0.15f, 3.0f, 0.30f },   // Classic Trap Boom
    };
    return table[jlimit (0, (int) std::size (table) - 1, style)];
}

double Engine::tailSeconds (const EngineParams& p) noexcept
{
    return 0.25 + (p.length > 0.0f ? 1.0 : 0.0);
}

//==============================================================================
void Engine::prepare (double sampleRate, int maxBlockSize)
{
    fs = sampleRate;
    maxBlock = jmax (1, maxBlockSize);

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<dsp::Oversampling<float>> (
            2, i + 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        oversamplers[i]->initProcessing ((size_t) maxBlock);
        osLatency[i] = (int) oversamplers[i]->getLatencyInSamples();
    }

    latency = *std::max_element (osLatency.begin(), osLatency.end());

    for (auto& c : ch)
    {
        c.lowDelay.prepare (latency);
        c.dryDelay.prepare (latency);
        c.padDelay.prepare (latency);
    }

    dryBuf.setSize (2, maxBlock);
    lowBuf.setSize (2, maxBlock);
    preBuf.setSize (2, maxBlock);
    driveGain.assign ((size_t) maxBlock, 1.0f);

    const dsp::ProcessSpec spec { fs, (uint32) maxBlock, 2 };
    cleanSplit.prepare (spec);
    cleanSplit.setType (dsp::LinkwitzRileyFilterType::lowpass);
    monoSplit.prepare (spec);
    monoSplit.setType (dsp::LinkwitzRileyFilterType::lowpass);

    for (auto* s : { &inGain, &outGain, &mixAmt, &bypassAmt, &drive })
        s->reset (fs, 0.03);

    phoneHp1.setHighPass (fs, 200.0, 0.707);
    phoneHp2.setHighPass (fs, 200.0, 0.707);
    phonePeak.setPeak (fs, 1500.0, 1.0, 4.0);
    phoneLp.setLowPass (fs, 7000.0, 0.707);

    for (auto& c : ch)
    {
        c.hp20.setHighPass (fs, 20.0, 0.707);
        c.harmBand.setBandPass (fs, 180.0, 0.6);
        c.harmHigh.setHighPass (fs, 150.0, 0.707);
        c.clickHigh.setHighPass (fs, 1800.0, 0.707);
        c.kw1.setHighShelf (fs, 1681.974450955533, 0.7071752369554196, 3.999843853973347);
        c.kw2.setHighPass (fs, 38.13547087602444, 0.5003270373238773);
    }

    lufsBinLength = jmax (1, (int) (fs * 0.1));
    reset();
}

void Engine::reset()
{
    for (auto& c : ch)
    {
        for (auto* b : { &c.hp20, &c.sub, &c.tiltLow, &c.tiltHigh, &c.filt1, &c.filt2, &c.harmBand,
                         &c.harmHigh, &c.clickHigh, &c.post, &c.kw1, &c.kw2 })
            b->reset();
        c.lowDelay.reset();
        c.dryDelay.reset();
        c.padDelay.reset();
        c.dcX = c.dcY = c.hold = 0.0f;
        c.holdCount = 0;
    }

    for (auto& os : oversamplers)
        if (os != nullptr)
            os->reset();

    cleanSplit.reset();
    monoSplit.reset();
    for (auto* b : { &phoneHp1, &phoneHp2, &phonePeak, &phoneLp })
        b->reset();

    envFast = envSlow = envGate = notePeak = scEnv = 0.0f;
    lengthGain = agGain = 1.0f;
    rmsPre = rmsPost = 0.0f;
    holdoff = 0;
    cSub = cTilt = -999.0f;
    cCutoff = cRes = cPost = cClean = cMono = -1.0f;
    lufsBins.fill (0.0);
    lufsBin = lufsCount = 0;
    lufsAcc = 0.0;
    shortTermLufs = -100.0f;
    inPeak = outPeak = 0.0f;
}

//==============================================================================
void Engine::updateFilters (const EngineParams& p, float subDb)
{
    auto changed = [] (float& cache, float value, float eps)
    {
        if (std::abs (cache - value) <= eps) return false;
        cache = value;
        return true;
    };

    const auto subChanged = changed (cSub, subDb, 0.01f);
    const auto tiltChanged = changed (cTilt, p.tiltDb, 0.01f);

    // glide the cutoff so automation does not step audibly
    smoothCutoff += (p.cutoff - smoothCutoff) * 0.35f;
    const auto cutoffChanged = changed (cCutoff, smoothCutoff, 0.5f) | changed (cRes, p.resonance, 0.001f);
    const auto postChanged = changed (cPost, p.postFreq, 1.0f);

    for (auto& c : ch)
    {
        if (subChanged)  c.sub.setLowShelf (fs, 55.0, 0.8, subDb);
        if (tiltChanged)
        {
            c.tiltLow.setLowShelf (fs, 600.0, 0.5, -p.tiltDb);
            c.tiltHigh.setHighShelf (fs, 600.0, 0.5, p.tiltDb);
        }
        if (cutoffChanged)
        {
            const auto q = 0.6 + p.resonance * 7.0;
            c.filt1.setLowPass (fs, smoothCutoff, q);
            c.filt2.setLowPass (fs, smoothCutoff, 0.54);
        }
        if (postChanged)
        {
            if (p.postFreq >= 19500.0f) c.post.setBypass();
            else c.post.setLowPass (fs, p.postFreq, 0.707);
        }
    }

    if (changed (cClean, p.cleanFreq, 0.5f))
        cleanSplit.setCutoffFrequency (p.cleanFreq);
    if (changed (cMono, jmax (20.0f, p.monoBelow), 0.5f))
        monoSplit.setCutoffFrequency (jmax (20.0f, p.monoBelow));
}

float Engine::shape (int mode, float x) const noexcept
{
    switch (mode)
    {
        case hardClip: return jlimit (-1.0f, 1.0f, x);
        case tape:     return 0.6366f * std::atan (1.3f * x + 0.08f * x * x);
        case tube:     return x >= 0.0f ? 1.0f - std::exp (-x) : -0.8f * (1.0f - std::exp (1.25f * x));
        case foldback: return std::sin (x);
        case soft:
        case bitcrush:
        default:       return std::tanh (x);
    }
}

//==============================================================================
void Engine::process (AudioBuffer<float>& buffer, int numCh, const AudioBuffer<float>* sidechain, const EngineParams& p)
{
    const auto total = buffer.getNumSamples();
    numCh = jlimit (1, 2, numCh);

    // ---- effective values (KILL macro on top of the style)
    const auto km = killMappingFor (p.style);
    const auto effDrive = jlimit (0.0f, 1.5f, p.drive + p.kill * km.drive);
    const auto effPunch = jlimit (0.0f, 1.0f, p.punch + p.kill * km.punch);
    const auto effSubDb = jlimit (-6.0f, 15.0f, p.subDb + p.kill * km.subDb);
    const auto effClip  = jlimit (0.0f, 1.0f, p.clipper + p.kill * km.clipper);

    updateFilters (p, p.toneOn ? effSubDb : 0.0f);

    inGain.setTargetValue (Decibels::decibelsToGain (p.inGainDb));
    outGain.setTargetValue (Decibels::decibelsToGain (p.outGainDb));
    mixAmt.setTargetValue (p.mix / 100.0f);
    bypassAmt.setTargetValue (p.bypass ? 1.0f : 0.0f);
    drive.setTargetValue (effDrive);

    const auto osIndex = jlimit (0, 2, p.oversampling);
    const auto padDelay = latency - osLatency[(size_t) osIndex];

    const auto fastA = onePole (0.0005, fs), fastR = onePole (0.030, fs);
    const auto slowA = onePole (0.020, fs),  slowR = onePole (0.150, fs);
    const auto gateA = onePole (0.001, fs),  gateR = onePole (0.030, fs);
    const auto absL = std::abs (p.length);
    const auto lenClose = 1.0f - onePole (p.length < 0.0f ? 0.025 - 0.02 * absL : 0.04, fs);
    const auto lenOpen = 1.0f - onePole (p.length < 0.0f ? 0.001 : 0.04, fs);
    const auto rmsCoef = onePole (0.3, fs);
    const auto agCoef = 1.0f - onePole (0.05, fs);
    const auto scA = onePole (0.001, fs), scR = onePole (p.duckReleaseMs * 0.001, fs);
    const auto ceilingGain = Decibels::decibelsToGain (p.ceilingDb);
    const auto crush = p.crushBits < 23.5f;
    const auto crushSteps = std::pow (2.0f, p.crushBits - 1.0f);
    const auto holdFactor = p.dirtMode == bitcrush ? 1 + roundToInt (effDrive * 12.0f) : 1;
    const auto duckExp = 0.5f + 1.5f * (1.0f - p.duckShape);

    const auto scChannels = sidechain != nullptr ? sidechain->getNumChannels() : 0;
    const auto useDuck = p.duckOn && p.duck > 0.0f && scChannels > 0;

    // input meter
    {
        float pk = 0.0f;
        for (int c = 0; c < numCh; ++c)
            pk = jmax (pk, buffer.getMagnitude (c, 0, total));
        if (pk > inPeak.load()) inPeak = pk;
    }

    float scPeak = 0.0f;

    for (int start = 0; start < total; start += maxBlock)
    {
        const auto n = jmin (maxBlock, total - start);
        float* data[2] = { buffer.getWritePointer (0, start), numCh > 1 ? buffer.getWritePointer (1, start) : nullptr };

        // ================= stage A: input, shape, tone, band split
        for (int i = 0; i < n; ++i)
        {
            const auto g = inGain.getNextValue();
            float x[2] = { 0.0f, 0.0f };
            float detect = 0.0f;

            for (int c = 0; c < numCh; ++c)
            {
                dryBuf.setSample (c, i, data[c][i]);
                x[c] = ch[(size_t) c].hp20.process (data[c][i] * g);
                detect = jmax (detect, std::abs (x[c]));
            }

            // linked note detection (808s are monophonic)
            envFast = follow (envFast, detect, fastA, fastR);
            envSlow = follow (envSlow, detect, slowA, slowR);
            envGate = follow (envGate, detect, gateA, gateR);

            if (holdoff > 0) --holdoff;
            if (envFast > 0.003f && envFast > envSlow * 1.6f && holdoff == 0)
            {
                notePeak = envFast;
                holdoff = (int) (fs * 0.06);
            }
            notePeak = jmax (notePeak, envFast);

            const auto transient = jlimit (0.0f, 1.0f, (envFast - envSlow) / (envSlow + 1.0e-4f) / 1.5f);

            float lengthTarget = 1.0f;
            if (p.shapeOn && p.length < -0.001f)
            {
                const auto thr = notePeak * (0.03f + 0.45f * absL);
                if (envGate < thr && thr > 0.0f)
                    lengthTarget = (envGate / thr) * (envGate / thr);
            }
            else if (p.shapeOn && p.length > 0.001f && envGate > 0.0005f)
            {
                lengthTarget = jlimit (1.0f, Decibels::decibelsToGain (15.0f * p.length),
                                       std::pow (notePeak * 0.9f / envGate, 0.75f * p.length));
            }
            lengthGain += (lengthTarget - lengthGain) * (lengthTarget > lengthGain ? lenOpen : lenClose);

            driveGain[(size_t) i] = Decibels::decibelsToGain (drive.getNextValue() * maxDriveDb);

            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                auto v = x[c];

                const auto clickPart = st.clickHigh.process (v);
                if (p.shapeOn)
                {
                    v *= 1.0f + effPunch * 3.0f * transient;
                    v += clickPart * transient * p.click * 3.0f;
                    v *= lengthGain;
                }

                if (p.toneOn)
                {
                    v = st.sub.process (v);
                    v = st.tiltHigh.process (st.tiltLow.process (v));

                    const auto h = st.harmHigh.process (std::tanh (4.0f * st.harmBand.process (v)));
                    v += p.harmonics * 0.6f * h;

                    if (p.filterOn)
                    {
                        v = st.filt1.process (v);
                        if (p.slope == 1)
                            v = st.filt2.process (v);
                    }
                }

                float low = 0.0f, high = 0.0f;
                cleanSplit.processSample (c, v, low, high);
                if (! p.cleanLow || ! p.dirtOn)
                {
                    low = 0.0f;
                    high = v;
                }

                lowBuf.setSample (c, i, st.lowDelay.process (low, latency));
                preBuf.setSample (c, i, high);
                data[c][i] = high;
            }
        }

        // ================= stage B: oversampled dirt on the high band
        {
            dsp::AudioBlock<float> block (data, (size_t) numCh, (size_t) n);
            auto& os = *oversamplers[(size_t) osIndex];
            auto up = os.processSamplesUp (block);
            const auto factor = (int) os.getOversamplingFactor();
            const auto upN = (int) up.getNumSamples();

            if (p.dirtOn)
            {
                for (int c = 0; c < numCh; ++c)
                {
                    auto* d = up.getChannelPointer ((size_t) c);
                    for (int j = 0; j < upN; ++j)
                    {
                        const auto dry = d[j];
                        const auto wet = shape (p.dirtMode, dry * driveGain[(size_t) (j / factor)]);
                        d[j] = dry + (wet - dry) * p.dirtMix;
                    }
                }
            }

            os.processSamplesDown (block);
        }

        // ================= stage C: crush, auto gain, duck, output
        for (int i = 0; i < n; ++i)
        {
            float y[2] = { 0.0f, 0.0f };
            float preSq = 0.0f, postSq = 0.0f;

            // sidechain envelope
            if (scChannels > 0)
            {
                float s = 0.0f;
                for (int c = 0; c < jmin (2, scChannels); ++c)
                    s = jmax (s, std::abs (sidechain->getSample (c, start + i)));
                scPeak = jmax (scPeak, s);
                scEnv = follow (scEnv, s, scA, scR);
            }
            const auto duckGain = useDuck ? 1.0f - p.duck * std::pow (jlimit (0.0f, 1.0f, scEnv * 3.0f), duckExp) : 1.0f;

            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                auto v = st.padDelay.process (data[c][i], padDelay);

                if (p.dirtOn)
                {
                    if (holdFactor > 1)
                    {
                        if (--st.holdCount <= 0) { st.hold = v; st.holdCount = holdFactor; }
                        v = st.hold;
                    }
                    if (crush)
                        v = std::round (v * crushSteps) / crushSteps;
                    v = st.post.process (v);
                }

                // DC blocker
                const auto dc = v - st.dcX + 0.9995f * st.dcY;
                st.dcX = v;
                st.dcY = dc;
                v = dc;

                const auto pre = preBuf.getSample (c, i);
                preSq += pre * pre;
                postSq += v * v;
                y[c] = v;
            }

            rmsPre = preSq + (rmsPre - preSq) * rmsCoef;
            rmsPost = postSq + (rmsPost - postSq) * rmsCoef;
            const auto agTarget = (p.autoGain && p.dirtOn && rmsPost > 1.0e-9f)
                                      ? jlimit (0.1f, 4.0f, std::sqrt ((rmsPre + 1.0e-9f) / (rmsPost + 1.0e-9f)))
                                      : 1.0f;
            agGain += (agTarget - agGain) * agCoef;

            const auto og = outGain.getNextValue();
            const auto m = mixAmt.getNextValue();
            const auto b = bypassAmt.getNextValue();

            for (int c = 0; c < numCh; ++c)
                y[c] = (y[c] * agGain + lowBuf.getSample (c, i)) * duckGain;

            if (numCh == 2)
            {
                float lowL, highL, lowR, highR;
                monoSplit.processSample (0, y[0], lowL, highL);
                monoSplit.processSample (1, y[1], lowR, highR);
                if (p.monoBelow >= 1.0f)
                {
                    const auto low = 0.5f * (lowL + lowR);
                    y[0] = low + highL;
                    y[1] = low + highR;
                }
            }

            for (int c = 0; c < numCh; ++c)
            {
                // clipper last, so the ceiling holds (the mono split can overshoot on clipped waves)
                auto v = y[c];
                if (effClip > 0.001f)
                {
                    const auto u = v * (1.0f + effClip) / ceilingGain;
                    v = ceilingGain * ((1.0f - effClip) * std::tanh (u) + effClip * jlimit (-1.0f, 1.0f, u));
                }

                const auto dry = ch[(size_t) c].dryDelay.process (dryBuf.getSample (c, i), latency);
                v *= og;
                v = dry + (v - dry) * m;
                v = v + (dry - v) * b;
                y[c] = std::isfinite (v) ? v : 0.0f;
            }

            // loudness (before the phone check, which is monitoring only)
            double kSum = 0.0;
            for (int c = 0; c < numCh; ++c)
            {
                const auto k = ch[(size_t) c].kw2.process (ch[(size_t) c].kw1.process (y[c]));
                kSum += (double) k * k;
            }
            lufsAcc += numCh == 1 ? kSum * 2.0 : kSum;
            if (++lufsCount >= lufsBinLength)
            {
                lufsBins[(size_t) lufsBin] = lufsAcc / lufsCount;
                lufsBin = (lufsBin + 1) % (int) lufsBins.size();
                lufsAcc = 0.0;
                lufsCount = 0;
                double sum = 0.0;
                for (auto v : lufsBins) sum += v;
                const auto ms = sum / (double) lufsBins.size();
                shortTermLufs = ms > 1.0e-10 ? (float) (-0.691 + 10.0 * std::log10 (ms)) : -100.0f;
            }

            if (p.phone)
            {
                auto mono = numCh == 2 ? 0.5f * (y[0] + y[1]) : y[0];
                mono = phoneLp.process (phonePeak.process (phoneHp2.process (phoneHp1.process (mono))));
                y[0] = y[1] = mono;
            }

            for (int c = 0; c < numCh; ++c)
                data[c][i] = y[c];
        }
    }

    // "connected" = the sidechain bus carried signal within the last 3 seconds
    scSilentSamples = scPeak > 1.0e-4f ? 0 : jmin (scSilentSamples + total, 1 << 30);
    sidechainActive = scChannels > 0 && scSilentSamples < (int) (fs * 3.0);

    float pk = 0.0f;
    for (int c = 0; c < numCh; ++c)
        pk = jmax (pk, buffer.getMagnitude (c, 0, total));
    if (pk > outPeak.load()) outPeak = pk;
}
