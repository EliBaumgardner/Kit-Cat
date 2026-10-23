#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class CorpusBar : public ArsNovaUI::Bar
{
public:
    CorpusBar();

    void resized () override;

    std::function<void()> onLoad;
    std::function<void()> onAnalyse;
    std::function<void()> onSaveAnalysis;
    std::function<void()> onLoadAnalysis;
    std::function<void()> onSettings;

    juce::Label statusLabel;

    static constexpr int analysisPaneWidth = 240;

private:
    ArsNovaUI::ButtonPane filePane;
    ArsNovaUI::ButtonPane analysisPane;
    ArsNovaUI::ButtonPane settingsPane;
};
