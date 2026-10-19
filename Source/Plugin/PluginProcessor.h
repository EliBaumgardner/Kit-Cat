#pragma once

#include "Audio/AudioUIBridge.h"
#include "Audio/CorpusPublisher.h"
#include "Audio/SlicePlayer.h"
#include "Audio/TargetAnalyser.h"
#include "Audio/TargetMatcher.h"
#include "Plugin/CorpusLoader.h"
#include "Plugin/StateIdentifiers.h"

#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

class CatKitAudioProcessor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    CatKitAudioProcessor();

    void prepareToPlay       (double sampleRate, int samplesPerBlock) override;
    void releaseResources    () override;
    void processBlock        (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept [[clang::nonblocking]] override;
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

    juce::UndoManager   undoManager;
    juce::ValueTree     settings { StateIdentifiers::CatKitState };
    juce::ValueTree     colourPresets { ArsNovaUI::InterfaceIdentifiers::ColourPresets };
    CorpusPublisher     corpusPublisher;
    SlicePlayer         slicePlayer;
    CorpusLoader        corpusLoader;
    AudioUIBridge       audioUIBridge;

    std::atomic<bool>   liveMatchingEnabled { false };
    std::atomic<float>  inputGateThresholdDb { -40.0f };
    std::atomic<double> pitchWeight { 0.5 };
    std::atomic<int>    candidateCount { 4 };
    std::atomic<double> repetitionPenalty { 0.5 };

    static constexpr int inputRingBufferSize = 4096;
    static constexpr int targetWindowSize    = 1024;
    static constexpr int targetHopSize       = 256;

private:
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    TargetAnalyser                         targetAnalyser;
    TargetMatcher                          targetMatcher;
    std::array<float, inputRingBufferSize> inputRingBuffer {};
    std::array<float, targetWindowSize>    inputWindow {};
    int                                    inputWritePos = 0;
    int                                    hopCounter    = 0;
};
