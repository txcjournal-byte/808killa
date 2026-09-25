#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Presets.h"
#include "DSP/Engine.h"

class K808Processor : public juce::AudioProcessor
{
public:
    K808Processor();
    ~K808Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    EngineParams readParams() const;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    Engine engine;

private:
    // cached raw parameter pointers (lookups by name would allocate on the audio thread)
    struct Raw { std::atomic<float>* style, *kill, *inGain, *bypass, *shapeOn, *punch, *punchClick, *length, *toneOn, *sub, *harmonics, *filterOn, *cutoff, *resonance, *slope, *tilt, *dirtOn, *dirtMode, *dirt, *dirtMix, *autoGain, *oversample, *cleanLow, *cleanFreq, *crushBits, *postFilter, *duckOn, *duck, *duckRel, *duckShape, *clipper, *ceiling, *monoBelow, *outGain, *mix, *phone, *pitchOn, *knock, *knockTime, *dive, *diveTime, *diveDelay, *octDown, *octUp, *wobbleOn, *wobble, *wobbleTarget, *wobbleRate, *wobbleShape, *wobbleFade, *wobbleRetrig, *chopOn, *chop, *chopPattern, *chopGate, *chopSmooth, *width; } raw {};
    juce::AudioProcessorParameter* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K808Processor)
};
