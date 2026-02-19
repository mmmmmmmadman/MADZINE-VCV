# theKICK 預設音色調校記錄

日期：2026-02-19

---

## 目標

以 `/Users/madzine/Desktop/Untitled.wav` 為參考音色，調整 theKICK 模組的預設參數，使出廠音色具有足夠的「重量感」。

---

## 目標音色分析

- **檔案**: `/Users/madzine/Desktop/Untitled.wav`（3200ms，48kHz，包含多個 kick hit）
- **基頻**: 46-47 Hz（恆定，從第一個週期起幾乎無 pitch sweep）
- **頻率實測**: 0ms: 47.3Hz → 21ms: 46.7Hz → 43ms: 46.4Hz → 64ms: 46.3Hz
- **頻譜分佈** (5-80ms body): Sub(20-60Hz) **89.5%** | Low(60-120Hz) 7.3% | LowMid(120-250Hz) 2.0%
- **波形**: 接近純正弦波，THD 低，-4.4% 正負不對稱，微小 DC offset (-0.006)
- **振幅包絡**: 非簡單指數衰減。半週期峰值衰減率隨時間加速：
  - 前期有效 tau ~120ms（慢）
  - 後期有效 tau ~86ms（快）
  - 最佳擬合：**二次指數 exp(-x*(1+kx))**，k=0.2，tau=130ms
- **諧波**: H2=-27dB, H3=-18dB（非常乾淨，基本上是純基頻）
- **包絡半週期峰值衰減**（相對第一峰 100%）:

| 時間 | 衰減 |
|------|------|
| 5ms | 100% |
| 16ms | 91.6% |
| 48ms | 69.4% |
| 80ms | 51.5% |
| 102ms | 41.6% |
| 145ms | 25.7% |
| 200ms | 11.5% |

---

## 調整歷程

### 原始預設（調整前）

```
PITCH: 46.3 Hz
SWEEP: 158 Hz
BEND: 1.0
DECAY: 136 ms
FOLD: 0.3
Output gain: 5.0
Envelope: exp(-t/tau) [簡單指數]
```

問題：四種 Drive 模式兩兩相像，整體重量不足。

---

### v2 → `Untitled-002.wav`

**改動**:
- PITCH: 46.3 → **55** Hz
- SWEEP: 158 → **230** Hz
- BEND: 1.0 → **1.5**
- Output: 5.0 → **5.8**

**結果**: 重量還是不夠。

---

### v3 → `Untitled-003.wav`

**改動**:
- PITCH: 55 → **52** Hz
- SWEEP: 230 → **280** Hz
- BEND: 1.5 → **2.0**
- DECAY: 136 → **105** ms
- Output: 5.8 → **6.5**

**結果**: 越差越遠了。

**分析發現**: 頻譜完全錯誤。能量分佈：Sub(20-60Hz) 只有 **5.7%**，LowMid(120-250Hz) 佔 **64.6%**。高 SWEEP 值導致音高在中頻停留太久，能量堆積在 120-250Hz 而非 Sub 頻段。

---

### v4 → `Untitled-004.wav`

**改動**（大幅修正方向）:
- PITCH: 52 → **46** Hz（對齊目標 46Hz 基頻）
- SWEEP: 280 → **100** Hz（大幅降低）
- BEND: 2.0 → **3.0**（tau=5ms，快速沉入 Sub）
- DECAY: 105 → **150** ms
- FOLD: 0.3 → **0.0**（預設無飽和）

**結果**: 還是差很多。使用者指出是合成法的問題（env 曲線等）。

---

### v5 → `Untitled-005.wav`

**改動**（包絡曲線重構）:
- SWEEP: 100 → **8** Hz（幾乎無 pitch sweep）
- DECAY: 150 → **140** ms
- **振幅包絡從簡單指數改為二次指數**:
  - 舊: `exp(-t/tau)`
  - 新: `exp(-x*(1+0.45*x))` where x=t/tau
  - 特性：前段衰減慢，尾段加速

**結果**: 還是差很多。

---

### v6 → `Untitled-006.wav`

**改動**:
- PITCH: 46 → **47** Hz（對齊目標 47.3Hz）
- SWEEP: 8 → **0** Hz（完全無 pitch sweep）
- DECAY: 140 → **130** ms
- 二次指數係數: 0.45 → **0.2**（較自然的尾巴）
- Output: 6.5 → **5.0**（±5V 標準輸出）

**結果**: 完全不一樣。

---

## 目前程式碼狀態（v6）

### configParam 預設值

