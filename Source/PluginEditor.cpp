#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;
using namespace Look;

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

    const char* describeCategory (const String& category)
    {
        static const std::map<String, const char*> text = {
            { "SANCTUS",        "Clean and big. Sub, level and a touch of harmonics." },
            { "VELVET COFFIN",  "Soft, round clipping. Warm and smooth." },
            { "BRICKFACE",      "Hard modern clipping. Flat, loud, in your face." },
            { "JAWBREAKER",     "All about the hit at the start of every note." },
            { "+1000 AURA",     "Maximum loudness without falling apart." },
            { "CASSETTE GHOST", "Tape saturation: warm, worn and dusty." },
            { "FURNACE",        "Tube drive: thick, glowing harmonics." },
            { "TOXICUM",        "Biting resonant filter over the dirt." },
            { "MOSH PIT",       "Folded, aggressive rage distortion." },
            { "GRAVE DUST",     "Lo-fi: crushed bits and old dust." },
            { "VOMITORIUM",     "Total destruction. Use responsibly." },
            { "BRAINROT",       "Harmonics so the 808 is heard on phones and earbuds." },
            { "FAVOURITES",     "Presets you starred." },
            { "USER",           "Your own saved presets." },
        };
        const auto it = text.find (category);
        return it != text.end() ? it->second : "";
    }

    // buttons created later must not take keyboard focus either (space bar = DAW play)
    template <typename C>
    C* noFocus (C* c)
    {
        c->setWantsKeyboardFocus (false);
        c->setMouseClickGrabsKeyboardFocus (false);
        return c;
    }
}

//==============================================================================
HeadView::HeadView (K808Processor& p)
    : processor (p),
      killParam (*p.apvts.getParameter (ParamIDs::kill)),
      killAttachment (killParam, [this] (float v) { kill = v; repaint(); }, &p.undoManager),
      scopeIn (300, 0.0f), scopeOut (300, 0.0f)
{
    background = ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);
    jawImage = ImageCache::getFromMemory (BinaryData::jaw_png, BinaryData::jaw_pngSize);
    setOpaque (true);
    setTooltip ("Drag the jaw down = KILL: more of everything the preset does. "
                "Double-click = default, Ctrl / Cmd + drag = fine. The mouth shows your 808: grey = in, red = out.");
    killAttachment.sendInitialUpdate();
}

bool HeadView::hitsJaw (Point<float> pos) const
{
    const auto o = Layout::head.getPosition().toFloat();
    const auto mouth = Layout::mouth.toFloat() - o;
    const auto open = mouth.withBottom (mouth.getBottom() + jawDrop());
    const auto jaw = (Layout::jaw.toFloat() - o).translated (0.0f, jawDrop()).reduced (10.0f, 0.0f);
    return open.contains (pos) || jaw.contains (pos);
}

