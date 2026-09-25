#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct PresetInfo
{
    juce::String name;
    juce::String category;     // Styles / Clean / Dirty / Pitch / FX / User
    bool factory = true;
    juce::File file;           // user presets only
};

// Factory presets live in the binary, user presets as .808k (JSON) files.
// All methods must be called on the message thread.
class PresetManager : private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);
    ~PresetManager() override;

    static constexpr int formatVersion = 1;
    static juce::File userFolder();

    const juce::Array<PresetInfo>& getPresets() const noexcept { return presets; }
    void rescan();

    void load (int index);
    void loadNext (int delta);
    void revert();
    void init();
    bool saveUser (const juce::String& name);

    juce::String getCurrentName() const;
    int getCurrentIndex() const;
    bool isModified() const noexcept { return modified.load(); }

    // restore name after the host loaded a project
    void restoreFromState();

private:
    void parameterChanged (const juce::String&, float) override;
    void applyValues (const juce::NamedValueSet& values);
    juce::var toJson (const juce::String& name) const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<PresetInfo> presets;
    std::atomic<bool> modified { false };
    std::atomic<bool> loading { false };
};
