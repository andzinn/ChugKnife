#include "PluginProcessor.h"
#include "PluginEditor.h"

ChugKnifeAudioProcessor::ChugKnifeAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)   // Bus 0 In:  Main In (Guitar DI)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)   // Bus 0 Out: Main Out (Post-Gate)
          .withInput  ("Return", juce::AudioChannelSet::stereo(), true)   // Bus 1 In:  Return (From Amp)
          .withOutput ("Send",   juce::AudioChannelSet::stereo(), true)), // Bus 1 Out: Send (To Amp)
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

    // Return (Bus 1 In) can be disabled, mono, or stereo
    if (layouts.inputBuses.size() > 1)
    {
        const auto& returnIn = layouts.getChannelSet (true, 1);
        if (! returnIn.isDisabled() &&
            returnIn != juce::AudioChannelSet::mono() &&
            returnIn != juce::AudioChannelSet::stereo())
            return false;
    }

    // Send (Bus 1 Out) can be disabled, mono, or stereo
    if (layouts.outputBuses.size() > 1)
    {
        const auto& sendOut = layouts.getChannelSet (false, 1);
        if (! sendOut.isDisabled() &&
            sendOut != juce::AudioChannelSet::mono() &&
            sendOut != juce::AudioChannelSet::stereo())
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

    preAmpBuffer.setSize (2, samplesPerBlock);
    preAmpBuffer.clear();

    postAmpBuffer.setSize (2, samplesPerBlock);
    postAmpBuffer.clear();

    masterGain.reset (sampleRate, 0.02);
}

void ChugKnifeAudioProcessor::releaseResources()
{
    noiseGate.reset();
    pitchShifter.reset();
    tubeScreamer.reset();
    chugger.reset();

    cleanDiBuffer.setSize (0, 0);
    preAmpBuffer.setSize (0, 0);
    postAmpBuffer.setSize (0, 0);
}

void ChugKnifeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // 1. Access the 4 distinct 4CM buses:
    // Input  (Main In, bus 0) - Guitar DI
    auto inputBus  = getBusBuffer (buffer, true, 0);

    // Return (Aux In, bus 1) - Coming back from Amp
    const bool hasReturnBus = (getBusCount (true) > 1);
    auto returnBus = hasReturnBus ? getBusBuffer (buffer, true, 1) : juce::AudioBuffer<float>();

    // Output (Main Out, bus 0) - Out to monitors post-gate
    auto outputBus = getBusBuffer (buffer, false, 0);

    // Send   (Aux Out, bus 1) - Sending pre-amp signal to Amp
    const bool hasSendBus = (getBusCount (false) > 1);
    auto sendBus   = hasSendBus ? getBusBuffer (buffer, false, 1) : juce::AudioBuffer<float>();

    const int inChannels  = inputBus.getNumChannels();
    const int outChannels = outputBus.getNumChannels();

    // 2. Stage 1: Capture clean DI from Input (Guiding the Noise Gate detector)
    cleanDiBuffer.setSize (inChannels, numSamples, false, false, true);
    for (int ch = 0; ch < inChannels; ++ch)
        cleanDiBuffer.copyFrom (ch, 0, inputBus.getReadPointer (ch), numSamples);

    // 3. Stage 2: Front-End / Pre-Amp Chain (Pitch Shifter -> Screamer -> Chugger)
    preAmpBuffer.setSize (inChannels, numSamples, false, false, true);
    for (int ch = 0; ch < inChannels; ++ch)
        preAmpBuffer.copyFrom (ch, 0, cleanDiBuffer.getReadPointer (ch), numSamples);

    // Load APVTS parameters
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

    // Process front-end stages:
    pitchShifter.process (preAmpBuffer);
    tubeScreamer.process (preAmpBuffer);
    chugger.process (preAmpBuffer);

    // 4. Stage 3: Send Output (Write processed signal out to the Amp)
    if (hasSendBus && sendBus.getNumChannels() > 0)
    {
        for (int ch = 0; ch < sendBus.getNumChannels(); ++ch)
        {
            const int srcCh = std::min (ch, inChannels - 1);
            sendBus.copyFrom (ch, 0, preAmpBuffer.getReadPointer (srcCh), numSamples);
        }
    }

    // 5. Stage 4: Return Input (Audio coming back from Amp) with Normalled Fallback
    returnConnected = false;
    if (hasReturnBus && returnBus.getNumChannels() > 0)
    {
        const float retMag = returnBus.getMagnitude (0, numSamples);
        if (retMag > 1e-6f)
            returnConnected = true;
    }

    const int postChannels = std::max (inChannels, outChannels);
    postAmpBuffer.setSize (postChannels, numSamples, false, false, true);

    if (returnConnected)
    {
        // True 4CM: Read the audio returning from the amplifier!
        for (int ch = 0; ch < postChannels; ++ch)
        {
            const int srcCh = std::min (ch, returnBus.getNumChannels() - 1);
            postAmpBuffer.copyFrom (ch, 0, returnBus.getReadPointer (srcCh), numSamples);
        }
    }
    else
    {
        // Normalled connection (when running without external amp routing):
        // Send feeds directly into Return internally!
        for (int ch = 0; ch < postChannels; ++ch)
        {
            const int srcCh = std::min (ch, inChannels - 1);
            postAmpBuffer.copyFrom (ch, 0, preAmpBuffer.getReadPointer (srcCh), numSamples);
        }
    }

    // 6. Stage 5: Noise Gate (The 4CM Heart)
    // Detector listens to the clean DI from Input (cleanDiBuffer).
    // Attenuation is applied to the post-amp audio (postAmpBuffer).
    noiseGate.process (postAmpBuffer, &cleanDiBuffer);

    // 7. Stage 6: Master Output (Post-Gate to Output)
    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = masterGain.getNextValue();
        for (int ch = 0; ch < outChannels; ++ch)
        {
            const int srcCh = std::min (ch, postChannels - 1);
            const float sample = postAmpBuffer.getSample (srcCh, i) * gain;
            outputBus.getWritePointer (ch)[i] = sample;
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
