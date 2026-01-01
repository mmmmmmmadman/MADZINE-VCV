# MADZINE Modules for VCV Rack

**Version 2.3.8**

A collection of creative modules for VCV Rack, focusing on rhythm generation, mixing, and experimental sound design.

## Module Categories

### Signature Series
- **weiii documenta** (12 HP) - 8-layer recording sampler with chaotic parameter morphing, feedback matrix and slice engine
- **Universal Rhythm** (40 HP) - Cross-cultural rhythm generator based on ethnomusicological principles with 10 world music styles

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

### Mixer System
- **U8** (4 HP) - Channel processor with gain/saturation, duck control, mute, and auto-patch
- **YAMANOTE** (8 HP) - 8-channel mixer with chain, send/return, and auto-patch
- **Env VCA 6** (12 HP) - 6-channel envelope VCA processor with AD generators

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
- **Quantizer** (4 HP) - Quantizer module
- **Ellen Ripley** (8 HP) - Chaos + Delay + Granular + Reverb processor
- **NIGOQ** (12 HP) - Complex oscillator module
- **Runshow** (12 HP) - Timer with bar counting and visual feedback

## Installation

Available in the [VCV Rack Library](https://library.vcvrack.com/?brand=MADZINE)

## Manual

See [https://linktr.ee/madzine](https://linktr.ee/madzine) for the interactive module manual.

## Changelog

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
