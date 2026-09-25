#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs are part of saved projects and presets:
// once released they must never be renamed or removed, only added.
namespace ParamIDs
{
    // global / macros
    inline constexpr auto style      = "style";
    inline constexpr auto kill       = "kill";
    inline constexpr auto inGain     = "in_gain";
    inline constexpr auto bypass     = "bypass";

    // shape
    inline constexpr auto shapeOn    = "shape_on";
    inline constexpr auto punch      = "punch";
    inline constexpr auto punchClick = "punch_click";
    inline constexpr auto length     = "length";

    // tone
    inline constexpr auto toneOn     = "tone_on";
    inline constexpr auto sub        = "sub";
    inline constexpr auto harmonics  = "harmonics";
    inline constexpr auto filterOn   = "filter_on";
    inline constexpr auto cutoff     = "filter_cutoff";
    inline constexpr auto resonance  = "filter_res";
    inline constexpr auto slope      = "filter_slope";
    inline constexpr auto tilt       = "tilt";

    // dirt
    inline constexpr auto dirtOn     = "dirt_on";
    inline constexpr auto dirtMode   = "dirt_mode";
    inline constexpr auto dirt       = "dirt";
    inline constexpr auto dirtMix    = "dirt_mix";
    inline constexpr auto autoGain   = "dirt_autogain";
    inline constexpr auto oversample = "dirt_os";
    inline constexpr auto cleanLow   = "clean_low";
    inline constexpr auto cleanFreq  = "clean_low_freq";
    inline constexpr auto crushBits  = "crush_bits";
    inline constexpr auto postFilter = "dirt_post";

    // duck
    inline constexpr auto duckOn     = "duck_on";
    inline constexpr auto duck       = "duck";
    inline constexpr auto duckRel    = "duck_release";
    inline constexpr auto duckShape  = "duck_shape";

    // output
    inline constexpr auto clipper    = "clipper";
    inline constexpr auto ceiling    = "ceiling";
    inline constexpr auto monoBelow  = "mono_below";
    inline constexpr auto outGain    = "out_gain";
    inline constexpr auto mix        = "mix";
    inline constexpr auto phone      = "phone_check";
}

namespace Choices
{
    const juce::StringArray styles { "Atlanta Clean", "Memphis Phonk", "Rage Underground", "Detroit Clip",
                                     "Drill Chicago", "Drill NY", "Drill UK", "Plugg Soft",
                                     "Chicago Boom", "Classic Trap Boom" };
    const juce::StringArray dirtModes { "Soft", "Hard Clip", "Tape", "Tube", "Foldback", "Bitcrush" };
    const juce::StringArray oversampling { "2x", "4x", "8x" };
    const juce::StringArray slopes { "12 dB", "24 dB" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Which parameters belong to which ADVANCED section (for section reset)
juce::StringArray sectionParameters (const juce::String& section);
