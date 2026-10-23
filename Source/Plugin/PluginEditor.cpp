#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"

#include <ArsNovaUI/Menus/SettingsMenu.h>
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>

KitCatAudioProcessorEditor::KitCatAudioProcessorEditor(KitCatAudioProcessor& audioProcessor)
    : AudioProcessorEditor(audioProcessor),
      audioUIBridge(audioProcessor.audioUIBridge),
      corpusPublisher(audioProcessor.corpusPublisher),
      slicePlayer(audioProcessor.slicePlayer),
      corpusLoader(audioProcessor.corpusLoader),
      undoManager(audioProcessor.undoManager),
      settings(audioProcessor.settings),
      colourPresets(audioProcessor.colourPresets),
      parameterPanel(audioProcessor.settings, audioProcessor.undoManager)
{
    juce::PropertiesFile::Options interfaceSettingsOptions;

    interfaceSettingsOptions.applicationName     = "Interface";
    interfaceSettingsOptions.filenameSuffix      = ".settings";
    interfaceSettingsOptions.folderName          = JucePlugin_Name;
    interfaceSettingsOptions.osxLibrarySubFolder = "Application Support";

    interfaceSettings = std::make_unique<juce::PropertiesFile>(interfaceSettingsOptions);

    if (interfaceSettings->containsKey(ArsNovaUI::InterfaceIdentifiers::ThemeColour.toString())) {
        lookAndFeel.applyThemeColour(juce::Colour::fromString(interfaceSettings->getValue(ArsNovaUI::InterfaceIdentifiers::ThemeColour.toString())));
    }

    setLookAndFeel(&lookAndFeel);

    scaledView = std::make_unique<ArsNovaUI::ScaledView>(workspace, *interfaceSettings);

    scaledView->minimumContentSize = { ArsNovaUI::Theme::windowGutter * 3 + ParameterPanel::minimumWidth + minimumMapWidth, ArsNovaUI::Theme::windowGutter * 3 + ArsNovaUI::Theme::barHeight + minimumMapHeight };
    settingsLauncher.openingScale  = ArsNovaUI::SettingsMenu::openingScale;

    corpusBar.onLoad         = [this] { chooseCorpus(); };
    corpusBar.onAnalyse      = [this] { corpusLoader.analyse(); };
    corpusBar.onSaveAnalysis = [this] { chooseAnalysisToSave(); };
    corpusBar.onLoadAnalysis = [this] { chooseAnalysisToLoad(); };

    corpusBar.onSettings = [this] {
        settingsLauncher.show(corpusBar, [this] {
            std::unique_ptr<ArsNovaUI::SettingsMenu> content = std::make_unique<ArsNovaUI::SettingsMenu>(lookAndFeel, *interfaceSettings, *scaledView, colourPresets, undoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        });
    };

    scaledView->onZoomChanged = [this] {
        const juce::Point<int> minimumSize = scaledView->minimumContentSize;

        juce::Component* const               window  = getTopLevelComponent();
        const juce::Displays::Display* const display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window->getScreenBounds());

        setResizeLimits(juce::roundToInt(minimumSize.x * scaledView->zoom), juce::roundToInt(minimumSize.y * scaledView->zoom), maximumWindowSide, maximumWindowSide);

        resized();

        if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone && display != nullptr) {
            window->setBounds(window->getBounds().constrainedWithin(display->userArea));
        }
    };

    corpusMap.onSlicePicked = [this](int sliceIndex) { slicePlayer.push({ SlicePlayer::CommandKind::Play, sliceIndex }); };

    parameterPanel.resizer.onWidthDragged = [this](int newWidth) {
        panelWidth = newWidth;

        resized();
    };

    corpusLoader.addChangeListener(this);

    changeListenerCallback(&corpusLoader);

    workspace.addAndMakeVisible(corpusBar);
    workspace.addAndMakeVisible(parameterPanel);
    workspace.addAndMakeVisible(corpusMap);

    addAndMakeVisible(*scaledView);

    setResizable(true, true);
    setSize(openingWidth, openingHeight);
    setResizeLimits(juce::roundToInt(scaledView->minimumContentSize.x * scaledView->zoom), juce::roundToInt(scaledView->minimumContentSize.y * scaledView->zoom), maximumWindowSide, maximumWindowSide);
    startTimerHz(60);
}

KitCatAudioProcessorEditor::~KitCatAudioProcessorEditor()
{
    stopTimer();

    corpusLoader.removeChangeListener(this);

    setLookAndFeel(nullptr);
}

void KitCatAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(lookAndFeel.windowColour);
}

void KitCatAudioProcessorEditor::resized()
{
    scaledView->setBounds(getLocalBounds());

    juce::Rectangle<int> bounds = workspace.getLocalBounds().reduced(ArsNovaUI::Theme::windowGutter);

    const int widestPanel = juce::jmax(ParameterPanel::minimumWidth, juce::jmin(ParameterPanel::maximumWidth, bounds.getWidth() - ArsNovaUI::Theme::windowGutter - minimumMapWidth));

    panelWidth = juce::jlimit(ParameterPanel::minimumWidth, widestPanel, panelWidth);

    corpusBar.setBounds(bounds.removeFromTop(ArsNovaUI::Theme::barHeight));
    bounds.removeFromTop(ArsNovaUI::Theme::windowGutter);
    parameterPanel.setBounds(bounds.removeFromRight(panelWidth));
    bounds.removeFromRight(ArsNovaUI::Theme::windowGutter);
    corpusMap.setBounds(bounds);
}

void KitCatAudioProcessorEditor::chooseCorpus()
{
    const int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser = std::make_unique<juce::FileChooser>("Choose a sample or a folder", juce::File(), CorpusAnalysis::audioFilePattern);

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        if (chooser.getResult() == juce::File()) {
            return;
        }

        settings.setProperty(StateIdentifiers::CorpusPath, chooser.getResult().getFullPathName(), nullptr);

        corpusLoader.analyse();
    });
}

void KitCatAudioProcessorEditor::chooseAnalysisToSave()
{
    const int flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser = std::make_unique<juce::FileChooser>("Save the analysis", juce::File(), CorpusAnalysis::analysisFilePattern);

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        if (chooser.getResult() == juce::File()) {
            return;
        }

        corpusLoader.saveAnalysis(chooser.getResult().withFileExtension("json"));
    });
}

void KitCatAudioProcessorEditor::chooseAnalysisToLoad()
{
    const int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Load an analysis", juce::File(), CorpusAnalysis::analysisFilePattern);

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        if (chooser.getResult() == juce::File()) {
            return;
        }

        corpusLoader.loadAnalysis(chooser.getResult());
    });
}

void KitCatAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    corpusBar.statusLabel.setText(corpusLoader.statusText, juce::dontSendNotification);

    corpusMap.corpus = corpusPublisher.publishedCorpus;
    corpusMap.state  = CorpusMap::MapState::Playable;

    if (corpusLoader.state == CorpusLoader::AnalysisState::Analysing) {
        corpusMap.state = CorpusMap::MapState::Frozen;
    }

    corpusMap.repaint();
}

void KitCatAudioProcessorEditor::timerCallback()
{
    audioUIBridge.flashFifo.drain([this](const AudioUIBridge::SliceFlash& flash) {
        corpusMap.flashSlice(flash.sliceIndex);
    });

    audioUIBridge.puckFifo.drain([this](const AudioUIBridge::LivePuck& puck) {
        corpusMap.setLivePuck(puck.position, puck.rms, puck.active);
    });

    corpusMap.updateFlashes();
}
