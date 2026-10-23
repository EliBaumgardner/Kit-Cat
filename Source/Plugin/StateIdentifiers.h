#pragma once

#include <juce_data_structures/juce_data_structures.h>

/*
    The keys of the settings ValueTree: KitCatState is its type, and each other identifier names one
    of its properties. CorpusPath and the slicing, MFCC and map keys are read by the next analysis
    (CorpusLoader::readSettings). The live matching and playback keys are copied into the atomics
    the audio thread reads (KitCatAudioProcessor::valueTreePropertyChanged). ParameterPanel binds
    its controls to them. The strings are written into every saved project, so a renamed key is
    lost from projects saved before.
*/

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
