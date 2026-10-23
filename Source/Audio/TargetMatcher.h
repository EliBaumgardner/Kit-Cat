#pragma once

#include "Analysis/CorpusAnalysis.h"
#include "Audio/TargetAnalyser.h"

#include <array>

/*
    The chooser of the corpus slice that answers a window of live input. match takes the window's
    TargetFeatures and a corpus, finds the candidateCount slices closest to it, picks one of them
    at random with closer ones more likely, and returns its index, its map position and the
    input's level. It remembers the last 8 slices it chose.

    Closeness is the distance between the window's MFCCs and each slice's, both standardised with
    the corpus's mfccMean and mfccStdDev, leaving out the first coefficient, which mostly follows
    loudness. When the window and the slice both have a pitch read with confidence above
    SliceAnalysis::voicedConfidence, pitchWeight (0 to 1) moves the distance from timbre toward
    the difference in octaves. A slice among the last 8 has its distance multiplied by
    1 + 4 x repetitionPenalty. The search walks the corpus's kdTree with fixed arrays, so it never
    allocates. reset forgets the recent slices and restarts the random sequence.

    KitCatAudioProcessor::processBlock calls match on the audio thread for each hop above the input
    gate, and prepareToPlay calls reset. A sliceIndex of -1 means the corpus has no index.
*/

class TargetMatcher
{
public:
    struct MatchResult
    {
        int sliceIndex = -1;

        juce::Point<float> mapPosition;

        float rms = 0.0f;
    };

    TargetMatcher();

    MatchResult match (const TargetAnalyser::TargetFeatures& features, const CorpusAnalysis::Result& corpus,
                       double pitchWeight, int candidateCount, double repetitionPenalty);

    void reset ();

    static constexpr int maximumCandidates = 16;
    static constexpr int recentHistorySize = 8;
    static constexpr int stackCapacity     = 64;

private:
    struct Candidate
    {
        double distance = 0.0;

        int sliceIndex = -1;
    };

    double calculateDistance (int sliceIndex, const CorpusAnalysis::Result& corpus, const std::array<double, TargetAnalyser::maximumCoefficients>& targetMfccs,
                             double targetNote, double targetConfidence, int coefficientCount, double pitchWeight, double repetitionPenalty) const;

    int selectCandidate (const std::array<Candidate, maximumCandidates>& candidates, size_t count);

    std::array<int, recentHistorySize> recentSlices {};

    size_t recentIndex = 0;

    std::uint32_t randomState = 123456789;
};
