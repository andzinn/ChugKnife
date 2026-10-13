#include "PluginEditor.h"

//==============================================================================
// ChugKnifeLookAndFeel Implementation
//==============================================================================

ChugKnifeLookAndFeel::ChugKnifeLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0e6ed));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff111215));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000)); // borderless
}

void ChugKnifeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto accentColour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // 1. Background Arc (Subtle Graphite)
    const float arcRadius = radius - 3.5f;
    juce::Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff20242c));
    g.strokePath (bgArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Active Value Arc
    const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    if (toAngle > rotaryStartAngle + 0.005f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

        // Soft Neon Glow behind arc
        g.setColour (accentColour.withAlpha (0.22f));
        g.strokePath (activeArc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Crisp Vector Core
        g.setColour (accentColour);
        g.strokePath (activeArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Inner Metallic Dial Body
    const float dialRadius = radius - 10.0f;
    if (dialRadius > 6.0f)
    {
        // Drop shadow
        g.setColour (juce::Colour (0x55000000));
        g.fillEllipse (centre.x - dialRadius + 1.0f, centre.y - dialRadius + 2.0f, dialRadius * 2.0f, dialRadius * 2.0f);

        // Outer Bevel Rim
        g.setColour (juce::Colour (0xff2d323c));
        g.drawEllipse (centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.2f);

        // Radial Metallic Gradient Body
        juce::ColourGradient dialGrad (juce::Colour (0xff262a33), centre.x - dialRadius * 0.35f, centre.y - dialRadius * 0.35f,
                                       juce::Colour (0xff131518), centre.x + dialRadius * 0.45f, centre.y + dialRadius * 0.45f, true);
        g.setGradientFill (dialGrad);
        g.fillEllipse (centre.x - dialRadius + 0.5f, centre.y - dialRadius + 0.5f, (dialRadius - 0.5f) * 2.0f, (dialRadius - 0.5f) * 2.0f);

        // 4. Concentric Dynamic Gain Reduction Ring (Exclusively on 4CM Gate Threshold)
        if (slider.getName() == "gate_threshold")
        {
            const float innerRadius = dialRadius - 4.5f;
            if (innerRadius > 6.0f)
            {
                // Inactive inner track
                juce::Path innerBg;
                innerBg.addCentredArc (centre.x, centre.y, innerRadius, innerRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
                g.setColour (juce::Colour (0x22ffffff));
                g.strokePath (innerBg, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                // Active Gain Reduction Arc
                if (currentGrDb < -0.3f)
                {
                    const float grNorm = juce::jlimit (0.0f, 1.0f, -currentGrDb / 40.0f);
                    const float grAngle = rotaryStartAngle + grNorm * (rotaryEndAngle - rotaryStartAngle);
                    juce::Path grArc;
                    grArc.addCentredArc (centre.x, centre.y, innerRadius, innerRadius, 0.0f, rotaryStartAngle, grAngle, true);

                    g.setColour (juce::Colour (0xffff3d00)); // Neon Fire Red/Orange
                    g.strokePath (grArc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            }
        }

        // 5. Crisp Radial Pointer Indicator
        juce::Line<float> pointerLine (centre.getPointOnCircumference (dialRadius * 0.35f, toAngle),
                                       centre.getPointOnCircumference (dialRadius - 1.5f, toAngle));
        g.setColour (accentColour.brighter (0.4f));
        g.drawLine (pointerLine, 2.2f);
    }
}

void ChugKnifeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                             const juce::Slider::SliderStyle /*style*/, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height));
    auto trackWidth = 8.0f;
    auto trackX = bounds.getCentreX() - trackWidth * 0.5f;
    auto trackRect = juce::Rectangle<float> (trackX, bounds.getY() + 4.0f, trackWidth, bounds.getHeight() - 8.0f);

    // Dark track background
    g.setColour (juce::Colour (0xff1a1d22));
    g.fillRoundedRectangle (trackRect, 4.0f);
    g.setColour (juce::Colour (0xff2c303a));
    g.drawRoundedRectangle (trackRect, 4.0f, 1.0f);

    // Active fill from bottom to slider position
    auto fillRect = juce::Rectangle<float> (trackX + 1.0f, sliderPos, trackWidth - 2.0f, trackRect.getBottom() - sliderPos);
    if (fillRect.getHeight() > 0.0f)
    {
        auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (accent);
        g.fillRoundedRectangle (fillRect, 3.0f);
    }

    // Modern Metallic Thumb Handle
    auto thumbWidth = 28.0f;
    auto thumbHeight = 12.0f;
    auto thumbRect = juce::Rectangle<float> (bounds.getCentreX() - thumbWidth * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight);

    g.setColour (juce::Colour (0x66000000));
    g.fillRoundedRectangle (thumbRect.translated (0.0f, 1.5f), 3.0f);

    juce::ColourGradient thumbGrad (juce::Colour (0xff323744), thumbRect.getX(), thumbRect.getY(),
                                    juce::Colour (0xff1c1f26), thumbRect.getX(), thumbRect.getBottom(), false);
    g.setGradientFill (thumbGrad);
    g.fillRoundedRectangle (thumbRect, 3.0f);

    g.setColour (juce::Colour (0xff484f5e));
    g.drawRoundedRectangle (thumbRect, 3.0f, 1.0f);

    // Thumb center accent line
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.fillRect (thumbRect.getCentreX() - 4.0f, thumbRect.getCentreY() - 1.0f, 8.0f, 2.0f);
}

void ChugKnifeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f, 3.0f);
    const bool isBypassed = button.getToggleState(); // true = bypassed, false = active

    // Button body
    g.setColour (juce::Colour (0xff181a20));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (isBypassed ? juce::Colour (0xff262a33) : juce::Colour (0xff383e4c));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Micro-LED indicator circle on the left
    const float ledRadius = 3.5f;
    const float ledX = bounds.getX() + 10.0f;
    const float ledY = bounds.getCentreY();

    if (!isBypassed)
    {
        // ACTIVE -> LED is brightly lit with module accent color
        auto accent = button.findColour (juce::TextButton::buttonColourId);
        g.setColour (accent.withAlpha (0.4f));
        g.fillEllipse (ledX - ledRadius - 2.0f, ledY - ledRadius - 2.0f, (ledRadius + 2.0f) * 2.0f, (ledRadius + 2.0f) * 2.0f);

        g.setColour (accent);
        g.fillEllipse (ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);

        g.setColour (juce::Colour (0xffe0e6ed));
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText ("ACTIVE", bounds.withTrimmedLeft (20.0f), juce::Justification::centredLeft, false);
    }
    else
    {
        // BYPASSED -> LED is dark graphite
        g.setColour (juce::Colour (0xff2a2e36));
        g.fillEllipse (ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);

        g.setColour (juce::Colour (0xff707785));
        g.setFont (juce::Font (11.0f, juce::Font::plain));
        g.drawText ("BYPASS", bounds.withTrimmedLeft (20.0f), juce::Justification::centredLeft, false);
    }
}

//==============================================================================
// ChugKnifeAudioProcessorEditor Implementation
//==============================================================================

ChugKnifeAudioProcessorEditor::ChugKnifeAudioProcessorEditor (ChugKnifeAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (920, 420);

    auto& apvts = processorRef.getAPVTS();

    // Setup utility helper for labels
    auto configureHeader = [] (juce::Label& title, juce::Label& sub, const juce::String& titleText, const juce::String& subText, juce::Colour accent)
    {
        title.setText (titleText, juce::dontSendNotification);
        title.setFont (juce::Font (13.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, accent);
        title.setJustificationType (juce::Justification::centredLeft);

        sub.setText (subText, juce::dontSendNotification);
        sub.setFont (juce::Font (9.5f, juce::Font::plain));
        sub.setColour (juce::Label::textColourId, juce::Colour (0xff828999));
        sub.setJustificationType (juce::Justification::centredLeft);
    };

    // -------------------------------------------------------------------------
    // 1. TRANSPOSE MODULE (Electric Cyan #00E5FF)
    // -------------------------------------------------------------------------
    const juce::Colour cyanColour (0xff00e5ff);
    configureHeader (transposeTitleLabel, transposeSubLabel, "TRANSPOSE", "Tone3000 Engine (±24 st)", cyanColour);
    addAndMakeVisible (transposeTitleLabel);
    addAndMakeVisible (transposeSubLabel);

    transposeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    transposeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    transposeSlider.setColour (juce::Slider::rotarySliderFillColourId, cyanColour);
    transposeSlider.setName ("transpose");
    addAndMakeVisible (transposeSlider);

    transposeValueLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    transposeValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    transposeValueLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (transposeValueLabel);

    transposeBypassButton.setColour (juce::TextButton::buttonColourId, cyanColour);
    addAndMakeVisible (transposeBypassButton);

    transposeAttach = std::make_unique<SliderAttachment> (apvts, "transpose_semi", transposeSlider);
    transposeBypassAttach = std::make_unique<ButtonAttachment> (apvts, "transpose_bypass", transposeBypassButton);

    // -------------------------------------------------------------------------
    // 2. SCREAMER MODULE (Acid Emerald #00E676)
    // -------------------------------------------------------------------------
    const juce::Colour emeraldColour (0xff00e676);
    configureHeader (tsTitleLabel, tsSubLabel, "SCREAMER", "TS808 Analog Model", emeraldColour);
    addAndMakeVisible (tsTitleLabel);
    addAndMakeVisible (tsSubLabel);

    for (auto* s : { &tsDriveSlider, &tsToneSlider, &tsLevelSlider })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        s->setColour (juce::Slider::rotarySliderFillColourId, emeraldColour);
        addAndMakeVisible (s);
    }

    tsDriveNameLabel.setText ("DRIVE", juce::dontSendNotification);
    tsToneNameLabel.setText  ("TONE",  juce::dontSendNotification);
    tsLevelNameLabel.setText ("LEVEL", juce::dontSendNotification);
    for (auto* l : { &tsDriveNameLabel, &tsToneNameLabel, &tsLevelNameLabel })
    {
        l->setFont (juce::Font (10.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, juce::Colour (0xff9aa1b0));
        l->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    }

    tsBypassButton.setColour (juce::TextButton::buttonColourId, emeraldColour);
    addAndMakeVisible (tsBypassButton);

    tsDriveAttach  = std::make_unique<SliderAttachment> (apvts, "ts_drive",  tsDriveSlider);
    tsToneAttach   = std::make_unique<SliderAttachment> (apvts, "ts_tone",   tsToneSlider);
    tsLevelAttach  = std::make_unique<SliderAttachment> (apvts, "ts_level",  tsLevelSlider);
    tsBypassAttach = std::make_unique<ButtonAttachment> (apvts, "ts_bypass", tsBypassButton);

    // -------------------------------------------------------------------------
    // 3. THE CHUGGER MODULE (High-Gain Crimson #FF1744)
    // -------------------------------------------------------------------------
    const juce::Colour crimsonColour (0xffff1744);
    configureHeader (chugTitleLabel, chugSubLabel, "THE CHUGGER", "Tight & Attack Shaper", crimsonColour);
    addAndMakeVisible (chugTitleLabel);
    addAndMakeVisible (chugSubLabel);

    chugSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    chugSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 20);
    chugSlider.setColour (juce::Slider::rotarySliderFillColourId, crimsonColour);
    addAndMakeVisible (chugSlider);

    chugValueLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    chugValueLabel.setColour (juce::Label::textColourId, crimsonColour.brighter (0.3f));
    chugValueLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (chugValueLabel);

    chugDescLabel.setText ("15-500Hz HPF + 2.2kHz Clank", juce::dontSendNotification);
    chugDescLabel.setFont (juce::Font (9.0f, juce::Font::plain));
    chugDescLabel.setColour (juce::Label::textColourId, juce::Colour (0xff707785));
    chugDescLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (chugDescLabel);

    chugBypassButton.setColour (juce::TextButton::buttonColourId, crimsonColour);
    addAndMakeVisible (chugBypassButton);

    chugAttach = std::make_unique<SliderAttachment> (apvts, "chug", chugSlider);
    chugBypassAttach = std::make_unique<ButtonAttachment> (apvts, "chug_bypass", chugBypassButton);

    // -------------------------------------------------------------------------
    // 4. 4CM NOISE GATE MODULE (Amber/Gold #FFB300)
    // -------------------------------------------------------------------------
    const juce::Colour amberColour (0xffffab00);
    configureHeader (gateTitleLabel, gateSubLabel, "4CM GATE", "DI-Keyed Noise Gate", amberColour);
    addAndMakeVisible (gateTitleLabel);
    addAndMakeVisible (gateSubLabel);

    gateThresholdSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gateThresholdSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 20);
    gateThresholdSlider.setColour (juce::Slider::rotarySliderFillColourId, amberColour);
    gateThresholdSlider.setName ("gate_threshold");
    addAndMakeVisible (gateThresholdSlider);

    gateStatusLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    gateStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff828999));
    gateStatusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gateStatusLabel);

    gateKeyLabel.setText ("Key: Clean DI (Pre-Gain)", juce::dontSendNotification);
    gateKeyLabel.setFont (juce::Font (9.0f, juce::Font::plain));
    gateKeyLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff));
    gateKeyLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gateKeyLabel);

    gateBypassButton.setColour (juce::TextButton::buttonColourId, amberColour);
    addAndMakeVisible (gateBypassButton);

    gateThreshAttach = std::make_unique<SliderAttachment> (apvts, "gate_threshold", gateThresholdSlider);
    gateBypassAttach = std::make_unique<ButtonAttachment> (apvts, "gate_bypass", gateBypassButton);

    // -------------------------------------------------------------------------
    // 5. MASTER MODULE (Ice Silver #E0E6ED)
    // -------------------------------------------------------------------------
    const juce::Colour silverColour (0xffe0e6ed);
    configureHeader (masterTitleLabel, masterSubLabel, "MASTER", "Output Level", silverColour);
    addAndMakeVisible (masterTitleLabel);
    addAndMakeVisible (masterSubLabel);

    masterSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    masterSlider.setColour (juce::Slider::rotarySliderFillColourId, silverColour);
    addAndMakeVisible (masterSlider);

    masterAttach = std::make_unique<SliderAttachment> (apvts, "master_level", masterSlider);

    startTimerHz (30); // 30 fps smooth meter animations
}

