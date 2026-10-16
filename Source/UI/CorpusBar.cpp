#include "UI/CorpusBar.h"

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

CorpusBar::CorpusBar()
    : Bar(Orientation::Horizontal, Theme::contentInsetRatio)
{
    IconButton& saveButton = analysisPane.addButton(nullptr, "Save the slices and the map to a file", [this] { onSaveAnalysis(); });
    IconButton& loadButton = analysisPane.addButton(nullptr, "Load slices and a map saved before", [this] { onLoadAnalysis(); });

    saveButton.setText("Save analysis");
    loadButton.setText("Load analysis");

    filePane.addButton(&CustomLookAndFeel::drawImportIcon, "Load a sample or a folder", [this] { onLoad(); });
    filePane.addButton(&CustomLookAndFeel::drawReloadIcon, "Analyse again with the current settings", [this] { onAnalyse(); });
    settingsPane.addButton(&CustomLookAndFeel::drawSettingsIcon, "Settings", [this] { onSettings(); });

    statusLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(filePane);
    addAndMakeVisible(analysisPane);
    addAndMakeVisible(settingsPane);
    addAndMakeVisible(statusLabel);
}

void CorpusBar::resized()
{
    const CustomLookAndFeel& lookAndFeel   = CustomLookAndFeel::get(*this);
    juce::Rectangle<int>     bounds        = getContentBounds();
    const int                paneWidth     = filePane.idealWidth(bounds.getHeight());
    const int                settingsWidth = settingsPane.idealWidth(bounds.getHeight());

    statusLabel.setFont(lookAndFeel.font(Theme::FontStyle::Regular, Theme::textHeight));
    statusLabel.setColour(juce::Label::textColourId, lookAndFeel.softTextColour);

    filePane.setBounds(bounds.removeFromLeft(paneWidth));
    bounds.removeFromLeft(contentSpacing);
    settingsPane.setBounds(bounds.removeFromRight(settingsWidth));
    bounds.removeFromRight(contentSpacing);
    analysisPane.setBounds(bounds.removeFromRight(analysisPaneWidth));
    bounds.removeFromRight(contentSpacing);
    statusLabel.setBounds(bounds);
}
