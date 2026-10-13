#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "dsp/NoiseGate.h"
#include "dsp/TubeScreamer.h"
#include "dsp/Chugger.h"

namespace
{
int totalTests = 0;
int passedTests = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        totalTests++; \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" << #cond << ")\n"; \
            return false; \
        } else { \
            passedTests++; \
            std::cout << "  [PASS] " << msg << "\n"; \
        } \
    } while(0)
}

//==============================================================================
// Test 1: NoiseGate — Low-Frequency Chatter & Fast Silence Cut
//==============================================================================
bool testNoiseGateLowFrequencyRipple()
{
    std::cout << "\n--- Running NoiseGate Low-Frequency Chatter Test ---\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };

    NoiseGate gate;
    gate.prepare (spec);
    gate.setThreshold (-45.0f); // -45 dB threshold

    // Sustained 44.0 Hz sine (Low F / Low E range on 8-string) at -30 dBFS (above threshold)
    const int numSamples = static_cast<int> (sampleRate * 0.5); // 0.5 sec = 22 cycles
    juce::AudioBuffer<float> buffer (2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float val = 0.0316f * std::sin (2.0f * juce::MathConstants<float>::pi * 44.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        buffer.setSample (0, i, val);
        buffer.setSample (1, i, val);
    }

    // Process through gate in blocks
    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> blockBuf (buffer.getArrayOfWritePointers(), 2, pos, curBlock);
        gate.process (blockBuf);
    }

    // After 24 ms sliding window fills, reduction MUST remain 0.0 dB throughout all remaining cycles!
    // No intra-cycle ripple/decay!
    float finalReduction = gate.getCurrentReductionDb();
    TEST_ASSERT (finalReduction >= -0.05f, "Gate stays 100% open on sustained 44 Hz note without ripple");

    // Now feed silence (player mutes strings)
    const int silenceSamples = static_cast<int> (sampleRate * 0.15); // 150 ms silence
    juce::AudioBuffer<float> silenceBuf (2, silenceSamples);
    silenceBuf.clear();

    for (int pos = 0; pos < silenceSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, silenceSamples - pos);
        juce::AudioBuffer<float> blockBuf (silenceBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        gate.process (blockBuf);
    }

    float silenceReduction = gate.getCurrentReductionDb();
    TEST_ASSERT (silenceReduction <= -80.0f, "Gate closes tightly down to <= -80 dB after mute");

    return true;
}

