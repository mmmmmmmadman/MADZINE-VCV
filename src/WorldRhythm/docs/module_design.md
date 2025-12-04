# World Rhythm Sequencer 模組設計
## VCV Rack Module Architecture

---

## 一、模組概覽

**名稱建議**：Pulse Matrix / Rhythm Weaver / Polygroove

**尺寸**：32-40 HP（大型模組）

**核心概念**：
- 4 個角色分組，每組 2-3 聲部
- 每組獨立 pattern 長度 → polymeter
- 風格選擇影響 generative 參數
- 一鍵隨機但「懂音樂」的演算法

---

## 二、面板佈局

```
┌─────────────────────────────────────────────────────────────┐
│  [WORLD RHYTHM SEQUENCER]                            32HP   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  MASTER                                    │
│  │ STYLE knob  │  ○ Clock In    ○ Reset In                 │
│  │ (with LED   │  ○ BPM knob    ○ Swing knob               │
│  │  ring)      │                                            │
│  └─────────────┘                                            │
│                                                             │
├──────────────┬──────────────┬──────────────┬────────────────┤
│  TIMELINE    │  FOUNDATION  │   GROOVE     │     LEAD       │
│  (2 voices)  │  (2 voices)  │  (3 voices)  │   (2 voices)   │
├──────────────┼──────────────┼──────────────┼────────────────┤
│              │              │              │                │
│ ○ Style      │ ○ Style      │ ○ Style      │  ○ Style       │
│   + CV       │   + CV       │   + CV       │    + CV        │
│              │              │              │                │
│ ○ Length     │ ○ Length     │ ○ Length     │  ○ Length      │
│   (3-64)     │   (3-64)     │   (3-64)     │    (3-64)      │
│   + CV       │   + CV       │   + CV       │    + CV        │
│              │              │              │                │
│ ○ Density    │ ○ Density    │ ○ Density    │  ○ Density     │
│   + CV       │   + CV       │   + CV       │    + CV        │
│              │              │              │                │
│ ○ Variation  │ ○ Variation  │ ○ Variation  │  ○ Variation   │
│   + CV       │   + CV       │   + CV       │    + CV        │
│              │              │              │                │
│ ○ Rest       │ ○ Rest       │ ○ Rest       │  ○ Rest        │
│   + CV       │   + CV       │   + CV       │    + CV        │
│              │              │              │                │
│ [DICE]       │ [DICE]       │ [DICE]       │  [DICE]        │
│              │              │              │                │
│ LED: ●●      │ LED: ●●      │ LED: ●●●     │  LED: ●●       │
│              │              │              │                │
│ Out1 ○       │ Out1 ○       │ Out1 ○       │  Out1 ○        │
│ Out2 ○       │ Out2 ○       │ Out2 ○       │  Out2 ○        │
│              │              │ Out3 ○       │                │
│ Acc1 ○       │ Acc1 ○       │ Acc1 ○       │  Acc1 ○        │
├──────────────┴──────────────┴──────────────┴────────────────┤
│  [GLOBAL DICE]                      Mix Out ○   Acc Out ○   │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、Style 系統

### Style Knob（連續或 detent）

| 位置 | Style | 主要特徵 |
|------|-------|----------|
| 0° | West African 12/8 | Bell timeline, hemiola, sparse bass |
| 45° | Afro-Cuban | Clave-based, tumbao groove |
| 90° | Brazilian | Surdo anchor, agogô timeline |
| 135° | Balkan Aksak | Asymmetric grouping (7, 9, 11) |
| 180° | Indian Tala | Sam emphasis, tihai-like phrases |
| 225° | Gamelan | Nested colotomic, kotekan interlock |
| 270° | Jazz Swing | Triplet feel, syncopated snare |
| 315° | Electronic | Four-on-floor / breakbeat hybrid |

### Style 影響的參數

每個 Style 定義一組 **generative weights**：

```cpp
struct StyleProfile {
    // 每個角色的 density 範圍
    float timeline_density_min, timeline_density_max;  // e.g., 0.4-0.6
    float foundation_density_min, foundation_density_max;  // e.g., 0.1-0.25
    float groove_density_min, groove_density_max;
    float lead_density_min, lead_density_max;
    
    // Pulse position weights (16 positions)
    float timeline_weights[16];
    float foundation_weights[16];
    float groove_weights[16];
    float lead_weights[16];
    
    // Swing profile
    float swing_amount;  // 0.5 = straight, 0.67 = triplet
    
    // Interlock rules
    bool avoid_foundation_on_timeline;
    bool groove_complements_foundation;
    
