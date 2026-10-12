#include "PluginProcessor.h"
#include "PluginEditor.h"

ChugKnifeAudioProcessor::ChugKnifeAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ChugKnifeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. Noise Gate Parameters
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gate_threshold", 1 },
        "Gate Threshold",
        juce::NormalisableRange<float> (-80.0f, 0.0f, 0.5f),
        -50.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (val, 1) + " dB"; }));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "gate_bypass", 1 },
        "Gate Bypass",
        false));

    // 2. Pitch Shifter Parameters
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "transpose_semi", 1 },
        "Transpose (Semitones)",
        -12, 12, 0,
        juce::String(),
        [] (int val, int) { return (val > 0 ? "+" : "") + juce::String (val) + " st"; }));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "transpose_bypass", 1 },
        "Transpose Bypass",
        false));

    // 3. Tube Screamer Parameters
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ts_drive", 1 },
        "Screamer Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.25f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (static_cast<int> (val * 100.0f)) + " %"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ts_tone", 1 },
        "Screamer Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.50f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (static_cast<int> (val * 100.0f)) + " %"; }));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ts_level", 1 },
        "Screamer Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.70f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (static_cast<int> (val * 100.0f)) + " %"; }));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ts_bypass", 1 },
        "Screamer Bypass",
        false));

    // 4. "O Chugger" Monster Knob Parameters
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chug", 1 },
        "Chug",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.50f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (static_cast<int> (val * 100.0f)) + " %"; }));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "chug_bypass", 1 },
        "Chug Bypass",
        false));

    // 5. Master Output
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_level", 1 },
        "Master Level",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.5f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float val, int) { return juce::String (val, 1) + " dB"; }));

    return { params.begin(), params.end() };
}

bool ChugKnifeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Main in and out must match, and be mono or stereo
    if (mainIn != mainOut)
        return false;
    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    // Sidechain (bus index 1 on input side) can be disabled, mono, or stereo
    const auto& sidechain = layouts.getChannelSet (true, 1);
    if (! sidechain.isDisabled() &&
        sidechain != juce::AudioChannelSet::mono() &&
        sidechain != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    return true;
}

void ChugKnifeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (std::max (getTotalNumInputChannels(), getTotalNumOutputChannels()));

    noiseGate.prepare (spec);
    pitchShifter.prepare (spec);
    tubeScreamer.prepare (spec);
    chugger.prepare (spec);

    cleanDiBuffer.setSize (2, samplesPerBlock);
    cleanDiBuffer.clear();

    masterGain.reset (sampleRate, 0.02);
}

void ChugKnifeAudioProcessor::releaseResources()
{
    noiseGate.reset();
    pitchShifter.reset();
    tubeScreamer.reset();
    chugger.reset();
    cleanDiBuffer.setSize (0, 0);
}

void ChugKnifeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // Get Main Input/Output buffer
    auto mainBusBuffer = getBusBuffer (buffer, true, 0);

    // Check Sidechain Bus (Input bus index 1)
    const bool hasSidechainBus = (getBusCount (true) > 1);
    juce::AudioBuffer<float> sidechainBusBuffer;
    sidechainConnected = false;

    if (hasSidechainBus)
    {
        sidechainBusBuffer = getBusBuffer (buffer, true, 1);
        if (sidechainBusBuffer.getNumChannels() > 0)
        {
            // Verify if sidechain buffer is receiving active audio
            const float scMagnitude = sidechainBusBuffer.getMagnitude (0, numSamples);
            if (scMagnitude > 1e-6f)
                sidechainConnected = true;
        }
    }

    // Capture clean DI input for internal 4CM mode if external sidechain is not active
    if (! sidechainConnected)
    {
        cleanDiBuffer.setSize (mainBusBuffer.getNumChannels(), numSamples, false, false, true);
        for (int ch = 0; ch < mainBusBuffer.getNumChannels(); ++ch)
        {
            cleanDiBuffer.copyFrom (ch, 0, mainBusBuffer.getReadPointer (ch), numSamples);
        }
    }

    // 1. UPDATE PARAMETERS FROM APVTS
    const float gateThresh = apvts.getRawParameterValue ("gate_threshold")->load();
    const bool  gateBypass = apvts.getRawParameterValue ("gate_bypass")->load() > 0.5f;
    noiseGate.setThreshold (gateThresh);
    noiseGate.setBypassed (gateBypass);

    const int  semi        = static_cast<int> (apvts.getRawParameterValue ("transpose_semi")->load());
    const bool transBypass = apvts.getRawParameterValue ("transpose_bypass")->load() > 0.5f;
    pitchShifter.setSemitones (semi);
    pitchShifter.setBypassed (transBypass);

    const float tsDrive    = apvts.getRawParameterValue ("ts_drive")->load();
    const float tsTone     = apvts.getRawParameterValue ("ts_tone")->load();
    const float tsLevel    = apvts.getRawParameterValue ("ts_level")->load();
    const bool  tsBypass   = apvts.getRawParameterValue ("ts_bypass")->load() > 0.5f;
    tubeScreamer.setDrive (tsDrive);
    tubeScreamer.setTone (tsTone);
    tubeScreamer.setLevel (tsLevel);
    tubeScreamer.setBypassed (tsBypass);

    const float chugVal    = apvts.getRawParameterValue ("chug")->load();
    const bool  chugBypass = apvts.getRawParameterValue ("chug_bypass")->load() > 0.5f;
    chugger.setChug (chugVal);
    chugger.setBypassed (chugBypass);

    const float masterDb   = apvts.getRawParameterValue ("master_level")->load();
    masterGain.setTargetValue (juce::Decibels::decibelsToGain (masterDb));

    // 2. SIGNAL PROCESSING PIPELINE
    // Step A: Transpose / Pitch Shift (Applied first to feed shifted DI to downstream pedals)
    pitchShifter.process (mainBusBuffer);

    // Step B: Tube Screamer Overdrive
    tubeScreamer.process (mainBusBuffer);

    // Step C: "O Chugger" Dynamic Palm Mute Sculptor
    chugger.process (mainBusBuffer);

    // Step D: Noise Gate (4-Cable Method detector routing)
    // If external sidechain has audio, use sidechainBusBuffer (external 4CM).
    // Otherwise, use cleanDiBuffer captured before distortion (internal 4CM).
    if (sidechainConnected)
    {
        noiseGate.process (mainBusBuffer, &sidechainBusBuffer);
    }
    else
    {
        noiseGate.process (mainBusBuffer, &cleanDiBuffer);
    }

    // Step E: Master Level
    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = masterGain.getNextValue();
        for (int ch = 0; ch < mainBusBuffer.getNumChannels(); ++ch)
        {
            mainBusBuffer.getWritePointer (ch)[i] *= gain;
        }
    }
}

juce::AudioProcessorEditor* ChugKnifeAudioProcessor::createEditor()
{
    return new ChugKnifeAudioProcessorEditor (*this);
}

void ChugKnifeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChugKnifeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChugKnifeAudioProcessor();
}
