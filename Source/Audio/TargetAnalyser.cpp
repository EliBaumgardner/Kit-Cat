#include "Audio/TargetAnalyser.h"

#include <algorithm>
#include <cmath>
#include <limits>

TargetAnalyser::TargetAnalyser()
    : inputBuffer(1024),
      magnitude(513),
      mels(40),
      coefficients(13),
      spectrum(513),
      stft(1024, 1024, 256),
      bands(40, 1024),
      dct(40, 13),
      pitchFFT(1024)
{
    prepare(44100.0, 1024, 256, 40, 13);
}

void TargetAnalyser::prepare(double newSampleRate, int newWindowSize, int newHopSize, int newBandCount, int newCoefficientCount)
{
    sampleRate       = newSampleRate;
    windowSize       = newWindowSize;
    fftSize          = newWindowSize;
    hopSize          = newHopSize;
    bandCount        = newBandCount;
    coefficientCount = newCoefficientCount;
    binCount         = fftSize / 2 + 1;

    inputBuffer.resize(windowSize);
    spectrum.resize(binCount);
    magnitude.resize(binCount);
    mels.resize(bandCount);
    coefficients.resize(coefficientCount);
    squareMagnitude.resize(binCount);
    mirroredSquareMagnitude.resize(2 * (binCount - 1));
    difference.resize(binCount);

    pitchFFT.resize(2 * (binCount - 1));
    bands.init(minimumFrequency, maximumFrequency, bandCount, binCount, sampleRate, windowSize);
    dct.init(bandCount, coefficientCount);
}

TargetAnalyser::TargetFeatures TargetAnalyser::processFrame(const float* windowSamples, int sampleCount)
{
    TargetFeatures features;

    double sumSquares = 0.0;

    const int count = std::min(sampleCount, windowSize);

    features.coefficientCount = coefficientCount;

    for (int sample = 0; sample < count; ++sample) {
        const double value = static_cast<double>(windowSamples[sample]);

        inputBuffer(sample) = value;
        sumSquares         += value * value;
    }

    for (int sample = count; sample < windowSize; ++sample) {
        inputBuffer(sample) = 0.0;
    }

    features.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(windowSize)));

    if (features.rms < 1e-5f) {
        return features;
    }

    stft.processFrame(inputBuffer, spectrum);
    fluid::algorithm::STFT::magnitude(spectrum, magnitude);
    bands.processFrame(magnitude, mels, false, false, true, fluid::FluidDefaultAllocator());
    dct.processFrame(mels, coefficients);

    for (int coeff = 0; coeff < coefficientCount && coeff < maximumCoefficients; ++coeff) {
        features.mfccs[static_cast<size_t>(coeff)] = coefficients(coeff);
    }

    detectPitch(features);

    return features;
}

void TargetAnalyser::detectPitch(TargetFeatures& features)
{
    fluid::index minimumLag = std::lrint(sampleRate / maximumFrequency);
    fluid::index maximumLag = std::lrint(sampleRate / minimumFrequency);

    double squareSum  = 0.0;
    double runningSum = 0.0;
    double floor      = std::numeric_limits<double>::max();
    double bestValue  = std::numeric_limits<double>::lowest();
    double bestLag    = -1.0;
    double pitch      = 0.0;

    for (fluid::index bin = 0; bin < binCount; ++bin) {
        squareMagnitude(bin) = magnitude(bin) * magnitude(bin);
    }

    squareSum = 2.0 * squareMagnitude.sum();

    for (fluid::index bin = 0; bin < binCount; ++bin) {
        mirroredSquareMagnitude(bin) = squareMagnitude(bin);
    }

    for (fluid::index bin = 0; bin < binCount - 2; ++bin) {
        mirroredSquareMagnitude(binCount + bin) = squareMagnitude(binCount - 2 - bin);
    }

    const Eigen::Map<Eigen::ArrayXcd> transform = pitchFFT.process(mirroredSquareMagnitude);

    difference    = squareSum - transform.real();
    difference(0) = 1.0;

    for (fluid::index lag = 1; lag < binCount; ++lag) {
        runningSum      += difference(lag);
        difference(lag) *= lag / runningSum;
    }

    minimumLag = std::min(minimumLag, binCount - 1);
    maximumLag = std::min(maximumLag, binCount - minimumLag - 1);

    if (runningSum <= 0.0 || maximumLag <= minimumLag) {
        return;
    }

    for (fluid::index lag = minimumLag; lag < maximumLag; ++lag) {
        floor = std::min(floor, -difference(lag));
    }

    for (fluid::index lag = minimumLag + 1; lag < maximumLag - 1; ++lag) {
        const double previous = -difference(lag - 1);
        const double current  = -difference(lag);
        const double next     = -difference(lag + 1);

        if (current > previous && current > next && current > floor) {
            const double offset = 0.5 * (previous - next) / (previous - 2.0 * current + next);
            const double value  = current - 0.25 * (previous - next) * offset;

            if (value > bestValue) {
                bestValue = value;
                bestLag   = static_cast<double>(lag - minimumLag) + offset;
            }
        }
    }

    if (bestLag < 0.0) {
        return;
    }

    pitch                    = sampleRate / (static_cast<double>(minimumLag) + bestLag);
    features.pitchConfidence = std::max(1.0 + bestValue, 0.0);

    if (pitch > minimumFrequency) {
        features.pitchNote = 69.0 + 12.0 * std::log2(pitch / 440.0);
    }
}
