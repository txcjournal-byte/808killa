#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;
using namespace Look;

namespace
{
    // design-space layout (1536 x 1024, see tools/make_background_v3.py)
    const Rectangle<int> topBarArea  { 362, 66, 1160, 70 };
    const Rectangle<int> contentArea { 362, 144, 1160, 866 };
    const Rectangle<int> meterArea   { 12, 678, 340, 332 };

    const char* describe (const String& preset)
    {
        static const std::map<String, const char*> text = {
            { "Atlanta Clean",     "Clean, long and punchy. The sub stays pure." },
            { "Memphis Phonk",     "Dirty and lo-fi: overdriven tape and crushed bits." },
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
            { "Slime Bend",        "Hard clipped with a fast pitch drop at the end of every note." },
            { "Dive Bomb",         "Every note dives two octaves down." },
            { "Sub Octave",        "Adds a sub one octave below the 808." },
            { "Octave Grit",       "Gritty upper octave layered on top." },
            { "Wobble Wave",       "Pitch, volume and filter wobble in 1/16." },
            { "Triplet Wub",       "Filter wub in 1/8 triplets." },
            { "Tape Drop",         "Slow tape-stop style pitch drop." },
            { "Detroit Clip",      "Aggressive and loud: hard clipped with a sharp knock." },
            { "Rage Underground",  "Dirty folded saturation with a gritty top octave." },
        };
        const auto it = text.find (preset);
        return it != text.end() ? it->second : "Your own preset.";
    }
}

