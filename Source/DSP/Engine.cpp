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
        { 0.08f, 0.20f, 3.0f, 0.15f },   // Pure
        { 0.30f, 0.15f, 2.0f, 0.30f },   // Grit
        { 0.30f, 0.15f, 3.0f, 0.30f },   // Rage
        { 0.20f, 0.20f, 2.0f, 0.40f },   // Brick
        { 0.25f, 0.35f, 2.0f, 0.30f },   // Knock
        { 0.35f, 0.20f, 2.0f, 0.35f },   // Hard
        { 0.30f, 0.15f, 2.0f, 0.25f },   // Warm
        { 0.15f, 0.10f, 3.0f, 0.15f },   // Soft
        { 0.30f, 0.10f, 4.0f, 0.25f },   // Boom
        { 0.30f, 0.15f, 3.0f, 0.45f },   // Loud
    };
    return table[jlimit (0, (int) std::size (table) - 1, style)];
}

double Engine::tailSeconds (const EngineParams& p) noexcept
{
    return 0.25 + (p.length > 0.0f ? 1.0 : 0.0) + (p.pitchOn && p.dive < 0.0f ? 1.0 : 0.0);
}

//==============================================================================
void Engine::prepare (double sampleRate, int maxBlockSize)
{
    fs = sampleRate;
    scopeLength = jmax (1, (int) (sampleRate * 0.005));
    maxBlock = jmax (1, maxBlockSize);

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<dsp::Oversampling<float>> (
            2, i + 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        oversamplers[i]->initProcessing ((size_t) maxBlock);
        osLatency[i] = (int) oversamplers[i]->getLatencyInSamples();
    }

    osMaxLatency = *std::max_element (osLatency.begin(), osLatency.end());
    lookahead = roundToInt (fs * 0.03);
    latency = lookahead + osMaxLatency;
    crossfadeLength = jmax (1, roundToInt (fs * 0.005));

    for (auto& c : ch)
    {
        c.lowDelay.prepare (osMaxLatency);
        c.osBypass.prepare (osMaxLatency);
        c.protectDelay.prepare (osMaxLatency);
        c.dryDelay.prepare (latency);
        c.padDelay.prepare (osMaxLatency);
        c.pitchBuf.assign ((size_t) (fs * 4.0) + (size_t) lookahead + 8, 0.0f);
        c.octLow.setLowPass (fs, 250.0, 0.707);
        c.octSub.setLowPass (fs, 160.0, 0.707);
        c.octUpHigh.setHighPass (fs, 35.0, 0.707);
    }

    dryBuf.setSize (2, maxBlock);
    lowBuf.setSize (2, maxBlock);
    preBuf.setSize (2, maxBlock);
    driveGain.assign ((size_t) maxBlock, 1.0f);
    wobbleGain.assign ((size_t) maxBlock, 1.0f);
    wobbleCutoff.assign ((size_t) maxBlock, 20000.0f);
    chopGain.assign ((size_t) maxBlock, 1.0f);
    protectEnv.assign ((size_t) maxBlock, 0.0f);
    protectEnvDelay.prepare (osMaxLatency);
    widthDelay.prepare ((int) (fs * 0.02));
    widthHp1.setHighPass (fs, 150.0, 0.707);
    widthHp2.setHighPass (fs, 150.0, 0.707);
    tunerFactor = jmax (1, roundToInt (fs / 2000.0));
    tunerRate = fs / tunerFactor;
    tunerLp1.setLowPass (fs, 400.0, 0.707);
    tunerLp2.setLowPass (fs, 400.0, 0.707);

    const dsp::ProcessSpec spec { fs, (uint32) maxBlock, 2 };
    cleanSplit.prepare (spec);
    cleanSplit.setType (dsp::LinkwitzRileyFilterType::lowpass);
    monoSplit.prepare (spec);
    monoSplit.setType (dsp::LinkwitzRileyFilterType::lowpass);
    wobbleFilter.prepare (spec);
    wobbleFilter.setType (dsp::StateVariableTPTFilterType::lowpass);
    wobbleFilter.setResonance (0.9f);

    for (auto* s : { &inGain, &outGain, &mixAmt, &bypassAmt, &drive })
        s->reset (fs, 0.03);

    phoneHp1.setHighPass (fs, 200.0, 0.707);
    phoneHp2.setHighPass (fs, 200.0, 0.707);
    phonePeak.setPeak (fs, 1500.0, 1.0, 4.0);
    phoneLp.setLowPass (fs, 7000.0, 0.707);
    f0Lp.setLowPass (fs, 150.0, 0.707);

    for (auto& c : ch)
    {
        c.hp20.setHighPass (fs, 20.0, 0.707);
        c.harmBand.setBandPass (fs, 180.0, 0.6);
        c.harmHigh.setHighPass (fs, 150.0, 0.707);
        c.clickHigh.setHighPass (fs, 1800.0, 0.707);
        c.kickHp.setHighPass (fs, 120.0, 0.707);
        c.kickLp.setLowPass (fs, 140.0, 0.707);
        c.kw1.setHighShelf (fs, 1681.974450955533, 0.7071752369554196, 3.999843853973347);
        c.kw2.setHighPass (fs, 38.13547087602444, 0.5003270373238773);
    }

    lufsBinLength = jmax (1, (int) (fs * 0.1));
    reset();
    if (! approximatelyEqual (sampleRate, preparedRate))   // some hosts re-prepare on every stop: keep the levels then
        forgetLevels();
    preparedRate = sampleRate;
}

