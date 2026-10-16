#pragma once

#include "Audio/AudioUIBridge.h"
#include "Audio/CorpusPublisher.h"
#include "Audio/SlicePlayer.h"
#include "Map/CorpusMap.h"
#include "Plugin/CorpusLoader.h"
#include "UI/CorpusBar.h"
#include "UI/ParameterPanel.h"

#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/ScaledView.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class CatKitAudioProcessor;

class CatKitAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer
{
public:
    explicit CatKitAudioProcessorEditor(CatKitAudioProcessor& audioProcessor);
    ~CatKitAudioProcessorEditor() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    void chooseCorpus         ();
    void chooseAnalysisToSave ();
    void chooseAnalysisToLoad ();

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback          () override;

    AudioUIBridge&                        audioUIBridge;
    CorpusPublisher&                      corpusPublisher;
    SlicePlayer&                          slicePlayer;
    CorpusLoader&                         corpusLoader;
    juce::ValueTree                       settings;
    juce::ValueTree                       colourPresets;
    juce::UndoManager&                    undoManager;
    CustomLookAndFeel                     lookAndFeel;
    std::unique_ptr<juce::PropertiesFile> interfaceSettings;
    juce::Component                       workspace;
    std::unique_ptr<ScaledView>           scaledView;
    PopupWindowLauncher                   settingsLauncher { "Settings" };
    CorpusBar                             corpusBar;
    ParameterPanel                        parameterPanel;
    CorpusMap                             corpusMap;
    std::unique_ptr<juce::FileChooser>    fileChooser;
    int                                   panelWidth = ParameterPanel::defaultWidth;

    static constexpr int minimumMapWidth   = 240;
    static constexpr int minimumMapHeight  = 200;
    static constexpr int openingWidth      = 960;
    static constexpr int openingHeight     = 560;
    static constexpr int maximumWindowSide = 8192;
};
