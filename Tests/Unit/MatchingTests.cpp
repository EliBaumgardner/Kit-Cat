#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Audio/TargetMatcher.h"
#include "TestCorpus.h"

#include <cmath>
#include <limits>

TEST_CASE("the KD-tree search returns the slice an exhaustive search over the same distance returns", "[matching]")
{
    const double pitchWeight = GENERATE(0.0, 0.5, 0.9);

    const CorpusAnalysis::Result corpus = makeTestCorpus(2000, 7);

    const int coeffCount = static_cast<int>(corpus.mfccMean.size());
    int       mismatches = 0;

    juce::Random random(11);

    TargetMatcher matcher;

    for (int query = 0; query < 300; ++query) {
        TargetAnalyser::TargetFeatures features;

        std::vector<double> targetStd(static_cast<size_t>(coeffCount), 0.0);

        const double targetPitch  = 80.0 + 920.0 * random.nextDouble();
        double       bestDistance = std::numeric_limits<double>::max();

        int bestSlice = -1;

        features.rms              = 0.5f;
        features.coefficientCount = coeffCount;
        features.pitchNote        = 69.0 + 12.0 * std::log2(targetPitch / 440.0);
        features.pitchConfidence  = 0.9;

        for (int coeff = 0; coeff < coeffCount; ++coeff) {
            features.mfccs[static_cast<size_t>(coeff)] = 40.0 * random.nextDouble() - 20.0;
            targetStd[static_cast<size_t>(coeff)]      = (features.mfccs[static_cast<size_t>(coeff)] - corpus.mfccMean[static_cast<size_t>(coeff)]) / corpus.mfccStdDev[static_cast<size_t>(coeff)];
        }

        for (size_t slice = 0; slice < corpus.slices.size(); ++slice) {
            const SliceAnalysis::Slice& sound = corpus.slices[slice].sound;

            double timbre   = 0.0;
            double distance = 0.0;

            for (int coeff = 1; coeff < coeffCount; ++coeff) {
                const double diff = targetStd[static_cast<size_t>(coeff)] - corpus.standardizedMfccs[slice * static_cast<size_t>(coeffCount) + static_cast<size_t>(coeff)];

                timbre += diff * diff;
            }

            distance = timbre;

            if (sound.pitchConfidence > 0.3) {
                const double octaves = std::log2(targetPitch / sound.pitch);

                distance = timbre * (1.0 - pitchWeight) + octaves * octaves * pitchWeight * 10.0;
            }

            if (distance < bestDistance) {
                bestDistance = distance;
                bestSlice    = static_cast<int>(slice);
            }
        }

        if (matcher.match(features, corpus, pitchWeight, 1, 0.0).sliceIndex != bestSlice) {
            mismatches += 1;
        }
    }

    INFO("pitch weight " << pitchWeight);
    CHECK(mismatches == 0);
}