void HeadView::paint (Graphics& g)
{
    const auto o = Layout::head.getPosition().toFloat();
    g.drawImage (background, 0, 0, getWidth(), getHeight(), Layout::head.getX(), Layout::head.getY(), getWidth(), getHeight());

    const auto drop = jawDrop();
    const auto mouth = Layout::mouth.toFloat() - o;
    const auto inner = mouth.withBottom (mouth.getBottom() + drop).reduced (2.0f);

    // mouth: dark throat with red heat that grows with KILL
    g.setColour (Colour (0xff050505));
    g.fillRoundedRectangle (inner, 10.0f);
    g.setGradientFill (ColourGradient (Palette::redGlow.withAlpha (jlimit (0.0f, 1.0f, 0.08f + 0.32f * kill + 0.2f * beat)),
                                       inner.getCentreX(), inner.getBottom(),
                                       Palette::redGlow.withAlpha (0.0f), inner.getCentreX(), inner.getY() + inner.getHeight() * 0.3f, false));
    g.fillRoundedRectangle (inner, 10.0f);

    // waveform: input grey behind, output red in front, clipped parts bright
    const auto wave = inner.reduced (12.0f, 8.0f).withTrimmedTop (36.0f);
    const auto cy = wave.getCentreY(), half = wave.getHeight() * 0.5f;
    const auto ceiling = Decibels::decibelsToGain (processor.apvts.getRawParameterValue (ParamIDs::ceiling)->load());
    const auto n = (int) scopeOut.size();
    const auto colW = wave.getWidth() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        const auto v = scopeOut[(size_t) i];
        const auto a = jmin (1.0f, v) * half;
        g.setColour (v >= ceiling * 0.97f ? Colour (0xffff7a5c) : Palette::red.withAlpha (0.85f));
        g.fillRect (wave.getX() + (float) i * colW, cy - a, colW + 0.4f, 2.0f * a);
    }

    // input as a light outline on top, so you see how much bigger the 808 got
    Path in;
    for (int side = 0; side < 2; ++side)
    {
        const auto sign = side == 0 ? -1.0f : 1.0f;
        for (int i = 0; i < n; ++i)
        {
            const Point<float> pt (wave.getX() + ((float) i + 0.5f) * colW, cy + sign * jmin (1.0f, scopeIn[(size_t) i]) * half);
            if (i == 0) in.startNewSubPath (pt); else in.lineTo (pt);
        }
    }
    g.setColour (Colour (0xe0e8e3d8));
    g.strokePath (in, PathStrokeType (1.6f));
    g.setColour (Palette::redBright.withAlpha (0.35f));
    g.fillRect (wave.getX(), cy - ceiling * half, wave.getWidth(), 1.2f);
    g.fillRect (wave.getX(), cy + ceiling * half, wave.getWidth(), 1.2f);

    // top of the mouth: KILL amount (or the phone check warning)
    const auto top = inner.withHeight (38.0f).translated (0.0f, 4.0f);
    if (phoneOn)
    {
        g.setColour (Palette::red);
        g.fillRoundedRectangle (top.reduced (40.0f, 3.0f), 3.0f);
        g.setColour (Colours::white);
        g.setFont (fonts->bold (24.0f));
        g.drawText ("PHONE CHECK ON", top, Justification::centred, false);
    }
    else
    {
        g.setColour (Palette::text);
        g.setFont (fonts->bold (32.0f));
        g.drawText ("KILL " + String (roundToInt (kill * 100.0f)) + "%", top, Justification::centred, false);
    }

    // jaw
    const auto jaw = (Layout::jaw.toFloat() - o).translated (0.0f, drop);
    g.drawImage (jawImage, jaw);

    if (hover && ! dragging)
    {
        const auto hint = Rectangle<float> (200.0f, 34.0f).withCentre ({ jaw.getCentreX(), jaw.getY() + 70.0f });
        g.setColour (Colours::black.withAlpha (0.7f));
        g.fillRoundedRectangle (hint, 4.0f);
        g.setColour (Palette::text);
        g.setFont (fonts->bold (24.0f));
        g.drawText ("DRAG THE JAW", hint, Justification::centred, false);
    }

    // eyes glow with KILL and flash on every hit
    for (auto eye : { Point<float> (606.0f, 283.0f), Point<float> (852.0f, 283.0f) })
    {
        const auto c = eye - o;
        const auto r = 30.0f + 34.0f * kill + 12.0f * beat;
        const auto a = jlimit (0.0f, 1.0f, 0.1f + 0.5f * kill + 0.4f * beat);
        g.setGradientFill (ColourGradient (Palette::redGlow.withAlpha (a), c, Palette::redGlow.withAlpha (0.0f), c.translated (r, 0.0f), true));
        g.fillEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
        g.setColour (Colour (0xffffd6cc).withAlpha (jlimit (0.0f, 1.0f, 0.2f + 0.6f * kill + 0.3f * beat)));
        g.fillEllipse (Rectangle<float> (8.0f, 8.0f).withCentre (c));
    }
}

void HeadView::tick (float outLevel, bool phone)
{
    // the jaw "bites" on every new hit: level above its own slow average
    const auto hit = jlimit (0.0f, 1.0f, (outLevel - slowLevel) * 2.5f);
    slowLevel += (outLevel - slowLevel) * 0.12f;
    beat = jmax (hit, beat * 0.8f);
    phoneOn = phone;

    auto& e = processor.engine;
    const auto n = (int) scopeOut.size();
    const auto w = e.scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        const auto idx = (size_t) ((w - n + i + Engine::scopeSize) % Engine::scopeSize);
        scopeIn[(size_t) i] = e.scopeIn[idx].load (std::memory_order_relaxed);
        scopeOut[(size_t) i] = e.scopeOut[idx].load (std::memory_order_relaxed);
    }
    repaint();
}

