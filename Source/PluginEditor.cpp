#include "PluginEditor.h"

ChugKnifeAudioProcessorEditor::ChugKnifeAudioProcessorEditor (ChugKnifeAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Configure LookAndFeel or Colors
    setSize (780, 520);

    auto& apvts = processorRef.getAPVTS();

    // 1. TRANSPOSE GROUP
    transposeGroup.setText ("1. TRANSPOSE (DROP TUNER)");
    addAndMakeVisible (transposeGroup);

    transposeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    transposeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    transposeSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff3498db));
    addAndMakeVisible (transposeSlider);

    transposeLabel.setText ("Semitones (-12 to +12)", juce::dontSendNotification);
    transposeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (transposeLabel);

    addAndMakeVisible (transposeBypassButton);

    transposeAttach = std::make_unique<SliderAttachment> (apvts, "transpose_semi", transposeSlider);
    transposeBypassAttach = std::make_unique<ButtonAttachment> (apvts, "transpose_bypass", transposeBypassButton);

    // 2. SCREAMER GROUP
    screamerGroup.setText ("2. SCREAMER (OVERDRIVE)");
    addAndMakeVisible (screamerGroup);

    for (auto* s : { &tsDriveSlider, &tsToneSlider, &tsLevelSlider })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff2ecc71));
        addAndMakeVisible (s);
    }

    tsDriveLabel.setText ("Drive", juce::dontSendNotification);
    tsToneLabel.setText  ("Tone",  juce::dontSendNotification);
    tsLevelLabel.setText ("Level", juce::dontSendNotification);
    for (auto* l : { &tsDriveLabel, &tsToneLabel, &tsLevelLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    }

    addAndMakeVisible (tsBypassButton);

    tsDriveAttach  = std::make_unique<SliderAttachment> (apvts, "ts_drive",  tsDriveSlider);
    tsToneAttach   = std::make_unique<SliderAttachment> (apvts, "ts_tone",   tsToneSlider);
    tsLevelAttach  = std::make_unique<SliderAttachment> (apvts, "ts_level",  tsLevelSlider);
    tsBypassAttach = std::make_unique<ButtonAttachment> (apvts, "ts_bypass", tsBypassButton);

    // 3. THE CHUGGER GROUP
    chuggerGroup.setText ("3. THE CHUGGER (MONSTER KNOB)");
    addAndMakeVisible (chuggerGroup);

    chugSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    chugSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 24);
    chugSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe74c3c));
    addAndMakeVisible (chugSlider);

    chugLabel.setText ("Chug (Weight & Attack)", juce::dontSendNotification);
    chugLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (chugLabel);

    addAndMakeVisible (chugBypassButton);

    chugAttach = std::make_unique<SliderAttachment> (apvts, "chug", chugSlider);
    chugBypassAttach = std::make_unique<ButtonAttachment> (apvts, "chug_bypass", chugBypassButton);

    // 4. 4CM NOISE GATE GROUP
    gateGroup.setText ("4. NOISE GATE (4-CABLE METHOD)");
    addAndMakeVisible (gateGroup);

    gateThresholdSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gateThresholdSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 75, 20);
    gateThresholdSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xfff39c12));
    addAndMakeVisible (gateThresholdSlider);

    gateThresholdLabel.setText ("Threshold", juce::dontSendNotification);
    gateThresholdLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gateThresholdLabel);

    gateStatusLabel.setText ("Routing: Internal 4CM (Pre-Drive DI)", juce::dontSendNotification);
    gateStatusLabel.setJustificationType (juce::Justification::centred);
    gateStatusLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible (gateStatusLabel);

    addAndMakeVisible (gateBypassButton);

    gateThreshAttach = std::make_unique<SliderAttachment> (apvts, "gate_threshold", gateThresholdSlider);
    gateBypassAttach = std::make_unique<ButtonAttachment> (apvts, "gate_bypass", gateBypassButton);

    // 5. MASTER GROUP
    masterGroup.setText ("5. MASTER");
    addAndMakeVisible (masterGroup);

    masterSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (masterSlider);

    masterLabel.setText ("Out (dB)", juce::dontSendNotification);
    masterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (masterLabel);

    masterAttach = std::make_unique<SliderAttachment> (apvts, "master_level", masterSlider);

    startTimerHz (25); // Refresh 25 times per second
}

ChugKnifeAudioProcessorEditor::~ChugKnifeAudioProcessorEditor()
{
    stopTimer();
}

