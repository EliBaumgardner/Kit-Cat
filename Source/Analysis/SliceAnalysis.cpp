#include "Analysis/SliceAnalysis.h"

#include <flucoma/algorithms/public/DCT.hpp>
#include <flucoma/algorithms/public/MelBands.hpp>
#include <flucoma/algorithms/public/OnsetSegmentation.hpp>
#include <flucoma/algorithms/public/STFT.hpp>
#include <flucoma/algorithms/public/YINFFT.hpp>
#include <flucoma/data/FluidMemory.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <algorithm>
#include <cmath>

SliceAnalysis::Result SliceAnalysis::analyse(const juce::File& file, const std::function<bool()>& shouldStop) const
{
    Result result;

    Frames frames;

    readMonoSamples(file, result);

    if (result.error.isNotEmpty()) {
        return result;
    }

    frames = analyseFrames(result.samples, shouldStop);

    if (shouldStop()) {
        result.error = stoppedError;
        return result;
    }

    collectSlices(frames, result);

    return result;
}

void SliceAnalysis::readMonoSamples(const juce::File& file, Result& result) const
{
    juce::AudioFormatManager formats;

    std::unique_ptr<juce::AudioFormatReader> reader;

    juce::AudioBuffer<float> buffer;

    juce::LagrangeInterpolator interpolator;

    formats.registerBasicFormats();

    reader.reset(formats.createReaderFor(file));

    if (reader == nullptr) {
        result.error = "Could not read " + file.getFullPathName();
        return;
    }

    const double speedRatio = reader->sampleRate / sampleRate;

    const int channelCount    = static_cast<int>(reader->numChannels);
    const int sourceLength    = static_cast<int>(reader->lengthInSamples);
    const int resampledLength = static_cast<int>(std::ceil(sourceLength / speedRatio));

    buffer.setSize(channelCount, sourceLength);
    reader->read(&buffer, 0, sourceLength, 0, true, true);

    for (int channel = 1; channel < channelCount; ++channel) {
        buffer.addFrom(0, 0, buffer, channel, 0, sourceLength);
    }

    buffer.applyGain(0, 0, sourceLength, 1.0f / static_cast<float>(channelCount));

    result.lengthInSamples = resampledLength;
    result.samples.assign(static_cast<size_t>(resampledLength), 0.0f);

    interpolator.process(speedRatio, buffer.getReadPointer(0), result.samples.data(), resampledLength, sourceLength, 0);
}

SliceAnalysis::Frames SliceAnalysis::analyseFrames(const std::vector<float>& samples, const std::function<bool()>& shouldStop) const
{
    const fluid::index sampleCount = static_cast<fluid::index>(samples.size());
    const fluid::index binCount    = fftSize / 2 + 1;
    const fluid::index frameCount  = (sampleCount + hopSize) / hopSize;
    const fluid::index metric      = static_cast<fluid::index>(onsetMetric);

    fluid::RealVector padded(sampleCount + windowSize + hopSize);
    fluid::RealVector magnitude(binCount);
    fluid::RealVector mels(bandCount);
    fluid::RealVector coefficients(coefficientCount);
    fluid::RealVector yinOutput(2);

    fluid::ComplexVector spectrum(binCount);

    fluid::algorithm::STFT stft(windowSize, fftSize, hopSize);

    fluid::algorithm::MelBands bands(bandCount, fftSize);

    fluid::algorithm::DCT dct(bandCount, coefficientCount);

    fluid::algorithm::OnsetSegmentation onsets(fftSize, maximumFilterSize);

    fluid::algorithm::YINFFT yin(binCount);

    Frames frames;

    std::fill(padded.begin(), padded.end(), 0.0);
    std::copy(samples.begin(), samples.end(), padded.begin() + windowSize / 2);

    frames.mfccs.reserve(static_cast<size_t>(frameCount * coefficientCount));
    frames.pitches.reserve(static_cast<size_t>(frameCount));
    frames.confidences.reserve(static_cast<size_t>(frameCount));

    bands.init(minimumFrequency, maximumFrequency, bandCount, binCount, sampleRate, windowSize);
    dct.init(bandCount, coefficientCount);
    onsets.init(windowSize, fftSize, filterSize);

    for (fluid::index frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        fluid::RealVectorView window = padded(fluid::Slice(frameIndex * hopSize, windowSize));

        if (shouldStop()) {
            return frames;
        }

        stft.processFrame(window, spectrum);
        fluid::algorithm::STFT::magnitude(spectrum, magnitude);
        bands.processFrame(magnitude, mels, false, false, true, fluid::FluidDefaultAllocator());
        dct.processFrame(mels, coefficients);
        yin.processFrame(magnitude, yinOutput, minimumFrequency, maximumFrequency, sampleRate, fluid::FluidDefaultAllocator());

        frames.mfccs.insert(frames.mfccs.end(), coefficients.begin(), coefficients.end());
        frames.pitches.push_back(yinOutput(0));
        frames.confidences.push_back(yinOutput(1));

        if (slicingMode == SlicingMode::Onset && onsets.processFrame(window, metric, filterSize, threshold, minimumSliceFrames, 0) > 0.0) {
            frames.onsetSamples.push_back(frameIndex * hopSize);
        }
    }

    return frames;
}

