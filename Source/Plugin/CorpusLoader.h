#pragma once

#include "Analysis/CorpusAnalysis.h"
#include "Audio/CorpusPublisher.h"
#include "Audio/SlicePlayer.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <cstdint>
#include <functional>

/*
    The starter of corpus analysis jobs, with the status the interface shows: statusText and state
    (Idle or Analysing). Jobs run on analysisPool, a single worker thread, so one runs at a time.

    analyse analyses the file or folder at the settings' CorpusPath, and loadAnalysis reads a saved
    analysis; saveAnalysis writes the published corpus to a file straight away. Starting a job reads
    the analysis settings into a CorpusAnalysis (readSettings), fades out the slices SlicePlayer is
    playing, and stops the job in progress. A finished job's Result comes back to the message
    thread, where it is dropped if a newer job has started since; otherwise it sets the status
    and, without an error, is published through CorpusPublisher and its source stored as
    CorpusPath. Every status change sends a change message.

    The editor calls it from the corpus bar and listens for its change messages, and
    KitCatAudioProcessor::setStateInformation calls analyse so a reopened project analyses its
    corpus again. A job that finishes after it is destroyed does nothing.
*/

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

    juce::String statusText = "Load a sample or a folder of samples to map its slices.";

    AnalysisState state = AnalysisState::Idle;

private:
    void startJob (const juce::String& status, std::function<CorpusAnalysis::Result(const CorpusAnalysis&)> job);

    juce::ValueTree settings;

    CorpusPublisher& corpusPublisher;
    SlicePlayer&     slicePlayer;

    std::uint64_t latestRequest = 0;

    juce::ThreadPool analysisPool { 1 };

    JUCE_DECLARE_WEAK_REFERENCEABLE(CorpusLoader)
};
