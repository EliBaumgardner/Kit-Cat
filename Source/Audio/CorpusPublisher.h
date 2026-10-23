#pragma once

#include "Analysis/CorpusAnalysis.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

/*
    The holder of the corpus the audio thread plays from: publishedCorpus, the shared_ptr the
    message thread keeps, and currentCorpus, an atomic pointer to the same CorpusAnalysis::Result
    that the audio thread reads without a lock. Through it a new analysis replaces the corpus while
    audio plays, and the audio thread never frees one.

    publish, on the message thread, stamps the new corpus with the next generation number, makes it
    current and lets go of the old one. beginBlock and endBlock each add one to blockEpoch, so the
    count is odd while a block runs, and beginBlock returns the corpus the block uses to its end;
    BlockScope calls both around one processBlock. A corpus replaced while the count is even is
    held by no block and is freed at once. One replaced while it is odd waits in retiredCorpora
    until a later publish sees the count has moved past it. Both sides use seq_cst, which rules out
    a block loading the old pointer after publish read an even count.

    KitCatAudioProcessor owns it. CorpusLoader publishes each finished analysis, the editor hands
    publishedCorpus to CorpusMap, and SlicePlayer::render empties its slots when the generation
    changes.
*/

class CorpusPublisher
{
public:
    const CorpusAnalysis::Result* beginBlock ();

    void endBlock ();
    void publish  (std::shared_ptr<CorpusAnalysis::Result> corpus);

    struct BlockScope
    {
        CorpusPublisher& publisher;

        const CorpusAnalysis::Result* corpus = publisher.beginBlock();

        ~BlockScope()
        {
            publisher.endBlock();
        }
    };

    std::shared_ptr<const CorpusAnalysis::Result> publishedCorpus;

private:
    void collectRetiredCorpora ();

    struct RetiredCorpus
    {
        std::shared_ptr<const CorpusAnalysis::Result> corpus;

        std::uint64_t retiredAtEpoch = 0;
    };

    std::atomic<const CorpusAnalysis::Result*> currentCorpus { nullptr };
    std::atomic<std::uint64_t>                 blockEpoch { 0 };

    std::vector<RetiredCorpus> retiredCorpora;

    std::uint64_t corpusGeneration = 0;
};
