#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * @brief Virtual Analog TS808 Circuit Model with Monotonically Convergent Diode Solver.
 *
 * Physical modeling of the authentic Ibanez TS808 circuit:
 * 1. Physical Volt Calibration: 1.0 FS = 4.0 V peak (matching +12 dBu instrument Hi-Z inputs).
 * 2. Non-inverting Op-Amp stage with antiparallel silicon clipping diodes (1N4148)
 *    in the negative feedback loop.
 * 3. Feedback branch: Rf = (51k + Drive * 500k) || Cf (51 pF) || Antiparallel Diodes.
 * 4. Ground branch: Rz = 4.7k in series with Cz = 0.047 uF (creates the 720 Hz high-pass).
 * 5. Monotonically convergent Newton-Raphson solver started from analytic upper bound.
 * 6. Authentic clean-blend physics: Vout = Vin + Vd.
 * 7. Active TS808 dual-branch RC tone network + 723 Hz pre-LP + output buffer.
 * 8. Passive audio-taper Level control (pure attenuation <= 0 dB, no artificial makeup gain).
 * 9. 4x Oversampling with half-band polyphase IIR for pristine diode harmonic fidelity.
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
    static constexpr double kVoltFullScale = 4.0;   // 1.0 FS = 4.0 V
    static constexpr double kRz = 4700.0;           // 4.7 kOhm ground resistor
    static constexpr double kCz = 0.047e-6;         // 0.047 uF ground capacitor (720 Hz cutoff)
    static constexpr double kRmin = 51000.0;        // 51 kOhm minimum feedback resistance
    static constexpr double kRpot = 500000.0;       // 500 kOhm Drive potentiometer
    static constexpr double kCf = 51.0e-12;         // 51 pF feedback capacitor
    static constexpr double kIs = 2.52e-9;          // 1N4148 saturation current (2.52 nA)
    static constexpr double kVt = 0.02585;          // Thermal voltage at 300 K (25.85 mV)
    static constexpr double kEta = 1.752;           // 1N4148 emission coefficient (1.752)

    double sampleRate = 48000.0;
    double internalSampleRate = 192000.0;
    float driveParam = 0.25f;
    float toneParam  = 0.50f;
    float levelParam = 0.70f;
    bool isBypassed  = false;

    // 4x Oversampler (2^2 = 4x) to eliminate non-linear aliasing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Per-channel state for the analog circuit
    struct ChannelState
    {
        // Ground RC state (Cz capacitor voltage and current)
        double vcz = 0.0;
        double iz_prev = 0.0;

        // Feedback capacitor state (Cf)
        double vd_prev = 0.0;
        double iCf_prev = 0.0;

        // Tone network filters (host rate)
        juce::dsp::IIR::Filter<float> r7c5Filter;
        juce::dsp::IIR::Filter<float> toneFilter;
        juce::dsp::IIR::Filter<float> postLpf;

        void reset();
    };

    std::array<ChannelState, kMaxChannels> channelStates;

    // Parameter smoothers
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedLevel;

    // Monotonically convergent Newton-Raphson solver for the feedback diode loop
    double solveDiodeVoltage (double iTarget, double Geq) const;

    void updateToneCoefficients();
};
