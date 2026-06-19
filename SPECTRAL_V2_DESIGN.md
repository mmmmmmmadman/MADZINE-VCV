# FFTComp Spectral v2 升級設計文件

版本：v2 design draft
日期：2026-06-19
作者：audio-processing agent
範圍：純設計文件，不含實作 code
基準 commit：d001393（12HP 強硬版 + 白區微調）

---

## 0. 升級目標總覽

| 編號 | 內容 | 主要影響 | 依據 |
|---|---|---|---|
| C-1 | Dual-window STFT 降延遲 | 42.7ms → 5.3ms | arXiv 2204.09911 |
| C-2 | ERB bin grouping | 4 band → 32 ERB band | soothe3 / ANINA |
| C-3 | Adaptive percentile prominence | 取代 ±9 bin 平滑差 | ERB 內 75th percentile |
| C-4 | Psychoacoustic masking gate | 抑制被遮蔽的偵測 | MPEG-1 Model 2 |
| C-5 | Group delay peak picking | 替代單純 magnitude | 經典 DSP |

---

## §依賴圖

```
            C-1 (dual-window STFT)
             │ 影響延遲與 OLA 結構，獨立於 detection
             │
            C-2 (ERB bin grouping)  ← 核心新資料結構
             ├──────────────┬──────────────┐
             ▼              ▼              ▼
            C-3            C-4            C-5
        (percentile)     (masking)    (group delay)
        在 ERB band     依 ERB band   per-bin 量，但
        內取 75th       聚合 tonal/    最終經 ERB band
        當 floor        noise masker   權重整合到 GR
                       ▼
                   合成 detection function
                   d(k) = max(prom(k), gd_prom(k)) − mask(k)
```

### 獨立 / 依賴

| 項目 | 獨立可單獨上 | 依賴 |
|---|---|---|
| C-1 | 是 | — |
| C-2 | 是（但無 detection 升級就無實際效益）| — |
| C-3 | 否 | 必須 C-2（要 ERB band 範圍才能取 percentile）|
| C-4 | 否 | 必須 C-2（masking threshold per ERB band 聚合）|
| C-5 | 部分依賴 | 不需 C-2 即可算 group delay，但 prominence 整合需 C-2/C-3 同 domain |

---

## §建議實作順序（commit 拆解）

| Commit | 內容 | 動檔案 | 為何此順序 | 回退方式 |
|---|---|---|---|---|
| v2.1 | C-1 dual-window 結構 | FFTCompDSP.hpp（rings / window / OLA gain）| 與 detection 無耦合，先驗證延遲與 OLA 重建正確 | `git revert v2.1`；單檔回到 d001393 版本的 FFTCompDSP.hpp |
| v2.2 | C-2 ERB band 結構（新檔 ERBGrouping.hpp）| 新增 ERBGrouping.hpp、FFTCompDSP.hpp 加成員 | 必須先有資料結構，後續 C-3/C-4/C-5 才能掛上去；保留舊 4-band weightBins_ 路徑可開關 | DSP 內保留 `useERB_` flag，false 退回 4-band 路徑 |
| v2.3 | C-3 adaptive percentile | FFTCompDSP.hpp（detection block 重寫）| 取代既有 ±9 bin smoothing；需 v2.2 的 band range | 保留舊 `SMOOTH_N=9` code path 由 flag 切換 |
| v2.4 | C-4 masking model（新檔 MaskingModel.hpp）| 新增 MaskingModel.hpp、FFTCompDSP.hpp detection 後處理 | 需 v2.3 的 prom 才能扣 mask | flag `useMasking_` 預設 off |
| v2.5 | C-5 group delay peak picking | FFTCompDSP.hpp（detection block 增量）| 最後加入，最複雜，最易出錯 | flag `useGroupDelay_` 預設 off；失敗單 revert v2.5 |
| v2.6 | UI 整理：4 旋鈕對映 32 ERB band 的 macro grouping、display 升級 | FFTComp.cpp（display 重畫）、可選 menu | 純 UI，待 DSP 穩定 | 純 widget 修改，無功能風險 |

每個 commit 後 build + install + Rack 載入測試，遞進不跳號。

---

## §C-1 Dual-window STFT（低延遲 OLA）

### 文獻摘要