```cpp
configParam(PITCH_PARAM, 20.f, 200.f, 47.f, "Pitch", " Hz");
configParam(SWEEP_PARAM, 0.f, 500.f, 0.f, "Sweep", " Hz");
configParam(BEND_PARAM, 0.5f, 4.f, 2.f, "Bend");
configParam(DECAY_PARAM, 10.f, 1000.f, 130.f, "Decay", " ms");
configParam(FOLD_PARAM, 0.f, 10.f, 0.f, "Fold");
configParam(SAMPLE_PARAM, 0.f, 10.f, 0.f, "Sample");
configParam(FB_PARAM, 0.f, 1.f, 0.f, "Feedback");
configParam(TONE_PARAM, 0.f, 10.f, 10.f, "Tone");
configParam(MODE_PARAM, 0.f, 3.f, 0.f, "FM Mode");
```

### 振幅包絡（行 475-480）

```cpp
float decaySec = state.decayMs * 0.001f;
float x = ampEnvTime / decaySec;
float ampEnv = std::exp(-x * (1.f + 0.2f * x));
```

### 輸出增益（行 483）

```cpp
float output = filtered * ampEnv * 5.f;
```

### 信號路徑

```
Trigger → phase/env reset
→ Pitch envelope: freq = pitch + sweep * exp(-t / (0.015/bend))
→ Self-feedback PM (if fb > 0)
→ Sample FM interaction (PM/RM/AM/SYNC, if sample loaded)
→ Phase accumulator: phase += freq * sampleTime
→ Oscillator: sin(2π*phase + fbPhase + samplePhase)
→ 4-pole LPF (24dB/oct, tone cutoff: 40-20000Hz)
→ Post-LPF Drive: tanh saturation (if fold > 0)
→ Amplitude envelope: exp(-x*(1+0.2x))
→ Output × gain(5.0) × accentLevel
→ 2x oversampling + HalfRateFilter D2 decimation + 2x compensation
→ Output voltage
```

---

## v6 WAV 直接比對分析（方法轉換）

放棄 Python 模擬方法，改為直接比對參考 WAV 與 v6 輸出 WAV 的原始波形資料。

### 目標音色分析修正

先前分析有誤差，以 WAV 直接測量修正：

- **諧波**: H2=**-17.9dB**, H3=**-23.7dB**（先前記錄 H2=-27dB/H3=-18dB 有誤）
- **頻譜分佈**: Sub 90.1% | Low(60-120Hz) **7.3%** | LowMid(120-250Hz) **2.0%**
- **不對稱性**: 正/負峰比 **0.91**（負峰較大，典型類比 kick 特性）

### 發現的三個核心問題

#### 問題 1：v6 太「乾淨」——缺少諧波（主因）

| 諧波 | 參考 | v6 | 差距 |
|------|------|-----|------|
| H2 (94Hz) | -17.9dB | -23.3dB | 少 5.4dB |
| H3 (141Hz) | -23.7dB | -36.0dB | 少 12.3dB |

能量分佈比較：

| 頻段 | 參考 | v6 |
|------|------|-----|
| Sub (20-60Hz) | 90.1% | 96.2% |
| Low (60-120Hz) | 7.3% | 3.5% |
| LowMid (120-250Hz) | 2.0% | 0.3% |

v6 的 Sub 佔比反而太高。「重量感」來自 60-250Hz 的諧波體感，不只是 Sub。
FOLD=0 + FB=0 的純正弦完全缺乏這些成分。

#### 問題 2：包絡公式方向錯誤

`exp(-x*(1+0.2x))` 中 k=0.2 讓衰減在所有時間點都比純指數更快，但目標需要前段慢、後段快。

正規化包絡比較：

| 時間 | 參考 | v6 | 差距 |
|------|------|-----|------|
| 50ms | 0.694 | 0.626 | v6 快 10% |
| 80ms | 0.515 | 0.430 | v6 快 17% |
| 100ms | 0.416 | 0.330 | v6 快 21% |

數學分析：`exp(-x*(1+kx))` 中 `(1+kx)` 在 k>0 時恆 >1，所以衰減恆快於純指數。
正確公式應為 `exp(-x*(0.7+0.3x))`：x<1 時有效率 <1（慢），x>1 時 >1（快）。

擬合驗證（tau=115ms, a=0.7, b=0.3）：

| 時間 | 目標 | 公式值 |
|------|------|--------|
| 50ms | 0.694 | 0.697 |
| 100ms | 0.416 | 0.434 |
| 145ms | 0.257 | 0.257 |
| 200ms | 0.115 | 0.119 |

#### 問題 3：波形不對稱性相反

- 參考: 正/負峰比 0.91（負峰較大）
- v6: 正/負峰比 1.13（正峰較大，方向相反）

---

### v7 修正方案

同時修正三個問題：

1. **包絡公式**: `exp(-x*(1+0.2x))` → `exp(-x*(0.7+0.3x))`，DECAY 預設 115ms
2. **諧波內容**: 預設 FB=0.15（自反饋 PM 產生 H2+H3），FOLD=0.3（tanh 飽和加強 H3）
3. **微量 pitch sweep**: SWEEP=5Hz, BEND=3.0（tau=5ms，產生自然初始 thump）
