#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

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

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // --- NOISE GATE ---
    juce::GroupComponent gateGroup;
    juce::Slider gateThresholdSlider;
    juce::Label  gateThresholdLabel;
    juce::ToggleButton gateBypassButton { "Bypass Gate" };
    juce::Label  gateStatusLabel;
    std::unique_ptr<SliderAttachment> gateThreshAttach;
    std::unique_ptr<ButtonAttachment> gateBypassAttach;

    // --- TRANSPOSE ---
    juce::GroupComponent transposeGroup;
    juce::Slider transposeSlider;
    juce::Label  transposeLabel;
    juce::ToggleButton transposeBypassButton { "Bypass Pitch" };
    std::unique_ptr<SliderAttachment> transposeAttach;
    std::unique_ptr<ButtonAttachment> transposeBypassAttach;

    // --- SCREAMER ---
    juce::GroupComponent screamerGroup;
    juce::Slider tsDriveSlider, tsToneSlider, tsLevelSlider;
    juce::Label  tsDriveLabel,  tsToneLabel,  tsLevelLabel;
    juce::ToggleButton tsBypassButton { "Bypass Screamer" };
    std::unique_ptr<SliderAttachment> tsDriveAttach;
    std::unique_ptr<SliderAttachment> tsToneAttach;
    std::unique_ptr<SliderAttachment> tsLevelAttach;
    std::unique_ptr<ButtonAttachment> tsBypassAttach;

    // --- THE CHUGGER ---
    juce::GroupComponent chuggerGroup;
    juce::Slider chugSlider;
    juce::Label  chugLabel;
    juce::ToggleButton chugBypassButton { "Bypass Chugger" };
    std::unique_ptr<SliderAttachment> chugAttach;
    std::unique_ptr<ButtonAttachment> chugBypassAttach;

    // --- MASTER ---
    juce::GroupComponent masterGroup;
    juce::Slider masterSlider;
    juce::Label  masterLabel;
    std::unique_ptr<SliderAttachment> masterAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChugKnifeAudioProcessorEditor)
};
