#pragma once

#include <flucoma/algorithms/public/DCT.hpp>
#include <flucoma/algorithms/public/MelBands.hpp>
#include <flucoma/algorithms/public/STFT.hpp>
#include <flucoma/algorithms/util/FFT.hpp>
#include <flucoma/data/FluidMemory.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <array>

class TargetAnalyser
{
public:
    struct TargetFeatures;

    TargetAnalyser();

    void prepare (double sampleRate, int windowSize = 1024, int hopSize = 256, int bandCount = 40, int coefficientCount = 13);

    TargetFeatures processFrame (const float* windowSamples, int sampleCount);

    static constexpr int maximumCoefficients = 40;

    static constexpr double minimumFrequency = 20.0;
    static constexpr double maximumFrequency = 20000.0;

    struct TargetFeatures
    {
        float rms = 0.0f;

        double pitchNote       = 0.0;
        double pitchConfidence = 0.0;

        int coefficientCount = 0;

        std::array<double, maximumCoefficients> mfccs {};
    };

private:
    void detectPitch (TargetFeatures& features);

    double sampleRate = 44100.0;

    int windowSize       = 1024;
    int fftSize          = 1024;
    int hopSize          = 256;
    int bandCount        = 40;
    int coefficientCount = 13;

    fluid::index binCount = 513;

    fluid::RealVector inputBuffer;
    fluid::RealVector magnitude;
    fluid::RealVector mels;
    fluid::RealVector coefficients;

    fluid::ComplexVector spectrum;

    Eigen::ArrayXd squareMagnitude;
    Eigen::ArrayXd mirroredSquareMagnitude;
    Eigen::ArrayXd difference;

    fluid::algorithm::STFT stft;

    fluid::algorithm::MelBands bands;

    fluid::algorithm::DCT dct;

    fluid::algorithm::FFT pitchFFT;
};
