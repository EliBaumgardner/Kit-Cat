#include "Analysis/CorpusAnalysis.h"

#include <flucoma/algorithms/public/Normalization.hpp>
#include <flucoma/algorithms/public/Standardization.hpp>
#include <flucoma/data/FluidDataSet.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

CorpusAnalysis::Result CorpusAnalysis::analyse(const juce::File& fileOrFolder) const
{
    Result result;

    result.source     = fileOrFolder;
    result.sampleRate = slicing.sampleRate;

    if (! fileOrFolder.exists()) {
        result.error = fileOrFolder.getFileName() + " is no longer at " + fileOrFolder.getParentDirectory().getFullPathName() + ". Load it from where it is now.";
        return result;
    }

    result.files = findAudioFiles(fileOrFolder);

    if (result.files.isEmpty()) {
        result.error = "No audio files in " + fileOrFolder.getFullPathName();
        return result;
    }

    for (int fileIndex = 0; fileIndex < result.files.size(); ++fileIndex) {
        SliceAnalysis::Result fileResult = slicing.analyse(result.files[fileIndex], shouldStop);

        if (fileResult.error.isNotEmpty()) {
            result.error = fileResult.error;
            return result;
        }

        result.fileSamples.push_back(std::move(fileResult.samples));

        for (SliceAnalysis::Slice& sound : fileResult.slices) {
            result.slices.push_back({ fileIndex, std::move(sound), {} });
        }
    }

    if (static_cast<int>(result.slices.size()) < minimumSliceCount) {
        result.error = "Only " + juce::String(static_cast<int>(result.slices.size())) + " slices; the map needs at least " + juce::String(minimumSliceCount);
        return result;
    }

    layOutMap(result);

    if (result.error.isNotEmpty()) {
        return result;
    }

    buildIndex(result);

    return result;
}

CorpusAnalysis::Result CorpusAnalysis::load(const juce::File& analysisFile) const
{
    const juce::var               json    = juce::JSON::parse(analysisFile);
    const juce::Array<juce::var>* files   = json["files"].getArray();
    const juce::Array<juce::var>* slices  = json["slices"].getArray();
    const juce::String            damaged = analysisFile.getFileName() + " is damaged";
    SliceAnalysis                 reading = slicing;
    Result                        result;

    if (files == nullptr || slices == nullptr || static_cast<int>(json["version"]) != analysisFileVersion) {
        result.error = analysisFile.getFileName() + " is not a Cat-Kit analysis";
        return result;
    }

    reading.sampleRate = json["sampleRate"];
    result.sampleRate  = reading.sampleRate;
    result.source      = juce::File::createFileWithoutCheckingPath(json["source"].toString());

    if (! result.source.exists() && json.hasProperty("relativeSource")) {
        result.source = analysisFile.getParentDirectory().getChildFile(json["relativeSource"].toString());
    }

    for (const juce::var& file : *files) {
        juce::File            audioFile = juce::File::createFileWithoutCheckingPath(file["path"].toString());
        SliceAnalysis::Result fileResult;

        if (shouldStop()) {
            result.error = SliceAnalysis::stoppedError;
            return result;
        }

        if (! audioFile.existsAsFile() && file.hasProperty("relativePath")) {
            audioFile = analysisFile.getParentDirectory().getChildFile(file["relativePath"].toString());
        }

        reading.readMonoSamples(audioFile, fileResult);

        if (fileResult.error.isNotEmpty()) {
            result.error = fileResult.error;
            return result;
        }

        if (fileResult.lengthInSamples != static_cast<juce::int64>(file["length"])) {
            result.error = audioFile.getFileName() + " has changed since the analysis was saved";
            return result;
        }

        result.files.add(audioFile);
        result.fileSamples.push_back(std::move(fileResult.samples));
    }

    for (const juce::var& slice : *slices) {
        const juce::Array<juce::var>* mfcc = slice["mfcc"].getArray();
        Slice                         entry;

        entry.fileIndex         = slice["file"];
        entry.sound.startSample = slice["start"];
        entry.sound.endSample   = slice["end"];
        entry.position          = { static_cast<float>(slice["x"]), static_cast<float>(slice["y"]) };

        if (slice.hasProperty("pitch")) {
            entry.sound.pitch = slice["pitch"];
        }

        if (slice.hasProperty("pitchConfidence")) {
            entry.sound.pitchConfidence = slice["pitchConfidence"];
        }

        if (entry.fileIndex < 0 || entry.fileIndex >= result.files.size() || mfcc == nullptr || entry.sound.startSample < 0 || entry.sound.startSample >= entry.sound.endSample
            || entry.sound.endSample > static_cast<juce::int64>(result.fileSamples[static_cast<size_t>(entry.fileIndex)].size())) {
            result.error = damaged;
            return result;
        }

        for (const juce::var& coefficient : *mfcc) {
            entry.sound.meanMfcc.push_back(coefficient);
        }

        if (! result.slices.empty() && entry.sound.meanMfcc.size() != result.slices.front().sound.meanMfcc.size()) {
            result.error = damaged;
            return result;
        }

        result.slices.push_back(std::move(entry));
    }

    buildIndex(result);

    return result;
}

