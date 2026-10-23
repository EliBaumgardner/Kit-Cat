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

/*
    The plugin's window: the corpus bar along the top, the map, and the parameter panel on the
    right, whose width the user drags. It exists only while the window is open, and reaches the
    processor through references to the parts it uses, taken in its constructor.

    It connects the components to the processor. The corpus bar's buttons open file choosers and
    call CorpusLoader, or open ArsNovaUI's SettingsMenu; a slice picked on the map is pushed to
    SlicePlayer as Play. As a change listener on CorpusLoader it shows the status text, hands
    CorpusMap the published corpus and freezes the map while an analysis runs. Its 60 Hz timer
    drains AudioUIBridge's flashes and pucks into CorpusMap and fades the flashes.

    Interface zoom and theme colour are saved in interfaceSettings, a file on this computer rather
    than in the project, and ScaledView scales the whole workspace by the zoom.
*/

class KitCatAudioProcessor;

class KitCatAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer
{
public:
    explicit KitCatAudioProcessorEditor(KitCatAudioProcessor& audioProcessor);
    ~KitCatAudioProcessorEditor() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    void chooseCorpus         ();
    void chooseAnalysisToSave ();
    void chooseAnalysisToLoad ();

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback          () override;

    AudioUIBridge&     audioUIBridge;
    CorpusPublisher&   corpusPublisher;
    SlicePlayer&       slicePlayer;
    CorpusLoader&      corpusLoader;
    juce::UndoManager& undoManager;

    juce::ValueTree settings;
    juce::ValueTree colourPresets;

    ArsNovaUI::CustomLookAndFeel lookAndFeel;

    std::unique_ptr<juce::PropertiesFile> interfaceSettings;

    juce::Component workspace;

    std::unique_ptr<ArsNovaUI::ScaledView> scaledView;
    std::unique_ptr<juce::FileChooser>     fileChooser;

    ArsNovaUI::PopupWindowLauncher settingsLauncher { "Settings" };

    CorpusBar corpusBar;

    ParameterPanel parameterPanel;

    CorpusMap corpusMap;

    int panelWidth = ParameterPanel::defaultWidth;

    static constexpr int minimumMapWidth   = 240;
    static constexpr int minimumMapHeight  = 200;
    static constexpr int openingWidth      = 960;
    static constexpr int openingHeight     = 560;
    static constexpr int maximumWindowSide = 8192;
};
