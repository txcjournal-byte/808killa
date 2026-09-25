#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;
using namespace Look;

namespace
{
    // design-space layout (1536 x 1024, matches the artwork)
    const Rectangle<int> topBarArea  { 452, 88, 1044, 72 };
    const Rectangle<int> contentArea { 460, 172, 906, 816 };
    const Rectangle<int> meterArea   { 1378, 172, 116, 816 };

    const char* describe (const String& preset)
    {
        static const std::map<String, const char*> text = {
            { "Atlanta Clean",     "Clean, long and punchy. The sub stays pure." },
            { "Memphis Phonk",     "Dirty and lo-fi: overdriven tape and crushed bits." },
            { "Rage Underground",  "Heavy foldback saturation, clipped hard." },
            { "Detroit Clip",      "Aggressive, short and clipped very hard." },
            { "Drill Chicago",     "Short and hard with a strong knock." },
            { "Drill NY",          "Short and hard with a clipped edge." },
            { "Drill UK",          "Hard with warm tape grit." },
            { "Plugg Soft",        "Soft, almost a pure sine with extra sub." },
            { "Chicago Boom",      "Long, thick and warm." },
            { "Classic Trap Boom", "Big boom with a long tail." },
            { "Clean Sub",         "Only firms up the low end. No distortion." },
            { "Knock Punch",       "Strong attack and click." },
            { "Plugg Bounce",      "Short and bouncy with a touch of tube." },
            { "Phone Punch",       "Extra harmonics so the 808 cuts through on phones." },
            { "Dirty Knock",       "Hard clipped knock with a clean sub underneath." },
            { "Lo-Fi Muffle",      "Muffled, crushed and dark." },
            { "Motor City Chop",   "Chopped short and clipped." },
        };
        const auto it = text.find (preset);
        return it != text.end() ? it->second : "Your own preset.";
    }
}

//==============================================================================
SimplePage::SimplePage (K808Processor& p)
    : kill   (p.apvts, ParamIDs::kill,   "KILL",   "One knob to push the whole style: more dirt, punch, sub and clipping."),
      length (p.apvts, ParamIDs::length, "LENGTH", "Shorten (left) or stretch (right) the tail of every 808 note.", true),
      punch  (p.apvts, ParamIDs::punch,  "PUNCH",  "Boosts the start of every note so the 808 hits harder."),
      dirt   (p.apvts, ParamIDs::dirt,   "DIRT",   "Amount of distortion. The type of dirt is set by the style (ADVANCED > DIRT)."),
      duck   (p.apvts, ParamIDs::duck,   "DUCK",   "Ducks the 808 under the kick. Needs the kick routed to the sidechain input.")
{
    for (auto* k : { &kill, &length, &punch, &dirt, &duck })
        addAndMakeVisible (*k);

    kill.setKnobArea ({ 283, 150, 340, 340 }, 48.0f);
    length.setKnobArea ({ 55, 95, 160, 160 }, 32.0f);
    punch.setKnobArea ({ 55, 430, 160, 160 }, 32.0f);
    dirt.setKnobArea ({ 691, 95, 160, 160 }, 32.0f);
    duck.setKnobArea ({ 691, 430, 160, 160 }, 32.0f);
}

void SimplePage::setStyleText (const String& title, const String& description)
{
    if (title == styleTitle && description == styleDescription)
        return;
    styleTitle = title;
    styleDescription = description;
    repaint();
}

void SimplePage::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    drawPanel (g, r);

    // style description under KILL
    const Rectangle<float> info (150.0f, 660.0f, r.getWidth() - 300.0f, 110.0f);
    drawPanel (g, info, true);
    g.setColour (Palette::redBright);
    g.setFont (fonts->bold (34.0f));
    g.drawText (styleTitle.toUpperCase(), info.withHeight (58.0f).translated (0.0f, 8.0f), Justification::centred, false);
    g.setColour (Palette::text);
    g.setFont (fonts->sans (21.0f));
    g.drawText (styleDescription, info.withTrimmedTop (60.0f).withTrimmedBottom (14.0f), Justification::centred, false);
}

