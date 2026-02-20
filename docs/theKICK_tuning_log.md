# theKICK 開發記錄

日期：2026-02-19 ~ 2026-02-20
狀態：**完成**

---

## 概要

theKICK 是 MADZINE VCV Rack 的 kick drum 合成器模組（8HP）。開發過程包含三個階段：預設音色調校、DSP 引擎重構、面板佈局重設計。

---

## 最終程式碼狀態

### configParam 預設值

```
PITCH: 47 Hz
SWEEP: 260 Hz
BEND: 0.88
DECAY: 136 ms
FOLD: 0.3
SAMPLE: 0 (FM depth)
FB: 0 (Feedback)
TONE: 10 (fully open)
MODE: 0 (PM)
```

### 信號路徑

```
Trigger → phase/env reset + oversampling buffer reset
→ Pitch envelope: freq = pitch + sweep * exp(-t / (0.015/bend))
→ Self-feedback PM (if fb > 0)
→ Sample interaction (PM/RM/AM/SYNC, if sample loaded)
→ Phase accumulator: phase += freq * sampleTime
→ Oscillator: sin(2pi*phase + fbPhase + samplePhase)
→ 4-pole LPF (24dB/oct, tone cutoff: 40-20000Hz)
→ Post-LPF Drive: tanh saturation (if fold > 0)
→ Amplitude envelope: exp(-t/tau) [simple exponential]
→ Output x gain(8.0) x accentLevel
→ 2x oversampling + HalfRateFilter D2 decimation + 2x compensation
→ Output voltage
```

### 面板佈局

```
Row 1: PITCH (left)     | LOAD button + MODE LED (right)
Row 2: SWEEP (left)     | FM knob + CV (right)
Row 3: BEND (left)      | FEEDBACK knob + CV (right)
Row 4: DECAY / TONE / DRIVE (3-column)
I/O:   TRIG / ACCENT / OUT
```

---

## 階段 1：預設音色調校（v1 ~ v10）

### 目標

以另一個 VCV 模組的 kick 音色為參考，調整預設參數使出廠音色有足夠重量感。

### 關鍵發現

- v1-v3 方向正確（有 pitch sweep），但重量不足
- v4-v8 走錯方向：移除 pitch sweep 嘗試純正弦 + FM 諧波，質感完全不同
- 使用者明確指出「一開始的 001~3 還比較像，後面越來越錯」
- v9 回到 v1 路線（SWEEP=158, BEND=1.0, 簡單指數包絡），方向恢復正確
- v10 經 WAV 瞬時頻率分析推導出 SWEEP=260, BEND=0.88，接近參考音色

### 調校歷程摘要

| 版本 | SWEEP | BEND | DECAY | 包絡 | 結果 |
|------|-------|------|-------|------|------|
| v1 原始 | 158 | 1.0 | 136 | exp(-t/tau) | 重量不足 |
| v2 | 230 | 1.5 | 136 | exp(-t/tau) | 重量不足 |
| v3 | 280 | 2.0 | 105 | exp(-t/tau) | 更差，能量在中頻 |
| v4 | 100 | 3.0 | 150 | exp(-t/tau) | 方向錯 |
| v5 | 8 | 3.0 | 140 | exp(-x*(1+0.45x)) | 方向錯 |
| v6 | 0 | 2.0 | 130 | exp(-x*(1+0.2x)) | 方向錯 |
| v7 | 5 | 3.0 | 115 | exp(-x*(0.7+0.3x)) | 方向錯 |
| v8 | 0 | 2.0 | 130 | exp(-t/tau) | 方向錯 |
| v9 | 158 | 1.0 | 136 | exp(-t/tau) | 方向正確 |
| v10 | 260 | 0.88 | 136 | exp(-t/tau) | 接近目標 |

### 教訓

1. 不要在沒有分析的情況下修改參數
2. Pitch sweep 是 kick 音色的核心特徵，不應該移除
3. WAV 瞬時頻率分析比頻譜能量分佈更可靠

---

## 階段 2：DSP 引擎重構

### 改動內容

1. **Waveshaper 模式 → Sample 互動模式**
   - 舊：Saturate/Fold/Asymmetric/Destroy（4 種 waveshaper）
   - 新：PM/RM/AM/SYNC（4 種 sample 互動方式）

2. **Drive 位置調整**
   - 舊：waveshaper 在 LPF 之前
   - 新：tanh saturation 在 LPF 之後（更自然的飽和特性）

3. **Trigger onset 修復**（關鍵 bug fix）
   - 問題：block-based oversampling 導致 trigger 後最多 BLOCK_SIZE-1 個 sample 的延遲
   - 修復：trigger 時重設 `processPosition = BLOCK_SIZE` 和 `downFilter.reset()`

4. **包絡公式**
   - 經過多次實驗後回到簡單指數 `exp(-t/tau)`

### Git 記錄

- `0343594` — DSP 引擎重構 + 預設調校

---

## 階段 3：面板佈局重設計

### 改動內容

1. **右欄重排**：LOAD/MODE (Row1) → FM (Row2) → FEEDBACK (Row3)
2. **底部三欄**：DECAY / TONE / DRIVE 橫排（X=24/61/98）
3. **FM 停用狀態**：無 sample 時旋鈕和 CV port 變暗（alpha 25%）且無法操作
4. **提示文字**：無 sample 時顯示 "Load wav / to activate / dynamic FM" 覆蓋在旋鈕區域

### Git 記錄

- `1139833` — 面板佈局重設計 + FM 停用狀態
