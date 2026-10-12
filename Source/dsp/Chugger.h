#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief "O Chugger" — The Ultimate 1-Knob Monster Processor for Heavy Guitar Rhythm.
 *
 * Designed from the ground up by combining audio engineering acoustics and modern DSP:
 *
 * 1. Subsonic Flub High-Pass (65-75 Hz):
 *    Tightens unwanted sub-rumble that muddies high-gain amps and causes flubby intermodulation.
 *
 * 2. Transient Detonation Detector (Differential Envelope Follower):
 *    Detects the instantaneous attack of palm-muted pick strikes:
 *    Δ(t) = max(0, Env_fast(t) - Env_slow(t)).
 *
 * 3. Dynamic Low-End Resonator (92 Hz, Q=2.4):
 *    Expands dynamically on pick impact by up to +8.5 dB, producing that chest-thumping,
 *    percussive punch characteristic of elite metal rhythm productions.
 *
 * 4. Dynamic Mud Scoop (280 Hz, Q=1.8):
 *    Dynamically compresses the tubby, boxy low-mid energy during heavy palm mutes,
 *    ensuring the guitar remains tight and laser-focused.
 *
 * 5. Pick "Clack" & Bite Injector (2.4 kHz):
 *    Boosts crisp transient bite on pick attack to guarantee definition in fast djent
 *    riffs and extreme low drop-tunings.
 *
 * 6. Low-Band Asymmetric Harmonic Saturator (<140 Hz):
 *    Generates 2nd and 3rd order harmonics on low frequencies to impart psychoacoustic
 *    weight and visceral growl even through small monitors and headphones.
 */
class Chugger
{
public:
    Chugger();
    ~Chugger() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets the "Chug" monster knob value (0.0 to 1.0). */
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

    // Per-channel biquad filters and state variables
    struct ChannelState
    {
        // Sub-cut 2nd order HPF
        juce::dsp::IIR::Filter<float> subCutFilter;

        // Dynamic Low-End Resonator Bell (92 Hz)
        juce::dsp::IIR::Filter<float> lowPunchFilter;

        // Dynamic Mud Scooper Bell (280 Hz)
        juce::dsp::IIR::Filter<float> mudScoopFilter;

        // Transient Clack Bell (2.4 kHz)
        juce::dsp::IIR::Filter<float> clackFilter;

        // Crossover filter for sub-band saturation (140 Hz)
        juce::dsp::IIR::Filter<float> lowBandLpf;

        // Envelope followers for transient detection
        float envFast = 0.0f;
        float envSlow = 0.0f;
        float envLowBand = 0.0f;

        void reset();
    };

    std::array<ChannelState, kMaxChannels> channels;

    // Smoothing coefficients
    float envFastCoeff = 0.0f;
    float envSlowCoeff = 0.0f;
    float envLowCoeff  = 0.0f;

    void updateFilterCoefficients (float chug);
};
