#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

K808Processor::K808Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "K808", createParameterLayout()),
      presets (apvts)
{
    auto rawFor = [this] (const char* id)
    {
        auto* v = apvts.getRawParameterValue (id);
        jassert (v != nullptr);
        return v;
    };
    raw.style = rawFor (ParamIDs::style);
    raw.kill = rawFor (ParamIDs::kill);
    raw.inGain = rawFor (ParamIDs::inGain);
    raw.bypass = rawFor (ParamIDs::bypass);
    raw.shapeOn = rawFor (ParamIDs::shapeOn);
    raw.punch = rawFor (ParamIDs::punch);
    raw.punchClick = rawFor (ParamIDs::punchClick);
    raw.length = rawFor (ParamIDs::length);
    raw.toneOn = rawFor (ParamIDs::toneOn);
    raw.sub = rawFor (ParamIDs::sub);
    raw.harmonics = rawFor (ParamIDs::harmonics);
    raw.filterOn = rawFor (ParamIDs::filterOn);
    raw.cutoff = rawFor (ParamIDs::cutoff);
    raw.resonance = rawFor (ParamIDs::resonance);
    raw.slope = rawFor (ParamIDs::slope);
    raw.tilt = rawFor (ParamIDs::tilt);
    raw.dirtOn = rawFor (ParamIDs::dirtOn);
    raw.dirtMode = rawFor (ParamIDs::dirtMode);
    raw.dirt = rawFor (ParamIDs::dirt);
    raw.dirtMix = rawFor (ParamIDs::dirtMix);
    raw.autoGain = rawFor (ParamIDs::autoGain);
    raw.oversample = rawFor (ParamIDs::oversample);
    raw.cleanLow = rawFor (ParamIDs::cleanLow);
    raw.cleanFreq = rawFor (ParamIDs::cleanFreq);
    raw.crushBits = rawFor (ParamIDs::crushBits);
    raw.postFilter = rawFor (ParamIDs::postFilter);
    raw.duckOn = rawFor (ParamIDs::duckOn);
    raw.duck = rawFor (ParamIDs::duck);
    raw.duckRel = rawFor (ParamIDs::duckRel);
    raw.duckShape = rawFor (ParamIDs::duckShape);
    raw.clipper = rawFor (ParamIDs::clipper);
    raw.ceiling = rawFor (ParamIDs::ceiling);
    raw.monoBelow = rawFor (ParamIDs::monoBelow);
    raw.outGain = rawFor (ParamIDs::outGain);
    raw.mix = rawFor (ParamIDs::mix);
    raw.phone = rawFor (ParamIDs::phone);

    bypassParam = apvts.getParameter (ParamIDs::bypass);

    // start on the first style
    presets.load (0);
}

bool K808Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    const auto monoOrStereo = [] (const AudioChannelSet& s)
    { return s == AudioChannelSet::mono() || s == AudioChannelSet::stereo(); };

    if (! monoOrStereo (in) || ! monoOrStereo (out))
        return false;
    if (in.size() > out.size())                       // mono->mono, mono->stereo, stereo->stereo
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && ! monoOrStereo (sc))
            return false;
    }
    return true;
}

void K808Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
}

void K808Processor::reset()
{
    engine.reset();
}

double K808Processor::getTailLengthSeconds() const
{
    return Engine::tailSeconds (readParams());
}

EngineParams K808Processor::readParams() const
{
    EngineParams p;
    p.style = (int) raw.style->load();
    p.kill = raw.kill->load();
    p.inGainDb = raw.inGain->load();
    p.bypass = raw.bypass->load() > 0.5f;

    p.shapeOn = raw.shapeOn->load() > 0.5f;
    p.punch = raw.punch->load();
    p.click = raw.punchClick->load();
    p.length = raw.length->load();

    p.toneOn = raw.toneOn->load() > 0.5f;
    p.subDb = raw.sub->load();
    p.harmonics = raw.harmonics->load();
    p.filterOn = raw.filterOn->load() > 0.5f;
    p.cutoff = raw.cutoff->load();
    p.resonance = raw.resonance->load();
    p.slope = (int) raw.slope->load();
    p.tiltDb = raw.tilt->load();

    p.dirtOn = raw.dirtOn->load() > 0.5f;
    p.dirtMode = (int) raw.dirtMode->load();
    p.drive = raw.dirt->load();
    p.dirtMix = raw.dirtMix->load();
    p.autoGain = raw.autoGain->load() > 0.5f;
    p.oversampling = (int) raw.oversample->load();
    p.cleanLow = raw.cleanLow->load() > 0.5f;
    p.cleanFreq = raw.cleanFreq->load();
    p.crushBits = raw.crushBits->load();
    p.postFreq = raw.postFilter->load();

    p.duckOn = raw.duckOn->load() > 0.5f;
    p.duck = raw.duck->load();
    p.duckReleaseMs = raw.duckRel->load();
    p.duckShape = raw.duckShape->load();

    p.clipper = raw.clipper->load();
    p.ceilingDb = raw.ceiling->load();
    p.monoBelow = raw.monoBelow->load();
    p.outGainDb = raw.outGain->load();
    p.mix = raw.mix->load();
    p.phone = raw.phone->load() > 0.5f;
    return p;
}

void K808Processor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    auto main = getBusBuffer (buffer, true, 0);
    const auto inChannels = getMainBusNumInputChannels();
    const auto outChannels = getMainBusNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    if (outChannels == 0 || numSamples == 0)
        return;

    auto out = getBusBuffer (buffer, false, 0);

    // mono in -> stereo out: duplicate the input
    if (inChannels == 1 && outChannels == 2)
        out.copyFrom (1, 0, out, 0, 0, numSamples);

    ignoreUnused (main);
    const auto params = readParams();

    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled() && getChannelCountOfBus (true, 1) > 0)
    {
        const auto sidechain = getBusBuffer (buffer, true, 1);   // refers to the host's data, no copy
        engine.process (out, jmin (2, outChannels), &sidechain, params);
    }
    else
    {
        engine.process (out, jmin (2, outChannels), nullptr, params);
    }
}

void K808Processor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("version", JucePlugin_VersionString, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void K808Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            presets.restoreFromState();
        }
}

AudioProcessorEditor* K808Processor::createEditor()
{
    return new K808Editor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new K808Processor();
}
