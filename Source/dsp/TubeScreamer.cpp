#include "TubeScreamer.h"
#include <algorithm>

void TubeScreamer::ChannelState::reset()
{
    vcz = 0.0;
    iz_prev = 0.0;
    vd_prev = 0.0;
    iCf_prev = 0.0;

    r7c5Filter.reset();
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

    // 4x oversampling with polyphase IIR filter (2^2 = 4x, pristine diode harmonic fidelity)
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (spec.numChannels),
        2, // 2^2 = 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    oversampling->initProcessing (static_cast<size_t> (spec.maximumBlockSize));
    internalSampleRate = sampleRate * 4.0;

    smoothedDrive.reset (internalSampleRate, 0.02);
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

    // 1. TS808 R7/C5 passive lowpass: R7 = 1 k, C5 = 220 nF -> fc = 723.4 Hz
    auto r7c5Coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 723.4f, 0.707f);

    // 2. TS808 Active Tone Network: dual-branch RC network around 3.2 kHz
    // Tone 0.0 -> -12.0 dB cut at highs
    // Tone 0.5 -> 0 dB flat
    // Tone 1.0 -> +14.8 dB boost at highs (1 + 1k/220)
    const float gainDb = (toneParam < 0.5f)
        ? (toneParam - 0.5f) * 24.0f
        : (toneParam - 0.5f) * 29.6f;
    const float toneGainLin = juce::Decibels::decibelsToGain (gainDb);
    auto toneCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 3200.0f, 0.707f, toneGainLin);

    // 3. TS808 Output RF lowpass filter
    auto postLpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 14000.0f, 0.707f);

    for (auto& s : channelStates)
    {
        *s.r7c5Filter.coefficients = *r7c5Coeffs;
        *s.toneFilter.coefficients = *toneCoeffs;
        *s.postLpf.coefficients    = *postLpfCoeffs;
    }
}

double TubeScreamer::solveDiodeVoltage (double iTarget, double Geq) const
{
    const double absI = std::abs (iTarget);
    if (absI < 1e-15)
        return 0.0;

    const double vDenom = kEta * kVt; // 1.752 * 0.02585 V = 0.04529 V

    // Analytic upper bound for monotonically convergent Newton iteration:
    // Geq * v <= absI  ==>  v <= absI / Geq
    // 2 * Is * sinh(v / vDenom) <= absI  ==>  v <= vDenom * asinh(absI / (2 * Is))
    const double vBoundResistor = absI / Geq;
    const double vBoundDiode = vDenom * std::asinh (absI / (2.0 * kIs));
    double v = std::min (vBoundResistor, vBoundDiode);

    // Monotonically convergent Newton-Raphson from upper bound
    for (int iter = 0; iter < 8; ++iter)
    {
        const double x = v / vDenom;
        const double sinhVal = std::sinh (x);
        const double coshVal = std::cosh (x);

        const double fVal = Geq * v + 2.0 * kIs * sinhVal - absI;
        const double fPrime = Geq + (2.0 * kIs / vDenom) * coshVal;

        if (std::abs (fPrime) < 1e-12)
            break;

        const double step = fVal / fPrime;
        v -= step;

        if (std::abs (step) < 1e-7)
            break;
    }

    return (iTarget >= 0.0) ? v : -v;
}

void TubeScreamer::process (juce::AudioBuffer<float>& buffer)
{
    if (isBypassed)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (static_cast<int> (buffer.getNumChannels()), kMaxChannels);

    // 1. Oversample input to 4x (internal rate)
    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampling->processSamplesUp (block);

    const int oversampledNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
    const double T = 1.0 / internalSampleRate;

    // Ground branch discretization coefficients (Trapezoidal companion for Cz in series with Rz)
    // Rz = 4.7 kOhm, Cz = 0.047 uF -> 720 Hz HPF
    const double dT_2Cz = T / (2.0 * kCz);
    const double Gz_eq = 1.0 / (kRz + dT_2Cz);

    // Feedback capacitor conductance: Gc = 2 * Cf / T
    const double Gc = (2.0 * kCf) / T;

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

            // Physical Volt Calibration: 1.0 FS = 4.0 V peak
            const double vin = static_cast<double> (channelData[i]) * kVoltFullScale;

            // 1. Solve Ground RC Network (Trapezoidal companion for Rz + Cz)
            const double iz = Gz_eq * (vin - s.vcz - dT_2Cz * s.iz_prev);
            s.vcz += dT_2Cz * (iz + s.iz_prev);
            s.iz_prev = iz;

            // 2. Feedback Loop History Current (Cf)
            const double iCf_hist = -Gc * s.vd_prev - s.iCf_prev;
            const double iTarget = iz - iCf_hist;

            // 3. Monotonically Convergent Diode Solver
            const double vd = solveDiodeVoltage (iTarget, Geq);

            // Update Cf capacitor current state
            const double iCf = Gc * (vd - s.vd_prev) - s.iCf_prev;
            s.vd_prev = vd;
            s.iCf_prev = iCf;

            // 4. Authentic TS808 Output: Vout = Vin + Vd (Intrinsic clean-blend!)
            const double vout = vin + vd;

            // Convert back to digital Full Scale (-1.0 to +1.0 FS reference)
            channelData[i] = static_cast<float> (vout / kVoltFullScale);
        }
    }

    // 2. Downsample back to host rate
    oversampling->processSamplesDown (block);

    // 3. Apply TS808 Tone Stack and Passive Audio-Taper Level at host rate
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& s = channelStates[static_cast<size_t> (ch)];
        float* chData = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            // Passive audio-taper Level attenuator (<= 0 dB; 1.0 -> 0 dB, 0.5 -> ~-15 dB, 0.0 -> mute)
            const float curLevel = smoothedLevel.getNextValue();
            const float levelTaper = std::pow (curLevel, 2.5f);

            // Active Tone EQ (R7/C5 lowpass + high-shelf tone control)
            float x = s.r7c5Filter.processSample (chData[i]);
            x = s.toneFilter.processSample (x);
            x = s.postLpf.processSample (x);

            chData[i] = x * levelTaper;
        }
    }
}
