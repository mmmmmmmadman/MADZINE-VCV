# WorldRhythm 學術依據更新計劃

基於 2025-12-16 研究驗證結果

---

## 1. Kotekan 百分比閾值 [刪除/修改]

### 問題
`KotekanEngine.hpp` 中的閾值 (互補性 ≥80%, 連續性 ≥60%, 平衡性 ≥60%) 無學術依據。

### 現有程式碼位置
- `KotekanEngine.hpp:645-647`
```cpp
result.isValid = (result.complementarity >= 0.8f) &&
                (result.continuity >= 0.6f) &&
                (result.balance >= 0.6f);
```

### 建議修改
**選項 A: 移除驗證邏輯**
```cpp
// Kotekan validation removed - no academic standard exists
// Quality assessment provided for reference only
result.isValid = true;  // Always valid, let user judge
```

**選項 B: 保留但加註說明**
```cpp
// NOTE: Thresholds are operational definitions by the author,
// not derived from Tenzer or other ethnomusicological literature.
// Tenzer uses qualitative analysis, not percentage metrics.
result.isValid = (result.complementarity >= 0.8f) &&
                (result.continuity >= 0.6f) &&
                (result.balance >= 0.6f);
```

### 文檔需更新
- `WorldRhythm_Algorithm_Paper.md`
- `DEVELOPMENT_LOG.md`

---

## 2. 鬼音力度 (Ghost Note Velocity) [更新數值]

### 學術依據
- **Matsuo & Sakaguchi (2024)**: 1:4 振幅比 → **25%**
- **Cheng et al. (2022)**: 10 dB 差異 → **~32%**

### 現有程式碼位置

#### PatternGenerator.hpp:711-713
```cpp
// Ghost notes: 15-25% of normal velocity
float ghostVel = 0.15f + dist(rng) * 0.1f + velVar(rng);
p.setOnset(i, std::clamp(ghostVel, 0.12f, 0.28f));
```
**修改為:**
```cpp
// Ghost notes: 25-32% of normal velocity
// Reference: Matsuo & Sakaguchi (2024) 1:4 ratio = 25%
//           Cheng et al. (2022) 10dB difference = ~32%
float ghostVel = 0.25f + dist(rng) * 0.07f + velVar(rng);
p.setOnset(i, std::clamp(ghostVel, 0.20f, 0.35f));
```

#### RhythmEngine.hpp:774
```cpp
float ghostVel = 0.12f + dist(humanRng) * 0.12f;
```
**修改為:**
```cpp
// Ghost velocity: 25-32% (Matsuo & Sakaguchi 2024, Cheng et al. 2022)
float ghostVel = 0.25f + dist(humanRng) * 0.07f;
```

#### HumanizeEngine.hpp:208-345 (各風格)
現有範圍: `ghostVelocityMin/Max` 約 0.20-0.50
**建議統一調整為 0.25-0.32 為基準，允許風格變異**

### 文檔需更新
- `DEVELOPMENT_LOG.md:57` - "弱拍12-28%力度" → "弱拍25-32%力度"
- `WorldRhythm_Paper.txt:109` - "Velocity: 12-28%" → "Velocity: 25-32%"
- 各語言 HTML 文檔中的 "Ghost Notes 力度" 說明

---

## 3. 搖擺比 (Swing Ratio) [補充學術來源]

### 學術依據
- **Friberg & Sundström (2002)**:
  - 慢速 (~120 BPM): 3.5:1
  - 中速: 2.0:1 (triplet feel)
  - 快速 (~300+ BPM): 1.0:1
  - 短音符絕對持續時間約 100ms

### 現有程式碼
`StyleProfiles.hpp:7` - 僅有註釋 "0.67 = triplet"

### 建議修改
在 `StyleProfiles.hpp` 或 `HumanizeEngine.hpp` 加入:
```cpp
// Swing Ratio Reference (Friberg & Sundström 2002, Music Perception):
// - Slow tempo (~120 BPM): ratio up to 3.5:1
// - Medium tempo: 2.0:1 (triplet feel, swing = 0.67)
// - Fast tempo (300+ BPM): approaches 1.0:1 (straight)
// - Short note duration constant ~100ms at medium-fast tempos
```

