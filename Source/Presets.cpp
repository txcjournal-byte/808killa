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

    using namespace ParamIDs;

    const std::vector<Factory>& factoryPresets()
    {
        static const std::vector<Factory> list = {
            // ---------------- SANCTUS
            { "Holy Water", "SANCTUS", 0, { { clip, 0.25f }, { clipper, 0.15f } } },
            { "Pure Sub", "SANCTUS", 0, { { clip, 0.15f }, { clipper, 0.0f } } },
            { "Angel Dust", "SANCTUS", 0, { { clip, 0.35f }, { clipper, 0.25f } } },
            { "Clean Soul", "SANCTUS", 0, { { clip, 0.3f }, { clipper, 0.2f }, { length, -0.15f } } },
            { "Sacred Boom", "SANCTUS", 0, { { clip, 0.3f }, { clipper, 0.1f }, { length, 0.4f } } },
            { "Confession", "SANCTUS", 0, { { clip, 0.2f }, { clipper, 0.3f }, { length, 0.2f } } },
            { "Halo", "SANCTUS", 0, { { clip, 0.4f }, { clipper, 0.35f }, { width, 0.2f } } },
            { "Amen", "SANCTUS", 0, { { clip, 0.45f }, { clipper, 0.3f }, { punch, 0.4f } } },
            // ---------------- VELVET COFFIN
            { "Velvet Coffin", "VELVET COFFIN", 0, { { clip, 0.35f }, { clipper, 0.0f }, { dirtMode, 0.0f }, { dirt, 0.25f }, { cleanLow, 1.0f } } },
            { "Silk Shroud", "VELVET COFFIN", 0, { { clip, 0.3f }, { clipper, 0.0f }, { dirtMode, 2.0f }, { dirt, 0.2f }, { cleanLow, 1.0f } } },
            { "Soft Burial", "VELVET COFFIN", 0, { { clip, 0.45f }, { clipper, 0.05f }, { dirtMode, 0.0f }, { dirt, 0.3f }, { cleanLow, 1.0f }, { length, 0.2f } } },
            { "Pillow Talk", "VELVET COFFIN", 0, { { clip, 0.3f }, { clipper, 0.0f }, { dirtMode, 0.0f }, { dirt, 0.25f }, { postFilter, 5000.0f }, { cleanLow, 1.0f } } },
            { "Lullaby", "VELVET COFFIN", 0, { { clip, 0.25f }, { clipper, 0.0f }, { dirtMode, 2.0f }, { dirt, 0.2f }, { length, 0.35f }, { cleanLow, 1.0f } } },
            { "Casket Cream", "VELVET COFFIN", 0, { { clip, 0.5f }, { clipper, 0.1f }, { dirtMode, 0.0f }, { dirt, 0.4f }, { cleanLow, 1.0f } } },
            { "Satin Sub", "VELVET COFFIN", 0, { { clip, 0.4f }, { clipper, 0.0f }, { dirtMode, 3.0f }, { dirt, 0.2f }, { cleanLow, 1.0f } } },
            { "Slow Rot", "VELVET COFFIN", 0, { { clip, 0.3f }, { clipper, 0.0f }, { dirtMode, 2.0f }, { dirt, 0.35f }, { length, 0.6f }, { postFilter, 7000.0f }, { cleanLow, 1.0f } } },
            // ---------------- BRICKFACE
            { "Brickface", "BRICKFACE", 0, { { clip, 0.6f }, { clipper, 1.0f } } },
            { "Wall Punch", "BRICKFACE", 0, { { clip, 0.55f }, { clipper, 1.0f }, { punch, 0.5f } } },
            { "Concrete", "BRICKFACE", 0, { { clip, 0.7f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 0.25f }, { cleanLow, 1.0f } } },
            { "Flat Top", "BRICKFACE", 0, { { clip, 0.85f }, { clipper, 1.0f } } },
            { "Cinder Block", "BRICKFACE", 0, { { clip, 0.6f }, { clipper, 0.9f }, { length, -0.3f } } },
            { "Hard Hat", "BRICKFACE", 0, { { clip, 0.65f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 0.3f } } },
            { "Sledge", "BRICKFACE", 0, { { clip, 0.75f }, { clipper, 0.9f }, { dirtMode, 1.0f }, { dirt, 0.4f }, { cleanLow, 1.0f } } },
            { "Square Jaw", "BRICKFACE", 0, { { clip, 1.0f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 0.5f } } },
            // ---------------- JAWBREAKER
            { "Jawbreaker", "JAWBREAKER", 0, { { clip, 0.5f }, { clipper, 0.8f }, { punch, 0.8f }, { punchClick, 0.3f } } },
            { "Uppercut", "JAWBREAKER", 0, { { clip, 0.55f }, { clipper, 0.7f }, { punch, 1.0f }, { length, -0.2f } } },
            { "Knockout", "JAWBREAKER", 0, { { clip, 0.6f }, { clipper, 1.0f }, { punch, 0.8f }, { knock, 5.0f }, { knockTime, 25.0f } } },
            { "Glass Chin", "JAWBREAKER", 0, { { clip, 0.45f }, { clipper, 0.6f }, { punch, 0.6f }, { punchClick, 0.6f } } },
            { "Haymaker", "JAWBREAKER", 0, { { clip, 0.6f }, { clipper, 0.8f }, { punch, 0.9f }, { dirtMode, 3.0f }, { dirt, 0.3f }, { cleanLow, 1.0f } } },
            { "Body Shot", "JAWBREAKER", 0, { { clip, 0.5f }, { clipper, 0.5f }, { punch, 0.7f }, { length, 0.2f } } },
            { "Sucker Punch", "JAWBREAKER", 0, { { clip, 0.7f }, { clipper, 1.0f }, { punch, 1.0f }, { punchClick, 0.5f }, { length, -0.4f } } },
            { "Brass Knuckles", "JAWBREAKER", 0, { { clip, 0.6f }, { clipper, 0.8f }, { punch, 0.7f }, { metal, 0.25f } } },
            // ---------------- +1000 AURA
            { "+1000 Aura", "+1000 AURA", 0, { { clip, 0.8f }, { clipper, 0.6f }, { dirtMode, 0.0f }, { dirt, 0.2f }, { cleanLow, 1.0f } } },
            { "Main Character", "+1000 AURA", 0, { { clip, 0.9f }, { clipper, 0.5f }, { dirtMode, 3.0f }, { dirt, 0.25f }, { cleanLow, 1.0f } } },
            { "Mog", "+1000 AURA", 0, { { clip, 1.0f }, { clipper, 0.8f } } },
            { "Sigma Sub", "+1000 AURA", 0, { { clip, 0.75f }, { clipper, 0.4f }, { length, 0.3f } } },
            { "Aura Farm", "+1000 AURA", 0, { { clip, 0.85f }, { clipper, 0.6f }, { dirtMode, 2.0f }, { dirt, 0.3f }, { cleanLow, 1.0f }, { width, 0.2f } } },
            { "Rizz Limit", "+1000 AURA", 0, { { clip, 1.0f }, { clipper, 0.9f }, { dirtMode, 0.0f }, { dirt, 0.15f } } },
            { "Loud Mode", "+1000 AURA", 0, { { clip, 0.95f }, { clipper, 0.7f }, { buzz, 0.15f } } },
            { "No Cap", "+1000 AURA", 0, { { clip, 0.9f }, { clipper, 1.0f }, { punch, 0.4f } } },
            // ---------------- CASSETTE GHOST
            { "Cassette Ghost", "CASSETTE GHOST", 0, { { clip, 0.35f }, { clipper, 0.2f }, { dirtMode, 2.0f }, { dirt, 0.5f }, { postFilter, 8000.0f } } },
            { "Chewed Tape", "CASSETTE GHOST", 0, { { clip, 0.35f }, { clipper, 0.2f }, { dirtMode, 2.0f }, { dirt, 0.7f }, { crushBits, 14.0f }, { postFilter, 6000.0f } } },
            { "Warm Grave", "CASSETTE GHOST", 0, { { clip, 0.3f }, { clipper, 0.1f }, { dirtMode, 2.0f }, { dirt, 0.45f }, { cleanLow, 1.0f } } },
            { "Rewind", "CASSETTE GHOST", 0, { { clip, 0.3f }, { clipper, 0.2f }, { dirtMode, 2.0f }, { dirt, 0.5f }, { dive, -12.0f }, { diveDelay, 350.0f }, { diveTime, 400.0f } } },
            { "Dusty Deck", "CASSETTE GHOST", 0, { { clip, 0.3f }, { clipper, 0.2f }, { dirtMode, 2.0f }, { dirt, 0.55f }, { filterOn, 1.0f }, { cutoff, 4000.0f } } },
            { "Haunted Hiss", "CASSETTE GHOST", 0, { { clip, 0.4f }, { clipper, 0.3f }, { dirtMode, 2.0f }, { dirt, 0.6f }, { buzz, 0.15f } } },
            { "Ferric", "CASSETTE GHOST", 0, { { clip, 0.45f }, { clipper, 0.3f }, { dirtMode, 2.0f }, { dirt, 0.65f }, { punch, 0.4f } } },
            { "Dead Batteries", "CASSETTE GHOST", 0, { { clip, 0.3f }, { clipper, 0.2f }, { dirtMode, 2.0f }, { dirt, 0.4f }, { dive, -24.0f }, { diveDelay, 0.0f }, { diveTime, 1200.0f } } },
            // ---------------- FURNACE
            { "Furnace", "FURNACE", 0, { { clip, 0.4f }, { clipper, 0.4f }, { dirtMode, 3.0f }, { dirt, 0.5f } } },
            { "Hellfire", "FURNACE", 0, { { clip, 0.5f }, { clipper, 0.5f }, { dirtMode, 3.0f }, { dirt, 0.8f } } },
            { "Glow Tube", "FURNACE", 0, { { clip, 0.35f }, { clipper, 0.3f }, { dirtMode, 3.0f }, { dirt, 0.35f }, { cleanLow, 1.0f } } },
            { "Coal Mine", "FURNACE", 0, { { clip, 0.4f }, { clipper, 0.4f }, { dirtMode, 3.0f }, { dirt, 0.6f }, { postFilter, 6000.0f } } },
            { "Molten", "FURNACE", 0, { { clip, 0.55f }, { clipper, 0.5f }, { dirtMode, 3.0f }, { dirt, 0.7f }, { buzz, 0.2f } } },
            { "Ember", "FURNACE", 0, { { clip, 0.35f }, { clipper, 0.3f }, { dirtMode, 3.0f }, { dirt, 0.3f }, { punch, 0.4f } } },
            { "Boiler Room", "FURNACE", 0, { { clip, 0.45f }, { clipper, 0.4f }, { dirtMode, 3.0f }, { dirt, 0.6f }, { length, 0.3f } } },
            { "Blast Furnace", "FURNACE", 0, { { clip, 0.7f }, { clipper, 0.6f }, { dirtMode, 3.0f }, { dirt, 0.95f } } },
            // ---------------- TOXICUM
            { "Toxicum", "TOXICUM", 0, { { clip, 0.4f }, { clipper, 0.5f }, { dirtMode, 1.0f }, { dirt, 0.5f }, { filterOn, 1.0f }, { cutoff, 2500.0f }, { resonance, 0.6f } } },
            { "Venom", "TOXICUM", 0, { { clip, 0.4f }, { clipper, 0.4f }, { dirtMode, 3.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 1800.0f }, { resonance, 0.7f } } },
            { "Acid Bath", "TOXICUM", 0, { { clip, 0.5f }, { clipper, 0.6f }, { dirtMode, 1.0f }, { dirt, 0.7f }, { filterOn, 1.0f }, { cutoff, 3500.0f }, { resonance, 0.55f }, { metal, 0.2f } } },
            { "Radioactive", "TOXICUM", 0, { { clip, 0.5f }, { clipper, 0.6f }, { dirtMode, 4.0f }, { dirt, 0.4f }, { filterOn, 1.0f }, { cutoff, 3000.0f }, { resonance, 0.6f } } },
            { "Nightshade", "TOXICUM", 0, { { clip, 0.35f }, { clipper, 0.4f }, { dirtMode, 2.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 1500.0f }, { resonance, 0.75f } } },
            { "Poison Ivy", "TOXICUM", 0, { { clip, 0.45f }, { clipper, 0.5f }, { dirtMode, 0.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 2200.0f }, { resonance, 0.55f }, { buzz, 0.2f } } },
            { "Biohazard", "TOXICUM", 0, { { clip, 0.6f }, { clipper, 0.7f }, { dirtMode, 1.0f }, { dirt, 0.8f }, { filterOn, 1.0f }, { cutoff, 4000.0f }, { resonance, 0.65f }, { crushBits, 12.0f } } },
            { "Snake Bite", "TOXICUM", 0, { { clip, 0.45f }, { clipper, 0.5f }, { dirtMode, 3.0f }, { dirt, 0.5f }, { filterOn, 1.0f }, { cutoff, 2800.0f }, { resonance, 0.5f }, { metal, 0.3f } } },
            // ---------------- MOSH PIT
            { "Mosh Pit", "MOSH PIT", 0, { { clip, 0.7f }, { clipper, 0.8f }, { dirtMode, 4.0f }, { dirt, 0.55f }, { buzz, 0.3f }, { cleanLow, 1.0f } } },
            { "Circle Pit", "MOSH PIT", 0, { { clip, 0.6f }, { clipper, 0.7f }, { dirtMode, 4.0f }, { dirt, 0.45f }, { cleanLow, 1.0f } } },
            { "Wall of Death", "MOSH PIT", 0, { { clip, 0.8f }, { clipper, 0.9f }, { dirtMode, 4.0f }, { dirt, 0.7f }, { metal, 0.35f }, { buzz, 0.3f } } },
            { "Stage Dive", "MOSH PIT", 0, { { clip, 0.6f }, { clipper, 0.7f }, { dirtMode, 4.0f }, { dirt, 0.5f }, { dive, -12.0f }, { diveDelay, 220.0f }, { diveTime, 90.0f } } },
            { "Crowd Kill", "MOSH PIT", 0, { { clip, 0.8f }, { clipper, 0.9f }, { dirtMode, 4.0f }, { dirt, 0.8f }, { buzz, 0.4f } } },
            { "Headbanger", "MOSH PIT", 0, { { clip, 0.7f }, { clipper, 0.8f }, { dirtMode, 4.0f }, { dirt, 0.6f }, { metal, 0.4f } } },
            { "Pit Boss", "MOSH PIT", 0, { { clip, 0.65f }, { clipper, 0.7f }, { dirtMode, 4.0f }, { dirt, 0.5f }, { octDown, 0.3f }, { cleanLow, 1.0f } } },
            { "Broken Barrier", "MOSH PIT", 0, { { clip, 0.9f }, { clipper, 1.0f }, { dirtMode, 4.0f }, { dirt, 0.9f }, { metal, 0.3f }, { buzz, 0.3f } } },
            // ---------------- GRAVE DUST
            { "Grave Dust", "GRAVE DUST", 0, { { clip, 0.35f }, { clipper, 0.3f }, { dirtMode, 5.0f }, { dirt, 0.4f }, { crushBits, 10.0f }, { postFilter, 6000.0f } } },
            { "Tombstone", "GRAVE DUST", 0, { { clip, 0.3f }, { clipper, 0.3f }, { dirtMode, 2.0f }, { dirt, 0.5f }, { crushBits, 8.0f }, { filterOn, 1.0f }, { cutoff, 2000.0f } } },
            { "Crypt", "GRAVE DUST", 0, { { clip, 0.4f }, { clipper, 0.4f }, { dirtMode, 5.0f }, { dirt, 0.6f }, { crushBits, 12.0f } } },
            { "Bone Dry", "GRAVE DUST", 0, { { clip, 0.35f }, { clipper, 0.3f }, { dirtMode, 5.0f }, { dirt, 0.3f }, { crushBits, 6.0f }, { dirtMix, 0.5f } } },
            { "Dust Devil", "GRAVE DUST", 0, { { clip, 0.4f }, { clipper, 0.4f }, { dirtMode, 2.0f }, { dirt, 0.5f }, { crushBits, 12.0f }, { postFilter, 5000.0f }, { buzz, 0.2f } } },
            { "Old Bones", "GRAVE DUST", 0, { { clip, 0.3f }, { clipper, 0.3f }, { dirtMode, 0.0f }, { dirt, 0.4f }, { crushBits, 10.0f }, { filterOn, 1.0f }, { cutoff, 1500.0f } } },
            { "Pixel Corpse", "GRAVE DUST", 0, { { clip, 0.5f }, { clipper, 0.5f }, { dirtMode, 5.0f }, { dirt, 0.8f }, { crushBits, 5.0f }, { dirtMix, 0.7f } } },
            { "Mausoleum", "GRAVE DUST", 0, { { clip, 0.35f }, { clipper, 0.3f }, { dirtMode, 5.0f }, { dirt, 0.5f }, { crushBits, 9.0f }, { length, 0.4f } } },
            // ---------------- VOMITORIUM
            { "Vomitorium", "VOMITORIUM", 0, { { clip, 1.0f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 1.0f }, { metal, 0.5f }, { buzz, 0.4f } } },
            { "Vomiter", "VOMITORIUM", 0, { { clip, 1.0f }, { clipper, 1.0f }, { dirtMode, 4.0f }, { dirt, 1.0f }, { buzz, 0.6f } } },
            { "Bonecrusher", "VOMITORIUM", 0, { { clip, 0.9f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 0.9f }, { crushBits, 8.0f }, { metal, 0.3f } } },
            { "Skull Crusher", "VOMITORIUM", 0, { { clip, 1.0f }, { clipper, 0.9f }, { dirtMode, 3.0f }, { dirt, 1.0f }, { metal, 0.7f } } },
            { "Gut Punch", "VOMITORIUM", 0, { { clip, 0.9f }, { clipper, 1.0f }, { dirtMode, 4.0f }, { dirt, 0.8f }, { punch, 1.0f }, { buzz, 0.4f } } },
            { "Meat Grinder", "VOMITORIUM", 0, { { clip, 0.9f }, { clipper, 1.0f }, { dirtMode, 5.0f }, { dirt, 0.9f }, { crushBits, 6.0f }, { buzz, 0.5f } } },
            { "Chainsaw", "VOMITORIUM", 0, { { clip, 1.0f }, { clipper, 1.0f }, { dirtMode, 1.0f }, { dirt, 1.0f }, { buzz, 0.8f } } },
            { "Total Annihilation", "VOMITORIUM", 0, { { clip, 1.0f }, { clipper, 1.0f }, { dirtMode, 4.0f }, { dirt, 1.0f }, { metal, 0.8f }, { buzz, 0.8f }, { crushBits, 10.0f } } },
            // ---------------- BRAINROT
            { "Brainrot", "BRAINROT", 0, { { clip, 0.5f }, { clipper, 0.5f }, { buzz, 0.5f } } },
            { "Phone Speaker", "BRAINROT", 0, { { clip, 0.5f }, { clipper, 0.5f }, { buzz, 0.35f }, { dirtMode, 3.0f }, { dirt, 0.3f }, { cleanLow, 1.0f } } },
            { "Earbud", "BRAINROT", 0, { { clip, 0.45f }, { clipper, 0.4f }, { buzz, 0.3f }, { punch, 0.4f } } },
            { "Scroll Stopper", "BRAINROT", 0, { { clip, 0.6f }, { clipper, 0.7f }, { buzz, 0.6f } } },
            { "Doomscroll", "BRAINROT", 0, { { clip, 0.5f }, { clipper, 0.5f }, { buzz, 0.4f }, { metal, 0.3f } } },
            { "NPC", "BRAINROT", 0, { { clip, 0.4f }, { clipper, 0.4f }, { metal, 0.4f } } },
            { "Ohio", "BRAINROT", 0, { { clip, 0.7f }, { clipper, 0.8f }, { metal, 0.6f }, { buzz, 0.5f } } },
            { "Low Battery", "BRAINROT", 0, { { clip, 0.45f }, { clipper, 0.5f }, { buzz, 0.4f }, { crushBits, 12.0f } } },
        };
        return list;
    }

    File favouritesFile()
    {
        return PresetManager::userFolder().getChildFile ("favourites.txt");
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

const StringArray& PresetManager::categories()
{
    static const StringArray list { "SANCTUS", "VELVET COFFIN", "BRICKFACE", "JAWBREAKER", "+1000 AURA", "CASSETTE GHOST",
                                    "FURNACE", "TOXICUM", "MOSH PIT", "GRAVE DUST", "VOMITORIUM", "BRAINROT" };
    return list;
}

void PresetManager::toggleFavourite (const String& name)
{
    if (favourites.contains (name)) favourites.removeString (name);
    else                            favourites.add (name);

    userFolder().createDirectory();
    favouritesFile().replaceWithText (favourites.joinIntoString ("\n"));
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
        presets.add ({ file.getFileNameWithoutExtension(), "USER", false, file });

    favourites = StringArray::fromLines (favouritesFile().loadFileAsString());
    favourites.removeEmptyStrings();
}

void PresetManager::parameterChanged (const String& id, float)
{
    // may be called from the audio thread (automation): only set a flag, the UI polls it
    if (! loading.load() && id != ParamIDs::bypass && id != ParamIDs::phone)
        modified = true;
}

void PresetManager::applyValues (const NamedValueSet& values)
{
    if (apvts.undoManager != nullptr)
        apvts.undoManager->beginNewTransaction ("Load preset");

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
        values.set (ParamIDs::kill, 0.0f);         // the jaw starts closed: the preset is what the knobs show

        for (auto& [id, v] : f.values)
            values.set (id, v);
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

void PresetManager::toggleAB()
{
    abState[abSlot] = apvts.copyState();
    abSlot = 1 - abSlot;

    if (abState[abSlot].isValid())
    {
        loading = true;
        apvts.replaceState (abState[abSlot].createCopy());
        loading = false;
    }
    else
    {
        abState[abSlot] = apvts.copyState();   // first switch: B starts as a copy of A
    }
}

void PresetManager::copyToOther()
{
    abState[1 - abSlot] = apvts.copyState();
}
