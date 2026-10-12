#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief Classic Tube Screamer style Overdrive DSP emulation.
 *
 * Emulates the iconic TS9/TS808 circuit topology:
 * 1. Pre-emphasis HPF: ~720 Hz high-pass in the feedback path, rolling off bass to keep the amp tight.
 * 2. Clipping Stage: Antiparallel diode clipper with soft saturation and subtle asymmetric even harmonics.
 * 3. Active Tone Stack: Dynamic tilt control pivoting around 1 kHz.
 * 4. Output Level Stage: Clean output volume booster.
 */
class TubeScreamer
{
public:
    TubeScreamer();
    ~TubeScreamer() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setDrive (float drive); // 0.0 to 1.0
    void setTone  (float tone);  // 0.0 to 1.0
    void setLevel (float level); // 0.0 to 1.0
    void setBypassed (bool bypassed) { isBypassed = bypassed; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kMaxChannels = 2;

    double sampleRate = 48000.0;
    float driveParam = 0.2f;
    float toneParam  = 0.5f;
    float levelParam = 0.7f;
    bool isBypassed  = false;

    // Filter states per channel
    struct ChannelFilters
    {
        // 720 Hz high-pass filter
        float hpZ1 = 0.0f;
        float hpCoeff = 0.0f;

        // Active tone low-pass & high-pass
        float toneLpZ1 = 0.0f;
        float toneLpCoeff = 0.0f;

        // Post smoothing
        float postZ1 = 0.0f;
        float postCoeff = 0.0f;

        void updateCoeffs (double sr, float tone);
        void reset();
    };

    std::array<ChannelFilters, kMaxChannels> filters;

    // Smoothed values for modulation
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedLevel;

    void updateFilterCoefficients();
};
