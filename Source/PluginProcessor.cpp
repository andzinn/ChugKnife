#include "PluginProcessor.h"
#include "PluginEditor.h"

ChugKnifeAudioProcessor::ChugKnifeAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
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

    // 3. Tube Screamer Parameters (Virtual Analog TS808)
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

    // 4. "O Chugger" Tightness & Attack Parameters
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chug", 1 },
        "Chug (Tightness & Attack)",
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

    cleanDiBuffer.setSize (static_cast<int> (spec.numChannels), samplesPerBlock);
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

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    // 1. Stage 1: Capture clean DI from Input (Guiding the Noise Gate detector)
    if (cleanDiBuffer.getNumChannels() < numChannels || cleanDiBuffer.getNumSamples() < numSamples)
        cleanDiBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        cleanDiBuffer.copyFrom (ch, 0, buffer.getReadPointer (ch), numSamples);

    // 2. Load APVTS parameters
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

    // 3. Stage 2: Front-End / Pre-Amp Chain (Pitch Shifter -> Screamer -> Chugger)
    pitchShifter.process (buffer);
    tubeScreamer.process (buffer);
    chugger.process (buffer);

    // 4. Stage 3: Noise Gate (The Internal 4CM Heart)
    // Detector listens to the clean DI from Input (cleanDiBuffer).
    // Compensate pitch-shifter latency in the gate hold timer.
    const int shifterDelaySamples = static_cast<int> (pitchShifter.getCurrentDelaySamples());
    noiseGate.setExtraHoldSamples (shifterDelaySamples);
    // Attenuation is applied to the post-distortion audio buffer.
    noiseGate.process (buffer, &cleanDiBuffer);

    // 5. Stage 4: Master Output Gain
    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = masterGain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= gain;
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
