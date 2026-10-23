#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "TestCorpus.h"

#include <cmath>

struct DrumLoopFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("KitCatDrumLoop", "", false);

    ~DrumLoopFolder()
    {
        folder.deleteRecursively();
    }
};

static std::vector<float> makeDrumLoop(double sampleRate, int hitCount, int hitSpacing)
{
    std::vector<float> samples(static_cast<size_t>(hitCount * hitSpacing), 0.0f);
    juce::Random       random(29);
    float              previousNoise = 0.0f;

    for (int hit = 0; hit < hitCount; ++hit) {
        const int   kind     = hit % 3;
        const float variance = 1.0f + 0.15f * static_cast<float>(hit / 3);

        for (int sample = 0; sample < hitSpacing; ++sample) {
            const double time  = static_cast<double>(sample) / sampleRate;
            const float  noise = random.nextFloat() * 2.0f - 1.0f;
            float        value = 0.0f;

            if (kind == 0) {
                const double sweep = 50.0 * variance + 100.0 * std::exp(-time * 30.0);

                value = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * sweep * time) * std::exp(-time * 20.0));
            }

            if (kind == 1) {
                value = static_cast<float>((0.6 * noise + 0.4 * std::sin(2.0 * juce::MathConstants<double>::pi * 190.0 * variance * time)) * std::exp(-time * 30.0));
            }

            if (kind == 2) {
                value = static_cast<float>((noise - previousNoise) * 0.5 * std::exp(-time * 80.0 / variance));
            }

            previousNoise = noise;

            samples[static_cast<size_t>(hit * hitSpacing + sample)] = 0.8f * value;
        }
    }

    return samples;
}

TEST_CASE("a drum loop analysed as a corpus answers the same drums with grains", "[live]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int    blockSize  = 512;
    constexpr int    hitCount   = 18;
    constexpr int    hitSpacing = 13230;

    DrumLoopFolder           temporary;
    const juce::File         audioFile = temporary.folder.getChildFile("drums.wav");
    const std::vector<float> drums     = makeDrumLoop(sampleRate, hitCount, hitSpacing);
    CorpusAnalysis           analysis;
    KitCatAudioProcessor     processor;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    int                      soundingSamples = 0;
    int                      invalidSamples  = 0;

    REQUIRE(temporary.folder.createDirectory());

    juce::AudioBuffer<float>                 audio(1, static_cast<int>(drums.size()));
    juce::WavAudioFormat                     wav;
    std::unique_ptr<juce::OutputStream>      stream = std::make_unique<juce::FileOutputStream>(audioFile);
    std::unique_ptr<juce::AudioFormatWriter> writer = wav.createWriterFor(stream, juce::AudioFormatWriterOptions().withSampleRate(sampleRate).withNumChannels(1).withBitsPerSample(32));

    audio.copyFrom(0, 0, drums.data(), audio.getNumSamples());

    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()));

    writer.reset();

    analysis.layout.seed = 7;

    CorpusAnalysis::Result corpus = analysis.analyse(audioFile);

    INFO(CorpusAnalysis::describe(corpus));
    REQUIRE(corpus.error.isEmpty());
    REQUIRE(corpus.slices.size() >= static_cast<size_t>(CorpusAnalysis::minimumSliceCount));
    REQUIRE(! corpus.kdTree.empty());

    processor.prepareToPlay(sampleRate, blockSize);
    processor.corpusPublisher.publish(std::make_shared<CorpusAnalysis::Result>(std::move(corpus)));
    processor.settings.setProperty(StateIdentifiers::LiveMatchingEnabled, true, nullptr);

    for (size_t start = 0; start + blockSize <= drums.size(); start += blockSize) {
        buffer.copyFrom(0, 0, drums.data() + start, blockSize);
        buffer.copyFrom(1, 0, drums.data() + start, blockSize);

        processor.processBlock(buffer, midi);

        for (int sample = 0; sample < blockSize; ++sample) {
            const float value = buffer.getSample(0, sample);

            if (! std::isfinite(value)) {
                ++invalidSamples;
            }

            if (std::abs(value) > 1.0e-4f) {
                ++soundingSamples;
            }
        }
    }

    CHECK(invalidSamples == 0);
    CHECK(soundingSamples > static_cast<int>(drums.size()) / 10);
}

TEST_CASE("a grain starts no earlier than the input that triggered it and within one hop of it", "[live]")
{
    constexpr double sampleRate   = 44100.0;
    constexpr int    blockSize    = 512;
    constexpr int    silentBlocks = 8;
    constexpr int    onsetOffset  = 300;
    constexpr int    onset        = silentBlocks * blockSize + onsetOffset;

    KitCatAudioProcessor     processor;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    int                      firstSound = -1;

    processor.prepareToPlay(sampleRate, blockSize);
    processor.corpusPublisher.publish(std::make_shared<CorpusAnalysis::Result>(makeTestCorpus(200, 3)));
    processor.settings.setProperty(StateIdentifiers::LiveMatchingEnabled, true, nullptr);

    for (int block = 0; block < silentBlocks + 4 && firstSound < 0; ++block) {
        for (int sample = 0; sample < blockSize; ++sample) {
            const int time  = block * blockSize + sample;
            float     value = 0.0f;

            if (time >= onset) {
                value = static_cast<float>(0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * static_cast<double>(time - onset) / sampleRate));
            }

            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value);
        }

        processor.processBlock(buffer, midi);

        for (int sample = 0; sample < blockSize && firstSound < 0; ++sample) {
            if (buffer.getSample(0, sample) != 0.0f) {
                firstSound = block * blockSize + sample;
            }
        }
    }

    REQUIRE(firstSound >= 0);

    const int latency = firstSound - onset;

    CHECK(latency >= 0);
    CHECK(latency <= KitCatAudioProcessor::targetHopSize + 1);
}
