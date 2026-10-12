#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief Zero-Latency "One-Knob" Noise Gate optimized for Modern Metal / Djent.
 *
 * Key Architecture:
 * - One Knob: Threshold (dB).
 * - Sidechain Detector: Listens either to an external sidechain bus (Clean DI for 4CM)
 *   or an internal pre-distortion copy.
 * - Detector Bandpass: 90 Hz HPF (eliminates hum/sub-rumble) + 4.2 kHz LPF (cuts fizz/hiss).
 * - Instant Attack (~0.15 ms): Catches transients immediately without clipping pick hits.
 * - Hysteresis: Open at Threshold, Close at (Threshold - 5.0 dB).
 * - Surgical Hold: 12 ms hold window prevents chattering on fast staccato palm mutes.
 * - Downward Expander Release: 8 ms time constant for bone-dry stops, transitioning smoothly to note decay.
 * - Depth/Range: 90 dB attenuation for silence in high-gain rigs.
 */
class NoiseGate
{
public:
    NoiseGate();
    ~NoiseGate() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets the primary threshold in decibels (-80.0 dB to 0.0 dB). */
    void setThreshold (float thresholdDb);

    /** Enables or bypasses gating. */
    void setBypassed (bool bypassed) { isBypassed = bypassed; }

    /** Returns current gain reduction in dB (useful for metering). */
    float getCurrentReductionDb() const { return currentGainDb; }

    /**
     * @brief Process audio buffer.
     * @param mainBuffer Main audio buffer to attenuate.
     * @param sidechainBuffer Optional sidechain buffer (DI clean guitar for 4CM).
     *                        If null or empty, mainBuffer is used as detector source.
     */
    void process (juce::AudioBuffer<float>& mainBuffer,
                  const juce::AudioBuffer<float>* sidechainBuffer = nullptr);

private:
    static constexpr int kMaxChannels = 2;
    static constexpr float kAttackMs = 0.15f;
    static constexpr float kHoldMs   = 12.0f;
    static constexpr float kReleaseMs = 8.0f;
    static constexpr float kHysteresisDb = 5.0f;
    static constexpr float kMaxRangeDb   = 90.0f;

    double sampleRate = 48000.0;
    bool isBypassed = false;
    float thresholdDb = -50.0f;

    // Filter coefficients & states for detector bandpass (90 Hz HPF, 4.2 kHz LPF)
    struct DetectorFilter
    {
        float hpZ1 = 0.0f;
        float lpZ1 = 0.0f;
        float hpA = 0.0f;
        float lpA = 0.0f;

        void updateCoeffs (double sr);
        void reset();
        float processSample (float input);
    };

    std::array<DetectorFilter, kMaxChannels> detectorFilters;

    // Envelope and gain smoothing
    float envelope = 0.0f;
    float currentGain = 1.0f;
    float currentGainDb = 0.0f;
    int holdSamplesRemaining = 0;
    bool gateIsOpen = false;

    // Smoothing coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    int holdSamplesTotal = 0;

    void updateTimingCoefficients();
};
