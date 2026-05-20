#pragma once
#include <JuceHeader.h>

class RhythmPannerProcessor : public juce::AudioProcessor
{
public:
    RhythmPannerProcessor();
    ~RhythmPannerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RhythmPanner"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getSmoothedPan() const { return smoothedPan; }

    // Tempo sync divisions: index -> beat fraction
    static constexpr int numDivisions = 8;
    static const char* divisionNames[numDivisions];
    static const float divisionValues[numDivisions]; // in beats (quarter notes)

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double currentSampleRate = 44100.0;
    double phaseAccumulator = 0.0; // 0..1 within one full cycle (L->C->R->C or similar)
    float smoothedPan = 0.0f;      // current output pan value

    // Cycle has 4 stages: [startPos -> center -> endPos -> center] or variations
    // We use a 3-step pattern: hold left, slide to center, hold right, slide to center (repeat)
    // The pattern depends on "start side"

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmPannerProcessor)
};
