#pragma once

#include "PluginProcessor.h"
#include "UI/Look.h"
#include "UI/Widgets.h"

//==============================================================================
// Where things sit on the artwork (design space 1536 x 1024, Resources/background.jpg)
namespace Layout
{
    inline const juce::Rectangle<int> leftPanel  { 56, 320, 198, 518 };
    inline const juce::Rectangle<int> rightPanel { 1250, 320, 230, 518 };
    inline const juce::Rectangle<int> bottomBar  { 396, 852, 688, 80 };
    inline const juce::Rectangle<int> head       { 470, 180, 540, 668 };   // animated part of the artwork
    inline const juce::Rectangle<int> mouth      { 568, 402, 330, 258 };
    inline const juce::Rectangle<int> jaw        { 552, 650, 368, 162 };   // Resources/jaw.png
    inline const juce::Rectangle<int> advanced   { 188, 79, 1160, 866 };
}

//==============================================================================
// The head: drag the jaw down = KILL, the mouth shows the waveform (input grey, output red)
class HeadView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit HeadView (K808Processor&);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // called by the editor timer
    void tick (float outLevel, bool phoneOn);

private:
    bool hitsJaw (juce::Point<float>) const;
    float jawDrop() const noexcept { return kill * 44.0f + beat * 7.0f; }

    K808Processor& processor;
    juce::RangedAudioParameter& killParam;
    juce::ParameterAttachment killAttachment;
    juce::Image background, jawImage;

    float kill = 0.0f, beat = 0.0f, slowLevel = 0.0f, dragStart = 0.0f;
    bool hover = false, dragging = false, phoneOn = false;
    std::vector<float> scopeIn, scopeOut;
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

//==============================================================================
// Left panel: IN / OUT meters, peak, loudness, clipper reduction, tuner
class MasterPanel : public juce::Component
{
public:
    explicit MasterPanel (K808Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setValues (float peakDb, float lufs, float clipDb, const juce::String& note, const juce::String& key);

    UI::Meter meter;

private:
    UI::LedToggle autoLevel, phone;
    juce::String values[5];
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

//==============================================================================
// Preset table: categories on the left, presets on the right, stars for favourites
class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser (K808Processor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void open();
    void showCategory (const juce::String& category);
    std::function<void()> onChange, onSaveAs;

private:
    struct Star : juce::Button
    {
        Star() : Button ("star") {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        bool on = false;
    };

    void rebuild();

    K808Processor& processor;
    juce::String category;
    juce::StringArray categoryNames;
    juce::OwnedArray<UI::FlatButton> categoryButtons, presetButtons, actionButtons;
    juce::OwnedArray<Star> stars;
    UI::FlatButton closeButton { "X" };
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

//==============================================================================
// Every parameter, sorted in tabs ("inside the head")
class AdvancedPage : public juce::Component
{
public:
    explicit AdvancedPage (K808Processor&);
    void paint (juce::Graphics&) override;
    void showTab (int index);

    UI::FlatButton closeButton { "CLOSE" };

private:
    struct Tab
    {
        juce::String name;
        juce::String sectionParam;                 // on/off switch (may be empty)
        juce::String note;                         // text shown on the tab
        std::vector<juce::Component*> controls;
    };

    template <typename T, typename... Args>
    T& add (int tab, juce::Rectangle<int> bounds, Args&&... args);
    UI::Knob& knob (int tab, int col, int row, const juce::String& id, const juce::String& label,
                    const juce::String& tip, bool bipolar = false);

    K808Processor& processor;
    std::vector<Tab> tabs;
    juce::OwnedArray<UI::FlatButton> tabButtons;
    juce::OwnedArray<juce::Component> owned;
    std::unique_ptr<UI::LedToggle> sectionToggles[6];
    UI::FlatButton resetButton { "RESET" }, openFolderButton { "OPEN PRESETS FOLDER" };
    int current = 0;
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

class AdvancedOverlay : public juce::Component
{
public:
    explicit AdvancedOverlay (K808Processor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    AdvancedPage page;

private:
    juce::Image plaster;
};

//==============================================================================
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
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

    static constexpr int designWidth = 1536;
    static constexpr int designHeight = 1024;

private:
    void timerCallback() override;
    void useSoftwareRenderer();
    void savePresetAs();
    void refreshPresetLabel();
    void updateTuner();

    K808Processor& processor;
    Look::LookAndFeel lnf;
    juce::TooltipWindow tooltips { nullptr, 600 };
    Canvas canvas;

    HeadView head;
    MasterPanel master;
    UI::Knob punch, sub, heat, tail, output, mix;
    UI::FlatButton prevButton { "<" }, nextButton { ">" }, presetButton { "" }, saveButton { "SAVE" },
                   abButton { "A" }, editButton { "EDIT" };
    PresetBrowser browser;
    AdvancedOverlay advanced;

    float meterDb[2] { -100.0f, -100.0f };
    float peakHoldDb = -100.0f, clipHoldDb = 0.0f;
    int peakHoldTicks = 0, clipHoldTicks = 0;
    juce::String shownPreset;
    bool shownModified = false;

    std::unique_ptr<juce::AlertWindow> saveDialog;

    // tuner
    std::array<float, 12> keyHistogram {};
    juce::String lastNote, lastKey { "--" };
    int noteHoldTicks = 0, tunerTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K808Editor)
};