void HeadView::mouseMove (const MouseEvent& e)
{
    const auto h = hitsJaw (e.position);
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h ? MouseCursor::UpDownResizeCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void HeadView::mouseExit (const MouseEvent&)
{
    hover = false;
    setMouseCursor (MouseCursor::NormalCursor);
    repaint();
}

void HeadView::mouseDown (const MouseEvent& e)
{
    if (! hitsJaw (e.position))
        return;
    dragging = true;
    dragStart = kill;
    killAttachment.beginGesture();
}

void HeadView::mouseDrag (const MouseEvent& e)
{
    if (! dragging)
        return;
    const auto fine = e.mods.isCtrlDown() || e.mods.isCommandDown();
    const auto v = jlimit (0.0f, 1.0f, dragStart + (float) e.getDistanceFromDragStartY() / (fine ? 1200.0f : 240.0f));
    killAttachment.setValueAsPartOfGesture (v);
}

void HeadView::mouseUp (const MouseEvent&)
{
    if (dragging)
        killAttachment.endGesture();
    dragging = false;
    repaint();
}

void HeadView::mouseDoubleClick (const MouseEvent& e)
{
    if (hitsJaw (e.position))
        killAttachment.setValueAsCompleteGesture (killParam.convertFrom0to1 (killParam.getDefaultValue()));
}

void HeadView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    if (hitsJaw (e.position))
        killAttachment.setValueAsCompleteGesture (jlimit (0.0f, 1.0f, kill + wheel.deltaY * 0.15f));
}

//==============================================================================
MasterPanel::MasterPanel (K808Processor& p)
    : autoLevel (p.apvts, ParamIDs::autoLevel, "AUTO LEVEL",
                 "Brings every 808 to the same level before the processing, so presets sound the same on quiet and loud samples."),
      phone (p.apvts, ParamIDs::phone, "PHONE", "Listen like on a phone speaker. Monitoring only: switch it off before you export!")
{
    addAndMakeVisible (meter);
    addAndMakeVisible (autoLevel);
    addAndMakeVisible (phone);
    for (auto& v : values)
        v = "--";
}

void MasterPanel::resized()
{
    meter.setBounds (4, 44, getWidth() - 8, 228);
    autoLevel.setBounds (4, getHeight() - 84, getWidth() - 8, 40);
    phone.setBounds (4, getHeight() - 42, getWidth() - 8, 40);
}

void MasterPanel::setValues (float peakDb, float lufs, float clipDb, const String& note, const String& key)
{
    const String next[5] = {
        peakDb <= -60.0f ? String ("-inf") : String (peakDb, 1),
        lufs <= -70.0f ? String ("--") : String (lufs, 1),
        clipDb < 0.05f ? String ("0.0") : "-" + String (clipDb, 1),
        note.isEmpty() ? String ("--") : note,
        key
    };

    bool changed = false;
    for (int i = 0; i < 5; ++i)
        if (values[i] != next[i]) { values[i] = next[i]; changed = true; }
    if (changed)
        repaint (0, 272, getWidth(), 160);
}

void MasterPanel::paint (Graphics& g)
{
    g.setColour (Palette::text);
    g.setFont (fonts->bold (32.0f));
    g.drawText ("MASTER", Rectangle<int> (0, 4, getWidth(), 38), Justification::centred, false);

    static const char* labels[] = { "PEAK", "LUFS", "CLIP", "NOTE", "KEY" };
    for (int i = 0; i < 5; ++i)
    {
        const Rectangle<float> row (6.0f, 276.0f + (float) i * 31.0f, (float) getWidth() - 12.0f, 29.0f);
        g.setColour (Colour (0xff050505));
        g.fillRect (row);
        g.setColour (Palette::textDim);
        g.setFont (fonts->sans (20.0f));
        g.drawText (labels[i], row.withTrimmedLeft (8.0f), Justification::centredLeft, false);
        g.setColour (i == 2 && values[i] != "0.0" ? Palette::redBright : Palette::text);
        g.setFont (fonts->mono (22.0f));
        g.drawText (values[i], row.withTrimmedRight (8.0f), Justification::centredRight, false);
    }
}

