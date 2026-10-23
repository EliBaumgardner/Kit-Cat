#include "UI/CorpusBar.h"

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

CorpusBar::CorpusBar()
    : ArsNovaUI::Bar(Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio)
{
    ArsNovaUI::IconButton& saveButton = analysisPane.addButton(nullptr, "Save the slices and the map to a file", [this] { onSaveAnalysis(); });
    ArsNovaUI::IconButton& loadButton = analysisPane.addButton(nullptr, "Load slices and a map saved before", [this] { onLoadAnalysis(); });

    saveButton.setText("Save analysis");
    loadButton.setText("Load analysis");

    filePane.addButton(&ArsNovaUI::CustomLookAndFeel::drawImportIcon, "Load a sample or a folder", [this] { onLoad(); });
    filePane.addButton(&ArsNovaUI::CustomLookAndFeel::drawReloadIcon, "Analyse again with the current settings", [this] { onAnalyse(); });
    settingsPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawSettingsIcon, "Settings", [this] { onSettings(); });

    statusLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(filePane);
    addAndMakeVisible(analysisPane);
    addAndMakeVisible(settingsPane);
    addAndMakeVisible(statusLabel);
}

void CorpusBar::resized()
{
    const ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    juce::Rectangle<int> bounds = getContentBounds();

    const int paneWidth     = filePane.idealWidth(bounds.getHeight());
    const int settingsWidth = settingsPane.idealWidth(bounds.getHeight());

    statusLabel.setFont(lookAndFeel.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::textHeight));
    statusLabel.setColour(juce::Label::textColourId, lookAndFeel.softTextColour);

    filePane.setBounds(bounds.removeFromLeft(paneWidth));
    bounds.removeFromLeft(contentSpacing);
    settingsPane.setBounds(bounds.removeFromRight(settingsWidth));
    bounds.removeFromRight(contentSpacing);
    analysisPane.setBounds(bounds.removeFromRight(analysisPaneWidth));
    bounds.removeFromRight(contentSpacing);
    statusLabel.setBounds(bounds);
}
