#pragma once

#include <juce_data_structures/juce_data_structures.h>

class StateIdentifiers
{
public:
    static const juce::Identifier KitCatState;
    static const juce::Identifier CorpusPath;

    static const juce::Identifier OnsetThreshold;
    static const juce::Identifier OnsetMetric;
    static const juce::Identifier MinimumSliceFrames;
    static const juce::Identifier CoefficientCount;
    static const juce::Identifier NeighbourCount;
    static const juce::Identifier MinimumDistance;

    static const juce::Identifier FadeMilliseconds;
    static const juce::Identifier LengthMilliseconds;

    static const juce::Identifier SlicingMode;
    static const juce::Identifier GrainSizeMilliseconds;
    static const juce::Identifier GrainHopMilliseconds;

    static const juce::Identifier LiveMatchingEnabled;
    static const juce::Identifier InputGateThreshold;
    static const juce::Identifier PitchWeight;
    static const juce::Identifier CandidateCount;
    static const juce::Identifier RepetitionPenalty;
};
