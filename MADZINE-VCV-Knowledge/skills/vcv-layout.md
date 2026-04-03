---
name: vcv-layout
description: VCV Rack module label offset rules and boundary checking
---

> **This is a Claude Code skill definition for another Claude Code instance to install.**
> Place this file in `<YOUR_PROJECT_DIR>/.claude/commands/vcv-layout.md`

# VCV Rack Label Offset Rules & Boundary Checking

---

## Label Type Definitions

Only three label types are used. Do NOT invent new types.

### Type A: Functional Label

- **Purpose**: Functional description in dark area (Y < 330)
- **Font size**: 8.f
- **Bold**: true
- **Color**: White nvgRGB(255, 255, 255)
- **Examples**: INPUT, LEVEL, DUCK, MUTE, SOLO, DECAY, SHAPE, FREQ, TIMELINE, FOUNDATION, GROOVE, LEAD

### Type B: Output Area Label

- **Purpose**: Output/IO description in white area (Y >= 330)
- **Font size**: 7.f
- **Bold**: true
- **Color**: Pink nvgRGB(255, 133, 133) or Black nvgRGB(0, 0, 0)
- **Examples**: OUT, CHAIN, MIX

### Type C: Background Decorative Text

- **Purpose**: Large background decoration (e.g., Pyramid X/Y/Z)
- **Font size**: 32.f (Pyramid) or 80.f (DECAPyramid)
- **Bold**: true
- **Color**: Light gray nvgRGB(160, 160, 160)
- **Add order**: Must be added BEFORE knobs, so knobs render on top
- **DECAPyramid special**: Uses OutlinedTextLabel with black 2px outline

---

## Label Offset Calculation Table

### Type A Label Offset (Functional Label 8.f)

| Component Type | Component Radius | Offset | Label Y Formula |
|---------------|-----------------|--------|----------------|
| PJ301MPort | 12px | 24px | Component Y - 24 |
| StandardBlackKnob (30px) | 15px | 28px | Component Y - 28 |
| StandardBlackKnob26 (26px) | 13px | 26px | Component Y - 26 |
| LargeWhiteKnob (37px) | 18.5px | 32px | Component Y - 32 |
| WhiteKnob (30px) | 15px | 28px | Component Y - 28 |
| MediumGrayKnob (26px) | 13px | 26px | Component Y - 26 |
| SmallGrayKnob (21px) | 10.5px | 24px | Component Y - 24 |
| MicrotuneKnob (20px) | 10px | 23px | Component Y - 23 |

Note: The "offset" above is the distance from label pos.y (box top) to component center. The actual label box typically has 10-20px height, with text rendered at box center. Therefore:

```
Label box pos.y = Component Y - offset (simplified)
In practice, reference modules commonly use: label pos.y = Component Y - 24 (unified 24px)
```

### Type B Label Offset (Output Area Label 7.f)

White area labels are typically placed at fixed positions, not directly offset from components. Reference:
```
Example: Vec(5, 335) for label, Port at Vec(45, 343)
```

---

## Boundary Checking Rules

### X-Axis Boundary

```
Panel width = HP * RACK_GRID_WIDTH (15.24px)

4HP:  60.96px  -> X: 15 ~ 45
8HP:  121.92px -> X: 15 ~ 105
12HP: 182.88px -> X: 15 ~ 165 (or 30 ~ 150)
16HP: 243.84px -> X: 15 ~ 228
32HP: 487.68px -> X: 15 ~ 472
40HP: 609.6px  -> X: 15 ~ 594
```

Component center X must satisfy:
```
centerX >= 15
centerX <= panel_width - 15
```

Labels CAN extend beyond panel boundary (text center-renders visually within panel).

### Y-Axis Boundary

```
Module height = 380px

Title area:   Y = 0~30
Control area: Y = 30~330
White I/O area: Y = 330~380
```

Controls must NOT enter white area (Y >= 330). I/O components must NOT be in dark area (Y < 330).

### Color Boundary

```
Y < 330:
  Title/Brand -> Orange (255, 200, 0)
  Functional labels -> White (255, 255, 255)
  Background decoration -> Light gray (160, 160, 160)

Y >= 330:
  Output labels -> Pink (255, 133, 133)
  Other labels -> Black (0, 0, 0)
```

---

## Label Horizontal Centering Rules

### Single Component Label

Label box center X should align with component center X:

```
label_pos_x = component_centerX - label_size_x / 2
```

### Multi-Component Shared Label

Labels spanning multiple components can use a wider size.x:

```cpp
// Example: MUTE/SOLO label spanning two buttons
addChild(new TextLabel(Vec(-5, 270), Vec(box.size.x + 10, 10), "MUTE     SOLO", ...));
```

### Full-Width Label

Some labels use full panel width:

```cpp
addChild(new TextLabel(Vec(0, 28), Vec(box.size.x, 16), "INPUT", ...));
```