arXiv 2204.09911 提出 analysis 大窗 / synthesis 小窗對偶：spectral analysis 用 2048 取得充足頻率解析（23.4 Hz/bin），合成輸出用 256 點 hop 與短 synthesis window 重建，總演算法延遲 ≈ synthesis window 長度，不再受 analysis 窗 size 拘束。

### 窗對選擇

| 用途 | 長度 | 函數 | 角色 |
|---|---|---|---|
| Analysis | 2048 | Hann | FFT 前對輸入加窗 |
| Synthesis | 256 | sqrt-Hann（或修剪過的窗）| iFFT 後對輸出加窗 |
| Hop | 256 | — | 75% overlap with synthesis（4x OLA）|

注意：analysis 窗仍跨 2048，但每個 hop 只「貢獻」最後 256 點到輸出，過往樣本因 synthesis 窗權重為 0 而不重建。延遲 = synthesis window center ≈ 128 samples ≈ 2.67 ms（@48k），保守估計含 hop 對齊約 5.3 ms。

### 與既有結構相容性

| 既有變數 | 新處理 |
|---|---|
| `inputRing_[FFT_SIZE]` | 維持 2048 ring，writePos 推進不變 |
| `outputRing_[FFT_SIZE*2]` | 改為 `outputRing_[FFT_SIZE + HOP_SIZE]`（只需覆蓋一個 analysis 窗範圍），OLA 寫入只覆蓋最後 256 點對齊區段 |
| `window_` | 留作 analysis Hann |
| 新增 `synWindow_[FFT_SIZE]` | 對應 synthesis 窗：中心 256 點為 sqrt-Hann，外圍為 0 |
| `HOP_SIZE` | 512 → 256 |
| `olaGain_` | 由新的 analysis × synthesis 重疊和重新推導：sum_n window[n] * synWin[n] 跨所有 hop 的 frame |
| `dryDelayL/R_` | 大小縮成 synthesis 延遲 + hop 對齊，不再是 FFT_SIZE |

### OLA 重建公式

對每個輸出樣本 y[n]：
y[n] = sum over frames m of (iFFT(X_m)[n - mH] · w_a[n - mH] · w_s[n - mH])

其中 w_a 為 analysis Hann（已乘進 X_m），w_s 為 sliding synthesis 窗。經 hop=256 且 sqrt-Hann synthesis 窗 256 點：sum w_a · w_s = COLA 常數，olaGain_ = 1 / 該常數（離線預算）。

### 與目前 readPos_ 結構相容性

readPos_ 仍以 sample 為單位推進；OLA 寫入範圍從 256 縮短到 synthesis window 有效長度。dry delay 從 FFT_SIZE 降到 synthesis_latency（約 256），需重新對齊 dryWritePos_。SC monitor 路徑共用相同 dual-window，延遲一致。

### CPU 影響

hop 512 → 256 = FFT 呼叫頻率 ×2。每 frame FFT 成本不變（2048），總 FFT CPU 約 ×2。可接受。

### 風險

- COLA gain 計算錯 → 全頻段 amplitude 不正確（高/低）
- synthesis 窗超出 256 → 延遲超預期
- dry/wet 對齊錯位 → 偵測到 phase cancellation

### 回退

v2.1 commit 單獨 revert 即恢復 hop=512 / OLA=2x。

---

## §C-2 ERB Bin Grouping

### 公式

採用 Glasberg-Moore (1990) ERB scale：

```
ERB(f) = 24.7 * (4.37 * f / 1000 + 1)        // ERB 頻寬 in Hz
ERBN(f) = 21.4 * log10(4.37 * f / 1000 + 1)  // ERB-rate (number, 0..~40 across 20-20k Hz)
```

20 Hz → ERBN ≈ 0.85，20 kHz → ERBN ≈ 42.5。取 **32 bands** 跨 ERBN 1..38 等距。也可改 40 但 32 對齊 soothe3 視覺密度。

### Bin 分配

預先計算：
- `bandLoBin_[B]`、`bandHiBin_[B]`：每 ERB band 涵蓋的 STFT bin 區間 [lo, hi]（含 fade）
- `binToBand_[NUM_BINS]`：每 bin 主屬 band（用於 percentile 收集）
- `bandWeight_[B][NUM_BINS]`：ERB band 對每個 bin 的權重（triangular 或 gammatone-like overlap，相鄰 band 共用邊緣）

