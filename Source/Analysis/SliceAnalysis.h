#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <vector>

class SliceAnalysis
{
public:
    struct Result;

    Result analyse (const juce::File& file) const;

    static juce::String summarise (const Result& result);

    enum class OnsetMetric
    {
        Energy,
        HighFrequencyContent,
        SpectralFlux,
        ModifiedKullbackLeibler,
        ItakuraSaito,
        Cosine,
        PhaseDeviation,
        WeightedPhaseDeviation,
        ComplexDeviation,
        RectifiedComplexDeviation
    };

    struct Slice
    {
        juce::int64         startSample = 0;
        juce::int64         endSample   = 0;
        std::vector<double> meanMfcc;
    };

    struct Result
    {
        juce::String       error;
        double             sampleRate      = 0.0;
        juce::int64        lengthInSamples = 0;
        std::vector<Slice> slices;
    };

    OnsetMetric onsetMetric        = OnsetMetric::Energy;
    double      threshold          = 0.5;
    int         minimumSliceFrames = 2;
    int         filterSize         = 5;
    int         windowSize         = 1024;
    int         fftSize            = 1024;
    int         hopSize            = 512;
    int         bandCount          = 40;
    int         coefficientCount   = 13;
    double      minimumFrequency   = 20.0;
    double      maximumFrequency   = 20000.0;

    static constexpr int maximumFilterSize = 101;

private:
    struct Frames;

    std::vector<double> readMonoSamples (const juce::File& file, Result& result) const;

    Frames analyseFrames (const std::vector<double>& samples, double sampleRate) const;

    void collectSlices (const Frames& frames, Result& result) const;

    struct Frames
    {
        std::vector<juce::int64>         onsetSamples;
        std::vector<std::vector<double>> mfccs;
    };
};
