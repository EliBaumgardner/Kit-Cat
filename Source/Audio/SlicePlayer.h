#pragma once

#include "Analysis/CorpusAnalysis.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>

/*
    The player of corpus audio: 24 slots, each playing one slice's span of a corpus file into the
    output, either as a slice the map asked for or as a grain the live engine chose. A slice fades
    in and out over fadeSeconds; a grain is shaped by a Hann window over its whole length and
    played at the gain it was given. Every slot reads its file at the corpus's sample rate
    converted to the host's, and writes the same mono signal to each output channel. Both play to
    the slice's end, unless lengthSeconds is above zero, which sets the length from its start.

    prepare sets the host sample rate, empties the slots and fills the window table. push, on the
    message thread, queues a Command: Play fades out the slices playing and starts a new one, and
    FadeOutAll only fades them out; grains are never faded. triggerGrain, on the audio thread,
    starts a grain at a sample within the block, in a free slot or else the one playing longest.
    render takes the queued commands and mixes every playing slot into the buffer.

    The editor pushes Play from CorpusMap's onSlicePicked, CorpusLoader pushes FadeOutAll when a job
    starts, and KitCatAudioProcessor::processBlock triggers grains and calls render; the processor
    stores fadeSeconds and lengthSeconds from the settings. When the corpus's generation changes,
    render empties every slot first, so no slot reads a corpus that has gone. A command pushed
    while the 64-entry queue is full is dropped.
*/

class SlicePlayer
{
public:
    struct Command;

    void prepare      (double sampleRate);
    void push         (Command command);
    void triggerGrain (int sliceIndex, float gain, const CorpusAnalysis::Result& corpus, int startSample);
    void render       (juce::AudioBuffer<float>& buffer, const CorpusAnalysis::Result* corpus);

    enum class CommandKind
    {
        Play,
        FadeOutAll
    };

    struct Command
    {
        CommandKind kind = CommandKind::Play;

        int sliceIndex = -1;
    };

    std::atomic<double> fadeSeconds { defaultFadeMilliseconds / 1000.0 };
    std::atomic<double> lengthSeconds { 0.0 };

    static constexpr int slotCount       = 24;
    static constexpr int commandCapacity = 64;
    static constexpr int hannTableSize   = 1024;

    static constexpr double defaultFadeMilliseconds = 10.0;

private:
    struct Slot;

    void takeCommands (const CorpusAnalysis::Result* corpus);
    void startSlice   (int sliceIndex, const CorpusAnalysis::Result& corpus);
    void renderSlot   (Slot& slot, juce::AudioBuffer<float>& buffer, const CorpusAnalysis::Result& corpus) const;

    enum class SlotState
    {
        Idle,
        Playing,
        FadingOut
    };

    enum class SlotKind
    {
        Slice,
        Grain
    };

    struct Slot
    {
        SlotState state = SlotState::Idle;

        SlotKind kind = SlotKind::Slice;

        int fileIndex  = 0;
        int startDelay = 0;

        double position       = 0.0;
        double endPosition    = 0.0;
        double elapsedSamples = 0.0;
        double grainLength    = 0.0;

        float fadeGain   = 1.0f;
        float targetGain = 1.0f;
    };

    juce::AbstractFifo commandFifo { commandCapacity };

    std::array<Command, commandCapacity> commands;
    std::array<Slot, slotCount>          slots;
    std::array<float, hannTableSize + 1> hannTable {};

    std::uint64_t playingGeneration = 0;

    double hostSampleRate = 44100.0;
    double fadeSamples    = 441.0;
};
