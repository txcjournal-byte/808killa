#include "Presets.h"
#include "Parameters.h"
#include "DSP/Engine.h"

using namespace juce;

namespace
{
    const Identifier presetProperty { "presetName" };

    struct Factory
    {
        const char* name;
        const char* category;
        int style;
        std::vector<std::pair<const char*, float>> values;
    };

    // KILL sits at this value after loading; the preset values below describe
    // the sound at that position, so the macro starts partway up.
    constexpr float presetKill = 0.3f;

    using namespace ParamIDs;

    const std::vector<Factory>& factoryPresets()
    {
        static const std::vector<Factory> list = {
            // ---------------- STYLES
            { "Atlanta Clean", "Styles", 0, { { length, 0.2f }, { punch, 0.75f }, { punchClick, 0.2f }, { dirtMode, 0 }, { dirt, 0.15f },
                                               { cleanLow, 1 }, { clipper, 0.25f }, { sub, 2.0f }, { knock, 5.0f }, { knockTime, 25.0f } } },
            { "Memphis Phonk", "Styles", 1, { { length, -0.2f }, { punch, 0.45f }, { dirtMode, 2 }, { dirt, 0.75f }, { crushBits, 12 },
                                               { filterOn, 1 }, { cutoff, 4000.0f }, { clipper, 0.5f }, { sub, 0.0f }, { wobble, 0.15f }, { wobbleTarget, 0 }, { wobbleRate, 1 } } },
            { "Rage Underground", "Styles", 2, { { punch, 0.45f }, { dirtMode, 4 }, { dirt, 1.0f }, { clipper, 0.8f }, { sub, 3.0f } } },
            { "Detroit Clip", "Styles", 3, { { length, -0.3f }, { punch, 0.75f }, { dirtMode, 1 }, { dirt, 0.85f }, { clipper, 0.95f }, { sub, 1.0f } } },
            { "Drill Chicago", "Styles", 4, { { length, -0.4f }, { punch, 0.75f }, { punchClick, 0.3f }, { dirtMode, 0 }, { dirt, 0.5f },
                                               { filterOn, 1 }, { cutoff, 8000.0f }, { clipper, 0.5f }, { sub, 2.0f }, { knock, 6.0f }, { knockTime, 30.0f } } },
            { "Drill NY", "Styles", 5, { { length, -0.25f }, { punch, 0.75f }, { dirtMode, 1 }, { dirt, 0.6f }, { clipper, 0.75f }, { sub, 2.0f } } },
            { "Drill UK", "Styles", 6, { { punch, 0.45f }, { dirtMode, 2 }, { dirt, 0.4f }, { clipper, 0.5f }, { sub, 2.0f } } },
            { "Plugg Soft", "Styles", 7, { { punch, 0.15f }, { dirtMode, 0 }, { dirt, 0.05f }, { filterOn, 1 }, { cutoff, 3000.0f },
                                            { clipper, 0.0f }, { sub, 3.0f } } },
            { "Chicago Boom", "Styles", 8, { { length, 0.3f }, { punch, 0.15f }, { dirtMode, 2 }, { dirt, 0.35f }, { crushBits, 14 },
                                              { filterOn, 1 }, { cutoff, 7000.0f }, { clipper, 0.25f }, { sub, 3.0f } } },
            { "Classic Trap Boom", "Styles", 9, { { length, 0.2f }, { punch, 0.45f }, { dirtMode, 0 }, { dirt, 0.5f }, { filterOn, 1 },
                                                   { cutoff, 8000.0f }, { clipper, 0.25f }, { sub, 3.0f } } },

            // ---------------- TECHNICAL
            { "Clean Sub", "Clean", 0, { { dirtOn, 0 }, { sub, 6.0f }, { harmonics, 0.4f }, { punch, 0.2f }, { clipper, 0.1f } } },
            { "Knock Punch", "Clean", 0, { { punch, 0.9f }, { punchClick, 0.6f }, { dirtMode, 0 }, { dirt, 0.15f }, { clipper, 0.3f }, { knock, 7.0f }, { knockTime, 35.0f } } },
            { "Plugg Bounce", "Clean", 7, { { length, -0.3f }, { punch, 0.3f }, { dirtMode, 3 }, { dirt, 0.15f }, { clipper, 0.1f }, { knock, 12.0f }, { knockTime, 25.0f } } },
            { "Phone Punch", "Clean", 9, { { harmonics, 0.85f }, { dirtMode, 0 }, { dirt, 0.3f }, { sub, 1.0f }, { clipper, 0.3f } } },
            { "Dirty Knock", "Dirty", 3, { { dirtMode, 1 }, { dirt, 0.7f }, { cleanLow, 1 }, { punch, 0.75f }, { punchClick, 0.3f }, { clipper, 0.6f } } },
            { "Lo-Fi Muffle", "Dirty", 1, { { filterOn, 1 }, { cutoff, 1500.0f }, { dirtMode, 2 }, { dirt, 0.5f }, { crushBits, 10 }, { clipper, 0.3f }, { wobble, 0.1f }, { wobbleTarget, 0 }, { wobbleRate, 1 } } },
            { "Slime Bend", "Pitch", 2, { { dive, -12.0f }, { diveDelay, 220.0f }, { diveTime, 90.0f }, { dirtMode, 1 }, { dirt, 0.8f },
                                           { octDown, 0.25f }, { clipper, 0.7f } } },
            { "Dive Bomb", "Pitch", 9, { { dive, -24.0f }, { diveDelay, 80.0f }, { diveTime, 700.0f }, { dirtMode, 0 }, { dirt, 0.45f }, { clipper, 0.4f } } },
            { "Sub Octave", "Pitch", 0, { { octDown, 0.6f }, { dirtMode, 0 }, { dirt, 0.25f }, { clipper, 0.3f } } },
            { "Octave Grit", "Pitch", 2, { { octUp, 0.5f }, { dirtMode, 2 }, { dirt, 0.6f }, { clipper, 0.6f } } },
            { "Wobble Wave", "FX", 9, { { wobble, 0.6f }, { wobbleTarget, 3 }, { wobbleRate, 6 }, { wobbleFade, 100.0f }, { dirtMode, 0 }, { dirt, 0.4f } } },
            { "Triplet Wub", "FX", 1, { { wobble, 0.8f }, { wobbleTarget, 2 }, { wobbleRate, 4 }, { wobbleShape, 1 }, { dirtMode, 2 }, { dirt, 0.6f } } },
            { "Tape Drop", "FX", 8, { { dive, -24.0f }, { diveDelay, 0.0f }, { diveTime, 1200.0f }, { dirtMode, 2 }, { dirt, 0.35f } } },
            { "Motor City Chop", "FX", 3, { { length, -0.5f }, { dirtMode, 1 }, { dirt, 0.5f }, { punch, 0.6f }, { clipper, 0.6f } } },
        };
        return list;
    }