//==============================================================================
void PresetBrowser::Star::paintButton (Graphics& g, bool over, bool)
{
    const auto r = getLocalBounds().toFloat().reduced (10.0f);
    Path star;
    star.addStar (r.getCentre(), 5, r.getWidth() * 0.22f, r.getWidth() * 0.5f, 0.0f);
    if (on)
    {
        g.setColour (Palette::redBright);
        g.fillPath (star);
    }
    g.setColour (on ? Palette::redBright : Palette::textDim.withAlpha (over ? 1.0f : 0.6f));
    g.strokePath (star, PathStrokeType (1.6f));
}

namespace
{
    const Rectangle<int> browserPanel { 150, 100, 1236, 824 };
    constexpr int gridX = 500, gridY = 220, gridW = 862, gridBottom = 844;
}

PresetBrowser::PresetBrowser (K808Processor& p) : processor (p)
{
    categoryNames = PresetManager::categories();
    categoryNames.add ("FAVOURITES");
    categoryNames.add ("USER");

    for (int i = 0; i < categoryNames.size(); ++i)
    {
        auto* b = noFocus (categoryButtons.add (new UI::FlatButton (categoryNames[i])));
        b->textHeight = 25.0f;
        b->setBounds (browserPanel.getX() + 20, browserPanel.getY() + 80 + i * 50, 300, 46);
        b->onClick = [this, name = categoryNames[i]] { showCategory (name); };
        addAndMakeVisible (b);
    }

    const StringArray actions { "SAVE AS", "INIT", "UNDO", "REDO", "FOLDER" };
    for (int i = 0; i < actions.size(); ++i)
    {
        auto* b = noFocus (actionButtons.add (new UI::FlatButton (actions[i])));
        b->textHeight = 24.0f;
        b->setBounds (gridX + i * 174, browserPanel.getBottom() - 68, 162, 50);
        addAndMakeVisible (b);
    }
    actionButtons[0]->setTooltip ("Save the current sound as your own preset");
    actionButtons[1]->setTooltip ("Reset every control to its default");
    actionButtons[4]->setTooltip ("Open the folder with your presets");
    actionButtons[0]->onClick = [this] { setVisible (false); if (onSaveAs) onSaveAs(); };
    actionButtons[1]->onClick = [this] { processor.presets.init(); if (onChange) onChange(); rebuild(); };
    actionButtons[2]->onClick = [this] { processor.undoManager.undo(); if (onChange) onChange(); };
    actionButtons[3]->onClick = [this] { processor.undoManager.redo(); if (onChange) onChange(); };
    actionButtons[4]->onClick = [] { PresetManager::userFolder().createDirectory(); PresetManager::userFolder().startAsProcess(); };

    noFocus (&closeButton);
    closeButton.textHeight = 28.0f;
    closeButton.setBounds (browserPanel.getRight() - 72, browserPanel.getY() + 16, 56, 50);
    closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closeButton);
}

void PresetBrowser::open()
{
    auto& pm = processor.presets;
    pm.rescan();
    const auto index = pm.getCurrentIndex();
    const auto& list = pm.getPresets();
    category = isPositiveAndBelow (index, list.size()) ? list.getReference (index).category : categoryNames[0];
    setVisible (true);
    toFront (false);
    rebuild();
}

void PresetBrowser::showCategory (const String& name)
{
    category = name;
    rebuild();
}

