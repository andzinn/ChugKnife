#include "PitchShifter.h"
#include <algorithm>

void PitchShifter::DelayRing::init (int minCapacity)
{
    int size = 1;
    while (size < minCapacity)
        size <<= 1;

    data.assign (static_cast<size_t> (size), 0.0f);
    mask = static_cast<uint32_t> (size - 1);
}

void PitchShifter::DelayRing::clear()
{
    std::fill (data.begin(), data.end(), 0.0f);
}

void PitchShifter::DelayRing::write (int64_t idx, float sample)
{
    data[static_cast<uint32_t> (idx) & mask] = sample;
}

float PitchShifter::DelayRing::at (int64_t idx) const
{
    return data[static_cast<uint32_t> (idx) & mask];
}

float PitchShifter::DelayRing::readHermite (double pos) const
{
    const auto i = static_cast<int64_t> (std::floor (pos));
    const float t = static_cast<float> (pos - static_cast<double> (i));

    const float xm1 = at (i - 1);
    const float x0  = at (i);
    const float x1  = at (i + 1);
    const float x2  = at (i + 2);

    const float c = 0.5f * (x1 - xm1);
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + 0.5f * (x2 - x0);
    const float b = w + a;

    return ((a * t - b) * t + c) * t + x0;
}

PitchShifter::PitchShifter()
{
    updateParameters();
}

void PitchShifter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Buffer capacity for 150 ms of audio (plenty of safety margin)
    const int minCapacity = static_cast<int> (sampleRate * 0.150);
    for (auto& ring : audioRings)
        ring.init (minCapacity);

    // Onset detector HPF (~600 Hz)
    const float w = 2.0f * juce::MathConstants<float>::pi * static_cast<float> (kDetectorHpfHz) / static_cast<float> (sampleRate);
    hpCoeff = std::exp (-w);

    // Fast (2 ms) and slow (40 ms) energy smoother for pick attack onset
    fastCoeff = std::exp (-1.0f / (0.002f * static_cast<float> (sampleRate)));
    slowCoeff = std::exp (-1.0f / (0.040f * static_cast<float> (sampleRate)));
    refractorySamplesTotal = static_cast<int> (0.035 * sampleRate);

    updateParameters();
    reset();
}

void PitchShifter::reset()
{
    for (auto& ring : audioRings)
        ring.clear();

    writePos = 0;
    tapA = -minDelaySamples;
    tapB = tapA;
    isCrossfading = false;
    fadeProgress = 0.0;
    hpZ1 = 0.0f;
    envFast = 0.0f;
    envSlow = 0.0f;
    refractorySamplesRemaining = 0;
}

void PitchShifter::setSemitones (int semitones)
{
    semitones = juce::jlimit (-12, 12, semitones);
    if (semitones != currentSemitones)
    {
        currentSemitones = semitones;
        pitchRatio = std::pow (2.0, static_cast<double> (currentSemitones) / 12.0);
    }
}

void PitchShifter::updateParameters()
{
    if (sampleRate <= 0.0) return;

    minDelaySamples = static_cast<int> (sampleRate * kMinDelayMs * 0.001);
    maxDelaySamples = static_cast<int> (sampleRate * kMaxWindowMs * 0.001);
    fadeSamples = std::max (16, static_cast<int> (sampleRate * kFadeMs * 0.001));
    onsetFadeSamples = std::max (8, static_cast<int> (sampleRate * kOnsetFadeMs * 0.001));
    corrWindowSamples = std::min (maxDelaySamples, static_cast<int> (sampleRate * 0.020)); // 20 ms corr window
}

void PitchShifter::triggerSplice (double targetDelaySamples, int fadeLength)
{
    targetDelaySamples = juce::jlimit (static_cast<double> (minDelaySamples),
                                       static_cast<double> (maxDelaySamples),
                                       targetDelaySamples);

    tapB = static_cast<double> (writePos) - targetDelaySamples;
    isCrossfading = true;
    fadeProgress = 0.0;
    fadeStep = 1.0 / static_cast<double> (std::max (1, fadeLength));
}

