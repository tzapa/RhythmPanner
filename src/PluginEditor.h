#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class RhythmPannerEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit RhythmPannerEditor(RhythmPannerProcessor&);
    ~RhythmPannerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RhythmPannerProcessor& processorRef;

    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttach;

    juce::ComboBox startPosBox;
    juce::Label startPosLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> startPosAttach;

    juce::Slider smoothingSlider;
    juce::Label smoothingLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> smoothingAttach;

    // Pan indicator
    float displayPan = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmPannerEditor)
};