//==============================================================================
namespace
{
    // YIN pitch detection on the decimated tuner feed. Returns 0 when no clear pitch.
    float detectPitch (const float* x, int n, double rate)
    {
        const int maxTau = jmin (n / 2, (int) (rate / 25.0));
        const int minTau = jmax (2, (int) (rate / 300.0));
        const int w = n - maxTau;
        if (w < 32) return 0.0f;

        std::vector<float> d ((size_t) maxTau + 1, 0.0f);
        for (int tau = 1; tau <= maxTau; ++tau)
        {
            float sum = 0.0f;
            for (int i = 0; i < w; ++i)
            {
                const auto diff = x[i] - x[i + tau];
                sum += diff * diff;
            }
            d[(size_t) tau] = sum;
        }

        float running = 0.0f;
        std::vector<float> cm ((size_t) maxTau + 1, 1.0f);
        for (int tau = 1; tau <= maxTau; ++tau)
        {
            running += d[(size_t) tau];
            cm[(size_t) tau] = running > 0.0f ? d[(size_t) tau] * (float) tau / running : 1.0f;
        }

        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (cm[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < maxTau && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau;
                const auto a = cm[(size_t) tau - 1], b = cm[(size_t) tau], c = cm[(size_t) tau + 1];
                const auto denom = a - 2.0f * b + c;
                const auto shift = std::abs (denom) > 1.0e-9f ? 0.5f * (a - c) / denom : 0.0f;
                return (float) (rate / ((float) tau + jlimit (-0.5f, 0.5f, shift)));
            }
        }
        return 0.0f;
    }

    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

//==============================================================================
SimplePage::SimplePage (K808Processor& p)
    : kill   (p.apvts, ParamIDs::kill,   "KILL",   "One knob to push the whole style: more dirt, punch, sub and clipping."),
      length (p.apvts, ParamIDs::length, "LENGTH", "Shorten (left) or stretch (right) the tail of every 808 note.", true),
      punch  (p.apvts, ParamIDs::punch,  "PUNCH",  "Boosts the start of every note so the 808 hits harder."),
      dirt   (p.apvts, ParamIDs::dirt,   "DIRT",   "Amount of distortion. The type of dirt is set by the style (ADVANCED > DIRT)."),
      sub    (p.apvts, ParamIDs::sub,    "SUB",    "How much sub (low end around 55 Hz) the 808 gets."),
      bend   (p.apvts, ParamIDs::dive,   "BEND",   "Pitch dive on every note (trap bend). Timing is in ADVANCED > PITCH.", false),
      wobble (p.apvts, ParamIDs::wobble, "WOBBLE", "Tempo-synced wobble. Target, rate and shape are in ADVANCED > WOBBLE.")
{
    for (auto* k : { &kill, &length, &punch, &dirt, &sub, &bend, &wobble })
        addAndMakeVisible (*k);
    bend.getSlider().getProperties().set ("fromEnd", true);   // arc grows as the dive gets deeper

    kill.setKnobArea ({ 380, 96, 400, 400 }, 58.0f);
    length.setKnobArea ({ 85, 60, 170, 170 }, 38.0f);
    punch.setKnobArea ({ 85, 330, 170, 170 }, 38.0f);
    sub.setKnobArea ({ 85, 600, 170, 170 }, 38.0f);
    kill.getSlider().getProperties().set ("hero", true);
    dirt.setKnobArea ({ 905, 60, 170, 170 }, 38.0f);
    bend.setKnobArea ({ 905, 330, 170, 170 }, 38.0f);
    wobble.setKnobArea ({ 905, 600, 170, 170 }, 38.0f);
}

void SimplePage::setTunerText (const String& text)
{
    if (text != tunerText)
    {
        tunerText = text;
        repaint (300, 780, getWidth() - 600, 66);
    }
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
    const Rectangle<float> info (300.0f, 632.0f, r.getWidth() - 600.0f, 214.0f);
    drawPanel (g, info, true);
    g.setColour (Palette::redBright);
    g.setFont (fonts->bold (44.0f));
    g.drawText (styleTitle.toUpperCase(), info.withHeight (64.0f).translated (0.0f, 10.0f), Justification::centred, false);
    g.setColour (Palette::text);
    g.setFont (fonts->sans (27.0f));
    g.drawFittedText (styleDescription, info.withTrimmedTop (72.0f).withHeight (70.0f).reduced (16.0f, 0.0f).toNearestInt(),
                      Justification::centred, 2);

    // tuner line
    const auto tunerArea = info.withTrimmedTop (148.0f).reduced (20.0f, 10.0f);
    g.setColour (Colour (0xff050505));
    g.fillRoundedRectangle (tunerArea, 3.0f);
    g.setColour (Palette::boxEdge);
    g.drawRoundedRectangle (tunerArea, 3.0f, 1.0f);
    g.setColour (tunerText.startsWith ("NOTE --") ? Palette::textDim : Palette::text);
    g.setFont (fonts->mono (30.0f));
    g.drawText (tunerText, tunerArea, Justification::centred, false);
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
    k->setKnobArea ({ 90 + col * 210, 220 + row * 300, 140, 140 }, 32.0f);
    tabs[(size_t) tab].controls.push_back (k);
    return *k;
}

AdvancedPage::AdvancedPage (K808Processor& p) : processor (p)
{
    using namespace ParamIDs;
    auto& s = p.apvts;

    tabs = {
        { "PITCH",    pitchOn,  "KNOCK = pitch hit at the start of each note.  BEND = trap dive after BEND DELAY.  "
                                "OCT DOWN / OCT UP = extra octave layers.", {} },
        { "SHAPE",    shapeOn,  {}, {} },
        { "WOBBLE",   wobbleOn, {}, {} },
        { "TONE",     toneOn,   {}, {} },
        { "DIRT",     dirtOn,   {}, {} },
        { "DUCK",     duckOn,   "Only needed when your beat has a kick. Send the kick to the 808 track as sidechain "
                                "(FL Studio: kick mixer track > right-click the arrow to the 808 track > Sidechain to this track).", {} },
        { "OUTPUT",   {},       {}, {} },
        { "SETTINGS", {},       {}, {} },
        { "CHOP",     chopOn,   "Rhythmic gate locked to your DAW tempo. ROLL speeds up towards the end of the bar, "
                                "GROSS and STUTTER are ready-made patterns.", {} },
    };
    static constexpr int displayOrder[] = { 0, 1, 2, 8, 3, 4, 5, 6, 7 };

    int visibleTabs = 0;
    for (int i = 0; i < (int) tabs.size(); ++i)
    {
        auto* b = tabButtons.add (new UI::FlatButton (tabs[(size_t) i].name));
        b->textHeight = 23.0f;
        b->onClick = [this, i] { showTab (i); };
        const auto slot = (int) (std::find (std::begin (displayOrder), std::end (displayOrder), i) - std::begin (displayOrder));
        b->setBounds (12 + slot * 126, 14, 123, 52);
        b->textHeight = 21.0f;
        addAndMakeVisible (b);
        ++visibleTabs;

        if (tabs[(size_t) i].sectionParam.isNotEmpty())
        {
            sectionToggles[i] = std::make_unique<UI::LedToggle> (s, tabs[(size_t) i].sectionParam, "ON", "Switch this whole section on or off.");
            sectionToggles[i]->setBounds (850, 84, 140, 52);
            addChildComponent (*sectionToggles[i]);
        }
    }

    resetButton.setBounds (1004, 84, 140, 52);
    resetButton.textHeight = 23.0f;
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

    // ---- PITCH
    knob (0, 0, 0, knock, "KNOCK", "Pitch hit at the start of every note (semitones above the note).");
    knob (0, 1, 0, knockTime, "KNOCK TIME", "How fast the knock falls back to the note.");
    knob (0, 2, 0, dive, "BEND", "Trap dive: how far each note drops (semitones).").getSlider().getProperties().set ("fromEnd", true);
    knob (0, 3, 0, diveTime, "BEND TIME", "How long the dive takes.");
    knob (0, 4, 0, diveDelay, "BEND DELAY", "How long after the note starts the dive begins.");
    knob (0, 0, 1, octDown, "OCT DOWN", "Adds a sub one octave below the 808.");
    knob (0, 1, 1, octUp, "OCT UP", "Adds a gritty octave above the 808.");
    for (auto* c : tabs[0].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c); k != nullptr && k->getY() > 400)
            k->setTopLeftPosition (k->getX(), k->getY() - 60);

