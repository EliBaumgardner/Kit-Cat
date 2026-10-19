#include "Map/CorpusMap.h"

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <algorithm>
#include <cmath>

CorpusMap::CorpusMap()
{
    setMouseCursor(juce::MouseCursor::NoCursor);
}

void CorpusMap::paint(juce::Graphics& g)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);
    const juce::Rectangle<float>  area        = getLocalBounds().toFloat().reduced(margin);
    const juce::AffineTransform   mapToScreen = juce::AffineTransform::scale(area.getWidth(), -area.getHeight()).translated(area.getX(), area.getBottom()).followedBy(viewTransform);
    ArsNovaUI::CanvasGrid         grid;

    grid.bounds         = getLocalBounds().toFloat();
    grid.modelTransform = viewTransform.inverted();
    grid.viewTransform  = viewTransform;
    grid.spacing        = gridSpacing;
    grid.visible        = true;

    lookAndFeel.drawCanvas(g, grid);

    if (corpus != nullptr) {
        for (size_t sliceIndex = 0; sliceIndex < corpus->slices.size(); ++sliceIndex) {
            const CorpusAnalysis::Slice& slice            = corpus->slices[sliceIndex];
            const juce::Point<float>     centre           = slice.position.transformedBy(mapToScreen);
            const juce::Point<float>     offsetFromMiddle = slice.position - juce::Point<float>(0.5f, 0.5f);
            const float                  hue              = (std::atan2(offsetFromMiddle.y, offsetFromMiddle.x) + juce::MathConstants<float>::pi) / juce::MathConstants<float>::twoPi;
            const float                  saturation       = 0.25f + 0.65f * std::min(1.0f, offsetFromMiddle.getDistanceFromOrigin() * 2.0f);
            juce::Colour                 colour           = juce::Colour::fromHSV(hue, saturation, 0.95f, 1.0f);
            float                        diameter         = dotDiameter;

            if (state == MapState::Frozen) {
                colour = colour.withSaturation(0.0f).withMultipliedAlpha(0.4f);
            }

            if (state == MapState::Playable && static_cast<int>(sliceIndex) == hoveredSlice) {
                diameter = hoveredDotDiameter;
            }

            g.setColour(colour);
            g.fillEllipse(juce::Rectangle<float>(diameter, diameter).withCentre(centre));

            if (sliceIndex < sliceFlashes.size() && sliceFlashes[sliceIndex] > 0.01f) {
                const float flash = sliceFlashes[sliceIndex];
                const float halo  = diameter + flash * 14.0f;

                g.setColour(lookAndFeel.textColour.withAlpha(flash * 0.8f));
                g.drawEllipse(juce::Rectangle<float>(halo, halo).withCentre(centre), 1.5f);
            }
        }
    }

    if (state == MapState::Frozen) {
        const juce::Rectangle<int> spinner = getLocalBounds().withSizeKeepingCentre(loadingSpinnerSize, loadingSpinnerSize);

        lookAndFeel.drawSpinningWaitAnimation(g, lookAndFeel.accentColour, spinner.getX(), spinner.getY(), spinner.getWidth(), spinner.getHeight());
    }

    if (livePuckActive && livePuckPosition.has_value()) {
        const juce::Point<float> puckCentre   = livePuckPosition->transformedBy(mapToScreen);
        const float              haloDiameter = 14.0f + std::clamp(livePuckRms * 60.0f, 0.0f, 30.0f);

        g.setColour(lookAndFeel.accentColour.withAlpha(0.25f + 0.4f * std::clamp(livePuckRms * 4.0f, 0.0f, 1.0f)));
        g.fillEllipse(juce::Rectangle<float>(haloDiameter, haloDiameter).withCentre(puckCentre));

        g.setColour(lookAndFeel.accentColour);
        g.fillEllipse(juce::Rectangle<float>(cursorDiameter, cursorDiameter).withCentre(puckCentre));

        g.setColour(lookAndFeel.onAccentColour);
        g.drawEllipse(juce::Rectangle<float>(cursorDiameter, cursorDiameter).withCentre(puckCentre), 1.2f);
    }

    if (cursorPosition.has_value()) {
        g.setColour(lookAndFeel.textColour);
        g.drawEllipse(juce::Rectangle<float>(cursorDiameter, cursorDiameter).withCentre(*cursorPosition), cursorStrokeWidth);
    }
}

