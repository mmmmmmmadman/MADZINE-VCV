# World Rhythm Sequencer - 開發說明

## 專案概述

VCV Rack 模組：基於世界節奏傳統的 generative rhythm sequencer

**核心特色：**
- Weighted probability 演算法（非 Euclidean）
- 4 角色分組 × 獨立 polymeter
- 8 種風格 preset 影響生成權重
- 角色間互鎖邏輯

---

## 檔案結構

```
/research
  unified_rhythm_analysis.md   - 節奏系統統一分析
  fills_ornaments_research.md  - 過門與裝飾音研究

/design
  module_design.md             - 模組架構設計

/src (待建立)
  plugin.cpp
  WorldRhythm.cpp
  WorldRhythm.hpp
  StyleProfiles.hpp
  GenerativeEngine.cpp
```

---

## 模組規格

| 項目 | 規格 |
|------|------|
| 尺寸 | 32-40 HP |
| 分組 | Timeline (2), Foundation (2), Groove (3), Lead (2) |
| 每組參數 | Style, Length (3-64), Density, Variation, Rest |
| CV inputs | 5 per group × 4 + Clock + Reset = 22 |
| Outputs | 9 gate + 4 accent + 2 mix = 15 |

---

## 核心演算法

### 1. Pattern Generation

```cpp
// 非 Euclidean - 使用 position weights
Pattern generate(Role role, StyleProfile style, int length, float density, float variation) {
    float weights[length];
    
    // Map style weights to pattern length
    for (int i = 0; i < length; i++) {
        int mapped = (i * 16) / length;
        weights[i] = style.weights[role][mapped];
        
        // Apply variation (blend with uniform)
        weights[i] = weights[i] * (1 - variation) + variation;
    }
    
    // Weighted random selection
    int targetOnsets = round(length * density);
    return weightedSelect(weights, targetOnsets);
}
```

### 2. Interlock Logic

```cpp
// Foundation 避開 Timeline
if (timeline.hasOnset(pos)) {
    foundation.weight[pos] *= 0.2;
    foundation.weight[pos+1] *= 1.5;  // 互補
}

// Groove Kotekan (2 voices)
polos:   on-beat weighted
sangsih: off-beat weighted
```

### 3. Rest System

```cpp
float restProb = restAmount;
if (isStrongBeat(pos)) restProb *= 0.3;
if (isAccented(pos)) restProb *= 0.5;
restProb = min(restProb, roleMaxRest[role]);
```

### 4. Accent Generation

- Timeline: strong beats (weight > 0.8)
- Foundation: all hits
- Groove: probability = position weight
- Lead: 30% random

---

## Style Profiles

| ID | Style | Swing | Key Weights |
|----|-------|-------|-------------|
| 0 | West African | 0.62 | Bell: 1,3,5,6,8,10,12 |
| 1 | Afro-Cuban | 0.58 | Clave: 1,4,7,11,13 |
| 2 | Brazilian | 0.57 | Surdo: 1,5,9,13 |
| 3 | Balkan | 0.50 | Aksak: asymmetric grouping |
| 4 | Indian | 0.50 | Sam emphasis, tihai |
| 5 | Gamelan | 0.50 | Colotomic nested |
| 6 | Jazz | 0.65 | Triplet feel |
| 7 | Electronic | 0.50 | Four-on-floor |

---

## 開發順序建議

### Phase 1: Core Engine
1. [ ] StyleProfile struct 定義
2. [ ] Weight tables (16 pos × 4 roles × 8 styles)
3. [ ] Weighted selection algorithm
4. [ ] Basic pattern storage

### Phase 2: Interlock
5. [ ] Timeline → Foundation avoidance
6. [ ] Groove kotekan generation
7. [ ] Global DICE sequential generation

### Phase 3: Modulation
8. [ ] Rest system
9. [ ] Accent generation
10. [ ] Variation blending

### Phase 4: I/O
11. [ ] Clock/Reset handling
12. [ ] Polymeter position tracking
13. [ ] Gate output generation
14. [ ] CV input processing

### Phase 5: UI
15. [ ] Panel SVG design
16. [ ] Knob/LED layout
17. [ ] DICE button behavior
18. [ ] Context menu options

---

## 技術注意事項

### Polymeter LCM
```cpp
// 計算 master cycle length
int lcm = lengths[0];
for (int i = 1; i < 4; i++) {
    lcm = std::lcm(lcm, lengths[i]);
}
// 用於 global reset timing
```

### CV Range
- 所有 CV: ±5V bipolar with attenuverter
- Style CV: 0-10V maps to 0-7 (8 styles)
- Length CV: quantized to integers

### Gate Timing
- Gate length: 1ms or until next clock (option)
- Accent: higher voltage (10V vs 5V) or separate output

---

## 研究文件參考

### 節奏分析 (unified_rhythm_analysis.md)
- 頻率分層：低/中/高頻角色分配
- 5 種統一角色：Timeline, Foundation, Groove, Interlock, Lead
- 4 種 Polyrhythm 類型：Cross-rhythm, Polymeter, Metric mod, Asymmetric
- Swing/Microtiming 數據

### 過門研究 (fills_ornaments_research.md)
- Fill 位置規則：4/8/16/32 bar 週期
- Tihai 公式：(Phrase×3)+(Gap×2)
- Trap hi-hat 細分技法
- EDM buildup 結構

---

## 參考資源

- VCV Rack SDK: https://vcvrack.com/manual/PluginDevelopmentTutorial
- Toussaint, G. "The Euclidean Algorithm Generates Traditional Musical Rhythms"
- Pressing, J. "Cognitive Isomorphisms in World Music"
- Tenzer, M. "Gamelan Gong Kebyar"
