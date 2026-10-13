#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"

CatKitAudioProcessorEditor::CatKitAudioProcessorEditor(CatKitAudioProcessor& audioProcessor)
    : AudioProcessorEditor(audioProcessor),
      analysisPool(1)
{
    loadButton.setButtonText("Load sample");
    loadButton.onClick = [this] { chooseFile(); };

    resultLabel.setText("Load a sample to slice it and describe each slice.", juce::dontSendNotification);
    resultLabel.setJustificationType(juce::Justification::topLeft);
    resultLabel.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

    addAndMakeVisible(loadButton);
    addAndMakeVisible(resultLabel);

    setSize(520, 320);
}

void CatKitAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CatKitAudioProcessorEditor::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds().reduced(12);

    loadButton.setBounds(bounds.removeFromTop(28).removeFromLeft(140));
    bounds.removeFromTop(8);
    resultLabel.setBounds(bounds);
}

void CatKitAudioProcessorEditor::chooseFile()
{
    const int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Choose a sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3");

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) { analyseFile(chooser.getResult()); });
}

void CatKitAudioProcessorEditor::analyseFile(const juce::File& file)
{
    juce::Component::SafePointer<CatKitAudioProcessorEditor> editor(this);

    if (file == juce::File()) {
        return;
    }

    resultLabel.setText("Analysing " + file.getFileName() + "...", juce::dontSendNotification);

    analysisPool.addJob([editor, file, settings = analysis] {
        const juce::String summary = SliceAnalysis::summarise(settings.analyse(file));

        juce::MessageManager::callAsync([editor, summary] {
            if (editor != nullptr) {
                editor->resultLabel.setText(summary, juce::dontSendNotification);
            }
        });
    });
}
