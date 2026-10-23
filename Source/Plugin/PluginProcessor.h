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

/*
    The plugin instance the host creates, owner of everything the sound depends on: the settings,
    the corpus and its analysis, the live engine and the bridge to the screen. It outlives any
    editor window.

    settings is one ValueTree of type KitCatState holding every setting and the corpus path, keyed
    by StateIdentifiers and edited through undoManager. The processor listens to it and copies each
    setting the audio thread reads into an atomic: liveMatchingEnabled, inputGateThresholdDb,
    pitchWeight, candidateCount, repetitionPenalty, and SlicePlayer's fadeSeconds and lengthSeconds.
    getStateInformation saves settings with colourPresets; setStateInformation restores both,
    clears the undo history and analyses the corpus again.

    processBlock runs live matching. With it on, the input is mixed to mono into a 4096-sample
    ring, and every targetHopSize samples TargetAnalyser reads the last targetWindowSize. Above
    the input gate, TargetMatcher picks a slice, SlicePlayer starts it as a grain at that sample
    with a level following the input's, and AudioUIBridge gets a flash and a puck. The buffer is
    then cleared, so the input is never heard, and SlicePlayer::render mixes in the grains and
    slices. CorpusPublisher::BlockScope holds the corpus for the whole block, and processBlock is
    declared nonblocking: it never allocates, locks or waits.
*/

class KitCatAudioProcessor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    KitCatAudioProcessor();

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

    juce::UndoManager undoManager;

    juce::ValueTree settings { StateIdentifiers::KitCatState };
    juce::ValueTree colourPresets { ArsNovaUI::InterfaceIdentifiers::ColourPresets };

    CorpusPublisher corpusPublisher;

    SlicePlayer slicePlayer;

    CorpusLoader corpusLoader;

    AudioUIBridge audioUIBridge;

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

    TargetAnalyser targetAnalyser;

    TargetMatcher targetMatcher;

    std::array<float, inputRingBufferSize> inputRingBuffer {};
    std::array<float, targetWindowSize>    inputWindow {};

    int inputWritePos = 0;
    int hopCounter    = 0;
};
