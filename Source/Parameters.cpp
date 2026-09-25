#include "Parameters.h"

using namespace juce;

namespace
{
    String percent (float v) { return String (roundToInt (v * 100.0f)) + " %"; }

    NormalisableRange<float> logRange (float lo, float hi)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }

    struct Builder
    {
        AudioProcessorValueTreeState::ParameterLayout layout;

        void unit (const char* id, const char* name, float def)
        {
            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([] (float v, int) { return percent (v); })
                    .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() / 100.0f; })));
        }

        void range (const char* id, const char* name, NormalisableRange<float> r, float def,
                    std::function<String (float)> text)
        {
            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id, 1 }, name, r, def,
                AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([text] (float v, int) { return text (v); })
                    .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));
        }

        void db (const char* id, const char* name, float lo, float hi, float def)
        {
            range (id, name, NormalisableRange<float> (lo, hi, 0.1f), def,
                   [] (float v) { return (v > 0.04f ? "+" : "") + String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; });
        }

        void hz (const char* id, const char* name, float lo, float hi, float def)
        {
            range (id, name, logRange (lo, hi), def,
                   [] (float v) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; });
        }

        void toggle (const char* id, const char* name, bool def)
        {
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
        }

        void choice (const char* id, const char* name, const StringArray& items, int def)
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
        }
    };
}

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    Builder b;

    // ---- global
    b.choice (ParamIDs::style, "Style", Choices::styles, 0);
    b.unit (ParamIDs::kill, "Kill", 0.4f);
    b.db (ParamIDs::inGain, "Input", -24.0f, 24.0f, 0.0f);
    b.toggle (ParamIDs::bypass, "Bypass", false);

    // ---- pitch
    auto semis = [] (float v) { const auto r = std::round (v * 10.0f) / 10.0f; return (r > 0.04f ? "+" : "") + String (std::abs (r) < 0.05f ? 0.0f : r, 1) + " st"; };
    auto ms = [] (float v) { return String (roundToInt (v)) + " ms"; };
    b.toggle (ParamIDs::pitchOn, "Pitch On", true);
    b.range (ParamIDs::knock, "Knock", NormalisableRange<float> (0.0f, 12.0f, 0.1f), 0.0f, semis);
    b.range (ParamIDs::knockTime, "Knock Time", logRange (5.0f, 150.0f), 30.0f, ms);
    b.range (ParamIDs::dive, "Bend", NormalisableRange<float> (-24.0f, 0.0f, 0.1f), 0.0f, semis);
    b.range (ParamIDs::diveTime, "Bend Time", logRange (20.0f, 1500.0f), 250.0f, ms);
    b.range (ParamIDs::diveDelay, "Bend Delay", NormalisableRange<float> (0.0f, 1000.0f, 1.0f), 150.0f, ms);
    b.unit (ParamIDs::octDown, "Octave Down", 0.0f);
    b.unit (ParamIDs::octUp, "Octave Up", 0.0f);

    // ---- wobble
    b.toggle (ParamIDs::wobbleOn, "Wobble On", true);
    b.unit (ParamIDs::wobble, "Wobble", 0.0f);
    b.choice (ParamIDs::wobbleTarget, "Wobble Target", Choices::wobbleTargets, 0);
    b.choice (ParamIDs::wobbleRate, "Wobble Rate", Choices::wobbleRates, 6);
    b.choice (ParamIDs::wobbleShape, "Wobble Shape", Choices::wobbleShapes, 0);
    b.range (ParamIDs::wobbleFade, "Wobble Fade", NormalisableRange<float> (0.0f, 500.0f, 1.0f), 0.0f, ms);
    b.toggle (ParamIDs::wobbleRetrig, "Wobble Retrigger", true);

    // ---- shape
    b.toggle (ParamIDs::shapeOn, "Shape On", true);
    b.unit (ParamIDs::punch, "Punch", 0.3f);
    b.unit (ParamIDs::punchClick, "Click", 0.0f);
    b.range (ParamIDs::length, "Length", NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
             [] (float v) { const auto p = roundToInt (v * 100.0f); return (p > 0 ? "+" : "") + String (p) + " %"; });

    // ---- tone
    b.toggle (ParamIDs::toneOn, "Tone On", true);
    b.db (ParamIDs::sub, "Sub", -6.0f, 12.0f, 0.0f);
    b.unit (ParamIDs::harmonics, "Harmonics", 0.0f);
    b.toggle (ParamIDs::filterOn, "Filter On", false);
    b.hz (ParamIDs::cutoff, "Cutoff", 200.0f, 20000.0f, 20000.0f);
    b.unit (ParamIDs::resonance, "Resonance", 0.1f);
    b.choice (ParamIDs::slope, "Slope", Choices::slopes, 1);
    b.db (ParamIDs::tilt, "Tilt", -6.0f, 6.0f, 0.0f);

    // ---- dirt
    b.toggle (ParamIDs::dirtOn, "Dirt On", true);
    b.choice (ParamIDs::dirtMode, "Dirt Mode", Choices::dirtModes, 0);
    b.unit (ParamIDs::dirt, "Dirt", 0.3f);
    b.unit (ParamIDs::dirtMix, "Dirt Mix", 1.0f);
    b.toggle (ParamIDs::autoGain, "Auto Gain", true);
    b.choice (ParamIDs::oversample, "Oversampling", Choices::oversampling, 0);
    b.toggle (ParamIDs::cleanLow, "Clean Low", false);
    b.hz (ParamIDs::cleanFreq, "Clean Low Freq", 40.0f, 300.0f, 100.0f);
    b.range (ParamIDs::crushBits, "Crush", NormalisableRange<float> (4.0f, 24.0f, 1.0f), 24.0f,
             [] (float v) { return v >= 23.5f ? String ("Off") : String (roundToInt (v)) + " bit"; });
    b.hz (ParamIDs::postFilter, "Post Filter", 1000.0f, 20000.0f, 20000.0f);

    // ---- duck
    b.toggle (ParamIDs::duckOn, "Duck On", true);
    b.unit (ParamIDs::duck, "Duck", 0.0f);
    b.range (ParamIDs::duckRel, "Duck Release", logRange (20.0f, 600.0f), 120.0f,
             [] (float v) { return String (roundToInt (v)) + " ms"; });
    b.unit (ParamIDs::duckShape, "Duck Shape", 0.5f);

    // ---- output
    b.unit (ParamIDs::clipper, "Clipper", 0.3f);
    b.db (ParamIDs::ceiling, "Ceiling", -12.0f, 0.0f, -0.3f);
    b.range (ParamIDs::monoBelow, "Mono Below", NormalisableRange<float> (0.0f, 300.0f, 1.0f), 100.0f,
             [] (float v) { return v < 1.0f ? String ("Off") : String (roundToInt (v)) + " Hz"; });
    b.db (ParamIDs::outGain, "Output", -24.0f, 24.0f, 0.0f);
    b.range (ParamIDs::mix, "Mix", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
             [] (float v) { return String (roundToInt (v)) + " %"; });
    b.toggle (ParamIDs::phone, "Phone Check", false);

    return std::move (b.layout);
}

StringArray sectionParameters (const String& section)
{
    using namespace ParamIDs;

    if (section == "PITCH")  return { pitchOn, knock, knockTime, dive, diveTime, diveDelay, octDown, octUp };
    if (section == "WOBBLE") return { wobbleOn, wobble, wobbleTarget, wobbleRate, wobbleShape, wobbleFade, wobbleRetrig };
    if (section == "SHAPE")  return { shapeOn, punch, punchClick, length };
    if (section == "TONE")   return { toneOn, sub, harmonics, filterOn, cutoff, resonance, slope, tilt };
    if (section == "DIRT")   return { dirtOn, dirtMode, dirt, dirtMix, autoGain, oversample, cleanLow, cleanFreq, crushBits, postFilter };
    if (section == "DUCK")   return { duckOn, duck, duckRel, duckShape };
    if (section == "OUTPUT") return { clipper, ceiling, monoBelow, outGain, mix, phone, inGain };
    return {};
}
