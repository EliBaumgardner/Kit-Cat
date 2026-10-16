# Cat-Kit

Cat-Kit is a real-time audio mosaicing instrument. Give it a folder of sounds and play or sing into it, and it answers with the pieces of those sounds that most resemble what you just did.

It doesn't process your input. It **replaces** it. Every sound you hear comes from your samples, chosen about 170 times a second to follow the timbre and pitch of the input. A voice fed into a corpus of drum hits comes out as a stream of drum hits that rises and falls with the voice. The same corpus also appears as a map you can play with the mouse.

It runs as a standalone app, an AU and a VST3, and is written in C++20 with [JUCE](https://juce.com/) and [FluCoMa](https://www.flucoma.org/).

## How it works

```
 your samples ──► cut into slices ──► describe each one ──► lay them out on a map
                                      (timbre + pitch)       (similar sounds sit together)

 live input ──► describe the last 23 ms ──► find the closest slices ──► play one as a grain
                (every 5.8 ms)
```

1. **Analysis.** When you load a corpus, Cat-Kit cuts each file into slices, at the start of each sound or as short fixed grains. It measures every slice's timbre (MFCCs) and pitch, then arranges the slices on a 2D map so that similar sounds sit near each other. This takes a few seconds and runs in the background.
2. **Matching.** While input plays, Cat-Kit describes the last 23 ms of it in the same terms. It looks up the closest-sounding slices and starts one of them as a short grain, every 5.8 ms. Many grains overlap, so the result is continuous.
3. **The map.** Each slice is a dot, coloured by where it sits, so nearby (similar-sounding) dots share a colour. A spinner shows on the map while a corpus is being analysed. While live input plays, a puck shows where the input lands among them, and the dots that answer it flash. Dragging the mouse across the map plays the nearest dot. Scrolling pans the map, and pinching or Shift+scrolling zooms in around the cursor, which spreads a dense cluster apart without enlarging its dots; a double-click shows the whole map again.

## Getting started

1. Build and launch the standalone app (see [Building](#building)).
2. Open **Options → Audio/MIDI Settings** and pick your microphone or interface as the input.
3. Click the **load** button and choose a sample or a folder of samples. The dots appear once analysis finishes.
4. Set **Match live input** to **On** and make a sound.

Short, varied material works best as a corpus: drums, percussion, speech, textures, or a few minutes of an instrument played many different ways. A single sustained note gives the matcher little to choose from.

## Two ways to play

| | Live input | The map |
|---|---|---|
| **You play it with** | A microphone or any audio input | The mouse: press and drag across the dots |
| **What sounds** | Grains of the slices closest to the input, overlapping | One slice at a time, crossfading to the next dot you reach |
| **Best for** | Performing, re-voicing a part, call-and-response | Exploring a corpus and hearing how it is organised |

Both can run at once.

## Settings

Analysis settings take effect when you click **Analyse again**. Live and playback settings take effect immediately.

**Analysis**

| Setting | What it does |
|---|---|
| Slicing mode | **Onset** cuts at the start of each sound. **Granular** cuts 60 ms grains every 30 ms, which suits sustained material. |
| Onset threshold, Onset metric | How strong a change must be to count as a new sound, and which kind of change counts (energy, spectral flux, and others). |
| Shortest slice | The shortest slice, in analysis hops (about 12 ms each). |
| MFCC coefficients | How much detail the timbre description keeps. |
| Map neighbours, Map minimum distance | The shape of the map. More neighbours favours the overall layout; a smaller minimum distance packs similar sounds tighter. |

**Live input**

| Setting | What it does |
|---|---|
| Match live input | Turns live matching on or off. |
| Input gate | Input quieter than this (in dB) plays nothing. |
| Pitch weight | 0 matches on timbre alone; 1 matches on pitch as much as possible. |
| Variation | How many of the closest slices to choose among at random. 1 always plays the single closest. |
| Repetition penalty | How strongly to avoid slices played in the last few grains. |

**Playback**

| Setting | What it does |
|---|---|
| Crossfade | The fade between slices when playing the map with the mouse. |

**Interface** (the settings button at the top right; saved on this computer, not in the project)

| Setting | What it does |
|---|---|
| Theme colour | The colour the whole interface is tinted from. |
| Interface zoom | The size of every button, label and bar, from 1x to 2x. |

**Save analysis** and **Load analysis** store the slices and the map, so a large corpus doesn't need analysing again. Keep the analysis file next to its corpus folder and the pair can be moved anywhere together.

## Under the hood

The analysis uses FluCoMa's algorithms on a background thread. The live engine runs entirely on the audio thread and never allocates, locks or waits. A RealtimeSanitizer test checks that guarantee for everything `processBlock` reaches.

The closest-slice search is a KD-tree that stays exact when timbre and pitch are blended. The live pitch tracker is a port of FluCoMa's YIN-FFT that does no allocation, and grains start on the exact sample their input hop ended. From an input onset to a grain starting is at most 5.8 ms, plus your interface's buffers.

[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) covers the threads, the matching distance, which parts are FluCoMa's and which are written here, the latency in detail, and the tests.

## Building

You need macOS, CMake 3.28 or later, and Ninja. FluCoMa's own dependencies are downloaded the first time you configure.

```bash
git clone https://github.com/EliBaumgardner/Cat-Kit.git
cd Cat-Kit
git submodule update --init
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target CatKit_Standalone
```

The app is built into `build/CatKit_artefacts/Release/Standalone/`. Building `CatKit_AU` or `CatKit_VST3` also installs the plugin into `~/Library/Audio/Plug-Ins`. `ctest --test-dir build` runs the tests once `CatKit_Tests` is built.

## Limitations

- Built and tested on macOS (Apple Silicon) only.
- Live input has been tested in the standalone app. Whether a DAW lets an instrument plugin receive audio, and how you route it, depends on the host.
- Pitch matching is coarse below about 200 Hz: the live window is 23 ms long, and a pure 220 Hz tone reads about a third of a semitone sharp.
- Grains fade in over half their length, so a long grain softens the attack of what you play.

## Credits

Analysis algorithms from [FluCoMa](https://www.flucoma.org/) (Fluid Corpus Manipulation, University of Huddersfield). Plugin framework [JUCE](https://juce.com/). Interface components from [ArsNovaUI](https://github.com/EliBaumgardner/ArsNovaUI). Tests use [Catch2](https://github.com/catchorg/Catch2).
