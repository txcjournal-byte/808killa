#include "Presets.h"
#include "Parameters.h"

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
            { "Holy Water", "SANCTUS", 0, { { satMode, 1.0f }, { driveAmount, 8.0f }, { midFocus, 2.0f }, { clipDrive, 3.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Pure Sub", "SANCTUS", 0, { { satMode, 0.0f }, { driveAmount, 0.0f }, { midFocus, 0.0f }, { clipDrive, 2.0f }, { clipKnee, 0.95f }, { crossoverFreq, 100.0f }, { duckRelease, 45.0f } } },
            { "Angel Dust", "SANCTUS", 0, { { satMode, 0.0f }, { driveAmount, 12.0f }, { midFocus, 4.0f }, { clipDrive, 4.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Clean Soul", "SANCTUS", 0, { { satMode, 1.0f }, { driveAmount, 10.0f }, { midFocus, 3.0f }, { clipDrive, 3.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Sacred Boom", "SANCTUS", 0, { { satMode, 1.0f }, { driveAmount, 6.0f }, { midFocus, 1.0f }, { clipDrive, 5.0f }, { clipKnee, 0.9f }, { crossoverFreq, 140.0f }, { duckRelease, 45.0f } } },
            { "Confession", "SANCTUS", 0, { { satMode, 0.0f }, { driveAmount, 8.0f }, { midFocus, 2.0f }, { clipDrive, 2.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Halo", "SANCTUS", 0, { { satMode, 1.0f }, { driveAmount, 14.0f }, { midFocus, 5.0f }, { clipDrive, 4.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Amen", "SANCTUS", 0, { { satMode, 1.0f }, { driveAmount, 12.0f }, { midFocus, 3.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- VELVET COFFIN
            { "Velvet Coffin", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 30.0f }, { midFocus, 2.0f }, { clipDrive, 4.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Silk Shroud", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 22.0f }, { midFocus, 1.0f }, { clipDrive, 3.0f }, { clipKnee, 0.55f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Soft Burial", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 35.0f }, { midFocus, 3.0f }, { clipDrive, 6.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Pillow Talk", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 25.0f }, { midFocus, 0.0f }, { clipDrive, 4.0f }, { clipKnee, 0.5f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Lullaby", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 18.0f }, { midFocus, 2.0f }, { clipDrive, 3.0f }, { clipKnee, 0.55f }, { crossoverFreq, 100.0f }, { duckRelease, 45.0f } } },
            { "Casket Cream", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 40.0f }, { midFocus, 3.0f }, { clipDrive, 7.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Satin Sub", "VELVET COFFIN", 0, { { satMode, 1.0f }, { driveAmount, 20.0f }, { midFocus, 2.0f }, { clipDrive, 4.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Slow Rot", "VELVET COFFIN", 0, { { satMode, 0.0f }, { driveAmount, 45.0f }, { midFocus, 1.0f }, { clipDrive, 5.0f }, { clipKnee, 0.5f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            // ---------------- BRICKFACE
            { "Brickface", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 20.0f }, { midFocus, 3.0f }, { clipDrive, 10.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Wall Punch", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 25.0f }, { midFocus, 5.0f }, { clipDrive, 9.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Concrete", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 35.0f }, { midFocus, 3.0f }, { clipDrive, 12.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Flat Top", "BRICKFACE", 0, { { satMode, 0.0f }, { driveAmount, 15.0f }, { midFocus, 2.0f }, { clipDrive, 15.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Cinder Block", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 30.0f }, { midFocus, 4.0f }, { clipDrive, 11.0f }, { clipKnee, 0.93f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Hard Hat", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 28.0f }, { midFocus, 6.0f }, { clipDrive, 10.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Sledge", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 45.0f }, { midFocus, 3.0f }, { clipDrive, 13.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Square Jaw", "BRICKFACE", 0, { { satMode, 1.0f }, { driveAmount, 55.0f }, { midFocus, 4.0f }, { clipDrive, 16.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- JAWBREAKER
            { "Jawbreaker", "JAWBREAKER", 0, { { satMode, 1.0f }, { driveAmount, 35.0f }, { midFocus, 8.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Uppercut", "JAWBREAKER", 0, { { satMode, 1.0f }, { driveAmount, 40.0f }, { midFocus, 9.0f }, { clipDrive, 7.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Knockout", "JAWBREAKER", 0, { { satMode, 1.0f }, { driveAmount, 45.0f }, { midFocus, 7.0f }, { clipDrive, 8.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Glass Chin", "JAWBREAKER", 0, { { satMode, 0.0f }, { driveAmount, 30.0f }, { midFocus, 10.0f }, { clipDrive, 5.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Haymaker", "JAWBREAKER", 0, { { satMode, 1.0f }, { driveAmount, 50.0f }, { midFocus, 8.0f }, { clipDrive, 8.0f }, { clipKnee, 0.85f }, { crossoverFreq, 140.0f }, { duckRelease, 45.0f } } },
            { "Body Shot", "JAWBREAKER", 0, { { satMode, 0.0f }, { driveAmount, 35.0f }, { midFocus, 6.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Sucker Punch", "JAWBREAKER", 0, { { satMode, 1.0f }, { driveAmount, 55.0f }, { midFocus, 10.0f }, { clipDrive, 9.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Brass Knuckles", "JAWBREAKER", 0, { { satMode, 2.0f }, { driveAmount, 20.0f }, { midFocus, 8.0f }, { clipDrive, 7.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- +1000 AURA
            { "+1000 Aura", "+1000 AURA", 0, { { satMode, 0.0f }, { driveAmount, 30.0f }, { midFocus, 4.0f }, { clipDrive, 14.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Main Character", "+1000 AURA", 0, { { satMode, 1.0f }, { driveAmount, 35.0f }, { midFocus, 5.0f }, { clipDrive, 15.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Mog", "+1000 AURA", 0, { { satMode, 1.0f }, { driveAmount, 30.0f }, { midFocus, 3.0f }, { clipDrive, 18.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Sigma Sub", "+1000 AURA", 0, { { satMode, 0.0f }, { driveAmount, 20.0f }, { midFocus, 2.0f }, { clipDrive, 13.0f }, { clipKnee, 0.7f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Aura Farm", "+1000 AURA", 0, { { satMode, 0.0f }, { driveAmount, 35.0f }, { midFocus, 4.0f }, { clipDrive, 15.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Rizz Limit", "+1000 AURA", 0, { { satMode, 1.0f }, { driveAmount, 25.0f }, { midFocus, 3.0f }, { clipDrive, 17.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Loud Mode", "+1000 AURA", 0, { { satMode, 1.0f }, { driveAmount, 40.0f }, { midFocus, 5.0f }, { clipDrive, 18.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "No Cap", "+1000 AURA", 0, { { satMode, 1.0f }, { driveAmount, 35.0f }, { midFocus, 6.0f }, { clipDrive, 16.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- CASSETTE GHOST
            { "Cassette Ghost", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 55.0f }, { midFocus, 2.0f }, { clipDrive, 4.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Chewed Tape", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 70.0f }, { midFocus, 1.0f }, { clipDrive, 5.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Warm Grave", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 45.0f }, { midFocus, 1.0f }, { clipDrive, 3.0f }, { clipKnee, 0.55f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Rewind", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 60.0f }, { midFocus, 3.0f }, { clipDrive, 4.0f }, { clipKnee, 0.6f }, { crossoverFreq, 100.0f }, { duckRelease, 45.0f } } },
            { "Dusty Deck", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 50.0f }, { midFocus, 0.0f }, { clipDrive, 4.0f }, { clipKnee, 0.55f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Haunted Hiss", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 65.0f }, { midFocus, 4.0f }, { clipDrive, 5.0f }, { clipKnee, 0.6f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Ferric", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 60.0f }, { midFocus, 5.0f }, { clipDrive, 6.0f }, { clipKnee, 0.65f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Dead Batteries", "CASSETTE GHOST", 0, { { satMode, 0.0f }, { driveAmount, 75.0f }, { midFocus, 2.0f }, { clipDrive, 4.0f }, { clipKnee, 0.55f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            // ---------------- FURNACE
            { "Furnace", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 55.0f }, { midFocus, 4.0f }, { clipDrive, 5.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Hellfire", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 80.0f }, { midFocus, 5.0f }, { clipDrive, 7.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Glow Tube", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 40.0f }, { midFocus, 3.0f }, { clipDrive, 4.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Coal Mine", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 60.0f }, { midFocus, 2.0f }, { clipDrive, 5.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Molten", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 70.0f }, { midFocus, 6.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Ember", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 45.0f }, { midFocus, 4.0f }, { clipDrive, 4.0f }, { clipKnee, 0.75f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Boiler Room", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 65.0f }, { midFocus, 4.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 140.0f }, { duckRelease, 45.0f } } },
            { "Blast Furnace", "FURNACE", 0, { { satMode, 1.0f }, { driveAmount, 90.0f }, { midFocus, 5.0f }, { clipDrive, 8.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- TOXICUM
            { "Toxicum", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 25.0f }, { midFocus, 10.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Venom", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 30.0f }, { midFocus, 11.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Acid Bath", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 35.0f }, { midFocus, 12.0f }, { clipDrive, 7.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Radioactive", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 40.0f }, { midFocus, 9.0f }, { clipDrive, 7.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Nightshade", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 20.0f }, { midFocus, 8.0f }, { clipDrive, 5.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Poison Ivy", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 30.0f }, { midFocus, 12.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Biohazard", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 45.0f }, { midFocus, 12.0f }, { clipDrive, 8.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Snake Bite", "TOXICUM", 0, { { satMode, 2.0f }, { driveAmount, 28.0f }, { midFocus, 10.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- MOSH PIT
            { "Mosh Pit", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 60.0f }, { midFocus, 6.0f }, { clipDrive, 8.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Circle Pit", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 50.0f }, { midFocus, 5.0f }, { clipDrive, 7.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Wall of Death", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 75.0f }, { midFocus, 7.0f }, { clipDrive, 10.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Stage Dive", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 55.0f }, { midFocus, 6.0f }, { clipDrive, 8.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Crowd Kill", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 80.0f }, { midFocus, 8.0f }, { clipDrive, 10.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Headbanger", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 65.0f }, { midFocus, 7.0f }, { clipDrive, 9.0f }, { clipKnee, 0.9f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Pit Boss", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 55.0f }, { midFocus, 4.0f }, { clipDrive, 8.0f }, { clipKnee, 0.9f }, { crossoverFreq, 150.0f }, { duckRelease, 45.0f } } },
            { "Broken Barrier", "MOSH PIT", 0, { { satMode, 2.0f }, { driveAmount, 90.0f }, { midFocus, 8.0f }, { clipDrive, 12.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            // ---------------- GRAVE DUST
            { "Grave Dust", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 80.0f }, { midFocus, 0.0f }, { clipDrive, 5.0f }, { clipKnee, 0.6f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            { "Tombstone", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 85.0f }, { midFocus, 0.0f }, { clipDrive, 4.0f }, { clipKnee, 0.55f }, { crossoverFreq, 80.0f }, { duckRelease, 45.0f } } },
            { "Crypt", "GRAVE DUST", 0, { { satMode, 1.0f }, { driveAmount, 75.0f }, { midFocus, 1.0f }, { clipDrive, 6.0f }, { clipKnee, 0.65f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            { "Bone Dry", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 70.0f }, { midFocus, 0.0f }, { clipDrive, 4.0f }, { clipKnee, 0.6f }, { crossoverFreq, 80.0f }, { duckRelease, 45.0f } } },
            { "Dust Devil", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 85.0f }, { midFocus, 2.0f }, { clipDrive, 5.0f }, { clipKnee, 0.6f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            { "Old Bones", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 75.0f }, { midFocus, 0.0f }, { clipDrive, 3.0f }, { clipKnee, 0.55f }, { crossoverFreq, 80.0f }, { duckRelease, 45.0f } } },
            { "Pixel Corpse", "GRAVE DUST", 0, { { satMode, 2.0f }, { driveAmount, 70.0f }, { midFocus, 1.0f }, { clipDrive, 6.0f }, { clipKnee, 0.7f }, { crossoverFreq, 90.0f }, { duckRelease, 45.0f } } },
            { "Mausoleum", "GRAVE DUST", 0, { { satMode, 0.0f }, { driveAmount, 80.0f }, { midFocus, 1.0f }, { clipDrive, 5.0f }, { clipKnee, 0.6f }, { crossoverFreq, 100.0f }, { duckRelease, 45.0f } } },
            // ---------------- VOMITORIUM
            { "Vomitorium", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 100.0f }, { midFocus, 10.0f }, { clipDrive, 18.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Vomiter", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 95.0f }, { midFocus, 12.0f }, { clipDrive, 16.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Bonecrusher", "VOMITORIUM", 0, { { satMode, 1.0f }, { driveAmount, 100.0f }, { midFocus, 8.0f }, { clipDrive, 18.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Skull Crusher", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 100.0f }, { midFocus, 6.0f }, { clipDrive, 17.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Gut Punch", "VOMITORIUM", 0, { { satMode, 1.0f }, { driveAmount, 95.0f }, { midFocus, 10.0f }, { clipDrive, 15.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Meat Grinder", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 90.0f }, { midFocus, 12.0f }, { clipDrive, 18.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Chainsaw", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 100.0f }, { midFocus, 12.0f }, { clipDrive, 16.0f }, { clipKnee, 0.95f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Total Annihilation", "VOMITORIUM", 0, { { satMode, 2.0f }, { driveAmount, 100.0f }, { midFocus, 12.0f }, { clipDrive, 18.0f }, { clipKnee, 0.95f }, { crossoverFreq, 160.0f }, { duckRelease, 45.0f } } },
            // ---------------- BRAINROT
            { "Brainrot", "BRAINROT", 0, { { satMode, 1.0f }, { driveAmount, 45.0f }, { midFocus, 12.0f }, { clipDrive, 7.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Phone Speaker", "BRAINROT", 0, { { satMode, 1.0f }, { driveAmount, 40.0f }, { midFocus, 11.0f }, { clipDrive, 6.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Earbud", "BRAINROT", 0, { { satMode, 0.0f }, { driveAmount, 35.0f }, { midFocus, 10.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Scroll Stopper", "BRAINROT", 0, { { satMode, 1.0f }, { driveAmount, 55.0f }, { midFocus, 12.0f }, { clipDrive, 8.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Doomscroll", "BRAINROT", 0, { { satMode, 0.0f }, { driveAmount, 45.0f }, { midFocus, 11.0f }, { clipDrive, 7.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "NPC", "BRAINROT", 0, { { satMode, 1.0f }, { driveAmount, 30.0f }, { midFocus, 9.0f }, { clipDrive, 5.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Ohio", "BRAINROT", 0, { { satMode, 2.0f }, { driveAmount, 40.0f }, { midFocus, 12.0f }, { clipDrive, 8.0f }, { clipKnee, 0.85f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
            { "Low Battery", "BRAINROT", 0, { { satMode, 0.0f }, { driveAmount, 50.0f }, { midFocus, 10.0f }, { clipDrive, 6.0f }, { clipKnee, 0.8f }, { crossoverFreq, 120.0f }, { duckRelease, 45.0f } } },
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
    if (! loading.load() && id != ParamIDs::bypass && id != ParamIDs::phonePreview)
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
        if (rp == nullptr || rp->getParameterID() == ParamIDs::bypass || rp->getParameterID() == ParamIDs::phonePreview)
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
            if (rp->getParameterID() != ParamIDs::bypass && rp->getParameterID() != ParamIDs::phonePreview)
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