void CorpusMap::mouseMove(const juce::MouseEvent& event)
{
    cursorPosition = event.position;
    hoveredSlice   = nearestSlice(event.position);

    repaint();
}

void CorpusMap::mouseDrag(const juce::MouseEvent& event)
{
    cursorPosition = event.position;
    hoveredSlice   = nearestSlice(event.position);

    if (hoveredSlice >= 0 && hoveredSlice != playedSlice && onSlicePicked != nullptr) {
        onSlicePicked(hoveredSlice);
    }

    playedSlice = hoveredSlice;

    repaint();
}

void CorpusMap::mouseExit(const juce::MouseEvent& event)
{
    cursorPosition.reset();
    hoveredSlice = -1;

    repaint();
}

void CorpusMap::mouseDown(const juce::MouseEvent& event)
{
    playedSlice = nearestSlice(event.position);

    if (playedSlice >= 0 && onSlicePicked != nullptr) {
        onSlicePicked(playedSlice);
    }
}

void CorpusMap::mouseDoubleClick(const juce::MouseEvent& event)
{
    viewTransform = juce::AffineTransform();
    zoom          = 1.0f;
    hoveredSlice  = nearestSlice(event.position);

    repaint();
}

void CorpusMap::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    float scrollX = wheel.deltaX;
    float scrollY = wheel.deltaY;

    if (wheel.isReversed) {
        scrollX = -wheel.deltaX;
        scrollY = -wheel.deltaY;
    }

    if (event.mods.isShiftDown()) {
        const float newZoom = std::clamp(zoom * (1.0f + scrollY * wheelZoomStep), minimumZoom, maximumZoom);

        viewTransform = viewTransform.followedBy(juce::AffineTransform::scale(newZoom / zoom, newZoom / zoom, event.position.x, event.position.y));
        zoom          = newZoom;
    }
    else {
        viewTransform = viewTransform.translated(-scrollX * wheelScrollDistance, -scrollY * wheelScrollDistance);
    }

    hoveredSlice = nearestSlice(event.position);

    repaint();
}

void CorpusMap::mouseMagnify(const juce::MouseEvent& event, float scaleFactor)
{
    const float newZoom = std::clamp(zoom * scaleFactor, minimumZoom, maximumZoom);

    viewTransform = viewTransform.followedBy(juce::AffineTransform::scale(newZoom / zoom, newZoom / zoom, event.position.x, event.position.y));
    zoom          = newZoom;
    hoveredSlice  = nearestSlice(event.position);

    repaint();
}

void CorpusMap::setLivePuck(juce::Point<float> position, float rms, bool active)
{
    livePuckPosition = position;
    livePuckRms      = rms;
    livePuckActive   = active;
}

void CorpusMap::flashSlice(int sliceIndex)
{
    if (corpus == nullptr || sliceIndex < 0 || sliceIndex >= static_cast<int>(corpus->slices.size())) {
        return;
    }

    if (sliceFlashes.size() != corpus->slices.size()) {
        sliceFlashes.assign(corpus->slices.size(), 0.0f);
    }

    sliceFlashes[static_cast<size_t>(sliceIndex)] = 1.0f;
}

void CorpusMap::updateFlashes()
{
    bool needsRepaint = livePuckActive || state == MapState::Frozen;

    for (float& flash : sliceFlashes) {
        if (flash > 0.01f) {
            flash        *= 0.82f;
            needsRepaint  = true;

            if (flash <= 0.01f) {
                flash = 0.0f;
            }
        }
    }

    if (needsRepaint) {
        repaint();
    }
}

int CorpusMap::nearestSlice(juce::Point<float> position) const
{
    const juce::Rectangle<float> area            = getLocalBounds().toFloat().reduced(margin);
    const juce::AffineTransform  mapToScreen     = juce::AffineTransform::scale(area.getWidth(), -area.getHeight()).translated(area.getX(), area.getBottom()).followedBy(viewTransform);
    int                          nearest         = -1;
    float                        nearestDistance = pickRadius;

    if (corpus == nullptr || state == MapState::Frozen) {
        return nearest;
    }

    for (size_t sliceIndex = 0; sliceIndex < corpus->slices.size(); ++sliceIndex) {
        const float distance = position.getDistanceFrom(corpus->slices[sliceIndex].position.transformedBy(mapToScreen));

        if (distance < nearestDistance) {
            nearest         = static_cast<int>(sliceIndex);
            nearestDistance = distance;
        }
    }

    return nearest;
}