void PresetBrowser::rebuild()
{
    presetButtons.clear();
    stars.clear();

    auto& pm = processor.presets;
    const auto& list = pm.getPresets();
    const auto current = pm.getCurrentIndex();

    Array<int> shown;
    for (int i = 0; i < list.size(); ++i)
    {
        const auto& info = list.getReference (i);
        if (category == "FAVOURITES" ? pm.isFavourite (info.name) : info.category == category)
            shown.add (i);
    }

    const auto cols = shown.size() > 20 ? 3 : 2;
    const auto rows = (gridBottom - gridY) / 62;
    const auto colW = (gridW - (cols - 1) * 14) / cols;

    for (int k = 0; k < jmin (shown.size(), cols * rows); ++k)
    {
        const auto index = shown[k];
        const auto name = list.getReference (index).name;
        const auto x = gridX + (k / rows) * (colW + 14);
        const auto y = gridY + (k % rows) * 62;

        auto* b = noFocus (presetButtons.add (new UI::FlatButton (name)));
        b->textHeight = 26.0f;
        b->active = index == current;
        b->setBounds (x, y, colW - 56, 54);
        b->onClick = [this, index]
        {
            processor.presets.load (index);
            if (onChange) onChange();
            // rebuilding deletes the button that is still inside its click handler, so do it afterwards
            MessageManager::callAsync ([sp = SafePointer<PresetBrowser> (this)] { if (sp != nullptr) sp->rebuild(); });
        };
        addAndMakeVisible (b);

        auto* s = noFocus (stars.add (new Star()));
        s->on = pm.isFavourite (name);
        s->setTooltip ("Add to / remove from FAVOURITES");
        s->setBounds (x + colW - 52, y, 52, 54);
        s->onClick = [this, s, name]
        {
            processor.presets.toggleFavourite (name);
            s->on = processor.presets.isFavourite (name);
            s->repaint();
            if (category == "FAVOURITES")
                MessageManager::callAsync ([sp = SafePointer<PresetBrowser> (this)] { if (sp != nullptr) sp->rebuild(); });
        };
        addAndMakeVisible (s);
    }

    for (int i = 0; i < categoryButtons.size(); ++i)
    {
        categoryButtons[i]->active = categoryNames[i] == category;
        categoryButtons[i]->repaint();
    }
    repaint();
}

void PresetBrowser::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.72f));

    const auto panel = browserPanel.toFloat();
    g.setColour (Colour (0xf50b0b0a));
    g.fillRect (panel);
    g.setColour (Palette::red.withAlpha (0.8f));
    g.drawRect (panel, 2.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawRect (panel.reduced (5.0f), 1.0f);

    g.setColour (Palette::text);
    g.setFont (fonts->bold (46.0f));
    g.drawText ("PRESETS", Rectangle<float> (panel.getX() + 22.0f, panel.getY() + 14.0f, 300.0f, 56.0f), Justification::centredLeft, false);

    g.setColour (Palette::redBright);
    g.setFont (fonts->bold (44.0f));
    g.drawText (category, Rectangle<float> ((float) gridX, panel.getY() + 14.0f, 700.0f, 56.0f), Justification::centredLeft, false);
    g.setColour (Palette::text);
    g.setFont (fonts->sans (26.0f));
    g.drawText (describeCategory (category), Rectangle<float> ((float) gridX, panel.getY() + 68.0f, (float) gridW, 40.0f),
                Justification::centredLeft, true);

    if (presetButtons.isEmpty())
    {
        g.setColour (Palette::textDim);
        g.setFont (fonts->sans (26.0f));
        g.drawText (category == "FAVOURITES" ? "Click the star next to a preset to add it here."
                                             : "Nothing here yet. Use SAVE AS to store your own sound.",
                    Rectangle<float> ((float) gridX, (float) gridY, (float) gridW, 60.0f), Justification::centredLeft, true);
    }
}

void PresetBrowser::mouseDown (const MouseEvent& e)
{
    if (! browserPanel.toFloat().contains (e.position))
        setVisible (false);
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
    k->setKnobArea ({ 90 + col * 210, 220 + row * 300, 140, 140 }, 38.0f);
    tabs[(size_t) tab].controls.push_back (k);
    return *k;
}

