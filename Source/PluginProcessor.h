#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace ParamIDs
{
    inline constexpr auto punch    = "punch";
    inline constexpr auto sub      = "sub";
    inline constexpr auto distort  = "distort";
    inline constexpr auto clip     = "clip";
    inline constexpr auto shortEnv = "short";
    inline constexpr auto boost    = "boost";
    inline constexpr auto hardClip = "hardclip";
    inline constexpr auto grit     = "grit";
    inline constexpr auto lowMono  = "lowmono";
    inline constexpr auto cook     = "cook";
    inline constexpr auto type     = "type";
    inline constexpr auto mode     = "mode";
    inline constexpr auto mix      = "mix";
    inline constexpr auto output   = "output";
    inline constexpr auto limiter  = "limiter";
    inline constexpr auto ceiling  = "ceiling";
}

// Simple transposed direct form II biquad (RBJ cookbook coefficients)
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() noexcept { z1 = z2 = 0.0f; }

    float process (float x) noexcept
    {
        const auto y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setLowShelf (double fs, double freq, double q, double gainDb);
    void setLowPass  (double fs, double freq, double q);
    void setHighPass (double fs, double freq, double q);
    void setBypass() noexcept { b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f; }
};

class K808Processor : public juce::AudioProcessor
{
public:
    K808Processor();
    ~K808Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Peak levels for the MASTER meters (written by audio thread, read+reset by UI)
    std::atomic<float> inPeak[2]  { 0.0f, 0.0f };
    std::atomic<float> outPeak[2] { 0.0f, 0.0f };

    static const juce::StringArray typeNames;
    static const juce::StringArray modeNames;

private:
    struct ChannelState
    {
        float envFast = 0.0f, envSlow = 0.0f, envGate = 0.0f, gateGain = 1.0f;
        Biquad subShelf, typeFilter, modeLowPass, modeShelf;
        float dcX = 0.0f, dcY = 0.0f;
        float holdValue = 0.0f;
        int holdCounter = 0;
    };

    float shape (int type, float x) const noexcept;
    void updateFilters (float subAmount, int type, int mode);

    std::array<ChannelState, 2> channels;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::LinkwitzRileyFilter<float> monoCrossover;

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> dryDelayBuffer;
    int dryDelayWritePos = 0;
    int latency = 0;

    juce::SmoothedValue<float> driveSmoothed, clipSmoothed, mixSmoothed, outGainSmoothed;
    float limiterGain = 1.0f;

    double currentSampleRate = 44100.0;
    float lastSub = -1.0f;
    int lastType = -1, lastMode = -1;

    // cached raw parameter pointers
    std::atomic<float>* pPunch = nullptr;
    std::atomic<float>* pSub = nullptr;
    std::atomic<float>* pDistort = nullptr;
    std::atomic<float>* pClip = nullptr;
    std::atomic<float>* pShort = nullptr;
    std::atomic<float>* pBoost = nullptr;
    std::atomic<float>* pHardClip = nullptr;
    std::atomic<float>* pGrit = nullptr;
    std::atomic<float>* pLowMono = nullptr;
    std::atomic<float>* pCook = nullptr;
    std::atomic<float>* pType = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pLimiter = nullptr;
    std::atomic<float>* pCeiling = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K808Processor)
};
