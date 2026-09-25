#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace Colours808
{
    const Colour red       { 0xffcc1c15 };
    const Colour redBright { 0xffff5a46 };
    const Colour redGlow   { 0xffe0241c };
    const Colour track     { 0xff121110 };
    const Colour boxFill   { 0xff080808 };
    const Colour boxEdge   { 0xff2e2c29 };
    const Colour text      { 0xffe8e3d8 };
}

//==============================================================================
void KnobLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& slider)
{
    const auto& props = slider.getProperties();
    const auto small = (bool) props["small"];
    const auto arcR = (float) props["arcR"];
    const auto pIn = (float) props["pIn"];
    const auto pOut = (float) props["pOut"];

    const auto cx = (float) x + (float) w * 0.5f;
    const auto cy = (float) y + (float) h * 0.5f;
    const auto angle = startAngle + pos * (endAngle - startAngle);

    auto polar = [cx, cy] (float a, float r) { return Point<float> (cx + r * std::sin (a), cy - r * std::cos (a)); };

    if (! small)
    {
        Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (Colours808::track);
        g.strokePath (track, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::butt));

        if (pos > 0.002f)
        {
            Path value;
            value.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (Colours808::redGlow.withAlpha (0.22f));
            g.strokePath (value, PathStrokeType (18.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colours808::red);
            g.strokePath (value, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::butt));
            g.setColour (Colours808::redBright.withAlpha (0.7f));
            g.strokePath (value, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::butt));
        }

        const Line<float> pointer (polar (angle, pIn), polar (angle, pOut));
        g.setColour (Colours808::redGlow.withAlpha (0.25f));
        g.drawLine (pointer, 15.0f);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawLine (pointer.withShortenedStart (-1.0f).withShortenedEnd (-1.0f), 10.0f);
        g.setColour (Colours808::red);
        g.drawLine (pointer, 7.5f);
        g.setColour (Colour (0xffff7a66).withAlpha (0.85f));
        g.drawLine (pointer, 2.2f);
    }
    else
    {
        const Line<float> pointer (polar (angle, pIn), polar (angle, pOut));
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawLine (pointer, 6.0f);
        g.setColour (Colour (0xffe9e3d6));
        g.drawLine (pointer, 3.5f);
    }
}

Slider::SliderLayout KnobLookAndFeel::getSliderLayout (Slider& slider)
{
    if (slider.isHorizontal())
    {
        Slider::SliderLayout layout;
        layout.sliderBounds = slider.getLocalBounds().reduced (15, 0);
        return layout;
    }

    return LookAndFeel_V4::getSliderLayout (slider);
}

