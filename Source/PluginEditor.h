#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * @brief Custom LookAndFeel for ChugKnife Dark Minimalist UI.
 * Inspired by Neural DSP Archetype and FabFilter vector aesthetics.
 */
class ChugKnifeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChugKnifeLookAndFeel();
    ~ChugKnifeLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void setGateReductionDb (float grDb) { currentGrDb = grDb; }

private:
    float currentGrDb = 0.0f;
};

//==============================================================================
/**
 * @brief Main AudioProcessorEditor for ChugKnife VST3 / Standalone.
 */
class ChugKnifeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit ChugKnifeAudioProcessorEditor (ChugKnifeAudioProcessor&);
    ~ChugKnifeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ChugKnifeAudioProcessor& processorRef;
    ChugKnifeLookAndFeel lookAndFeel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // --- MODULE 1: TRANSPOSE ---
    juce::Slider transposeSlider;
    juce::Label  transposeTitleLabel;
    juce::Label  transposeSubLabel;
    juce::Label  transposeValueLabel;
    juce::ToggleButton transposeBypassButton { "ACTIVE" };
    std::unique_ptr<SliderAttachment> transposeAttach;
    std::unique_ptr<ButtonAttachment> transposeBypassAttach;

    // --- MODULE 2: SCREAMER (TS808) ---
    juce::Slider tsDriveSlider, tsToneSlider, tsLevelSlider;
    juce::Label  tsTitleLabel;
    juce::Label  tsSubLabel;
    juce::Label  tsDriveNameLabel, tsToneNameLabel, tsLevelNameLabel;
    juce::ToggleButton tsBypassButton { "ACTIVE" };
    std::unique_ptr<SliderAttachment> tsDriveAttach;
    std::unique_ptr<SliderAttachment> tsToneAttach;
    std::unique_ptr<SliderAttachment> tsLevelAttach;
    std::unique_ptr<ButtonAttachment> tsBypassAttach;

    // --- MODULE 3: THE CHUGGER ---
    juce::Slider chugSlider;
    juce::Label  chugTitleLabel;
    juce::Label  chugSubLabel;
    juce::Label  chugValueLabel;
    juce::Label  chugDescLabel;
    juce::ToggleButton chugBypassButton { "ACTIVE" };
    std::unique_ptr<SliderAttachment> chugAttach;
    std::unique_ptr<ButtonAttachment> chugBypassAttach;

    // --- MODULE 4: 4CM NOISE GATE ---
    juce::Slider gateThresholdSlider;
    juce::Label  gateTitleLabel;
    juce::Label  gateSubLabel;
    juce::Label  gateStatusLabel;
    juce::Label  gateKeyLabel;
    juce::ToggleButton gateBypassButton { "ACTIVE" };
    std::unique_ptr<SliderAttachment> gateThreshAttach;
    std::unique_ptr<ButtonAttachment> gateBypassAttach;

    // --- MODULE 5: MASTER OUTPUT ---
    juce::Slider masterSlider;
    juce::Label  masterTitleLabel;
    juce::Label  masterSubLabel;
    juce::Label  masterValueLabel;
    std::unique_ptr<SliderAttachment> masterAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChugKnifeAudioProcessorEditor)
};