//==============================================================================
template <typename T, typename... Args>
T& AdvancedPage::add (int tab, Rectangle<int> bounds, Args&&... args)
{
    auto* c = new T (std::forward<Args> (args)...);
    owned.add (c);
    addChildComponent (*c);
    c->setBounds (bounds);
    tabs[(size_t) tab].controls.push_back (c);
    return *c;
}

UI::Knob& AdvancedPage::knob (int tab, int col, int row, const String& id, const String& label, const String& tip, bool bipolar)
{
    auto* k = new UI::Knob (processor.apvts, id, label, tip, bipolar);
    owned.add (k);
    addChildComponent (*k);
    k->setKnobArea ({ 70 + col * 160, 190 + row * 270, 120, 120 }, 26.0f);
    tabs[(size_t) tab].controls.push_back (k);
    return *k;
}

AdvancedPage::AdvancedPage (K808Processor& p) : processor (p)
{
    using namespace ParamIDs;
    auto& s = p.apvts;

    tabs = {
        { "PITCH",    {},       "Pitch FX (bend, slide, knock, octave jump, key lock) arrive in the next update.", {} },
        { "SHAPE",    shapeOn,  {}, {} },
        { "WOBBLE",   {},       "Wobble (pitch / volume / filter LFO) arrives in the next update.", {} },
        { "TONE",     toneOn,   {}, {} },
        { "DIRT",     dirtOn,   {}, {} },
        { "DUCK",     duckOn,   "Route your kick to the sidechain input of 808 KILLA (FL Studio: kick mixer track > "
                                "'Sidechain to this track' on the 808 track, then pick it as the plugin's sidechain input).", {} },
        { "OUTPUT",   {},       {}, {} },
        { "SETTINGS", {},       {}, {} },
    };

    int visibleTabs = 0;
    for (int i = 0; i < (int) tabs.size(); ++i)
    {
        auto* b = tabButtons.add (new UI::FlatButton (tabs[(size_t) i].name));
        b->textHeight = 19.0f;
        b->onClick = [this, i] { showTab (i); };

        // PITCH and WOBBLE stay hidden until their DSP exists
        if (tabs[(size_t) i].name != "PITCH" && tabs[(size_t) i].name != "WOBBLE")
        {
            b->setBounds (12 + visibleTabs * 147, 14, 144, 46);
            addAndMakeVisible (b);
            ++visibleTabs;
        }

        if (tabs[(size_t) i].sectionParam.isNotEmpty())
        {
            sectionToggles[i] = std::make_unique<UI::LedToggle> (s, tabs[(size_t) i].sectionParam, "ON", "Switch this whole section on or off.");
            sectionToggles[i]->setBounds (632, 82, 120, 44);
            addChildComponent (*sectionToggles[i]);
        }
    }

    resetButton.setBounds (764, 82, 120, 44);
    resetButton.setTooltip ("Reset every control on this tab to its default.");
    resetButton.onClick = [this]
    {
        for (auto& id : sectionParameters (tabs[(size_t) current].name))
            if (auto* param = processor.apvts.getParameter (id))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->getDefaultValue());
                param->endChangeGesture();
            }
    };
    addChildComponent (resetButton);

    // ---- SHAPE
    knob (1, 0, 0, punch, "PUNCH", "Boosts the attack of every note.");
    knob (1, 1, 0, punchClick, "CLICK", "Adds a short click on top of each hit so it cuts through.");
    knob (1, 2, 0, length, "LENGTH", "Negative = shorter notes (gate). Positive = longer, fuller tails.", true);

    // ---- TONE
    knob (3, 0, 0, sub, "SUB", "Low shelf around 55 Hz: more or less sub.", true);
    knob (3, 1, 0, harmonics, "HARMONICS", "Adds upper harmonics so the 808 is heard on small speakers.");
    knob (3, 2, 0, tilt, "TILT", "Tilts the tone darker (left) or brighter (right).", true);
    add<UI::LedToggle> (3, { 40, 470, 220, 54 }, s, filterOn, "FILTER", "Low-pass filter for a muffled, lo-fi 808.");
    knob (3, 2, 1, cutoff, "CUTOFF", "Low-pass cutoff frequency.");
    knob (3, 3, 1, resonance, "RESO", "Resonance at the cutoff.");
    add<UI::ChoiceSelector> (3, { 40, 550, 220, 54 }, s, slope, "Filter steepness.");

    // ---- DIRT
    add<UI::ChoiceSelector> (4, { 40, 150, 826, 56 }, s, dirtMode, "Type of distortion.");
    knob (4, 0, 1, dirt, "DRIVE", "How hard the 808 is pushed into the distortion.");
    knob (4, 1, 1, dirtMix, "DIRT MIX", "Blend between clean and distorted 808.");
    knob (4, 2, 1, crushBits, "CRUSH", "Bit reduction for lo-fi grit (24 = off).");
    knob (4, 3, 1, postFilter, "POST FILTER", "Low-pass after the distortion to tame fizz.");
    knob (4, 4, 1, cleanFreq, "CLEAN FREQ", "Below this frequency the sub stays clean when CLEAN LOW is on.");
    for (auto* c : tabs[4].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c))
            k->setTopLeftPosition (k->getX(), k->getY() - 150);
    add<UI::LedToggle> (4, { 40, 610, 200, 54 }, s, cleanLow, "CLEAN LOW", "Distort only above the crossover: the sub stays clean.");
    add<UI::LedToggle> (4, { 260, 610, 200, 54 }, s, autoGain, "AUTO GAIN", "Keeps the level steady while you change the drive.");
    add<UI::ChoiceSelector> (4, { 560, 610, 306, 54 }, s, oversample, "Oversampling quality. Higher = cleaner but more CPU.");

    // ---- DUCK
    knob (5, 0, 0, duck, "DUCK", "How much the 808 ducks under the kick.");
    knob (5, 1, 0, duckRel, "RELEASE", "How fast the 808 comes back after the kick.");
    knob (5, 2, 0, duckShape, "SHAPE", "Soft (left) or hard (right) ducking curve.");

    // ---- OUTPUT
    knob (6, 0, 0, inGain, "INPUT", "Level going into the plugin.", true);
    knob (6, 1, 0, clipper, "CLIPPER", "Soft to hard clipping at the ceiling. 0 = off.");
    knob (6, 2, 0, ceiling, "CEILING", "Maximum output level of the clipper.");
    knob (6, 3, 0, outGain, "OUTPUT", "Output level.", true);
    knob (6, 4, 0, mix, "MIX", "Dry / wet mix of the whole plugin.");
    knob (6, 0, 1, monoBelow, "MONO BELOW", "Makes everything below this frequency mono. 0 = off.");

    // ---- SETTINGS
    openFolderButton.setBounds (40, 330, 360, 54);
    openFolderButton.onClick = [] { PresetManager::userFolder().createDirectory(); PresetManager::userFolder().startAsProcess(); };
    addChildComponent (openFolderButton);
    tabs[7].controls.push_back (&openFolderButton);

    showTab (4);
}

