#include "Audio/SlicePlayer.h"

#include <algorithm>
#include <cmath>

void SlicePlayer::prepare(double sampleRate)
{
    hostSampleRate = sampleRate;

    slots.fill({});

    for (int index = 0; index <= hannTableSize; ++index) {
        hannTable[static_cast<size_t>(index)] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * index / hannTableSize)));
    }
}

void SlicePlayer::push(Command command)
{
    juce::AbstractFifo::ScopedWrite write = commandFifo.write(1);

    if (write.blockSize1 > 0) {
        commands[static_cast<size_t>(write.startIndex1)] = command;
    }
}

void SlicePlayer::triggerGrain(int sliceIndex, float gain, const CorpusAnalysis::Result& corpus, int startSample)
{
    if (sliceIndex < 0 || sliceIndex >= static_cast<int>(corpus.slices.size())) {
        return;
    }

    const CorpusAnalysis::Slice& slice = corpus.slices[static_cast<size_t>(sliceIndex)];

    const double fileEnd       = static_cast<double>(corpus.fileSamples[static_cast<size_t>(slice.fileIndex)].size());
    const double lengthSamples = lengthSeconds.load() * corpus.sampleRate;
    double       endPosition   = static_cast<double>(slice.sound.endSample);

    size_t chosen = 0;

    if (lengthSamples > 0.0) {
        endPosition = std::min(static_cast<double>(slice.sound.startSample) + lengthSamples, fileEnd);
    }

    for (size_t index = 0; index < slots.size(); ++index) {
        if (slots[index].state == SlotState::Idle) {
            chosen = index;
            break;
        }

        if (slots[index].elapsedSamples > slots[chosen].elapsedSamples) {
            chosen = index;
        }
    }

    slots[chosen] = { .state       = SlotState::Playing,
                      .kind        = SlotKind::Grain,
                      .fileIndex   = slice.fileIndex,
                      .startDelay  = startSample,
                      .position    = static_cast<double>(slice.sound.startSample),
                      .endPosition = endPosition,
                      .grainLength = std::max(1.0, endPosition - static_cast<double>(slice.sound.startSample)),
                      .targetGain  = gain };
}

void SlicePlayer::render(juce::AudioBuffer<float>& buffer, const CorpusAnalysis::Result* corpus)
{
    std::uint64_t generation = 0;

    fadeSamples = fadeSeconds.load() * hostSampleRate;

    if (corpus != nullptr) {
        generation = corpus->generation;
    }

    if (generation != playingGeneration) {
        slots.fill({});
        playingGeneration = generation;
    }

    takeCommands(corpus);

    if (corpus == nullptr) {
        return;
    }

    for (Slot& slot : slots) {
        if (slot.state != SlotState::Idle) {
            renderSlot(slot, buffer, *corpus);
        }
    }
}

void SlicePlayer::takeCommands(const CorpusAnalysis::Result* corpus)
{
    juce::AbstractFifo::ScopedRead read = commandFifo.read(commandFifo.getNumReady());

    read.forEach([this, corpus](int index) {
        const Command& command = commands[static_cast<size_t>(index)];

        for (Slot& slot : slots) {
            if (slot.kind == SlotKind::Slice && slot.state == SlotState::Playing) {
                slot.state = SlotState::FadingOut;
            }
        }

        if (command.kind == CommandKind::Play && corpus != nullptr && command.sliceIndex >= 0 && command.sliceIndex < static_cast<int>(corpus->slices.size())) {
            startSlice(command.sliceIndex, *corpus);
        }
    });
}

void SlicePlayer::startSlice(int sliceIndex, const CorpusAnalysis::Result& corpus)
{
    const CorpusAnalysis::Slice& slice = corpus.slices[static_cast<size_t>(sliceIndex)];

    const double fileEnd       = static_cast<double>(corpus.fileSamples[static_cast<size_t>(slice.fileIndex)].size());
    const double lengthSamples = lengthSeconds.load() * corpus.sampleRate;
    double       endPosition   = static_cast<double>(slice.sound.endSample);

    size_t chosen = 0;

    if (lengthSamples > 0.0) {
        endPosition = std::min(static_cast<double>(slice.sound.startSample) + lengthSamples, fileEnd);
    }

    for (size_t index = 0; index < slots.size(); ++index) {
        if (slots[index].state == SlotState::Idle) {
            chosen = index;
            break;
        }

        if (slots[index].fadeGain < slots[chosen].fadeGain) {
            chosen = index;
        }
    }

    slots[chosen] = { .state       = SlotState::Playing,
                      .kind        = SlotKind::Slice,
                      .fileIndex   = slice.fileIndex,
                      .position    = static_cast<double>(slice.sound.startSample),
                      .endPosition = endPosition };
}

void SlicePlayer::renderSlot(Slot& slot, juce::AudioBuffer<float>& buffer, const CorpusAnalysis::Result& corpus) const
{
    const std::vector<float>& samples = corpus.fileSamples[static_cast<size_t>(slot.fileIndex)];

    const double speed = corpus.sampleRate / hostSampleRate;

    const float fadeStep = static_cast<float>(1.0 / fadeSamples);

    const size_t lastIndex = samples.size() - 1;

    const int firstSample  = slot.startDelay;
    const int channelCount = buffer.getNumChannels();

    float* const* channels = buffer.getArrayOfWritePointers();

    slot.startDelay = 0;

    for (int sample = firstSample; sample < buffer.getNumSamples() && slot.state != SlotState::Idle; ++sample) {
        const size_t index     = static_cast<size_t>(slot.position);
        const size_t nextIndex = std::min(index + 1, lastIndex);

        const float fraction     = static_cast<float>(slot.position - static_cast<double>(index));
        const float interpolated = samples[index] + (samples[nextIndex] - samples[index]) * fraction;
        float       value        = 0.0f;

        if (slot.kind == SlotKind::Grain) {
            const double tablePosition = std::clamp(slot.elapsedSamples / slot.grainLength, 0.0, 1.0) * hannTableSize;

            const size_t tableIndex = static_cast<size_t>(tablePosition);
            const size_t tableNext  = std::min(tableIndex + 1, static_cast<size_t>(hannTableSize));

            const float tableFraction = static_cast<float>(tablePosition - static_cast<double>(tableIndex));
            const float window        = hannTable[tableIndex] + (hannTable[tableNext] - hannTable[tableIndex]) * tableFraction;

            value = interpolated * window * slot.targetGain;
        } else {
            const float attack  = static_cast<float>(std::min(1.0, slot.elapsedSamples / fadeSamples));
            const float release = static_cast<float>(std::clamp((slot.endPosition - slot.position) / (fadeSamples * speed), 0.0, 1.0));

            value = interpolated * attack * release * slot.fadeGain;
        }

        for (int channel = 0; channel < channelCount; ++channel) {
            channels[channel][sample] += value;
        }

        slot.position       += speed;
        slot.elapsedSamples += 1.0;

        if (slot.kind == SlotKind::Slice && slot.state == SlotState::FadingOut) {
            slot.fadeGain -= fadeStep;
        }

        if (slot.position >= slot.endPosition || (slot.kind == SlotKind::Slice && slot.fadeGain <= 0.0f) || (slot.kind == SlotKind::Grain && slot.elapsedSamples >= slot.grainLength)) {
            slot.state = SlotState::Idle;
        }
    }
}
