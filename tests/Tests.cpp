// DSP unit tests for 808 KILLA. Run: K808Tests (exit code 0 = all passed)
#include "PluginProcessor.h"

using namespace juce;

namespace
{
    int failures = 0;

    void check (bool ok, const String& what)
    {
        if (! ok)
        {
            ++failures;
            std::cout << "  FAIL: " << what << std::endl;
        }
    }

    // 808-like test signal: pitched sine with a pitch drop and exponential decay, one note per 0.5 s
    float test808 (double t)
    {
        const auto local = std::fmod (t, 0.5);
        return (float) (0.8 * std::exp (-local * 4.0) * std::sin (2.0 * MathConstants<double>::pi * (45.0 * local + 3.0 * (1.0 - std::exp (-local * 40.0)))));
    }

    AudioBuffer<float> makeInput (double sampleRate, double seconds, int channels = 2)
    {
        AudioBuffer<float> b (channels, (int) (sampleRate * seconds));
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const auto t = i / sampleRate;
            const auto v = t < seconds - 0.5 ? test808 (t) : 0.0f;
            for (int c = 0; c < channels; ++c)
                b.setSample (c, i, v * (c == 0 ? 1.0f : 0.9f));
        }
        return b;
    }

    AudioBuffer<float> run (K808Processor& p, const AudioBuffer<float>& input, double sampleRate, int blockSize,
                            const AudioBuffer<float>* sidechain = nullptr)
    {
        const auto chans = input.getNumChannels();
        const auto total = chans + (sidechain != nullptr ? 2 : 0);
        p.setRateAndBufferSizeDetails (sampleRate, blockSize);
        p.prepareToPlay (sampleRate, blockSize);

        AudioBuffer<float> out (chans, input.getNumSamples());
        AudioBuffer<float> block (total, blockSize);
        MidiBuffer midi;

        for (int start = 0; start < input.getNumSamples(); start += blockSize)
        {
            const auto n = jmin (blockSize, input.getNumSamples() - start);
            AudioBuffer<float> view (block.getArrayOfWritePointers(), total, n);
            for (int c = 0; c < chans; ++c)
                view.copyFrom (c, 0, input, c, start, n);
            if (sidechain != nullptr)
                for (int c = 0; c < 2; ++c)
                    view.copyFrom (chans + c, 0, *sidechain, jmin (c, sidechain->getNumChannels() - 1), start, n);
            p.processBlock (view, midi);
            for (int c = 0; c < chans; ++c)
                out.copyFrom (c, start, view, c, 0, n);
        }
        return out;
    }

    bool allFinite (const AudioBuffer<float>& b)
    {
        for (int c = 0; c < b.getNumChannels(); ++c)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (c, i)))
                    return false;
        return true;
    }

    void setParam (K808Processor& p, const char* id, float value)
    {
        auto* param = p.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    void enableSidechain (K808Processor& p, bool on)
    {
        auto layout = p.getBusesLayout();
        if (layout.inputBuses.size() > 1)
            layout.inputBuses.getReference (1) = on ? AudioChannelSet::stereo() : AudioChannelSet::disabled();
        p.setBusesLayout (layout);
    }
}