void AdvancedPage::showTab (int index)
{
    current = jlimit (0, (int) tabs.size() - 1, index);

    for (int i = 0; i < (int) tabs.size(); ++i)
    {
        tabButtons[i]->active = i == current;
        tabButtons[i]->repaint();
        for (auto* c : tabs[(size_t) i].controls)
            c->setVisible (i == current);
        if (sectionToggles[i] != nullptr)
            sectionToggles[i]->setVisible (i == current);
    }

    resetButton.setVisible (! sectionParameters (tabs[(size_t) current].name).isEmpty());
    repaint();
}

void AdvancedPage::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    drawPanel (g, r);

    const auto& tab = tabs[(size_t) current];
    drawLabel (g, tab.name, { 40.0f, 80.0f, 400.0f, 48.0f }, 44.0f, Palette::ink, Justification::centredLeft);

    if (tab.note.isNotEmpty())
    {
        const Rectangle<float> box (40.0f, tab.controls.empty() ? 160.0f : 640.0f, r.getWidth() - 80.0f, 110.0f);
        drawPanel (g, box, true);
        g.setColour (Palette::text);
        g.setFont (fonts->sans (21.0f));
        g.drawFittedText (tab.note, box.reduced (20.0f, 10.0f).toNearestInt(), Justification::centredLeft, 4);
    }

    if (tab.name == "SETTINGS")
    {
        const Rectangle<float> box (40.0f, 150.0f, r.getWidth() - 80.0f, 160.0f);
        drawPanel (g, box, true);
        g.setColour (Palette::text);
        g.setFont (fonts->sans (21.0f));
        const auto text = String (JucePlugin_Name) + "  v" + JucePlugin_VersionString + "\n"
                          + "by " + JucePlugin_Manufacturer + "\n"
                          + "Double-click a knob = default value.  Ctrl / Cmd + drag = fine adjustment.";
        g.drawFittedText (text, box.reduced (20.0f, 14.0f).toNearestInt(), Justification::topLeft, 5);
    }
}

