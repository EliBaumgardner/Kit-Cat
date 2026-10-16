#include <catch2/catch_test_macros.hpp>

#include "Audio/AudioUIBridge.h"

#include <thread>

struct SequencedCommand
{
    int sequence = 0;
    int check    = 0;
};

TEST_CASE("CommandFifo hands commands across threads in order, untorn, and flags what it drops", "[bridge]")
{
    constexpr int commandCount = 200000;

    CommandFifo<SequencedCommand, 64> fifo;
    std::atomic<bool>                 producing { true };
    int                               received     = 0;
    int                               lastSequence = -1;
    int                               outOfOrder   = 0;
    int                               torn         = 0;

    std::thread producer([&fifo, &producing] {
        for (int sequence = 0; sequence < commandCount; ++sequence) {
            fifo.push({ sequence, sequence * 3 + 1 });
        }

        producing.store(false);
    });

    const auto receive = [&](const SequencedCommand& command) {
        if (command.sequence <= lastSequence) {
            outOfOrder += 1;
        }

        if (command.check != command.sequence * 3 + 1) {
            torn += 1;
        }

        lastSequence  = command.sequence;
        received     += 1;
    };

    while (producing.load() || fifo.hasPending()) {
        fifo.drain(receive);
    }

    producer.join();

    fifo.drain(receive);

    CHECK(outOfOrder == 0);
    CHECK(torn == 0);
    CHECK(received > 0);
    CHECK((received == commandCount) != fifo.overflowed.load());
}
