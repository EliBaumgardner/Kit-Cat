#include "UI/ParameterPanel.h"
#include "Plugin/StateIdentifiers.h"

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <array>
#include <vector>

ParameterPanel::ParameterPanel(juce::ValueTree settings, juce::UndoManager& undoManager)
    : ArsNovaUI::Bar(Orientation::Vertical, ArsNovaUI::Theme::contentInsetRatio),
      resizer(ArsNovaUI::PanelResizer::Edge::Left),
      settings(settings),
      undoManager(undoManager),
      slicingModeSelector(undoManager),
      metricSelector(undoManager),
      liveMatchingSelector(undoManager),
      thresholdEditor(undoManager),
      minimumSliceEditor(undoManager),
      coefficientEditor(undoManager),
      neighbourEditor(undoManager),
      minimumDistanceEditor(undoManager),
      gateThresholdEditor(undoManager),
      pitchWeightEditor(undoManager),
      candidateCountEditor(undoManager),
      repetitionPenaltyEditor(undoManager),
      fadeEditor(undoManager),
      lengthEditor(undoManager)
{
    const juce::StringArray metricNames { "Energy", "High frequency content", "Spectral flux", "Modified Kullback-Leibler", "Itakura-Saito", "Cosine",
                                          "Phase deviation", "Weighted phase deviation", "Complex deviation", "Rectified complex deviation" };

    std::unique_ptr<ArsNovaUI::NumberFormat> fadeFormat   = std::make_unique<ArsNovaUI::NumberFormat>(1.0, 100.0);
    std::unique_ptr<ArsNovaUI::NumberFormat> lengthFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, 10000.0);
    std::unique_ptr<ArsNovaUI::NumberFormat> gateFormat   = std::make_unique<ArsNovaUI::NumberFormat>(-80.0, 0.0, 1);

    fadeFormat->suffix   = " ms";
    lengthFormat->suffix = " ms";
    gateFormat->suffix   = " dB";

    analysisHeading.setText("Analysis", juce::dontSendNotification);
    slicingModeLabel.setText("Slicing mode", juce::dontSendNotification);
    thresholdEditor.label.setText("Onset threshold", juce::dontSendNotification);
    metricLabel.setText("Onset metric", juce::dontSendNotification);
    minimumSliceEditor.label.setText("Shortest slice (hops)", juce::dontSendNotification);
    coefficientEditor.label.setText("MFCC coefficients", juce::dontSendNotification);
    neighbourEditor.label.setText("Map neighbours", juce::dontSendNotification);
    minimumDistanceEditor.label.setText("Map minimum distance", juce::dontSendNotification);

    liveMatchingHeading.setText("Live input matching", juce::dontSendNotification);
    liveMatchingLabel.setText("Match live input", juce::dontSendNotification);
    gateThresholdEditor.label.setText("Input gate", juce::dontSendNotification);
    pitchWeightEditor.label.setText("Pitch weight", juce::dontSendNotification);
    candidateCountEditor.label.setText("Variation (Top-K)", juce::dontSendNotification);
    repetitionPenaltyEditor.label.setText("Repetition penalty", juce::dontSendNotification);

    playbackHeading.setText("Playback", juce::dontSendNotification);
    fadeEditor.label.setText("Crossfade", juce::dontSendNotification);
    lengthEditor.label.setText("Slice length", juce::dontSendNotification);

    thresholdEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, 10.0, 2));
    minimumSliceEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(1.0, 999.0));
    coefficientEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(2.0, 40.0));
    neighbourEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(2.0, 99.0));
    minimumDistanceEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, 2));

    gateThresholdEditor.editor.setFormat(std::move(gateFormat));
    pitchWeightEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, 2));
    candidateCountEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(1.0, 8.0));
    repetitionPenaltyEditor.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, 2));

    fadeEditor.editor.setFormat(std::move(fadeFormat));
    lengthEditor.editor.setFormat(std::move(lengthFormat));

    thresholdEditor.editor.bindEditor(settings, StateIdentifiers::OnsetThreshold);
    minimumSliceEditor.editor.bindEditor(settings, StateIdentifiers::MinimumSliceFrames);
    coefficientEditor.editor.bindEditor(settings, StateIdentifiers::CoefficientCount);
    neighbourEditor.editor.bindEditor(settings, StateIdentifiers::NeighbourCount);
    minimumDistanceEditor.editor.bindEditor(settings, StateIdentifiers::MinimumDistance);

    gateThresholdEditor.editor.bindEditor(settings, StateIdentifiers::InputGateThreshold);
    pitchWeightEditor.editor.bindEditor(settings, StateIdentifiers::PitchWeight);
    candidateCountEditor.editor.bindEditor(settings, StateIdentifiers::CandidateCount);
    repetitionPenaltyEditor.editor.bindEditor(settings, StateIdentifiers::RepetitionPenalty);

    fadeEditor.editor.bindEditor(settings, StateIdentifiers::FadeMilliseconds);
    lengthEditor.editor.bindEditor(settings, StateIdentifiers::LengthMilliseconds);

    slicingModeSelector.addItem(1, "Onset");
    slicingModeSelector.addItem(2, "Granular");
    slicingModeSelector.setSelectedItem(static_cast<int>(settings[StateIdentifiers::SlicingMode]) + 1);
    slicingModeSelector.onItemSelected = [this](int itemId) { this->settings.setProperty(StateIdentifiers::SlicingMode, itemId - 1, &this->undoManager); };

    for (int metricIndex = 0; metricIndex < metricNames.size(); ++metricIndex) {
        metricSelector.addItem(metricIndex + 1, metricNames[metricIndex]);
    }

    metricSelector.setSelectedItem(static_cast<int>(settings[StateIdentifiers::OnsetMetric]) + 1);
    metricSelector.setTooltip("How onsets are detected");
    metricSelector.onItemSelected = [this](int itemId) { this->settings.setProperty(StateIdentifiers::OnsetMetric, itemId - 1, &this->undoManager); };

    liveMatchingSelector.addItem(1, "Off");
    liveMatchingSelector.addItem(2, "On");
    liveMatchingSelector.setSelectedItem(static_cast<int>(static_cast<bool>(settings[StateIdentifiers::LiveMatchingEnabled])) + 1);
    liveMatchingSelector.onItemSelected = [this](int itemId) { this->settings.setProperty(StateIdentifiers::LiveMatchingEnabled, itemId == 2, &this->undoManager); };

    this->settings.addListener(this);

    addAndMakeVisible(analysisHeading);
    addAndMakeVisible(slicingModeLabel);
    addAndMakeVisible(slicingModeSelector);
    addAndMakeVisible(thresholdEditor);
    addAndMakeVisible(metricLabel);
    addAndMakeVisible(metricSelector);
    addAndMakeVisible(minimumSliceEditor);
    addAndMakeVisible(coefficientEditor);
    addAndMakeVisible(neighbourEditor);
    addAndMakeVisible(minimumDistanceEditor);

    addAndMakeVisible(liveMatchingHeading);
    addAndMakeVisible(liveMatchingLabel);
    addAndMakeVisible(liveMatchingSelector);
    addAndMakeVisible(gateThresholdEditor);
    addAndMakeVisible(pitchWeightEditor);
    addAndMakeVisible(candidateCountEditor);
    addAndMakeVisible(repetitionPenaltyEditor);

    addAndMakeVisible(playbackHeading);
    addAndMakeVisible(fadeEditor);
    addAndMakeVisible(lengthEditor);

    addAndMakeVisible(resizer);
}