void SliceAnalysis::collectSlices(const Frames& frames, Result& result) const
{
    const juce::int64 frameTotal = static_cast<juce::int64>(frames.pitches.size());

    const size_t mfccWidth = static_cast<size_t>(coefficientCount);

    std::vector<juce::Range<juce::int64>> spans;

    if (slicingMode == SlicingMode::Granular) {
        const juce::int64 grainLengthSamples = std::max(static_cast<juce::int64>(1), static_cast<juce::int64>(grainSizeMilliseconds * sampleRate / 1000.0));
        const juce::int64 grainHopSamples    = std::max(static_cast<juce::int64>(1), static_cast<juce::int64>(grainHopMilliseconds * sampleRate / 1000.0));

        for (juce::int64 start = 0; start + grainLengthSamples <= result.lengthInSamples; start += grainHopSamples) {
            spans.emplace_back(start, start + grainLengthSamples);
        }
    } else {
        juce::int64 sliceStart = 0;

        for (const juce::int64 onset : frames.onsetSamples) {
            if (onset > sliceStart && onset < result.lengthInSamples) {
                spans.emplace_back(sliceStart, onset);
                sliceStart = onset;
            }
        }

        spans.emplace_back(sliceStart, result.lengthInSamples);
    }

    result.slices.reserve(spans.size());

    for (const juce::Range<juce::int64>& span : spans) {
        const juce::int64 firstFrame = std::min(span.getStart() / hopSize, frameTotal - 1);
        const juce::int64 endFrame   = std::clamp(span.getEnd() / hopSize, firstFrame + 1, frameTotal);

        const double frameCount    = static_cast<double>(endFrame - firstFrame);
        double       pitchSum      = 0.0;
        double       confidenceSum = 0.0;
        double       voicedWeight  = 0.0;

        Slice slice;

        slice.startSample = span.getStart();
        slice.endSample   = span.getEnd();
        slice.meanMfcc.assign(mfccWidth, 0.0);

        for (juce::int64 frame = firstFrame; frame < endFrame; ++frame) {
            const size_t frameIndex = static_cast<size_t>(frame);

            const double pitch      = frames.pitches[frameIndex];
            const double confidence = frames.confidences[frameIndex];

            confidenceSum += confidence;

            if (confidence > voicedConfidence && pitch > 0.0) {
                pitchSum     += pitch * confidence;
                voicedWeight += confidence;
            }

            for (size_t coefficient = 0; coefficient < mfccWidth; ++coefficient) {
                slice.meanMfcc[coefficient] += frames.mfccs[frameIndex * mfccWidth + coefficient];
            }
        }

        for (double& coefficient : slice.meanMfcc) {
            coefficient /= frameCount;
        }

        slice.pitchConfidence = confidenceSum / frameCount;

        if (voicedWeight > 0.0) {
            slice.pitch = pitchSum / voicedWeight;
        }

        result.slices.push_back(std::move(slice));
    }
}
