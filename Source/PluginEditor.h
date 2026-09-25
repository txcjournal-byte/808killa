#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Knobs: red value arc + red pointer drawn over the knob caps in the background
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos,
                           float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

//==============================================================================
// Readout box ("28", "0.0 dB", ...)
class ValueBox : public juce::Component
{
public:
    explicit ValueBox (std::function<juce::String()> textFn) : getText (std::move (textFn))
    {
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics&) override;
    juce::Font font { juce::FontOptions{} };

private:
    std::function<juce::String()> getText;
};

//==============================================================================
// Rocker switch / limiter button: only the LED is drawn, the body is in the background
class LedButton : public juce::Button
{
public:
    LedButton (const juce::String& name, juce::Point<float> ledCentreLocal, float ledSize);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    juce::Point<float> ledCentre;
    float size;
};

//==============================================================================
// TYPE / MODE selector strips
class SegmentSelector : public juce::Component
{
public:
    SegmentSelector (juce::RangedAudioParameter& param, juce::StringArray names,
                     std::vector<int> edgesX, int top, int bottom, juce::Font font);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> segment (int index) const;

    juce::StringArray names;
    std::vector<int> edges;
    int top, bottom;
    juce::Font font { juce::FontOptions{} };
    int selected = 0;
    juce::ParameterAttachment attachment;

    static constexpr int margin = 14;
};

//==============================================================================
// MASTER meters (IN L/R, OUT L/R)
class MasterMeter : public juce::Component
{
public:
    MasterMeter() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void setLevels (const float newLevelsDb[4]);

private:
    float levels[4] { -100.0f, -100.0f, -100.0f, -100.0f };
};

//==============================================================================
// Everything is laid out in the 1536 x 1024 coordinate space of the artwork
class Canvas : public juce::Component
{
public:
    Canvas();
    void paint (juce::Graphics&) override;

private:
    juce::Image background;
};

//==============================================================================
class K808Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit K808Editor (K808Processor&);
    ~K808Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 1536;
    static constexpr int designHeight = 1024;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
        std::unique_ptr<ValueBox> box;
    };

    Knob& addKnob (const char* paramID, juce::Point<int> centre, float arcRadius,
                   float pointerIn, float pointerOut, bool small, juce::Rectangle<int> boxBounds,
                   std::function<juce::String (double)> format);
    Knob& addFader (const char* paramID, juce::Rectangle<int> bounds, juce::Rectangle<int> boxBounds,
                    std::function<juce::String (double)> format);
    void addLed (const char* paramID, juce::Rectangle<int> bounds, juce::Point<float> led, float ledSize);

    K808Processor& processor;
    KnobLookAndFeel lnf;
    Canvas canvas;
    juce::Font monoFont { juce::FontOptions{} }, buttonFont { juce::FontOptions{} };

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<LedButton>> leds;
    std::vector<std::unique_ptr<ButtonAttachment>> ledAttachments;
    std::unique_ptr<SegmentSelector> typeSelector, modeSelector;

    MasterMeter meter;
    std::unique_ptr<ValueBox> inReadout, outReadout;
    float meterDb[4] { -100.0f, -100.0f, -100.0f, -100.0f };
    float readoutDb[2] { -100.0f, -100.0f };
    int readoutCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K808Editor)
};
