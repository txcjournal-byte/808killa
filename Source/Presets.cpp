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
            // ---------------- SANCTUS
            { "Holy Water", "SANCTUS", 0, { { sub, 3.0f }, { harmonics, 0.2f }, { dirtMode, 0.0f }, { dirt, 0.1f }, { clipper, 0.15f }, { punch, 0.3f } } },
            { "Pure Sub", "SANCTUS", 0, { { dirtOn, 0.0f }, { sub, 5.0f }, { harmonics, 0.1f }, { clipper, 0.1f }, { punch, 0.25f } } },
            { "Angel Dust", "SANCTUS", 0, { { harmonics, 0.45f }, { dirtMode, 0.0f }, { dirt, 0.15f }, { tilt, 1.5f }, { clipper, 0.2f } } },
            { "Clean Soul", "SANCTUS", 0, { { punch, 0.5f }, { punchClick, 0.1f }, { dirtMode, 0.0f }, { dirt, 0.1f }, { clipper, 0.2f }, { sub, 2.0f } } },
            { "Sacred Boom", "SANCTUS", 0, { { length, 0.4f }, { sub, 4.0f }, { dirtMode, 0.0f }, { dirt, 0.1f }, { clipper, 0.2f } } },
            { "Confession", "SANCTUS", 0, { { filterOn, 1.0f }, { cutoff, 6000.0f }, { sub, 3.0f }, { dirtMode, 0.0f }, { dirt, 0.1f }, { clipper, 0.15f } } },
            { "Halo", "SANCTUS", 0, { { harmonics, 0.3f }, { width, 0.3f }, { dirtMode, 0.0f }, { dirt, 0.1f }, { sub, 2.0f }, { clipper, 0.15f } } },
            { "Amen", "SANCTUS", 0, { { knock, 3.0f }, { knockTime, 25.0f }, { punch, 0.45f }, { dirtMode, 0.0f }, { dirt, 0.12f }, { sub, 2.0f }, { clipper, 0.2f } } },
            // ---------------- VELVET COFFIN
            { "Velvet Coffin", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.35f }, { clipper, 0.35f }, { sub, 2.0f } } },
            { "Silk Shroud", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.25f }, { clipper, 0.3f }, { tilt, -1.0f }, { sub, 3.0f } } },
            { "Soft Burial", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.4f }, { clipper, 0.4f }, { length, 0.2f } } },
            { "Pillow Talk", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.3f }, { filterOn, 1.0f }, { cutoff, 5000.0f }, { clipper, 0.3f }, { sub, 2.0f } } },
            { "Lullaby", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.2f }, { sub, 4.0f }, { clipper, 0.25f }, { length, 0.3f } } },
            { "Casket Cream", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.45f }, { dirtMix, 0.8f }, { clipper, 0.45f } } },
            { "Satin Sub", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.3f }, { harmonics, 0.25f }, { sub, 3.0f }, { clipper, 0.35f } } },
            { "Slow Rot", "VELVET COFFIN", 7, { { dirtMode, 0.0f }, { dirt, 0.35f }, { length, 0.5f }, { clipper, 0.3f }, { tilt, -2.0f } } },
            // ---------------- BRICKFACE
            { "Brickface", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.6f }, { clipper, 0.8f }, { sub, 1.0f } } },
            { "Wall Punch", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.5f }, { punch, 0.7f }, { clipper, 0.85f } } },
            { "Concrete", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.7f }, { harmonics, 0.3f }, { clipper, 0.9f } } },
            { "Flat Top", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.8f }, { clipper, 1.0f } } },
            { "Cinder Block", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.55f }, { length, -0.2f }, { clipper, 0.8f }, { punchClick, 0.2f } } },
            { "Hard Hat", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.65f }, { knock, 3.0f }, { knockTime, 20.0f }, { clipper, 0.8f } } },
            { "Sledge", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.75f }, { sub, 2.0f }, { clipper, 0.85f } } },
            { "Square Jaw", "BRICKFACE", 3, { { dirtMode, 1.0f }, { dirt, 0.9f }, { dirtMix, 0.7f }, { clipper, 0.9f } } },
            // ---------------- JAWBREAKER
            { "Jawbreaker", "JAWBREAKER", 4, { { punch, 0.9f }, { punchClick, 0.4f }, { knock, 5.0f }, { knockTime, 30.0f }, { dirtMode, 0.0f }, { dirt, 0.3f }, { clipper, 0.5f } } },
            { "Uppercut", "JAWBREAKER", 4, { { punch, 1.0f }, { punchClick, 0.3f }, { knock, 7.0f }, { knockTime, 30.0f }, { dirtMode, 0.0f }, { dirt, 0.25f }, { clipper, 0.5f } } },
            { "Knockout", "JAWBREAKER", 4, { { punch, 0.85f }, { knock, 12.0f }, { knockTime, 20.0f }, { dirtMode, 1.0f }, { dirt, 0.3f }, { clipper, 0.6f } } },
            { "Glass Chin", "JAWBREAKER", 4, { { punch, 0.7f }, { punchClick, 0.7f }, { dirtMode, 0.0f }, { dirt, 0.2f }, { clipper, 0.45f } } },
            { "Haymaker", "JAWBREAKER", 4, { { punch, 0.9f }, { length, -0.2f }, { dirtMode, 3.0f }, { dirt, 0.4f }, { clipper, 0.55f } } },
            { "Body Shot", "JAWBREAKER", 4, { { punch, 0.8f }, { sub, 3.0f }, { dirtMode, 0.0f }, { dirt, 0.25f }, { clipper, 0.45f } } },
            { "Sucker Punch", "JAWBREAKER", 4, { { punch, 1.0f }, { punchClick, 0.5f }, { knock, 4.0f }, { knockTime, 15.0f }, { dirtMode, 1.0f }, { dirt, 0.35f }, { clipper, 0.6f } } },
            { "Brass Knuckles", "JAWBREAKER", 4, { { punch, 0.75f }, { harmonics, 0.4f }, { dirtMode, 2.0f }, { dirt, 0.45f }, { clipper, 0.55f } } },
            // ---------------- +1000 AURA
            { "+1000 Aura", "+1000 AURA", 9, { { dirtMode, 0.0f }, { dirt, 0.5f }, { harmonics, 0.4f }, { clipper, 0.9f }, { sub, 2.0f } } },
            { "Main Character", "+1000 AURA", 9, { { dirtMode, 3.0f }, { dirt, 0.45f }, { harmonics, 0.5f }, { clipper, 0.85f }, { punch, 0.5f } } },
            { "Mog", "+1000 AURA", 9, { { dirtMode, 1.0f }, { dirt, 0.6f }, { clipper, 0.95f } } },
            { "Sigma Sub", "+1000 AURA", 9, { { dirtMode, 0.0f }, { dirt, 0.3f }, { sub, 5.0f }, { clipper, 0.8f } } },
            { "Aura Farm", "+1000 AURA", 9, { { dirtMode, 2.0f }, { dirt, 0.5f }, { harmonics, 0.35f }, { clipper, 0.85f }, { width, 0.2f } } },
            { "Rizz Limit", "+1000 AURA", 9, { { dirtMode, 0.0f }, { dirt, 0.55f }, { clipper, 1.0f }, { harmonics, 0.3f } } },
            { "Loud Mode", "+1000 AURA", 9, { { dirtMode, 3.0f }, { dirt, 0.65f }, { harmonics, 0.45f }, { clipper, 1.0f } } },
            { "No Cap", "+1000 AURA", 9, { { dirtMode, 1.0f }, { dirt, 0.5f }, { punch, 0.6f }, { clipper, 0.9f } } },
            // ---------------- CASSETTE GHOST
            { "Cassette Ghost", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.5f }, { postFilter, 9000.0f }, { clipper, 0.3f } } },
            { "Chewed Tape", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.7f }, { crushBits, 14.0f }, { postFilter, 7000.0f }, { clipper, 0.35f } } },
            { "Warm Grave", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.45f }, { sub, 3.0f }, { tilt, -1.5f }, { clipper, 0.3f } } },
            { "Rewind", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.5f }, { dive, -12.0f }, { diveDelay, 350.0f }, { diveTime, 400.0f }, { clipper, 0.3f } } },
            { "Dusty Deck", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.55f }, { filterOn, 1.0f }, { cutoff, 4500.0f }, { clipper, 0.3f } } },
            { "Haunted Hiss", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.6f }, { harmonics, 0.3f }, { clipper, 0.35f } } },
            { "Ferric", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.65f }, { punch, 0.5f }, { clipper, 0.4f } } },
            { "Dead Batteries", "CASSETTE GHOST", 6, { { dirtMode, 2.0f }, { dirt, 0.4f }, { dive, -24.0f }, { diveDelay, 0.0f }, { diveTime, 1200.0f }, { clipper, 0.3f } } },
            // ---------------- FURNACE
            { "Furnace", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.5f }, { harmonics, 0.3f }, { clipper, 0.4f } } },
            { "Hellfire", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.75f }, { dirtMix, 0.9f }, { clipper, 0.5f } } },
            { "Glow Tube", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.35f }, { sub, 3.0f }, { clipper, 0.3f } } },
            { "Coal Mine", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.6f }, { tilt, -2.0f }, { clipper, 0.4f } } },
            { "Molten", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.7f }, { harmonics, 0.45f }, { clipper, 0.5f } } },
            { "Ember", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.3f }, { punch, 0.5f }, { clipper, 0.35f } } },
            { "Boiler Room", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.6f }, { length, 0.3f }, { clipper, 0.45f } } },
            { "Blast Furnace", "FURNACE", 6, { { dirtMode, 3.0f }, { dirt, 0.85f }, { clipper, 0.6f }, { sub, 2.0f } } },
            // ---------------- TOXICUM
            { "Toxicum", "TOXICUM", 1, { { dirtMode, 1.0f }, { dirt, 0.5f }, { filterOn, 1.0f }, { cutoff, 2500.0f }, { resonance, 0.6f }, { clipper, 0.4f } } },
            { "Venom", "TOXICUM", 1, { { dirtMode, 3.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 1800.0f }, { resonance, 0.7f }, { clipper, 0.4f } } },
            { "Acid Bath", "TOXICUM", 1, { { dirtMode, 1.0f }, { dirt, 0.7f }, { filterOn, 1.0f }, { cutoff, 3500.0f }, { resonance, 0.5f }, { harmonics, 0.3f }, { clipper, 0.5f } } },
            { "Radioactive", "TOXICUM", 1, { { dirtMode, 4.0f }, { dirt, 0.3f }, { filterOn, 1.0f }, { cutoff, 3000.0f }, { resonance, 0.6f }, { clipper, 0.5f } } },
            { "Nightshade", "TOXICUM", 1, { { dirtMode, 2.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 1500.0f }, { resonance, 0.75f }, { sub, 3.0f }, { clipper, 0.35f } } },
            { "Poison Ivy", "TOXICUM", 1, { { dirtMode, 0.0f }, { dirt, 0.6f }, { filterOn, 1.0f }, { cutoff, 2200.0f }, { resonance, 0.55f }, { punch, 0.5f }, { clipper, 0.4f } } },
            { "Biohazard", "TOXICUM", 1, { { dirtMode, 1.0f }, { dirt, 0.8f }, { filterOn, 1.0f }, { cutoff, 4000.0f }, { resonance, 0.65f }, { crushBits, 12.0f }, { clipper, 0.5f } } },
            { "Snake Bite", "TOXICUM", 1, { { dirtMode, 3.0f }, { dirt, 0.5f }, { filterOn, 1.0f }, { cutoff, 2800.0f }, { resonance, 0.5f }, { knock, 5.0f }, { knockTime, 25.0f }, { clipper, 0.4f } } },
            // ---------------- MOSH PIT
            { "Mosh Pit", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.55f }, { dirtMix, 0.8f }, { octUp, 0.3f }, { harmonics, 0.3f }, { clipper, 0.8f }, { sub, 3.0f } } },
            { "Circle Pit", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.45f }, { dirtMix, 0.7f }, { clipper, 0.7f } } },
            { "Wall of Death", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.7f }, { clipper, 0.9f }, { octUp, 0.4f } } },
            { "Stage Dive", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.5f }, { dive, -12.0f }, { diveDelay, 220.0f }, { diveTime, 90.0f }, { clipper, 0.7f } } },
            { "Crowd Kill", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.8f }, { dirtMix, 0.9f }, { clipper, 0.85f } } },
            { "Headbanger", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.6f }, { punch, 0.7f }, { clipper, 0.8f } } },
            { "Pit Boss", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.5f }, { octDown, 0.3f }, { clipper, 0.75f } } },
            { "Broken Barrier", "MOSH PIT", 2, { { dirtMode, 4.0f }, { dirt, 0.9f }, { harmonics, 0.4f }, { clipper, 0.9f } } },
            // ---------------- GRAVE DUST
            { "Grave Dust", "GRAVE DUST", 1, { { dirtMode, 5.0f }, { dirt, 0.4f }, { crushBits, 10.0f }, { postFilter, 6000.0f }, { clipper, 0.3f } } },
            { "Tombstone", "GRAVE DUST", 1, { { dirtMode, 2.0f }, { dirt, 0.5f }, { crushBits, 8.0f }, { filterOn, 1.0f }, { cutoff, 2000.0f }, { clipper, 0.3f } } },
            { "Crypt", "GRAVE DUST", 1, { { dirtMode, 5.0f }, { dirt, 0.6f }, { crushBits, 12.0f }, { clipper, 0.4f } } },
            { "Bone Dry", "GRAVE DUST", 1, { { dirtMode, 5.0f }, { dirt, 0.3f }, { crushBits, 6.0f }, { dirtMix, 0.5f }, { clipper, 0.3f } } },
            { "Dust Devil", "GRAVE DUST", 1, { { dirtMode, 2.0f }, { dirt, 0.5f }, { crushBits, 12.0f }, { postFilter, 5000.0f }, { clipper, 0.35f } } },
            { "Old Bones", "GRAVE DUST", 1, { { dirtMode, 0.0f }, { dirt, 0.4f }, { crushBits, 10.0f }, { filterOn, 1.0f }, { cutoff, 1500.0f }, { clipper, 0.3f } } },
            { "Pixel Corpse", "GRAVE DUST", 1, { { dirtMode, 5.0f }, { dirt, 0.8f }, { crushBits, 5.0f }, { dirtMix, 0.7f }, { clipper, 0.4f } } },
            { "Mausoleum", "GRAVE DUST", 1, { { dirtMode, 5.0f }, { dirt, 0.5f }, { crushBits, 9.0f }, { length, 0.4f }, { clipper, 0.3f } } },
            // ---------------- VOMITORIUM
            { "Vomitorium", "VOMITORIUM", 5, { { dirtMode, 1.0f }, { dirt, 1.0f }, { harmonics, 0.6f }, { clipper, 1.0f } } },
            { "Vomiter", "VOMITORIUM", 5, { { dirtMode, 4.0f }, { dirt, 1.0f }, { clipper, 1.0f } } },
            { "Bonecrusher", "VOMITORIUM", 5, { { dirtMode, 1.0f }, { dirt, 0.9f }, { crushBits, 8.0f }, { clipper, 0.9f } } },
            { "Skull Crusher", "VOMITORIUM", 5, { { dirtMode, 3.0f }, { dirt, 1.0f }, { octUp, 0.5f }, { clipper, 1.0f } } },
            { "Gut Punch", "VOMITORIUM", 5, { { dirtMode, 4.0f }, { dirt, 0.8f }, { punch, 1.0f }, { punchClick, 0.5f }, { clipper, 0.9f } } },
            { "Meat Grinder", "VOMITORIUM", 5, { { dirtMode, 5.0f }, { dirt, 0.9f }, { crushBits, 6.0f }, { clipper, 0.9f } } },
            { "Chainsaw", "VOMITORIUM", 5, { { dirtMode, 1.0f }, { dirt, 1.0f }, { octUp, 0.6f }, { harmonics, 0.5f }, { clipper, 0.9f } } },
            { "Total Annihilation", "VOMITORIUM", 5, { { dirtMode, 4.0f }, { dirt, 1.0f }, { crushBits, 10.0f }, { harmonics, 0.6f }, { clipper, 1.0f } } },
            // ---------------- BRAINROT
            { "Brainrot", "BRAINROT", 9, { { harmonics, 0.85f }, { dirtMode, 0.0f }, { dirt, 0.3f }, { sub, 1.0f }, { clipper, 0.3f } } },
            { "Phone Speaker", "BRAINROT", 9, { { harmonics, 0.7f }, { dirtMode, 3.0f }, { dirt, 0.4f }, { clipper, 0.4f } } },
            { "Earbud", "BRAINROT", 9, { { harmonics, 0.6f }, { punch, 0.5f }, { dirtMode, 0.0f }, { dirt, 0.3f }, { clipper, 0.35f } } },
            { "Scroll Stopper", "BRAINROT", 9, { { harmonics, 0.8f }, { dirtMode, 1.0f }, { dirt, 0.4f }, { clipper, 0.5f } } },
            { "Doomscroll", "BRAINROT", 9, { { harmonics, 0.75f }, { dirtMode, 2.0f }, { dirt, 0.45f }, { clipper, 0.4f } } },
            { "NPC", "BRAINROT", 9, { { harmonics, 0.5f }, { dirtMode, 0.0f }, { dirt, 0.3f }, { sub, 2.0f }, { clipper, 0.3f } } },
            { "Ohio", "BRAINROT", 9, { { harmonics, 0.8f }, { dirtMode, 4.0f }, { dirt, 0.3f }, { clipper, 0.5f } } },
            { "Low Battery", "BRAINROT", 9, { { harmonics, 0.7f }, { dirtMode, 0.0f }, { dirt, 0.35f }, { crushBits, 12.0f }, { clipper, 0.4f } } },
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
        const auto km = killMappingFor (f.style);
        values.set (ParamIDs::style, f.style);
        values.set (ParamIDs::kill, presetKill);
        values.set (ParamIDs::cleanLow, 1);        // the sub always stays clean under the dirt

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
