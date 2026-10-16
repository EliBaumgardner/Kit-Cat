#pragma once

#include "Analysis/CorpusAnalysis.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>

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
        CommandKind kind       = CommandKind::Play;
        int         sliceIndex = -1;
    };

    std::atomic<double> fadeSeconds { defaultFadeMilliseconds / 1000.0 };
    std::atomic<double> lengthSeconds { 0.0 };

    static constexpr int    slotCount               = 24;
    static constexpr int    commandCapacity         = 64;
    static constexpr double defaultFadeMilliseconds = 10.0;
    static constexpr int    hannTableSize           = 1024;

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
        SlotState state          = SlotState::Idle;
        SlotKind  kind           = SlotKind::Slice;
        int       fileIndex      = 0;
        double    position       = 0.0;
        double    endPosition    = 0.0;
        double    elapsedSamples = 0.0;
        double    grainLength    = 0.0;
        float     fadeGain       = 1.0f;
        float     targetGain     = 1.0f;
        int       startDelay     = 0;
    };

    juce::AbstractFifo                   commandFifo { commandCapacity };
    std::array<Command, commandCapacity> commands;
    std::array<Slot, slotCount>          slots;
    std::array<float, hannTableSize + 1> hannTable {};
    std::uint64_t                        playingGeneration = 0;
    double                               hostSampleRate    = 44100.0;
    double                               fadeSamples       = 441.0;
};
