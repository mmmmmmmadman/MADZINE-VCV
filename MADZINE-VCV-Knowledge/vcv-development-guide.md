# VCV Rack Module Development Guide (MADZINE)

> **This document is knowledge for Claude Code to learn.**
> It captures development experience, conventions, and lessons learned from the MADZINE VCV Rack plugin project.
> All paths use placeholders — replace them with your actual project directories.

Version: 2.5.2
Last Updated: 2026-02-21

---

## Path Placeholders

| Placeholder | Description |
|------------|-------------|
| `<VCV_PROJECT_DIR>` | Your VCV Rack plugin source directory |
| `<RACK_SDK_DIR>` | VCV Rack SDK location (e.g., `~/Rack-SDK`) |
| `<VCV_UPLOAD_DIR>` | GitHub upload directory for the plugin |

---

## Mandatory Agent Usage Rules

**Important**: The following tasks MUST use their corresponding specialized agents. Do NOT handle them manually.

| Task Type | Agent | Description |
|-----------|-------|-------------|
| UI layout design & modification | `vcv-layout` | Widget component positioning, spacing, label configuration |
| Audio processing architecture | `audio-processing` | Audio engine, effects, multi-device output |
| C++ core architecture | `cpp-core-architect` | JUCE project extraction, cross-platform core library |
| GPU rendering optimization | `gpu-optimization` | Shaders, render pipelines, blend modes |
| MIDI control integration | `midi-control` | MIDI Learn, CC/Note mapping |

### Usage

When a task requires a specific agent, invoke it via the Agent tool:
```
Agent tool with subagent_type="vcv-layout"
```

### Prohibited Actions

- **Do NOT** modify code yourself after the user specifies an agent
- **Do NOT** skip agent usage claiming it's a "small change"
- **Do NOT** attempt the task yourself before using the agent

### Violation Case Record

**UniRhythm UI Layout Issue (2026-01-20)**:
- User explicitly said "use our UI Agent, don't do it yourself"
- Error: Modified code directly without using agent
- Consequence: Layout errors requiring multiple corrections
- Lesson: When a user specifies an agent, use it immediately without exception

---

## Manual Documentation Update Rules

**Important**: After modifying `Manual/modules/*.json`, you **MUST immediately run in order**:

```bash
python3 Manual/generate_help_data.py   # Update VCV in-module tooltips
cd Manual && python3 build_manual.py   # Update HTML documentation
```

This applies to any JSON content change: adding translations, modifying descriptions, etc. Verify output has no errors before continuing.

---

## Build Instructions

### Standard Workflow
```bash
export RACK_DIR=<RACK_SDK_DIR>
make clean && make && make install
```

### Common Commands
```bash
# Build only (no install)
make

# Incremental build + install
make && make install

# Check errors
make 2>&1 | grep -A5 "error:"

# Release build
make dist
```

---

## Project Structure

```
<VCV_PROJECT_DIR>/
+-- src/
|   +-- plugin.hpp              # Main header (extern Model* declarations)
|   +-- plugin.cpp              # Module registration
|   +-- widgets/
|   |   +-- KnobBase.hpp        # Knob base class
|   |   +-- Knobs.hpp           # Knob definitions
|   |   +-- PanelTheme.hpp      # Theme system
|   +-- [ModuleName].cpp        # Individual module implementations
+-- res/                        # SVG panels
+-- Manual/                     # Interactive documentation (JSON + HTML)
+-- plugin.json                 # Module registry
+-- Makefile
+-- MADZINE_DESIGN_SPECIFICATION.md
+-- VCV_UI_SPECIFICATION.md
```

---

## Adding a New Module

1. **Create module file**
   ```bash
   cp src/ExistingModule.cpp src/NewModule.cpp
   ```

2. **Register the module**
   - `src/plugin.hpp`: `extern Model* modelNewModule;`
   - `src/plugin.cpp`: `p->addModel(modelNewModule);`

3. **Update plugin.json**
   ```json
   {
     "slug": "NewModule",
     "name": "New Module",
     "description": "Description",
     "tags": ["Tag1"]
   }
   ```

4. **Create SVG panel**
   ```bash
   cp res/ExistingModule.svg res/NewModule.svg
   ```

5. **Build and test**
   ```bash
   make && make install
   ```

---

## Dependencies

- **Rack SDK**: VCV Rack core API
- **SST Filters**: Surge XT filters (oversampling)
- **SST Basic Blocks**: Surge XT basic components

---

## Version Number Convention

`Major.Minor.Patch` (e.g., 2.3.6)
- Major: Breaking changes
- Minor: New modules or features
- Patch: Bug fixes

