#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Biquad coefficients (RBJ audio EQ cookbook)
namespace
{
    constexpr double pi = juce::MathConstants<double>::pi;

    void setNormalised (Biquad& f, double b0, double b1, double b2, double a0, double a1, double a2)
    {
        f.b0 = (float) (b0 / a0);
        f.b1 = (float) (b1 / a0);
        f.b2 = (float) (b2 / a0);
        f.a1 = (float) (a1 / a0);
        f.a2 = (float) (a2 / a0);
    }

    float onePoleCoef (double seconds, double fs)
    {
        return (float) std::exp (-1.0 / (seconds * fs));
    }

    float follow (float env, float input, float attack, float release)
    {
        const auto coef = input > env ? attack : release;
        return input + (env - input) * coef;
    }

    enum Types { spinz, zay, shortType, boostType, crunch, rage, dirty, subType };
    enum Modes { clean, clipped, underground, deep, destroy };
}

void Biquad::setLowShelf (double fs, double freq, double q, double gainDb)
{
    const auto A = std::pow (10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * freq / fs;
    const auto cosw = std::cos (w0);
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto sqA = 2.0 * std::sqrt (A) * alpha;

    setNormalised (*this,
                   A * ((A + 1) - (A - 1) * cosw + sqA),
                   2 * A * ((A - 1) - (A + 1) * cosw),
                   A * ((A + 1) - (A - 1) * cosw - sqA),
                   (A + 1) + (A - 1) * cosw + sqA,
                   -2 * ((A - 1) + (A + 1) * cosw),
                   (A + 1) + (A - 1) * cosw - sqA);
}

void Biquad::setLowPass (double fs, double freq, double q)
{
    const auto w0 = 2.0 * pi * juce::jmin (freq, fs * 0.45) / fs;
    const auto cosw = std::cos (w0);
    const auto alpha = std::sin (w0) / (2.0 * q);
    setNormalised (*this, (1 - cosw) / 2, 1 - cosw, (1 - cosw) / 2, 1 + alpha, -2 * cosw, 1 - alpha);
}

void Biquad::setHighPass (double fs, double freq, double q)
{
    const auto w0 = 2.0 * pi * freq / fs;
    const auto cosw = std::cos (w0);
    const auto alpha = std::sin (w0) / (2.0 * q);
    setNormalised (*this, (1 + cosw) / 2, -(1 + cosw), (1 + cosw) / 2, 1 + alpha, -2 * cosw, 1 - alpha);
}

//==============================================================================
const juce::StringArray K808Processor::typeNames { "SPINZ", "ZAY", "SHORT", "BOOST", "CRUNCH", "RAGE", "DIRTY", "SUB" };
const juce::StringArray K808Processor::modeNames { "CLEAN", "CLIPPED", "UNDERGROUND", "DEEP", "DESTROY" };

K808Processor::K808Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "K808", createLayout())
{
    pPunch    = apvts.getRawParameterValue (ParamIDs::punch);
    pSub      = apvts.getRawParameterValue (ParamIDs::sub);
    pDistort  = apvts.getRawParameterValue (ParamIDs::distort);
    pClip     = apvts.getRawParameterValue (ParamIDs::clip);
    pShort    = apvts.getRawParameterValue (ParamIDs::shortEnv);
    pBoost    = apvts.getRawParameterValue (ParamIDs::boost);
    pHardClip = apvts.getRawParameterValue (ParamIDs::hardClip);
    pGrit     = apvts.getRawParameterValue (ParamIDs::grit);
    pLowMono  = apvts.getRawParameterValue (ParamIDs::lowMono);
    pCook     = apvts.getRawParameterValue (ParamIDs::cook);
    pType     = apvts.getRawParameterValue (ParamIDs::type);
    pMode     = apvts.getRawParameterValue (ParamIDs::mode);
    pMix      = apvts.getRawParameterValue (ParamIDs::mix);
    pOutput   = apvts.getRawParameterValue (ParamIDs::output);
    pLimiter  = apvts.getRawParameterValue (ParamIDs::limiter);
    pCeiling  = apvts.getRawParameterValue (ParamIDs::ceiling);
}

