# VCV Rack Known Issues & Crash Prevention Database

> **This document is knowledge for Claude Code to learn.**
> It documents real crash bugs found in VCV Rack module development,
> their root causes, and proven fix patterns.
> Use this as a reference to avoid the same pitfalls in your own modules.

Version: 2.3.8
Last Updated: 2026-01-02

---

## Status Legend

| Status | Description |
|--------|-------------|
| PENDING | Discovered but not yet fixed, awaiting actual crash reports |
| CONFIRMED | Confirmed to cause crashes |
| FIXED | Resolved |

---

## weiiidocumenta (8-Layer Recording Sampler)

### ISSUE-001: slices Empty Array Causes size_t Underflow [PENDING]

**Severity**: HIGH

**Problem**:
When `slices` vector is empty, `slices.size() - 1` underflows because `size_t` is unsigned, becoming SIZE_MAX, causing out-of-bounds array access crash.

**Trigger**: Turning SCAN knob before recording, or clearing recording then immediately using slice scan.

**Fix**:
```cpp
if (slices.size() > 1) {
    int maxIndex = (int)slices.size() - 1;  // Cast to int first
    int targetSliceIndex = (int)std::round(scanValue * maxIndex);
    targetSliceIndex = clamp(targetSliceIndex, 0, maxIndex);

    if (targetSliceIndex >= 0 && targetSliceIndex < (int)slices.size()) {
        // Safe access
    }
}
```

---

### ISSUE-002: Playback Buffer Out-of-Bounds Access [PENDING]

**Severity**: HIGH

**Problem**:
During playback, `recordedLength` modulo calculation has issues:
1. `recordedLength = 0` causes division by zero
2. `recordedLength > bufferL.size()` causes overflow
3. Negative `floatPos` (reverse playback) produces negative modulo results

**Fix**:
```cpp
if (layer.recordedLength > 0) {
    int safeLen = std::min(layer.recordedLength, (int)layer.bufferL.size());
    int pos0 = ((int)floatPos % safeLen + safeLen) % safeLen;  // Handle negatives
    int pos1 = (pos0 + 1) % safeLen;
    // ... access bufferL/bufferR
}
```

---

### ISSUE-003: voices Array and numVoices Out of Sync [PENDING]

**Severity**: HIGH

**Problem**:
Loop uses `numVoices` but directly accesses `voices[i]`. If they're out of sync, overflow occurs.

**Trigger**: Rapidly switching POLY parameter while playing.

**Fix**:
```cpp
for (int i = 0; i < numVoices && i < (int)voices.size(); i++) {
```

---

### ISSUE-004: S&H Rate Division by Zero [PENDING]

**Severity**: MEDIUM

**Problem**: `shRate` near 0 causes `1.0f / shRate` to produce Inf.

**Fix**:
```cpp
float samplePeriod = (shRate > 0.0001f) ? 1.0f / shRate : 10000.0f;
```

---

## Launchpad (8x8 Grid Looper)

### ISSUE-005: dragSource->module Null Pointer [PENDING]

**Severity**: HIGH

**Problem**: `draw()` checks `dragSource` is not null, then directly accesses `dragSource->module`, but `module` may be nullptr.

**Fix**:
```cpp
if (dragSource && dragSource->module &&
    dragSource->module->cells[dragSource->row][dragSource->col].state != CELL_EMPTY)
```

---

### ISSUE-006: buffer and recordedLength Out of Sync [PENDING]

**Severity**: MEDIUM

**Problem**: `buffer[playPosition]` access fails because `recordedLength` and `buffer.size()` are maintained separately and may diverge.

---

## SongMode

### ISSUE-007: activeInput Array Out-of-Bounds [PENDING]

**Severity**: HIGH

**Problem**: `activeInput` is used as index for `params[]`, `inputs[]`, `trigPulses[]` without range validation (should be 0-7).

**Fix**:
```cpp
// Add at beginning of process()
activeInput = clamp(activeInput, 0, 7);
previousInput = clamp(previousInput, 0, 7);
```