//==============================================================================
// Test 2: TubeScreamer — Gain Staging, 4V Calibration & Level Attenuation
//==============================================================================
bool testTubeScreamerGainStaging()
{
    std::cout << "\n--- Running TubeScreamer Gain Staging & Diode Model Test ---\n";

    const double sampleRate = 48000.0;
    const int blockSize = 256;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };

    TubeScreamer screamer;
    screamer.prepare (spec);

    // 1. Test Drive 0 + Level 1.0 (Level Max)
    // Real TS808 pedal: Drive 0 + Level Max is a clean/mid boost with bounded output (NO wall of distortion)
    screamer.setDrive (0.0f);
    screamer.setTone (0.5f);
    screamer.setLevel (1.0f);

    const int numSamples = static_cast<int> (sampleRate * 0.1);
    juce::AudioBuffer<float> buffer (2, numSamples);

    // Typical guitar DI pickup peak ~ 0.25 V peak = 0.25 / 4.0 = 0.0625 FS (-24 dBFS)
    for (int i = 0; i < numSamples; ++i)
    {
        const float val = 0.0625f * std::sin (2.0f * juce::MathConstants<float>::pi * 500.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        buffer.setSample (0, i, val);
        buffer.setSample (1, i, val);
    }

    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> blockBuf (buffer.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (blockBuf);
    }

    float maxPeak = buffer.getMagnitude (0, numSamples);
    TEST_ASSERT (maxPeak < 1.0f, "Drive 0 + Level 1.0 output is bounded and does not clip digital 0 dBFS");
    TEST_ASSERT (maxPeak > 0.0625f, "Drive 0 + Level 1.0 provides healthy boost for amp frontend");

    // 2. Test Level knob attenuation (must strictly attenuate <= 1.0 and reach silence at 0.0)
    screamer.setLevel (0.0f);
    juce::AudioBuffer<float> muteBuf (2, numSamples);
    muteBuf.copyFrom (0, 0, buffer, 0, 0, numSamples);
    muteBuf.copyFrom (1, 0, buffer, 1, 0, numSamples);

    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> blockBuf (muteBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (blockBuf);
    }

    // After the 20 ms parameter smoother finishes ramping down, output must be completely silent
    float mutePeak = muteBuf.getMagnitude (0, numSamples / 2, numSamples / 2);
    TEST_ASSERT (mutePeak < 0.0001f, "Level at 0.0 is completely silent (passive potentiometer attenuation)");

    // 3. Test hot input diode solver convergence (+6 dBFS = 8.0 V input)
    screamer.setLevel (1.0f);
    screamer.setDrive (1.0f);
    juce::AudioBuffer<float> hotBuf (2, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        hotBuf.setSample (0, i, 2.0f); // +6 dBFS
        hotBuf.setSample (1, i, 2.0f);
    }

    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> blockBuf (hotBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (blockBuf);
    }

    float hotOut = hotBuf.getSample (0, numSamples - 1);
    TEST_ASSERT (!std::isnan (hotOut) && !std::isinf (hotOut), "Diode solver converges cleanly without NaN/Inf on extreme inputs");

    // 4. Test Overdrive Saturation Range: Drive 1.0 generates rich overdrive & sustain
    screamer.setLevel (0.7f);
    screamer.setTone (0.5f);
    screamer.setDrive (0.0f);
    juce::AudioBuffer<float> odCleanBuf (2, numSamples);
    juce::AudioBuffer<float> odDirtyBuf (2, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        const float s = 0.2f * std::sin (2.0f * juce::MathConstants<float>::pi * 1000.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        odCleanBuf.setSample (0, i, s);
        odCleanBuf.setSample (1, i, s);
        odDirtyBuf.setSample (0, i, s);
        odDirtyBuf.setSample (1, i, s);
    }
    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> b (odCleanBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (b);
    }
    float cleanRms = odCleanBuf.getRMSLevel (0, numSamples / 2, numSamples / 2);

    screamer.setDrive (1.0f);
    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> b (odDirtyBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (b);
    }
    float dirtyRms = odDirtyBuf.getRMSLevel (0, numSamples / 2, numSamples / 2);
    TEST_ASSERT (dirtyRms > cleanRms * 1.5f, "Drive 1.0 provides rich saturated overdrive sustain over Drive 0.0");

    // 5. Test Tone Control Dramatic Range: Tone 0.0 vs Tone 1.0 at 3500 Hz has >= 15 dB range
    screamer.setDrive (0.5f);
    screamer.setLevel (0.7f);
    screamer.setTone (0.0f);
    juce::AudioBuffer<float> toneDarkBuf (2, numSamples);
    juce::AudioBuffer<float> toneBrightBuf (2, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        const float s = 0.15f * std::sin (2.0f * juce::MathConstants<float>::pi * 3500.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        toneDarkBuf.setSample (0, i, s);
        toneDarkBuf.setSample (1, i, s);
        toneBrightBuf.setSample (0, i, s);
        toneBrightBuf.setSample (1, i, s);
    }
    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> b (toneDarkBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (b);
    }
    float darkRms = toneDarkBuf.getRMSLevel (0, numSamples / 2, numSamples / 2);

    screamer.setTone (1.0f);
    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, numSamples - pos);
        juce::AudioBuffer<float> b (toneBrightBuf.getArrayOfWritePointers(), 2, pos, curBlock);
        screamer.process (b);
    }
    float brightRms = toneBrightBuf.getRMSLevel (0, numSamples / 2, numSamples / 2);
    float toneDiffDb = juce::Decibels::gainToDecibels (brightRms / std::max (1e-5f, darkRms));
    TEST_ASSERT (toneDiffDb >= 15.0f, "Tone control provides >= 15 dB dramatic range between dark (0.0) and bright (1.0)");

    return true;
}

