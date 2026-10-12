#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief "O Chugger" — Dynamic Palm-Mute Tightener & Transient Attack Shaper.
 *
 * Designed to deliver the tightness and percussive aggression of analog boost pedals
 * (Fortin 33 / TC Integrated Preamp / Precision Drive) WITHOUT their fatal flaw
 * of permanently making the guitar sound thin, honky, and nasal on open chords.
 *
 * Algorithm Architecture:
 * 1. Intelligent Palm-Mute Detector:
 *    Monitors transient impact vs. decay time. Recognizes when the player is executing
 *    a damped percussive palm mute versus letting open chords sustain.
 *
 * 2. Dynamic Pre-Tightening (Dynamic Low-Cut):
 *    - Open chords: Filter sits transparently at ~40 Hz, preserving 100% of the guitar's
 *      natural body, pickup richness, and low-end girth.
 *    - Palm mutes: The cutoff dynamically shifts up to 140 Hz - 240 Hz (proportional to Chug),
 *      instantly stripping the flubby sub-bass that causes amplifier blocking distortion.
 *
 * 3. Transient "Pick Clank / Grind" Attack Injector (3.5 kHz):
 *    Injects aggressive percussive bite exclusively during the first 8-15 ms of pick attack.
 *    Gives razor-sharp pick definition in low tunings without adding permanent high-end fizz.
 *
 * 4. Low-Frequency Resonance Damping:
 *    Damps loose ringing in the 120-180 Hz region during note decay, ensuring palm mutes
 *    stop with surgical, bone-dry precision.
 */
class Chugger
{
public:
    Chugger();
    ~Chugger() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets the "Chug" tightness and aggression knob (0.0 to 1.0). */
    void setChug (float chugAmount);

    /** Enables or bypasses the Chugger effect. */
    void setBypassed (bool bypassed) { isBypassed = bypassed; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kMaxChannels = 2;

    double sampleRate = 48000.0;
    float chugParam = 0.5f;
    bool isBypassed = false;

    juce::SmoothedValue<float> smoothedChug;

    // Per-channel DSP filters and state
    struct ChannelState
    {
        // 1. Dynamic Variable State Filter (SVF) for instantaneous low-cut tightening
        juce::dsp::StateVariableTPTFilter<float> dynamicHpFilter;

        // 2. Transient Attack Clank Filter (Peaking bell at 3.5 kHz)
        juce::dsp::IIR::Filter<float> clankBellFilter;

        // 3. Low-mid resonance damper bell (140 Hz)
        juce::dsp::IIR::Filter<float> lowDamperFilter;

        // Envelope followers for palm-mute and transient detection
        float envFast = 0.0f;
        float envSlow = 0.0f;
        float envTightness = 0.0f;

        void reset();
    };

    std::array<ChannelState, kMaxChannels> channels;

    // Time constants for envelope tracking
    float fastAttackCoeff  = 0.0f;
    float slowDecayCoeff   = 0.0f;
    float tightDecayCoeff  = 0.0f;

    void updateStaticCoefficients();
};