void CorpusAnalysis::buildIndex(Result& result) const
{
    const size_t        sliceCount       = result.slices.size();
    size_t              coefficientCount = 0;
    std::vector<size_t> order(sliceCount);

    if (sliceCount > 0) {
        coefficientCount = result.slices.front().sound.meanMfcc.size();
    }

    if (coefficientCount <= 1) {
        return;
    }

    result.mfccMean.assign(coefficientCount, 0.0);
    result.mfccStdDev.assign(coefficientCount, 0.0);
    result.pitchNotes.assign(sliceCount, 0.0);
    result.standardizedMfccs.assign(sliceCount * coefficientCount, 0.0);
    result.kdTree.clear();

    for (size_t slice = 0; slice < sliceCount; ++slice) {
        const SliceAnalysis::Slice& sound = result.slices[slice].sound;

        if (sound.pitch > slicing.minimumFrequency) {
            result.pitchNotes[slice] = 69.0 + 12.0 * std::log2(sound.pitch / 440.0);
        }

        for (size_t coefficient = 0; coefficient < coefficientCount; ++coefficient) {
            result.mfccMean[coefficient] += sound.meanMfcc[coefficient];
        }
    }

    for (double& mean : result.mfccMean) {
        mean /= static_cast<double>(sliceCount);
    }

    for (const Slice& slice : result.slices) {
        for (size_t coefficient = 0; coefficient < coefficientCount; ++coefficient) {
            const double deviation = slice.sound.meanMfcc[coefficient] - result.mfccMean[coefficient];

            result.mfccStdDev[coefficient] += deviation * deviation;
        }
    }

    for (double& deviation : result.mfccStdDev) {
        deviation = std::sqrt(deviation / static_cast<double>(sliceCount));

        if (deviation < 1e-6) {
            deviation = 1.0;
        }
    }

    for (size_t slice = 0; slice < sliceCount; ++slice) {
        for (size_t coefficient = 0; coefficient < coefficientCount; ++coefficient) {
            const double value = result.slices[slice].sound.meanMfcc[coefficient];

            result.standardizedMfccs[slice * coefficientCount + coefficient] = (value - result.mfccMean[coefficient]) / result.mfccStdDev[coefficient];
        }
    }

    std::iota(order.begin(), order.end(), size_t { 0 });

    auto buildSubTree = [&](auto& self, size_t start, size_t end, size_t depth) -> int {
        if (start >= end) {
            return -1;
        }

        const size_t splitDim  = 1 + depth % (coefficientCount - 1);
        const size_t middle    = start + (end - start) / 2;
        const size_t nodeIndex = result.kdTree.size();

        std::nth_element(order.begin() + start, order.begin() + middle, order.begin() + end, [&](size_t a, size_t b) {
            return result.standardizedMfccs[a * coefficientCount + splitDim] < result.standardizedMfccs[b * coefficientCount + splitDim];
        });

        const size_t sliceIndex = order[middle];
        const double splitValue = result.standardizedMfccs[sliceIndex * coefficientCount + splitDim];

        result.kdTree.push_back({ static_cast<int>(sliceIndex), -1, -1, static_cast<int>(splitDim), splitValue });

        const int left  = self(self, start, middle, depth + 1);
        const int right = self(self, middle + 1, end, depth + 1);

        result.kdTree[nodeIndex].leftChild  = left;
        result.kdTree[nodeIndex].rightChild = right;

        return static_cast<int>(nodeIndex);
    };

    result.kdTree.reserve(sliceCount);
    buildSubTree(buildSubTree, 0, sliceCount, 0);
}

juce::Array<juce::File> CorpusAnalysis::findAudioFiles(const juce::File& fileOrFolder)
{
    juce::Array<juce::File> files;

    if (fileOrFolder.existsAsFile()) {
        files.add(fileOrFolder);
        return files;
    }

    files = fileOrFolder.findChildFiles(juce::File::findFiles, true, audioFilePattern);

    files.sort();

    return files;
}