低頻 bin 寬於 ERB 寬度時：多個 ERB band 共享同 bin（用權重分配，避免 double count）。
高頻 bin 細於 ERB 寬度時：多個 bin 屬於同 band。

### 跨 sample rate

setSampleRate 時整批重算 `bandLoBin_/bandHiBin_/binToBand_/bandWeight_`，與既有 recomputeAll_ 同 pattern。

### 與 4 旋鈕 UI 相容性（macro grouping）

保留 PRE_EQ / AMT / POST_EQ 各 4 旋鈕，但內部映射為 macro：

| Macro 旋鈕 | 影響 ERB band 範圍（依 bandFreq_ 中心可調）|
|---|---|
| LO | ERB band 0..7（含 low shelf 平滑邊界）|
| LMID | ERB band 6..15 |
| HMID | ERB band 14..23 |
| HI | ERB band 22..31 |

範圍由現有 4 個 bandFreq_ 中心點動態切割：以 ERB-rate 內距找最近 ERB band index 作為 macro 中心，左右各延伸 N band。這保留右鍵 menu 改 4 個 freq 的能力。

AMT 旋鈕 → 套到該 macro 範圍內所有 ERB band 的 `bandAmount_[B]`（每 ERB band 對應的 detection 強度）。
PRE_EQ / POST_EQ 旋鈕 → 仍走原本的 shelf+bell 頻域 gain（不改 EQ 模型，使用者預期一致）。

### SC LPF/HPF / OUT_SEL / Monitor

不受影響。SC filter 仍是 bin-domain gain；OUT_SEL ramp 不變；Monitor 路徑共用同 dual-window OLA。

### GR overlay display

從目前 per-bin 折線改畫法：
- Spectrum 折線：維持 per-bin 顯示，看得到頻譜細節
- GR overlay：改畫 32 個 ERB band 的 GR 柱（band 範圍寬度橫條），垂直高度 = band 平均 reduction dB，alpha 200，橙色填充
- 視覺類似 soothe3 的 dynamic EQ ribbon

### CPU

預計算所有 LUT，每 frame detection 由 NUM_BINS 線性運算改為 NUM_BANDS=32 聚合 + per-bin gain 套用，CPU 持平或略低。

### 風險

- 邊界 bin 重疊權重不歸一 → band 邊緣 GR 過強或失真
- 低頻 ERB band 內只有 1-2 bin → percentile/masking 失效，需 fallback 到 raw magnitude

---

## §C-3 Adaptive Percentile Prominence

### 取代什麼

現行：log domain ±9 bin moving average → prominence = log[k] − smoothedLog[k] − 3 dB。
缺點：固定 9 bin（@48k = 211 Hz）在低頻過寬、高頻過窄；對 broadband noise 容易誤觸發。

### 新做法

每 ERB band B：
1. 收集該 band 涵蓋的 bin log magnitude 集合（用 `binToBand_`）
2. 取 **75th percentile** 作為該 band 的 noise floor（log dB）
3. per-bin prom_dB[k] = logBuf_[k] − floor_dB[binToBand_[k]] − T（T = 3 dB 可調）

75th 比中位數更接近 tonal peak 之下的 noise level，對單一強 peak 在 ERB 內不會被自身拉高 floor。

### Domain

dB（log）domain，與目前 logBuf_ 一致。

### Selection algorithm

每 frame 對每 ERB band 跑一次選取：
- band size 平均 ~32 bin（1025 bin / 32 band）
- 用 `std::nth_element`（O(n) 平均）找 75th index：複雜度可接受
- 也可用 quickselect 自實作避免 STL 開銷
- 不用 sort（O(n log n)）

per-frame total cost：32 band × ~32 bin × nth_element ≈ 1024 比較，與既有 ±9 smoothing 的 ~18000 加法相比反而便宜。

### Edge case

ERB band 涵蓋 bin < 4 時，nth_element 不穩 → fallback 用 band mean。

### 風險

- 75th 比例調整錯 → 偵測過敏或漏掉
- 跨 frame 不平滑 → percentile 跳動會讓 GR 抖動，需在 floor_dB[B] 上加一階 IIR temporal smoothing（與 attack/release 不同層）

---

## §C-4 Psychoacoustic Masking Threshold Gate

### 模型

MPEG-1 Audio Psychoacoustic Model 2（ISO/IEC 11172-3 annex D）簡化版：

