#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <array>
#include <atomic>
#include <concepts>

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
