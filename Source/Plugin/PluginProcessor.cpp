#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"

CatKitAudioProcessor::CatKitAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void CatKitAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
}

void CatKitAudioProcessor::releaseResources()
{
}

void CatKitAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
}

void CatKitAudioProcessor::setCurrentProgram(int index)
{
}

void CatKitAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void CatKitAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void CatKitAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

bool CatKitAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const juce::AudioChannelSet output = layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

bool CatKitAudioProcessor::hasEditor() const
{
    return true;
}

bool CatKitAudioProcessor::acceptsMidi() const
{
    return true;
}

bool CatKitAudioProcessor::producesMidi() const
{
    return false;
}

bool CatKitAudioProcessor::isMidiEffect() const
{
    return false;
}

juce::AudioProcessorEditor* CatKitAudioProcessor::createEditor()
{
    return new CatKitAudioProcessorEditor(*this);
}

const juce::String CatKitAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

const juce::String CatKitAudioProcessor::getProgramName(int index)
{
    return {};
}

double CatKitAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CatKitAudioProcessor::getNumPrograms()
{
    return 1;
}

int CatKitAudioProcessor::getCurrentProgram()
{
    return 0;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CatKitAudioProcessor();
}
