# VCV Rack Module Design Specification (MADZINE)

> **This document is knowledge for Claude Code to learn.**
> It describes the MADZINE-specific design language and conventions.
> **This is a REFERENCE for the MADZINE design system** — it reflects one team's design choices, not universal VCV Rack rules.
> Adapt these patterns to your own project's visual identity.

Version: 2.5.0
Last Updated: 2026-02-21

---

## 1. Text Label Specification

### Font Sizes
| Purpose | Font Size | Bold | Notes |
|---------|-----------|------|-------|
| Module title | 14.f | true | Standard size |
| Module title (long name) | 10.f | true | e.g., PPaTTTerning |
| Brand label (MADZINE) | 10.f | false | |
| Functional labels | 8.f | true | Y < 330 region |
| Output area labels | 7.f | true | Y >= 330 region |
| Background decorative text | 32.f | true | e.g., Pyramid X/Y/Z |

### Color Specification
| Context | RGB | When to Use |
|---------|-----|-------------|
| Orange (brand color) | (255, 200, 0) | Module title, brand logo |
| White | (255, 255, 255) | Labels on dark background |
| Black | (0, 0, 0) | Labels in white area |
| Pink | (255, 133, 133) | Output port labels |
| Light gray | (160, 160, 160) | Background decorative text |

### Color Decision Tree
```
Y < 330 (dark background)
+-- Title/Brand -> Orange
+-- General controls -> White
+-- Background decoration -> Light gray (160,160,160)

Y >= 330 (white background)
+-- Output -> Pink
+-- Other -> Black
```

### Background Decorative Labels (Pyramid/DECAPyramid X/Y/Z)

**Pyramid:**
```
Font size: 32.f
Color: nvgRGB(160, 160, 160)
Bold: true
Add order: Labels BEFORE knobs, so knobs render on top
```

**DECAPyramid:**
```
Font size: 80.f
Color: nvgRGB(160, 160, 160)
Outline: Black 2px (using OutlinedTextLabel)
Y coordinates: X=80, Y=145, Z=215
Add order: Labels BEFORE knobs, so knobs render on top
```

---

## 2. Knob Specification

### Knob Types
| Type | Diameter | Radius | Label Offset | Purpose |
|------|----------|--------|-------------|---------|
| LargeWhiteKnob | 37px | 18.5px | 32px | EQ large knob |
| StandardBlackKnob | 30px | 15px | 28px | Primary parameter |
| StandardBlackKnob26 | 26px | 13px | 26px | Compact primary parameter |
| WhiteKnob | 30px | 15px | 28px | CV/modulation parameter |
| MediumGrayKnob | 26px | 13px | 26px | Secondary parameter |
| SmallGrayKnob | 21px | 10.5px | 24px | Tight spaces |
| SnapKnob | 26px | 13px | 26px | Discrete value selection |
| MicrotuneKnob | 20px | 10px | 23px | Fine-tune control |

### Label Offset Formula
```
Label-to-knob offset = knob_radius + label_height(10) + gap(3)
Label box X = knob_center_X - (label_box_width / 2)
```

### Knob Selection Decision Tree
```
Control type?
+-- Discrete value -> SnapKnob / MADDYSnapKnob
+-- Continuous value
|   +-- Primary parameter -> StandardBlackKnob / StandardBlackKnob26
|   +-- CV/modulation -> WhiteKnob / LargeWhiteKnob
|   +-- Secondary parameter -> MediumGrayKnob
|   +-- Fine-tune -> MicrotuneKnob
+-- Hidden control -> HiddenKnob series
```

---

## 3. Custom defaultValue (Double-Click Reset)

When a parameter needs a non-standard defaultValue (e.g., Speed=0.5 means 1x), you must:

1. **Use a custom ParamQuantity**
2. **Manually set all fields**
3. **onDoubleClick uses setValue(getDefaultValue())**

### Setup
```cpp
// 1. Define custom ParamQuantity
struct SpeedParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        return knobToSpeed(getValue());  // Custom display conversion
    }
};

// 2. Replace in Module constructor
configParam(SPEED_PARAM, 0.0f, 1.0f, 0.5f, "Speed", "x");
delete paramQuantities[SPEED_PARAM];
paramQuantities[SPEED_PARAM] = new SpeedParamQuantity;
paramQuantities[SPEED_PARAM]->module = this;
paramQuantities[SPEED_PARAM]->paramId = SPEED_PARAM;
paramQuantities[SPEED_PARAM]->minValue = 0.0f;
paramQuantities[SPEED_PARAM]->maxValue = 1.0f;
paramQuantities[SPEED_PARAM]->defaultValue = 0.5f;  // Key: set default
paramQuantities[SPEED_PARAM]->name = "Speed";
paramQuantities[SPEED_PARAM]->unit = "x";
```

### KnobBase.hpp onDoubleClick Implementation
```cpp
void onDoubleClick(const event::DoubleClick& e) override {
    if (enableDoubleClickReset) {
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            // Important: use setValue + getDefaultValue, NOT reset()
            pq->setValue(pq->getDefaultValue());
            e.consume(this);
            return;
        }
    }
    app::Knob::onDoubleClick(e);
}
```

**Note**: Do NOT use `pq->reset()` — it may not use the custom defaultValue.

---

## 4. Y=330 White Block

### Rules
- Y >= 330 is the white I/O area
- All input/output ports go here
- Labels use black or pink

### Standard Y Coordinates
```
330: White area start
343: I/O row 1 center
368: I/O row 2 center
380: Module bottom
```

### X Coordinate Reference
| HP | Width | Left | Center | Right |
|----|-------|------|--------|-------|
| 4HP | 60px | 15 | 30 | 45 |
| 8HP | 120px | 15 | 60 | 105 |
| 12HP | 180px | 30 | 90 | 150 |

---

## 5. Design Checklist

### Visual
- [ ] Title: Orange (255,200,0), 14.f, bold (long names can use 12.f)
- [ ] Brand: Orange, 10.f, non-bold
- [ ] Functional labels: White, 8.f, bold
- [ ] Output area labels: Pink, 7.f, bold
- [ ] Y=330 uses WhiteBottomPanel
- [ ] White area labels use black or pink

### Controls
- [ ] Primary parameters: StandardBlackKnob series
- [ ] CV parameters: WhiteKnob series
- [ ] Discrete values: SnapKnob
- [ ] Label positions correct (using offset formula)

### Code
- [ ] Context menu includes addPanelThemeMenu()
- [ ] dataToJson/dataFromJson saves panelTheme
- [ ] Custom defaultValue uses correct approach

### Layout
- [ ] All I/O in white area
- [ ] Controls in dark area
- [ ] Labels above controls

---

## 6. Reference Modules

| Purpose | Reference Module |
|---------|-----------------|
| 8HP compact layout | MADDY, EuclideanRhythm |
| 12HP standard layout | weiii documenta, NIGOQ |
| I/O area design | U8, Observer |
| Custom defaultValue | weiii documenta (Speed, Poly) |

---

## Appendix: Related Files

- `src/widgets/KnobBase.hpp` — Knob base class (includes double-click reset)
- `src/widgets/Knobs.hpp` — Knob definitions
- `src/widgets/PanelTheme.hpp` — Theme switching system