1. **Bark/ERB band 能量聚合**：用 §C-2 結構（ERB band 與 Bark 近似，可直接用 ERB band）
2. **Tonal vs noise masker 區分**：
   - Tonal index TI(B) = ERB band B 的 spectral flatness measure（SFM in dB），SFM = 10 log10(geo_mean / arith_mean)
   - SFM ≈ 0 → 純 noise（無 tonal），SFM ≪ 0 → 強 tonal
   - tonality α(B) = min(SFM(B) / SFM_max, 1)
3. **Spreading function**：在 ERB-rate 軸上以 Schroeder 近似 spreading
   - SF(dz) = 15.81 + 7.5(dz + 0.474) − 17.5 * sqrt(1 + (dz + 0.474)^2)  dB
   - dz = ERB-rate 距離
4. **Tonal masker masking offset** = 14.5 + i（i = Bark/ERB index）dB
   **Noise masker masking offset** = 5.5 dB
5. **每 band masking threshold**：對所有其他 band j，T(B) = sum (E(j) − offset(j) + SF(B−j))，能量加總
6. **Absolute threshold of hearing (ATH)**：
   - ATH(f) [dB SPL] = 3.64(f/1000)^−0.8 − 6.5 exp(−0.6(f/1000−3.3)^2) + 1e−3(f/1000)^4
   - 取 ATH 與計算出的 mask threshold 的較大值
7. **Mask gate**：
   - mask_dB(k) = T(binToBand_[k])
   - 修正 prominence：prom_eff(k) = max(0, prom_dB(k) − margin_mask)
   - margin_mask = log[k] − mask_dB(k) − safety（safety ~3 dB）
   - 若 prom_eff(k) ≤ 0：該 bin 被遮蔽，GR 不觸發

### Per-frame cost

- SFM per band：32 band × ~32 bin × log =  ~1024 log ops
- Spreading：32×32 = 1024 mul-add
- ATH：預算 LUT per bin
總計 < detection 5%，無壓力

### 風險

- spreading offset 量級錯 → 全頻段被遮蔽（無 GR）或完全不遮蔽（退回 C-3 行為）
- tonality 計算對極短頻段不穩 → fallback noise masker
- 須在 floor_dB 已 temporal smoothing 後才扣，避免 frame-to-frame 跳動

---

## §C-5 Group Delay Peak Picking

### Why

resonance 在 frequency domain 不只有 magnitude peak，phase response 在 resonance 中心斜率最陡 → group delay (GD = −dφ/dω) 在 peak 出現高值。對重疊 / 已被 magnitude 平滑掩蓋的 resonance 較強。

### 計算

從 STFT 複數係數 X(k) = re + i*im：

1. φ(k) = atan2(im, re)
2. φ_uw(k) = unwrap φ across k（accumulate 2π jumps）
3. GD(k) ≈ −(φ_uw(k+1) − φ_uw(k−1)) / (2 · Δω)，Δω = 2π / FFT_SIZE

也可用 Flanagan 法（無需 unwrap）：
GD(k) = Re((X_t · conj(X_w)) / |X|^2)
其中 X_t = FFT(time-weighted signal n·x[n])，X_w = FFT(window-weighted)。較貴但更穩。

選擇：第一版用 phase difference 配合 unwrap，每 frame O(N) cost。

### Prominence from GD

per ERB band：
- gd_floor_dB[B] = 75th percentile of log|GD| within band（同 §C-3 方法）
- gd_prom_dB[k] = 10 * log10(|GD(k)| / |GD_floor|) − T_gd

### 與 magnitude prominence 結合

最終 detection function：
d(k) = max(prom_mag(k), α_gd · gd_prom(k)) − mask(k)

其中 α_gd 可調權重（預設 0.5），讓 magnitude-based detection 仍為主，group delay 為輔。

### Risk

- phase unwrap 失敗 → spurious GD spikes，需 clamp |GD| ≤ FFT_SIZE
- DC / Nyquist 不適用，跳過
- 跨 frame 相位連續性不可預設 → 每 frame 獨立計算 GD，不做跨 frame phase smoothing

### CPU

每 frame：unwrap O(N) + diff O(N) + log O(N) ≈ 6N ops。約等同一次 magnitude detection 的 1.5 倍。

---

## §UI 影響清單