    // ---- WOBBLE
    add<UI::ChoiceSelector> (2, { 220, 150, 900, 58 }, s, wobbleTarget, "What the wobble moves: pitch, volume, filter or all of them.");
    add<UI::ChoiceSelector> (2, { 220, 222, 900, 58 }, s, wobbleRate, "Wobble speed, synced to the DAW tempo (T = triplet, D = dotted).");
    add<UI::ChoiceSelector> (2, { 220, 294, 900, 58 }, s, wobbleShape, "Wobble waveform.");
    knob (2, 0, 1, wobble, "DEPTH", "How strong the wobble is.");
    knob (2, 1, 1, wobbleFade, "FADE IN", "The wobble fades in after each note starts.");
    for (auto* c : tabs[2].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c))
            k->setTopLeftPosition (k->getX(), k->getY() - 60);
    add<UI::LedToggle> (2, { 560, 520, 300, 60 }, s, wobbleRetrig, "RETRIGGER", "Restart the wobble on every new note (off = locked to the DAW grid).");

    // ---- SHAPE
    knob (1, 1, 0, punch, "PUNCH", "Boosts the attack of every note.");
    knob (1, 2, 0, punchClick, "CLICK", "Adds a short click on top of each hit so it cuts through.");
    knob (1, 3, 0, length, "LENGTH", "Negative = shorter notes (gate). Positive = longer, fuller tails.", true);

    // ---- TONE
    knob (3, 0, 0, sub, "SUB", "Low shelf around 55 Hz: more or less sub.", true);
    knob (3, 1, 0, harmonics, "HARMONICS", "Adds upper harmonics so the 808 is heard on small speakers.");
    knob (3, 2, 0, tilt, "TILT", "Tilts the tone darker (left) or brighter (right).", true);
    add<UI::LedToggle> (3, { 40, 540, 280, 60 }, s, filterOn, "FILTER", "Low-pass filter for a muffled, lo-fi 808.");
    knob (3, 2, 1, cutoff, "CUTOFF", "Low-pass cutoff frequency.");
    knob (3, 3, 1, resonance, "RESO", "Resonance at the cutoff.");
    add<UI::ChoiceSelector> (3, { 40, 620, 280, 60 }, s, slope, "Filter steepness.");

    // ---- DIRT
    add<UI::ChoiceSelector> (4, { 40, 150, 1080, 62 }, s, dirtMode, "Type of distortion.");
    knob (4, 0, 1, dirt, "DRIVE", "How hard the 808 is pushed into the distortion.");
    knob (4, 1, 1, dirtMix, "DIRT MIX", "Blend between clean and distorted 808.");
    knob (4, 2, 1, crushBits, "CRUSH", "Bit reduction for lo-fi grit (24 = off).");
    knob (4, 3, 1, postFilter, "POST FILTER", "Low-pass after the distortion to tame fizz.");
    knob (4, 4, 1, cleanFreq, "CLEAN FREQ", "Below this frequency the sub stays clean when CLEAN LOW is on.");
    for (auto* c : tabs[4].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c))
            k->setTopLeftPosition (k->getX(), k->getY() - 150);
    add<UI::LedToggle> (4, { 40, 660, 280, 62 }, s, cleanLow, "CLEAN LOW", "Distort only above the crossover: the sub stays clean.");
    add<UI::LedToggle> (4, { 340, 660, 280, 62 }, s, autoGain, "AUTO GAIN", "Keeps the level steady while you change the drive.");
    add<UI::ChoiceSelector> (4, { 700, 660, 420, 62 }, s, oversample, "Oversampling quality. Higher = cleaner but more CPU.");

    // ---- DUCK
    knob (5, 1, 0, duck, "DUCK", "How much the 808 ducks under the kick.");
    knob (5, 2, 0, duckRel, "RELEASE", "How fast the 808 comes back after the kick.");
    knob (5, 3, 0, duckShape, "SHAPE", "Soft (left) or hard (right) ducking curve.");

    // ---- OUTPUT
    knob (6, 0, 0, inGain, "INPUT", "Level going into the plugin.", true);
    knob (6, 1, 0, clipper, "CLIPPER", "Soft to hard clipping at the ceiling. 0 = off.");
    knob (6, 2, 0, ceiling, "CEILING", "Maximum output level of the clipper.");
    knob (6, 3, 0, outGain, "OUTPUT", "Output level.", true);
    knob (6, 4, 0, mix, "MIX", "Dry / wet mix of the whole plugin.");
    knob (6, 0, 1, monoBelow, "MONO BELOW", "Makes everything below this frequency mono. 0 = off.");
    knob (6, 1, 1, width, "WIDTH", "Widens the upper part of the 808. The sub stays mono.");

    // ---- CHOP
    add<UI::ChoiceSelector> (8, { 220, 150, 900, 58 }, s, chopPattern, "Chop rhythm.");
    knob (8, 1, 1, chop, "DEPTH", "How deep the chop cuts (100 % = silence between hits).");
    knob (8, 2, 1, chopGate, "GATE", "How long each chop stays open.");
    knob (8, 3, 1, chopSmooth, "SMOOTH", "Hard cuts (left) or soft pulsing (right).");
    for (auto* c : tabs[8].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c))
            k->setTopLeftPosition (k->getX(), k->getY() - 200);

    // ---- SETTINGS
    openFolderButton.setBounds (40, 360, 420, 62);
    openFolderButton.textHeight = 23.0f;
    openFolderButton.onClick = [] { PresetManager::userFolder().createDirectory(); PresetManager::userFolder().startAsProcess(); };
    addChildComponent (openFolderButton);
    tabs[7].controls.push_back (&openFolderButton);

    showTab (0);
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
    drawLabel (g, tab.name, { 40.0f, 82.0f, 400.0f, 56.0f }, 52.0f, Palette::ink, Justification::centredLeft);

    if (tab.name == "CHOP")
        drawLabel (g, "PATTERN", { 40.0f, 150.0f, 170.0f, 58.0f }, 30.0f, Palette::ink, Justification::centredLeft);

    if (tab.name == "WOBBLE")
    {
        drawLabel (g, "TARGET", { 40.0f, 150.0f, 170.0f, 58.0f }, 30.0f, Palette::ink, Justification::centredLeft);
        drawLabel (g, "RATE",   { 40.0f, 222.0f, 170.0f, 58.0f }, 30.0f, Palette::ink, Justification::centredLeft);
        drawLabel (g, "SHAPE",  { 40.0f, 294.0f, 170.0f, 58.0f }, 30.0f, Palette::ink, Justification::centredLeft);
    }

    if (tab.note.isNotEmpty())
    {
        const Rectangle<float> box (40.0f, tab.controls.empty() ? 160.0f : 660.0f, r.getWidth() - 80.0f, 180.0f);
        drawPanel (g, box, true);
        g.setColour (Palette::text);
        g.setFont (fonts->sans (31.0f));
        g.drawFittedText (tab.note, box.reduced (26.0f, 14.0f).toNearestInt(), Justification::centredLeft, 4, 1.0f);
    }

    if (tab.name == "SETTINGS")
    {
        const Rectangle<float> box (40.0f, 160.0f, r.getWidth() - 80.0f, 170.0f);
        drawPanel (g, box, true);
        g.setColour (Palette::text);
        g.setFont (fonts->sans (26.0f));
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
    g.setFont (fonts->bold (32.0f));
    g.drawText ("MASTER", meterArea.toFloat().withHeight (46.0f).translated (18.0f, 8.0f), Justification::centredLeft, false);

    // version in the title bar
    const Rectangle<float> title (1000.0f, 14.0f, 396.0f, 32.0f);
    g.setColour (Colour (0xff0d0c0b));
    g.fillRect (title);
    g.setColour (Colour (0xffd9d4ca));
    g.setFont (fonts->mono (21.0f));
    g.drawText ("LOW END DAMAGE UNIT / REV " + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false),
                title, Justification::centredRight, false);
}

