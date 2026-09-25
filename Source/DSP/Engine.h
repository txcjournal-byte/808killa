#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Filters.h"

// Snapshot of all parameter values for one block
struct EngineParams
{
    int style = 0;
    float kill = 0.0f, inGainDb = 0.0f;
    bool bypass = false;

    bool pitchOn = true;
    float knock = 0.0f, knockTimeMs = 30.0f, dive = 0.0f, diveTimeMs = 250.0f, diveDelayMs = 150.0f;
    float octDown = 0.0f, octUp = 0.0f;

    bool wobbleOn = true;
    float wobble = 0.0f, wobbleFadeMs = 0.0f;
    int wobbleTarget = 0, wobbleRate = 6, wobbleShape = 0;
    bool wobbleRetrig = true;

    // host transport (tempo sync)
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;

    bool shapeOn = true;
    float punch = 0.0f, click = 0.0f, length = 0.0f;

    bool toneOn = true;
    float subDb = 0.0f, harmonics = 0.0f;
    bool filterOn = false;
    float cutoff = 20000.0f, resonance = 0.1f;
    int slope = 1;
    float tiltDb = 0.0f;

    bool dirtOn = true;
    int dirtMode = 0;
    float drive = 0.0f, dirtMix = 1.0f;
    bool autoGain = true;
    int oversampling = 0;
    bool cleanLow = false;
    float cleanFreq = 100.0f, crushBits = 24.0f, postFreq = 20000.0f;

    bool duckOn = true;
    float duck = 0.0f, duckReleaseMs = 120.0f, duckShape = 0.5f;

    float clipper = 0.0f, ceilingDb = 0.0f, monoBelow = 0.0f, outGainDb = 0.0f, mix = 100.0f;
    bool phone = false;
};

// How the KILL macro pushes each style (added on top of the style's own settings)
struct KillMapping { float drive, punch, subDb, clipper; };
KillMapping killMappingFor (int style) noexcept;

class Engine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // buffer: main in/out (1 or 2 channels), sidechain may be null
    void process (juce::AudioBuffer<float>& buffer, int numChannels,
                  const juce::AudioBuffer<float>* sidechain, const EngineParams& p);

    int getLatencySamples() const noexcept { return latency; }
    static double tailSeconds (const EngineParams& p) noexcept;

    // metering (audio thread writes, UI reads)
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f }, shortTermLufs { -100.0f };
    std::atomic<bool> sidechainActive { false };

private:
    struct Channel
    {
        Biquad hp20, sub, tiltLow, tiltHigh, filt1, filt2, harmBand, harmHigh, clickHigh, post, kw1, kw2;
        DelayLine lowDelay, dryDelay, padDelay;
        std::vector<float> pitchBuf;
        int pitchWrite = 0;
        Biquad octLow, octSub, octUpHigh;
        float octFlip = 1.0f;
        bool octWasNegative = false;
        float dcX = 0.0f, dcY = 0.0f, hold = 0.0f;
        int holdCount = 0;
    };

    void updateFilters (const EngineParams& p, float subDb);
    float readPitch (Channel&, float delay) const noexcept;
    float lfoValue (int shape, float phase) const noexcept;
    float shape (int mode, float x) const noexcept;

    double fs = 44100.0;
    int maxBlock = 512;
    int latency = 0;            // total reported latency = lookahead + oversampling
    int osMaxLatency = 0;
    int lookahead = 0;          // pitch FX lookahead (lets KNOCK read ahead of the note)
    std::array<int, 3> osLatency {};
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    std::array<Channel, 2> ch;
    juce::dsp::LinkwitzRileyFilter<float> cleanSplit, monoSplit;
    Biquad phoneHp1, phoneHp2, phonePeak, phoneLp;

    juce::AudioBuffer<float> dryBuf, lowBuf, preBuf;
    std::vector<float> driveGain, wobbleGain, wobbleCutoff;
    juce::dsp::StateVariableTPTFilter<float> wobbleFilter;

    // pitch FX state
    float pitchDelay = 0.0f, vibrato = 0.0f, oldDelay = 0.0f;
    int crossfade = 0, crossfadeLength = 1, onsetCountdown = -1, noteSamples = 1 << 30, preHold = 0;
    float preFast = 0.0f, preSlow = 0.0f;

    // wobble LFO state
    double lfoPhase = 0.0;
    float sampleHold = 0.0f;
    juce::Random random { 808 };

    // detection
    float envFast = 0.0f, envSlow = 0.0f, envGate = 0.0f, notePeak = 0.0f, lengthGain = 1.0f;
    int holdoff = 0;
    float scEnv = 0.0f;
    int scSilentSamples = 1 << 30;
    float rmsPre = 0.0f, rmsPost = 0.0f, agGain = 1.0f;

    juce::SmoothedValue<float> inGain, outGain, mixAmt, bypassAmt, drive;

    // cached filter settings
    float cSub = -999.0f, cTilt = -999.0f, cCutoff = -1.0f, cRes = -1.0f, cPost = -1.0f, cClean = -1.0f, cMono = -1.0f;
    float smoothCutoff = 20000.0f;

    // loudness (K-weighted, 3 s short-term from 100 ms bins)
    std::array<double, 30> lufsBins {};
    int lufsBin = 0, lufsCount = 0, lufsBinLength = 4410;
    double lufsAcc = 0.0;
};