//==============================================================================
Canvas::Canvas()
{
    background = ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);
    setOpaque (true);
}

void Canvas::paint (Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    drawPanel (g, topBarArea.toFloat());
    drawPanel (g, meterArea.toFloat(), true);

    g.setColour (Palette::text);
    g.setFont (fonts->bold (26.0f));
    g.drawText ("MASTER", meterArea.toFloat().withHeight (46.0f).translated (0.0f, 6.0f), Justification::centred, false);


    // version in the title bar (covers the artwork's static text)
    const Rectangle<float> title (1060.0f, 32.0f, 290.0f, 30.0f);
    g.setColour (Colour (0xff0c0c0b));
    g.fillRect (title);
    g.setColour (Colour (0xffd9d4ca));
    g.setFont (fonts->mono (19.0f));
    g.drawText ("LOW END DAMAGE UNIT / REV " + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false),
                title, Justification::centredRight, false);
}

void Canvas::paintOverChildren (Graphics& g)
{
    if (phoneOn)
    {
        const Rectangle<float> banner ((float) contentArea.getX() + 190.0f, (float) contentArea.getY() + 12.0f, 526.0f, 34.0f);
        g.setColour (Palette::red);
        g.fillRect (banner);
        g.setColour (Colours::white);
        g.setFont (fonts->bold (24.0f));
        g.drawText ("PHONE CHECK ON  -  TURN OFF BEFORE EXPORT", banner, Justification::centred, false);
    }
}

