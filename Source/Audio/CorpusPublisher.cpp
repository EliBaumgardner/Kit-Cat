#include "Audio/CorpusPublisher.h"

const CorpusAnalysis::Result* CorpusPublisher::beginBlock()
{
    blockEpoch.fetch_add(1, std::memory_order_seq_cst);

    return currentCorpus.load(std::memory_order_seq_cst);
}

void CorpusPublisher::endBlock()
{
    blockEpoch.fetch_add(1, std::memory_order_seq_cst);
}

void CorpusPublisher::publish(std::shared_ptr<CorpusAnalysis::Result> corpus)
{
    static_assert(std::atomic<const CorpusAnalysis::Result*>::is_always_lock_free, "the audio thread must be able to read the corpus without a lock");

    const CorpusAnalysis::Result*                 raw = corpus.get();
    std::shared_ptr<const CorpusAnalysis::Result> retired = std::move(publishedCorpus);

    corpus->generation = ++corpusGeneration;
    publishedCorpus = std::move(corpus);

    currentCorpus.store(raw, std::memory_order_seq_cst);

    const std::uint64_t retiredAtEpoch = blockEpoch.load(std::memory_order_seq_cst);
    const bool          audioThreadInsideBlock = retiredAtEpoch % 2 != 0;

    if (retired != nullptr && audioThreadInsideBlock) {
        retiredCorpora.push_back({ std::move(retired), retiredAtEpoch });
    }

    collectRetiredCorpora();
}

void CorpusPublisher::collectRetiredCorpora()
{
    const std::uint64_t epoch = blockEpoch.load(std::memory_order_seq_cst);

    std::erase_if(retiredCorpora, [epoch](const RetiredCorpus& entry) { return epoch > entry.retiredAtEpoch; });
}