void ParameterPanel::resized()
{
    const ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Font font        = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::textHeight);
    const juce::Font headingFont = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::SemiBold, ArsNovaUI::Theme::textHeight);

    const int rowHeight = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuRowHeightRatio);
    const int spacing   = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuSpacingRatio);

    const std::array<ArsNovaUI::CaptionLabel*, 3> headings { &analysisHeading, &liveMatchingHeading, &playbackHeading };
    const std::array<ArsNovaUI::CaptionLabel*, 3> captions { &slicingModeLabel, &metricLabel, &liveMatchingLabel };

    const std::array<ArsNovaUI::LabeledEditor*, 11> editors { &thresholdEditor, &minimumSliceEditor, &coefficientEditor, &neighbourEditor, &minimumDistanceEditor,
                                                              &gateThresholdEditor, &pitchWeightEditor, &candidateCountEditor, &repetitionPenaltyEditor,
                                                              &fadeEditor, &lengthEditor };

    const std::array<std::vector<juce::Component*>, 3> sectionRows { std::vector<juce::Component*> { &slicingModeSelector, &thresholdEditor, &metricSelector, &minimumSliceEditor,
                                                                                                       &coefficientEditor, &neighbourEditor, &minimumDistanceEditor },
                                                                     std::vector<juce::Component*> { &liveMatchingSelector, &gateThresholdEditor, &pitchWeightEditor,
                                                                                                       &candidateCountEditor, &repetitionPenaltyEditor },
                                                                     std::vector<juce::Component*> { &fadeEditor, &lengthEditor } };

    juce::Rectangle<int> bounds = getLocalBounds().reduced(ArsNovaUI::Theme::windowGutter);

    resizer.setBounds(getLocalBounds().removeFromLeft(ArsNovaUI::Theme::windowGutter));

    for (ArsNovaUI::CaptionLabel* heading : headings) {
        heading->setFont(headingFont);
        heading->setColour(juce::Label::textColourId, lookAndFeel.textColour);
    }

    for (ArsNovaUI::CaptionLabel* caption : captions) {
        caption->setFont(font);
        caption->setColour(juce::Label::textColourId, lookAndFeel.softTextColour);
    }

    for (ArsNovaUI::LabeledEditor* editor : editors) {
        editor->label.setFont(font);
        editor->label.setColour(juce::Label::textColourId, lookAndFeel.softTextColour);
        editor->editor.setFontHeight(ArsNovaUI::Theme::textHeight);
        editor->labelWidth = labelWidth;
        editor->gap        = spacing;
    }

    for (size_t section = 0; section < sectionRows.size(); ++section) {
        const int rowCount = static_cast<int>(sectionRows[section].size()) + 1;
        const int height   = rowCount * rowHeight + (rowCount - 1) * spacing + sectionInset * 2;

        juce::Rectangle<int> rowArea = bounds.removeFromTop(height).reduced(sectionInset);

        sectionBounds[section] = rowArea.expanded(sectionInset);

        headings[section]->setBounds(rowArea.removeFromTop(rowHeight));

        for (juce::Component* row : sectionRows[section]) {
            rowArea.removeFromTop(spacing);
            row->setBounds(rowArea.removeFromTop(rowHeight));
        }

        bounds.removeFromTop(sectionGap);
    }

    slicingModeLabel.setBounds(slicingModeSelector.getBounds().withWidth(labelWidth));
    slicingModeSelector.setBounds(slicingModeSelector.getBounds().withTrimmedLeft(labelWidth + spacing));

    metricLabel.setBounds(metricSelector.getBounds().withWidth(labelWidth));
    metricSelector.setBounds(metricSelector.getBounds().withTrimmedLeft(labelWidth + spacing));

    liveMatchingLabel.setBounds(liveMatchingSelector.getBounds().withWidth(labelWidth));
    liveMatchingSelector.setBounds(liveMatchingSelector.getBounds().withTrimmedLeft(labelWidth + spacing));
}

void ParameterPanel::paintOverBar(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    for (const juce::Rectangle<int>& section : sectionBounds) {
        lookAndFeel.drawPane(graphics, section.toFloat());
    }
}

void ParameterPanel::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == StateIdentifiers::OnsetMetric) {
        metricSelector.setSelectedItem(static_cast<int>(tree[property]) + 1);
    }

    if (property == StateIdentifiers::SlicingMode) {
        slicingModeSelector.setSelectedItem(static_cast<int>(tree[property]) + 1);
    }

    if (property == StateIdentifiers::LiveMatchingEnabled) {
        liveMatchingSelector.setSelectedItem(static_cast<int>(static_cast<bool>(tree[property])) + 1);
    }
}
