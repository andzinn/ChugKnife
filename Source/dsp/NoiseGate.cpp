#include "NoiseGate.h"
#include <algorithm>

namespace
{
inline float calculateOnePoleCoeff (float timeMs, double sr)
{
    if (timeMs <= 0.001f || sr <= 0.0)
        return 0.0f;
    return std::exp (-1.0f / (timeMs * 0.001f * static_cast<float> (sr)));
}
}

NoiseGate::NoiseGate()
{
    updateTimingCoefficients();
    reset();
}

void NoiseGate::DetectorFilter::updateCoeffs (double sr)
{
    if (sr <= 0.0) return;
    // 1-pole HPF at ~90 Hz
    const float hpW = 2.0f * juce::MathConstants<float>::pi * 90.0f / static_cast<float> (sr);
    hpA = std::exp (-hpW);

    // 1-pole LPF at ~4200 Hz
    const float lpW = 2.0f * juce::MathConstants<float>::pi * 4200.0f / static_cast<float> (sr);
    lpA = 1.0f - std::exp (-lpW);
}

void NoiseGate::DetectorFilter::reset()
{
    hpZ1 = 0.0f;
    lpZ1 = 0.0f;
}

float NoiseGate::DetectorFilter::processSample (float input)
{
    // High-pass (y = x - hpZ1; hpZ1 = x * (1 - hpA) + hpZ1 * hpA)
    const float hpOut = input - hpZ1;
    hpZ1 = input * (1.0f - hpA) + hpZ1 * hpA;

    // Low-pass
    lpZ1 += lpA * (hpOut - lpZ1);
    return lpZ1;
}

void NoiseGate::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (auto& f : detectorFilters)
        f.updateCoeffs (sampleRate);

    updateTimingCoefficients();
    reset();
}

void NoiseGate::reset()
{
    for (auto& f : detectorFilters)
        f.reset();

    cellMaxValues.fill (0.0f);
    currentCellIndex = 0;
    sampleInCellCounter = 0;
    ongoingCellMax = 0.0f;
    cachedWindowMax = 0.0f;

    currentGain = 1.0f;
    currentGainDb.store (0.0f, std::memory_order_relaxed);
    holdSamplesRemaining = 0;
    gateIsOpen = false;
}

void NoiseGate::setThreshold (float newThresholdDb)
{
    thresholdDb = juce::jlimit (-80.0f, 0.0f, newThresholdDb);
}

void NoiseGate::setExtraHoldSamples (int extraSamples)
{
    extraHoldSamples = std::max (0, extraSamples);
}

void NoiseGate::updateTimingCoefficients()
{
    attackCoeff  = calculateOnePoleCoeff (kAttackMs, sampleRate);
    releaseCoeff = calculateOnePoleCoeff (kReleaseMs, sampleRate);
    baseHoldSamplesTotal = static_cast<int> (kBaseHoldMs * 0.001f * static_cast<float> (sampleRate));
    samplesPerCell = std::max (1, static_cast<int> (0.001 * sampleRate)); // 1 ms per cell
}

void NoiseGate::updateWindowMax()
{
    float m = 0.0f;
    for (float val : cellMaxValues)
        m = std::max (m, val);
    cachedWindowMax = m;
}

void NoiseGate::process (juce::AudioBuffer<float>& mainBuffer,
                         const juce::AudioBuffer<float>* sidechainBuffer)
{
    if (isBypassed)
    {
        currentGain = 1.0f;
        currentGainDb.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const int numSamples = mainBuffer.getNumSamples();
    const int mainChannels = std::min (static_cast<int> (mainBuffer.getNumChannels()), kMaxChannels);

    const bool useSidechain = (sidechainBuffer != nullptr && sidechainBuffer->getNumChannels() > 0);
    const int scChannels = useSidechain ? std::min (static_cast<int> (sidechainBuffer->getNumChannels()), kMaxChannels) : mainChannels;

    const float openThreshLin  = juce::Decibels::decibelsToGain (thresholdDb);
    const float closeThreshLin = juce::Decibels::decibelsToGain (thresholdDb - kHysteresisDb);
    const float floorGainLin   = juce::Decibels::decibelsToGain (-kMaxRangeDb);
    const int totalHoldSamples = baseHoldSamplesTotal + extraHoldSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Detect Peak from detector channels (bandpass filtered)
        float maxDetectorAbs = 0.0f;
        for (int ch = 0; ch < scChannels; ++ch)
        {
            const float rawSample = useSidechain ? sidechainBuffer->getSample (ch, i)
                                                 : mainBuffer.getSample (ch, i);
            const float filtered = detectorFilters[static_cast<size_t> (ch)].processSample (rawSample);
            maxDetectorAbs = std::max (maxDetectorAbs, std::abs (filtered));
        }

        // 2. Advance 24 ms sliding-window peak tracker (1 ms cells)
        if (maxDetectorAbs > ongoingCellMax)
            ongoingCellMax = maxDetectorAbs;

        ++sampleInCellCounter;
        if (sampleInCellCounter >= samplesPerCell)
        {
            cellMaxValues[static_cast<size_t> (currentCellIndex)] = ongoingCellMax;
            currentCellIndex = (currentCellIndex + 1) % kNumWindowCells;
            sampleInCellCounter = 0;
            ongoingCellMax = 0.0f;
            updateWindowMax();
        }

        const float detectedPeak = std::max (cachedWindowMax, ongoingCellMax);

        // 3. State Machine: Hysteresis + Hold
        if (detectedPeak >= openThreshLin)
        {
            gateIsOpen = true;
            holdSamplesRemaining = totalHoldSamples;
        }
        else if (detectedPeak < closeThreshLin)
        {
            if (holdSamplesRemaining > 0)
                --holdSamplesRemaining;
            else
                gateIsOpen = false;
        }

        // 4. Downward Expander Target Gain
        float targetGain = floorGainLin;
        if (gateIsOpen)
        {
            targetGain = 1.0f;
        }
        else if (detectedPeak > floorGainLin && openThreshLin > floorGainLin)
        {
            // Soft knee 4:1 downward expansion down to noise floor (-90 dB)
            const float envDb = juce::Decibels::gainToDecibels (detectedPeak, -100.0f);
            const float diff = envDb - thresholdDb; // negative
            const float expandGainDb = std::max (-kMaxRangeDb, diff * 3.0f);
            targetGain = juce::Decibels::decibelsToGain (expandGainDb);
        }

        // 5. Smooth gain ramp
        if (targetGain > currentGain)
            currentGain = attackCoeff * currentGain + (1.0f - attackCoeff) * targetGain;
        else
            currentGain = releaseCoeff * currentGain + (1.0f - releaseCoeff) * targetGain;

        // Apply gain to all main channels
        for (int ch = 0; ch < mainChannels; ++ch)
            mainBuffer.getWritePointer (ch)[i] *= currentGain;
    }

    currentGainDb.store (juce::Decibels::gainToDecibels (currentGain, -kMaxRangeDb), std::memory_order_relaxed);
}
