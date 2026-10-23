#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/*
    The bar along the top of the window: a file pane (load a sample or a folder, analyse again), an
    analysis pane (save and load an analysis), a settings button, and statusLabel in the space
    between, which shows CorpusLoader's status. It holds no logic of its own: each button calls one
    of onLoad, onAnalyse, onSaveAnalysis, onLoadAnalysis and onSettings, which
    KitCatAudioProcessorEditor assigns.
*/

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