Update location: `"version"` field in `plugin.json`

---

## Release Process

1. Test all functionality
2. Update version in `plugin.json`
3. `make dist`
4. Push to GitHub
5. Wait for VCV Library automated build

---

## DSP Design Principles

### Dry/Wet Mix Rules

- Unless there is a dedicated dry/wet knob, always implement as **100% wet**
- If your implementation uses dry/wet mixing (crossfade, dry/wet blend), confirm with the user first
- Do NOT silently add implicit dry/wet crossfading to "soften" an effect

---

## Known Issues & Notes

### Manual HTML Click Functionality Disappearing

**Problem**: When modifying the HTML manual, clicking small cards to open large cards can break due to JavaScript syntax errors.

**Root Causes**:
1. Deleting HTML elements while their corresponding JavaScript still references them, causing `null` errors
2. Removing code blocks while accidentally dropping opening/closing braces `{}`
3. Translation strings with multiline content must use `\n` instead of actual newlines

**Prevention**:
- After modifying HTML structure, always sync the corresponding JavaScript
- After each modification, run JavaScript syntax validation:
  ```bash
  awk 'NR>=3122 && NR<=3887' manual_file.html > /tmp/js.js && node --check /tmp/js.js
  ```
- Translation strings must remain single-line format

### Manual HTML Dual-Card CSS Stacking Issue

**Problem**: When switching between Manual (red) and Quick Reference (blue) dual cards, the bottom card's size and hover effects are inconsistent.

**Root Causes**:
1. **CSS specificity**: `.card-stack.has-quickref .stacked-card.card-front` (0,0,4,0) is higher than `.stacked-card.card-front.inactive` (0,0,3,0), causing style overrides
2. **transform property conflict**: Setting only `translate()` on hover without `scale()` overrides the base state scaling, causing the card to suddenly enlarge

**Solution**:
1. Red card (card-front) when on bottom:
   - Base: `transform: scale(0.92) translate(3px, 3px)`
   - Hover: `transform: scale(0.92) translate(25px, 25px)`
   - Must use `.card-stack.has-quickref .stacked-card.card-front.inactive` selector for higher specificity
2. Blue card (card-back) when on bottom:
   - Base: `transform: translate(5px, 5px)`
   - Hover: `transform: translate(15px, 15px)`

**Key**: When modifying hover transforms, always retain all transform functions from the base state (scale, translate, etc.)

### Manual Version Number Updates

**Important**: Update version number after each Manual content modification to avoid overwriting older versions.

**Update location**: End of `Manual/build_manual.py`
```python
with open('module_manual_vX.X.html', 'w', encoding='utf-8') as f:
    ...
print("Generated: module_manual_vX.X.html")
```

### UniversalRhythm Array Out-of-Bounds Crash (Fixed in v2.3.7)

**Problem**: VCV Rack crashes ~4 seconds after launch with `EXC_BAD_ACCESS (SIGBUS)` / `KERN_PROTECTION_FAILURE`.

**Root Causes**:
1. **`accents[useStep]` direct index overflow**: When `fillActive = true`, `useStep` may exceed the `accents` vector's actual size
2. **`fillStep` uses wrong pattern length**: Uses fixed `fillPatterns.patterns[0].length` instead of the current role's pattern length

**Fix**:
```cpp
// Fix 1: Safe index
bool accent = primaryPattern.accents[useStep % primaryPattern.length];

// Fix 2: Use corresponding role's pattern length + bounds checking
int fillPatternLen = static_cast<int>(fillPatterns.patterns[voiceBase].length);
int fillStep = fillActive ? (fillPatternLen - fillStepsRemaining) : step;
if (fillActive && fillStep < 0) fillStep = 0;
if (fillActive && fillStep >= fillPatternLen) fillStep = fillPatternLen - 1;
```

**Prevention**:
- When accessing `std::vector` with externally computed indices, always use `% length` or bounds checking
- When multiple patterns have different lengths, ensure you use the correct pattern length for index calculation
- Fill pattern lengths may differ from normal pattern lengths — pay special attention

---

## References

### Internal Documents
- `MADZINE_DESIGN_SPECIFICATION.md` — Design spec (knobs, labels, layout)
- `VCV_UI_SPECIFICATION.md` — Complete VCV Rack UI layout specification

### External Resources
- [VCV Rack Official Documentation](https://vcvrack.com/manual/)
- [Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial)

---

## Contact

- GitHub: https://github.com/mmmmmmmadman/MADZINE-VCV
- Email: madzinetw@gmail.com
