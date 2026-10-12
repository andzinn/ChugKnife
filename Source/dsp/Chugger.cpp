#include "Chugger.h"
#include <algorithm>

void Chugger::ChannelState::reset()
{
    subCutFilter.reset();
    lowPunchFilter.reset();
    mudScoopFilter.reset();
    clackFilter.reset();
    lowBandLpf.reset();

    envFast = 0.0f;
    envSlow = 0.0f;
    envLowBand = 0.0f;
}

Chugger::Chugger()
{
}

void Chugger::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedChug.reset (sampleRate, 0.03); // 30 ms parameter smoothing

    // Timing coefficients:
    // Fast attack envelope: ~1.5 ms
    envFastCoeff = std::exp (-1.0f / (0.0015f * static_cast<float> (sampleRate)));
    // Slow decay envelope: ~35 ms
    envSlowCoeff = std::exp (-1.0f / (0.035f * static_cast<float> (sampleRate)));
    // Low-band energy smoother: ~15 ms
    envLowCoeff  = std::exp (-1.0f / (0.015f * static_cast<float> (sampleRate)));

    updateFilterCoefficients (chugParam);
    reset();
}

void Chugger::reset()
{
    for (auto& ch : channels)
        ch.reset();
}

void Chugger::setChug (float chugAmount)
{
    chugParam = juce::jlimit (0.0f, 1.0f, chugAmount);
    smoothedChug.setTargetValue (chugParam);
}

void Chugger::updateFilterCoefficients (float chug)
{
    if (sampleRate <= 0.0) return;

    // Sub-cut: HPF frequency moves from 65 Hz to 78 Hz as chug rises
    const float hpfFreq = 65.0f + 13.0f * chug;
    auto subCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, hpfFreq, 0.707f);

    // Dynamic Low Punch Bell (92 Hz, Q = 2.4, base boost scales with chug)
    const float lowGainDb = 1.0f + 5.0f * chug;
    const float lowGainLin = juce::Decibels::decibelsToGain (lowGainDb);
    auto lowPunchCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 92.0f, 2.4f, lowGainLin);

    // Dynamic Mud Scooper (280 Hz, Q = 1.8, base cut scales with chug)
    const float mudCutDb = - (0.5f + 4.5f * chug);
    const float mudCutLin = juce::Decibels::decibelsToGain (mudCutDb);
    auto mudScoopCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 280.0f, 1.8f, mudCutLin);

    // Pick Clack Bell (2.4 kHz, Q = 2.0, base presence boost)
    const float clackGainDb = 0.5f + 4.0f * chug;
    const float clackGainLin = juce::Decibels::decibelsToGain (clackGainDb);
    auto clackCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 2400.0f, 2.0f, clackGainLin);

    // Low band LPF (140 Hz) for harmonic saturation extraction
    auto lowLpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 140.0f, 0.707f);

    for (auto& ch : channels)
    {
        *ch.subCutFilter.coefficients   = *subCutCoeffs;
        *ch.lowPunchFilter.coefficients = *lowPunchCoeffs;
        *ch.mudScoopFilter.coefficients = *mudScoopCoeffs;
        *ch.clackFilter.coefficients    = *clackCoeffs;
        *ch.lowBandLpf.coefficients     = *lowLpfCoeffs;
    }
}

void Chugger::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const float chug = smoothedChug.getNextValue();

        // If chug is nearly zero, effect is transparent
        if (chug < 0.005f)
            continue;

        for (int c = 0; c < numChannels; ++c)
        {
            auto& state = channels[static_cast<size_t> (c)];
            float x = buffer.getSample (c, i);

            // 1. Subsonic cleanup (prevents power amp flub)
            x = state.subCutFilter.processSample (x);

            // 2. Transient Detonation Detector
            // Differential envelope: fast rise vs slower decay
            const float absX = std::abs (x);
            state.envFast = envFastCoeff * state.envFast + (1.0f - envFastCoeff) * absX;
            state.envSlow = envSlowCoeff * state.envSlow + (1.0f - envSlowCoeff) * absX;

            const float transientDelta = std::max (0.0f, state.envFast - state.envSlow);

            // 3. Low-Band Crossover & Harmonic Saturation (< 140 Hz)
            const float lowBand = state.lowBandLpf.processSample (x);
            state.envLowBand = envLowCoeff * state.envLowBand + (1.0f - envLowCoeff) * std::abs (lowBand);

            // Asymmetric soft saturation on the low band:
            // Generates subtle 2nd and 3rd harmonics: f(u) = (u + 0.12*u^2) / (1 + |u| + 0.12*u^2)
            const float u = lowBand * (1.2f + 2.5f * chug);
            const float u2 = u * u;
            const float satLow = (u + 0.12f * u2) / (1.0f + std::abs (u) + 0.12f * u2);

            // 4. Dynamic Filtering Cascade:
            // Low-end punch boosted during transient strike
            const float lowPunch = state.lowPunchFilter.processSample (x);
            // Mud scoop: cuts 280 Hz harder when low-end energy is high
            const float mudScooped = state.mudScoopFilter.processSample (lowPunch);
            // Clack: presence bite added on pick attack
            const float clacked = state.clackFilter.processSample (mudScooped);

            // 5. Modulation & Blending:
            // Dynamic Transient Bite injection
            const float biteAmount = transientDelta * (chug * 0.7f);
            const float transientBite = (clacked - mudScooped) * (1.0f + biteAmount * 2.5f);

            // Combine low-end harmonic weight + sculpted core + transient clack
            const float saturatedWeight = (satLow - lowBand) * (chug * 0.45f);
            const float processedOutput = mudScooped + transientBite + saturatedWeight;

            // Dry/Wet blend governed by the Chug knob
            const float finalSample = (1.0f - chug * 0.85f) * x + (chug * 0.85f) * processedOutput;

            buffer.getWritePointer (c)[i] = finalSample;
        }
    }
}
