#pragma once

#include <flucoma/algorithms/public/DCT.hpp>
#include <flucoma/algorithms/public/MelBands.hpp>
#include <flucoma/algorithms/public/STFT.hpp>
#include <flucoma/algorithms/util/FFT.hpp>
#include <flucoma/data/FluidMemory.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <array>

/*
    The analyser of one window of live input. From up to windowSize samples it produces a
    TargetFeatures: the window's level (rms), its MFCCs (coefficients describing the shape of its
    spectrum, of the same kind SliceAnalysis keeps for each slice), and its pitch as a fractional
    MIDI note with a confidence from 0 to 1.

    prepare sets the sample rate and the window, hop, mel band and coefficient counts, and sizes
    every buffer; it allocates, so it runs from prepareToPlay. processFrame analyses one window
    into those buffers without allocating. A window quieter than 1e-5 gets its level only, with no
    MFCCs or pitch. The pitch step is FluCoMa's YINFFT rewritten onto the buffers prepare made,
    because FluCoMa's own version allocates on every frame.

    KitCatAudioProcessor::processBlock calls processFrame on the audio thread once per hop, on the
    last 1024 input samples, and passes the result to TargetMatcher.
*/

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