    File presetFileFor (const String& name)
    {
        return PresetManager::userFolder().getChildFile (File::createLegalFileName (name) + ".808k");
    }
}

//==============================================================================
PresetManager::PresetManager (AudioProcessorValueTreeState& state) : apvts (state)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->getParameterID(), this);
    rescan();
}

PresetManager::~PresetManager()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            apvts.removeParameterListener (rp->getParameterID(), this);
}

File PresetManager::userFolder()
{
   #if JUCE_MAC
    return File::getSpecialLocation (File::userMusicDirectory).getChildFile ("808 KILLA/Presets");
   #else
    return File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("808 KILLA/Presets");
   #endif
}

void PresetManager::rescan()
{
    presets.clear();

    for (auto& f : factoryPresets())
        presets.add ({ f.name, f.category, true, {} });

    for (auto& file : userFolder().findChildFiles (File::findFiles, false, "*.808k"))
        presets.add ({ file.getFileNameWithoutExtension(), "User", false, file });
}

void PresetManager::parameterChanged (const String& id, float)
{
    // may be called from the audio thread (automation): only set a flag, the UI polls it
    if (! loading.load() && id != ParamIDs::bypass && id != ParamIDs::phone)
        modified = true;
}

void PresetManager::applyValues (const NamedValueSet& values)
{
    loading = true;

    for (auto* p : apvts.processor.getParameters())
    {
        auto* rp = dynamic_cast<RangedAudioParameter*> (p);
        if (rp == nullptr || rp->getParameterID() == ParamIDs::bypass || rp->getParameterID() == ParamIDs::phone)
            continue;

        const auto id = rp->getParameterID();
        const auto normalised = values.contains (id) ? rp->convertTo0to1 ((float) values[id]) : rp->getDefaultValue();
        rp->beginChangeGesture();
        rp->setValueNotifyingHost (normalised);
        rp->endChangeGesture();
    }

    loading = false;
    modified = false;
}

