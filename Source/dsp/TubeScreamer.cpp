#include "TubeScreamer.h"
#include <algorithm>

void TubeScreamer::ChannelFilters::updateCoeffs (double sr, float tone)
{
    if (sr <= 0.0) return;

    // 720 Hz high-pass filter (TS op-amp feedback RC network)
    const float hpW = 2.0f * juce::MathConstants<float>::pi * 720.0f / static_cast<float> (sr);
    hpCoeff = std::exp (-hpW);

    // Active tone control: low-pass cutoff moves from 800 Hz up to 3500 Hz
    const float toneFreq = 800.0f + 2700.0f * tone;
    const float toneW = 2.0f * juce::MathConstants<float>::pi * toneFreq / static_cast<float> (sr);
    toneLpCoeff = 1.0f - std::exp (-toneW);

    // Post-clipping smoothing lowpass (around 10 kHz)
    const float postW = 2.0f * juce::MathConstants<float>::pi * 10000.0f / static_cast<float> (sr);
    postCoeff = 1.0f - std::exp (-postW);
}

void TubeScreamer::ChannelFilters::reset()
{
    hpZ1 = 0.0f;
    toneLpZ1 = 0.0f;
    postZ1 = 0.0f;
}

TubeScreamer::TubeScreamer()
{
    updateFilterCoefficients();
}

void TubeScreamer::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedDrive.reset (sampleRate, 0.02); // 20 ms smoothing
    smoothedLevel.reset (sampleRate, 0.02);

    updateFilterCoefficients();
    reset();
}

void TubeScreamer::reset()
{
    for (auto& f : filters)
        f.reset();
}

void TubeScreamer::setDrive (float drive)
{
    driveParam = juce::jlimit (0.0f, 1.0f, drive);
    smoothedDrive.setTargetValue (driveParam);
}

void TubeScreamer::setTone (float tone)
{
    toneParam = juce::jlimit (0.0f, 1.0f, tone);
    updateFilterCoefficients();
}

void TubeScreamer::setLevel (float level)
{
    levelParam = juce::jlimit (0.0f, 1.0f, level);
    smoothedLevel.setTargetValue (levelParam);
}

void TubeScreamer::updateFilterCoefficients()
{
    for (auto& f : filters)
        f.updateCoeffs (sampleRate, toneParam);
}

void TubeScreamer::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const float drive = smoothedDrive.getNextValue();
        const float level = smoothedLevel.getNextValue();

        // Gain range: from clean boost (+6 dB) to heavy screaming drive (+40 dB)
        const float preGain = 1.5f + drive * 35.0f;
        // Output makeup
        const float outGain = 0.5f + level * 2.5f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& f = filters[static_cast<size_t> (ch)];
            float x = buffer.getSample (ch, i);

            // 1. High-pass pre-emphasis at 720 Hz (op-amp feedback loop)
            const float hpOut = x - f.hpZ1;
            f.hpZ1 = x * (1.0f - f.hpCoeff) + f.hpZ1 * f.hpCoeff;

            // In Tube Screamer, the variable drive branch is highpassed, but mixed with clean pass
            const float drivePath = hpOut * preGain;
            const float mixedInput = x + drivePath;

            // 2. Diode soft clipping with subtle asymmetric even harmonics
            // Bias x slightly with a small DC offset that is removed later
            const float biased = mixedInput + 0.05f * drive;
            // Hyperbolic tangent soft saturator
            const float clipped = std::tanh (biased);

            // 3. Active Tone Filter
            // Low-pass component
            f.toneLpZ1 += f.toneLpCoeff * (clipped - f.toneLpZ1);

            // Tone mix: when tone > 0.5, mix in more high-frequency content; when < 0.5, darken
            const float toneHigh = clipped - f.toneLpZ1;
            const float toneOut = f.toneLpZ1 * (1.0f - toneParam * 0.4f) + toneHigh * (toneParam * 1.5f);

            // 4. Post-filter high cut to avoid harsh alias fizz
            f.postZ1 += f.postCoeff * (toneOut - f.postZ1);

            // 5. Output Level
            buffer.getWritePointer (ch)[i] = f.postZ1 * outGain;
        }
    }
}