| 升級 | 旋鈕 | Tooltip | Display | Menu |
|---|---|---|---|---|
| C-1 | 無變動 | 可選：在 model description 註明「low latency mode」 | 無 | 無 |
| C-2 | PRE_EQ/AMT/POST_EQ 4 旋鈕保留但改 macro 語意 | AMT tooltip 改「Suppression Amount (Macro LO band)」等 | GR overlay 改 ERB band ribbon | 可選：menu 加「ERB band count」（32/40 切換）|
| C-3 | 無新旋鈕 | 無 | 偵測曲線可選顯示 floor line | 可選：menu 「Percentile (50/75/90)」debug 用 |
| C-4 | 無新旋鈕 | 無 | 可選：mask threshold 灰色曲線 overlay | menu 「Enable Masking」toggle |
| C-5 | 無新旋鈕 | 無 | 無（內部偵測量） | menu 「Enable Group Delay Detection」toggle |

旋鈕總數不變（避免 12HP 面板重排）。所有新功能透過 menu toggle 控制。

---

## §檔案分塊

| 檔案 | 角色 | 變更類別 |
|---|---|---|
| FFTCompDSP.hpp | 主 DSP class | C-1 ring/window 結構、C-3 detection 改寫、C-5 GD 計算 |
| ERBGrouping.hpp（新）| ERB band 結構與 bin LUT 計算 | C-2 全部 |
| MaskingModel.hpp（新）| MPEG-1 Model 2 簡化 masking threshold | C-4 全部 |
| FFTComp.cpp | Module + Widget | C-2 macro 旋鈕映射、display 改 ERB ribbon、menu toggle |

---

## §CPU / Latency 估算

| 項目 | Baseline | 新 | Δ |
|---|---|---|---|
| 延遲 @48k | 42.7 ms | 5.3 ms | −37.4 ms |
| FFT 呼叫/秒 @48k | 93.75 | 187.5 | +100% |
| Per-frame detection | ±9 smoothing ~18k op | percentile + masking + GD ~10k op | 略降 |
| Per-frame masking | 0 | spreading 1k op | +1k |
| 總 CPU（VCV 模組）| baseline X% | 約 1.6 ~ 1.8 X | 估計仍 < 5% single core @48k |

最差情況（192k SR + 多 instance）：FFT call 線性放大，預估 < 10% per instance，VCV 可接受。

---

## §風險彙整

| 風險 | 嚴重度 | 緩解 |
|---|---|---|
| C-1 COLA gain 算錯 | 高 | 離線計算 olaGain_、加 unit test sine sweep 驗證 |
| C-2 邊界權重不歸一 | 中 | bandWeight_ normalize per bin 確保 sum = 1 |
| C-3 percentile 跳動 | 中 | floor_dB 加 IIR temporal smoothing |
| C-4 masking 過強 | 高 | margin/safety 可由 menu 調，預設保守 |
| C-5 phase unwrap fail | 中 | 預設 menu off，需驗證再開 |
| 多 toggle 互動 bug | 中 | 每 commit 加單 toggle smoke test |

---

## §回退路徑

| Commit | 失敗 symptom | 回退指令 |
|---|---|---|
| v2.1 | 全頻段音量錯誤 / 喀喀聲 | `git revert <v2.1>` 或 reset 到 d001393 |
| v2.2 | UI 旋鈕對映亂掉 | flag `useERB_ = false` 退回 4-band；嚴重則 revert |
| v2.3 | GR 抖動 / 不觸發 | flag `useAdaptiveFloor_ = false` 退回 ±9 smoothing |
| v2.4 | 完全不 GR | flag `useMasking_ = false` |
| v2.5 | 偵測過敏 | flag `useGroupDelay_ = false` |
| v2.6 | display 崩潰 | widget revert 不影響 DSP |

所有 flag 集中在 DSP class 公開介面，可由 menu toggle 動態切換，方便 A/B 比較。

---

## §開放問題（需使用者決定）

| 問題 | 選項 |
|---|---|
| ERB band 數 | 24 / 32 / 40（建議 32）|
| Masking 預設 on/off | on / off（建議第一版 off 經驗證後 on）|
| Group delay α_gd 權重 | 0.3 / 0.5 / 0.7（建議 0.5）|
| Display GR overlay 樣式 | per-bin 折線維持 / 改 ERB ribbon（建議 ribbon）|

---

文件結束。實作前請確認 §開放問題的決策。
