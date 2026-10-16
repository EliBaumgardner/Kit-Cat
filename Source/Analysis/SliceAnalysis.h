#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <functional>
#include <vector>

class SliceAnalysis
{
public:
    struct Result;

    Result analyse (const juce::File& file, const std::function<bool()>& shouldStop) const;

    void readMonoSamples (const juce::File& file, Result& result) const;

    enum class SlicingMode
    {
        Onset,
        Granular
    };

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
        juce::int64         startSample     = 0;
        juce::int64         endSample       = 0;
        double              pitch           = 0.0;
        double              pitchConfidence = 0.0;
        std::vector<double> meanMfcc;
    };

    struct Result
    {
        juce::String       error;
        juce::int64        lengthInSamples = 0;
        std::vector<float> samples;
        std::vector<Slice> slices;
    };

    double      sampleRate            = 44100.0;

    SlicingMode slicingMode           = SlicingMode::Onset;
    OnsetMetric onsetMetric           = OnsetMetric::Energy;

    double      threshold             = 0.5;
    double      grainSizeMilliseconds = 60.0;
    double      grainHopMilliseconds  = 30.0;

    int         minimumSliceFrames    = 2;
    int         filterSize            = 5;
    int         windowSize            = 1024;
    int         fftSize               = 1024;
    int         hopSize               = 512;
    int         bandCount             = 40;
    int         coefficientCount      = 13;

    double      minimumFrequency      = 20.0;
    double      maximumFrequency      = 20000.0;

    static constexpr int         maximumFilterSize = 101;
    static constexpr double      voicedConfidence  = 0.3;
    static constexpr const char* stoppedError      = "The analysis was stopped";

private:
    struct Frames;

    Frames analyseFrames (const std::vector<float>& samples, const std::function<bool()>& shouldStop) const;

    void collectSlices (const Frames& frames, Result& result) const;

    struct Frames
    {
        std::vector<juce::int64> onsetSamples;
        std::vector<double>      mfccs;
        std::vector<double>      pitches;
        std::vector<double>      confidences;
    };
};
