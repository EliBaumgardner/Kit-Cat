#include "Audio/AudioUIBridge.h"

void AudioUIBridge::pushPuck(juce::Point<float> position, float rms, bool active)
{
    puckFifo.push({ position, rms, active });
}

void AudioUIBridge::pushFlash(int sliceIndex)
{
    flashFifo.push({ sliceIndex });
}
