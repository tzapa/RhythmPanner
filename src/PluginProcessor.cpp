#include "PluginProcessor.h"
#include "PluginEditor.h"

const char* RhythmPannerProcessor::divisionNames[numDivisions] = {
    "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T"
};

// Values in quarter notes (1 quarter note = 1 beat)
const float RhythmPannerProcessor::divisionValues[numDivisions] = {
    4.0f,   // 1/1
    2.0f,   // 1/2
    1.0f,   // 1/4
    0.5f,   // 1/8
    0.25f,  // 1/16
    0.125f, // 1/32
    0.6667f,// 1/4T (triplet)
    0.3333f // 1/8T (triplet)
};

juce::AudioProcessorValueTreeState::ParameterLayout RhythmPannerProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Division (tempo sync step)
    juce::StringArray divChoices;
    for (int i = 0; i < numDivisions; ++i)
        divChoices.add(divisionNames[i]);
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "division", "Division", divChoices, 2)); // default 1/4

    // Start position: 0=Left, 1=Center, 2=Right
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "startPos", "Start Position",
        juce::StringArray{"Left", "Center", "Right"}, 0));

    // Smoothing: 0=instant, 1=very smooth (glide fraction 0..1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "smoothing", "Smoothing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));

    return layout;
}

RhythmPannerProcessor::RhythmPannerProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

RhythmPannerProcessor::~RhythmPannerProcessor() {}

void RhythmPannerProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    phaseAccumulator = 0.0;
    smoothedPan = 0.0f;
}

void RhythmPannerProcessor::releaseResources() {}

void RhythmPannerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto* playHead = getPlayHead();
    double bpm = 120.0;
    bool isPlaying = false;
    double ppqPosition = 0.0;

    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
            if (pos->getIsPlaying())
                isPlaying = true;
            if (pos->getPpqPosition().hasValue())
                ppqPosition = *pos->getPpqPosition();
        }
    }

    // If not playing, keep running freely
    if (!isPlaying)
        isPlaying = true;

    int divIndex = (int)*apvts.getRawParameterValue("division");
    float divBeats = divisionValues[divIndex]; // duration of one step in quarter notes

    int startPos = (int)*apvts.getRawParameterValue("startPos");
    float smoothing = *apvts.getRawParameterValue("smoothing");

    // Smoothing: convert 0..1 to a lag coefficient
    // smoothing=0 -> coeff near 1 (instant), smoothing=1 -> coeff near 0 (very slow)
    // Use: smoothCoeff = exp(-1 / (smoothingTime * sampleRate))
    // smoothingTime in seconds: 0..0.3s
    float smoothTimeSec = smoothing * 0.25f;
    float lagCoeff = (smoothTimeSec < 0.0001f) ? 0.0f :
        (float)std::exp(-1.0 / (smoothTimeSec * currentSampleRate));

    // Cycle: 4 steps of divBeats each
    // Pattern positions (pan values):
    // startPos=Left  (0): L(-1), C(0), R(1), C(0)
    // startPos=Center(1): C(0), R(1), C(0), L(-1)
    // startPos=Right (2): R(1), C(0), L(-1), C(0)

    float stepPans[3][4] = {
        { -1.0f,  0.0f,  1.0f,  0.0f }, // Left start
        {  0.0f,  1.0f,  0.0f, -1.0f }, // Center start
        {  1.0f,  0.0f, -1.0f,  0.0f }, // Right start
    };

    double samplesPerBeat = currentSampleRate * 60.0 / bpm;
    double samplesPerStep = samplesPerBeat * divBeats;
    double samplesPerCycle = samplesPerStep * 4.0;

    // Sync phase to host position
    double hostPhase = std::fmod(ppqPosition / divBeats, 4.0) / 4.0;
    if (isPlaying)
        phaseAccumulator = hostPhase;

    int numSamples = buffer.getNumSamples();
    auto* leftIn  = buffer.getReadPointer(0);
    auto* rightIn = buffer.getReadPointer(1 < buffer.getNumChannels() ? 1 : 0);
    auto* leftOut  = buffer.getWritePointer(0);
    auto* rightOut = buffer.getWritePointer(1 < buffer.getNumChannels() ? 1 : 0);

    for (int i = 0; i < numSamples; ++i)
    {
        // Which of the 4 steps are we in?
        double cyclePhase = std::fmod(phaseAccumulator * 4.0, 4.0);
        int stepIdx = (int)cyclePhase;
        if (stepIdx > 3) stepIdx = 3;

        float targetPan = stepPans[startPos][stepIdx];

        // Apply smoothing
        smoothedPan = lagCoeff * smoothedPan + (1.0f - lagCoeff) * targetPan;

        // Convert pan to L/R gains (constant power)
        float pan01 = (smoothedPan + 1.0f) * 0.5f; // 0..1
        float leftGain  = (float)std::cos(pan01 * juce::MathConstants<float>::halfPi);
        float rightGain = (float)std::sin(pan01 * juce::MathConstants<float>::halfPi);

        // Mix input to mono, then pan
        float mono = (leftIn[i] + rightIn[i]) * 0.5f;
        leftOut[i]  = mono * leftGain  * 2.0f;
        rightOut[i] = mono * rightGain * 2.0f;

        // Advance phase
        phaseAccumulator += 1.0 / samplesPerCycle;
        if (phaseAccumulator >= 1.0)
            phaseAccumulator -= 1.0;
    }
}

void RhythmPannerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RhythmPannerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* RhythmPannerProcessor::createEditor()
{
    return new RhythmPannerEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RhythmPannerProcessor();
}
