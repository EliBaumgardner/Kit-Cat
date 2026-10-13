#pragma once

#include "Analysis/SliceAnalysis.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class CatKitAudioProcessor;

class CatKitAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CatKitAudioProcessorEditor(CatKitAudioProcessor& audioProcessor);

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    void chooseFile  ();
    void analyseFile (const juce::File& file);

    SliceAnalysis                      analysis;
    juce::TextButton                   loadButton;
    juce::Label                        resultLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::ThreadPool                   analysisPool;
};
