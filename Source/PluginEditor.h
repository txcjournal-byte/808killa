#pragma once

#include "PluginProcessor.h"
#include "UI/Look.h"
#include "UI/Widgets.h"

//==============================================================================
// SIMPLE page: KILL + macros
class SimplePage : public juce::Component
{
public:
    explicit SimplePage (K808Processor&);
    void paint (juce::Graphics&) override;
    void setStyleText (const juce::String& title, const juce::String& description);
    void setTunerText (const juce::String& text);

    UI::Knob kill, length, punch, dirt, sub, bend, wobble;

private:
    juce::String styleTitle, styleDescription, tunerText { "NOTE --     KEY --" };
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

//==============================================================================
// ADVANCED page: tabs with every parameter
class AdvancedPage : public juce::Component
{
public:
    explicit AdvancedPage (K808Processor&);
    void paint (juce::Graphics&) override;
    void showTab (int index);

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
    std::unique_ptr<UI::LedToggle> sectionToggles[9];
    UI::FlatButton resetButton { "RESET" }, openFolderButton { "OPEN PRESETS FOLDER" };
    int current = 0;
    juce::SharedResourcePointer<Look::Fonts> fonts;
};

//==============================================================================
class Canvas : public juce::Component
{
public:
    Canvas();
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    bool phoneOn = false;

private:
    juce::Image background;
    juce::SharedResourcePointer<Look::Fonts> fonts;
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
    void showPage (bool advanced);
    void showPresetMenu();
    void savePresetAs();
    void refreshPresetLabel();
    void updateTuner();

    K808Processor& processor;
    Look::LookAndFeel lnf;
    juce::TooltipWindow tooltips { nullptr, 600 };
    Canvas canvas;

    UI::FlatButton simpleTab { "SIMPLE" }, advancedTab { "ADVANCED" };
    UI::FlatButton prevButton { "<" }, nextButton { ">" }, presetButton { "" }, saveButton { "SAVE" }, abButton { "A" };

    SimplePage simple;
    AdvancedPage advanced;

    UI::Meter meter;
    UI::LedToggle phoneButton;
    juce::Label lufsLabel, peakLabel;

    float meterDb[2] { -100.0f, -100.0f };
    float peakHoldDb = -100.0f;
    int peakHoldTicks = 0;
    juce::String shownPreset;
    bool shownModified = false;

    std::unique_ptr<juce::AlertWindow> saveDialog;

    // tuner
    std::array<float, 12> keyHistogram {};
    juce::String lastNote;
    int noteHoldTicks = 0, tunerTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K808Editor)
};
