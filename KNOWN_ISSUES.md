# MADZINE VCV Rack 已知問題與待修復清單

版本：2.3.8
更新日期：2026-01-02

---

## 狀態說明

| 狀態 | 說明 |
|------|------|
| PENDING | 已發現但尚未修復，等待實際崩潰報告 |
| CONFIRMED | 已確認會導致崩潰 |
| FIXED | 已修復 |

---

## weiiidocumenta (8 層錄音取樣器)

### ISSUE-001: slices 空陣列導致 size_t underflow [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02
**狀態**: PENDING - 等待實際崩潰報告

**問題描述**:
當 `slices` 向量為空時，`slices.size() - 1` 會因為 `size_t` 無符號整數 underflow 變成極大值 (SIZE_MAX)，導致後續陣列存取越界崩潰。

**位置**:
- 行 864-865: `targetSliceIndex` 計算
- 行 868: `slices[targetSliceIndex].active` 存取
- 行 877: `slices[targetSliceIndex].startSample` 存取
- 行 884: 多聲道模式同樣問題

**觸發情境**:
- 模組剛載入，尚未錄音時轉動 SCAN 旋鈕
- 清除錄音後立即使用 slice scan 功能

**建議修復**:
```cpp
// 行 864-865 修改為:
if (slices.size() > 1) {
    int maxIndex = (int)slices.size() - 1;  // 先轉成 int
    int targetSliceIndex = (int)std::round(scanValue * maxIndex);
    targetSliceIndex = clamp(targetSliceIndex, 0, maxIndex);

    if (targetSliceIndex >= 0 && targetSliceIndex < (int)slices.size()) {
        // 安全存取
    }
}
```

---

### ISSUE-002: 播放緩衝區越界存取 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02
**狀態**: PENDING - 等待實際崩潰報告

**問題描述**:
播放時使用 `recordedLength` 取模計算位置，但：
1. `recordedLength = 0` 時會除零
2. `recordedLength > bufferL.size()` 時會越界
3. `floatPos` 為負數（反向播放）時取模結果可能為負

**位置**:
- 行 515-521: 單聲道播放
- 行 552-558: 多聲道播放

**觸發情境**:
- 清除錄音後立即播放 (`recordedLength = 0`)
- 反向播放時 `floatPos` 為負數

**建議修復**:
```cpp
if (layer.recordedLength > 0) {
    int safeLen = std::min(layer.recordedLength, (int)layer.bufferL.size());
    int pos0 = ((int)floatPos % safeLen + safeLen) % safeLen;  // 處理負數
    int pos1 = (pos0 + 1) % safeLen;
    // ... 存取 bufferL/bufferR
}
```

---

### ISSUE-003: voices 陣列與 numVoices 不同步 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02
**狀態**: PENDING - 等待實際崩潰報告

**問題描述**:
迴圈使用 `numVoices` 但直接存取 `voices[i]`，若兩者不同步會越界。

**位置**:
- 行 528-561: 多聲道播放迴圈
- 行 980-1077: 多聲道切片切換

**觸發情境**:
- 快速切換 POLY 參數同時播放

**建議修復**:
```cpp
for (int i = 0; i < numVoices && i < (int)voices.size(); i++) {
```

---

### ISSUE-004: S&H Rate 除零風險 [PENDING]

**嚴重性**: MEDIUM
**發現日期**: 2026-01-02
**狀態**: PENDING

**問題描述**:
`shRate` 接近 0 時，`1.0f / shRate` 會產生極大值或 Inf。

**位置**:
- 行 1157: `float samplePeriod = 1.0f / shRate;`

**建議修復**:
```cpp
float samplePeriod = (shRate > 0.0001f) ? 1.0f / shRate : 10000.0f;
```

---

## Launchpad (8x8 Grid Looper)

### ISSUE-005: dragSource->module 空指標 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`draw()` 中檢查 `dragSource` 不為空後，直接存取 `dragSource->module`，但 `module` 可能為 nullptr。

**位置**: 行 1077

**建議修復**:
```cpp
if (dragSource && dragSource->module &&
    dragSource->module->cells[dragSource->row][dragSource->col].state != CELL_EMPTY)
```

---

### ISSUE-006: buffer 與 recordedLength 不同步 [PENDING]

**嚴重性**: MEDIUM
**發現日期**: 2026-01-02

**問題描述**:
`buffer[playPosition]` 存取時，`recordedLength` 和 `buffer.size()` 是分開維護的，可能不一致。

**位置**: 行 767, 776

---

## SongMode

### ISSUE-007: activeInput 陣列越界 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`activeInput` 被用作 `params[]`、`inputs[]`、`trigPulses[]` 的索引，但缺乏範圍驗證（應在 0-7）。

**位置**: 行 351, 391, 425, 438

