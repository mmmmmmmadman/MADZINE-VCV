# MetaModule Porting Guide (MADZINE)

> **This document is knowledge for Claude Code to learn.**
> It captures the complete experience of porting VCV Rack modules to the 4ms MetaModule hardware platform.
> All paths use placeholders — replace them with your actual project directories.

Version: 2.6.0
Last Updated: 2026-03-31

---

## Path Placeholders

| Placeholder | Description |
|------------|-------------|
| `<METAMODULE_PROJECT_DIR>` | Your MetaModule plugin source directory |
| `<METAMODULE_SDK_DIR>` | 4ms MetaModule SDK / simulator directory |

---

## Build Commands

```bash
cd <METAMODULE_PROJECT_DIR>
cmake --build build
```

### Test in Simulator

```bash
cd <METAMODULE_SDK_DIR>/simulator
timeout 8 ./build/simulator
```

### Force Rebuild Panel Assets (Fix Panel Cache Issues)

```bash
cd <METAMODULE_SDK_DIR>/simulator
rm -rf build/assets build/assets.uimg
cmake --build build
```

---

## Project Structure

```
<METAMODULE_PROJECT_DIR>/
+-- src/
|   +-- plugin.hpp              # Main header (extern Model* declarations)
|   +-- plugin.cpp              # Module registration (p->addModel)
|   +-- [ModuleName].cpp        # Individual module implementations
+-- assets/                     # PNG panels (height 240px, width varies by HP)
+-- CMakeLists.txt              # Build configuration
```

---

## Porting Rules

### 1. Memory Management (Most Important)

MetaModule does NOT support dynamic memory allocation:

```cpp
// FORBIDDEN
std::vector<float> buffer;
std::string name;

// USE FIXED ARRAYS INSTEAD
static constexpr int MAX_BUFFER = 48000;
float buffer[MAX_BUFFER];
int bufferSize = 0;
char name[64];
```

### 2. Custom Widgets Cause UI Offset

**Problem**: Widgets inheriting from TransparentWidget or Widget cause MetaModule UI element offset.

**Solution**: Remove all custom widgets, keep only core module functionality.

```cpp
// REMOVE THESE
struct WaveformDisplay : TransparentWidget { ... };
struct CustomLight : GreenRedLight { ... };

// USE STANDARD COMPONENTS
addChild(createLightCentered<MediumLight<GreenRedLight>>(...));
```

### 3. Unsupported Features

- `osdialog` (file dialogs)
- Dynamic string operations
- Complex Widget inheritance

---

## Adding a New Module

1. **Copy and modify source**
   ```bash
   cp /path/to/original/Module.cpp src/Module.cpp
   # Apply porting rules
   ```

2. **Create panel**
   - Convert from VCV version's `.img` file (data URI base64 PNG)
   - Height fixed at 240px, width by HP: 4HP=38, 8HP=76, 12HP=113, 16HP=152, 32HP=304, 40HP=380
   - Format: PNG
   - Place at `assets/ModuleName.png`

3. **Register the module**
   - `src/plugin.hpp`: `extern Model* modelModuleName;`
   - `src/plugin.cpp`: `p->addModel(modelModuleName);`
   - `CMakeLists.txt`: add `src/ModuleName.cpp`

4. **Build and test**
   ```bash
   cmake --build build
   # Test in simulator
   cd <METAMODULE_SDK_DIR>/simulator
   timeout 8 ./build/simulator
   ```

---

## Ported Modules List (34 modules as of v2.6.0)

| Module | Status | Notes |
|--------|--------|-------|
| SwingLFO | Done | |
| EuclideanRhythm | Done | |
| ADGenerator | Done | |
| Pinpple | Done | |
| MADDY | Done | |
| PPaTTTerning | Done | |
| TWNC | Done | |
| TWNCLight | Done | UI offset fixed |
| QQ | Done | |
| Observer | Done | |
| TWNC2 | Done | |
| U8 | Done | |
| YAMANOTE | Done | |
| Obserfour | Done | |
| KIMO | Done | |
| Quantizer | Done | |
| EllenRipley | Done | |
| MADDYPlus | Done | |
| EnvVCA6 | Done | |
| NIGOQ | Done | |
| Runshow | Done | |
| DECAPyramid | Done | |
| KEN | Done | |
| Launchpad | Done | |
| Pyramid | Done | |
| SongMode | Done | |
| UniversalRhythm | Done | UI offset + vector fixed |
| WeiiiDocumenta | Done | 10s recording limit, v2.6.0 added Load WAV (async_open_file) |
| Facehugger | Done | v2.5.0 |
| Ovomorph | Done | v2.5.0 |
| Runner | Done | v2.5.0 |
| theKICK | Done | v2.5.0, panel width fixed, v2.6.0 added Load Sample (async_open_file + dr_wav) |
| Drummmmmmer | Done | v2.5.0 |
| ALEXANDERPLATZ | Done | v2.5.0 |
| SHINJUKU | Done | v2.5.0, custom Widget removed |
| UniRhythm | Done | v2.5.0, std::vector removed |

---

## Common Problems

### Panel Not Loading or Displaying Incorrectly

1. Verify `assets/ModuleName.png` exists and has correct dimensions
2. Clear cache: `rm -rf build/assets build/assets.uimg`
3. Rebuild

### UI Component Position Offset

1. Remove all custom Widgets (TransparentWidget, Widget subclasses)
2. If ALL modules have knobs/ports that are too small and offset to bottom-right, the problem is that the simulator's `build/assets/` is missing standard component images (`rack-lib/` etc.). CMake build order race condition causes `firmware/assets/` copy to be skipped. Fix:
   ```bash
   cd <METAMODULE_SDK_DIR>/simulator
   cp -r ../firmware/assets/* build/assets/
   rm -f build/assets.uimg
   cmake --build build -- asset-image
   ```

### Compilation Errors: vector/string Related

Convert dynamic containers to fixed arrays — see Porting Rule #1 above.

---

## References

- [4ms MetaModule Plugin SDK](https://github.com/4ms/metamodule-plugin-sdk)
- [VCV Rack Plugin Development](https://vcvrack.com/manual/PluginDevelopmentTutorial)
