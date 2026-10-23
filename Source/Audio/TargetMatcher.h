#pragma once

#include "Analysis/CorpusAnalysis.h"
#include "Audio/TargetAnalyser.h"

#include <array>

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
