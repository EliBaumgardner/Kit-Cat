#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <array>
#include <atomic>
#include <concepts>

/*
    The one-way channel from the audio thread to the map, which is how live matching is shown on
    screen. It holds two CommandFifos of plain structs: puckFifo carries LivePucks (the map
    position of the slice the input matched, the input's level, and whether the input was above
    the gate) and flashFifo carries SliceFlashes (the index of a slice that just started as a
    grain).

    A CommandFifo is a queue of fixed size with one thread writing and one reading. push copies a
    command in, or drops it and sets overflowed when the queue is full, so the writer never waits.
    drain hands every waiting command, oldest first, to the function it is given, and hasPending
    says whether any are waiting.

    KitCatAudioProcessor::processBlock calls pushFlash and pushPuck for each grain it starts, and
    pushPuck with active false when the input is below the gate. The editor's 60 Hz timerCallback
    drains both queues into CorpusMap on the message thread. With no editor open the 64-entry
    queues fill and drop what follows; the audio thread never locks, allocates or posts a message.
*/

template <typename Command, int Capacity = 256>
class CommandFifo
{
public:
    void push (const Command& command)
    {
        const juce::AbstractFifo::ScopedWrite scope = fifo.write(1);

        if (scope.blockSize1 > 0) {
            buffer[static_cast<size_t>(scope.startIndex1)] = command;
        } else {
            overflowed.store(true, std::memory_order_relaxed);
        }
    }

    template <std::invocable<const Command&> ApplyCommand>
    void drain (ApplyCommand&& apply)
    {
        const juce::AbstractFifo::ScopedRead scope = fifo.read(fifo.getNumReady());

        for (int index = 0; index < scope.blockSize1; ++index) {
            apply(buffer[static_cast<size_t>(scope.startIndex1 + index)]);
        }

        for (int index = 0; index < scope.blockSize2; ++index) {
            apply(buffer[static_cast<size_t>(scope.startIndex2 + index)]);
        }
    }

    bool hasPending () const
    {
        return fifo.getNumReady() > 0;
    }

    std::atomic<bool> overflowed { false };

private:
    juce::AbstractFifo fifo { Capacity };

    std::array<Command, Capacity> buffer {};
};

class AudioUIBridge
{
public:
    struct LivePuck
    {
        juce::Point<float> position;

        float rms = 0.0f;

        bool active = false;
    };

    struct SliceFlash
    {
        int sliceIndex = -1;
    };

    void pushPuck  (juce::Point<float> position, float rms, bool active);
    void pushFlash (int sliceIndex);

    CommandFifo<LivePuck, 64>   puckFifo;
    CommandFifo<SliceFlash, 64> flashFifo;
};