//==============================================================================
// Test 3: Chugger v3 — Flat Magnitude at Chug 0 & Palm-Mute Dominance Tightening
//==============================================================================
bool testChuggerPhaseCoherenceAndDominance()
{
    std::cout << "\n--- Running Chugger v3 Crossover & Palm-Mute Tests ---\n";

    const double sampleRate = 48000.0;
    const int blockSize = 256;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };

    Chugger chugger;
    chugger.prepare (spec);

    // 1. Transparency test at Chug = 0: Low + Mid + High must sum to flat magnitude (allpass)
    chugger.setChug (0.0f);

    std::vector<float> testFrequencies { 80.0f, 150.0f, 500.0f, 1800.0f, 4000.0f, 10000.0f };
    const int testLength = static_cast<int> (sampleRate * 0.1); // 100 ms per freq

    for (float freq : testFrequencies)
    {
        juce::AudioBuffer<float> sineBuf (2, testLength);
        for (int i = 0; i < testLength; ++i)
        {
            const float s = std::sin (2.0f * juce::MathConstants<float>::pi * freq * static_cast<float> (i) / static_cast<float> (sampleRate));
            sineBuf.setSample (0, i, s);
            sineBuf.setSample (1, i, s);
        }

        const float inRms = sineBuf.getRMSLevel (0, testLength / 2, testLength / 2);

        for (int pos = 0; pos < testLength; pos += blockSize)
        {
            const int curBlock = std::min (blockSize, testLength - pos);
            juce::AudioBuffer<float> blockBuf (sineBuf.getArrayOfWritePointers(), 2, pos, curBlock);
            chugger.process (blockBuf);
        }

        const float outRms = sineBuf.getRMSLevel (0, testLength / 2, testLength / 2);
        const float diffDb = std::abs (juce::Decibels::gainToDecibels (outRms / inRms));

        TEST_ASSERT (diffDb < 0.15f, juce::String ("Chug 0 magnitude is transparent at " + juce::String (freq) + " Hz (deviation < 0.15 dB)").toRawUTF8());
    }

    // 2. Palm-Mute Tightening Test:
    // Low-heavy signal (90 Hz fundamental dominating) triggers dynamic reduction when Chug = 1.0
    chugger.setChug (1.0f);
    chugger.reset();

    const int chugTestSamples = static_cast<int> (sampleRate * 0.2); // 200 ms
    juce::AudioBuffer<float> lowHeavy (2, chugTestSamples);
    for (int i = 0; i < chugTestSamples; ++i)
    {
        // 90 Hz strong fundamental at 0.5 amplitude
        const float s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 90.0f * static_cast<float> (i) / static_cast<float> (sampleRate));
        lowHeavy.setSample (0, i, s);
        lowHeavy.setSample (1, i, s);
    }

    for (int pos = 0; pos < chugTestSamples; pos += blockSize)
    {
        const int curBlock = std::min (blockSize, chugTestSamples - pos);
        juce::AudioBuffer<float> blockBuf (lowHeavy.getArrayOfWritePointers(), 2, pos, curBlock);
        chugger.process (blockBuf);
    }

    // Verify low-band is clamped during decay (palm-mute tightening active)
    const float lateRms = lowHeavy.getRMSLevel (0, chugTestSamples - 2000, 2000);
    const float inputRms = 0.5f * 0.7071f;
    const float reductionDb = juce::Decibels::gainToDecibels (lateRms / inputRms);
    TEST_ASSERT (reductionDb < -4.0f, "Chugger dynamically clamps low-frequency bloom during palm-mute resonance");

    return true;
}

//==============================================================================
// Test 4: PluginProcessor — 2-In / 2-Out Standard Audio Bus Routing
//==============================================================================
bool testPluginProcessorBuses()
{
    std::cout << "\n--- Running PluginProcessor Bus Routing & Integration Tests ---\n";

    ChugKnifeAudioProcessor processor;

    // Verify bus setup
    TEST_ASSERT (processor.getBusCount (true) == 1, "PluginProcessor exposes exactly 1 input bus");
    TEST_ASSERT (processor.getBusCount (false) == 1, "PluginProcessor exposes exactly 1 output bus");

    // Verify stereo layout support
    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add (juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
    TEST_ASSERT (processor.isBusesLayoutSupported (stereoLayout), "Stereo In / Stereo Out layout is supported");

    // Verify mono layout support
    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
    monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
    TEST_ASSERT (processor.isBusesLayoutSupported (monoLayout), "Mono In / Mono Out layout is supported");

    // Verify mismatched layout is rejected
    juce::AudioProcessor::BusesLayout mismatchedLayout;
    mismatchedLayout.inputBuses.add (juce::AudioChannelSet::stereo());
    mismatchedLayout.outputBuses.add (juce::AudioChannelSet::mono());
    TEST_ASSERT (!processor.isBusesLayoutSupported (mismatchedLayout), "Mismatched Stereo In / Mono Out layout is rejected");

    // Test audio processing execution
    processor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> testBlock (2, 512);
    testBlock.clear();
    juce::MidiBuffer midi;
    processor.processBlock (testBlock, midi);
    TEST_ASSERT (true, "PluginProcessor processBlock completes without crash");

    processor.releaseResources();
    return true;
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    std::cout << "=========================================================\n";
    std::cout << "   ChugKnife Automated DSP & Routing Test Suite (v1.1)   \n";
    std::cout << "=========================================================\n";

    bool allPassed = true;
    allPassed &= testNoiseGateLowFrequencyRipple();
    allPassed &= testTubeScreamerGainStaging();
    allPassed &= testChuggerPhaseCoherenceAndDominance();
    allPassed &= testPluginProcessorBuses();

    std::cout << "\n=========================================================\n";
    std::cout << " Test Summary: " << passedTests << " / " << totalTests << " assertions passed.\n";
    if (allPassed && passedTests == totalTests)
    {
        std::cout << " RESULT: ALL TESTS PASSED SUCCESSFULLY! (Exit code 0)\n";
        std::cout << "=========================================================\n";
        return 0;
    }
    else
    {
        std::cerr << " RESULT: TEST SUITE FAILED!\n";
        std::cout << "=========================================================\n";
        return 1;
    }
}
