#include "Chugger.h"
#include <algorithm>

void Chugger::ChannelFilters::reset()
{
    tightHpf.reset();
    attackBoost.reset();
    punchBoost.reset();
}

Chugger::Chugger()
{
    updateFilterCoefficients (chugParam);
}

void Chugger::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedChug.reset (sampleRate, 0.02); // 20 ms smoothing to prevent zipper noise
    smoothedChug.setCurrentAndTargetValue (chugParam);

    updateFilterCoefficients (chugParam);
    reset();
}

void Chugger::reset()
{
    for (auto& cf : channelFilters)
        cf.reset();

    lastChugProcessed = -1.0f;
}

void Chugger::setChug (float chugAmount)
{
    chugParam = juce::jlimit (0.0f, 1.0f, chugAmount);
    smoothedChug.setTargetValue (chugParam);
}

void Chugger::updateFilterCoefficients (float currentChug)
{
    if (sampleRate <= 0.0)
        return;

    const float clampedChug = juce::jlimit (0.0f, 1.0f, currentChug);

    // 1. Pre-Gain Tight HPF: 1st-order variable cutoff (6 dB/octave)
    // Chug 0.0: 10.0 Hz (flat/transparent, < 0.07 dB cut at 80 Hz)
    // Chug 0.5: ~195 Hz (Standard / Drop D sweet spot)
    // Chug 0.75: ~340 Hz (Drop C / Drop A 7-string)
    // Chug 1.0: 500.0 Hz (Drop F / 8-string laser precision)
    const float hpfCutoff = 10.0f + std::pow (clampedChug, 1.35f) * 490.0f;
    auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (
        static_cast<float> (sampleRate), hpfCutoff);

    // 2. High-Mid Pick Attack & Clank Boost:
    // Peaking filter centered at 2200 Hz (Q = 1.3), 0 dB up to +8.5 dB
    const float attackGainDb = clampedChug * 8.5f;
    auto attackCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, 2200.0f, 1.3f, juce::Decibels::decibelsToGain (attackGainDb));

    // 3. Cabinet Punch & Girth Resonance:
    // Peaking filter centered at 90 Hz (Q = 1.2), 0 dB up to +3.0 dB
    const float punchGainDb = clampedChug * 3.0f;
    auto punchCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, 90.0f, 1.2f, juce::Decibels::decibelsToGain (punchGainDb));

    for (auto& cf : channelFilters)
    {
        *cf.tightHpf.coefficients    = *hpfCoeffs;
        *cf.attackBoost.coefficients = *attackCoeffs;
        *cf.punchBoost.coefficients  = *punchCoeffs;
    }

    lastChugProcessed = clampedChug;
}

void Chugger::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);
    if (numSamples == 0 || numChannels == 0)
        return;

    // Check if smoothed parameter requires coefficient recalculation
    const float currentChug = smoothedChug.getNextValue();
    smoothedChug.skip (numSamples - 1); // Fast forward to end of block

    if (std::abs (currentChug - lastChugProcessed) > 0.002f)
        updateFilterCoefficients (currentChug);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& cf = channelFilters[static_cast<size_t> (ch)];
        float* channelData = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float s = channelData[i];
            s = cf.tightHpf.processSample (s);
            s = cf.attackBoost.processSample (s);
            s = cf.punchBoost.processSample (s);
            channelData[i] = s;
        }
    }
}