### 文檔需更新
- `WorldRhythm_Algorithm_Paper.md` - 加入 Friberg & Sundström 引用

---

## 4. 時間變異數 (Timing Variance) [驗證/補充來源]

### 學術依據
- **Polak & London (2014)**: 西非 jembe 最短細分 80-100ms，標準差 0.0056
- **EDM 研究**: 人性化參數 ±10ms，Seeb remix 使用 23ms swing 偏移

### 現有數值
| 風格 | 現有值 | 學術依據 |
|------|--------|----------|
| West African | ±25ms | ⚠️ 需驗證 - Polak 測量的是細分間隔，非變異數 |
| Jazz | ±15ms | ✅ 合理 (Friberg & Sundström 提到 <20ms 同步) |
| Electronic | ±2-3ms | ✅ 合理 (EDM 研究支持 grid-based) |
| Techno | ±2ms | ✅ 合理 |

### 建議修改
在文檔中加入學術來源說明:
```
Timing variance values are informed by:
- Polak & London (2014): West African subdivision ~80-100ms minimum
- Friberg & Sundström (2002): Jazz ensemble sync <20ms
- EDM research: humanization typically ±10ms
```

### 文檔需更新
- `WorldRhythm_Paper.txt:95, 215-250` - 加入來源引用

---

## 5. 巴爾幹 Aksak 7/8 分組 [已正確實現]

### 現況
`MeterEngine.hpp` 已包含兩種分組:
- `METER_7_8_A`: 2+2+3 (Balkan)
- `METER_7_8_B`: 3+2+2 (Rupak-style)

`AsymmetricGroupingEngine.hpp` 更完整:
- `SEVEN_223`: 2+2+3 (Rachenitsa)
- `SEVEN_322`: 3+2+2
- `SEVEN_232`: 2+3+2

### 結論
✅ **程式碼已正確實現，無需修改**

### 文檔需更新
- 確保文檔說明兩種分組都是常見的

---

## 修改優先順序

| 優先級 | 項目 | 影響範圍 | 工作量 |
|--------|------|----------|--------|
| 1 | Kotekan 閾值說明 | KotekanEngine.hpp, 文檔 | 小 |
| 2 | 鬼音力度更新 | PatternGenerator, RhythmEngine, HumanizeEngine, 文檔 | 中 |
| 3 | 搖擺比來源註釋 | StyleProfiles, 文檔 | 小 |
| 4 | 時間變異數來源 | 文檔 | 小 |
| 5 | Aksak 文檔確認 | 文檔 | 小 |

---

## 學術引用清單 (建議加入文檔)

```
References:

[1] Matsuo, H., & Sakaguchi, Y. (2024). Effects of Rhythm and Accent
    Patterns on Tempo-Keeping Property of Finger Tapping. i-Perception.
    DOI: 10.1177/20592043241276959

[2] Cheng, T.Z., Creel, S.C., & Iversen, J.R. (2022). How Do You Feel
    the Rhythm: Dynamic Motor-Auditory Interactions Are Involved in
    the Imagination of Hierarchical Timing. Journal of Neuroscience,
    42(3), 500-512.

[3] Friberg, A., & Sundström, A. (2002). Swing Ratios and Ensemble
    Timing in Jazz Performance: Evidence for a Common Rhythmic Pattern.
    Music Perception, 19(3), 333-349.

[4] Polak, R., & London, J. (2014). Timing and Meter in Mande Drumming
    from Mali. Music Theory Online, 20(1).

[5] Dahl, S. (2004). Playing the Accent - Comparing Striking Velocity
    and Timing in an Ostinato Rhythm Performed by Four Drummers.
    Acta Acustica united with Acustica, 90(4), 762-776.

[6] Danielsen, A., et al. (2015). Effects of instructed timing and
    tempo on snare drum sound in drum kit performance. Journal of
    the Acoustical Society of America, 138(4), 2301-2316.
```

