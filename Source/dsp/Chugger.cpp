#include "Chugger.h"
#include <algorithm>

namespace
{
inline float calcCoeff (float timeMs, double sr)
{
    if (timeMs <= 0.001f || sr <= 0.0)
        return 0.0f;
    return std::exp (-1.0f / (timeMs * 0.001f * static_cast<float> (sr)));
}
}

void Chugger::ChannelFilters::prepare (const juce::dsp::ProcessSpec& spec)
{
    lrHighLp.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    lrHighLp.setCutoffFrequency (kHighSplitHz);
    lrHighLp.prepare (spec);

    lrHighHp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    lrHighHp.setCutoffFrequency (kHighSplitHz);
    lrHighHp.prepare (spec);

    lrLowLp.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    lrLowLp.setCutoffFrequency (kLowSplitHz);
    lrLowLp.prepare (spec);

    lrLowHp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    lrLowHp.setCutoffFrequency (kLowSplitHz);
    lrLowHp.prepare (spec);

    lrHighAp.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
    lrHighAp.setCutoffFrequency (kLowSplitHz);
    lrHighAp.prepare (spec);
}

void Chugger::ChannelFilters::reset()
{
    lrHighLp.reset();
    lrHighHp.reset();
    lrLowLp.reset();
    lrLowHp.reset();
    lrHighAp.reset();
}

Chugger::Chugger()
{
}

void Chugger::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedChug.reset (sampleRate, 0.02);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    for (auto& cf : channelFilters)
        cf.prepare (monoSpec);

    monoDetLowLp.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    monoDetLowLp.setCutoffFrequency (kLowSplitHz);
    monoDetLowLp.prepare (monoSpec);

    monoDetLowHp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    monoDetLowHp.setCutoffFrequency (kLowSplitHz);
    monoDetLowHp.prepare (monoSpec);

    updateCoefficients();
    reset();
}

void Chugger::reset()
{
    for (auto& cf : channelFilters)
        cf.reset();

    monoDetLowLp.reset();
    monoDetLowHp.reset();

    envLow = 0.0f;
    envRest = 0.0f;
    lowGainReductionDb = 0.0f;
    envHighFast = 0.0f;
    envHighSlow = 0.0f;
}

void Chugger::setChug (float chugAmount)
{
    chugParam = juce::jlimit (0.0f, 1.0f, chugAmount);
    smoothedChug.setTargetValue (chugParam);
}

void Chugger::updateCoefficients()
{
    if (sampleRate <= 0.0) return;

    // Dominance detector smoothing: ~4 ms attack, ~50 ms release
    detAttackCoeff = calcCoeff (4.0f, sampleRate);
    detReleaseCoeff = calcCoeff (50.0f, sampleRate);

    // Low compressor timing: 4 ms attack (punch passes), 80 ms release (flub clamped)
    lowCompAttackCoeff = calcCoeff (4.0f, sampleRate);
    lowCompReleaseCoeff = calcCoeff (80.0f, sampleRate);

    // High transient shaper: 2 ms fast, 25 ms slow
    highFastCoeff = calcCoeff (2.0f, sampleRate);
    highSlowCoeff = calcCoeff (25.0f, sampleRate);
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

        // 1. Mono sum for linked detection
        float mono = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            mono += buffer.getSample (c, i);
        mono /= static_cast<float> (numChannels);

        // 2. Low-Dominance Palm-Mute Detector
        const float detLow = monoDetLowLp.processSample (0, mono);
        const float detRest = monoDetLowHp.processSample (0, mono);

        const float absDetLow = std::abs (detLow);
        const float absDetRest = std::abs (detRest);

        if (absDetLow > envLow)
            envLow = detAttackCoeff * envLow + (1.0f - detAttackCoeff) * absDetLow;
        else
            envLow = detReleaseCoeff * envLow + (1.0f - detReleaseCoeff) * absDetLow;

        if (absDetRest > envRest)
            envRest = detAttackCoeff * envRest + (1.0f - detAttackCoeff) * absDetRest;
        else
            envRest = detReleaseCoeff * envRest + (1.0f - detReleaseCoeff) * absDetRest;

        const float lowDb  = juce::Decibels::gainToDecibels (envLow, -100.0f);
        const float restDb = juce::Decibels::gainToDecibels (envRest, -100.0f);
        const float dominance = lowDb - restDb;

        // Palm-mute threshold: from +6 dB at Chug 0 down to -10 dB at Chug 1
        const float thresholdDb = 6.0f - chug * 16.0f;
        float targetReductionDb = 0.0f;

        if (chug > 0.001f && dominance > thresholdDb && envLow > 1e-4f)
        {
            const float excess = dominance - thresholdDb;
            // 3:1 ratio downward compression, max 12 dB * chug
            targetReductionDb = std::min (12.0f * chug, excess * (1.0f - 1.0f / 3.0f));
        }

        // Smooth low gain reduction
        if (targetReductionDb > lowGainReductionDb)
            lowGainReductionDb = lowCompAttackCoeff * lowGainReductionDb + (1.0f - lowCompAttackCoeff) * targetReductionDb;
        else
            lowGainReductionDb = lowCompReleaseCoeff * lowGainReductionDb + (1.0f - lowCompReleaseCoeff) * targetReductionDb;

        const float lowGain = (chug > 0.001f)
            ? juce::Decibels::decibelsToGain (-lowGainReductionDb)
            : 1.0f;

        // 3. High-Band Transient Attack Shaper
        // Track high-band energy from mono sum through fast vs slow envelopes
        const float absHigh = std::abs (detRest); // High energy dominated by >150Hz / pick attack
        envHighFast = highFastCoeff * envHighFast + (1.0f - highFastCoeff) * absHigh;
        envHighSlow = highSlowCoeff * envHighSlow + (1.0f - highSlowCoeff) * absHigh;

        float highGain = 1.0f;
        if (chug > 0.001f && envHighFast > 1e-4f)
        {
            const float fastDb = juce::Decibels::gainToDecibels (envHighFast, -100.0f);
            const float slowDb = juce::Decibels::gainToDecibels (envHighSlow, -100.0f);
            const float transientDelta = std::max (0.0f, fastDb - slowDb);
            const float boostDb = std::min (12.0f, transientDelta) * 0.6f * chug;
            highGain = juce::Decibels::decibelsToGain (boostDb);
        }

        // 4. Process Phase-Coherent 3-Band Split and Recombine Per Channel
        for (int c = 0; c < numChannels; ++c)
        {
            auto& cf = channelFilters[static_cast<size_t> (c)];
            const float in = buffer.getSample (c, i);

            // Split 1: 1800 Hz (LowMid vs High)
            const float lowMid = cf.lrHighLp.processSample (0, in);
            const float highRaw = cf.lrHighHp.processSample (0, in);

            // Phase alignment: All-pass filter High band at 150 Hz
            const float high = cf.lrHighAp.processSample (0, highRaw);

            // Split 2: 150 Hz (Low vs Mid)
            const float low = cf.lrLowLp.processSample (0, lowMid);
            const float mid = cf.lrLowHp.processSample (0, lowMid);

            // Recombine with dynamic band gains (NO dry/wet blending)
            const float out = low * lowGain + mid + high * highGain;
            buffer.getWritePointer (c)[i] = out;
        }
    }
}
