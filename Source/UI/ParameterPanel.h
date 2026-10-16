#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Menus/ItemSelector.h>
#include <ArsNovaUI/PanelResizer.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class ParameterPanel : public Bar, private juce::ValueTree::Listener
{
public:
    ParameterPanel (juce::ValueTree settings, juce::UndoManager& undoManager);

    void resized () override;

    PanelResizer resizer;

    static constexpr int defaultWidth = 260;
    static constexpr int minimumWidth = 220;
    static constexpr int maximumWidth = 480;
    static constexpr int labelWidth   = 120;
    static constexpr int sectionInset = 8;
    static constexpr int sectionGap   = 8;

private:
    void paintOverBar             (juce::Graphics& graphics) override;
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    juce::ValueTree                     settings;
    juce::UndoManager&                  undoManager;
    CaptionLabel                        analysisHeading;
    CaptionLabel                        slicingModeLabel;
    ItemSelector                        slicingModeSelector;
    LabeledEditor                       thresholdEditor;
    CaptionLabel                        metricLabel;
    ItemSelector                        metricSelector;
    LabeledEditor                       minimumSliceEditor;
    LabeledEditor                       coefficientEditor;
    LabeledEditor                       neighbourEditor;
    LabeledEditor                       minimumDistanceEditor;
    CaptionLabel                        liveMatchingHeading;
    CaptionLabel                        liveMatchingLabel;
    ItemSelector                        liveMatchingSelector;
    LabeledEditor                       gateThresholdEditor;
    LabeledEditor                       pitchWeightEditor;
    LabeledEditor                       candidateCountEditor;
    LabeledEditor                       repetitionPenaltyEditor;
    CaptionLabel                        playbackHeading;
    LabeledEditor                       fadeEditor;
    LabeledEditor                       lengthEditor;
    std::array<juce::Rectangle<int>, 3> sectionBounds;
};
