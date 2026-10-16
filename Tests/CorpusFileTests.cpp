#include <catch2/catch_test_macros.hpp>

#include "TestCorpus.h"

struct TemporaryFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("CatKitCorpusFileTests", "", false);

    ~TemporaryFolder()
    {
        folder.deleteRecursively();
    }
};

TEST_CASE("a saved analysis loads again after its corpus folder has been moved", "[corpus]")
{
    TemporaryFolder        temporary;
    const juce::File       original  = temporary.folder.getChildFile("Recorded");
    const juce::File       moved     = temporary.folder.getChildFile("Elsewhere").getChildFile("Recorded");
    const juce::File       audioFile = original.getChildFile("take.wav");
    CorpusAnalysis::Result corpus    = makeTestCorpus(20, 13);
    const CorpusAnalysis   analysis;
    CorpusAnalysis::Result loaded;

    REQUIRE(original.createDirectory());
    REQUIRE(moved.getParentDirectory().createDirectory());

    juce::AudioBuffer<float>                 audio(1, static_cast<int>(corpus.fileSamples[0].size()));
    juce::WavAudioFormat                     wav;
    std::unique_ptr<juce::OutputStream>      stream = std::make_unique<juce::FileOutputStream>(audioFile);
    std::unique_ptr<juce::AudioFormatWriter> writer = wav.createWriterFor(stream, juce::AudioFormatWriterOptions().withSampleRate(corpus.sampleRate).withNumChannels(1).withBitsPerSample(32));

    audio.copyFrom(0, 0, corpus.fileSamples[0].data(), audio.getNumSamples());

    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()));

    writer.reset();

    corpus.source = original;
    corpus.files.set(0, audioFile);

    REQUIRE(CorpusAnalysis::save(corpus, original.getChildFile("analysis.json")).isEmpty());
    REQUIRE(original.moveFileTo(moved));

    loaded = analysis.load(moved.getChildFile("analysis.json"));

    CHECK(loaded.error.isEmpty());
    CHECK(loaded.slices.size() == corpus.slices.size());
    CHECK(loaded.source == moved);
    CHECK(loaded.files[0] == moved.getChildFile("take.wav"));
}
