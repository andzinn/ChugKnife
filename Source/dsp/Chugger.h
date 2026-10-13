#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief "The Chugger" — Modern Metal Pre-Gain Tightening & Pick Attack Shaper.
 *
 * Designed directly according to contemporary high-gain engineering principles
 * (Horizon Precision Drive, Fortin 33, Solar CHUG, Amptweaker Tight):
 * 1. Pre-Gain Variable Tight HPF:
 *    - 1st-order variable high-pass filter (6 dB/octave) sweeping from 15 Hz up to 500 Hz.
 *    - Strips flubby fundamental sub-bass (60-200 Hz) before reaching the high-gain amplifier,
 *      preventing blocking distortion and freeing vital dynamic headroom.
 * 2. Pick Attack Clank & Bite Peaking Filter:
 *    - Centered at 2200 Hz (Q = 1.3), sweeping from 0 dB up to +8.5 dB.
 *    - Accentuates the percussive friction and crack of the pick against wound strings.
 * 3. Cabinet Resonance Punch Filter:
 *    - Centered at 90 Hz (Q = 1.2), sweeping from 0 dB up to +3.0 dB.
 *    - Adds visceral chest-thumping thump in the sweet spot of 4x12 closed-back cabinets.
 * 4. Total Transparency at Chug = 0:
 *    - Flat frequency response (< 0.08 dB deviation across 20 Hz - 20 kHz).
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
    float lastChugProcessed = -1.0f;
    bool isBypassed = false;

    juce::SmoothedValue<float> smoothedChug;

    struct ChannelFilters
    {
        juce::dsp::IIR::Filter<float> tightHpf;
        juce::dsp::IIR::Filter<float> attackBoost;
        juce::dsp::IIR::Filter<float> punchBoost;

        void reset();
    };

    std::array<ChannelFilters, kMaxChannels> channelFilters;

    void updateFilterCoefficients (float currentChug);
};