void KnobLookAndFeel::drawLinearSlider (Graphics& g, int x, int, int w, int, float pos, float, float,
                                        Slider::SliderStyle, Slider& slider)
{
    const auto cy = (float) slider.getHeight() * 0.5f + 1.0f;
    const Rectangle<float> groove ((float) x - 2.0f, cy - 9.0f, (float) w + 4.0f, 18.0f);

    g.setColour (Colour (0xff050505));
    g.fillRoundedRectangle (groove.expanded (1.5f), 9.5f);
    g.setGradientFill (ColourGradient (Colour (0xff1c1b1a), 0.0f, groove.getY(),
                                       Colour (0xff0c0c0c), 0.0f, groove.getBottom(), false));
    g.fillRoundedRectangle (groove, 9.0f);
    g.setColour (Colour (0x30ffffff));
    g.drawHorizontalLine ((int) groove.getBottom() - 1, groove.getX() + 8.0f, groove.getRight() - 8.0f);

    const Rectangle<float> fill (groove.getX() + 6.0f, cy - 2.0f, jmax (0.0f, pos - groove.getX() - 6.0f), 4.0f);
    g.setColour (Colours808::redGlow.withAlpha (0.25f));
    g.fillRoundedRectangle (fill.expanded (0.0f, 3.0f), 3.0f);
    g.setColour (Colours808::red);
    g.fillRoundedRectangle (fill, 2.0f);

    // metal fader cap
    const auto cap = Rectangle<float> (26.0f, 58.0f).withCentre ({ pos, cy });
    g.setColour (Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (cap.translated (3.0f, 4.0f).expanded (1.0f), 5.0f);

    ColourGradient body (Colour (0xffe6e1d6), cap.getX(), 0.0f, Colour (0xff57534d), cap.getRight(), 0.0f, false);
    body.addColour (0.45, Colour (0xffb3ada2));
    g.setGradientFill (body);
    g.fillRoundedRectangle (cap, 4.0f);

    const auto face = cap.reduced (5.0f, 8.0f);
    g.setGradientFill (ColourGradient (Colour (0xffd9d3c7), 0.0f, face.getY(), Colour (0xff8a847a), 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (face, 2.0f);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawLine (face.getX(), face.getY(), face.getRight(), face.getY(), 1.0f);

    g.setColour (Colour (0xff2c2a27));
    g.fillRect (Rectangle<float> (face.getWidth() - 2.0f, 2.0f).withCentre ({ pos, cy }));
    g.setColour (Colour (0xff1a1917));
    g.drawRoundedRectangle (cap, 4.0f, 1.2f);
}

//==============================================================================
void ValueBox::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (Colours808::boxFill);
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (Colours808::boxEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);
    // shrink the font if the text would not fit (e.g. "-24.0 dB")
    const auto text = getText();
    auto f = font;
    while (f.getHeight() > 8.0f && GlyphArrangement::getStringWidth (f, text) > (float) getWidth() - 8.0f)
        f = f.withHeight (f.getHeight() - 1.0f);

    g.setColour (Colours808::text);
    g.setFont (f);
    g.drawText (text, getLocalBounds(), Justification::centred, false);
}

//==============================================================================
LedButton::LedButton (const String& name, Point<float> ledCentreLocal, float ledSize)
    : Button (name), ledCentre (ledCentreLocal), size (ledSize)
{
    setClickingTogglesState (true);
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void LedButton::paintButton (Graphics& g, bool, bool)
{
    const auto on = getToggleState();
    const auto led = Rectangle<float> (size, size).withCentre (ledCentre);

    g.setColour (Colour (0xff060606));
    g.fillRoundedRectangle (led.expanded (3.5f), 3.0f);

    if (on)
    {
        const auto glowR = size * 2.2f;
        g.setGradientFill (ColourGradient (Colours808::redGlow.withAlpha (0.5f), ledCentre,
                                           Colours808::redGlow.withAlpha (0.0f), ledCentre.translated (glowR, 0.0f), true));
        g.fillEllipse (Rectangle<float> (glowR * 2.0f, glowR * 2.0f).withCentre (ledCentre));

        g.setGradientFill (ColourGradient (Colour (0xffffe0d6), ledCentre,
                                           Colour (0xffd0140c), ledCentre.translated (size * 0.62f, 0.0f), true));
        g.fillRoundedRectangle (led, 2.0f);
    }
    else
    {
        g.setGradientFill (ColourGradient (Colour (0xff4a0e0a), ledCentre,
                                           Colour (0xff170403), ledCentre.translated (size * 0.62f, 0.0f), true));
        g.fillRoundedRectangle (led, 2.0f);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.fillEllipse (led.reduced (size * 0.3f).translated (-size * 0.12f, -size * 0.12f));
    }
}

//==============================================================================
SegmentSelector::SegmentSelector (RangedAudioParameter& param, StringArray segmentNames,
                                  std::vector<int> edgesX, int topY, int bottomY, Font f)
    : names (std::move (segmentNames)), edges (std::move (edgesX)), top (topY), bottom (bottomY), font (f),
      attachment (param, [this] (float v) { selected = roundToInt (v); repaint(); }, nullptr)
{
    setBounds (edges.front() - margin, top - margin, edges.back() - edges.front() + 2 * margin, bottom - top + 2 * margin);
    setMouseCursor (MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

Rectangle<float> SegmentSelector::segment (int i) const
{
    const auto x0 = (float) (edges[(size_t) i] - edges.front() + margin);
    const auto x1 = (float) (edges[(size_t) i + 1] - edges.front() + margin);
    return { x0 + 1.0f, (float) margin, x1 - x0 - 2.0f, (float) (bottom - top) };
}

void SegmentSelector::paint (Graphics& g)
{
    g.setColour (Colour (0xff050505));
    g.fillRect (Rectangle<float> ((float) margin - 1.0f, (float) margin - 1.0f,
                                  (float) (edges.back() - edges.front()) + 2.0f, (float) (bottom - top) + 2.0f));

    for (int i = 0; i < names.size(); ++i)
    {
        const auto r = segment (i);

        if (i == selected)
            continue;

        g.setGradientFill (ColourGradient (Colour (0xff171716), 0.0f, r.getY(), Colour (0xff0a0a0a), 0.0f, r.getBottom(), false));
        g.fillRect (r);
        g.setColour (Colours::black);
        g.drawRect (r, 1.0f);
        g.setColour (Colour (0xff8b877f).withAlpha (0.5f));
        g.drawRect (r.reduced (3.0f), 1.2f);
        g.setColour (Colour (0xffd9d4ca));
        g.setFont (font);
        g.drawText (names[i], r, Justification::centred, false);
    }

    if (isPositiveAndBelow (selected, names.size()))
    {
        const auto r = segment (selected);

        for (int k = 3; k > 0; --k)
        {
            g.setColour (Colours808::redGlow.withAlpha (0.09f));
            g.fillRoundedRectangle (r.expanded ((float) k * 3.5f), 4.0f + (float) k * 2.0f);
        }

        g.setGradientFill (ColourGradient (Colour (0xffa3170f), r.getCentreX(), r.getCentreY(),
                                           Colour (0xff4d0604), r.getX(), r.getY(), true));
        g.fillRect (r);
        g.setColour (Colour (0xff2a0303));
        g.drawRect (r, 1.0f);
        g.setColour (Colour (0xffff4b3c));
        g.drawRect (r.reduced (3.0f), 1.6f);
        g.setColour (Colour (0xffffe5de));
        g.setFont (font);
        g.drawText (names[selected], r, Justification::centred, false);
    }
}

void SegmentSelector::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < names.size(); ++i)
    {
        if (segment (i).contains (e.position))
        {
            attachment.setValueAsCompleteGesture ((float) i);
            return;
        }
    }
}

//==============================================================================
namespace
{
    // dB -> y (design coordinates), matching the printed MASTER scale
    float meterY (float db)
    {
        static constexpr float dbs[] = { 3.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f, -60.0f };
        static constexpr float ys[]  = { 160.0f, 167.0f, 231.0f, 294.0f, 362.0f, 430.0f, 466.0f };

        if (db >= dbs[0]) return ys[0];
        for (int i = 1; i < 7; ++i)
            if (db >= dbs[i])
                return jmap (db, dbs[i], dbs[i - 1], ys[i], ys[i - 1]);
        return ys[6] + 1.0f;
    }

    const Rectangle<int> meterBounds { 1376, 156, 92, 314 };
    constexpr int barX[] = { 1380, 1400, 1429, 1448 };
}

void MasterMeter::setLevels (const float newLevelsDb[4])
{
    for (int i = 0; i < 4; ++i)
        levels[i] = newLevelsDb[i];
    repaint();
}

void MasterMeter::paint (Graphics& g)
{
    g.fillAll (Colour (0xff0b0b0b));

    const auto ox = (float) meterBounds.getX();
    const auto oy = (float) meterBounds.getY();

    for (int bar = 0; bar < 4; ++bar)
    {
        const auto levelY = meterY (levels[bar]);
        const auto isOut = bar >= 2;

        for (float y = 463.0f; y >= 160.0f; y -= 5.0f)
        {
            const Rectangle<float> slat ((float) barX[bar] - ox, y - oy, 16.0f, 3.2f);
            const auto lit = y + 1.5f >= levelY;

            if (! lit)
                g.setColour (Colour (0xff2a2927));
            else if (isOut && y < meterY (-12.0f))
                g.setColour (Colours808::red);
            else
                g.setColour (Colour (0xffd8d2c6));

            g.fillRect (slat);
        }
    }
}

//==============================================================================
Canvas::Canvas()
{
    background = ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);
    setOpaque (true);
}

void Canvas::paint (Graphics& g)
{
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImageAt (background, 0, 0);
}

//==============================================================================
K808Editor::K808Editor (K808Processor& p)
    : AudioProcessorEditor (p), processor (p)
{
    auto mono = Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
    auto sans = Typeface::createSystemTypefaceFor (BinaryData::ShareTechRegular_ttf, BinaryData::ShareTechRegular_ttfSize);
    monoFont = Font (FontOptions (mono));
    buttonFont = Font (FontOptions (sans).withHeight (22.0f));

    addAndMakeVisible (canvas);
    canvas.setBounds (0, 0, designWidth, designHeight);

    auto percent = [] (double v) { return String (roundToInt (v * 100.0)); };
    auto decibels = [] (double v) { return String (std::abs (v) < 0.05 ? 0.0 : v, 1) + " dB"; };

    // ---- big knobs
    addKnob (ParamIDs::punch,   { 570, 262 },  105.0f, 26.0f, 78.0f, false, { 537, 355, 66, 30 }, percent);
    addKnob (ParamIDs::sub,     { 796, 264 },  100.0f, 26.0f, 78.0f, false, { 763, 355, 66, 30 }, percent);
    addKnob (ParamIDs::distort, { 1018, 262 }, 101.0f, 26.0f, 78.0f, false, { 983, 355, 66, 30 }, percent);
    addKnob (ParamIDs::clip,    { 1237, 264 }, 99.0f,  26.0f, 78.0f, false, { 1202, 355, 66, 30 }, percent);

    // ---- switches
    addLed (ParamIDs::shortEnv, { 465, 454, 99, 57 },  { 538.0f, 484.0f }, 15.0f);
    addLed (ParamIDs::boost,    { 601, 454, 99, 57 },  { 673.0f, 481.0f }, 15.0f);
    addLed (ParamIDs::hardClip, { 749, 454, 117, 57 }, { 838.0f, 483.0f }, 15.0f);
    addLed (ParamIDs::grit,     { 914, 454, 100, 57 }, { 988.0f, 483.0f }, 15.0f);
    addLed (ParamIDs::lowMono,  { 1056, 454, 118, 57 }, { 1148.0f, 483.0f }, 15.0f);
    addLed (ParamIDs::cook,     { 1218, 454, 110, 57 }, { 1297.0f, 480.0f }, 15.0f);

    // ---- TYPE / MODE
    typeSelector = std::make_unique<SegmentSelector> (*processor.apvts.getParameter (ParamIDs::type),
                                                      K808Processor::typeNames,
                                                      std::vector<int> { 460, 590, 714, 838, 962, 1088, 1214, 1350, 1490 },
                                                      591, 648, buttonFont);
    modeSelector = std::make_unique<SegmentSelector> (*processor.apvts.getParameter (ParamIDs::mode),
                                                      K808Processor::modeNames,
                                                      std::vector<int> { 460, 655, 860, 1078, 1282, 1490 },
                                                      716, 771, buttonFont);
    canvas.addAndMakeVisible (*typeSelector);
    canvas.addAndMakeVisible (*modeSelector);

    // ---- MIX / OUTPUT faders
    addFader (ParamIDs::mix,    { 466, 852, 305, 70 }, { 773, 868, 58, 40 },
              [] (double v) { return String (roundToInt (v)); });
    addFader (ParamIDs::output, { 872, 852, 279, 70 }, { 1151, 868, 84, 40 }, decibels);

    // ---- LIMITER / CEILING
    addLed (ParamIDs::limiter, { 1279, 848, 69, 73 }, { 1320.0f, 878.0f }, 16.0f);
    addKnob (ParamIDs::ceiling, { 1424, 879 }, 0.0f, 6.0f, 30.0f, true, { 1384, 931, 94, 37 }, decibels);

    // ---- MASTER meter
    canvas.addAndMakeVisible (meter);
    meter.setBounds (meterBounds);

    auto readout = [] (float db) { return db <= -60.0f ? String ("-inf") : String (db, 1); };
    inReadout  = std::make_unique<ValueBox> ([this, readout] { return readout (readoutDb[0]); });
    outReadout = std::make_unique<ValueBox> ([this, readout] { return readout (readoutDb[1]); });
    for (auto* box : { inReadout.get(), outReadout.get() })
    {
        box->font = monoFont.withHeight (24.0f);
        canvas.addAndMakeVisible (*box);
    }
    inReadout->setBounds (1364, 503, 57, 29);
    outReadout->setBounds (1429, 503, 57, 29);

    setResizable (true, true);
    setResizeLimits (designWidth / 2, designHeight / 2, designWidth, designHeight);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    setSize (designWidth * 3 / 4, designHeight * 3 / 4);

    startTimerHz (30);
}

K808Editor::~K808Editor()
{
    stopTimer();
    for (auto& k : knobs)
        k->slider.setLookAndFeel (nullptr);
}

K808Editor::Knob& K808Editor::addKnob (const char* paramID, Point<int> centre, float arcRadius, float pointerIn,
                                       float pointerOut, bool small, Rectangle<int> boxBounds,
                                       std::function<String (double)> format)
{
    auto knob = std::make_unique<Knob>();
    auto& s = knob->slider;

    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity (220);
    s.setLookAndFeel (&lnf);
    s.getProperties().set ("small", small);
    s.getProperties().set ("arcR", arcRadius);
    s.getProperties().set ("pIn", pointerIn);
    s.getProperties().set ("pOut", pointerOut);

    const auto half = (int) (small ? pointerOut + 16.0f : arcRadius + 14.0f);
    s.setBounds (centre.x - half, centre.y - half, half * 2, half * 2);
    canvas.addAndMakeVisible (s);

    knob->attachment = std::make_unique<SliderAttachment> (processor.apvts, paramID, s);

    if (auto* param = processor.apvts.getParameter (paramID))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    knob->box = std::make_unique<ValueBox> ([&s, format] { return format (s.getValue()); });
    knob->box->font = monoFont.withHeight ((float) boxBounds.getHeight() * 0.8f);
    knob->box->setBounds (boxBounds);
    canvas.addAndMakeVisible (*knob->box);
    s.onValueChange = [box = knob->box.get()] { box->repaint(); };

    knobs.push_back (std::move (knob));
    return *knobs.back();
}

K808Editor::Knob& K808Editor::addFader (const char* paramID, Rectangle<int> bounds, Rectangle<int> boxBounds,
                                        std::function<String (double)> format)
{
    auto fader = std::make_unique<Knob>();
    auto& s = fader->slider;

    s.setSliderStyle (Slider::LinearHorizontal);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setLookAndFeel (&lnf);
    s.setBounds (bounds);
    canvas.addAndMakeVisible (s);

    fader->attachment = std::make_unique<SliderAttachment> (processor.apvts, paramID, s);

    if (auto* param = processor.apvts.getParameter (paramID))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    fader->box = std::make_unique<ValueBox> ([&s, format] { return format (s.getValue()); });
    fader->box->font = monoFont.withHeight ((float) boxBounds.getHeight() * 0.72f);
    fader->box->setBounds (boxBounds);
    canvas.addAndMakeVisible (*fader->box);
    s.onValueChange = [box = fader->box.get()] { box->repaint(); };

    knobs.push_back (std::move (fader));
    return *knobs.back();
}

void K808Editor::addLed (const char* paramID, Rectangle<int> bounds, Point<float> led, float ledSize)
{
    const auto area = bounds.expanded (22);
    auto button = std::make_unique<LedButton> (paramID, led - area.getPosition().toFloat(), ledSize);
    button->setBounds (area);
    canvas.addAndMakeVisible (*button);
    ledAttachments.push_back (std::make_unique<ButtonAttachment> (processor.apvts, paramID, *button));
    leds.push_back (std::move (button));
}

void K808Editor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void K808Editor::resized()
{
    canvas.setTransform (AffineTransform::scale ((float) getWidth() / (float) designWidth));
}

void K808Editor::timerCallback()
{
    const float peaks[4] = { processor.inPeak[0].exchange (0.0f), processor.inPeak[1].exchange (0.0f),
                             processor.outPeak[0].exchange (0.0f), processor.outPeak[1].exchange (0.0f) };

    for (int i = 0; i < 4; ++i)
    {
        const auto db = Decibels::gainToDecibels (peaks[i], -100.0f);
        meterDb[i] = db >= meterDb[i] ? db : jmax (db, meterDb[i] - 1.2f);
    }

    meter.setLevels (meterDb);

    if (++readoutCounter >= 6)
    {
        readoutCounter = 0;
        readoutDb[0] = jmax (meterDb[0], meterDb[1]);
        readoutDb[1] = jmax (meterDb[2], meterDb[3]);
        inReadout->repaint();
        outReadout->repaint();
    }
}
