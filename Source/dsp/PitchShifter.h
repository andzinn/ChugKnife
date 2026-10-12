#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <cmath>

/**
 * @brief Polyphonic Drop-Tuning Pitch Shifter inspired by Tone3000.
 *
 * Algorithm details:
 * - Correlation-spliced delay line (SOLA / Eventide H949 approach).
 * - Ring buffer with 4-point Hermite cubic interpolation.
 * - Read tap speed = 2^(semitones / 12).
 * - Transient Onset Re-Sync: A 600 Hz HPF energy detector identifies pick attacks.
 *   On attack, the read tap is spliced to the buffer head (2-3 ms delay) with a fast
 *   2 ms raised-cosine fade. Transient attacks arrive with ~2-4 ms perceptual latency!
 * - Periodic Splice: When the tap reaches the window boundary, a normalized cross-correlation
 *   (NCC) lag search finds the best phase alignment to crossfade cleanly without comb filtering.
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets semitones (-12 to +12). */
    void setSemitones (int semitones);

    /** Enables or bypasses pitch shifting. */
    void setBypassed (bool bypassed) { isBypassed = bypassed; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kMaxChannels = 2;
    static constexpr double kMinDelayMs = 2.5;     // Lowest delay behind write head (ms)
    static constexpr double kMaxWindowMs = 35.0;   // Buffer splice window (ms)
    static constexpr double kFadeMs = 15.0;        // Periodic splice crossfade (ms)
    static constexpr double kOnsetFadeMs = 2.0;    // Pick attack re-sync crossfade (ms)
    static constexpr double kDetectorHpfHz = 600.0;// Onset detector HPF cutoff (Hz)

    double sampleRate = 48000.0;
    int currentSemitones = 0;
    double pitchRatio = 1.0;
    bool isBypassed = false;

    // Power-of-two ring buffer with Hermite interpolation
    struct DelayRing
    {
        std::vector<float> data;
        uint32_t mask = 0;

        void init (int minCapacity);
        void clear();
        void write (int64_t idx, float sample);
        float at (int64_t idx) const;
        float readHermite (double pos) const;
    };

    std::array<DelayRing, kMaxChannels> audioRings;
    int64_t writePos = 0;

    // Tap pointers and crossfade states
    double tapA = 0.0;
    double tapB = 0.0;
    bool isCrossfading = false;
    double fadeProgress = 0.0;
    double fadeStep = 0.0;

    // Converted sample boundaries
    int minDelaySamples = 0;
    int maxDelaySamples = 0;
    int fadeSamples = 0;
    int onsetFadeSamples = 0;
    int corrWindowSamples = 0;

    // Onset detector (HPF + energy follower)
    float hpZ1 = 0.0f;
    float hpCoeff = 0.0f;
    float envFast = 0.0f;
    float envSlow = 0.0f;
    float fastCoeff = 0.0f;
    float slowCoeff = 0.0f;
    int refractorySamplesRemaining = 0;
    int refractorySamplesTotal = 0;

    // Internal helper methods
    void updateParameters();
    void triggerSplice (double targetDelaySamples, int fadeLength);
    double findBestCorrelationLag();
};
