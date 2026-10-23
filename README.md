# Kit-Cat

Kit-Cat is a real-time audio mosaicing instrument: it rebuilds a live input out of short pieces of your own samples. You give it a folder of sounds and play or sing into it, and about 170 times a second it plays the piece of those sounds that most resembles the input's timbre and pitch. A voice fed into a corpus of drum hits comes out as drum hits that rise and fall with the voice. Nothing of the input itself is heard.

It runs as a standalone app, an AU and a VST3, written in C++20 with [JUCE](https://juce.com/) and [FluCoMa](https://www.flucoma.org/).

## How it works

1. **Analysis.** Loading a corpus cuts each file into slices, either at the start of each sound or as fixed 60 ms grains. Each slice's timbre and pitch are measured, and the slices are laid out on a 2D map with similar sounds close together. This runs in the background and takes a few seconds.
2. **Matching.** Every 5.8 ms, the last 23 ms of input is measured the same way, and one of the closest slices starts as a short grain. The grains overlap, so the output is continuous.
3. **The map.** Each slice is a dot, coloured by its position, so similar sounds share a colour. A puck marks where the live input lands and the dots that answer it flash. Dragging the mouse across the map plays the nearest dot, crossfading from one to the next. Scroll pans, pinch or Shift+scroll zooms around the cursor, and a double-click shows the whole map.

Live input and the map can play at the same time.

## Getting started

1. Build and launch the standalone app (see [Building](#building)).
2. In **Options → Audio/MIDI Settings**, choose your microphone or interface as the input.
3. Click **Load a sample or a folder** and pick your samples. The dots appear when analysis finishes.
4. Set **Match live input** to **On** and make a sound.

Short, varied material makes the best corpus: drums, speech, textures, or an instrument played many ways. A single sustained note leaves little to choose from.

## Settings

Analysis settings apply when you click **Analyse again with the current settings**; the others apply immediately.

| Setting | What it does |
|---|---|
| **Analysis** | |
| Slicing mode | **Onset** cuts at the start of each sound. **Granular** cuts 60 ms grains every 30 ms, for sustained material. |
| Onset threshold, Onset metric | How large a change starts a new slice, and which kind of change is measured. |
| Shortest slice (hops) | The minimum slice length, in steps of about 12 ms. |
| MFCC coefficients | How much detail the timbre measurement keeps. |
| Map neighbours, Map minimum distance | The map's shape: more neighbours keeps the overall layout, a smaller distance packs similar sounds tighter. |
| **Live input** | |
| Match live input | Turns matching on or off. |
| Input gate | Input below this level, in dB, plays nothing. |
| Pitch weight | 0 matches timbre only; 1 weighs pitch as much as possible. |
| Variation (Top-K) | How many of the closest slices to pick from at random. 1 always plays the closest. |
| Repetition penalty | How strongly to avoid slices from the last few grains. |
| **Playback** | |
| Crossfade | The fade between dots when playing the map with the mouse. |

The settings button at the top right holds the theme colour and interface zoom, saved on this computer rather than in the project. **Save** and **Load analysis** store the slices and map so a large corpus isn't analysed twice; keep the file beside its corpus folder.

## Building

Requires macOS, CMake 3.28 or later, and Ninja. FluCoMa's dependencies download on the first configure.

```bash
git clone https://github.com/EliBaumgardner/Kit-Cat.git
cd Kit-Cat
git submodule update --init
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KitCat_Standalone
```

The app lands in `build/KitCat_artefacts/Release/Standalone/`. `KitCat_AU` and `KitCat_VST3` also install into `~/Library/Audio/Plug-Ins`. Build `KitCat_Tests`, then run `ctest --test-dir build`.

The live engine runs on the audio thread without allocating, locking or waiting, which a RealtimeSanitizer test checks. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) covers the threads, the matching, latency and tests.

## Limitations

- Built and tested on macOS (Apple Silicon) only.
- Live input is tested in the standalone app. Routing audio into an instrument plugin depends on the host.
- Pitch matching is coarse below about 200 Hz: a 23 ms window reads a 220 Hz tone about a third of a semitone sharp.
- Grains fade in over half their length, softening the attack of what you play.

## Credits

Analysis from [FluCoMa](https://www.flucoma.org/) (University of Huddersfield), framework [JUCE](https://juce.com/), interface components [ArsNovaUI](https://github.com/EliBaumgardner/ArsNovaUI), tests [Catch2](https://github.com/catchorg/Catch2).