void Canvas::paintOverChildren (Graphics& g)
{
    if (phoneOn)
    {
        const Rectangle<float> banner ((float) contentArea.getX() + 280.0f, (float) contentArea.getY() + 10.0f, 600.0f, 40.0f);
        g.setColour (Palette::red);
        g.fillRect (banner);
        g.setColour (Colours::white);
        g.setFont (fonts->bold (28.0f));
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
    simpleTab.setBounds (bar.getX(), bar.getY(), 150, bar.getHeight());
    advancedTab.setBounds (bar.getX() + 154, bar.getY(), 180, bar.getHeight());
    prevButton.setBounds (bar.getX() + 350, bar.getY(), 60, bar.getHeight());
    presetButton.setBounds (bar.getX() + 414, bar.getY(), 410, bar.getHeight());
    nextButton.setBounds (bar.getX() + 828, bar.getY(), 60, bar.getHeight());
    abButton.setBounds (bar.getX() + 894, bar.getY(), 72, bar.getHeight());
    abButton.setTooltip ("A/B compare: switch between two versions of your settings");
    abButton.onClick = [this]
    {
        processor.presets.toggleAB();
        abButton.setButtonText (processor.presets.isOnB() ? "B" : "A");
        abButton.active = processor.presets.isOnB();
        abButton.repaint();
    };
    canvas.addAndMakeVisible (abButton);
    saveButton.setBounds (bar.getRight() - 170, bar.getY(), 170, bar.getHeight());

    presetButton.textHeight = 31.0f;
    for (auto* b : { &simpleTab, &advancedTab, &prevButton, &nextButton, &saveButton, &abButton })
        b->textHeight = 25.0f;
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
    meter.setBounds (meterArea.getX() + 12, meterArea.getY() + 56, 160, meterArea.getHeight() - 68);
    canvas.addAndMakeVisible (meter);
    const Rectangle<int> m (meterArea.getX() + 182, meterArea.getY() + 12, meterArea.getWidth() - 194, meterArea.getHeight() - 24);

    for (auto* l : { &lufsLabel, &peakLabel })
    {
        l->setJustificationType (Justification::centred);
        l->setColour (Label::textColourId, Palette::text);
        l->setColour (Label::backgroundColourId, Palette::boxFill);
        l->setColour (Label::outlineColourId, Palette::boxEdge);
        l->setFont (lnf.fonts->mono (26.0f));
        canvas.addAndMakeVisible (*l);
    }
    peakLabel.setBounds (m.getX(), m.getY(), m.getWidth(), 46);
    lufsLabel.setBounds (m.getX(), m.getY() + 54, m.getWidth(), 46);
    peakLabel.setTooltip ("Output peak (dBFS)");
    lufsLabel.setTooltip ("Output loudness, short-term (LUFS)");

    phoneButton.big = true;
    phoneButton.setBounds (m.getX(), m.getY() + 112, m.getWidth(), m.getHeight() - 112);
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

// Direct2D can leave the window blank on some Windows GPUs/drivers inside hosts,
// so the editor always uses JUCE's software renderer.
void K808Editor::useSoftwareRenderer()
{
   #if JUCE_WINDOWS
    if (auto* peer = getPeer())
        if (peer->getCurrentRenderingEngine() != 0 && peer->getAvailableRenderingEngines().size() > 1)
            peer->setCurrentRenderingEngine (0);
   #endif
}

void K808Editor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    useSoftwareRenderer();
}

void K808Editor::visibilityChanged()
{
    AudioProcessorEditor::visibilityChanged();
    useSoftwareRenderer();
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
    auto& undo = processor.undoManager;
    menu.addItem (1005, "Undo", undo.canUndo());
    menu.addItem (1006, "Redo", undo.canRedo());
    menu.addItem (1007, String ("Copy ") + (pm.isOnB() ? "B to A" : "A to B"));
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
                            else if (result == 1005)            processor.undoManager.undo();
                            else if (result == 1006)            processor.undoManager.redo();
                            else if (result == 1007)            presets.copyToOther();
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
    useSoftwareRenderer();

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

    if (++tunerTick >= 3)
    {
        tunerTick = 0;
        updateTuner();
    }

    refreshPresetLabel();
}

void K808Editor::updateTuner()
{
    auto& e = processor.engine;
    constexpr int n = 600;
    float buf[n];
    const auto w = e.tunerWrite.load (std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
        buf[i] = e.tunerRing[(size_t) ((w - n + i + Engine::tunerSize) % Engine::tunerSize)];

    float rms = 0.0f;
    for (auto v : buf) rms += v * v;
    rms = std::sqrt (rms / (float) n);

    const auto freq = rms > 0.004f ? detectPitch (buf, n, e.tunerRate) : 0.0f;

    for (auto& h : keyHistogram)
        h *= 0.997f;

    if (freq > 20.0f)
    {
        const auto midi = 69.0f + 12.0f * std::log2 (freq / 440.0f);
        const auto nearest = roundToInt (midi);
        const auto cents = roundToInt ((midi - (float) nearest) * 100.0f);
        const auto pc = ((nearest % 12) + 12) % 12;
        keyHistogram[(size_t) pc] += 1.0f;
        lastNote = String (noteNames[pc]) + String (nearest / 12 - 1) + " " + (cents >= 0 ? "+" : "") + String (cents) + " ct";
        noteHoldTicks = 6;
    }
    else if (noteHoldTicks > 0 && --noteHoldTicks == 0)
    {
        lastNote = {};
    }

    // key estimate (Krumhansl profiles) once enough notes were heard
    String key = "--";
    float total = 0.0f;
    for (auto h : keyHistogram) total += h;
    if (total > 15.0f)
    {
        static constexpr float major[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
        static constexpr float minor[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
        float best = -1.0e9f;
        for (int tonic = 0; tonic < 12; ++tonic)
            for (int mode = 0; mode < 2; ++mode)
            {
                float score = 0.0f;
                for (int k = 0; k < 12; ++k)
                    score += keyHistogram[(size_t) ((tonic + k) % 12)] * (mode == 0 ? major[k] : minor[k]);
                if (score > best)
                {
                    best = score;
                    key = String (noteNames[tonic]) + (mode == 0 ? " MAJ" : " MIN");
                }
            }
    }

    simple.setTunerText ("NOTE " + (lastNote.isEmpty() ? String ("--") : lastNote) + "     KEY " + key);
}
