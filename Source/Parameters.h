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

    // pitch
    inline constexpr auto pitchOn    = "pitch_on";
    inline constexpr auto knock      = "knock";
    inline constexpr auto knockTime  = "knock_time";
    inline constexpr auto dive       = "dive";
    inline constexpr auto diveTime   = "dive_time";
    inline constexpr auto diveDelay  = "dive_delay";
    inline constexpr auto octDown    = "oct_down";
    inline constexpr auto octUp      = "oct_up";

    // wobble
    inline constexpr auto wobbleOn     = "wobble_on";
    inline constexpr auto wobble       = "wobble";
    inline constexpr auto wobbleTarget = "wobble_target";
    inline constexpr auto wobbleRate   = "wobble_rate";
    inline constexpr auto wobbleShape  = "wobble_shape";
    inline constexpr auto wobbleFade   = "wobble_fade";
    inline constexpr auto wobbleRetrig = "wobble_retrig";

    // chop (tempo-synced gate)
    inline constexpr auto chopOn      = "chop_on";
    inline constexpr auto chop        = "chop";
    inline constexpr auto chopPattern = "chop_pattern";
    inline constexpr auto chopGate    = "chop_gate";
    inline constexpr auto chopSmooth  = "chop_smooth";

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
    inline constexpr auto width      = "width";
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
    const juce::StringArray wobbleTargets { "Pitch", "Volume", "Filter", "All" };
    const juce::StringArray wobbleRates { "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/32" };
    const juce::StringArray wobbleShapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
    const juce::StringArray chopPatterns { "1/8", "1/16", "1/16T", "1/32", "Roll", "Gross", "Stutter" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Which parameters belong to which ADVANCED section (for section reset)
juce::StringArray sectionParameters (const juce::String& section);
