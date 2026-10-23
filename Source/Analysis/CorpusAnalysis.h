#pragma once

#include "Analysis/MapLayout.h"
#include "Analysis/SliceAnalysis.h"

#include <juce_graphics/juce_graphics.h>

#include <cstdint>
#include <functional>
#include <vector>

/*
    The analysis of a corpus (a sample, or a folder of samples searched through its subfolders) and
    the Result it produces. A Result holds the audio files, each file's samples as mono at
    sampleRate, and the slices cut from them; each Slice names its file, its span and sound
    (SliceAnalysis::Slice) and its map position, 0 to 1 on each axis. It also holds the index live
    matching searches: every slice's MFCCs standardised with mfccMean and mfccStdDev (in
    standardizedMfccs, one row per slice), its pitch as a MIDI note (pitchNotes, 0 when it has
    none), and a kdTree over the standardised MFCCs. error is empty unless something failed.

    analyse cuts every file with slicing, places the slices with layout, then calls buildIndex;
    fewer than 7 slices is an error. load rebuilds a Result from a saved JSON analysis: it reads the
    audio again, from the saved path or else the one relative to the analysis file, refuses a file
    whose length changed, keeps the saved map positions and calls buildIndex. save writes one and
    returns an error message, empty on success. describe gives a one-line status and summarise
    lists every slice. When shouldStop answers true, the analysis ends with stoppedError.

    CorpusLoader runs analyse and load on its worker thread and publishes the Result through
    CorpusPublisher, after which nothing changes it. KitCatProbe runs analyse from the command line.
*/

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
        int sliceIndex = -1;
        int leftChild  = -1;
        int rightChild = -1;
        int splitDim   = 0;

        double splitValue = 0.0;
    };

    struct Slice
    {
        int fileIndex = 0;

        SliceAnalysis::Slice sound;

        juce::Point<float> position;
    };

    struct Result
    {
        juce::String error;

        juce::File source;

        double sampleRate = 0.0;

        std::uint64_t generation = 0;

        juce::Array<juce::File> files;

        std::vector<std::vector<float>> fileSamples;
        std::vector<Slice>              slices;
        std::vector<KDTreeNode>         kdTree;

        std::vector<double>             mfccMean;
        std::vector<double>             mfccStdDev;
        std::vector<double>             pitchNotes;
        std::vector<double>             standardizedMfccs;
    };

    SliceAnalysis slicing;

    MapLayout layout;

    std::function<bool()> shouldStop = [] { return false; };

    static constexpr int minimumSliceCount   = 7;
    static constexpr int analysisFileVersion = 1;

    static constexpr const char* audioFilePattern    = "*.wav;*.aif;*.aiff;*.flac;*.mp3";
    static constexpr const char* analysisFilePattern = "*.json";

private:
    void layOutMap (Result& result) const;
};
