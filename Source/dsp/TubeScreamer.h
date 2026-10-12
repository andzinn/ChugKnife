#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief Virtual Analog TS808 Circuit Model with Newton-Raphson Non-Linear Diode Solver.
 *
 * Physical modeling of the classic Ibanez TS808 circuit:
 * 1. Non-inverting Op-Amp stage with antiparallel silicon clipping diodes (1N4148/1N914)
 *    in the negative feedback loop.
 * 2. Feedback branch: Rf = (51k + Drive * 500k) || Cf (51 pF) || Antiparallel Diodes.
 * 3. Ground branch: Rz = 4.7k in series with Cz = 0.047 uF (creates the 720 Hz high-pass).
 * 4. Shockley Diode equation: Id(Vd) = 2 * Is * sinh(Vd / (eta * Vt)), solved implicitly
 *    at each audio sample using an unconditionally convergent Newton-Raphson solver.
 * 5. Intrinsic TS808 clean-blend behavior: Vout = Vin + Vd.
 * 6. TS808 Active Tone Stack RC network with dual-branch tone shaping.
 * 7. 2x Oversampling to ensure zero digital aliasing on high-order diode harmonics.
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

    // TS808 Physical Circuit Constants
    static constexpr double kRz = 4700.0;          // 4.7 kOhm ground resistor
    static constexpr double kCz = 0.047e-6;        // 0.047 uF ground capacitor (720 Hz cutoff)
    static constexpr double kRmin = 51000.0;       // 51 kOhm minimum feedback resistance
    static constexpr double kRpot = 500000.0;      // 500 kOhm Drive potentiometer
    static constexpr double kCf = 51.0e-12;        // 51 pF feedback capacitor
    static constexpr double kIs = 2.52e-9;         // Silicon diode saturation current (1N4148)
    static constexpr double kVt = 0.026;           // Thermal voltage (~26 mV at room temp)
    static constexpr double kEta = 1.25;           // Diode ideality factor (1.25 for 1N4148)

    double sampleRate = 48000.0;
    double internalSampleRate = 96000.0;
    float driveParam = 0.25f;
    float toneParam  = 0.50f;
    float levelParam = 0.70f;
    bool isBypassed  = false;

    // 2x Oversampler to eliminate non-linear aliasing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Per-channel state for the analog circuit
    struct ChannelState
    {
        // Ground RC state (Cz capacitor voltage)
        double vc_z = 0.0;
        double vin_prev = 0.0;

        // Feedback capacitor state (Cf)
        double vd_prev = 0.0;
        double iCf_prev = 0.0;

        // Tone stack filter state (biquad)
        juce::dsp::IIR::Filter<float> toneFilter;
        juce::dsp::IIR::Filter<float> postLpf;

        void reset();
    };

    std::array<ChannelState, kMaxChannels> channelStates;

    // Parameter smoothers
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedLevel;

    // Newton-Raphson solver for the feedback diode loop
    double solveDiodeVoltage (double iTarget, double Geq, double initialGuess) const;

    void updateToneCoefficients();
};
