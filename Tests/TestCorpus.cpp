#include "TestCorpus.h"

#include <cmath>

CorpusAnalysis::Result makeTestCorpus(int sliceCount, juce::int64 seed)
{
    constexpr int sliceLength = 4096;

    constexpr double sampleRate = 44100.0;

    const CorpusAnalysis analysis;

    juce::Random random(seed);

    CorpusAnalysis::Result corpus;

    std::vector<float> samples(static_cast<size_t>(sliceCount * sliceLength));

    corpus.sampleRate = sampleRate;
    corpus.files.add(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("KitCatTestCorpus.wav"));

    for (size_t sample = 0; sample < samples.size(); ++sample) {
        samples[sample] = static_cast<float>(0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * static_cast<double>(sample) / sampleRate));
    }

    corpus.fileSamples.push_back(std::move(samples));

    for (int slice = 0; slice < sliceCount; ++slice) {
        CorpusAnalysis::Slice entry;

        entry.sound.startSample     = static_cast<juce::int64>(slice) * sliceLength;
        entry.sound.endSample       = entry.sound.startSample + sliceLength;
        entry.sound.pitch           = 80.0 + 920.0 * random.nextDouble();
        entry.sound.pitchConfidence = random.nextDouble();
        entry.position              = { random.nextFloat(), random.nextFloat() };

        for (int coefficient = 0; coefficient < analysis.slicing.coefficientCount; ++coefficient) {
            entry.sound.meanMfcc.push_back(40.0 * random.nextDouble() - 20.0);
        }

        corpus.slices.push_back(std::move(entry));
    }

    analysis.buildIndex(corpus);

    return corpus;
}
