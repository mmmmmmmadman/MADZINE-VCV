# MADZINE Modules for VCV Rack

**Version 2.6.0**

A collection of creative modules for VCV Rack, focusing on rhythm generation, mixing, and experimental sound design.

## Module Categories

### Signature Series
- **weiii documenta** (12 HP) - 8-layer recording sampler with chaotic parameter morphing, feedback matrix and slice engine
- **Universal Rhythm** (40 HP) - Cross-cultural rhythm generator based on ethnomusicological principles with 10 world music styles
- **Uni Rhythm** (32 HP) - Compact 4-track rhythm generator with Primary-priority merged outputs, derived from Universal Rhythm
- **Launchpad** (40 HP) - 8x8 grid CV/audio looper with session mode, per-cell recording, and send/return mixer

### Euclidean Drum Machine Series
- **MADDY** (8 HP) - Integrated sequencer with swing clock and 3-track Euclidean rhythm generator
- **MADDY Plus** (12 HP) - Extended MADDY sequencer
- **TWNC** (8 HP) - Dual-track Euclidean rhythm generator with drum and hi-hats synthesis
- **TWNC Light** (4 HP) - Simplified dual-track Euclidean rhythm generator with envelope control
- **TWNC 2** (8 HP) - Three-track Euclidean drum machine with drum synthesis
- **KIMO** (4 HP) - Single-track Euclidean drum machine with bass drum synthesis

### Rhythmic Modulator
- **SwingLFO** (4 HP) - Dual-waveform LFO with swing and shape control
- **Euclidean Rhythm** (8 HP) - Three-track Euclidean Rhythm Generator with CV control and Slew
- **PPaTTTerning** (8 HP) - Pattern-based CV sequencer with style and density control
- **SONG MODE** (8 HP) - 8-input sequential switch with customizable playback order, per-input clock length, and learn mode

### Drum Synthesis
- **Drummmmmmer** (8 HP) - 4-voice world drum synthesizer with style presets and spread control
- **theKICK** (8 HP) - Kick drum synthesizer with pitch sweep, bend curve and tanh waveshaper

### Envelope Chain
- **MAD REPEATER** (6 HP) - 6HP single-stage envelope generator with clock sync, chainable to the right with F expanders for multi-stage envelopes
- **F** (4 HP) - 4HP single-stage expander for MAD REPEATER, adds one level/curve/duration segment to the envelope chain

### Effects
- **Ellen Ripley** (8 HP) - Chaos-modulated multi-effects processor with delay, granular and reverb
- **Runner** (4 HP) - Chaos-modulated stereo delay with spread control, derived from Ellen Ripley
- **Facehugger** (4 HP) - Chaos-modulated granular processor (Gratch), derived from Ellen Ripley
- **Ovomorph** (4 HP) - Chaos-modulated stereo reverb with room and decay control, derived from Ellen Ripley

### Mixer System
- **U8** (4 HP) - Channel processor with gain/saturation, duck control, mute, and auto-patch
- **YAMANOTE** (8 HP) - 8-channel mixer with chain, send/return, and auto-patch
- **Env VCA 6** (12 HP) - 6-channel envelope VCA processor with AD generators
- **ALEXANDERPLATZ** (16 HP) - 4-track mixer with 8-band master EQ, level, duck and mute controls, Berlin U8 themed
- **SHINJUKU** (32 HP) - 8-track mixer with 12-band master EQ, level, duck and mute controls, Tokyo Marunouchi themed
- **Portal** (8 HP) - 8HP poly crossfader for UniRhythm with 3-band isolator, drive, DJ-style cue monitoring and curve selection

### Pyramid
3D Panning mixing workstation designed for HATAKEN.
- **KEN** (4 HP) - 8-to-2 binaural processor for 3D spatial audio rendering
- **Pyramid** (8 HP) - 3D panning router
- **DECAPyramid** (40 HP) - 8-track 3D panning router with send/return

### Utility
- **AD Generator** (8 HP) - Attack Decay envelope generator
- **Pinpple** (4 HP) - Ping filter hihat synthesizer with dynamic FM modulation
- **QQ** (4 HP) - 3-track S-Curve Decay Trigger envelope generator with CV control and waveform scope
- **Observer** (8 HP) - 8-track color scope module for waveform visualization
- **Obserfour** (8 HP) - 4-track color scope module for waveform visualization
- **Quantizer** (4 HP) - Pitch quantizer with microtune control and scale presets
- **NIGOQ** (12 HP) - Complex oscillator module
- **Runshow** (12 HP) - Timer with bar counting and visual feedback
- **Manual** (12 HP) - Interactive help display showing module descriptions when hovering MADZINE modules

## Installation

Available in the [VCV Rack Library](https://library.vcvrack.com/?brand=MADZINE)

## Manual

See [https://linktr.ee/madzine](https://linktr.ee/madzine) for the interactive module manual.

## Changelog

### v2.6.0
- Added **MAD REPEATER** (6 HP) and **F** (4 HP) multi-stage envelope modules
  - MAD REPEATER is the host stage; F expanders chain to the right to add stages
  - Clock sync and per-stage SYNC
- Removed FFT Comp, EATINGCV and EATINGGATE
- Custom knob sensitivity doubled
- Fixed MADDY and MADDY Plus pattern start alignment after RESET
- Fixed NIGOQ oversample filter sample rate
- Added LICENSE-dist.txt documenting third-party licenses

### v2.3.8
- **Auto-Patch Feature**: U8 and YAMANOTE modules now automatically connect when placed adjacent
  - Chain outputs auto-connect to chain inputs with visible cables
  - Cable colors match the train colors on modules (U8: yellow, YAMANOTE: green)
  - Auto CH Input: When audio connects to U8's input, it automatically connects to YAMANOTE's corresponding channel (leftmost U8 → CH1, second U8 → CH2, etc.)

### v2.3.6
- Replaced 20+ individual PDF manuals with unified interactive HTML manual
- Reorganized module categories for better navigation
- Added new panel themes (Sashimi, Wine, Boring, ToiletPaper)
- Various UI improvements and bug fixes

## License

GPL-3.0-or-later

## Links

- [Source Code](https://github.com/mmmmmmmadman/MADZINE-VCV)
- [Support on Patreon](https://www.patreon.com/c/madzinetw)

## Author

MAD (madzinetw@gmail.com)