ChugKnifeAudioProcessorEditor::~ChugKnifeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChugKnifeAudioProcessorEditor::timerCallback()
{
    // 1. Update Real-time Noise Gate Gain Reduction
    const float redDb = processorRef.getGateReductionDb();
    lookAndFeel.setGateReductionDb (redDb);
    gateThresholdSlider.repaint();

    if (redDb < -0.5f)
    {
        gateStatusLabel.setText ("GR: " + juce::String (redDb, 1) + " dB", juce::dontSendNotification);
        gateStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff3d00)); // Luminous Fire Red
    }
    else
    {
        gateStatusLabel.setText ("GATE OPEN", juce::dontSendNotification);
        gateStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff555c6b));
    }

    // 2. Update Chug Tight Cutoff Readout
    const float chugVal = static_cast<float> (chugSlider.getValue());
    const int tightHz = static_cast<int> (10.0f + std::pow (chugVal, 1.35f) * 490.0f);
    chugValueLabel.setText ("TIGHT: " + juce::String (tightHz) + " Hz", juce::dontSendNotification);

    // 3. Update Transpose Readout
    const int semi = static_cast<int> (transposeSlider.getValue());
    transposeValueLabel.setText ((semi > 0 ? "+" : "") + juce::String (semi) + " st", juce::dontSendNotification);
}

void ChugKnifeAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Deep Carbon / Obsidian Background
    g.fillAll (juce::Colour (0xff111215));

    // Top Header Banner
    const int headerHeight = 48;
    g.setColour (juce::Colour (0xff14161a));
    g.fillRect (0, 0, getWidth(), headerHeight);

    g.setColour (juce::Colour (0xff20242c));
    g.drawHorizontalLine (headerHeight, 0.0f, static_cast<float> (getWidth()));

    // Title & Subtitle Branding
    g.setColour (juce::Colour (0xffe0e6ed));
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawText ("CHUGKNIFE", 18, 12, 140, 24, juce::Justification::left);

    g.setFont (juce::Font (10.5f, juce::Font::plain));
    g.setColour (juce::Colour (0xff767d8d));
    g.drawText ("// 4CM PRE-AMP WEAPON", 158, 15, 200, 20, juce::Justification::left);

    // Routing Status Pill (Header Right)
    auto statusPill = juce::Rectangle<float> (getWidth() - 310.0f, 12.0f, 292.0f, 24.0f);
    g.setColour (juce::Colour (0xff1a1d22));
    g.fillRoundedRectangle (statusPill, 4.0f);
    g.setColour (juce::Colour (0xff262b35));
    g.drawRoundedRectangle (statusPill, 4.0f, 1.0f);

    // Glowing Cyan Dot
    g.setColour (juce::Colour (0xff00e5ff));
    g.fillEllipse (statusPill.getX() + 10.0f, statusPill.getCentreY() - 3.5f, 7.0f, 7.0f);

    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffc4cbd6));
    g.drawText ("INTERNAL 4CM: PRE-GAIN DI DETECTOR", statusPill.withTrimmedLeft (24.0f), juce::Justification::centredLeft, false);

    // -------------------------------------------------------------------------
    // Draw 5 Modular Cards
    // -------------------------------------------------------------------------
    struct CardInfo { int x; int w; juce::Colour accent; };
    const int cardY = 56;
    const int cardH = 350;

    std::array<CardInfo, 5> cards = {{
        { 14,  165, juce::Colour (0xff00e5ff) }, // Transpose (Cyan)
        { 184, 235, juce::Colour (0xff00e676) }, // Screamer (Emerald)
        { 424, 165, juce::Colour (0xffff1744) }, // Chugger (Crimson)
        { 594, 185, juce::Colour (0xffffab00) }, // 4CM Gate (Amber)
        { 784, 122, juce::Colour (0xffe0e6ed) }  // Master (Silver)
    }};

    for (const auto& c : cards)
    {
        auto cardRect = juce::Rectangle<float> (static_cast<float> (c.x), static_cast<float> (cardY),
                                                static_cast<float> (c.w), static_cast<float> (cardH));
        // Dark panel fill
        g.setColour (juce::Colour (0xff16181d));
        g.fillRoundedRectangle (cardRect, 6.0f);

        // 1px Subtle Border
        g.setColour (juce::Colour (0xff232730));
        g.drawRoundedRectangle (cardRect, 6.0f, 1.0f);

        // Top Accent Stripe (1.5px)
        g.setColour (c.accent.withAlpha (0.8f));
        g.fillRect (cardRect.getX() + 10.0f, cardRect.getY(), cardRect.getWidth() - 20.0f, 2.0f);
    }
}