juce::AudioProcessorValueTreeState::ParameterLayout K808Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = AudioParameterFloatAttributes()
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })
                       .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() / 100.0f; });

    auto knob = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           NormalisableRange<float> (0.0f, 1.0f), def, percent));
    };

    knob (ParamIDs::punch,   "Punch",   0.28f);
    knob (ParamIDs::sub,     "Sub",     0.46f);
    knob (ParamIDs::distort, "Distort", 0.72f);
    knob (ParamIDs::clip,    "Clip",    0.63f);

    auto toggle = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    toggle (ParamIDs::shortEnv, "Short",     true);
    toggle (ParamIDs::boost,    "Boost",     false);
    toggle (ParamIDs::hardClip, "Hard Clip", true);
    toggle (ParamIDs::grit,     "Grit",      false);
    toggle (ParamIDs::lowMono,  "Low Mono",  true);
    toggle (ParamIDs::cook,     "Cook",      false);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::type, 1 }, "Type", typeNames, 6));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", modeNames, 2));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 72.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));

    auto db = AudioParameterFloatAttributes()
                  .withLabel ("dB")
                  .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, db));
    toggle (ParamIDs::limiter, "Limiter", true);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::ceiling, 1 }, "Ceiling",
                                                       NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -1.0f, db));
    return layout;
}

bool K808Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

//==============================================================================
void K808Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const auto numCh = (size_t) juce::jmax (1, juce::jmin (2, getTotalNumOutputChannels()));

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        numCh, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    oversampling->reset();

    latency = (int) oversampling->getLatencyInSamples();
    setLatencySamples (latency);

    dryBuffer.setSize ((int) numCh, samplesPerBlock);
    dryDelayBuffer.setSize ((int) numCh, juce::jmax (1, latency));
    dryDelayBuffer.clear();
    dryDelayWritePos = 0;

    monoCrossover.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    monoCrossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    monoCrossover.setCutoffFrequency (120.0f);
    monoCrossover.reset();

    const auto osRate = sampleRate * 2.0;
    driveSmoothed.reset (osRate, 0.03);
    clipSmoothed.reset (osRate, 0.03);
    mixSmoothed.reset (sampleRate, 0.03);
    outGainSmoothed.reset (sampleRate, 0.03);
    driveSmoothed.setCurrentAndTargetValue (1.0f);
    clipSmoothed.setCurrentAndTargetValue (1.0f);
    mixSmoothed.setCurrentAndTargetValue (pMix->load() / 100.0f);
    outGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    for (auto& c : channels)
    {
        c = ChannelState();
    }

    limiterGain = 1.0f;
    lastSub = -1.0f;
    lastType = lastMode = -1;
}

void K808Processor::updateFilters (float subAmount, int type, int mode)
{
    if (juce::approximatelyEqual (subAmount, lastSub) && type == lastType && mode == lastMode)
        return;

    lastSub = subAmount;
    lastType = type;
    lastMode = mode;

    const auto fs = currentSampleRate;
    const auto subDb = subAmount * 12.0 + (type == subType ? 5.0 : 0.0) + (mode == deep ? 3.0 : 0.0);

    for (auto& c : channels)
    {
        c.subShelf.setLowShelf (fs, 70.0, 0.8, subDb);

        switch (type)
        {
            case zay:       c.typeFilter.setHighPass (fs, 30.0, 0.7); break;
            case shortType: c.typeFilter.setHighPass (fs, 45.0, 0.7); break;
            case crunch:    c.typeFilter.setHighPass (fs, 60.0, 0.7); break;
            case subType:   c.typeFilter.setLowPass  (fs, 2500.0, 0.7); break;
            default:        c.typeFilter.setBypass(); break;
        }

        switch (mode)
        {
            case underground: c.modeLowPass.setLowPass (fs, 5500.0, 0.707); break;
            case deep:        c.modeLowPass.setLowPass (fs, 9000.0, 0.707); break;
            default:          c.modeLowPass.setBypass(); break;
        }

        if (mode == deep)
            c.modeShelf.setLowShelf (fs, 55.0, 0.8, 3.0);
        else
            c.modeShelf.setBypass();
    }
}