void Engine::reset()
{
    for (auto& c : ch)
    {
        for (auto* b : { &c.hp20, &c.sub, &c.tiltLow, &c.tiltHigh, &c.filt1, &c.filt2, &c.harmBand,
                         &c.harmHigh, &c.clickHigh, &c.post, &c.kw1, &c.kw2,
                         &c.kickHp, &c.kickLp })
            b->reset();
        c.lowDelay.reset();
        c.osBypass.reset();
        c.protectDelay.reset();
        c.dryDelay.reset();
        c.padDelay.reset();
        c.dcX = c.dcY = c.hold = 0.0f;
        c.holdCount = 0;
        std::fill (c.pitchBuf.begin(), c.pitchBuf.end(), 0.0f);
        c.pitchWrite = 0;
        for (auto* b : { &c.octLow, &c.octSub, &c.octUpHigh })
            b->reset();
        c.octFlip = 1.0f;
        c.octWasNegative = false;
    }

    pitchDelay = oldDelay = (float) lookahead;
    vibrato = 0.0f;
    crossfade = 0;
    onsetCountdown = -1;
    noteSamples = 1 << 30;
    preFast = preSlow = 0.0f;
    preHold = 0;
    preRise = 0;
    lfoPhase = 0.0;
    sampleHold = 0.0f;
    wobbleFilter.reset();
    widthDelay.reset();
    protectEnvDelay.reset();
    for (auto* b : { &widthHp1, &widthHp2, &tunerLp1, &tunerLp2 })
        b->reset();
    chopBeat = 0.0;
    chopLevel = 1.0f;

    for (auto& os : oversamplers)
        if (os != nullptr)
            os->reset();

    cleanSplit.reset();
    monoSplit.reset();
    for (auto* b : { &phoneHp1, &phoneHp2, &phonePeak, &phoneLp })
        b->reset();

    envFast = envSlow = envGate = notePeak = scEnv = 0.0f;
    levelMeasurePeak = 0.0f;
    levelMeasureLeft = 0;
    f0Lp.reset();
    f0Last = 0.0f;
    f0Count = 0;
    dropPhase = 0.0;
    scopeCount = 0;
    scopeInMax = scopeOutMax = 0.0f;
    lengthGain = 1.0f;
    holdoff = 0;
    cSub = cTilt = -999.0f;
    cCutoff = cRes = cPost = cClean = cMono = -1.0f;
    lufsBins.fill (0.0);
    lufsBin = lufsCount = 0;
    lufsAcc = 0.0;
    shortTermLufs = -100.0f;
    inPeak = outPeak = 0.0f;
}

