#pragma once

#include "Analysis/CorpusAnalysis.h"
#include "Audio/CorpusPublisher.h"
#include "Audio/SlicePlayer.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <cstdint>
#include <functional>

class CorpusLoader : public juce::ChangeBroadcaster
{
public:
    CorpusLoader(juce::ValueTree settings, CorpusPublisher& corpusPublisher, SlicePlayer& slicePlayer);

    void analyse      ();
    void loadAnalysis (const juce::File& analysisFile);
    void saveAnalysis (const juce::File& analysisFile);

    CorpusAnalysis readSettings () const;

    enum class AnalysisState
    {
        Idle,
        Analysing
    };

    juce::String  statusText = "Load a sample or a folder of samples to map its slices.";
    AnalysisState state      = AnalysisState::Idle;

private:
    void startJob (const juce::String& status, std::function<CorpusAnalysis::Result(const CorpusAnalysis&)> job);

    juce::ValueTree  settings;
    CorpusPublisher& corpusPublisher;
    SlicePlayer&     slicePlayer;
    std::uint64_t    latestRequest = 0;
    juce::ThreadPool analysisPool { 1 };

    JUCE_DECLARE_WEAK_REFERENCEABLE(CorpusLoader)
};
