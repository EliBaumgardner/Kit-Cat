#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"

KitCatAudioProcessor::KitCatAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      corpusLoader(settings, corpusPublisher, slicePlayer)
{
    const CorpusAnalysis defaults;

    settings.addListener(this);

    settings.setProperty(StateIdentifiers::CorpusPath, juce::String(), nullptr);
    settings.setProperty(StateIdentifiers::OnsetThreshold, defaults.slicing.threshold, nullptr);
    settings.setProperty(StateIdentifiers::OnsetMetric, static_cast<int>(defaults.slicing.onsetMetric), nullptr);
    settings.setProperty(StateIdentifiers::MinimumSliceFrames, defaults.slicing.minimumSliceFrames, nullptr);
    settings.setProperty(StateIdentifiers::CoefficientCount, defaults.slicing.coefficientCount, nullptr);
    settings.setProperty(StateIdentifiers::NeighbourCount, defaults.layout.neighbourCount, nullptr);
    settings.setProperty(StateIdentifiers::MinimumDistance, defaults.layout.minimumDistance, nullptr);
    settings.setProperty(StateIdentifiers::FadeMilliseconds, SlicePlayer::defaultFadeMilliseconds, nullptr);
    settings.setProperty(StateIdentifiers::LengthMilliseconds, 0.0, nullptr);

    settings.setProperty(StateIdentifiers::SlicingMode, static_cast<int>(defaults.slicing.slicingMode), nullptr);
    settings.setProperty(StateIdentifiers::GrainSizeMilliseconds, defaults.slicing.grainSizeMilliseconds, nullptr);
    settings.setProperty(StateIdentifiers::GrainHopMilliseconds, defaults.slicing.grainHopMilliseconds, nullptr);

    settings.setProperty(StateIdentifiers::LiveMatchingEnabled, liveMatchingEnabled.load(), nullptr);
    settings.setProperty(StateIdentifiers::InputGateThreshold, inputGateThresholdDb.load(), nullptr);
    settings.setProperty(StateIdentifiers::PitchWeight, pitchWeight.load(), nullptr);
    settings.setProperty(StateIdentifiers::CandidateCount, candidateCount.load(), nullptr);
    settings.setProperty(StateIdentifiers::RepetitionPenalty, repetitionPenalty.load(), nullptr);
}

void KitCatAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    slicePlayer.prepare(sampleRate);
    targetAnalyser.prepare(sampleRate, targetWindowSize, targetHopSize, SliceAnalysis().bandCount, static_cast<int>(settings[StateIdentifiers::CoefficientCount]));
    targetMatcher.reset();

    inputRingBuffer.fill(0.0f);
    inputWritePos = 0;
    hopCounter    = 0;
}

void KitCatAudioProcessor::releaseResources()
{
}

void KitCatAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept [[clang::nonblocking]]
{
    juce::ScopedNoDenormals     noDenormals;
    CorpusPublisher::BlockScope block { corpusPublisher };
    const int                   numSamples    = buffer.getNumSamples();
    const int                   inputChannels = getTotalNumInputChannels();
    const float                 channelShare  = 1.0f / static_cast<float>(juce::jmax(1, inputChannels));
    const float                 gateGain      = juce::Decibels::decibelsToGain(inputGateThresholdDb.load());

    if (liveMatchingEnabled.load() && inputChannels > 0) {
        for (int sample = 0; sample < numSamples; ++sample) {
            float monoSample = 0.0f;

            for (int channel = 0; channel < inputChannels; ++channel) {
                monoSample += buffer.getSample(channel, sample);
            }

            inputRingBuffer[static_cast<size_t>(inputWritePos)] = monoSample * channelShare;
            inputWritePos = (inputWritePos + 1) % inputRingBufferSize;

            if (++hopCounter >= targetHopSize) {
                const size_t windowStart = static_cast<size_t>((inputWritePos - targetWindowSize + inputRingBufferSize) % inputRingBufferSize);
                const size_t beforeWrap  = std::min(static_cast<size_t>(targetWindowSize), inputRingBuffer.size() - windowStart);

                hopCounter = 0;

                std::copy_n(inputRingBuffer.begin() + static_cast<std::ptrdiff_t>(windowStart), beforeWrap, inputWindow.begin());
                std::copy_n(inputRingBuffer.begin(), inputWindow.size() - beforeWrap, inputWindow.begin() + static_cast<std::ptrdiff_t>(beforeWrap));

                TargetAnalyser::TargetFeatures features = targetAnalyser.processFrame(inputWindow.data(), targetWindowSize);

                if (features.rms >= gateGain && block.corpus != nullptr) {
                    TargetMatcher::MatchResult match = targetMatcher.match(features, *block.corpus, pitchWeight.load(), candidateCount.load(), repetitionPenalty.load());

                    if (match.sliceIndex >= 0) {
                        const float grainGain = std::clamp(features.rms * 2.0f, 0.1f, 1.0f);

                        slicePlayer.triggerGrain(match.sliceIndex, grainGain, *block.corpus, sample);
                        audioUIBridge.pushFlash(match.sliceIndex);
                        audioUIBridge.pushPuck(match.mapPosition, features.rms, true);
                    }
                } else {
                    audioUIBridge.pushPuck({ 0.5f, 0.5f }, features.rms, false);
                }
            }
        }
    }

    buffer.clear();

    slicePlayer.render(buffer, block.corpus);
}

