# VCV Rack UI Layout Specification

> **This document is knowledge for Claude Code to learn.**
> It provides complete UI layout rules for VCV Rack module development,
> derived from the MADZINE project's experience.
> All paths use placeholders — replace with your actual directories.

Version: 1.1
Last Updated: 2026-01-21

---

## 1. Basic Constants

```cpp
// VCV Rack standard constants
RACK_GRID_WIDTH  = 15.24px  // Width per HP
RACK_GRID_HEIGHT = 380px    // Standard module height

// HP to pixel width mapping
4HP  = 60.96px
8HP  = 121.92px
12HP = 182.88px
16HP = 243.84px
32HP = 487.68px
40HP = 609.6px
```

---

## 2. Label-to-Component Relative Positioning

### Core Rule

```
Label Y = Component Y - 24px (label 24px above component)

Example:
Knob Y = 123 -> Label Y = 99
Port Y = 59  -> Label Y = 35
```

### Vertical Spacing

| Relationship | Spacing |
|-------------|---------|
| Label to Knob | 24px |
| Label to Port | 24px |
| Knob to CV Input | 38px |
| Port to Port (vertical) | 25px |
| Row 1 to Row 2 | 25px |

### Horizontal Spacing

| Component | Spacing |
|-----------|---------|
| Left-Right Ports (same track) | 30px |
| Knob to CV Input | 28px |
| Between tracks | trackWidth |

---

## 3. White Output Area Specification

```
Y start: 330
Height: 50-60px
Y end: 380 (RACK_GRID_HEIGHT)

Row 1: Y = 343
Row 2: Y = 368
Spacing: 25px
```

Background color: `nvgRGB(255, 255, 255)` white

---

## 4. Title Area Specification

```
Module title: Y = 1-2, fontSize = 14.f (standard) or 12.f (long names)
Subtitle "MADZINE": Y = 13-16, fontSize = 10.f, color = #FFC800 (gold)
```

---

## 5. Font Size Standards

| Purpose | Font Size | Bold | Notes |
|---------|-----------|------|-------|
| Module title | 14.f | true | Standard size |
| Module title (long name) | 10.f | true | e.g., PPaTTTerning |
| Subtitle MADZINE | 10.f | false | |
| Functional labels | 8.f | true | Y < 330 region |
| Output area labels | 7.f | true | Y >= 330 region, pink |
| Background decorative | 32.f | true | e.g., Pyramid X/Y/Z, light gray |

### Background Decorative Label Specification (Pyramid/DECAPyramid)

**Pyramid (32.f, no outline):**
```cpp
// Large background text, knobs render on top layer
addChild(new TechnoEnhancedTextLabel(Vec(7, 75), Vec(50, 10), "X", 32.f, nvgRGB(160, 160, 160), true));
// Then add knobs
addParam(createParamCentered<StandardBlackKnob26>(Vec(17, 95), module, X_PARAM));
```

**DECAPyramid (80.f with black outline):**
```cpp
// OutlinedTextLabel for large background text with black outline
addChild(new OutlinedTextLabel(Vec(7, 80), Vec(50, 10), "X", 80.f, nvgRGB(160, 160, 160), 2.f));
addChild(new OutlinedTextLabel(Vec(7, 145), Vec(50, 10), "Y", 80.f, nvgRGB(160, 160, 160), 2.f));
addChild(new OutlinedTextLabel(Vec(7, 215), Vec(50, 10), "Z", 80.f, nvgRGB(160, 160, 160), 2.f));
// Then add knobs
addParam(createParamCentered<StandardBlackKnob26>(Vec(30, 95), module, X_PARAM));
```

---

## 6. Track Designs by HP Size

### 4HP (60.96px)

```
Center X = 30
Left Port: X = 15
Right Port: X = 45
Horizontal spacing: 30px
```

### 8HP (121.92px)

```
2-track design: each track 60.96px
Track 1 center: X = 30
Track 2 center: X = 91
```

### 16HP (243.84px)

```
4-track design: each track 60.96px
Track center X: 30, 91, 152, 213

Or compact design: each track 55px
Track center X: 30, 85, 140, 195
```

### 32HP (487.68px)

```
8-track design: each track 60.96px
Track center X = 30 + t * 60.96
```

### 40HP (609.6px)

