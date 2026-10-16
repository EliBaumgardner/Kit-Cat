#pragma once

#include "Analysis/CorpusAnalysis.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

class CorpusPublisher
{
public:
    const CorpusAnalysis::Result* beginBlock ();

    void endBlock ();
    void publish  (std::shared_ptr<CorpusAnalysis::Result> corpus);

    struct BlockScope
    {
        CorpusPublisher&              publisher;
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
        std::uint64_t                                 retiredAtEpoch = 0;
    };

    std::atomic<const CorpusAnalysis::Result*>    currentCorpus { nullptr };
    std::atomic<std::uint64_t>                    blockEpoch { 0 };
    std::vector<RetiredCorpus>                    retiredCorpora;
    std::uint64_t                                 corpusGeneration = 0;
};