void KitCatAudioProcessor::setCurrentProgram(int index)
{
}

void KitCatAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void KitCatAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state = settings.createCopy();

    state.addChild(colourPresets.createCopy(), -1, nullptr);

    const std::unique_ptr<juce::XmlElement> xml = state.createXml();

    copyXmlToBinary(*xml, destData);
}

void KitCatAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const std::unique_ptr<juce::XmlElement> xml = getXmlFromBinary(data, sizeInBytes);
    juce::ValueTree                         restored;

    if (xml != nullptr) {
        restored = juce::ValueTree::fromXml(*xml);
    }

    if (! restored.hasType(StateIdentifiers::KitCatState)) {
        return;
    }

    for (int index = 0; index < restored.getNumProperties(); ++index) {
        const juce::Identifier property = restored.getPropertyName(index);

        settings.setProperty(property, restored[property], nullptr);
    }

    colourPresets.copyPropertiesAndChildrenFrom(restored.getChildWithName(ArsNovaUI::InterfaceIdentifiers::ColourPresets), nullptr);

    undoManager.clearUndoHistory();

    corpusLoader.analyse();
}

bool KitCatAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const juce::AudioChannelSet output = layouts.getMainOutputChannelSet();
    const juce::AudioChannelSet input  = layouts.getMainInputChannelSet();

    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo()) {
        return false;
    }

    if (! input.isDisabled() && input != juce::AudioChannelSet::mono() && input != juce::AudioChannelSet::stereo()) {
        return false;
    }

    return true;
}

bool KitCatAudioProcessor::hasEditor() const
{
    return true;
}

bool KitCatAudioProcessor::acceptsMidi() const
{
    return true;
}

bool KitCatAudioProcessor::producesMidi() const
{
    return false;
}

bool KitCatAudioProcessor::isMidiEffect() const
{
    return false;
}

juce::AudioProcessorEditor* KitCatAudioProcessor::createEditor()
{
    return new KitCatAudioProcessorEditor(*this);
}

const juce::String KitCatAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

const juce::String KitCatAudioProcessor::getProgramName(int index)
{
    return {};
}

double KitCatAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KitCatAudioProcessor::getNumPrograms()
{
    return 1;
}

int KitCatAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KitCatAudioProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == StateIdentifiers::FadeMilliseconds) {
        slicePlayer.fadeSeconds.store(static_cast<double>(tree[property]) / 1000.0);
    }

    if (property == StateIdentifiers::LengthMilliseconds) {
        slicePlayer.lengthSeconds.store(static_cast<double>(tree[property]) / 1000.0);
    }

    if (property == StateIdentifiers::LiveMatchingEnabled) {
        liveMatchingEnabled.store(static_cast<bool>(tree[property]));
    }

    if (property == StateIdentifiers::InputGateThreshold) {
        inputGateThresholdDb.store(static_cast<float>(tree[property]));
    }

    if (property == StateIdentifiers::PitchWeight) {
        pitchWeight.store(static_cast<double>(tree[property]));
    }

    if (property == StateIdentifiers::CandidateCount) {
        candidateCount.store(static_cast<int>(tree[property]));
    }

    if (property == StateIdentifiers::RepetitionPenalty) {
        repetitionPenalty.store(static_cast<double>(tree[property]));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KitCatAudioProcessor();
}