AdvancedPage::AdvancedPage (K808Processor& p) : processor (p)
{
    using namespace ParamIDs;
    auto& s = p.apvts;

    tabs = {
        { "KICK",     {},      "KICK: right = a hit made from the note itself, left = soften the hit that is there.  "
                               "DROP: the hit falls from high above into the note.  PROTECT: the hit skips the dirt.", {} },
        { "PITCH",    pitchOn, "KNOCK = pitch hit at the start of each note.  BEND = trap dive after BEND DELAY.  "
                               "OCT DOWN / OCT UP = extra octave layers.", {} },
        { "SHAPE",    shapeOn, {}, {} },
        { "TONE",     toneOn,  {}, {} },
        { "DIRT",     dirtOn,  {}, {} },
        { "OUTPUT",   {},      {}, {} },
        { "SETTINGS", {},      {}, {} },
    };

    for (int i = 0; i < (int) tabs.size(); ++i)
    {
        auto* b = tabButtons.add (new UI::FlatButton (tabs[(size_t) i].name));
        b->textHeight = 28.0f;
        b->onClick = [this, i] { showTab (i); };
        b->setBounds (12 + i * 140, 14, 134, 52);
        addAndMakeVisible (b);

        if (tabs[(size_t) i].sectionParam.isNotEmpty())
        {
            sectionToggles[i] = std::make_unique<UI::LedToggle> (s, tabs[(size_t) i].sectionParam, "ON", "Switch this whole section on or off.");
            sectionToggles[i]->setBounds (850, 84, 140, 52);
            addChildComponent (*sectionToggles[i]);
        }
    }

    closeButton.setBounds (1004, 14, 140, 52);
    closeButton.textHeight = 28.0f;
    closeButton.setTooltip ("Back to the head");
    addAndMakeVisible (closeButton);

    resetButton.setBounds (1004, 84, 140, 52);
    resetButton.textHeight = 29.0f;
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

    // ---- KICK
    knob (0, 0, 0, kick, "KICK", "Right = a hit made from the note itself, left = soften the hit that is there.", true);
    knob (0, 1, 0, kickDrop, "DROP", "The hit falls from high above into the note (the spin hit).");
    knob (0, 2, 0, kickLength, "LENGTH", "How long the hit lasts.");
    knob (0, 3, 0, kickBite, "BITE", "How much the hit bites (harmonics on the hit only).");
    add<UI::LedToggle> (0, { 40, 540, 320, 62 }, s, kickProtect, "PROTECT", "The hit skips the dirt, so it stays as punchy as the original.");

    // ---- PITCH
    knob (1, 0, 0, knock, "KNOCK", "Pitch hit at the start of every note (semitones above the note).");
    knob (1, 1, 0, knockTime, "KNOCK TIME", "How fast the knock falls back to the note.");
    knob (1, 2, 0, dive, "BEND", "Trap dive: how far each note drops (semitones).").getSlider().getProperties().set ("fromEnd", true);
    knob (1, 3, 0, diveTime, "BEND TIME", "How long the dive takes.");
    knob (1, 4, 0, diveDelay, "BEND DELAY", "How long after the note starts the dive begins.");
    knob (1, 0, 1, octDown, "OCT DOWN", "Adds a sub one octave below the 808.");
    knob (1, 1, 1, octUp, "OCT UP", "Adds a gritty octave above the 808.");
    for (auto* c : tabs[1].controls)
        if (auto* k = dynamic_cast<UI::Knob*> (c); k != nullptr && k->getY() > 400)
            k->setTopLeftPosition (k->getX(), k->getY() - 60);

    // ---- SHAPE
    knob (2, 1, 0, punch, "PUNCH", "Boosts the attack of every note.");
    knob (2, 2, 0, punchClick, "CLICK", "Adds a short click on top of each hit so it cuts through.");
    knob (2, 3, 0, length, "TAIL", "Negative = shorter notes (gate). Positive = longer, fuller tails.", true);

    // ---- TONE
    knob (3, 0, 0, sub, "SUB", "Low shelf around 55 Hz: more or less sub.", true);
    knob (3, 1, 0, harmonics, "HEAT", "Adds upper harmonics so the 808 is heard on small speakers.");
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

    // ---- OUTPUT
    knob (5, 0, 0, inGain, "INPUT", "Level going into the plugin.", true);
    knob (5, 1, 0, clipper, "CLIPPER", "Soft to hard clipping at the ceiling. 0 = off.");
    knob (5, 2, 0, ceiling, "CEILING", "Maximum output level of the clipper.");
    knob (5, 3, 0, outGain, "OUTPUT", "Output level.", true);
    knob (5, 4, 0, mix, "MIX", "Dry / wet mix of the whole plugin.");
    knob (5, 0, 1, monoBelow, "MONO BELOW", "Makes everything below this frequency mono. 0 = off.");
    knob (5, 1, 1, width, "WIDTH", "Widens the upper part of the 808. The sub stays mono.");
    add<UI::LedToggle> (5, { 760, 600, 340, 62 }, s, autoLevel, "AUTO LEVEL",
                        "Brings every 808 to the same level before the processing.");

    // ---- SETTINGS
    openFolderButton.setBounds (40, 360, 420, 62);
    openFolderButton.textHeight = 23.0f;
    openFolderButton.onClick = [] { PresetManager::userFolder().createDirectory(); PresetManager::userFolder().startAsProcess(); };
    addChildComponent (openFolderButton);
    tabs[6].controls.push_back (&openFolderButton);
    add<UI::ChoiceSelector> (6, { 40, 500, 1080, 62 }, s, ParamIDs::style,
                             "How the KILL jaw pushes the sound: drive, punch, sub and clipper in different amounts.");

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

    if (tab.note.isNotEmpty())
    {
        const Rectangle<float> box (40.0f, 660.0f, r.getWidth() - 80.0f, 180.0f);
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
        drawLabel (g, "KILL CHARACTER", { 40.0f, 446.0f, 600.0f, 46.0f }, 36.0f, Palette::ink, Justification::centredLeft);
    }
}

