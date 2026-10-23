#pragma once

#include "Analysis/CorpusAnalysis.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class CorpusMap : public juce::Component
{
public:
    CorpusMap();

    void paint (juce::Graphics& g) override;

    void mouseMove        (const juce::MouseEvent& event) override;
    void mouseDrag        (const juce::MouseEvent& event) override;
    void mouseExit        (const juce::MouseEvent& event) override;
    void mouseDown        (const juce::MouseEvent& event) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;
    void mouseWheelMove   (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify     (const juce::MouseEvent& event, float scaleFactor) override;

    void setLivePuck   (juce::Point<float> position, float rms, bool active);
    void flashSlice    (int sliceIndex);
    void updateFlashes ();

    enum class MapState
    {
        Playable,
        Frozen
    };

    std::shared_ptr<const CorpusAnalysis::Result> corpus;

    std::function<void(int sliceIndex)> onSlicePicked;

    MapState state = MapState::Playable;

    static constexpr float dotDiameter         = 6.0f;
    static constexpr float hoveredDotDiameter  = 11.0f;
    static constexpr float cursorDiameter      = 8.0f;
    static constexpr float cursorStrokeWidth   = 1.5f;
    static constexpr float gridSpacing         = 24.0f;
    static constexpr float margin              = 12.0f;
    static constexpr float pickRadius          = 20.0f;
    static constexpr float minimumZoom         = 0.5f;
    static constexpr float maximumZoom         = 40.0f;
    static constexpr float wheelZoomStep       = 0.15f;
    static constexpr float wheelScrollDistance = 100.0f;

    static constexpr int loadingSpinnerSize = 40;

private:
    int nearestSlice (juce::Point<float> position) const;

    std::optional<juce::Point<float>> cursorPosition;
    std::optional<juce::Point<float>> livePuckPosition;

    float livePuckRms = 0.0f;
    float zoom        = 1.0f;

    bool livePuckActive = false;

    std::vector<float> sliceFlashes;

    juce::AffineTransform viewTransform;

    int hoveredSlice = -1;
    int playedSlice  = -1;
};
