#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <functional>
#include <vector>

/*
    The settings and steps that cut one audio file into slices and describe each slice's sound. A
    Result holds the file's samples, mixed to mono and resampled to sampleRate, and its Slices. A
    Slice holds its start and end sample, its MFCCs averaged over the frames it covers, and its
    pitch in Hz with a confidence from 0 to 1.

    analyse reads the file and goes through it a frame at a time (windowSize samples every
    hopSize), taking MFCCs, pitch and, in SlicingMode::Onset, onsets. Onset mode cuts at every
    onset onsetMetric finds above threshold, at least minimumSliceFrames apart, so the slices cover
    the whole file. Granular mode cuts grains of grainSizeMilliseconds every grainHopMilliseconds,
    which overlap when the hop is shorter, and leaves out a last grain the file ends inside. A
    slice's pitch averages only the frames with confidence above voicedConfidence, weighted by it,
    and is 0 when there are none. readMonoSamples is the reading step alone. analyse asks
    shouldStop on every frame and returns stoppedError when it answers true.

    CorpusAnalysis holds one as slicing, set by CorpusLoader::readSettings, runs it on each file on
    CorpusLoader's worker thread, and calls readMonoSamples to load a saved analysis.
*/

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
        juce::int64 startSample = 0;
        juce::int64 endSample   = 0;

        double pitch           = 0.0;
        double pitchConfidence = 0.0;

        std::vector<double> meanMfcc;
    };

    struct Result
    {
        juce::String error;

        juce::int64 lengthInSamples = 0;

        std::vector<float> samples;
        std::vector<Slice> slices;
    };

    double sampleRate            = 44100.0;
    double threshold             = 0.5;

    double grainSizeMilliseconds = 60.0;
    double grainHopMilliseconds  = 30.0;

    double minimumFrequency      = 20.0;
    double maximumFrequency      = 20000.0;

    SlicingMode slicingMode = SlicingMode::Onset;

    OnsetMetric onsetMetric = OnsetMetric::Energy;

    int minimumSliceFrames = 2;
    int filterSize         = 5;
    int windowSize         = 1024;
    int fftSize            = 1024;
    int hopSize            = 512;
    int bandCount          = 40;
    int coefficientCount   = 13;

    static constexpr int maximumFilterSize = 101;

    static constexpr double voicedConfidence = 0.3;

    static constexpr const char* stoppedError = "The analysis was stopped";

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