void ChugKnifeAudioProcessorEditor::resized()
{
    const int cardY = 56;

    // -------------------------------------------------------------------------
    // 1. TRANSPOSE CARD (x = 14, w = 165)
    // -------------------------------------------------------------------------
    transposeTitleLabel.setBounds (24, cardY + 12, 110, 18);
    transposeSubLabel.setBounds   (24, cardY + 30, 110, 14);
    transposeBypassButton.setBounds (24, cardY + 54, 78, 24);

    transposeSlider.setBounds (36, cardY + 100, 120, 120);
    transposeValueLabel.setBounds (24, cardY + 230, 145, 26);

    // -------------------------------------------------------------------------
    // 2. SCREAMER CARD (x = 184, w = 235)
    // -------------------------------------------------------------------------
    tsTitleLabel.setBounds (194, cardY + 12, 110, 18);
    tsSubLabel.setBounds   (194, cardY + 30, 130, 14);
    tsBypassButton.setBounds (194, cardY + 54, 78, 24);

    // Top Knobs (Drive & Tone)
    tsDriveSlider.setBounds (194, cardY + 86, 100, 95);
    tsDriveNameLabel.setBounds (194, cardY + 182, 100, 16);

    tsToneSlider.setBounds (309, cardY + 86, 100, 95);
    tsToneNameLabel.setBounds (309, cardY + 182, 100, 16);

    // Bottom Knob (Level)
    tsLevelSlider.setBounds (251, cardY + 205, 100, 95);
    tsLevelNameLabel.setBounds (251, cardY + 301, 100, 16);

    // -------------------------------------------------------------------------
    // 3. THE CHUGGER CARD (x = 424, w = 165)
    // -------------------------------------------------------------------------
    chugTitleLabel.setBounds (434, cardY + 12, 110, 18);
    chugSubLabel.setBounds   (434, cardY + 30, 130, 14);
    chugBypassButton.setBounds (434, cardY + 54, 78, 24);

    chugSlider.setBounds (444, cardY + 95, 125, 125);
    chugValueLabel.setBounds (434, cardY + 230, 145, 22);
    chugDescLabel.setBounds (428, cardY + 256, 157, 18);

    // -------------------------------------------------------------------------
    // 4. 4CM NOISE GATE CARD (x = 594, w = 185)
    // -------------------------------------------------------------------------
    gateTitleLabel.setBounds (604, cardY + 12, 110, 18);
    gateSubLabel.setBounds   (604, cardY + 30, 140, 14);
    gateBypassButton.setBounds (604, cardY + 54, 78, 24);

    gateThresholdSlider.setBounds (626, cardY + 95, 120, 120);
    gateStatusLabel.setBounds (604, cardY + 230, 165, 22);
    gateKeyLabel.setBounds (604, cardY + 256, 165, 18);

    // -------------------------------------------------------------------------
    // 5. MASTER CARD (x = 784, w = 122)
    // -------------------------------------------------------------------------
    masterTitleLabel.setBounds (794, cardY + 12, 90, 18);
    masterSubLabel.setBounds   (794, cardY + 30, 90, 14);

    masterSlider.setBounds (825, cardY + 65, 40, 240);
}
