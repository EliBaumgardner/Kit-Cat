#include "Plugin/CorpusLoader.h"
#include "Plugin/StateIdentifiers.h"

CorpusLoader::CorpusLoader(juce::ValueTree settings, CorpusPublisher& corpusPublisher, SlicePlayer& slicePlayer)
    : settings(settings),
      corpusPublisher(corpusPublisher),
      slicePlayer(slicePlayer)
{
}

void CorpusLoader::analyse()
{
    const juce::File fileOrFolder = juce::File::createFileWithoutCheckingPath(settings[StateIdentifiers::CorpusPath].toString());

    if (fileOrFolder == juce::File()) {
        return;
    }

    startJob("Analysing " + fileOrFolder.getFileName() + "...", [fileOrFolder](const CorpusAnalysis& analysis) { return analysis.analyse(fileOrFolder); });
}

void CorpusLoader::loadAnalysis(const juce::File& analysisFile)
{
    startJob("Loading " + analysisFile.getFileName() + "...", [analysisFile](const CorpusAnalysis& analysis) { return analysis.load(analysisFile); });
}

void CorpusLoader::saveAnalysis(const juce::File& analysisFile)
{
    const std::shared_ptr<const CorpusAnalysis::Result> corpus = corpusPublisher.publishedCorpus;

    juce::String error = "There is no analysis to save yet.";

    if (corpus != nullptr) {
        error = CorpusAnalysis::save(*corpus, analysisFile);
    }

    if (error.isEmpty()) {
        statusText = "Saved the analysis to " + analysisFile.getFileName();
    } else {
        statusText = error;
    }

    sendChangeMessage();
}

CorpusAnalysis CorpusLoader::readSettings() const
{
    CorpusAnalysis analysis;

    analysis.slicing.slicingMode           = static_cast<SliceAnalysis::SlicingMode>(static_cast<int>(settings[StateIdentifiers::SlicingMode]));
    analysis.slicing.threshold             = settings[StateIdentifiers::OnsetThreshold];
    analysis.slicing.onsetMetric           = static_cast<SliceAnalysis::OnsetMetric>(static_cast<int>(settings[StateIdentifiers::OnsetMetric]));
    analysis.slicing.minimumSliceFrames    = settings[StateIdentifiers::MinimumSliceFrames];
    analysis.slicing.grainSizeMilliseconds = settings[StateIdentifiers::GrainSizeMilliseconds];
    analysis.slicing.grainHopMilliseconds  = settings[StateIdentifiers::GrainHopMilliseconds];
    analysis.slicing.coefficientCount      = settings[StateIdentifiers::CoefficientCount];
    analysis.layout.neighbourCount         = settings[StateIdentifiers::NeighbourCount];
    analysis.layout.minimumDistance        = settings[StateIdentifiers::MinimumDistance];

    return analysis;
}

void CorpusLoader::startJob(const juce::String& status, std::function<CorpusAnalysis::Result(const CorpusAnalysis&)> job)
{
    juce::WeakReference<CorpusLoader> loader(this);

    CorpusAnalysis analysis = readSettings();

    latestRequest += 1;
    statusText = status;
    state      = AnalysisState::Analysing;

    analysis.shouldStop = [] { return juce::ThreadPoolJob::getCurrentThreadPoolJob()->shouldExit(); };

    slicePlayer.push({ SlicePlayer::CommandKind::FadeOutAll, -1 });

    sendChangeMessage();

    analysisPool.removeAllJobs(true, 0);

    analysisPool.addJob([loader, job = std::move(job), analysis = std::move(analysis), request = latestRequest] {
        std::shared_ptr<CorpusAnalysis::Result> corpus = std::make_shared<CorpusAnalysis::Result>(job(analysis));

        juce::MessageManager::callAsync([loader, corpus, request] {
            if (loader == nullptr || request != loader->latestRequest) {
                return;
            }

            loader->statusText = CorpusAnalysis::describe(*corpus);
            loader->state      = AnalysisState::Idle;

            if (corpus->error.isEmpty()) {
                loader->settings.setProperty(StateIdentifiers::CorpusPath, corpus->source.getFullPathName(), nullptr);

                loader->corpusPublisher.publish(corpus);
            }

            loader->sendChangeMessage();
        });
    });
}