juce::String CorpusAnalysis::describe(const Result& result)
{
    if (result.error.isNotEmpty()) {
        return result.error;
    }

    return juce::String(static_cast<int>(result.slices.size())) + " slices from " + juce::String(result.files.size()) + " files at " + juce::String(result.sampleRate) + " Hz";
}

juce::String CorpusAnalysis::summarise(const Result& result)
{
    juce::String text = describe(result) + "\n";

    if (result.error.isNotEmpty()) {
        return text;
    }

    for (const Slice& slice : result.slices) {
        const double startSeconds = static_cast<double>(slice.sound.startSample) / result.sampleRate;

        text << "  " << result.files[slice.fileIndex].getFileName() << " " << juce::String(startSeconds, 3) << " s  map " << juce::String(slice.position.x, 3) << " " << juce::String(slice.position.y, 3) << "\n";
    }

    return text;
}

juce::String CorpusAnalysis::save(const Result& result, const juce::File& analysisFile)
{
    juce::DynamicObject::Ptr json = new juce::DynamicObject();
    juce::Array<juce::var>   files;
    juce::Array<juce::var>   slices;

    for (int fileIndex = 0; fileIndex < result.files.size(); ++fileIndex) {
        juce::DynamicObject::Ptr file = new juce::DynamicObject();

        file->setProperty("path", result.files[fileIndex].getFullPathName());
        file->setProperty("relativePath", result.files[fileIndex].getRelativePathFrom(analysisFile.getParentDirectory()));
        file->setProperty("length", static_cast<juce::int64>(result.fileSamples[static_cast<size_t>(fileIndex)].size()));

        files.add(file.get());
    }

    for (const Slice& slice : result.slices) {
        juce::DynamicObject::Ptr entry = new juce::DynamicObject();
        juce::Array<juce::var>   mfcc;

        for (const double coefficient : slice.sound.meanMfcc) {
            mfcc.add(coefficient);
        }

        entry->setProperty("file", slice.fileIndex);
        entry->setProperty("start", slice.sound.startSample);
        entry->setProperty("end", slice.sound.endSample);
        entry->setProperty("x", slice.position.x);
        entry->setProperty("y", slice.position.y);
        entry->setProperty("pitch", slice.sound.pitch);
        entry->setProperty("pitchConfidence", slice.sound.pitchConfidence);
        entry->setProperty("mfcc", mfcc);

        slices.add(entry.get());
    }

    json->setProperty("version", analysisFileVersion);
    json->setProperty("sampleRate", result.sampleRate);
    json->setProperty("source", result.source.getFullPathName());
    json->setProperty("relativeSource", result.source.getRelativePathFrom(analysisFile.getParentDirectory()));
    json->setProperty("files", files);
    json->setProperty("slices", slices);

    if (! analysisFile.replaceWithText(juce::JSON::toString(juce::var(json.get()), false, std::numeric_limits<double>::max_digits10))) {
        return "Could not write " + analysisFile.getFullPathName();
    }

    return {};
}

void CorpusAnalysis::layOutMap(Result& result) const
{
    const fluid::index sliceCount       = static_cast<fluid::index>(result.slices.size());
    const fluid::index coefficientCount = slicing.coefficientCount;

    fluid::FluidTensor<std::string, 1> ids(sliceCount);
    fluid::RealMatrix                  descriptors(sliceCount, coefficientCount);
    fluid::RealMatrix                  scaled(sliceCount, coefficientCount);
    fluid::algorithm::Standardization  standardization;
    MapLayout::DataSet                 scaledSet;

    fluid::RealMatrix               embedding;
    fluid::RealMatrix               positions(sliceCount, 2);
    fluid::algorithm::Normalization normalization;

    for (fluid::index slice = 0; slice < sliceCount; ++slice) {
        ids(slice) = std::to_string(slice);

        for (fluid::index coefficient = 0; coefficient < coefficientCount; ++coefficient) {
            descriptors(slice, coefficient) = result.slices[static_cast<size_t>(slice)].sound.meanMfcc[static_cast<size_t>(coefficient)];
        }
    }

    standardization.init(descriptors);
    standardization.process(descriptors, scaled);

    scaledSet = MapLayout::DataSet(ids, scaled);
    embedding = layout.train(scaledSet, shouldStop);

    if (shouldStop()) {
        result.error = SliceAnalysis::stoppedError;
        return;
    }

    normalization.init(0.0, 1.0, embedding);
    normalization.process(embedding, positions);

    for (fluid::index slice = 0; slice < sliceCount; ++slice) {
        result.slices[static_cast<size_t>(slice)].position = { static_cast<float>(positions(slice, 0)), static_cast<float>(positions(slice, 1)) };
    }
}
