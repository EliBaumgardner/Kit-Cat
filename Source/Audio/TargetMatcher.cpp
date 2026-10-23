#include "Audio/TargetMatcher.h"

#include <algorithm>
#include <cmath>

TargetMatcher::TargetMatcher()
{
    reset();
}

TargetMatcher::MatchResult TargetMatcher::match(const TargetAnalyser::TargetFeatures& features, const CorpusAnalysis::Result& corpus,
                                                double pitchWeight, int candidateCount, double repetitionPenalty)
{
    MatchResult result;

    if (corpus.slices.empty() || corpus.kdTree.empty() || corpus.mfccMean.empty()) {
        return result;
    }

    const int coeffCount = std::min(features.coefficientCount, static_cast<int>(corpus.mfccMean.size()));

    const double pitchShare  = std::clamp(pitchWeight, 0.0, 1.0);
    double       timbreShare = 1.0;

    const size_t wanted    = static_cast<size_t>(std::clamp(candidateCount, 1, maximumCandidates));
    size_t       found     = 0;
    size_t       worst     = 0;
    size_t       stackSize = 0;

    std::array<double, TargetAnalyser::maximumCoefficients> targetStd {};
    std::array<Candidate, maximumCandidates>                candidates {};
    std::array<int, stackCapacity>                          stack {};

    if (features.pitchConfidence > SliceAnalysis::voicedConfidence && features.pitchNote > 0.0) {
        timbreShare = 1.0 - pitchShare;
    }

    for (size_t coeff = 1; coeff < static_cast<size_t>(coeffCount); ++coeff) {
        targetStd[coeff] = (features.mfccs[coeff] - corpus.mfccMean[coeff]) / corpus.mfccStdDev[coeff];
    }

    stack[stackSize++] = 0;

    while (stackSize > 0) {
        const CorpusAnalysis::KDTreeNode& node = corpus.kdTree[static_cast<size_t>(stack[--stackSize])];

        const double distance    = calculateDistance(node.sliceIndex, corpus, targetStd, features.pitchNote, features.pitchConfidence, coeffCount, pitchShare, repetitionPenalty);
        const double splitOffset = targetStd[static_cast<size_t>(node.splitDim)] - node.splitValue;

        int nearChild = node.leftChild;
        int farChild  = node.rightChild;

        if (found < wanted || distance < candidates[worst].distance) {
            if (found < wanted) {
                worst  = found;
                found += 1;
            }

            candidates[worst] = { distance, node.sliceIndex };
            worst             = 0;

            for (size_t index = 1; index < found; ++index) {
                if (candidates[index].distance > candidates[worst].distance) {
                    worst = index;
                }
            }
        }

        if (splitOffset > 0.0) {
            std::swap(nearChild, farChild);
        }

        if (farChild >= 0 && stackSize < stackCapacity && (found < wanted || splitOffset * splitOffset * timbreShare < candidates[worst].distance)) {
            stack[stackSize++] = farChild;
        }

        if (nearChild >= 0 && stackSize < stackCapacity) {
            stack[stackSize++] = nearChild;
        }
    }

    const int chosenSlice = selectCandidate(candidates, found);

    recentSlices[recentIndex] = chosenSlice;
    recentIndex               = (recentIndex + 1) % recentHistorySize;

    result.sliceIndex  = chosenSlice;
    result.mapPosition = corpus.slices[static_cast<size_t>(chosenSlice)].position;
    result.rms         = features.rms;

    return result;
}

void TargetMatcher::reset()
{
    recentSlices.fill(-1);
    recentIndex = 0;
    randomState = 123456789;
}

double TargetMatcher::calculateDistance(int sliceIndex, const CorpusAnalysis::Result& corpus, const std::array<double, TargetAnalyser::maximumCoefficients>& targetMfccs,
                                        double targetNote, double targetConfidence, int coefficientCount, double pitchWeight, double repetitionPenalty) const
{
    const size_t slice = static_cast<size_t>(sliceIndex);
    const size_t row   = slice * corpus.mfccMean.size();

    const double sliceNote      = corpus.pitchNotes[slice];
    const double sliceConfident = corpus.slices[slice].sound.pitchConfidence;
    double       timbreDistSq   = 0.0;
    double       distance       = 0.0;

    for (size_t coeff = 1; coeff < static_cast<size_t>(coefficientCount); ++coeff) {
        const double diff = targetMfccs[coeff] - corpus.standardizedMfccs[row + coeff];

        timbreDistSq += diff * diff;
    }

    distance = timbreDistSq;

    if (targetConfidence > SliceAnalysis::voicedConfidence && sliceConfident > SliceAnalysis::voicedConfidence && targetNote > 0.0 && sliceNote > 0.0) {
        const double octaveDiff  = (targetNote - sliceNote) / 12.0;
        const double pitchDistSq = octaveDiff * octaveDiff;

        distance = timbreDistSq * (1.0 - pitchWeight) + pitchDistSq * (pitchWeight * 10.0);
    }

    if (repetitionPenalty > 0.0 && std::find(recentSlices.begin(), recentSlices.end(), sliceIndex) != recentSlices.end()) {
        distance *= 1.0 + repetitionPenalty * 4.0;
    }

    return distance;
}

int TargetMatcher::selectCandidate(const std::array<Candidate, maximumCandidates>& candidates, size_t count)
{
    std::array<double, maximumCandidates> weights {};

    double totalWeight = 0.0;
    double accumulated = 0.0;
    double target      = 0.0;

    if (count <= 1) {
        return candidates[0].sliceIndex;
    }

    for (size_t index = 0; index < count; ++index) {
        weights[index]  = 1.0 / (candidates[index].distance + 1e-4);
        totalWeight    += weights[index];
    }

    randomState = randomState * 1664525u + 1013904223u;
    target      = static_cast<double>(randomState & 0x00ffffff) / static_cast<double>(0x01000000) * totalWeight;

    for (size_t index = 0; index < count; ++index) {
        accumulated += weights[index];

        if (accumulated >= target) {
            return candidates[index].sliceIndex;
        }
    }

    return candidates[0].sliceIndex;
}
