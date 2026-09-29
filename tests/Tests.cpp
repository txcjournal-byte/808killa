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
                            const AudioBuffer<float>* sidechain = nullptr, int resetAtSample = -1)
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
            if (resetAtSample >= start && resetAtSample < start + blockSize)
                p.reset();                                   // what a host does when the transport stops
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
                    setParam (proc, ParamIDs::clip, 0.3f);    // octave layers add level: keep the ceiling in the loop
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

    // ---------------------------------------------------------------- chop + width
    std::cout << "[1d] chop patterns and width" << std::endl;
    {
        K808Processor proc;
        enableSidechain (proc, false);
        proc.presets.init();
        const auto plain = run (proc, makeInput (48000.0, 1.5), 48000.0, 512);
        const auto plainRms = plain.getRMSLevel (0, 0, plain.getNumSamples());
        for (int pattern = 0; pattern < 7; ++pattern)
        {
            proc.presets.init();
            setParam (proc, ParamIDs::chop, 1.0f);
            setParam (proc, ParamIDs::chopPattern, (float) pattern);
            setParam (proc, ParamIDs::width, 1.0f);
            const auto out = run (proc, makeInput (48000.0, 1.5), 48000.0, 512);
            const auto rms = out.getRMSLevel (0, 0, out.getNumSamples());
            check (allFinite (out) && out.getMagnitude (0, out.getNumSamples()) < 2.0f, "chop pattern " + String (pattern));
            check (rms < plainRms * 0.95f, "chop pattern " + String (pattern) + " does not cut (" + String (rms) + " vs " + String (plainRms) + ")");
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
        p.presets.load (4);   // Sacred Boom: long tail
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

    // ---------------------------------------------------------------- auto level after stop / play
    std::cout << "[9] auto level: first hit after a long silence is not louder" << std::endl;
    {
        K808Processor p;
        enableSidechain (p, false);
        p.presets.load (1);   // Pure Sub: no dirt, almost linear
        setParam (p, ParamIDs::clipper, 0.0f);
        setParam (p, ParamIDs::kill, 0.0f);
        setParam (p, ParamIDs::punch, 0.0f);     // PUNCH reacts to the silence on purpose; test the level only
        setParam (p, ParamIDs::autoLevel, 1.0f);

        const double sr = 48000.0;
        AudioBuffer<float> input (2, (int) (sr * 10.0));
        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            const auto t = i / sr;
            const auto playing = t < 2.0 || t >= 8.0;              // play, stop for 6 s, play again
            const auto v = playing ? 0.15f * test808 (t < 2.0 ? t : t - 8.0) : 0.0f;
            input.setSample (0, i, v);
            input.setSample (1, i, v);
        }
        const auto out = run (p, input, sr, 512);
        const auto lat = p.getLatencySamples();
        const auto steady = out.getMagnitude (0, (int) (sr * 1.5) + lat, (int) (sr * 0.5));
        const auto first = out.getMagnitude (0, (int) (sr * 8.0) + lat, (int) (sr * 0.4));
        std::cout << "    steady peak " << steady << ", first hit after silence " << first << std::endl;
        check (first < steady * 1.25f, "first hit after silence is " + String (Decibels::gainToDecibels (first / steady), 1) + " dB louder");
    }

    // ---------------------------------------------------------------- first hit after stop / play, full processing
    std::cout << "[10] first hit after stop (host reset) sounds like every other hit (dirt, punch, clipper)" << std::endl;
    {
        const double sr = 48000.0;
        AudioBuffer<float> input (2, (int) (sr * 12.0));
        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            const auto t = i / sr;
            const auto playing = t < 3.0 || t >= 9.0;              // play, stop for 6 s, play again
            const auto v = playing ? 0.15f * test808 (t < 3.0 ? t : t - 9.0) : 0.0f;
            input.setSample (0, i, v);
            input.setSample (1, i, v);
        }

        for (auto index : { 0, 16, 24, 64, 80 })   // Holy Water, Brickface, Jawbreaker, Mosh Pit, Vomitorium
        {
            K808Processor p;
            enableSidechain (p, false);
            p.presets.load (index);
            const auto out = run (p, input, sr, 512, nullptr, (int) (sr * 6.0));   // host reset in the silence
            const auto lat = p.getLatencySamples();
            auto rms = [&] (double t) { return out.getRMSLevel (0, (int) (sr * t) + lat, (int) (sr * 0.12)); };
            const auto steady = rms (2.5), first = rms (9.0);
            const auto diffDb = Decibels::gainToDecibels (first / steady);
            std::cout << "    " << p.presets.getCurrentName() << ": first hit " << String (diffDb, 2) << " dB vs steady" << std::endl;
            check (diffDb < 0.5f, p.presets.getCurrentName() + ": first hit after silence is " + String (diffDb, 1) + " dB louder");
        }
    }

    // ---------------------------------------------------------------- KICK
    std::cout << "[11] KICK: adds a hit to a plain sub, DROP falls into the note without a gap, left softens" << std::endl;
    {
        const double sr = 48000.0;
        AudioBuffer<float> sub (2, (int) (sr * 3.0));
        for (int i = 0; i < sub.getNumSamples(); ++i)
        {
            const auto t = std::fmod (i / sr, 1.0);
            const auto fade = jmin (1.0, t * 2000.0) * jmin (1.0, (1.0 - t) * 200.0);   // no clicks between the notes
            const auto v = (float) (0.6 * std::sin (MathConstants<double>::twoPi * 32.7 * t) * std::exp (-t * 1.2) * fade);
            sub.setSample (0, i, v);
            sub.setSample (1, i, v);
        }

        auto render = [&] (const AudioBuffer<float>& in, float kick, float drop)
        {
            K808Processor p;
            enableSidechain (p, false);
            setParam (p, ParamIDs::dirtOn, 0.0f);
            setParam (p, ParamIDs::clipper, 0.0f);
            setParam (p, ParamIDs::punch, 0.0f);
            setParam (p, ParamIDs::toneOn, 0.0f);    // HEAT adds its own harmonics; test the hit alone
            setParam (p, ParamIDs::kick, kick);
            setParam (p, ParamIDs::kickDrop, drop);
            auto out = run (p, in, sr, 512);
            AudioBuffer<float> aligned (1, in.getNumSamples() - p.getLatencySamples());
            aligned.copyFrom (0, 0, out, 0, p.getLatencySamples(), aligned.getNumSamples());
            return aligned;
        };
        // energy above 250 Hz in the first 30 ms of the note at 2 s, relative to the whole attack
        auto attackBrightness = [&] (const AudioBuffer<float>& b)
        {
            IIRFilter hp, hp2;   // 4th order, so the fundamental of the 808 stays out
            hp.setCoefficients (IIRCoefficients::makeHighPass (sr, 250.0));
            hp2.setCoefficients (IIRCoefficients::makeHighPass (sr, 250.0));
            double all = 0.0, high = 0.0;
            const auto start = (int) (sr * 2.0);
            for (int i = start - 4800; i < start + (int) (sr * 0.03); ++i)
            {
                const auto v = b.getSample (0, i);
                const auto h = hp2.processSingleSampleRaw (hp.processSingleSampleRaw (v));
                if (i >= start) { all += v * v; high += h * h; }
            }
            return (float) (10.0 * std::log10 ((high + 1.0e-12) / (all + 1.0e-12)));
        };

        const auto plain = render (sub, 0.0f, 0.0f);
        const auto kicked = render (sub, 1.0f, 0.0f);
        const auto b0 = attackBrightness (plain), b1 = attackBrightness (kicked);
        std::cout << "    attack brightness: plain " << String (b0, 1) << " dB, KICK +100 % " << String (b1, 1) << " dB" << std::endl;
        check (b1 > b0 + 10.0f, "KICK does not add a hit to a plain sub");

        const auto dropped = render (sub, 0.0f, 0.8f);
        const auto start = (int) (sr * 2.0);
        int crossings = 0;
        for (int i = start + 1; i < start + (int) (sr * 0.03); ++i)
            if (dropped.getSample (0, i - 1) <= 0.0f && dropped.getSample (0, i) > 0.0f)
                ++crossings;
        float worst = 0.0f;   // deepest gap of the drop vs the plain note, 5 ms windows over the first 80 ms
        for (int w = 0; w < 16; ++w)
        {
            const auto a = dropped.getMagnitude (0, start + w * 240, 240), b = plain.getMagnitude (0, start + w * 240, 240);
            worst = jmin (worst, Decibels::gainToDecibels (a / jmax (b, 1.0e-6f)));
        }
        std::cout << "    DROP 80 %: " << crossings << " cycles in the first 30 ms (plain sub: 1), deepest gap " << String (worst, 1) << " dB" << std::endl;
        check (crossings >= 3, "DROP does not fall from above");
        check (worst > -6.0f, "DROP leaves a gap of " + String (worst, 1) + " dB");

        // kit-like 808: the sub plus a short bright click on every note
        auto kit = sub;
        for (int i = 0; i < kit.getNumSamples(); ++i)
        {
            const auto t = std::fmod (i / sr, 1.0);
            const auto click = (float) (0.3 * std::sin (MathConstants<double>::twoPi * 1500.0 * t) * std::exp (-t * 300.0));
            for (int c = 0; c < 2; ++c)
                kit.setSample (c, i, kit.getSample (c, i) + click);
        }
        const auto k0 = render (kit, 0.0f, 0.0f), kSoft = render (kit, -1.0f, 0.0f);
        const auto s0 = attackBrightness (k0), s1 = attackBrightness (kSoft);
        std::cout << "    kit 808 attack brightness: " << String (s0, 1) << " dB, KICK -100 % " << String (s1, 1) << " dB" << std::endl;
        check (s1 < s0 - 3.0f, "KICK left does not soften the hit");
    }

    // ---------------------------------------------------------------- transparent when inserted
    std::cout << "[12] a fresh instance passes the 808 unchanged; every effect stays finite and under the ceiling" << std::endl;
    {
        const double sr = 48000.0;
        auto input = makeInput (sr, 2.0);
        input.copyFrom (1, 0, input, 0, 0, input.getNumSamples());   // mono 808
        {
            K808Processor p;
            enableSidechain (p, false);
            const auto out = run (p, input, sr, 512);
            const auto lat = p.getLatencySamples();
            float maxDiff = 0.0f;
            for (int i = 4800; i < input.getNumSamples() - lat; ++i)
                maxDiff = jmax (maxDiff, std::abs (out.getSample (0, i + lat) - input.getSample (0, i)));
            const auto db = Decibels::gainToDecibels (maxDiff / input.getMagnitude (0, 0, input.getNumSamples()), -200.0f);
            std::cout << "    fresh instance vs input: " << String (db, 1) << " dB" << std::endl;
            check (db < -50.0f, "a fresh instance changes the 808 (" + String (db, 1) + " dB)");
        }
        for (auto* id : { ParamIDs::clip, ParamIDs::dirt, ParamIDs::metal, ParamIDs::buzz, ParamIDs::kill })
        {
            K808Processor p;
            enableSidechain (p, false);
            setParam (p, id, 1.0f);
            const auto out = run (p, input, sr, 512);
            const auto peak = out.getMagnitude (0, 0, out.getNumSamples());
            std::cout << "    " << id << " 100 %: peak " << String (Decibels::gainToDecibels (peak), 1) << " dB" << std::endl;
            check (allFinite (out), String (id) + ": NaN");
            check (peak < 4.0f, String (id) + ": runaway level");
        }
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " FAILURES") << std::endl;
    return failures == 0 ? 0 : 1;
}
