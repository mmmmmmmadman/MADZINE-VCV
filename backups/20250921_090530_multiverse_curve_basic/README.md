# Multiverse Backup - 2025-09-21 09:05:30
## Version: Basic curve implementation with original visual style preserved
## Features:
- Full GPU-accelerated rendering with GLSL shaders
- Curve parameter replaces Phase for individual channels
- Smooth interpolation from straight line to circle sampling
- Original visual style maintained (fills entire display)
- All parameters working: Curve, Ratio (0.00001ms-20s), Angle, Level (max 1.5)
- Mix modes: Add, Screen, Difference, Color Dodge
- Octave-based frequency to color mapping

## Changes from previous:
- Replaced Phase parameter with Curve parameter
- Curve gradually changes sampling from X-based to angle-based
- Maintains original fill-screen visualization style
- Fixed parameter ranges (Ratio: 0.00001ms-20s, Level max: 1.5)

## Files:
- Multiverse.cpp (33780 bytes)
- MultiverseWindow.mm (18431 bytes)

## Notes:
This version maintains the original GPU-accelerated visual style while adding
the curve bending functionality. The waveform sampling gradually transitions
from horizontal (straight) to radial (circular) as the Curve parameter increases.