// The learned levels (auto level, dirt auto gain) survive reset(): hosts reset plugins when the
// transport stops, and forgetting them made the first hit after play too loud.
void Engine::forgetLevels()
{
    levelNote = 0.0f;
    levelGain = 1.0f;
    agGain = 1.0f;
    rmsPre = rmsPost = 0.0f;
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

float Engine::readPitch (Channel& c, float delay) const noexcept
{
    // cubic (Hermite) interpolated read, `delay` samples behind the last written sample
    const auto size = (int) c.pitchBuf.size();
    auto pos = (float) c.pitchWrite - 1.0f - delay;
    while (pos < 0.0f) pos += (float) size;
    const auto i1 = (int) pos;
    const auto t = pos - (float) i1;
    auto at = [&] (int i) { i %= size; if (i < 0) i += size; return c.pitchBuf[(size_t) i]; };
    const auto y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
    const auto c1 = 0.5f * (y2 - y0);
    const auto c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const auto c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

float Engine::lfoValue (int lfoShape, float phase) const noexcept
{
    switch (lfoShape)
    {
        case 1:  return 1.0f - 4.0f * std::abs (phase - 0.5f);           // triangle
        case 2:  return 1.0f - 2.0f * phase;                              // saw (down)
        case 3:  return phase < 0.5f ? 1.0f : -1.0f;                      // square
        case 4:  return sampleHold;                                       // sample & hold
        default: return std::sin (MathConstants<float>::twoPi * phase);   // sine
    }
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
    const auto padDelay = osMaxLatency - osLatency[(size_t) osIndex];

    // ---- pitch FX + wobble setup
    const auto pitchActive = p.pitchOn;
    const auto pitchRun = pitchActive;
    const auto kickLen = jmax (5.0f, p.kickLengthMs) * 0.001f;
    // measured on "spin" kit 808s: the hit starts ~3 octaves above the note and falls with ~15 ms
    const auto dropSweep = (float) fs * kickLen / 2.4f;         // how fast the kick falls to the note
    const auto dropOctaves = p.kickDrop * 3.2f;                 // starts up to 3.2 octaves above the note
    const auto kickTau = (float) fs * kickLen / 2.5f;
    const auto kickAttack = (float) fs * 0.0004f;
    const auto protectHold = (float) fs * kickLen;
    const auto biteGain = 6.0f + 60.0f * p.kickBite;       // hard: sharp edges = bright harmonics up into the kHz range
    const auto useProtect = p.kickProtect && p.dirtOn;
    const auto knockTau = jmax (0.001f, p.knockTimeMs * 0.001f / 3.0f);
    const auto diveDelayS = p.diveDelayMs * 0.001f;
    const auto diveTimeS = jmax (0.01f, p.diveTimeMs * 0.001f);
    const auto octActive = pitchActive && (p.octDown > 0.001f || p.octUp > 0.001f);
    const auto wobbleActive = p.wobbleOn && p.wobble > 0.001f;
    const auto wobblePitch = wobbleActive && (p.wobbleTarget == 0 || p.wobbleTarget == 3);
    const auto wobbleVolume = wobbleActive && (p.wobbleTarget == 1 || p.wobbleTarget == 3);
    const auto wobbleFilterOn = wobbleActive && (p.wobbleTarget == 2 || p.wobbleTarget == 3);
    static constexpr double cyclesPerBeat[] = { 0.5, 1.0, 1.5, 2.0, 3.0, 4.0 / 3.0, 4.0, 6.0, 8.0 };
    const auto cpb = cyclesPerBeat[jlimit (0, 8, p.wobbleRate)];
    const auto bpm = p.bpm > 20.0 ? p.bpm : 120.0;
    const auto lfoInc = bpm / 60.0 * cpb / fs;
    if (! p.wobbleRetrig && p.playing)
        lfoPhase = p.ppq * cpb - std::floor (p.ppq * cpb);   // lock to the DAW grid
    const auto fadeSamples = p.wobbleFadeMs * 0.001f * (float) fs;
    const auto vibratoLeak = 1.0f / (float) (0.3 * fs);
    const auto maxDelay = (float) (ch[0].pitchBuf.size() - 4);
    const auto semitoneToRate = std::log (2.0f) / 12.0f;

    // wobble pitch: keep the vibrato window centred so wide, slow wobbles never hit the lookahead limit
    const auto lfoHz = (float) (bpm / 60.0 * cpb);
    const auto vibratoCentre = wobblePitch
        ? jmin ((float) lookahead * 3.0f, p.wobble * (std::exp2 (5.0f / 12.0f) - 1.0f) / (MathConstants<float>::twoPi * lfoHz) * (float) fs)
        : 0.0f;

    // chop: tempo-synced gate, locked to the DAW grid while playing
    const auto chopActive = p.chopOn && p.chop > 0.001f;
    if (p.playing)
        chopBeat = p.ppq;
    const auto beatInc = bpm / 60.0 / fs;
    const auto chopCoef = 1.0f - onePole (0.0005 + p.chopSmooth * 0.025, fs);

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
    const auto levelCoef = 1.0f - onePole (0.002, fs);
    const auto punchAttack = (float) (fs * 0.001), punchDecay = (float) (fs * 0.035);
    float clipRatio = 1.0f;

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
            float detectIn = 0.0f;

            for (int c = 0; c < numCh; ++c)
            {
                dryBuf.setSample (c, i, data[c][i]);
                x[c] = data[c][i] * g;     // no input filter: any phase shift here smears the attack of a finished 808
                detectIn = jmax (detectIn, std::abs (x[c]));
            }


            // ---- tuner feed (mono input, low-passed and decimated)
            {
                const auto mono = numCh > 1 ? 0.5f * (x[0] + x[1]) : x[0];
                const auto lp = tunerLp2.process (tunerLp1.process (mono));
                if (++tunerCount >= tunerFactor)
                {
                    tunerCount = 0;
                    const auto w = tunerWrite.load (std::memory_order_relaxed);
                    tunerRing[(size_t) w] = lp;
                    tunerWrite.store ((w + 1) % tunerSize, std::memory_order_release);
                }
            }

            // ---- chop gate
            if (chopActive)
            {
                static constexpr int gross[16]   = { 1,0,1,1, 0,1,0,1, 1,0,1,0, 1,0,1,1 };
                static constexpr int stutter[16] = { 1,1,1,1, 1,0,1,0, 1,1,1,1, 0,1,0,1 };
                const auto beat = chopBeat;
                const auto beatInBar = beat - 4.0 * std::floor (beat / 4.0);
                double stepsPerBeat = 4.0;
                bool stepOn = true;
                switch (p.chopPattern)
                {
                    case 0: stepsPerBeat = 2.0; break;
                    case 1: stepsPerBeat = 4.0; break;
                    case 2: stepsPerBeat = 6.0; break;
                    case 3: stepsPerBeat = 8.0; break;
                    case 4: stepsPerBeat = beatInBar < 2.0 ? 2.0 : (beatInBar < 3.0 ? 4.0 : (beatInBar < 3.5 ? 8.0 : 16.0)); break;
                    case 5: stepOn = gross[(int) (beatInBar * 4.0) & 15] != 0; break;
                    default: stepOn = stutter[(int) (beatInBar * 4.0) & 15] != 0; break;
                }
                const auto stepPos = beat * stepsPerBeat;
                const auto inStep = (float) (stepPos - std::floor (stepPos));
                const auto open = stepOn && inStep < jlimit (0.05f, 0.95f, p.chopGate);
                const auto target = open ? 1.0f : 1.0f - p.chop;
                chopLevel += (target - chopLevel) * chopCoef;
                chopGain[(size_t) i] = chopLevel;
            }
            else
            {
                chopLevel = 1.0f;
                chopGain[(size_t) i] = 1.0f;
            }
            chopBeat += beatInc;

            // ---- note onsets on the incoming signal; they reach the read head `lookahead` samples later
            preFast = follow (preFast, detectIn, fastA, fastR);
            preSlow = follow (preSlow, detectIn, slowA, slowR);
            if (preHold > 0) --preHold;
            // how long the level has been rising: the note really started that long before it was detected
            if (preFast <= preSlow * 1.1f || preFast < 0.001f) preRise = 0;
            else if (preRise < lookahead) ++preRise;

            if (preFast > 0.003f && preFast > preSlow * 1.6f && preHold == 0)
            {
                // the read head reaches the true start of the note (plus 0.5 ms margin), not the detection point
                onsetCountdown = jmax (0, lookahead - preRise - (int) (fs * 0.0005));
                preHold = (int) (fs * 0.06);
                levelMeasureLeft = lookahead;
                levelMeasurePeak = 0.0f;
            }

            // ---- auto level: every 808 hits the processing at about -6 dBFS peak, so presets and KILL
            // sound the same on quiet and loud samples. Each note is measured during the lookahead
            // (before it is heard) and the gain is applied after the lookahead buffer, so it is set
            // before the note arrives. Silence (transport stopped) keeps the last level.
            if (levelMeasureLeft > 0)
            {
                levelMeasurePeak = jmax (levelMeasurePeak, detectIn);
                if (--levelMeasureLeft == 0 && levelMeasurePeak > 0.01f)
                    levelNote = (levelNote <= 0.0f || levelMeasurePeak > levelNote)
                                    ? levelMeasurePeak                                  // louder: follow at once
                                    : levelNote * 0.7f + levelMeasurePeak * 0.3f;       // quieter: over a few notes
            }
            {
                const auto target = (p.autoLevel && levelNote > 0.0f) ? jlimit (0.25f, 8.0f, 0.5f / levelNote) : 1.0f;
                levelGain += (target - levelGain) * levelCoef;
            }

            if (onsetCountdown >= 0 && --onsetCountdown < 0)
            {
                // new note reaches the read head: restart the pitch envelope, crossfade from the old position
                oldDelay = pitchDelay + vibrato;
                crossfade = crossfadeLength;
                pitchDelay = (float) lookahead;
                vibrato = 0.0f;
                noteSamples = 0;
                if (p.wobbleRetrig)
                    lfoPhase = 0.0;
            }
            else if (noteSamples < (1 << 30))
            {
                ++noteSamples;
            }

            // ---- wobble LFO
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0)
            {
                lfoPhase -= std::floor (lfoPhase);
                sampleHold = random.nextFloat() * 2.0f - 1.0f;
            }
            const auto fade = fadeSamples > 1.0f ? jmin (1.0f, (float) noteSamples / fadeSamples) : 1.0f;
            const auto wobbleDepth = wobbleActive ? p.wobble * fade : 0.0f;
            const auto lfo = lfoValue (p.wobbleShape, (float) lfoPhase);
            if (wobbleVolume)
            {
                const auto wg = 1.0f - wobbleDepth * (0.5f - 0.5f * lfo);
                wobbleGain[(size_t) i] = wg * wg;
            }
            else
            {
                wobbleGain[(size_t) i] = 1.0f;
            }
            wobbleCutoff[(size_t) i] = wobbleFilterOn ? 16000.0f * std::exp2 (-wobbleDepth * 8.0f * (0.5f - 0.5f * lfo)) : 20000.0f;

            // ---- pitch envelope -> vari-speed read position
            float semis = 0.0f;
            if (pitchRun)
            {
                const auto t = (float) noteSamples / (float) fs;
                if (pitchActive && p.knock > 0.0f)
                    semis += p.knock * std::exp (-t / knockTau);
                if (pitchActive && p.dive < 0.0f && t > diveDelayS)
                {
                    const auto k = jmin (1.0f, (t - diveDelayS) / diveTimeS);
                    semis += p.dive * k * k * (3.0f - 2.0f * k);
                }
            }
            pitchDelay = jlimit (2.0f, maxDelay, pitchDelay + 1.0f - std::exp2 (semis / 12.0f));
            if (! pitchRun && crossfade == 0)
                pitchDelay = (float) lookahead;

            if (wobblePitch)
                vibrato -= wobbleDepth * 5.0f * lfo * semitoneToRate;     // integrate the pitch deviation (up to +-5 st)
            vibrato -= (vibrato - vibratoCentre) * vibratoLeak;
            const auto readDelay = jlimit (2.0f, maxDelay, pitchDelay + jlimit (-(float) lookahead + 4.0f, (float) lookahead * 4.0f, vibrato));
            const auto xfade = crossfade > 0 ? (float) crossfade / (float) crossfadeLength : 0.0f;
            if (crossfade > 0) --crossfade;

            float detect = 0.0f;
            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                st.pitchBuf[(size_t) st.pitchWrite] = x[c];
                if (++st.pitchWrite >= (int) st.pitchBuf.size()) st.pitchWrite = 0;

                auto v = readPitch (st, readDelay);
                if (xfade > 0.0f)
                    v = v * (1.0f - xfade) + readPitch (st, jlimit (2.0f, maxDelay, oldDelay)) * xfade;
                v *= levelGain;

                if (octActive)
                {
                    // octave down: divide-by-two flip-flop on the fundamental; octave up: full-wave rectifier
                    const auto fund = st.octLow.process (v);
                    if (fund < -1.0e-4f) st.octWasNegative = true;
                    else if (fund > 1.0e-4f && st.octWasNegative)
                    {
                        st.octWasNegative = false;
                        st.octFlip = -st.octFlip;
                    }
                    const auto sub = st.octSub.process (fund * st.octFlip) * 1.8f;
                    const auto up = st.octUpHigh.process (std::abs (fund)) * 1.6f;
                    v += p.octDown * sub + p.octUp * up;
                }

                x[c] = v;
                detect = jmax (detect, std::abs (v));
            }
            if (crossfade == 0)
                oldDelay = readDelay;

            // ---- pitch of the 808 (zero crossings of the low-passed signal), for the KICK DROP layer
            {
                const auto mono = f0Lp.process (numCh > 1 ? 0.5f * (x[0] + x[1]) : x[0]);
                ++f0Count;
                if (f0Last <= 0.0f && mono > 0.0f && std::abs (mono) + std::abs (f0Last) > 1.0e-3f)
                {
                    const auto hz = (float) fs / (float) f0Count;
                    if (hz > 25.0f && hz < 200.0f)
                        f0Est += (hz - f0Est) * 0.3f;
                    f0Count = 0;
                }
                f0Last = mono;
            }

            // ---- KICK DROP: the note is replaced by a sine that falls from high above into it (the "spin" hit),
            // then handed back to the note. The phase of the note is read from the lookahead buffer, so the
            // falling sine lands exactly in phase and the hand-back is seamless.
            if (p.kickDrop > 0.001f && noteSamples < (1 << 30))
            {
                const auto tn = (float) noteSamples;
                const auto cycle = (double) f0Est / fs;
                if (noteSamples == 0)
                {
                    dropPhase = 0.0;
                    dropZcSeen = false;
                    // first rising zero crossing of the note ahead of the read head -> phase of the note now
                    const auto& buf = ch[0].pitchBuf;
                    const auto size = (int) buf.size();
                    const auto readPos = ch[0].pitchWrite - (int) readDelay;
                    int ahead = -1;
                    for (int k = 1; k < (int) readDelay - 1; ++k)
                    {
                        const auto a = buf[(size_t) (((readPos + k - 1) % size + size) % size)];
                        const auto b = buf[(size_t) (((readPos + k) % size + size) % size)];
                        if (a <= 0.0f && b > 0.0f) { ahead = k; break; }
                    }
                    dropNotePhase = ahead > 0 ? -(double) ahead * cycle : 0.0;
                }
                else
                {
                    dropNotePhase += cycle;
                }
                if (f0Count == 0 && noteSamples > 0)
                    dropZcSeen = true;          // from the first measured crossing on, follow the note directly
                const auto notePhase = dropZcSeen ? (double) f0Count * cycle + 0.05 : dropNotePhase;

                const auto octaves = dropOctaves * std::exp (-tn / dropSweep);
                dropPhase += cycle * std::exp2 (octaves);

                auto err = notePhase - dropPhase;
                err -= std::round (err);
                const auto lock = jlimit (0.0f, 1.0f, 1.0f - octaves);           // locks in while the fall ends
                dropPhase += err * 0.02 * lock;
                dropPhase -= std::floor (dropPhase);

                const auto fadeIn = tn < kickAttack ? tn / kickAttack : 1.0f;
                const auto handBack = jlimit (0.0f, 1.0f, (octaves - 0.08f) / 0.35f); // 1 while falling, 0 once back on the note
                const auto w = handBack;                                           // DROP sets the depth, not a blend
                const auto ref = levelNote > 0.0f ? levelNote * levelGain : 0.5f;
                const auto boost = 1.0f + 0.35f * std::exp (-tn / dropSweep);    // the hit is a bit louder than the body
                const auto layer = (float) std::sin (MathConstants<double>::twoPi * dropPhase) * ref * boost * fadeIn;
                for (int c = 0; c < numCh; ++c)
                    x[c] = x[c] * (1.0f - w) + layer * w;
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

            // punch envelope starts at every note onset (found in the lookahead), so every hit gets the
            // same punch, no matter whether the previous note is still ringing or there was silence
            float transient = 0.0f, kickEnv = 0.0f, protect = 0.0f;
            if (noteSamples < (1 << 30))
            {
                const auto tn = (float) noteSamples;
                transient = tn < punchAttack ? tn / punchAttack : std::exp (-(tn - punchAttack) / punchDecay);
                kickEnv = tn < kickAttack ? tn / kickAttack : std::exp (-(tn - kickAttack) / kickTau);
                protect = tn < protectHold ? 1.0f : std::exp (-(tn - protectHold) / (0.5f * protectHold));
            }
            protectEnv[(size_t) i] = useProtect ? protect : 0.0f;

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

                // ---- KICK: add a hit made from the note itself, or soften the hit that is there
                {
                    const auto bite = st.kickHp.process (std::tanh (v * biteGain));   // harmonics of the note
                    const auto soft = st.kickLp.process (v);
                    if (p.kick > 0.001f)
                    {
                        const auto k = p.kick * kickEnv;
                        v = v * (1.0f + 0.45f * k) + bite * k * (0.3f + 1.2f * p.kickBite);
                    }
                    else if (p.kick < -0.001f)
                    {
                        const auto k = -p.kick * protect;          // soften over the whole hit, not only its first ms
                        v = (v + (soft - v) * k) * (1.0f - 0.35f * k * kickEnv);
                    }
                }

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

                // complementary split (high = input - low): low + high is exactly the input again,
                // so the split alone never changes the waveform
                float low = 0.0f, high = 0.0f;
                cleanSplit.processSample (c, v, low, high);
                if (p.cleanLow && p.dirtOn)
                {
                    high = v - low;
                }
                else
                {
                    low = 0.0f;
                    high = v;
                }

                lowBuf.setSample (c, i, st.lowDelay.process (low, osMaxLatency));
                preBuf.setSample (c, i, high);
                data[c][i] = high;
            }
        }

        // ================= stage B: oversampled dirt on the high band
        {
            dsp::AudioBlock<float> block (data, (size_t) numCh, (size_t) n);
            auto& os = *oversamplers[(size_t) osIndex];

            if (! p.dirtOn)
            {
                // nothing to distort: skip the oversampling filters (their phase would bend the attack),
                // an integer delay keeps the timing identical
                for (int c = 0; c < numCh; ++c)
                    for (int j = 0; j < n; ++j)
                        data[c][j] = ch[(size_t) c].osBypass.process (data[c][j], osLatency[(size_t) osIndex]);
            }
            else
            {
                auto up = os.processSamplesUp (block);
                const auto factor = (int) os.getOversamplingFactor();
                const auto upN = (int) up.getNumSamples();

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

                os.processSamplesDown (block);
            }
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
            const auto protectNow = protectEnvDelay.process (protectEnv[(size_t) i], osMaxLatency);
            // clean hit at the level of the dirty signal around it, so PROTECT never makes the hit quieter
            const auto protectMatch = jlimit (0.25f, 8.0f, std::sqrt ((rmsPost + 1.0e-9f) / (rmsPre + 1.0e-9f)));

            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[(size_t) c];
                auto v = st.padDelay.process (data[c][i], padDelay);

                // PROTECT: the hit of every note skips the dirt (clean signal lined up with the dirt path)
                const auto clean = st.protectDelay.process (preBuf.getSample (c, i), osMaxLatency);
                if (protectNow > 0.0f)
                    v += (clean * protectMatch - v) * protectNow;

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

            // in silence (transport stopped) keep the last ratio, so the first hit is compensated like every other
            if (preSq > 1.0e-6f)
            {
                rmsPre = preSq + (rmsPre - preSq) * rmsCoef;
                rmsPost = postSq + (rmsPost - postSq) * rmsCoef;
            }
            const auto agTarget = (p.autoGain && p.dirtOn && rmsPost > 1.0e-9f)
                                      ? jlimit (0.1f, 4.0f, std::sqrt ((rmsPre + 1.0e-9f) / (rmsPost + 1.0e-9f)))
                                      : 1.0f;
            agGain += (agTarget - agGain) * agCoef;

            const auto og = outGain.getNextValue();
            const auto m = mixAmt.getNextValue();
            const auto b = bypassAmt.getNextValue();

            if (wobbleFilterOn)
                wobbleFilter.setCutoffFrequency (jmin (wobbleCutoff[(size_t) i], (float) fs * 0.45f));

            for (int c = 0; c < numCh; ++c)
            {
                auto v = (y[c] * agGain + lowBuf.getSample (c, i)) * duckGain * wobbleGain[(size_t) i] * chopGain[(size_t) i];
                if (wobbleFilterOn)
                    v = wobbleFilter.processSample (c, v);
                y[c] = v;
            }

            // mono below: only the low part of the side signal is removed, the mid is untouched
            // (a mono 808 passes bit-exact, no crossover phase shift on the attack)
            if (numCh == 2 && p.monoBelow >= 1.0f)
            {
                const auto mid = 0.5f * (y[0] + y[1]);
                const auto side = 0.5f * (y[0] - y[1]);
                float sideLow, sideHigh;
                monoSplit.processSample (0, side, sideLow, sideHigh);
                const auto keep = side - sideLow;
                y[0] = mid + keep;
                y[1] = mid - keep;
            }

            // width: decorrelated copy of the upper band added to L and subtracted from R
            // (the sum is unchanged, so mono playback and the sub stay intact)
            if (numCh == 2 && p.width > 0.001f)
            {
                const auto mid = 0.5f * (y[0] + y[1]);
                const auto high = widthHp2.process (widthHp1.process (mid));
                const auto side = widthDelay.process (high, (int) (fs * 0.011)) * p.width * 0.8f;
                y[0] += side;
                y[1] -= side;
            }

            for (int c = 0; c < numCh; ++c)
            {
                // clipper last, so the ceiling holds (the mono split can overshoot on clipped waves)
                auto v = y[c];
                if (effClip > 0.001f)
                {
                    const auto before = std::abs (v);
                    const auto u = v * (1.0f + effClip) / ceilingGain;
                    v = ceilingGain * ((1.0f - effClip) * std::tanh (u) + effClip * jlimit (-1.0f, 1.0f, u));
                    if (before > 1.0e-3f && std::abs (v) > 1.0e-6f)
                        clipRatio = jmax (clipRatio, before / std::abs (v));
                }

                const auto dry = ch[(size_t) c].dryDelay.process (dryBuf.getSample (c, i), latency);
                v *= og;
                v = dry + (v - dry) * m;
                v = v + (dry - v) * b;
                y[c] = std::isfinite (v) ? v : 0.0f;
                scopeInMax = jmax (scopeInMax, std::abs (dry));
                scopeOutMax = jmax (scopeOutMax, std::abs (y[c]));
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

    const auto reduction = Decibels::gainToDecibels (clipRatio);
    if (reduction > clipDb.load()) clipDb = reduction;
}