AdvancedOverlay::AdvancedOverlay (K808Processor& p) : page (p)
{
    plaster = ImageCache::getFromMemory (BinaryData::plaster_jpg, BinaryData::plaster_jpgSize);
    page.setBounds (Layout::advanced);
    page.closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (page);
}

void AdvancedOverlay::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.7f));
    g.drawImage (plaster, Layout::advanced.toFloat());
}

void AdvancedOverlay::mouseDown (const MouseEvent& e)
{
    if (! Layout::advanced.toFloat().contains (e.position))
        setVisible (false);
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
}

//==============================================================================
K808Editor::K808Editor (K808Processor& p)
    : AudioProcessorEditor (p), processor (p),
      head (p), master (p),
      kick   (p.apvts, ParamIDs::kick,      "KICK",   "The hit at the start of every note. Right = kicks like a drum, left = soft, only sub. "
                                                          "Turns a plain sub into an 808 that kicks. More in EDIT > KICK.", true),
      drop   (p.apvts, ParamIDs::kickDrop,  "DROP",   "The hit falls from high above into the note (the spin hit of kit 808s)."),
      sub    (p.apvts, ParamIDs::sub,       "SUB",    "How much sub (low end around 55 Hz) the 808 gets."),
      heat   (p.apvts, ParamIDs::harmonics, "HEAT",   "Harmonics so the 808 is heard on phones and small speakers."),
      output (p.apvts, ParamIDs::outGain,   "OUTPUT", "Output level.", true),
      mix    (p.apvts, ParamIDs::mix,       "MIX",    "Blend the original 808 (left) with the processed one (right)."),
      browser (p), advanced (p)
{
    setLookAndFeel (&lnf);
    tooltips.setLookAndFeel (&lnf);

    addAndMakeVisible (canvas);
    canvas.setBounds (0, 0, designWidth, designHeight);

    head.setBounds (Layout::head);
    canvas.addAndMakeVisible (head);

    master.setBounds (Layout::leftPanel);
    canvas.addAndMakeVisible (master);

    // right panel: 2 x 3 knobs
    UI::Knob* knobs[] = { &kick, &drop, &sub, &heat, &output, &mix };
    for (int i = 0; i < 6; ++i)
    {
        const auto x = Layout::rightPanel.getX() + (i % 2) * 115 + 20;
        const auto y = Layout::rightPanel.getY() + (i / 2) * 172 + 36;
        knobs[i]->setKnobArea ({ x, y, 75, 75 }, 26.0f);
        knobs[i]->setDark();
        canvas.addAndMakeVisible (*knobs[i]);
    }

    // bottom bar: < preset > A/B SAVE EDIT
    const auto bar = Layout::bottomBar;
    const auto y = bar.getY() + 10, h = bar.getHeight() - 20;
    prevButton.setBounds (bar.getX() + 8, y, 54, h);
    presetButton.setBounds (bar.getX() + 66, y, 328, h);
    nextButton.setBounds (bar.getX() + 398, y, 54, h);
    abButton.setBounds (bar.getX() + 460, y, 58, h);
    saveButton.setBounds (bar.getX() + 522, y, 78, h);
    editButton.setBounds (bar.getX() + 604, y, 78, h);

    for (auto* b : { &prevButton, &presetButton, &nextButton, &abButton, &saveButton, &editButton })
    {
        b->textHeight = 26.0f;
        canvas.addAndMakeVisible (*b);
    }
    presetButton.textHeight = 25.0f;

    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    presetButton.setTooltip ("Open the preset table");
    saveButton.setTooltip ("Save the current sound as your own preset");
    abButton.setTooltip ("A/B compare: switch between two versions of your settings");
    editButton.setTooltip ("Open the skull: every parameter");

    prevButton.onClick = [this] { processor.presets.loadNext (-1); refreshPresetLabel(); };
    nextButton.onClick = [this] { processor.presets.loadNext (1); refreshPresetLabel(); };
    presetButton.onClick = [this] { browser.open(); };
    saveButton.onClick = [this] { savePresetAs(); };
    editButton.onClick = [this] { advanced.setVisible (true); advanced.toFront (false); };
    abButton.onClick = [this]
    {
        processor.presets.toggleAB();
        abButton.setButtonText (processor.presets.isOnB() ? "B" : "A");
        abButton.active = processor.presets.isOnB();
        abButton.repaint();
    };

    // overlays
    browser.setBounds (0, 0, designWidth, designHeight);
    browser.onChange = [this] { shownPreset = {}; refreshPresetLabel(); };
    browser.onSaveAs = [this] { savePresetAs(); };
    canvas.addChildComponent (browser);

    advanced.setBounds (0, 0, designWidth, designHeight);
    canvas.addChildComponent (advanced);

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
    std::function<void (Component&)> clearFocus = [&clearFocus] (Component& c)
    {
        c.setWantsKeyboardFocus (false);
        c.setMouseClickGrabsKeyboardFocus (false);
        for (auto* child : c.getChildren())
            clearFocus (*child);
    };
    clearFocus (*this);

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

void K808Editor::refreshPresetLabel()
{
    const auto name = processor.presets.getCurrentName();
    const auto modified = processor.presets.isModified();
    if (name == shownPreset && modified == shownModified)
        return;

    shownPreset = name;
    shownModified = modified;

    const auto index = processor.presets.getCurrentIndex();
    const auto& list = processor.presets.getPresets();
    auto category = isPositiveAndBelow (index, list.size()) ? list.getReference (index).category : String();
    if (category.isEmpty() || category == name.toUpperCase())
        category = {};
    else
        category += " / ";
    presetButton.setButtonText (category + name.toUpperCase() + (modified ? " *" : ""));
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
    master.meter.setLevels (meterDb[0], meterDb[1]);

    const auto outDb = Decibels::gainToDecibels (peaks[1], -100.0f);
    if (outDb >= peakHoldDb || --peakHoldTicks <= 0)
    {
        peakHoldDb = outDb;
        peakHoldTicks = 45;
    }

    const auto clip = e.clipDb.exchange (0.0f);
    if (clip >= clipHoldDb || --clipHoldTicks <= 0)
    {
        clipHoldDb = clip;
        clipHoldTicks = 30;
    }

    if (++tunerTick >= 3)
    {
        tunerTick = 0;
        updateTuner();
    }

    master.setValues (peakHoldDb, e.shortTermLufs.load(), clipHoldDb, lastNote, lastKey);

    const auto phoneOn = processor.apvts.getRawParameterValue (ParamIDs::phone)->load() > 0.5f;
    head.tick (peaks[1], phoneOn);

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
        lastNote = String (noteNames[pc]) + String (nearest / 12 - 1) + " " + (cents >= 0 ? "+" : "") + String (cents);
        noteHoldTicks = 6;
    }
    else if (noteHoldTicks > 0 && --noteHoldTicks == 0)
    {
        lastNote = {};
    }

    // key estimate (Krumhansl profiles) once enough notes were heard
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
                    lastKey = String (noteNames[tonic]) + (mode == 0 ? " MAJ" : " MIN");
                }
            }
    }
}