```
4-role design: each role 152px
Role center X = 76 + role * 152
```

---

## 7. Standard Vertical Positions (ALEXANDERPLATZ Reference)

```
Title area: Y = 1-18
INPUT label: Y = 35
INPUT Ports: Y = 59
VU Meter: Y = 71, 79
LEVEL label: Y = 89
LEVEL knob: Y = 123
LEVEL CV: Y = 161
DUCK label: Y = 182
DUCK knob: Y = 216
DUCK Input: Y = 254
MUTE/SOLO label: Y = 270
Button: Y = 292
Trigger Input: Y = 316
White area start: Y = 330
Row 1: Y = 343
Row 2: Y = 368
```

---

## 8. Component Sizes

### Port

```
PJ301MPort: ~24px diameter
Center-aligned (createCentered)
```

### Knobs

```
SmallWhiteKnob: ~22px
SmallGrayKnob: ~22px
MediumGrayKnob: ~26px
StandardBlackKnob: ~30px
```

### Buttons

```
VCVButton: ~16px
LightLatch: ~20px
```

---

## 9. Layout Examples

### Single Track Structure (top to bottom)

```cpp
float centerX = 30;  // 4HP center

// Label 24px above component
addChild(new TextLabel(Vec(centerX - 15, Y - 24), "LABEL"));
addParam(createParamCentered<Knob>(Vec(centerX, Y), module, PARAM_ID));
addInput(createInputCentered<PJ301MPort>(Vec(centerX, Y + 38), module, INPUT_ID));
```

### Multi-Track Structure

```cpp
float trackWidth = 4 * RACK_GRID_WIDTH;  // 60.96px

for (int t = 0; t < TRACKS; t++) {
    float trackX = t * trackWidth;
    float centerX = trackX + trackWidth / 2;

    // Label
    addChild(new TextLabel(Vec(trackX, Y - 24), "LABEL"));
    // Component
    addParam(createParamCentered<Knob>(Vec(centerX, Y), module, PARAM + t));
}
```

### White Output Area

```cpp
float outputY = 330;
addChild(new WhiteBox(Vec(0, outputY), Vec(box.size.x, 50)));

float row1Y = 343;
float row2Y = 368;

// Left side input
addInput(createInputCentered<PJ301MPort>(Vec(15, row1Y), module, CHAIN_L));
addInput(createInputCentered<PJ301MPort>(Vec(15, row2Y), module, CHAIN_R));

// Right side output
addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, row1Y), module, OUT_L));
addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, row2Y), module, OUT_R));
```

---

## 10. Common Errors & Fixes

### Error 1: Label Hidden Behind Knob

```
Wrong: Label Y = Knob Y + 10 (label below)
Correct: Label Y = Knob Y - 24 (label above)
```

### Error 2: Component Outside Panel

```
Wrong: X calculation ignoring boundaries
Correct: Ensure 15 <= X <= box.size.x - 15
```

### Error 3: Inconsistent Output Positioning

```
Wrong: Output ports placed arbitrarily
Correct: Use fixed row1Y=343, row2Y=368
```

### Error 4: Using Abbreviations

```
Wrong: TL, FD, GR, LD, F, D
Correct: TIMELINE, FOUNDATION, GROOVE, LEAD, FREQ, DECAY
Abbreviations are NEVER allowed, even if space is tight
```

---

## 11. Role/Track Display Order

For multi-role modules (e.g., UniversalRhythm), display order is:

```
UI order: Foundation, Timeline, Groove, Lead
Role index: 1, 0, 2, 3

const int roleOrder[4] = {1, 0, 2, 3};
for (int uiPos = 0; uiPos < 4; uiPos++) {
    int role = roleOrder[uiPos];
    // Use uiPos for X position calculation
    // Use role for enum indexing
}
```

---

## 12. Panel Theme Support

All modules must support panel themes:

```cpp
// Module class
int panelTheme = -1;
float panelContrast = panelContrastDefault;

json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
    json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
    return rootJ;
}

// Widget class
PanelThemeHelper panelThemeHelper;

// Constructor
panelThemeHelper.init(this, "16HP", module ? &module->panelContrast : nullptr);

// step()
panelThemeHelper.step(module);

// appendContextMenu()
addPanelThemeMenu(menu, module);
```
