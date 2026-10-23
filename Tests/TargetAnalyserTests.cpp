#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Audio/TargetAnalyser.h"

#include <flucoma/algorithms/public/YINFFT.hpp>

#include <juce_core/juce_core.h>

#include <cmath>
#include <vector>

TEST_CASE("the live analyser reads a sine's level and its equal-tempered note", "[analyser]")
{
    constexpr double sampleRate = 44100.0;
    constexpr double amplitude  = 0.5;

    constexpr int windowSize = 1024;

    const double expectedNote = GENERATE(57.0, 69.0, 81.0);
    const double frequency    = 440.0 * std::pow(2.0, (expectedNote - 69.0) / 12.0);

    TargetAnalyser analyser;

    std::vector<float> window(windowSize);

    TargetAnalyser::TargetFeatures features;

    analyser.prepare(sampleRate, windowSize, 256, 40, 13);

    for (int sample = 0; sample < windowSize; ++sample) {
        window[static_cast<size_t>(sample)] = static_cast<float>(amplitude * std::sin(2.0 * juce::MathConstants<double>::pi * frequency * sample / sampleRate));
    }

    features = analyser.processFrame(window.data(), windowSize);

    INFO("note " << expectedNote);
    CHECK_THAT(features.rms, Catch::Matchers::WithinAbs(amplitude / std::sqrt(2.0), 0.01));
    CHECK_THAT(features.pitchNote, Catch::Matchers::WithinAbs(expectedNote, 0.5));
    CHECK(features.pitchConfidence > 0.3);
}

TEST_CASE("the live analyser reports silence as no level and no note", "[analyser]")
{
    TargetAnalyser analyser;

    std::vector<float> window(1024, 0.0f);

    TargetAnalyser::TargetFeatures features;

    analyser.prepare(44100.0, 1024, 256, 40, 13);

    features = analyser.processFrame(window.data(), 1024);

    CHECK(features.rms == 0.0f);
    CHECK(features.pitchNote == 0.0);
}

TEST_CASE("the live analyser's pitch step agrees with FluCoMa's YINFFT on the same frames", "[analyser]")
{
    constexpr double sampleRate = 44100.0;

    constexpr int windowSize = 1024;
    constexpr int binCount   = windowSize / 2 + 1;

    juce::Random random(21);

    TargetAnalyser analyser;

    fluid::algorithm::STFT stft(windowSize, windowSize, 256);

    fluid::algorithm::YINFFT yin(binCount);

    fluid::RealVector frame(windowSize);
    fluid::RealVector magnitude(binCount);
    fluid::RealVector reference(2);

    fluid::ComplexVector spectrum(binCount);

    std::vector<float> window(windowSize);

    int disagreements = 0;

    analyser.prepare(sampleRate, windowSize, 256, 40, 13);

    for (int trial = 0; trial < 200; ++trial) {
        const double fundamental   = 50.0 + 1500.0 * random.nextDouble();
        const double noise         = 0.3 * random.nextDouble();
        double       referenceNote = 0.0;

        TargetAnalyser::TargetFeatures features;

        for (int sample = 0; sample < windowSize; ++sample) {
            double value = noise * (2.0 * random.nextDouble() - 1.0);

            for (int harmonic = 1; harmonic <= 1 + trial % 5; ++harmonic) {
                value += 0.4 / harmonic * std::sin(2.0 * juce::MathConstants<double>::pi * fundamental * harmonic * sample / sampleRate);
            }

            window[static_cast<size_t>(sample)] = static_cast<float>(value);
            frame(sample)                       = static_cast<double>(window[static_cast<size_t>(sample)]);
        }

        features = analyser.processFrame(window.data(), windowSize);

        stft.processFrame(frame, spectrum);
        fluid::algorithm::STFT::magnitude(spectrum, magnitude);
        yin.processFrame(magnitude, reference, TargetAnalyser::minimumFrequency, TargetAnalyser::maximumFrequency, sampleRate);

        if (reference(0) > TargetAnalyser::minimumFrequency) {
            referenceNote = 69.0 + 12.0 * std::log2(reference(0) / 440.0);
        }

        if (std::abs(features.pitchNote - referenceNote) > 1e-9 || std::abs(features.pitchConfidence - reference(1)) > 1e-9) {
            disagreements += 1;
        }
    }

    CHECK(disagreements == 0);
}