    // Suggested lengths
    int suggested_lengths[4];  // per group
};
```

---

## 四、Generative 演算法

### 核心原則：不是 Euclidean，而是 Weighted Probability

**問題**：Euclidean 產生 maximally even patterns，但真實音樂有：
- 特定位置偏好（downbeat, clave positions）
- 角色間互補（foundation 不在 timeline 同位置）
- 密度差異（timeline 密，foundation 疏）

### Algorithm: Role-Aware Weighted Generation

```cpp
Pattern generatePattern(Role role, int length, float density, StyleProfile style) {
    Pattern p(length);
    
    // 1. 取得此角色在此 style 的 position weights
    float* weights = getWeightsForRole(role, style);
    
    // 2. 根據 density 決定目標 onset 數
    int targetOnsets = round(length * density);
    
    // 3. 計算每個 position 的調整後權重
    float adjustedWeights[length];
    for (int i = 0; i < length; i++) {
        int mappedPos = (i * 16) / length;  // map to 16-grid
        adjustedWeights[i] = weights[mappedPos];
        
        // 4. 如果有互鎖規則，降低與其他角色衝突的位置權重
        if (style.avoid_foundation_on_timeline && role == FOUNDATION) {
            if (timelinePattern.hasOnsetAt(i)) {
                adjustedWeights[i] *= 0.2;  // 大幅降低
            }
        }
    }
    
    // 5. 用 weighted random selection 選擇 onset 位置
    vector<int> selectedPositions = weightedSelect(adjustedWeights, targetOnsets);
    
    for (int pos : selectedPositions) {
        p.setOnset(pos, true);
    }
    
    return p;
}
```

### Position Weight 範例

**West African Style - Timeline (bell)**:
```
Position:  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
Weight:   1.0 0.1 0.7 0.1 0.8 1.0 0.1 0.9 0.1 0.8 0.1 0.9 0.2 0.1 0.2 0.1
          ↑       ↑       ↑   ↑       ↑       ↑       ↑
          Standard bell pattern 的典型位置有高權重
```

**Afro-Cuban Style - Foundation (tumba)**:
```
Position:  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
Weight:   0.3 0.1 0.8 0.1 0.1 0.1 0.9 1.0 0.1 0.1 0.7 0.1 0.1 0.1 0.8 0.9
                  ↑               ↑   ↑           ↑               ↑   ↑
                  Tumbao ponche 位置
```

**Electronic Style - Foundation (kick)**:
```
Position:  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
Weight:   1.0 0.1 0.1 0.1 1.0 0.1 0.1 0.1 1.0 0.1 0.1 0.1 1.0 0.1 0.1 0.1
          ↑               ↑               ↑               ↑
          Four-on-the-floor
