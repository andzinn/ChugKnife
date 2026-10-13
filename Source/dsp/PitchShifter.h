#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>

/**
 * @brief Polyphonic Pitch Shifter engine adapted from TONE3000 (https://github.com/tone-3000/tone3000-plugin).
 * Copyright (c) 2026 TONE3000.
 * Licensed under the MIT License.
 *
 * Engine Architecture:
 * - Correlation-spliced delay line (Eventide H949 / Digitech Drop lineage).
 * - Read tap speed = 2^(semitones / 12) across a ring buffer.
 * - Full Two-Octave Range (-24 to +24 semitones) with dynamic buffer budgeting.
 * - Rate-Cost Lag Selection: scores candidates by damage-per-splice (1 - ncc) over jump size,
 *   preventing micro-jumps and sideband distortion on chords and low bass.
 * - Adaptive Crossfade (30 ms to 120 ms): match-quality-driven fade law with power-normalized
 *   crossfading, eliminating periodic chuffing on open chords.
 * - Onset Re-Sync: 600 Hz HPF energy detector with dual-threshold (9 dB over floor, 6 dB over ceiling)
 *   re-syncs read tap to floor on pick attacks, delivering sub-5ms felt latency on palm mutes.
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets semitones (-24 to +24). */
    void setSemitones (int semitones);

    /** Enables or bypasses pitch shifting. */
    void setBypassed (bool bypassed);

    /** Returns the current nominal delay in samples for 4CM gate hold compensation. */
    float getCurrentDelaySamples() const;

    void process (juce::AudioBuffer<float>& buffer);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
