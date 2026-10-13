#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class CatKitAudioProcessor : public juce::AudioProcessor
{
public:
    CatKitAudioProcessor();

    void prepareToPlay       (double sampleRate, int samplesPerBlock) override;
    void releaseResources    () override;
    void processBlock        (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void setCurrentProgram   (int index) override;
    void changeProgramName   (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    bool hasEditor              () const override;
    bool acceptsMidi            () const override;
    bool producesMidi           () const override;
    bool isMidiEffect           () const override;

    juce::AudioProcessorEditor* createEditor () override;

    const juce::String getName        () const override;
    const juce::String getProgramName (int index) override;

    double getTailLengthSeconds () const override;

    int getNumPrograms    () override;
    int getCurrentProgram () override;
};
