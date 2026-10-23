#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "TestCorpus.h"

#include <cmath>

TEST_CASE("processBlock stays realtime-safe while matching live input, browsing, retuning and republishing", "[realtime]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;
    constexpr int    blockCount = 600;

    KitCatAudioProcessor     processor;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    double                   phase   = 0.0;
    float                    loudest = 0.0f;

    processor.prepareToPlay(sampleRate, blockSize);
    processor.corpusPublisher.publish(std::make_shared<CorpusAnalysis::Result>(makeTestCorpus(300, 5)));
    processor.settings.setProperty(StateIdentifiers::LiveMatchingEnabled, true, nullptr);

    for (int block = 0; block < blockCount; ++block) {
        const double frequency = 110.0 * std::pow(2.0, static_cast<double>(block % 48) / 12.0);
        const bool   silent    = block % 40 >= 34;

        if (block % 100 == 50) {
            processor.settings.setProperty(StateIdentifiers::PitchWeight, static_cast<double>(block % 300) / 300.0, nullptr);
            processor.settings.setProperty(StateIdentifiers::CandidateCount, 1 + block % 7, nullptr);
            processor.settings.setProperty(StateIdentifiers::RepetitionPenalty, static_cast<double>(block % 200) / 200.0, nullptr);
        }

        if (block == blockCount / 2) {
            processor.corpusPublisher.publish(std::make_shared<CorpusAnalysis::Result>(makeTestCorpus(400, 9)));
        }

        if (block % 25 == 0) {
            processor.slicePlayer.push({ SlicePlayer::CommandKind::Play, block % 300 });
        }

        if (block % 90 == 0) {
            processor.slicePlayer.push({ SlicePlayer::CommandKind::FadeOutAll, -1 });
        }

        for (int sample = 0; sample < blockSize; ++sample) {
            float value = 0.0f;

            if (! silent) {
                value = static_cast<float>(0.4 * std::sin(phase));
            }

            phase += 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;

            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value);
        }

        processor.processBlock(buffer, midi);

        loudest = std::max(loudest, buffer.getMagnitude(0, 0, blockSize));
    }

    CHECK(loudest > 0.0f);
}
