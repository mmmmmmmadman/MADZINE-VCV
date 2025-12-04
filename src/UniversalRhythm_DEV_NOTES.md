# Universal Rhythm - Development Notes

## Overview

Universal Rhythm is a 40HP cross-cultural rhythm generator module based on ethnomusicological principles. It integrates the WorldRhythm algorithm with an 8-voice drum synthesizer, providing 10 world music styles with per-role customization.

## Architecture

### Core Components
- **WorldRhythm Algorithm**: Pattern generation based on 4-role hierarchy (Timeline, Foundation, Groove, Lead)
- **Interlock Mechanism**: Style-specific rules for pattern relationships between roles
- **ExtendedDrumSynth**: 8-voice synthesizer with per-role Freq/Decay control
- **RestEngine**: Position-weighted rest application
- **HumanizeEngine**: Style-aware humanization with swing

### 4-Role Hierarchy
Each role has 2 voices (8 total):
- **Timeline** (TL1, TL2): Bell/clave patterns - provides rhythmic reference
- **Foundation** (FD1, FD2): Bass patterns - sparse, beat 1 dominant
- **Groove** (GR1, GR2): Mid-range patterns - complements foundation
- **Lead** (LD1, LD2): Accent/fill patterns - highest density

### 10 World Music Styles
0. West African (12/8 feel)
1. Afro-Cuban (Clave-based)
2. Brazilian (Samba)
3. Balkan (Odd meters)
4. Indian (Tala-based)
5. Gamelan (Colotomic)
6. Jazz (Swing)
7. Electronic (4/4)
8. Breakbeat (Syncopated)
9. Techno (Minimal)

## Module Parameters

### Global Controls (Clock Row)
- CLOCK input
- RESET input
- REGEN input + button
- VARI (Variation)
- HUMAN (Humanize)
- SWING
- REST + CV input
- FILL input + PROB + INTEN (Fill trigger and parameters)
- ARTIC (Articulation type: Normal, Ghost, Accent, Rim, Flam, Drag, Buzz, Ruff)
- GROOVE (Groove template: Auto, Straight, Swing, African, Latin, LaidBack, Pushed)
- GHOST (Ghost note amount)
- ACCENT (Accent probability)

### Per-Role Controls (x4)
- STYLE knob + CV (snap, displays style name)
- DENS knob + CV (Density)
- LEN knob (Length 4-32 steps, supports polymeter)
- FREQ knob + CV (per-role frequency modifier, -1 to +1 octave)
- DECAY knob + CV (per-role decay modifier, 0.2x to 2x)

### Outputs (White Area Y=330)
- 8 voices x 4 outputs each: AUD, GATE, CV (velocity), ACC (accent)
- MIX output (summed audio)

## Visual Features

### Dynamic Color System
- Role titles change color based on selected style
- Pattern display colors follow each role's style
- 10 distinct colors with good contrast:
  - West African: Warm coral
  - Afro-Cuban: Sky blue
  - Brazilian: Golden yellow
  - Balkan: Deep rose
  - Indian: Pink
  - Gamelan: Mint green
  - Jazz: Lavender
  - Electronic: Cyan
  - Breakbeat: Orange
  - Techno: Silver gray

### Style Name Display
- Shows current style name below role title
- Updates dynamically when style changes
- Uses same color as role title with white outline

### Panel Themes
- Sashimi (#FF8585) - default pink
- Boring (#333333) - dark gray
- Toilet Paper (#E2E2DD) - light beige

## Layout (Y coordinates)
- Title: Y=1
- Pattern Display: Y=42, height=50
- Clock Row: ctrlY=120, labels at Y=101
- Horizontal separator: Y=151
- Role Section: roleY=180
  - Role title: roleY-24
  - Style name: roleY-9
  - Style/Freq row: roleY+8
  - Dens/Decay row: roleY+5+knobVSpacing
  - Length row: roleY+2+knobVSpacing*2
- Vertical separators: Y=151 to Y=261
- White output area: Y=330 to Y=380

## Key Files
- `src/UniversalRhythm.cpp` - Main module implementation
- `src/WorldRhythm/PatternGenerator.hpp` - Pattern generation algorithm
- `src/WorldRhythm/StyleProfiles.hpp` - 10 style definitions
- `src/WorldRhythm/HumanizeEngine.hpp` - Humanization with swing
- `src/WorldRhythm/RestEngine.hpp` - Position-weighted rest
- `src/WorldRhythm/FillGenerator.hpp` - Fill pattern generation
- `src/WorldRhythm/MinimalDrumSynth.hpp` - Synthesis engine
- `res/UniversalRhythm.svg` (and _Boring, _ToiletPaper variants)

## New Features (v2.4.0)

### Fill Generator Integration
- FILL input: Trigger to initiate fill pattern
- PROB: Probability of fill occurring (0-100%)
- INTEN: Fill intensity (affects length and density)
- Style-specific fill types automatically selected
- Fills temporarily replace normal patterns

### Articulation Types
8 articulation types for expressive performance:
- **Normal**: Standard hit
- **Ghost**: Very soft (20% velocity)
- **Accent**: Emphasized (130% velocity)
- **Rim**: Rim shot feel
- **Flam**: Grace note + main note (~30ms apart)
- **Drag**: Two grace notes + main (~40ms total)
- **Buzz**: Multiple rapid triggers
- **Ruff**: Three grace notes + main (3-stroke ruff)

### Groove Templates
7 groove templates for microtiming feel:
- **Auto**: Style-dependent (default)
- **Straight**: Machine-like timing
- **Swing**: Upbeats pushed late
- **African**: Polyrhythmic feel
- **Latin**: Clave-based timing
- **LaidBack**: Behind the beat
- **Pushed**: Ahead of the beat

### Ghost and Accent Control
- GHOST: Global ghost note amount (applies to all roles with role-specific multipliers)
- ACCENT: Accent probability modifier (reduces/increases generated accents)

## TODO / Future Improvements
- [ ] Integrate AsymmetricGroupingEngine (7/8, 9/8, 11/8 meters)
- [ ] Integrate CrossRhythmEngine (cross-cultural rhythm layering)
- [ ] Integrate CallResponseEngine (call-response patterns)
- [ ] Integrate TalaEngine (Indian classical rhythm)
- [ ] Integrate IramaEngine (Gamelan tempo levels)
- [ ] Integrate KotekanEngine (Balinese interlocking)
- [ ] Integrate AmenBreakEngine (Amen break variations)
- [ ] Optimize pattern display performance
- [ ] Add per-voice mute/solo

## Version History
- v2.4.1: Bug fixes and improvements
  - Fixed Groove Template connection (now properly applies to HumanizeEngine)
  - Fixed Style CV handling in pattern regeneration functions
  - Unified Ghost Note application logic across all regeneration paths
  - Confirmed Accent logic is correct (accentProb determines percentage retained)
  - Verified Polymeter UI complete with per-role LEN knobs (4-32 steps)

- v2.4.0: Major feature expansion
  - Fill generator with style-specific fill types
  - 8 articulation types (Normal, Ghost, Accent, Rim, Flam, Drag, Buzz, Ruff)
  - 7 groove templates for microtiming
  - Global Ghost and Accent control
  - 21 CV inputs total (FILL + 4 per role)
  - Delayed trigger system for complex articulations

- v2.3.3: Initial implementation with full feature set
  - 40HP layout with 4-role sections
  - Dynamic color system based on style
  - Per-role Freq/Decay synthesis control
  - 16 CV inputs (4 per role: Style, Density, Freq, Decay)
  - 33 outputs (8 voices x 4 + MIX)
  - Style name display with real-time updates
