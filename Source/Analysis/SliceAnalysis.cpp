#include "Analysis/SliceAnalysis.h"

#include <flucoma/algorithms/public/DCT.hpp>
#include <flucoma/algorithms/public/MelBands.hpp>
#include <flucoma/algorithms/public/OnsetSegmentation.hpp>
#include <flucoma/algorithms/public/STFT.hpp>
#include <flucoma/data/FluidMemory.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <algorithm>

SliceAnalysis::Result SliceAnalysis::analyse(const juce::File& file) const
{
    Result              result;
    std::vector<double> samples = readMonoSamples(file, result);

    if (result.error.isNotEmpty()) {
        return result;
    }

    collectSlices(analyseFrames(samples, result.sampleRate), result);

    return result;
}

juce::String SliceAnalysis::summarise(const Result& result)
{
    constexpr int listedSliceLimit = 8;
    constexpr int listedCoefficientLimit = 4;

    const int    listedSlices = std::min(listedSliceLimit, static_cast<int>(result.slices.size()));
    const double seconds = static_cast<double>(result.lengthInSamples) / result.sampleRate;
    juce::String text;

    if (result.error.isNotEmpty()) {
        return result.error + "\n";
    }

    text << static_cast<int>(result.slices.size()) << " slices in " << juce::String(seconds, 2) << " s at " << result.sampleRate << " Hz\n";

    for (int sliceIndex = 0; sliceIndex < listedSlices; ++sliceIndex) {
        const Slice& slice = result.slices[static_cast<size_t>(sliceIndex)];
        const int    listedCoefficients = std::min(listedCoefficientLimit, static_cast<int>(slice.meanMfcc.size()));

        text << "  " << juce::String(static_cast<double>(slice.startSample) / result.sampleRate, 3) << " s  mfcc";

        for (int coefficient = 0; coefficient < listedCoefficients; ++coefficient) {
            text << " " << juce::String(slice.meanMfcc[static_cast<size_t>(coefficient)], 2);
        }

        text << "\n";
    }

    return text;
}

std::vector<double> SliceAnalysis::readMonoSamples(const juce::File& file, Result& result) const
{
    juce::AudioFormatManager                 formats;
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float>                 buffer;
    std::vector<double>                      samples;

    formats.registerBasicFormats();

    reader.reset(formats.createReaderFor(file));

    if (reader == nullptr) {
        result.error = "Could not read " + file.getFullPathName();
        return samples;
    }

    result.sampleRate = reader->sampleRate;
    result.lengthInSamples = reader->lengthInSamples;

    buffer.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
    samples.assign(static_cast<size_t>(buffer.getNumSamples()), 0.0);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            samples[static_cast<size_t>(sample)] += buffer.getSample(channel, sample) / buffer.getNumChannels();
        }
    }

    return samples;
}

SliceAnalysis::Frames SliceAnalysis::analyseFrames(const std::vector<double>& samples, double sampleRate) const
{
    const fluid::index sampleCount = static_cast<fluid::index>(samples.size());
    const fluid::index binCount = fftSize / 2 + 1;
    const fluid::index frameCount = (sampleCount + hopSize) / hopSize;
    const fluid::index metric = static_cast<fluid::index>(onsetMetric);

    fluid::RealVector                   padded(sampleCount + windowSize + hopSize);
    fluid::ComplexVector                spectrum(binCount);
    fluid::RealVector                   magnitude(binCount);
    fluid::RealVector                   mels(bandCount);
    fluid::RealVector                   coefficients(coefficientCount);
    fluid::algorithm::STFT              stft(windowSize, fftSize, hopSize);
    fluid::algorithm::MelBands          bands(bandCount, fftSize);
    fluid::algorithm::DCT               dct(bandCount, coefficientCount);
    fluid::algorithm::OnsetSegmentation onsets(fftSize, maximumFilterSize);
    Frames                              frames;

    std::fill(padded.begin(), padded.end(), 0.0);
    std::copy(samples.begin(), samples.end(), padded.begin() + windowSize / 2);

    bands.init(minimumFrequency, maximumFrequency, bandCount, binCount, sampleRate, windowSize);
    dct.init(bandCount, coefficientCount);
    onsets.init(windowSize, fftSize, filterSize);

    for (fluid::index frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        fluid::RealVectorView window = padded(fluid::Slice(frameIndex * hopSize, windowSize));

        stft.processFrame(window, spectrum);
        fluid::algorithm::STFT::magnitude(spectrum, magnitude);
        bands.processFrame(magnitude, mels, false, false, true, fluid::FluidDefaultAllocator());
        dct.processFrame(mels, coefficients);

        frames.mfccs.emplace_back(coefficients.begin(), coefficients.end());

        if (onsets.processFrame(window, metric, filterSize, threshold, minimumSliceFrames, 0) > 0.0) {
            frames.onsetSamples.push_back(frameIndex * hopSize);
        }
    }

    return frames;
}

void SliceAnalysis::collectSlices(const Frames& frames, Result& result) const
{
    const juce::int64        lastFrame = static_cast<juce::int64>(frames.mfccs.size());
    std::vector<juce::int64> boundaries { 0 };

    for (const juce::int64 onset : frames.onsetSamples) {
        if (onset > boundaries.back() && onset < result.lengthInSamples) {
            boundaries.push_back(onset);
        }
    }

    boundaries.push_back(result.lengthInSamples);

    for (size_t boundary = 0; boundary + 1 < boundaries.size(); ++boundary) {
        const juce::int64 firstFrame = std::min(boundaries[boundary] / hopSize, lastFrame - 1);
        const juce::int64 endFrame = std::clamp(boundaries[boundary + 1] / hopSize, firstFrame + 1, lastFrame);
        Slice             slice;

        slice.startSample = boundaries[boundary];
        slice.endSample = boundaries[boundary + 1];
        slice.meanMfcc.assign(static_cast<size_t>(coefficientCount), 0.0);

        for (juce::int64 frame = firstFrame; frame < endFrame; ++frame) {
            for (int coefficient = 0; coefficient < coefficientCount; ++coefficient) {
                slice.meanMfcc[static_cast<size_t>(coefficient)] += frames.mfccs[static_cast<size_t>(frame)][static_cast<size_t>(coefficient)] / static_cast<double>(endFrame - firstFrame);
            }
        }

        result.slices.push_back(std::move(slice));
    }
}