float K808Processor::shape (int type, float x) const noexcept
{
    switch (type)
    {
        case spinz:
            return std::tanh (x);

        case zay:
        {
            constexpr float k = 2.0f / juce::MathConstants<float>::pi;
            return k * (std::atan (1.2f * x + 0.2f) - std::atan (0.2f));
        }

        case shortType:
        {
            if (x >= 1.5f)  return 1.0f;
            if (x <= -1.5f) return -1.0f;
            return x - (4.0f / 27.0f) * x * x * x;
        }

        case boostType:
            return 0.95f * std::tanh (1.8f * x);

        case crunch:
            return 0.8f * (x / (1.0f + std::abs (x))) + 0.2f * std::tanh (4.0f * x);

        case rage:
            return juce::jlimit (-1.0f, 1.0f, 1.5f * x);

        case dirty:
            return x >= 0.0f ? 1.0f - std::exp (-1.5f * x)
                             : -0.85f * (1.0f - std::exp (1.1f * x));

        case subType:
        default:
            return std::tanh (0.8f * x);
    }
}

//==============================================================================
void K808Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numCh = juce::jmin (2, getTotalNumInputChannels(), dryBuffer.getNumChannels());
    const auto total = buffer.getNumSamples();

    for (auto i = numCh; i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, total);

    if (numCh == 0 || total == 0 || oversampling == nullptr)
        return;

    // ---- parameters
    const auto punch    = pPunch->load();
    const auto sub      = pSub->load();
    const auto distort  = pDistort->load();
    const auto clipAmt  = pClip->load();
    const auto shortOn  = pShort->load() > 0.5f;
    const auto boostOn  = pBoost->load() > 0.5f;
    const auto hardOn   = pHardClip->load() > 0.5f;
    const auto gritOn   = pGrit->load() > 0.5f;
    const auto monoOn   = pLowMono->load() > 0.5f && numCh == 2;
    const auto cookOn   = pCook->load() > 0.5f;
    const auto type     = juce::jlimit (0, 7, (int) pType->load());
    const auto mode     = juce::jlimit (0, 4, (int) pMode->load());
    const auto limitOn  = pLimiter->load() > 0.5f;
    const auto ceilGain = juce::Decibels::decibelsToGain (pCeiling->load());

    updateFilters (sub, type, mode);

    static constexpr float modeDriveScale[] = { 0.35f, 1.0f, 1.0f, 0.8f, 1.5f };
    driveSmoothed.setTargetValue (juce::Decibels::decibelsToGain (distort * 30.0f * modeDriveScale[mode]));
    clipSmoothed.setTargetValue (1.0f - clipAmt * 0.75f);
    mixSmoothed.setTargetValue (pMix->load() / 100.0f);
    outGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    const auto fs = currentSampleRate;
    const auto fastAtk = onePoleCoef (0.0005, fs), fastRel = onePoleCoef (0.030, fs);
    const auto slowAtk = onePoleCoef (0.020, fs),  slowRel = onePoleCoef (0.150, fs);
    const auto gateAtk = onePoleCoef (0.001, fs),  gateRel = onePoleCoef (0.040, fs);
    const auto gateOpen = 1.0f - onePoleCoef (0.001, fs), gateClose = 1.0f - onePoleCoef (0.025, fs);
    const auto limRelease = onePoleCoef (0.080, fs);
    const auto inGain = boostOn ? 2.0f : 1.0f;
    constexpr float gateThreshold = 0.12f;

    // ---- input meter
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto peak = buffer.getMagnitude (ch, 0, total);
        if (peak > inPeak[ch].load()) inPeak[ch].store (peak);
    }
    if (numCh == 1)
        inPeak[1].store (inPeak[0].load());

    const auto maxChunk = dryBuffer.getNumSamples();

    for (int start = 0; start < total; start += maxChunk)
    {
        const auto n = juce::jmin (maxChunk, total - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), numCh, start, n);

        for (int ch = 0; ch < numCh; ++ch)
            dryBuffer.copyFrom (ch, 0, chunk, ch, 0, n);

        // ---- pre stage: boost, punch, short, sub, type filter
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& st = channels[(size_t) ch];
            auto* d = chunk.getWritePointer (ch);

            for (int i = 0; i < n; ++i)
            {
                auto x = d[i] * inGain;
                const auto a = std::abs (x);

                st.envFast = follow (st.envFast, a, fastAtk, fastRel);
                st.envSlow = follow (st.envSlow, a, slowAtk, slowRel);
                const auto transient = juce::jmax (0.0f, st.envFast - st.envSlow) / (st.envSlow + 1.0e-3f);
                x *= 1.0f + punch * 2.5f * juce::jmin (transient, 1.5f);

                if (shortOn)
                {
                    st.envGate = follow (st.envGate, a, gateAtk, gateRel);
                    const auto ratio = st.envGate / gateThreshold;
                    const auto target = ratio < 1.0f ? ratio * ratio : 1.0f;
                    st.gateGain += (target - st.gateGain) * (target > st.gateGain ? gateOpen : gateClose);
                    x *= st.gateGain;
                }
                else
                {
                    st.gateGain = 1.0f;
                }

                x = st.subShelf.process (x);
                d[i] = st.typeFilter.process (x);
            }
        }

        // ---- oversampled non-linear stage
        {
            juce::dsp::AudioBlock<float> block (chunk);
            auto os = oversampling->processSamplesUp (block);
            const auto osN = (int) os.getNumSamples();

            for (int i = 0; i < osN; ++i)
            {
                const auto drive = driveSmoothed.getNextValue();
                const auto t = clipSmoothed.getNextValue();
                const auto makeup = 1.0f / std::sqrt (t);

                for (int ch = 0; ch < numCh; ++ch)
                {
                    auto* d = os.getChannelPointer ((size_t) ch);
                    auto y = shape (type, d[i] * drive);

                    if (cookOn)
                        y = 0.85f * std::tanh (1.8f * (y + 0.3f * y * y));

                    y = hardOn ? juce::jlimit (-t, t, y) : t * std::tanh (y / t);
                    y *= makeup;

                    switch (mode)
                    {
                        case clipped:     y = juce::jlimit (-0.85f, 0.85f, 1.3f * y); break;
                        case underground: y = std::tanh (1.3f * y); break;
                        case destroy:     y = 0.9f * std::sin (2.2f * y); break;
                        default: break;
                    }

                    d[i] = y;
                }
            }

            oversampling->processSamplesDown (block);
        }

        // ---- post stage: grit, dc, mode filters, mix, low mono, output, limiter
        for (int i = 0; i < n; ++i)
        {
            const auto mix = mixSmoothed.getNextValue();
            const auto outGain = outGainSmoothed.getNextValue();
            float s[2] = { 0.0f, 0.0f };

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& st = channels[(size_t) ch];
                auto x = chunk.getSample (ch, i);

                if (gritOn)
                {
                    if (--st.holdCounter <= 0)
                    {
                        st.holdValue = x;
                        st.holdCounter = 3;
                    }
                    const auto q = std::round (st.holdValue * 48.0f) / 48.0f;
                    x = 0.4f * x + 0.6f * q;
                }

                const auto dc = x - st.dcX + 0.9995f * st.dcY;
                st.dcX = x;
                st.dcY = dc;
                x = st.modeShelf.process (st.modeLowPass.process (dc));

                auto dry = dryBuffer.getSample (ch, i);
                if (latency > 0)
                {
                    auto* delay = dryDelayBuffer.getWritePointer (ch);
                    std::swap (dry, delay[dryDelayWritePos]);
                }

                s[ch] = dry * (1.0f - mix) + x * mix;
            }

            if (latency > 0)
                dryDelayWritePos = (dryDelayWritePos + 1) % latency;

            if (monoOn)
            {
                float lowL, highL, lowR, highR;
                monoCrossover.processSample (0, s[0], lowL, highL);
                monoCrossover.processSample (1, s[1], lowR, highR);
                const auto low = 0.5f * (lowL + lowR);
                s[0] = low + highL;
                s[1] = low + highR;
            }

            s[0] *= outGain;
            s[1] *= outGain;

            if (limitOn)
            {
                const auto peak = juce::jmax (std::abs (s[0]), std::abs (s[1]));
                const auto target = peak > ceilGain ? ceilGain / peak : 1.0f;
                limiterGain = target < limiterGain ? target : target + (limiterGain - target) * limRelease;

                for (auto& v : s)
                    v = juce::jlimit (-ceilGain, ceilGain, v * limiterGain);
            }

            for (int ch = 0; ch < numCh; ++ch)
                chunk.setSample (ch, i, s[ch]);
        }
    }

    // ---- output meter
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto peak = buffer.getMagnitude (ch, 0, total);
        if (peak > outPeak[ch].load()) outPeak[ch].store (peak);
    }
    if (numCh == 1)
        outPeak[1].store (outPeak[0].load());
}

//==============================================================================
void K808Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void K808Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* K808Processor::createEditor()
{
    return new K808Editor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new K808Processor();
}