void PresetManager::load (int index)
{
    if (! isPositiveAndBelow (index, presets.size()))
        return;

    const auto& info = presets.getReference (index);
    NamedValueSet values;

    if (info.factory)
    {
        const auto& f = factoryPresets()[(size_t) index];
        const auto km = killMappingFor (f.style);
        values.set (ParamIDs::style, f.style);
        values.set (ParamIDs::kill, presetKill);

        for (auto& [id, v] : f.values)
            values.set (id, v);

        // compensate for KILL so the preset sounds exactly as described at presetKill
        auto base = [&] (const char* id, float fallback) { return values.contains (id) ? (float) values[id] : fallback; };
        values.set (ParamIDs::dirt,    jmax (0.0f, base (ParamIDs::dirt, 0.3f) - presetKill * km.drive));
        values.set (ParamIDs::punch,   jmax (0.0f, base (ParamIDs::punch, 0.3f) - presetKill * km.punch));
        values.set (ParamIDs::sub,     base (ParamIDs::sub, 0.0f) - presetKill * km.subDb);
        values.set (ParamIDs::clipper, jmax (0.0f, base (ParamIDs::clipper, 0.3f) - presetKill * km.clipper));
    }
    else
    {
        const auto json = JSON::parse (info.file);
        if (auto* params = json["params"].getDynamicObject())
            values = params->getProperties();
        // future format versions would be migrated here (json["version"])
    }

    applyValues (values);
    apvts.state.setProperty (presetProperty, info.name, nullptr);
}

void PresetManager::loadNext (int delta)
{
    const auto n = presets.size();
    if (n == 0) return;
    const auto current = getCurrentIndex();
    load (current < 0 ? 0 : ((current + delta) % n + n) % n);
}

void PresetManager::revert()
{
    if (const auto index = getCurrentIndex(); index >= 0)
        load (index);
}

void PresetManager::init()
{
    applyValues ({});
    apvts.state.setProperty (presetProperty, "Init", nullptr);
}

var PresetManager::toJson (const String& name) const
{
    auto* params = new DynamicObject();
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            if (rp->getParameterID() != ParamIDs::bypass && rp->getParameterID() != ParamIDs::phone)
                params->setProperty (rp->getParameterID(), rp->convertFrom0to1 (rp->getValue()));

    auto* root = new DynamicObject();
    root->setProperty ("format", "808k");
    root->setProperty ("version", formatVersion);
    root->setProperty ("name", name);
    root->setProperty ("category", "User");
    root->setProperty ("params", var (params));
    return var (root);
}

bool PresetManager::saveUser (const String& name)
{
    const auto clean = name.trim();
    if (clean.isEmpty()) return false;

    userFolder().createDirectory();
    const auto file = presetFileFor (clean);
    if (! file.replaceWithText (JSON::toString (toJson (clean))))
        return false;

    rescan();
    apvts.state.setProperty (presetProperty, clean, nullptr);
    modified = false;
    return true;
}

String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty (presetProperty, "Init").toString();
}

int PresetManager::getCurrentIndex() const
{
    const auto name = getCurrentName();
    for (int i = 0; i < presets.size(); ++i)
        if (presets.getReference (i).name == name)
            return i;
    return -1;
}

void PresetManager::restoreFromState()
{
    modified = false;
}