//==============================================================================
K808Editor::K808Editor (K808Processor& p)
    : AudioProcessorEditor (p), processor (p),
      simple (p), advanced (p),
      phoneButton (p.apvts, ParamIDs::phone, "PHONE CHECK", "Listen like on a phone speaker. Monitoring only: switch it off before you export!")
{
    setLookAndFeel (&lnf);
    tooltips.setLookAndFeel (&lnf);

    addAndMakeVisible (canvas);
    canvas.setBounds (0, 0, designWidth, designHeight);

    // ---- top bar
    const auto bar = topBarArea.reduced (10, 10);
    simpleTab.setBounds (bar.getX(), bar.getY(), 130, bar.getHeight());
    advancedTab.setBounds (bar.getX() + 134, bar.getY(), 150, bar.getHeight());
    prevButton.setBounds (bar.getX() + 310, bar.getY(), 52, bar.getHeight());
    presetButton.setBounds (bar.getX() + 366, bar.getY(), 440, bar.getHeight());
    nextButton.setBounds (bar.getX() + 810, bar.getY(), 52, bar.getHeight());
    saveButton.setBounds (bar.getRight() - 150, bar.getY(), 150, bar.getHeight());

    presetButton.textHeight = 26.0f;
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    presetButton.setTooltip ("Pick a style or preset");
    saveButton.setTooltip ("Save the current sound as your own preset");
    simpleTab.setTooltip ("Main page: style, KILL and the macros");
    advancedTab.setTooltip ("Every parameter, sorted in tabs");

    simpleTab.onClick = [this] { showPage (false); };
    advancedTab.onClick = [this] { showPage (true); };
    prevButton.onClick = [this] { processor.presets.loadNext (-1); refreshPresetLabel(); };
    nextButton.onClick = [this] { processor.presets.loadNext (1); refreshPresetLabel(); };
    presetButton.onClick = [this] { showPresetMenu(); };
    saveButton.onClick = [this] { savePresetAs(); };

    for (auto* b : { &simpleTab, &advancedTab, &prevButton, &presetButton, &nextButton, &saveButton })
        canvas.addAndMakeVisible (*b);

    // ---- pages
    simple.setBounds (contentArea);
    advanced.setBounds (contentArea);
    canvas.addAndMakeVisible (simple);
    canvas.addChildComponent (advanced);

    // ---- master column
    const auto m = meterArea.reduced (10, 0);
    meter.setBounds (m.getX(), meterArea.getY() + 56, m.getWidth(), 520);
    canvas.addAndMakeVisible (meter);

    for (auto* l : { &lufsLabel, &peakLabel })
    {
        l->setJustificationType (Justification::centred);
        l->setColour (Label::textColourId, Palette::text);
        l->setColour (Label::backgroundColourId, Palette::boxFill);
        l->setColour (Label::outlineColourId, Palette::boxEdge);
        l->setFont (lnf.fonts->mono (20.0f));
        canvas.addAndMakeVisible (*l);
    }
    peakLabel.setBounds (m.getX(), meterArea.getY() + 590, m.getWidth(), 36);
    lufsLabel.setBounds (m.getX(), meterArea.getY() + 632, m.getWidth(), 36);
    peakLabel.setTooltip ("Output peak (dBFS)");
    lufsLabel.setTooltip ("Output loudness, short-term (LUFS)");

    phoneButton.big = true;
    phoneButton.setBounds (m.getX(), meterArea.getBottom() - 130, m.getWidth(), 118);
    canvas.addAndMakeVisible (phoneButton);

    showPage (false);
    refreshPresetLabel();

    setResizable (true, true);
    setResizeLimits (designWidth * 2 / 5, designHeight * 2 / 5, designWidth * 5 / 4, designHeight * 5 / 4);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    // first open: half size; afterwards the size the user dragged it to
    const auto savedWidth = (int) processor.apvts.state.getProperty ("uiWidth", designWidth / 2);
    const auto width = jlimit (designWidth * 2 / 5, designWidth * 5 / 4, savedWidth);
    setSize (width, width * designHeight / designWidth);

    // never take keyboard focus: the space bar etc. must keep reaching the DAW (play / stop)
    setWantsKeyboardFocus (false);
    std::function<void (Component&)> noFocus = [&noFocus] (Component& c)
    {
        c.setWantsKeyboardFocus (false);
        c.setMouseClickGrabsKeyboardFocus (false);
        for (auto* child : c.getChildren())
            noFocus (*child);
    };
    noFocus (*this);

    startTimerHz (30);
}

