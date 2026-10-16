#pragma once

#include "Analysis/MapLayout.h"
#include "Analysis/SliceAnalysis.h"

#include <juce_graphics/juce_graphics.h>

#include <cstdint>
#include <functional>
#include <vector>

class CorpusAnalysis
{
public:
    struct Result;

    Result analyse (const juce::File& fileOrFolder) const;
    Result load    (const juce::File& analysisFile) const;

    void buildIndex (Result& result) const;

    static juce::Array<juce::File> findAudioFiles (const juce::File& fileOrFolder);

    static juce::String describe  (const Result& result);
    static juce::String summarise (const Result& result);
    static juce::String save      (const Result& result, const juce::File& analysisFile);

    struct KDTreeNode
    {
        int    sliceIndex = -1;
        int    leftChild  = -1;
        int    rightChild = -1;
        int    splitDim   = 0;
        double splitValue = 0.0;
    };

    struct Slice
    {
        int                  fileIndex = 0;
        SliceAnalysis::Slice sound;
        juce::Point<float>   position;
    };

    struct Result
    {
        juce::String                    error;
        juce::File                      source;
        double                          sampleRate = 0.0;
        std::uint64_t                   generation = 0;

        juce::Array<juce::File>         files;
        std::vector<std::vector<float>> fileSamples;

        std::vector<Slice>              slices;
        std::vector<KDTreeNode>         kdTree;

        std::vector<double>             mfccMean;
        std::vector<double>             mfccStdDev;
        std::vector<double>             pitchNotes;
        std::vector<double>             standardizedMfccs;
    };

    SliceAnalysis         slicing;
    MapLayout             layout;
    std::function<bool()> shouldStop = [] { return false; };

    static constexpr int         minimumSliceCount   = 7;
    static constexpr int         analysisFileVersion = 1;
    static constexpr const char* audioFilePattern    = "*.wav;*.aif;*.aiff;*.flac;*.mp3";
    static constexpr const char* analysisFilePattern = "*.json";

private:
    void layOutMap (Result& result) const;
};