void ChugKnifeAudioProcessorEditor::timerCallback()
{
    const bool retActive = processorRef.isReturnConnected();
    const float redDb    = processorRef.getGateReductionDb();

    if (retActive)
    {
        gateStatusLabel.setText ("4CM Mode: Send/Return Loop Active! (GR: " + juce::String (redDb, 1) + " dB)",
                                 juce::dontSendNotification);
        gateStatusLabel.setColour (juce::Label::textColourId, juce::Colours::lime);
    }
    else
    {
        gateStatusLabel.setText ("4CM Mode: Normalled / Single Track (GR: " + juce::String (redDb, 1) + " dB)",
                                 juce::dontSendNotification);
        gateStatusLabel.setColour (juce::Label::textColourId, juce::Colours::deepskyblue);
    }
}

void ChugKnifeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff191b1f));

    // Title banner
    g.setColour (juce::Colour (0xff252830));
    g.fillRect (0, 0, getWidth(), 50);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("CHUGKNIFE", 20, 10, 200, 30, juce::Justification::left);

    g.setFont (juce::Font (14.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xffa0a5b5));
    g.drawText ("The Definitive 4CM Pre/Post Heavy Guitar Weapon (Tone3000 Pitch + Djent Gate + Screamer + The Chugger)",
                180, 12, 580, 26, juce::Justification::left);
}

void ChugKnifeAudioProcessorEditor::resized()
{
    const int topMargin = 60;
    const int rowHeight = 210;
    const int pad = 12;

    // Row 1: Transpose (left) and Screamer (right)
    const int colW1 = 230;
    const int colW2 = 380;
    const int colW3 = getWidth() - (colW1 + colW2 + pad * 4);

    // 1. Transpose Box
    transposeGroup.setBounds (pad, topMargin, colW1, rowHeight);
    transposeSlider.setBounds (pad + 40, topMargin + 30, 150, 120);
    transposeLabel.setBounds (pad + 15, topMargin + 155, 200, 20);
    transposeBypassButton.setBounds (pad + 55, topMargin + 175, 120, 24);

    // 2. Screamer Box
    screamerGroup.setBounds (pad * 2 + colW1, topMargin, colW2, rowHeight);
    const int knobW = 100;
    const int startX = pad * 2 + colW1 + 25;
    tsDriveSlider.setBounds (startX, topMargin + 30, knobW, 110);
    tsDriveLabel.setBounds  (startX, topMargin + 145, knobW, 20);

    tsToneSlider.setBounds  (startX + 110, topMargin + 30, knobW, 110);
    tsToneLabel.setBounds   (startX + 110, topMargin + 145, knobW, 20);

    tsLevelSlider.setBounds (startX + 220, topMargin + 30, knobW, 110);
    tsLevelLabel.setBounds  (startX + 220, topMargin + 145, knobW, 20);

    tsBypassButton.setBounds (startX + 105, topMargin + 175, 130, 24);

    // 5. Master Box (Top Right)
    masterGroup.setBounds (pad * 3 + colW1 + colW2, topMargin, colW3, rowHeight);
    masterSlider.setBounds (pad * 3 + colW1 + colW2 + 25, topMargin + 30, 60, 130);
    masterLabel.setBounds  (pad * 3 + colW1 + colW2 + 10, topMargin + 165, 90, 20);

    // Row 2: Chugger (left) and 4CM Gate (right)
    const int row2Top = topMargin + rowHeight + pad;
    const int row2Height = 220;

    const int chugW = 280;
    const int gateW = getWidth() - (chugW + pad * 3);

    // 3. The Chugger Box
    chuggerGroup.setBounds (pad, row2Top, chugW, row2Height);
    chugSlider.setBounds (pad + 65, row2Top + 30, 150, 130);
    chugLabel.setBounds (pad + 30, row2Top + 165, 220, 20);
    chugBypassButton.setBounds (pad + 75, row2Top + 185, 130, 24);

    // 4. Noise Gate Box
    gateGroup.setBounds (pad * 2 + chugW, row2Top, gateW, row2Height);
    gateThresholdSlider.setBounds (pad * 2 + chugW + 40, row2Top + 30, 140, 120);
    gateThresholdLabel.setBounds (pad * 2 + chugW + 30, row2Top + 155, 160, 20);
    gateBypassButton.setBounds (pad * 2 + chugW + 50, row2Top + 180, 120, 24);

    gateStatusLabel.setBounds (pad * 2 + chugW + 200, row2Top + 80, 250, 40);
}