---

## 待用戶確認

### 問題 1：Kotekan 閾值處理方式

**背景說明**：
你的 KotekanEngine 會檢查生成的 kotekan 模式是否「有效」，使用三個百分比閾值：
- 互補性 ≥ 80%（兩個聲部不會同時敲擊的比例）
- 連續性 ≥ 60%（沒有空隙的比例）
- 平衡性 ≥ 60%（兩個聲部音符數量的平衡度）

**問題**：這些數字（80%、60%、60%）是你自己定義的，Michael Tenzer 的學術研究中並沒有使用這種百分比量化方式。Tenzer 使用的是質性描述（如「高度互補」），而非數字閾值。

**選項 A - 移除驗證**：
- 所有生成的 kotekan 都被視為「有效」
- 優點：不會誤導用戶以為這是學術標準
- 缺點：可能產生品質較差的模式

**選項 B - 保留但加註**：
- 保持現有功能，但在程式碼和文檔中說明這是「作者自定義的操作性定義」
- 優點：保持品質控制功能
- 缺點：用戶可能仍誤解為學術標準

**請選擇**: A 或 B？

---

### 問題 2：鬼音力度範圍

**背景說明**：
「鬼音」（Ghost Note）是鼓手在主要拍子之間演奏的極輕聲音符，用來增加律動感。

**原始設定**：12-28%（相對於正常音符的力度）

**問題**：這個範圍沒有學術來源。經過研究，我找到兩篇同行評審論文：
- Matsuo & Sakaguchi (2024)：實驗中使用 1:4 的振幅比，即非重音 = 重音的 **25%**
- Cheng et al. (2022)：實驗中重音比非重音高 10 dB，換算約 **32%**

**建議新範圍**：25-32%

**影響**：
- 鬼音會比原來稍微大聲一點（從 12-28% 變成 25-32%）
- 這個改變有學術論文支持

**請確認**: 接受 25-32% 的新範圍？還是有其他考量？

---

### 問題 3：時間變異數處理方式

**背景說明**：
「時間變異數」是指演奏時與完美節拍之間的微小偏差（以毫秒計）。不同音樂風格有不同的「鬆緊」程度：
- 西非音樂：較鬆散 (±25ms)
- 爵士樂：中等 (±15ms)
- 電子音樂：很緊 (±2-3ms)

**現況**：
你的程式碼中這些數值大致合理，但沒有直接引用學術來源。
- Polak 的研究測量的是「細分間隔」（80-100ms），不是「變異數」
- Friberg 的研究提到爵士樂隊同步在 20ms 以內

**選項 A - 只加註釋**：
- 保持現有數值不變
- 在文檔中說明這些是「基於學術研究的合理估計值」
- 列出參考的學術來源

**選項 B - 調整數值並加註釋**：
- 根據學術資料重新校準數值
- 但這需要更多研究來確定精確數字

**請選擇**: A（只加註釋）還是 B（也調整數值）？

---

### 問題 4：多語言文檔更新

**背景說明**：
你的 WorldRhythm 有多個語言版本的文檔：
- `WorldRhythm_EN.html`（英文）
- `WorldRhythm_TW.html`（繁體中文）
- `WorldRhythm_JP.html`（日文）

這些文檔中包含技術參數說明，例如「Ghost Notes 力度 25-50%」。

**問題**：如果我們修改了鬼音力度等參數，是否也要同步更新這些 HTML 文檔？

**選項 A - 全部更新**：
- 更新所有語言版本
- 優點：保持一致性
- 工作量：較大

**選項 B - 只更新主要文檔**：
- 只更新英文版和程式碼註釋
- 其他語言版本之後再處理

**請選擇**: A（全部更新）還是 B（只更新主要文檔）？

---

*計劃建立日期: 2025-12-16*
