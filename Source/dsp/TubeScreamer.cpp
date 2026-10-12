#include "TubeScreamer.h"
#include <algorithm>

void TubeScreamer::ChannelState::reset()
{
    vc_z = 0.0;
    vin_prev = 0.0;
    vd_prev = 0.0;
    iCf_prev = 0.0;

    toneFilter.reset();
    postLpf.reset();
}

TubeScreamer::TubeScreamer()
{
    updateToneCoefficients();
}

void TubeScreamer::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // 2x oversampling with polyphase IIR filter (ultra low latency, analog smoothness)
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (spec.numChannels),
        1, // 2^1 = 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    oversampling->initProcessing (static_cast<size_t> (spec.maximumBlockSize));
    internalSampleRate = sampleRate * 2.0;

    smoothedDrive.reset (sampleRate, 0.02);
    smoothedLevel.reset (sampleRate, 0.02);

    updateToneCoefficients();
    reset();
}

void TubeScreamer::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    for (auto& s : channelStates)
        s.reset();
}

void TubeScreamer::setDrive (float drive)
{
    driveParam = juce::jlimit (0.0f, 1.0f, drive);
    smoothedDrive.setTargetValue (driveParam);
}

void TubeScreamer::setTone (float tone)
{
    toneParam = juce::jlimit (0.0f, 1.0f, tone);
    updateToneCoefficients();
}

void TubeScreamer::setLevel (float level)
{
    levelParam = juce::jlimit (0.0f, 1.0f, level);
    smoothedLevel.setTargetValue (levelParam);
}

void TubeScreamer::updateToneCoefficients()
{
    if (sampleRate <= 0.0) return;

    // TS808 Active Tone Stage: Shelving/tilt filter pivoting around 1.2 kHz
    // Tone 0.0 -> -8.5 dB cut at highs
    // Tone 0.5 -> 0 dB flat
    // Tone 1.0 -> +8.5 dB boost at highs
    const float toneGainDb = (toneParam - 0.5f) * 17.0f;
    const float toneGainLin = juce::Decibels::decibelsToGain (toneGainDb);
    auto toneCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 1200.0f, 0.707f, toneGainLin);

    // TS808 Output buffer lowpass filter (R = 10k, C = 0.1 uF -> fc approx 7.2 kHz)
    auto postLpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 7200.0f, 0.707f);

    for (auto& s : channelStates)
    {
        *s.toneFilter.coefficients = *toneCoeffs;
        *s.postLpf.coefficients    = *postLpfCoeffs;
    }
}

double TubeScreamer::solveDiodeVoltage (double iTarget, double Geq, double initialGuess) const
{
    const double vDenom = kEta * kVt; // ~0.0325 V
    double vd = juce::jlimit (-1.2, 1.2, initialGuess);

    // Newton-Raphson iteration with adaptive damping to prevent numerical overflow
    for (int iter = 0; iter < 6; ++iter)
    {
        const double x = vd / vDenom;
        // Clamp argument to avoid std::exp overflow
        const double clampedX = juce::jlimit (-35.0, 35.0, x);

        const double sinhVal = std::sinh (clampedX);
        const double coshVal = std::cosh (clampedX);

        const double fVal = Geq * vd + 2.0 * kIs * sinhVal - iTarget;
        const double fPrime = Geq + (2.0 * kIs / vDenom) * coshVal;

        if (std::abs (fPrime) < 1e-12)
            break;

        double deltaV = fVal / fPrime;
        // Damping: limit voltage jump per step to prevent wild oscillations
        deltaV = juce::jlimit (-0.08, 0.08, deltaV);

        vd -= deltaV;

        if (std::abs (deltaV) < 1e-6)
            break; // Converged
    }

    return vd;
}

void TubeScreamer::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);

    // 1. Oversample input to 2x (internal rate)
    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampling->processSamplesUp (block);

    const int oversampledNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
    const double T = 1.0 / internalSampleRate;

    // Ground branch discretization coefficients
    // Cz = 0.047 uF, Rz = 4.7 kOhm -> fc = 720 Hz
    const double alphaZ = T / (2.0 * kRz * kCz);
    const double cZ1 = (1.0 - alphaZ) / (1.0 + alphaZ);
    const double cZ2 = alphaZ / (1.0 + alphaZ);

    // Feedback capacitor conductance: Gc = 2 * Cf / T
    const double Gc = (2.0 * kCf) / T;

    // Level output booster (TS808 Level control)
    const float levelVal = smoothedLevel.getNextValue();
    const float outMakeup = 0.4f + levelVal * 3.5f;

    for (int i = 0; i < oversampledNumSamples; ++i)
    {
        const float driveVal = smoothedDrive.getNextValue();
        // Feedback resistance: Rf = 51k + Drive * 500k
        const double Rf = kRmin + static_cast<double> (driveVal) * kRpot;
        const double Geq = (1.0 / Rf) + Gc;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& s = channelStates[static_cast<size_t> (ch)];
            float* channelData = oversampledBlock.getChannelPointer (static_cast<size_t> (ch));

            // Scale input signal (guitar signal ~ 0.2V peak)
            const double vin = static_cast<double> (channelData[i]) * 0.45;

            // 1. Solve Ground RC Network (State equation for Cz)
            const double vcz = cZ1 * s.vc_z + cZ2 * (vin + s.vin_prev);
            s.vc_z = vcz;
            s.vin_prev = vin;

            // Current drawn from inverting node to ground:
            const double iz = (vin - vcz) / kRz;

            // 2. Feedback Loop History Current (Cf)
            const double iCf_hist = -Gc * s.vd_prev - s.iCf_prev;
            const double iTarget = iz - iCf_hist;

            // 3. Newton-Raphson Non-Linear Diode Solver
            const double vd = solveDiodeVoltage (iTarget, Geq, s.vd_prev);

            // Update Cf capacitor current state
            const double iCf = Gc * (vd - s.vd_prev) - s.iCf_prev;
            s.vd_prev = vd;
            s.iCf_prev = iCf;

            // 4. TS808 Output equation: Vout = Vin + Vd (Intrinsic clean-blend!)
            const double vout = vin + vd;

            channelData[i] = static_cast<float> (vout * 2.2);
        }
    }

    // 2. Downsample back to host rate
    oversampling->processSamplesDown (block);

    // 3. Apply TS808 Tone Stack and Output Level at host rate
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& s = channelStates[static_cast<size_t> (ch)];
        float* chData = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            // Active Tone EQ
            float x = s.toneFilter.processSample (chData[i]);
            // Output lowpass buffer filter
            x = s.postLpf.processSample (x);

            chData[i] = x * outMakeup;
        }
    }
}