```

---

## 五、互鎖邏輯

### Interlock Rules（角色間互補）

```cpp
void applyInterlockRules(Pattern& foundation, Pattern& groove, StyleProfile style) {
    if (style.groove_complements_foundation) {
        for (int i = 0; i < foundation.length; i++) {
            if (foundation.hasOnsetAt(i)) {
                // Groove 在 foundation hit 位置的權重降低
                groove.reduceWeightAt(i, 0.3);
                // 但前後位置權重增加（互補填充）
                groove.increaseWeightAt((i + 1) % groove.length, 1.5);
                groove.increaseWeightAt((i - 1 + groove.length) % groove.length, 1.2);
            }
        }
    }
}
```

### Kotekan-style Interlock（Groove 內部兩聲部）

當 Groove 有多個 output 時，可產生互鎖 pattern：

```cpp
void generateKotekanPair(Pattern& polos, Pattern& sangsih, int length, float density) {
    // Polos: on-beat 傾向
    float polosWeights[16] = {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    // Sangsih: off-beat 傾向
    float sangsihWeights[16] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
    
    polos = generateWithWeights(polosWeights, length, density);
    sangsih = generateWithWeights(sangsihWeights, length, density);
    
    // 確保不完全重疊
    resolveOverlaps(polos, sangsih);
}
```

---

## 六、Variation 參數

每組的 **Variation** knob 控制隨機程度：

| Variation 值 | 效果 |
|-------------|------|
| 0% | 嚴格遵循 style 的 position weights |
| 50% | Weights 與 uniform random 混合 |
| 100% | 完全隨機（忽略 weights，只保留 density） |

```cpp
float getFinalWeight(int pos, float styleWeight, float variation) {
    float uniform = 1.0;
    return styleWeight * (1.0 - variation) + uniform * variation;
}
```

---

## 七、Rest 系統

### 概念
Rest knob 在已生成的 pattern 上「挖洞」，創造節奏性的靜默。與降低 density 不同：density 決定「生成多少 onset」，rest 決定「哪些 onset 被靜音」。

### 參數
- **Rest Amount** (0-100%): 靜音機率
- **Rest CV**: 外部調變

### Rhythmic Rest 生成規則

**Position-Weighted Rest（非純隨機）**：
```cpp
float getRestProbability(int position, Role role, float restAmount, StyleProfile style) {
    // 基礎機率
    float prob = restAmount;
    
    // 強拍不易 rest（維持骨架）
    if (isStrongBeat(position)) prob *= 0.3;
    
    // 弱拍容易 rest（清理密度）
    if (isWeakSubdivision(position)) prob *= 1.5;
    
    // 角色上限
    float roleMax;
    switch (role) {
        case TIMELINE:   roleMax = 0.2; break;  // Timeline 最多 20% rest
        case FOUNDATION: roleMax = 0.4; break;  // Foundation 維持錨定
        case GROOVE:     roleMax = 1.0; break;  // Groove 最自由
        case LEAD:       roleMax = 1.0; break;  // Lead 可大量 rest
    }
    
    return min(prob, roleMax);
}
```

### Style-Influenced Rest 行為

| Style | Rest 特性 |
|-------|----------|
| West African | Rest 避開 timeline，創造 call-response 空間 |
| Afro-Cuban | Rest 避開 clave 位置，填充之間 |
| Indian | Rest 創造 tihai-like 短語邊界 |
| Gamelan | 傾向同步 rest（angsel 風格的齊斷） |
| Jazz | 隨機但尊重 swing feel |
| Electronic | Grid-locked rest，創造 stutter/glitch 效果 |

### Rest Clustering（連續 rest 傾向）

```cpp
float getClusterProbability(float restAmount) {
    // 低 rest: 散落單點
    if (restAmount < 0.3) return 0.0;
    // 中 rest: 有時 2-3 連續
    if (restAmount < 0.6) return 0.3;
    // 高 rest: 長空白，短語隔離
    return 0.6;
}

// 若前一位置是 rest，這位置更可能也是 rest
if (previousWasRest && random() < clusterProb) {
    currentRest = true;
}
```

### Accent 保護

Accented 位置較不易被 rest：
```cpp
if (isAccented(position)) prob *= 0.5;
```

### 參數互動

| 組合 | 效果 |
|------|------|
| High Density + Low Rest | 密集 pattern |
| High Density + High Rest | 碎片化、glitchy |
| Low Density + Low Rest | 稀疏但穩定 |
| Low Density + High Rest | 極簡，大量空白 |

### 音樂用途

1. **Breakdown**: Groove/Lead 增加 rest，Timeline/Foundation 維持
2. **Build-up**: 漸減 rest 走向高潮
3. **Call-Response**: CV 控制兩組交替高/低 rest
4. **Humanize**: 低 rest (5-15%) 加入自然呼吸感

---

## 八、Accent 生成

每組有獨立的 Accent output，規則：

1. **Timeline**：第一拍和 strong beats 有 accent
2. **Foundation**：所有 hit 都是 accent（本來就少）
3. **Groove**：依 position weight 高低決定 accent
4. **Lead**：隨機 accent，模擬即興

```cpp
bool shouldAccent(Role role, int position, StyleProfile style) {
    switch (role) {
        case TIMELINE:
            return position == 0 || style.timeline_weights[position] > 0.8;
        case FOUNDATION:
            return true;  // all foundation hits are accented
        case GROOVE:
            return random() < style.groove_weights[position];
        case LEAD:
            return random() < 0.3;  // 30% random accent
    }
}
```

---

## 九、Polymeter 處理

### 每組獨立 Length

```cpp
struct SequencerState {
    int lengths[4] = {12, 8, 16, 7};  // Timeline, Foundation, Groove, Lead
    int positions[4] = {0, 0, 0, 0};   // current position per group
    
    void advance() {
        for (int i = 0; i < 4; i++) {
            positions[i] = (positions[i] + 1) % lengths[i];
        }
    }
    
    int getLCM() {
        // 計算所有長度的最小公倍數，用於 master reset
    }
};
```

### Reset 行為選項（可用 context menu）

- **Immediate**：Reset 時所有組歸零
- **Quantized**：等到下一個 downbeat
- **Per-group**：只有特定組 reset

---

## 十、輸入/輸出規格

### Inputs

| Port | 功能 |
|------|------|
| Clock | Master clock (每 pulse 一次) |
| Reset | 重設所有 position |
| Style CV ×4 | 每組 style 調變 |
| Length CV ×4 | 每組 length 調變 |
| Density CV ×4 | 每組 density 調變 |
| Variation CV ×4 | 每組 variation 調變 |
| Rest CV ×4 | 每組 rest 調變 |

### Outputs

| Port | 訊號 |
|------|------|
| Timeline 1-2 | Gate |
| Foundation 1-2 | Gate |
| Groove 1-3 | Gate |
| Lead 1-2 | Gate |
| Accent ×4 | 每組一個 accent gate |
| Mix | 所有 gate 混合 |
| Accent Mix | 所有 accent 混合 |

---

## 十一、UI 互動細節

### DICE 按鈕行為

- **單擊**：只 randomize 該組（保持其他組）
- **長按**：Randomize 該組 + 重新計算互鎖

### Global DICE

- 一次 randomize 全部，但依序執行：
  1. Timeline → 2. Foundation → 3. Groove → 4. Lead
  - 每步都考慮前一步的結果（互鎖）

### LED 顯示

- 每組的 LED 顯示當前 step 位置
- 亮 = 有 onset，暗 = 無
- Accent 時更亮

---

## 十二、下一步

1. 定義所有 8 個 style 的完整 weight tables
2. 決定 CV 輸入範圍和 attenuverter 需求
3. 設計 context menu 選項（reset 行為、swing per-group 等）
4. 面板 SVG 繪製

要先處理哪個部分？