K808Editor::~K808Editor()
{
    stopTimer();
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void K808Editor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void K808Editor::resized()
{
    canvas.setTransform (AffineTransform::scale ((float) getWidth() / (float) designWidth));
    processor.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}

void K808Editor::showPage (bool adv)
{
    simple.setVisible (! adv);
    advanced.setVisible (adv);
    simpleTab.active = ! adv;
    advancedTab.active = adv;
    simpleTab.repaint();
    advancedTab.repaint();
}

void K808Editor::refreshPresetLabel()
{
    const auto name = processor.presets.getCurrentName();
    const auto modified = processor.presets.isModified();
    if (name == shownPreset && modified == shownModified)
        return;

    shownPreset = name;
    shownModified = modified;
    presetButton.setButtonText (name.toUpperCase() + (modified ? " *" : ""));

    const auto index = processor.presets.getCurrentIndex();
    const auto& list = processor.presets.getPresets();
    const auto factory = isPositiveAndBelow (index, list.size()) && list.getReference (index).factory;
    simple.setStyleText (name, factory ? describe (name) : "Your own preset.");
}

void K808Editor::showPresetMenu()
{
    auto& pm = processor.presets;
    const auto& list = pm.getPresets();
    const auto current = pm.getCurrentIndex();

    PopupMenu menu;
    std::map<String, PopupMenu> categories;
    StringArray order { "Styles", "Clean", "Dirty", "Pitch", "FX", "User" };

    for (int i = 0; i < list.size(); ++i)
        categories[list.getReference (i).category].addItem (i + 1, list.getReference (i).name, true, i == current);

    for (auto& cat : order)
        if (categories.count (cat) > 0)
        {
            if (cat == "Styles")
            {
                menu.addSectionHeader ("STYLES");
                for (PopupMenu::MenuItemIterator it (categories[cat]); it.next();)
                    menu.addItem (it.getItem());
            }
            else
            {
                menu.addSubMenu (cat.toUpperCase(), categories[cat]);
            }
        }

    menu.addSeparator();
    menu.addItem (1001, "Save As...");
    menu.addItem (1002, "Revert changes", pm.isModified() && current >= 0);
    menu.addItem (1003, "Init (all defaults)");
    menu.addItem (1004, "Open presets folder");

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetButton),
                        [this] (int result)
                        {
                            auto& presets = processor.presets;
                            if (result >= 1 && result <= 1000)  presets.load (result - 1);
                            else if (result == 1001)            savePresetAs();
                            else if (result == 1002)            presets.revert();
                            else if (result == 1003)            presets.init();
                            else if (result == 1004)
                            {
                                PresetManager::userFolder().createDirectory();
                                PresetManager::userFolder().startAsProcess();
                            }
                            shownPreset = {};
                            refreshPresetLabel();
                        });
}

void K808Editor::savePresetAs()
{
    saveDialog = std::make_unique<AlertWindow> ("SAVE PRESET", "Name your preset:", MessageBoxIconType::NoIcon);
    saveDialog->setLookAndFeel (&lnf);
    saveDialog->addTextEditor ("name", processor.presets.getCurrentName());
    saveDialog->addButton ("SAVE", 1, KeyPress (KeyPress::returnKey));
    saveDialog->addButton ("CANCEL", 0, KeyPress (KeyPress::escapeKey));
    saveDialog->enterModalState (true, ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
        {
            const auto name = saveDialog->getTextEditorContents ("name");
            if (! processor.presets.saveUser (name))
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "808 KILLA", "Could not save the preset.");
        }
        saveDialog.reset();
        shownPreset = {};
        refreshPresetLabel();
    }), false);
}

void K808Editor::timerCallback()
{
    auto& e = processor.engine;
    const float peaks[2] = { e.inPeak.exchange (0.0f), e.outPeak.exchange (0.0f) };

    for (int i = 0; i < 2; ++i)
    {
        const auto db = Decibels::gainToDecibels (peaks[i], -100.0f);
        meterDb[i] = db >= meterDb[i] ? db : jmax (db, meterDb[i] - 1.2f);
    }
    meter.setLevels (meterDb[0], meterDb[1]);

    const auto outDb = Decibels::gainToDecibels (peaks[1], -100.0f);
    if (outDb >= peakHoldDb || --peakHoldTicks <= 0)
    {
        peakHoldDb = outDb;
        peakHoldTicks = 45;
    }
    peakLabel.setText (peakHoldDb <= -60.0f ? "-inf" : String (peakHoldDb, 1), dontSendNotification);

    const auto lufs = e.shortTermLufs.load();
    lufsLabel.setText (lufs <= -70.0f ? "-- LU" : String (lufs, 1) + " LU", dontSendNotification);

    const auto phoneOn = processor.apvts.getRawParameterValue (ParamIDs::phone)->load() > 0.5f;
    if (phoneOn != canvas.phoneOn)
    {
        canvas.phoneOn = phoneOn;
        canvas.repaint();
    }

    // DUCK is only useful with a sidechain signal
    const auto scActive = e.sidechainActive.load();
    const auto duckAlpha = scActive ? 1.0f : 0.45f;
    if (! approximatelyEqual (simple.duck.getAlpha(), duckAlpha))
    {
        simple.duck.setAlpha (duckAlpha);
        simple.duck.getSlider().setTooltip (scActive
            ? "Ducks the 808 under the kick."
            : "No sidechain signal. Send your kick to this track's sidechain input to use DUCK.");
    }

    refreshPresetLabel();
}