int main()
{
    ScopedJuceInitialiser_GUI init;

    // ---------------------------------------------------------------- presets x sample rates
    std::cout << "[1] every preset, sample rates 44.1-192 kHz, odd block sizes" << std::endl;
    {
        K808Processor proc;
        enableSidechain (proc, false);
        const auto& list = proc.presets.getPresets();

        for (int i = 0; i < list.size(); ++i)
        {
            proc.presets.load (i);
            for (auto sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                const auto input = makeInput (sr, 1.5);
                const auto out = run (proc, input, sr, sr > 90000 ? 1024 : 333);
                const auto peak = out.getMagnitude (0, out.getNumSamples());
                check (allFinite (out), list[i].name + " @" + String (sr) + ": NaN/inf");
                check (peak < 2.0f, list[i].name + " @" + String (sr) + ": output peak " + String (peak));
                check (peak > 0.01f, list[i].name + " @" + String (sr) + ": output silent");
            }
            std::cout << "    ok  " << list[i].name << std::endl;
        }

        // every dirt mode + oversampling factor
        for (int mode = 0; mode < 6; ++mode)
            for (int os = 0; os < 3; ++os)
            {
                proc.presets.init();
                setParam (proc, ParamIDs::dirtMode, (float) mode);
                setParam (proc, ParamIDs::oversample, (float) os);
                setParam (proc, ParamIDs::dirt, 1.0f);
                setParam (proc, ParamIDs::kill, 1.0f);
                const auto out = run (proc, makeInput (48000.0, 1.0), 48000.0, 512);
                check (allFinite (out) && out.getMagnitude (0, out.getNumSamples()) < 2.0f,
                       "dirt mode " + String (mode) + " os " + String (os));
            }
    }

    // ---------------------------------------------------------------- pitch FX + wobble
    std::cout << "[1b] pitch FX and wobble combinations" << std::endl;
    {
        K808Processor proc;
        enableSidechain (proc, false);
        for (int target = 0; target < 4; ++target)
            for (int rate : { 0, 4, 8 })
                for (int lfoShape = 0; lfoShape < 5; ++lfoShape)
                {
                    proc.presets.init();
                    setParam (proc, ParamIDs::knock, 12.0f);
                    setParam (proc, ParamIDs::dive, -24.0f);
                    setParam (proc, ParamIDs::diveDelay, 50.0f);
                    setParam (proc, ParamIDs::octDown, 1.0f);
                    setParam (proc, ParamIDs::octUp, 1.0f);
                    setParam (proc, ParamIDs::wobble, 1.0f);
                    setParam (proc, ParamIDs::wobbleTarget, (float) target);
                    setParam (proc, ParamIDs::wobbleRate, (float) rate);
                    setParam (proc, ParamIDs::wobbleShape, (float) lfoShape);
                    const auto out = run (proc, makeInput (48000.0, 1.5), 48000.0, 256);
                    const auto peak = out.getMagnitude (0, out.getNumSamples());
                    check (allFinite (out) && peak < 2.0f && peak > 0.01f,
                           "pitch/wobble target " + String (target) + " rate " + String (rate) + " shape " + String (lfoShape) + " peak " + String (peak));
                }
    }

    // ---------------------------------------------------------------- BEND really changes the pitch
    std::cout << "[1c] BEND -12 st halves the frequency" << std::endl;
    {
        K808Processor proc;
        enableSidechain (proc, false);
        proc.presets.init();
        setParam (proc, ParamIDs::dirtOn, 0.0f);
        setParam (proc, ParamIDs::clipper, 0.0f);
        setParam (proc, ParamIDs::shapeOn, 0.0f);
        setParam (proc, ParamIDs::dive, -12.0f);
        setParam (proc, ParamIDs::diveDelay, 0.0f);
        setParam (proc, ParamIDs::diveTime, 50.0f);
        AudioBuffer<float> in (2, 48000);
        for (int i = 0; i < in.getNumSamples(); ++i)
            for (int c = 0; c < 2; ++c)
                in.setSample (c, i, 0.5f * (float) std::sin (2.0 * MathConstants<double>::pi * 110.0 * i / 48000.0));
        const auto out = run (proc, in, 48000.0, 512);
        int crossings = 0;
        for (int i = 24000; i < 43200; ++i)
            if (out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f)
                ++crossings;
        const auto freq = crossings / 0.4;
        std::cout << "    measured " << freq << " Hz (expected ~55)" << std::endl;
        check (freq > 45.0 && freq < 65.0, "bend did not drop the pitch an octave (" + String (freq) + " Hz)");
    }

    // ---------------------------------------------------------------- offline == realtime
    std::cout << "[2] block size independence (offline render = realtime)" << std::endl;
    {
        K808Processor a, b;
        enableSidechain (a, false);
        enableSidechain (b, false);
        a.presets.load (1);
        b.presets.load (1);
        const auto input = makeInput (48000.0, 2.0);
        const auto outA = run (a, input, 48000.0, 64);
        const auto outB = run (b, input, 48000.0, 512);
        float maxDiff = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 24000; i < input.getNumSamples(); ++i)
                maxDiff = jmax (maxDiff, std::abs (outA.getSample (c, i) - outB.getSample (c, i)));
        std::cout << "    max difference " << maxDiff << std::endl;
        check (maxDiff < 1.0e-3f, "64 vs 512 sample blocks differ by " + String (maxDiff));
    }

    // ---------------------------------------------------------------- bypass
    std::cout << "[3] bypass = input delayed by the reported latency" << std::endl;
    {
        K808Processor p;
        enableSidechain (p, false);
        p.presets.load (2);
        setParam (p, ParamIDs::bypass, 1.0f);
        const auto input = makeInput (48000.0, 1.0);
        const auto out = run (p, input, 48000.0, 256);
        const auto lat = p.getLatencySamples();
        float maxDiff = 0.0f;
        for (int i = 4800; i < input.getNumSamples(); ++i)
            maxDiff = jmax (maxDiff, std::abs (out.getSample (0, i) - input.getSample (0, i - lat)));
        std::cout << "    latency " << lat << " samples, max difference " << maxDiff << std::endl;
        check (maxDiff < 1.0e-5f, "bypass is not transparent");
    }

    // ---------------------------------------------------------------- silence
    std::cout << "[4] silence after the notes stop" << std::endl;
    {
        K808Processor p;
        enableSidechain (p, false);
        p.presets.load (9);   // long tail style
        auto input = makeInput (48000.0, 3.0);
        input.clear (0, 48000 * 2, 48000);
        input.clear (1, 48000 * 2, 48000);
        const auto out = run (p, input, 48000.0, 512);
        const auto tail = out.getMagnitude (0, 48000 * 2 + 36000, 12000);
        std::cout << "    residual " << Decibels::gainToDecibels (tail) << " dB" << std::endl;
        check (tail < Decibels::decibelsToGain (-90.0f), "residual output in silence");
    }

    // ---------------------------------------------------------------- mono
    std::cout << "[5] mono -> mono" << std::endl;
    {
        K808Processor p;
        AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (AudioChannelSet::mono());
        layout.inputBuses.add (AudioChannelSet::disabled());
        layout.outputBuses.add (AudioChannelSet::mono());
        check (p.setBusesLayout (layout), "mono layout rejected");
        const auto out = run (p, makeInput (48000.0, 1.0, 1), 48000.0, 480);
        check (allFinite (out) && out.getMagnitude (0, out.getNumSamples()) > 0.01f, "mono processing");
    }

    // ---------------------------------------------------------------- duck
    std::cout << "[6] sidechain duck" << std::endl;
    {
        K808Processor withSc, noSc;
        enableSidechain (withSc, true);
        enableSidechain (noSc, true);
        for (auto* p : { &withSc, &noSc })
        {
            p->presets.load (0);
            setParam (*p, ParamIDs::duck, 1.0f);
        }
        const auto input = makeInput (48000.0, 1.5);
        AudioBuffer<float> kick (2, input.getNumSamples()), silent (2, input.getNumSamples());
        kick.clear();
        silent.clear();
        for (int i = 0; i < kick.getNumSamples(); ++i)
            if (std::fmod (i / 48000.0, 0.5) < 0.08)
                kick.setSample (0, i, 0.9f * (float) std::sin (i * 0.01)), kick.setSample (1, i, kick.getSample (0, i));

        const auto ducked = run (withSc, input, 48000.0, 512, &kick);
        const auto plain = run (noSc, input, 48000.0, 512, &silent);
        const auto rDuck = ducked.getRMSLevel (0, 0, ducked.getNumSamples());
        const auto rPlain = plain.getRMSLevel (0, 0, plain.getNumSamples());
        std::cout << "    rms with kick " << rDuck << ", without " << rPlain << std::endl;
        check (rDuck < rPlain * 0.9f, "duck has no effect");
        check (allFinite (ducked), "duck NaN");
    }

    // ---------------------------------------------------------------- state
    std::cout << "[7] state save / restore" << std::endl;
    {
        K808Processor a, b;
        a.presets.load (4);
        setParam (a, ParamIDs::kill, 0.77f);
        MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        for (auto* param : a.getParameters())
        {
            auto* ra = dynamic_cast<RangedAudioParameter*> (param);
            auto* rb = b.apvts.getParameter (ra->getParameterID());
            check (std::abs (ra->getValue() - rb->getValue()) < 1.0e-6f, "state mismatch " + ra->getParameterID());
        }
        check (b.presets.getCurrentName() == a.presets.getCurrentName(), "preset name not restored");
    }

    // ---------------------------------------------------------------- instances + CPU
    std::cout << "[8] 10 instances, CPU" << std::endl;
    {
        OwnedArray<K808Processor> procs;
        for (int i = 0; i < 10; ++i)
        {
            auto* p = procs.add (new K808Processor());
            enableSidechain (*p, false);
            p->presets.load (i);
        }

        const auto input = makeInput (48000.0, 5.0);
        const auto t0 = Time::getMillisecondCounterHiRes();
        for (auto* p : procs)
            check (allFinite (run (*p, input, 48000.0, 512)), "instance NaN");
        const auto seconds = (Time::getMillisecondCounterHiRes() - t0) / 1000.0;
        const auto perInstance = seconds / 10.0 / 5.0 * 100.0;
        std::cout << "    one instance uses about " << String (perInstance, 2) << " % of one CPU core (2x oversampling)" << std::endl;
        check (perInstance < 10.0, "too much CPU");
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " FAILURES") << std::endl;
    return failures == 0 ? 0 : 1;
}
