#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Menus/ItemSelector.h>
#include <ArsNovaUI/PanelResizer.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class ParameterPanel : public ArsNovaUI::Bar, private juce::ValueTree::Listener
{
public:
    ParameterPanel (juce::ValueTree settings, juce::UndoManager& undoManager);

    void resized () override;

    ArsNovaUI::PanelResizer resizer;

    static constexpr int defaultWidth = 260;
    static constexpr int minimumWidth = 220;
    static constexpr int maximumWidth = 480;
    static constexpr int labelWidth   = 120;
    static constexpr int sectionInset = 8;
    static constexpr int sectionGap   = 8;

private:
    void paintOverBar             (juce::Graphics& graphics) override;
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    juce::ValueTree settings;

    juce::UndoManager& undoManager;

    ArsNovaUI::CaptionLabel analysisHeading;
    ArsNovaUI::CaptionLabel slicingModeLabel;
    ArsNovaUI::CaptionLabel metricLabel;
    ArsNovaUI::CaptionLabel liveMatchingHeading;
    ArsNovaUI::CaptionLabel liveMatchingLabel;
    ArsNovaUI::CaptionLabel playbackHeading;

    ArsNovaUI::ItemSelector slicingModeSelector;
    ArsNovaUI::ItemSelector metricSelector;
    ArsNovaUI::ItemSelector liveMatchingSelector;

    ArsNovaUI::LabeledEditor thresholdEditor;
    ArsNovaUI::LabeledEditor minimumSliceEditor;
    ArsNovaUI::LabeledEditor coefficientEditor;
    ArsNovaUI::LabeledEditor neighbourEditor;
    ArsNovaUI::LabeledEditor minimumDistanceEditor;
    ArsNovaUI::LabeledEditor gateThresholdEditor;
    ArsNovaUI::LabeledEditor pitchWeightEditor;
    ArsNovaUI::LabeledEditor candidateCountEditor;
    ArsNovaUI::LabeledEditor repetitionPenaltyEditor;
    ArsNovaUI::LabeledEditor fadeEditor;
    ArsNovaUI::LabeledEditor lengthEditor;

    std::array<juce::Rectangle<int>, 3> sectionBounds;
};
