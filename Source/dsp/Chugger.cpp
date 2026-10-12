#include "Chugger.h"
#include <algorithm>

void Chugger::ChannelState::reset()
{
    dynamicHpFilter.reset();
    clankBellFilter.reset();
    lowDamperFilter.reset();

    envFast = 0.0f;
    envSlow = 0.0f;
    envTightness = 0.0f;
}

Chugger::Chugger()
{
}

void Chugger::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedChug.reset (sampleRate, 0.02);

    // Fast transient rise: ~1.2 ms
    fastAttackCoeff = std::exp (-1.0f / (0.0012f * static_cast<float> (sampleRate)));
    // Slow body decay: ~45 ms
    slowDecayCoeff  = std::exp (-1.0f / (0.045f * static_cast<float> (sampleRate)));
    // Tightness release: ~28 ms (fast recovery so open chords immediately regain low end)
    tightDecayCoeff = std::exp (-1.0f / (0.028f * static_cast<float> (sampleRate)));

    for (auto& ch : channels)
    {
        ch.dynamicHpFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        ch.dynamicHpFilter.setResonance (0.707f);
    }

    updateStaticCoefficients();
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

void Chugger::updateStaticCoefficients()
{
    if (sampleRate <= 0.0) return;

    // 1. Pick Clack / Bite Bell at 3.5 kHz (Q = 2.4, +6 dB boost shape)
    const float clackGainLin = juce::Decibels::decibelsToGain (6.0f);
    auto clackCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 3500.0f, 2.4f, clackGainLin);

    // 2. Low-mid resonance damper at 150 Hz (Q = 1.8, -4.5 dB cut shape)
    const float damperGainLin = juce::Decibels::decibelsToGain (-4.5f);
    auto damperCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 150.0f, 1.8f, damperGainLin);

    for (auto& ch : channels)
    {
        *ch.clankBellFilter.coefficients = *clackCoeffs;
        *ch.lowDamperFilter.coefficients = *damperCoeffs;
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

        if (chug < 0.005f)
            continue; // Fully transparent at 0

        for (int c = 0; c < numChannels; ++c)
        {
            auto& state = channels[static_cast<size_t> (c)];
            const float x = buffer.getSample (c, i);
            const float absX = std::abs (x);

            // 1. Transient Detonation & Palm-Mute Envelope Follower
            state.envFast = fastAttackCoeff * state.envFast + (1.0f - fastAttackCoeff) * absX;
            state.envSlow = slowDecayCoeff  * state.envSlow + (1.0f - slowDecayCoeff)  * absX;

            // Attack transient delta (isolated pick strike)
            const float attackDelta = std::max (0.0f, state.envFast - state.envSlow);

            // Tightness modulation: triggers on transient attack, then decays fast
            if (attackDelta > state.envTightness)
                state.envTightness = attackDelta;
            else
                state.envTightness *= tightDecayCoeff;

            const float normTight = juce::jlimit (0.0f, 1.0f, state.envTightness * 3.5f);

            // 2. Dynamic Pre-Gain Low-Cut (The Core "Anti-Flub" Tightening):
            // - Open chords / idle: Cutoff sits at 45 Hz (100% natural bass girth).
            // - On Palm-Mute strike: Cutoff dynamically leaps up to 140 Hz - 250 Hz (scaled by Chug).
            const float dynamicCutoff = 45.0f + (normTight * chug * 205.0f);
            state.dynamicHpFilter.setCutoffFrequency (dynamicCutoff);

            // Process through dynamic high-pass
            const float tightenedSample = state.dynamicHpFilter.processSample (0, x);

            // 3. Transient "Pick Clank / Grind" Injection (3.5 kHz bite):
            // Only injected during the initial 8-15 ms of pick attack.
            const float clacked = state.clankBellFilter.processSample (tightenedSample);
            const float clackBite = (clacked - tightenedSample) * (attackDelta * chug * 2.8f);

            // 4. Low-End Resonance Damping (removes the lingering 150 Hz "tubby" ringing):
            const float damped = state.lowDamperFilter.processSample (tightenedSample);
            // Engage damper smoothly during palm mute decay
            const float postDamped = tightenedSample + (damped - tightenedSample) * (normTight * chug * 0.85f);

            // 5. Combine: Tightened signal + Attack Clank bite + Damped resonance
            const float chuggedSignal = postDamped + clackBite;

            // Smooth dry/wet crossfade based on the Chug knob
            const float finalSample = (1.0f - chug * 0.9f) * x + (chug * 0.9f) * chuggedSignal;

            buffer.getWritePointer (c)[i] = finalSample;
        }
    }
}
