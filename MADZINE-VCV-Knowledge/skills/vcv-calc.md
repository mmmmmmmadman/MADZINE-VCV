---
name: vcv-calc
description: VCV Rack module space calculation and overlap pre-check rules
---

> **This is a Claude Code skill definition for another Claude Code instance to install.**
> Place this file in `<YOUR_PROJECT_DIR>/.claude/commands/vcv-calc.md`

# VCV Rack Space Calculation & Overlap Pre-Check

These rules define mandatory pre-calculation verification steps that must be executed BEFORE writing any layout code.

---

## Complete Pre-Calculation Verification Steps

### Step 1: Build Component Configuration Table

List all components in this format:

```
Component Name | Type | Center X | Center Y | X Range (left~right) | Y Range (top~bottom)
```

X/Y range calculations:
- Port: center +/- 12px
- 30px knob: center +/- 15px
- 26px knob: center +/- 13px
- Label: pos.x + size.x/2 +/- textWidth/2, pos.y + size.y/2 +/- fontSize/2

### Step 2: Boundary Checking

For each component:

```
X boundary:
  Component left edge >= 0 (ideal >= 3)
  Component right edge <= panel_width (ideal <= panel_width - 3)
  Component center X >= 15
  Component center X <= panel_width - 15

Y boundary:
  Component top edge >= 0
  Component bottom edge <= 380
  Dark area components: Y < 330
  White area components: Y >= 330
```

### Step 3: Overlap Detection

#### 3a. Label-Component Overlap

For each label, check against all components added AFTER it (higher z-order):

```
Label rectangle:
  L_left   = pos.x + size.x/2 - textWidth/2
  L_right  = pos.x + size.x/2 + textWidth/2
  L_top    = pos.y + size.y/2 - fontSize/2
  L_bottom = pos.y + size.y/2 + fontSize/2

  textWidth = character_count x fontSize x 0.43

Component rectangle:
  C_left   = centerX - radius
  C_right  = centerX + radius
  C_top    = centerY - radius
  C_bottom = centerY + radius

Overlap condition:
  L_left < C_right AND L_right > C_left AND
  L_top < C_bottom AND L_bottom > C_top
```

If overlapping, output:
```
[OVERLAP] Label "XXX" overlaps with component YYY
  Label range: (L_left, L_top) ~ (L_right, L_bottom)
  Component range: (C_left, C_top) ~ (C_right, C_bottom)
```

#### 3b. Component-Component Overlap

Check minimum spacing between adjacent components:

```
Horizontal spacing = |centerX_A - centerX_B|
Vertical spacing = |centerY_A - centerY_B|

Port-Port minimum horizontal spacing: 26px
Knob-Knob minimum horizontal spacing: diameter + 2px
Port-Port minimum vertical spacing: 25px
```

#### 3c. Label-Label Overlap

Check text range overlap between adjacent labels:

```
Text width = character_count x fontSize x 0.43
Overlap condition uses same rectangle overlap test
```

### Step 4: Output Verification Results

```
=== Pre-Calculation Verification Results ===
Boundary check: PASS / FAIL (list violations)
Label-component overlap: PASS / FAIL (list violations)
Component-component spacing: PASS / FAIL (list violations)
Label-label overlap: PASS / FAIL (list violations)
```

### Step 5: Fix and Re-verify

If any failures:
1. Adjust coordinates
2. Re-run steps 1-4
3. Repeat until all pass

---

## Repeating Block Special Checks

When a module has repeating blocks (e.g., multiple voices, multiple tracks), additionally check:

```
1. Calculate single block occupied height:
   occupied_height = block_bottom_element_bottom - block_top_element_top

2. Confirm spacing > occupied height:
   spacing = rowHeight or blockSpacing
   gap = spacing - occupied_height
   Required: gap >= 6px (recommended)

3. Confirm last block doesn't exceed boundary:
   lastBlockBottom = startY + (count-1) * spacing + blockHeight
   Required: lastBlockBottom <= 330 (or white area start position)
```