---

## Runshow

### ISSUE-008: totalCycleClocks Division by Zero [PENDING]

**Severity**: HIGH

**Problem**: When all bar lengths are 0, `clockCount % (int)totalCycleClocks` causes modulo-zero crash.

**Fix**:
```cpp
float totalCycleClocks = bar0Clocks + bar1Clocks + bar2Clocks + bar3Clocks;
if (totalCycleClocks <= 0.f) totalCycleClocks = 1.f;
```

---

## MADDYPlus

### ISSUE-009: sequenceLength Division by Zero [PENDING]

**Severity**: HIGH

**Problem**: `random::u32() % sequenceLength` crashes if `sequenceLength` is 0.

---

### ISSUE-010: track.multiplication Division by Zero [PENDING]

**Severity**: MEDIUM

**Problem**: `track.length * track.division / track.multiplication` crashes if `multiplication` is 0.

---

## DECAPyramid

### ISSUE-011: trackIndex Not Validated [PENDING]

**Severity**: HIGH

**Problem**: `VolumeMeterWidget` uses `trackIndex` as array index without checking 0-7 range.

**Fix**:
```cpp
if (trackIndex < 0 || trackIndex >= 8) return;
```

---

## Pinpple

### ISSUE-012: paramQuantities Out-of-Bounds Deletion [PENDING]

**Severity**: HIGH

**Problem**: `delete module->paramQuantities[NOISE_MIX_PARAM]` has no bounds or validity check.

---

## NIGOQ

### ISSUE-013: bufferIndex Boundary Check [PENDING]

**Severity**: HIGH

**Problem**: `bufferIndex >= SCOPE_BUFFER_SIZE` check logic may allow boundary access.

---

## PPaTTTerning

### ISSUE-014: primaryKnobs Division by Zero [PENDING]

**Severity**: MEDIUM

**Problem**: `random::u32() % (5 - primaryKnobs)` divides by zero when `primaryKnobs = 5`.

---

## TWNCLight

### ISSUE-015: pattern Array Out-of-Bounds [PENDING]

**Severity**: HIGH

**Problem**: `pattern[currentStep]` access when `length > pattern.size()` causes overflow.

**Fix**:
```cpp
pattern[currentStep % pattern.size()]
```

---

## YAMANOTE

### ISSUE-016: chIndex Negative Overflow [PENDING]

**Severity**: MEDIUM

**Problem**: `chIndex = u8Count - 1 - i` becomes negative when `u8Count < i`, causing array overflow.

---

## Low-Risk Modules (No Immediate Action Needed)

The following modules were reviewed and have no high-risk issues:
- MADDY, KIMO, Obserfour, Observer (MEDIUM only), Pyramid, KEN
- SwingLFO, ADGenerator (MEDIUM precision issue only), QQ, U8, EnvVCA6
- EllenRipley, EuclideanRhythm

---

## Fixed Issues

### ISSUE-F001: UniversalRhythm accents Array Out-of-Bounds [FIXED v2.3.7]

**Fixed in**: v2.3.7 (2025-12-24)

**Problem**: `fillActive = true` causes `useStep` to exceed `accents` vector size.

**Fix**:
```cpp
// Before
bool accent = primaryPattern.accents[useStep];

// After
bool accent = primaryPattern.accents[useStep % primaryPattern.length];
```

---

## Common Crash Patterns Summary

### 1. Array/Vector Out-of-Bounds (Most Common)
Always use `% size` or explicit bounds checking when index comes from external computation.

### 2. Division by Zero
Any parameter that controls a denominator must have a minimum value guard.

### 3. size_t Underflow
Never subtract from `size_t` without checking the value first. Cast to `int` before subtraction.

### 4. Null Pointer Dereference
Always check `module` pointer before access in Widget code — it's null during module browsing.

### 5. Separate Length/Size Variables
When buffer size and recorded length are maintained separately, always use `std::min()` of both.
