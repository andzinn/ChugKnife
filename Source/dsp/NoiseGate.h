#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>

/**
 * @brief Zero-Latency "One-Knob" Noise Gate optimized for Modern Heavy / Djent Guitars.
 *
 * Key Architecture:
 * - One Knob: Threshold (dB).
 * - Detector Bandpass: 90 Hz HPF (eliminates sub-rumble/DC) + 4.2 kHz LPF (cuts fizz/string scrape).
 * - 23-24 ms Sliding-Window Maximum: Eliminates intra-cycle ripple on low-tuned fundamentals (down to ~41 Hz, Low E/F# on 8-string).
 * - Instant Attack (~0.15 ms): Catches transients immediately.
 * - Hysteresis: Open at Threshold, Close at (Threshold - 5.0 dB).
 * - Surgical Hold: 12 ms hold window + pitch-shifter latency compensation.
 * - Downward Expander Release: 6 ms time constant for bone-dry stops.
 * - Thread-safe atomic reduction metering for GUI.
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

    /** Adds extra hold samples (e.g., to compensate pitch-shifter tap latency). */
    void setExtraHoldSamples (int extraSamples);

    /** Returns current gain reduction in dB (thread-safe for UI metering). */
    float getCurrentReductionDb() const { return currentGainDb.load (std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer.
     * @param mainBuffer Main audio buffer to attenuate.
     * @param sidechainBuffer Optional clean DI buffer (Internal 4CM detector).
     *                        If null or empty, mainBuffer is used as detector source.
     */
    void process (juce::AudioBuffer<float>& mainBuffer,
                  const juce::AudioBuffer<float>* sidechainBuffer = nullptr);

private:
    static constexpr int kMaxChannels = 2;
    static constexpr float kAttackMs = 0.15f;
    static constexpr float kBaseHoldMs = 12.0f;
    static constexpr float kReleaseMs = 6.0f;
    static constexpr float kHysteresisDb = 5.0f;
    static constexpr float kMaxRangeDb   = 90.0f;
    static constexpr int kNumWindowCells = 24; // 24 ms window (1 ms cells)

    double sampleRate = 48000.0;
    bool isBypassed = false;
    float thresholdDb = -50.0f;
    int extraHoldSamples = 0;

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

    // 24 ms Sliding-Window Maximum detector
    int samplesPerCell = 48;
    std::array<float, kNumWindowCells> cellMaxValues {};
    int currentCellIndex = 0;
    int sampleInCellCounter = 0;
    float ongoingCellMax = 0.0f;
    float cachedWindowMax = 0.0f;

    // Envelope and gain smoothing
    float currentGain = 1.0f;
    std::atomic<float> currentGainDb { 0.0f };
    int holdSamplesRemaining = 0;
    bool gateIsOpen = false;

    // Smoothing coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    int baseHoldSamplesTotal = 0;

    void updateTimingCoefficients();
    void updateWindowMax();
};
