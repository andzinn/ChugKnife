#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief "O Chugger" v3 — Phase-Coherent Palm-Mute-Aware Dynamic Tightener & Attack Shaper.
 *
 * Algorithm Architecture:
 * 1. Phase-Coherent 3-Band Linkwitz-Riley 4th Order (LR4) Crossover:
 *    - Low band: < 150 Hz
 *    - Mid band: 150 Hz - 1800 Hz
 *    - High band: > 1800 Hz
 *    - All-pass phase alignment on high band ensures Low + Mid + High sums with flat magnitude (0 dB everywhere).
 * 2. Level-Independent Palm-Mute Dominance Detector:
 *    - Measures spectral balance (Low dB - Rest dB) from mono sum.
 *    - Unaffected by pickup output or playing volume; triggers specifically on palm mutes.
 * 3. Low-Band Punch & Anti-Flub Compressor:
 *    - 4 ms attack lets initial percussive wavefront punch through.
 *    - 80 ms release clamps lingering sub-bass bloom that causes amplifier blocking.
 * 4. High-Band Transient Attack Shaper:
 *    - Fast/slow envelope follower detects pick impacts.
 *    - Injects up to +7.2 dB of pick bite exclusively in the first 10-15 ms of each transient.
 * 5. Transparent at Chug = 0:
 *    - Band gains are 1.0 -> flat magnitude, zero comb-filtering, no dry/wet blending.
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
    static constexpr float kLowSplitHz = 150.0f;
    static constexpr float kHighSplitHz = 1800.0f;

    double sampleRate = 48000.0;
    float chugParam = 0.5f;
    bool isBypassed = false;

    juce::SmoothedValue<float> smoothedChug;

    // Per-channel 3-band LR4 crossover filters
    struct ChannelFilters
    {
        juce::dsp::LinkwitzRileyFilter<float> lrHighLp; // Low+Mid path at 1800 Hz
        juce::dsp::LinkwitzRileyFilter<float> lrHighHp; // High path at 1800 Hz
        juce::dsp::LinkwitzRileyFilter<float> lrLowLp;  // Low band (< 150 Hz)
        juce::dsp::LinkwitzRileyFilter<float> lrLowHp;  // Mid band (150 - 1800 Hz)
        juce::dsp::LinkwitzRileyFilter<float> lrHighAp; // All-pass alignment for High band at 150 Hz

        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
    };

    std::array<ChannelFilters, kMaxChannels> channelFilters;

    // Mono detectors (linked across channels for stable stereo image)
    juce::dsp::LinkwitzRileyFilter<float> monoDetLowLp;
    juce::dsp::LinkwitzRileyFilter<float> monoDetLowHp;

    float envLow = 0.0f;
    float envRest = 0.0f;
    float lowGainReductionDb = 0.0f;

    float envHighFast = 0.0f;
    float envHighSlow = 0.0f;

    // Envelope time constants
    float lowCompAttackCoeff = 0.0f;
    float lowCompReleaseCoeff = 0.0f;
    float detAttackCoeff = 0.0f;
    float detReleaseCoeff = 0.0f;
    float highFastCoeff = 0.0f;
    float highSlowCoeff = 0.0f;

    void updateCoefficients();
};
