#include "PluginEditor.h"

RhythmPannerEditor::RhythmPannerEditor(RhythmPannerProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(360, 260);

    // Division
    divisionLabel.setText("Tempo Sync", juce::dontSendNotification);
    divisionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    divisionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(divisionLabel);

    for (int i = 0; i < RhythmPannerProcessor::numDivisions; ++i)
        divisionBox.addItem(RhythmPannerProcessor::divisionNames[i], i + 1);
    divisionBox.setSelectedId(3); // 1/4 default
    divisionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, "division", divisionBox);
    addAndMakeVisible(divisionBox);

    // Start position
    startPosLabel.setText("Start Position", juce::dontSendNotification);
    startPosLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    startPosLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(startPosLabel);

    startPosBox.addItem("Left", 1);
    startPosBox.addItem("Center", 2);
    startPosBox.addItem("Right", 3);
    startPosBox.setSelectedId(1);
    startPosAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, "startPos", startPosBox);
    addAndMakeVisible(startPosBox);

    // Smoothing
    smoothingLabel.setText("Smoothing", juce::dontSendNotification);
    smoothingLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(smoothingLabel);

    smoothingSlider.setRange(0.0, 1.0, 0.01);
    smoothingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    smoothingSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00AAFF));
    smoothingSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF444444));
    smoothingSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFFFFFF));
    smoothingAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "smoothing", smoothingSlider);
    addAndMakeVisible(smoothingSlider);

    startTimer(30); // ~30fps refresh for pan indicator
}

RhythmPannerEditor::~RhythmPannerEditor()
{
    stopTimer();
}

void RhythmPannerEditor::timerCallback()
{
    // Read current pan from processor's smoothedPan (we expose it via a public accessor)
    // For now just trigger repaint so indicator can update
    repaint();
}

void RhythmPannerEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Title
    g.setColour(juce::Colour(0xFF00AAFF));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("RHYTHM PANNER", 0, 10, getWidth(), 28, juce::Justification::centred);

    // Pan indicator bar
    int barY = 55;
    int barH = 18;
    int barX = 20;
    int barW = getWidth() - 40;

    // Background track
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle((float)barX, (float)barY, (float)barW, (float)barH, 6.0f);

    // Center line
    g.setColour(juce::Colour(0xFF444444));
    int cx = barX + barW / 2;
    g.drawLine((float)cx, (float)barY, (float)cx, (float)(barY + barH), 1.0f);

    // Pan position dot
    // Get pan from processor
    float pan = processorRef.getSmoothedPan();
    float panX = barX + (pan + 1.0f) * 0.5f * barW;
    g.setColour(juce::Colour(0xFF00AAFF));
    g.fillEllipse(panX - 8.0f, (float)(barY + barH / 2 - 8), 16.0f, 16.0f);

    // L / R labels
    g.setColour(juce::Colour(0xFF888888));
    g.setFont(juce::Font(11.0f));
    g.drawText("L", barX - 16, barY, 14, barH, juce::Justification::centred);
    g.drawText("R", barX + barW + 2, barY, 14, barH, juce::Justification::centred);
}

void RhythmPannerEditor::resized()
{
    int margin = 20;
    int labelH = 20;
    int controlH = 28;
    int col1X = margin;
    int col2X = getWidth() / 2 + 10;
    int colW = getWidth() / 2 - margin - 10;
    int startY = 90;

    // Row 1: Division | Start Position
    divisionLabel.setBounds(col1X, startY, colW, labelH);
    divisionBox.setBounds(col1X, startY + labelH + 4, colW, controlH);

    startPosLabel.setBounds(col2X, startY, colW, labelH);
    startPosBox.setBounds(col2X, startY + labelH + 4, colW, controlH);

    // Row 2: Smoothing (centered)
    int knobSize = 80;
    int knobX = (getWidth() - knobSize) / 2;
    int knobY = startY + labelH + controlH + 24;
    smoothingLabel.setBounds(knobX - 10, knobY - labelH - 4, knobSize + 20, labelH);
    smoothingSlider.setBounds(knobX, knobY, knobSize, knobSize + 20);
}