**建議修復**:
```cpp
// 在 process() 開頭加入
activeInput = clamp(activeInput, 0, 7);
previousInput = clamp(previousInput, 0, 7);
```

---

## Runshow

### ISSUE-008: totalCycleClocks 除零 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
當所有 bar 長度都為 0 時，`clockCount % (int)totalCycleClocks` 會導致模零崩潰。

**位置**: 行 302, 724

**觸發情境**:
- 將所有 bar 參數設為 0

**建議修復**:
```cpp
float totalCycleClocks = bar0Clocks + bar1Clocks + bar2Clocks + bar3Clocks;
if (totalCycleClocks <= 0.f) totalCycleClocks = 1.f;
```

---

## MADDYPlus

### ISSUE-009: sequenceLength 除零 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`random::u32() % sequenceLength` 等操作，若 `sequenceLength` 為 0 會崩潰。

**位置**: 行 1211, 1295, 1379

---

### ISSUE-010: track.multiplication 除零 [PENDING]

**嚴重性**: MEDIUM
**發現日期**: 2026-01-02

**問題描述**:
`track.length * track.division / track.multiplication` 若 `multiplication` 為 0 會崩潰。

**位置**: 行 865

---

## DECAPyramid

### ISSUE-011: trackIndex 未驗證 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`VolumeMeterWidget` 中 `trackIndex` 直接用於陣列索引，未檢查是否在 0-7 範圍內。

**位置**: 行 450-451, 854-870

**建議修復**:
```cpp
if (trackIndex < 0 || trackIndex >= 8) return;
```

---

## Pinpple

### ISSUE-012: paramQuantities 越界刪除 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`delete module->paramQuantities[NOISE_MIX_PARAM]` 未檢查陣列邊界和指標有效性。

**位置**: 行 821

---

## NIGOQ

### ISSUE-013: bufferIndex 邊界檢查 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`bufferIndex >= SCOPE_BUFFER_SIZE` 檢查邏輯可能導致邊界存取問題。

**位置**: 行 1441, 1526-1528

---

## PPaTTTerning

### ISSUE-014: primaryKnobs 除零 [PENDING]

**嚴重性**: MEDIUM
**發現日期**: 2026-01-02

**問題描述**:
`random::u32() % (5 - primaryKnobs)` 當 `primaryKnobs = 5` 時導致除零。

**位置**: 行 258

---

## TWNCLight

### ISSUE-015: pattern 陣列越界 [PENDING]

**嚴重性**: HIGH
**發現日期**: 2026-01-02

**問題描述**:
`pattern[currentStep]` 存取時，`length > pattern.size()` 可能越界。

**位置**: 行 323

**建議修復**:
```cpp
pattern[currentStep % pattern.size()]
```

---

## YAMANOTE

### ISSUE-016: chIndex 負數越界 [PENDING]

**嚴重性**: MEDIUM
**發現日期**: 2026-01-02

**問題描述**:
`chIndex = u8Count - 1 - i` 當 `u8Count < i` 時為負數，導致陣列越界。

**位置**: 行 594

---

## 低風險模組（無需立即處理）

以下模組經檢查後無高風險問題：
- MADDY
- KIMO
- Obserfour
- Observer（僅 MEDIUM）
- Pyramid
- KEN
- SwingLFO
- ADGenerator（僅 MEDIUM 精度問題）
- QQ
- U8
- EnvVCA6
- EllenRipley
- EuclideanRhythm

---

## 已修復問題

### ISSUE-F001: UniversalRhythm accents 陣列越界 [FIXED v2.3.7]

**修復版本**: v2.3.7
**修復日期**: 2025-12-24

**問題描述**:
`fillActive = true` 時，`useStep` 可能超出 `accents` 向量大小。

**修復方案**:
```cpp
// 修復前
bool accent = primaryPattern.accents[useStep];

// 修復後
bool accent = primaryPattern.accents[useStep % primaryPattern.length];
```

詳見 `CLAUDE.md` 的「UniversalRhythm 陣列越界崩潰問題」章節。

---

## 佈局問題（非崩潰）

### LAYOUT-001: TWNC 區段標題顏色 [WONTFIX]

**位置**: TWNC.cpp 行 885, 917
**問題**: "Drum"/"HATs" 使用 `(255,200,100)` 而非標準橘色 `(255,200,0)`
**決定**: 設計選擇，不修復

### LAYOUT-002: TWNC 白色區域輸入標籤顏色 [WONTFIX]

**位置**: TWNC.cpp 行 947-956
**問題**: 輸入標籤使用粉紅色，規範建議輸入用黑色
**決定**: 視覺一致性考量，不修復

---

## 報告新問題

如果遇到崩潰，請提供：
1. 崩潰時的操作步驟
2. 錯誤訊息或堆疊追蹤
3. VCV Rack 版本
4. 作業系統

回報至：https://github.com/mmmmmmmadman/MADZINE-VCV/issues