double PitchShifter::findBestCorrelationLag()
{
    // Search best candidate lag in the ring buffer using Normalized Cross Correlation (NCC)
    // to match recent tapA waveform with candidate waveform.
    const auto& ring = audioRings[0];
    const int N = corrWindowSamples;
    if (N <= 0) return static_cast<double> (minDelaySamples);

    double bestScore = -1e9;
    int bestDelay = minDelaySamples;

    // Evaluate candidate delays with coarse step (4 samples)
    const int step = 4;
    for (int candidateDelay = minDelaySamples; candidateDelay < maxDelaySamples - N; candidateDelay += step)
    {
        double dotProd = 0.0;
        double energyRef = 0.0;
        double energyCand = 0.0;

        for (int k = 0; k < N; k += 2) // evaluate every 2nd sample for speed
        {
            const float refSample  = ring.readHermite (tapA - k);
            const float candSample = ring.at (writePos - candidateDelay - k);

            dotProd += refSample * candSample;
            energyRef += refSample * refSample;
            energyCand += candSample * candSample;
        }

        const double denom = std::sqrt (energyRef * energyCand) + 1e-6;
        const double ncc = dotProd / denom;

        // Cost scoring: prefer large delay jump to minimize splices per second
        const double jump = std::abs ((static_cast<double> (writePos) - tapA) - candidateDelay);
        const double score = (jump > 10.0) ? (ncc + 0.15 * (jump / static_cast<double> (maxDelaySamples))) : ncc;

        if (score > bestScore)
        {
            bestScore = score;
            bestDelay = candidateDelay;
        }
    }

    return static_cast<double> (bestDelay);
}

void PitchShifter::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed || currentSemitones == 0)
        return; // Passthrough if 0 semitones or bypassed

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Write incoming samples to ring buffers & calculate mono detector sample
        float monoInput = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float sample = buffer.getSample (ch, i);
            audioRings[static_cast<size_t> (ch)].write (writePos, sample);
            monoInput += sample;
        }
        monoInput /= static_cast<float> (numChannels);

        // 2. Pick Attack (Onset) Detector
        // High-pass filter
        const float hpOut = monoInput - hpZ1;
        hpZ1 = monoInput * (1.0f - hpCoeff) + hpZ1 * hpCoeff;
        const float absHp = std::abs (hpOut);

        // Fast & slow envelope
        envFast = fastCoeff * envFast + (1.0f - fastCoeff) * absHp;
        envSlow = slowCoeff * envSlow + (1.0f - slowCoeff) * absHp;

        if (refractorySamplesRemaining > 0)
        {
            --refractorySamplesRemaining;
        }
        else
        {
            // Transient trigger: fast envelope surges over slow baseline by factor of 3 (~9.5 dB)
            if (envFast > (envSlow * 2.8f + 0.01f))
            {
                // Resync tap immediately to buffer head!
                triggerSplice (static_cast<double> (minDelaySamples), onsetFadeSamples);
                refractorySamplesRemaining = refractorySamplesTotal;
            }
        }

        // 3. Normal delay drift boundaries check
        const double currentDelay = static_cast<double> (writePos) - tapA;
        if (! isCrossfading)
        {
            if (currentDelay >= static_cast<double> (maxDelaySamples))
            {
                // Down-shift tap reached end: find best lag and splice back
                const double bestLag = findBestCorrelationLag();
                triggerSplice (bestLag, fadeSamples);
            }
            else if (currentDelay <= static_cast<double> (minDelaySamples) && pitchRatio > 1.0)
            {
                // Up-shift tap gained on write head: splice back
                triggerSplice (static_cast<double> (maxDelaySamples * 0.75), fadeSamples);
            }
        }

        // 4. Sample playback from ring buffers
        float gainA = 1.0f;
        float gainB = 0.0f;

        if (isCrossfading)
        {
            // Raised-cosine crossfade
            const double rad = fadeProgress * juce::MathConstants<double>::pi;
            gainA = static_cast<float> (0.5 * (1.0 + std::cos (rad)));
            gainB = static_cast<float> (0.5 * (1.0 - std::cos (rad)));

            fadeProgress += fadeStep;
            if (fadeProgress >= 1.0)
            {
                tapA = tapB;
                isCrossfading = false;
                fadeProgress = 0.0;
            }
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto& ring = audioRings[static_cast<size_t> (ch)];
            const float sampleA = ring.readHermite (tapA);
            float outputSample = sampleA;

            if (isCrossfading)
            {
                const float sampleB = ring.readHermite (tapB);
                outputSample = gainA * sampleA + gainB * sampleB;
            }

            buffer.getWritePointer (ch)[i] = outputSample;
        }

        // 5. Advance taps at pitchRatio speed
        tapA += pitchRatio;
        if (isCrossfading)
            tapB += pitchRatio;

        ++writePos;
    }
}
