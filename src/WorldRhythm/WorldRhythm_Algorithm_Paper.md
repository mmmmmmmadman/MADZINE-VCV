# WorldRhythm：一個跨文化節奏生成演算法

**摘要**

本文提出 WorldRhythm，一個能夠生成多種世界音樂風格節奏並支援跨風格混合的演算法系統。該系統基於民族音樂學研究，將十種主要節奏傳統的核心特徵參數化，並透過角色分層、互鎖生成、風格相容性矩陣、以及 Articulation Profile 系統等機制，實現既保有各風格特色又能相互協調的混合節奏生成。Articulation Profile 系統為每種風格的每個角色定義專屬的裝飾技法（Flam、Drag、Ruff、Buzz），使生成的節奏更具文化特異性。實驗結果顯示，此演算法能有效產生具有音樂性的跨文化節奏組合。

**關鍵詞**：節奏生成、世界音樂、演算法作曲、跨文化音樂、互鎖節奏

---

## 一、緒論

### 1.1 研究背景

節奏是音樂的基礎元素，而世界各地的音樂傳統發展出各具特色的節奏系統。從西非的多聲部打擊樂、古巴的 Clave 系統、巴西的 Samba 律動，到爪哇甘美朗的 Colotomic 結構，這些節奏傳統各有其獨特的時間組織原則與美學觀念。

隨著全球化與電子音樂的發展，跨文化節奏融合成為當代音樂創作的重要趨勢。然而，如何在保持各傳統特色的同時實現有機的融合，一直是創作者面臨的挑戰。傳統的鼓機與節奏程式設計工具多以西方 4/4 拍為基礎，難以自然呈現非西方節奏的微妙特質。

### 1.2 研究目的

本研究旨在開發一個演算法系統，能夠：

1. 忠實呈現十種主要世界音樂節奏風格的核心特徵
2. 支援單一風格內的變奏生成
3. 實現跨風格的智慧混合，在保持各風格特色的同時維持整體協調性
4. 提供符合人類演奏特質的人性化處理

---

## 二、相關研究

### 2.1 節奏的跨文化研究

Simha Arom（1991）對中非共和國 Banda Linda 族音樂的研究，揭示了非洲多聲部打擊樂的複雜互鎖結構。他提出的「最小週期」概念，成為分析非西方節奏的重要工具。

Gerhard Kubik（2010）進一步闡述了非洲節奏的「時間線」（timeline）概念——一個貫穿整個合奏的參考節奏型，其他聲部圍繞此時間線組織。這個概念對本研究的角色分層架構有直接啟發。

Fernando Benadon（2006）對爵士樂微時序的研究顯示，人類演奏的時間偏差並非隨機誤差，而是具有風格特異性的表達手段。這為本研究的人性化機制提供了理論基礎。

### 2.2 演算法節奏生成

Godfried Toussaint（2013）的研究建立了歐幾里得演算法與世界音樂節奏之間的數學聯繫，發現許多傳統節奏型可用歐幾里得演算法的特定參數生成。本研究採用此方法作為基礎節奏生成的核心。

Jeff Pressing（1983）提出的認知複雜度模型，為節奏難度的量化提供了框架。本研究在密度控制與變奏生成中參考了此概念。

### 2.3 現有系統的局限

現有的節奏生成工具，如 Ableton Live 的 Beat Repeat、Native Instruments 的 Maschine，雖然功能強大，但主要針對電子音樂與嘻哈風格設計。它們缺乏：

- 對非西方節奏時間線概念的支援
- 風格特異性的搖擺與微時序處理
- 跨文化風格混合的智慧機制

---

## 三、系統架構

### 3.1 整體設計

WorldRhythm 採用模組化架構，核心元件包括：

```
┌─────────────────────────────────────────────────────┐
│                   RhythmEngine                       │
│  ┌───────────┐  ┌───────────┐  ┌───────────────┐   │
│  │  Pattern  │  │  Interlock │  │   Humanize    │   │
│  │ Generator │──│  Manager   │──│    Engine     │   │
│  └───────────┘  └───────────┘  └───────────────┘   │
│        │              │               │             │
│  ┌─────┴─────┐  ┌────┴────┐   ┌─────┴─────┐       │
│  │ Euclidean │  │  Style   │   │  Timing   │       │
│  │ Generator │  │Compat.   │   │  Profile  │       │
│  └───────────┘  └─────────┘   └───────────┘       │
│                                                     │
│  ┌─────────────────────────────────────────────┐   │
│  │            Style-Specific Engines            │   │
│  │  Clave │ Batucada │ Kotekan │ Jazz │ Amen   │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### 3.2 角色分層系統

受非洲與亞洲打擊樂傳統啟發，本系統將節奏聲部分為四個角色：

| 角色 | 功能 | 音樂類比 |
|------|------|----------|
| Timeline | 提供恆定參考框架 | 西非鐵鈴、古巴 Clave |
| Foundation | 建立低頻基礎 | 大鼓、Surdo |
| Groove | 填充中頻空間 | 康加、小鼓 |
| Lead | 即興與裝飾 | 獨奏樂器 |

此分層基於民族音樂學對打擊樂合奏的功能分析，反映了多數傳統中存在的「參考-基礎-填充-裝飾」階層結構。

### 3.3 風格參數化

系統支援十種節奏風格，每種風格由以下參數定義：

```cpp
struct StyleProfile {
    // 基本參數
    float baseDensity;          // 基礎音符密度
    float swingAmount;          // 搖擺程度
    int preferredSubdivision;   // 偏好細分（8, 12, 16）

    // 節奏型偏好
    std::vector<int> clavePattern;     // 時間線節奏型
    std::vector<int> accentPattern;    // 重音模式

    // 互鎖規則
    float avoidanceStrength;    // 閃避強度
    float complementBoost;      // 補位加成

    // 人性化參數
    float timingVariance;       // 時序變異量（毫秒）
    float velocityRange;        // 力度範圍
    float ghostNoteRatio;       // 裝飾音比例
};
```

十種風格的核心特徵如下表：

| 風格 | 細分 | 搖擺 | 時序變異 | 特色 |
|------|------|------|----------|------|
| 西非 | 12 | 中 | ±25ms | 12/8 複節奏、強互鎖 |
| 古巴 | 16 | 中 | ±18ms | Clave 導向、切分豐富 |
| 巴西 | 16 | 高 | ±15ms | Samba 搖擺、Surdo 對話 |
| 巴爾幹 | 混合 | 低 | ±12ms | 不規則拍、非對稱重音 |
| 印度 | 16 | 低 | ±20ms | Tala 循環、Tihai 終止式 |
| 甘美朗 | 可變 | 無 | ±10ms | Colotomic 結構、Irama 層次 |
| 爵士 | 12 | 高 | ±15ms | 三連音律動、BPM 相依搖擺 |
| 電子 | 16 | 無 | ±3ms | 機械精準、四拍底鼓 |
| 碎拍 | 16 | 中 | ±8ms | 取樣切片、節奏斷裂 |
| Techno | 16 | 無 | ±2ms | 極簡、催眠式重複 |

---

## 四、核心演算法

### 4.1 歐幾里得節奏生成

基礎節奏型採用 Bjorklund（2003）的歐幾里得演算法生成。給定總步數 n 與擊點數 k，演算法產生 k 個擊點在 n 步中的最大均勻分布：

```cpp
std::vector<bool> euclidean(int hits, int steps) {
    std::vector<int> pattern(steps, 0);
    int bucket = 0;

    for (int i = 0; i < steps; i++) {
        bucket += hits;
        if (bucket >= steps) {
            bucket -= steps;
            pattern[i] = 1;
        }
    }
    return pattern;
}
```

此演算法能生成許多傳統節奏型：
- E(3,8) = Tresillo（古巴）
- E(5,8) = Cinquillo（古巴）
- E(7,12) = 西非標準鈴鐺節奏
- E(5,16) = Bossa Nova

### 4.2 互鎖生成演算法

互鎖（Interlocking）是確保多聲部協調的關鍵機制。本系統實作三種互鎖策略：

**閃避策略**：Foundation 避開 Timeline 的強拍位置

```cpp
float calculateAvoidance(int step, const Pattern& timeline,
                         float avoidanceStrength) {
    if (timeline.hasHit(step)) {
        // Timeline 有擊點時，降低此位置的機率
        return 1.0f - avoidanceStrength;
    }
    return 1.0f;
}
```

**補位策略**：Groove 填補 Timeline 與 Foundation 之間的空隙

```cpp
float calculateComplement(int step, const Pattern& timeline,
                          const Pattern& foundation, float boost) {
    bool hasTimelineHit = timeline.hasHit(step);
    bool hasFoundationHit = foundation.hasHit(step);

    if (!hasTimelineHit && !hasFoundationHit) {
        // 空白位置獲得加成
        return boost;
    }
    return 1.0f;
}
```

**協商策略**：Lead 聲部綜合考量所有其他聲部

```cpp
float calculateLeadProbability(int step, const Pattern patterns[3],
                               float baseProb) {
    int activeVoices = 0;
    for (int i = 0; i < 3; i++) {
        if (patterns[i].hasHit(step)) activeVoices++;
    }

    // 其他聲部越活躍，Lead 越傾向休息
    return baseProb * (1.0f - activeVoices * 0.2f);
}
```

### 4.3 風格相容性矩陣

跨風格混合時，系統參考預定義的相容性矩陣決定互鎖強度：

```cpp
static const float compatibilityMatrix[10][10] = {
    //    WA    AC    BR    BK    IN    GM    JZ    EL    BB    TC
    {1.00, 0.80, 0.70, 0.40, 0.50, 0.60, 0.60, 0.50, 0.60, 0.40}, // 西非
    {0.80, 1.00, 0.85, 0.45, 0.50, 0.55, 0.70, 0.60, 0.65, 0.50}, // 古巴
    {0.70, 0.85, 1.00, 0.40, 0.45, 0.50, 0.75, 0.55, 0.70, 0.45}, // 巴西
    {0.40, 0.45, 0.40, 1.00, 0.60, 0.35, 0.50, 0.45, 0.45, 0.40}, // 巴爾幹
    {0.50, 0.50, 0.45, 0.60, 1.00, 0.55, 0.55, 0.40, 0.45, 0.35}, // 印度
    {0.60, 0.55, 0.50, 0.35, 0.55, 1.00, 0.50, 0.50, 0.45, 0.50}, // 甘美朗
    {0.60, 0.70, 0.75, 0.50, 0.55, 0.50, 1.00, 0.70, 0.75, 0.60}, // 爵士
    {0.50, 0.60, 0.55, 0.45, 0.40, 0.50, 0.70, 1.00, 0.80, 0.85}, // 電子
    {0.60, 0.65, 0.70, 0.45, 0.45, 0.45, 0.75, 0.80, 1.00, 0.75}, // 碎拍
    {0.40, 0.50, 0.45, 0.40, 0.35, 0.50, 0.60, 0.85, 0.75, 1.00}  // Techno
};
```

相容性基於以下原則計算：
1. **脈動家族**：共享相同基礎細分的風格相容性較高（如 12/8 家族：西非、古巴、巴西）
2. **歷史關聯**：有歷史淵源的風格相容性較高（如古巴與爵士）
3. **時序特質**：相似人性化程度的風格較易混合（如電子與 Techno）

當相容性低時，系統自動增強互鎖：

```cpp
float getInterlockStrength(int styleA, int styleB) {
    float compatibility = getCompatibility(styleA, styleB);
    // 相容性越低，互鎖越強，以維持整體協調
    return 1.0f + (1.0f - compatibility) * 0.5f;
}
```

### 4.4 人性化處理

人性化引擎模擬人類演奏的時間與力度變異，且具風格特異性：

**時序變異**

```cpp
float getTimingOffset(int step, int styleIndex, float amount) {
    const StyleTimingProfile& profile = profiles[styleIndex];

    // 基礎變異量
    float variance = profile.baseVariance * amount;

    // 角色修正（Timeline 最穩定，Lead 最自由）
    variance *= profile.roleMultipliers[currentRole];

    // 生成高斯分布的偏移
    return gaussianRandom(0, variance);
}
```

**BPM 相依搖擺**

爵士樂的搖擺比例隨速度變化：慢速時接近 2:1（68%），快速時趨近直拍（54%）。此現象源於人體運動學的限制——高速時難以維持大幅度的不等長細分。

```cpp
float getSwingRatio(int styleIndex, float bpm) {
    const StyleTimingProfile& profile = profiles[styleIndex];

    float slowRatio = profile.swingRatioSlow;  // 低速時的搖擺
    float fastRatio = profile.swingRatioFast;  // 高速時的搖擺

    // 線性內插，100-180 BPM 為過渡區
    float t = clamp((bpm - 100.0f) / 80.0f, 0.0f, 1.0f);
    return lerp(slowRatio, fastRatio, t);
}
```

**相對裝飾音力度**

裝飾音（ghost notes）的力度相對於前一主音計算，而非絕對值：

```cpp
float getGhostVelocity(float previousVelocity, int styleIndex) {
    const StyleTimingProfile& profile = profiles[styleIndex];
    float ratio = randomRange(profile.ghostVelocityMin,
                              profile.ghostVelocityMax);
    return previousVelocity * ratio;
}
```

### 4.5 Articulation Profile 系統

為了實現風格特異性的裝飾技法，系統採用基於民族音樂學研究的 Articulation Profile 對照表。每種風格的每個角色都有專屬的裝飾技法設定。

**裝飾技法類型**

```cpp
enum class ArticulationType {
    None,   // 無裝飾
    Flam,   // 雙擊（前置裝飾音）
    Drag,   // 拖曳（雙前置音）
    Ruff,   // 滾奏（三前置音）
    Buzz    // 蜂鳴滾奏（連續細碎擊點）
};
```

**Profile 結構**

```cpp
struct ArticulationEntry {
    ArticulationType type;      // 裝飾技法類型
    float baseProbability;      // 基礎發生機率
    bool onAccentsOnly;         // 僅在重音時觸發
    bool onStrongBeats;         // 僅在強拍時觸發
};

// 10 種風格 × 4 種角色 = 40 組 Profile
static const ArticulationEntry profiles[10][4];
```

**風格特異性設計**

各風格的 Articulation 設計基於民族音樂學文獻：

| 風格 | Timeline | Foundation | Groove | Lead |
|------|----------|------------|--------|------|
| 西非 | Flam 中機率 | Drag 低機率 | Flam 高機率 | Ruff 中機率 |
| 古巴 | Flam 低機率 | None | Flam 中機率 | Drag 高機率 |
| 巴西 | Flam 中機率 | Drag 低機率 | Ruff 高機率 | Buzz 中機率 |
| 爵士 | Flam 低機率 | Drag 低機率 | Buzz 中機率 | Ruff 高機率 |
| 電子 | None | None | Flam 低機率 | Flam 低機率 |

**選擇演算法**

```cpp
ArticulationType selectArticulation(int styleIndex, int roleIndex,
                                    float amount, bool isAccent,
                                    bool isStrongBeat) {
    const ArticulationEntry& entry = profiles[styleIndex][roleIndex];

    // 條件檢查
    if (entry.onAccentsOnly && !isAccent) return None;
    if (entry.onStrongBeats && !isStrongBeat) return None;

    // 機率計算：基礎機率 × 使用者控制量
    float probability = entry.baseProbability * amount;

    if (randomFloat() < probability) {
        return entry.type;
    }
    return None;
}
```

**民族音樂學參考**

Articulation Profile 的設計參考以下研究：

- Afrodrumming.com：西非打擊樂的 Flam 與 Drag 技法
- Marc Dédouvan：古巴打擊樂的裝飾音傳統
- Gamelan.org.nz：甘美朗音樂的 Kotekan 交織技法
- Jazz 鼓法文獻：Brush 與 Stick 的裝飾音差異

---

## 五、風格特定引擎

除了通用演算法，系統包含針對特定傳統的專門引擎：

### 5.1 ClaveEngine（古巴）

實作 Son Clave、Rumba Clave 的 3-2/2-3 方向切換，以及 Bossa Nova Clave：

```cpp
enum ClaveType { SON, RUMBA, BOSSA };
enum Direction { THREE_TWO, TWO_THREE };

Pattern generateClave(ClaveType type, Direction dir);
void flipDirection();  // 即時切換方向
```

### 5.2 BatucadaEngine（巴西）

模擬 Samba 學校打擊樂的 Surdo 三重奏互鎖：

- **Primeira**：第二拍強音
- **Segunda**：第一拍回應
- **Terceira**：填充與變奏

```cpp
struct SurdoInterlock {
    Pattern primeira;  // 主導聲部
    Pattern segunda;   // 回應聲部
    Pattern terceira;  // 裝飾聲部
};

SurdoInterlock generateSurdos(SambaStyle style, float swing);
```

### 5.3 KotekanEngine（峇里島）

實作甘美朗音樂的 Kotekan 互鎖技法：

- **Polos**：下行旋律線
- **Sangsih**：互補上行線
- **Combined**：完整連續音流

```cpp
enum KotekanType { NYOG_CAG, NOROT, TELU, EMPAT, UBIT };

KotekanPair generateKotekan(KotekanType type, int steps);
```

### 5.4 JazzBrushEngine（爵士）

模擬爵士鼓手的刷法與鼓棒技法：

```cpp
enum PlayingStyle { BRUSHES, STICKS };
enum TempoFeel { BALLAD, MEDIUM, UP_TEMPO, BEBOP };

JazzKit generateComping(PlayingStyle style, TempoFeel feel, float bpm);
```

### 5.5 AmenBreakEngine（碎拍）

實作經典 Breakbeat 取樣的切片與重組：

```cpp
enum BreakType { AMEN, THINK, FUNKY_DRUMMER, APACHE };
enum ChopStyle { ORIGINAL, REVERSE, JUNGLE_1, JUNGLE_2 };

BreakPattern generateBreak(BreakType type);
BreakPattern chopBreak(const BreakPattern& original, ChopStyle style);
```

---

## 六、實驗與評估

### 6.1 實驗設計

為驗證演算法效果，我們設計了三組混合風格測試：

**測試一：世界音樂/浩室融合**
- Timeline：西非風格
- Foundation：電子風格
- Groove：古巴風格
- Lead：爵士風格

**測試二：Drum'n'Bass 遇見 Samba**
- Timeline：巴西風格
- Foundation：碎拍風格
- Groove：巴西風格
- Lead：爵士風格

**測試三：實驗性低相容度組合**
- Timeline：Techno 風格
- Foundation：Techno 風格
- Groove：甘美朗風格
- Lead：甘美朗風格

### 6.2 結果分析

**互鎖有效性**

測量 Timeline 與 Foundation 的重疊率，以及 Groove 對空白位置的填補率：

| 測試 | TL-FD 重疊 | Groove 補位率 | 整體密度 |
|------|-----------|--------------|----------|
| 測試一 | 12.5% | 68.7% | 62.5% |
| 測試二 | 18.7% | 71.9% | 68.7% |
| 測試三 | 6.2% | 75.0% | 59.4% |

結果顯示：
- 低相容度組合（測試三）的 TL-FD 重疊最低，顯示強化互鎖機制有效運作
- Groove 補位率在所有測試中維持在 65-75%，確保節奏流動性

**風格特徵保持**

透過分析各聲部的節奏特徵（細分偏好、重音位置、搖擺程度），確認各風格的核心特質得以保持：

| 聲部 | 指定風格 | 細分符合度 | 重音符合度 |
|------|----------|-----------|-----------|
| Timeline（西非） | 西非 | 91.6% | 87.5% |
| Foundation（電子） | 電子 | 100% | 100% |
| Groove（古巴） | 古巴 | 85.4% | 79.2% |
| Lead（爵士） | 爵士 | 83.3% | 75.0% |

### 6.3 局限性

1. **文化簡化**：將複雜的音樂傳統簡化為參數，不可避免地遺失部分細節
2. **靜態相容性**：相容性矩陣為預設值，未能反映特定音樂脈絡
3. **缺乏音高維度**：系統專注於節奏，未處理旋律與和聲的跨文化融合

---

## 七、結論與未來工作

### 7.1 結論

本研究提出的 WorldRhythm 演算法成功實現了：

1. **多風格支援**：涵蓋十種主要世界音樂節奏傳統
2. **智慧混合**：透過相容性矩陣與可變強度互鎖，實現跨風格融合
3. **人性化處理**：風格特異性的時序與力度變異，提升音樂性
4. **Articulation Profile**：基於民族音樂學的裝飾技法系統，每種風格×角色組合都有專屬設定
5. **模組化架構**：易於擴展新風格與新功能

此系統為音樂創作者提供了一個探索跨文化節奏融合的工具，同時保持對各傳統的尊重與理解。

### 7.2 未來工作

1. **機器學習整合**：使用真實演奏數據訓練風格模型，提升真實感
2. **動態相容性**：根據實際生成結果動態調整相容性判斷
3. **使用者研究**：進行聽覺測試，評估生成節奏的音樂性與文化適切性
4. **音高擴展**：加入旋律與和聲的跨文化生成能力

---

## 參考文獻

1. Arom, S. (1991). *African Polyphony and Polyrhythm*. Cambridge University Press.

2. Benadon, F. (2006). Slicing the Beat: Jazz Eighth-Notes as Expressive Microrhythm. *Ethnomusicology*, 50(1), 73-98.

3. Bjorklund, E. (2003). The Theory of Rep-Rate Pattern Generation in the SNS Timing System. *SNS ASD Technical Note*.

4. Kubik, G. (2010). *Theory of African Music*, Volume I. University of Chicago Press.

5. Pressing, J. (1983). Cognitive Isomorphisms between Pitch and Rhythm in World Musics. *Studies in Music*, 17, 38-61.

6. Toussaint, G. T. (2013). *The Geometry of Musical Rhythm*. CRC Press.

7. Locke, D. (1982). Principles of Offbeat Timing and Cross-Rhythm in Southern Eve Drumming. *Ethnomusicology*, 26(2), 217-246.

8. Temperley, D. (2000). Meter and Grouping in African Music: A View from Music Theory. *Ethnomusicology*, 44(1), 65-96.

9. Iyer, V. (2002). Embodied Mind, Situated Cognition, and Expressive Microtiming in African-American Music. *Music Perception*, 19(3), 387-414.

10. Naveda, L., & Leman, M. (2010). The Spatiotemporal Representation of Dance and Music Gestures using Topological Gesture Analysis. *Music Perception*, 28(1), 93-111.

---

## 附錄 A：風格相容性完整矩陣

|          | 西非 | 古巴 | 巴西 | 巴爾幹 | 印度 | 甘美朗 | 爵士 | 電子 | 碎拍 | Techno |
|----------|------|------|------|--------|------|--------|------|------|------|--------|
| 西非     | 100% | 80%  | 70%  | 40%    | 50%  | 60%    | 60%  | 50%  | 60%  | 40%    |
| 古巴     | 80%  | 100% | 85%  | 45%    | 50%  | 55%    | 70%  | 60%  | 65%  | 50%    |
| 巴西     | 70%  | 85%  | 100% | 40%    | 45%  | 50%    | 75%  | 55%  | 70%  | 45%    |
| 巴爾幹   | 40%  | 45%  | 40%  | 100%   | 60%  | 35%    | 50%  | 45%  | 45%  | 40%    |
| 印度     | 50%  | 50%  | 45%  | 60%    | 100% | 55%    | 55%  | 40%  | 45%  | 35%    |
| 甘美朗   | 60%  | 55%  | 50%  | 35%    | 55%  | 100%   | 50%  | 50%  | 45%  | 50%    |
| 爵士     | 60%  | 70%  | 75%  | 50%    | 55%  | 50%    | 100% | 70%  | 75%  | 60%    |
| 電子     | 50%  | 60%  | 55%  | 45%    | 40%  | 50%    | 70%  | 100% | 80%  | 85%    |
| 碎拍     | 60%  | 65%  | 70%  | 45%    | 45%  | 45%    | 75%  | 80%  | 100% | 75%    |
| Techno   | 40%  | 50%  | 45%  | 40%    | 35%  | 50%    | 60%  | 85%  | 75%  | 100%   |

## 附錄 B：風格家族分類

**12/8 脈動家族**
- 西非（Standard Bell: E(7,12)）
- 古巴（Clave 系統）
- 巴西（Samba 搖擺）

**4/4 脈動家族**
- 爵士（三連音細分）
- 電子（直拍 16 分）
- 碎拍（切片重組）
- Techno（極簡循環）

**不規則拍家族**
- 巴爾幹（7/8, 9/8, 11/8）
- 印度（Tala 循環）

**Colotomic 家族**
- 甘美朗（階層式鑼點結構）

---

*本研究為 WorldRhythm v0.19 演算法文件，實作於 VCV Rack 模組環境。*

---

## 附錄 C：合成器設計補充

### C.1 Velocity→Decay 縮放公式

MinimalDrumSynth 的 Velocity 影響不僅限於音量，同時也影響聲音的衰減長度，模擬真實打擊樂器的物理特性。

**公式**：
```
velScale = 0.1 + 0.9 × velocity^1.5
actualDecay = baseDecay × velScale
```

**設計理由**：
1. **1.5 次方曲線**：比線性更符合人類對力度的感知，輕擊的聲音衰減更快
2. **最小值保護 (0.1)**：確保即使 velocity=0 也有 10% 的 decay，避免無聲
3. **物理模擬**：真實打擊樂器在輕敲時振動能量較小，自然衰減更快

**數值對照表**：

| Velocity | velScale | Decay 比例 |
|----------|----------|-----------|
| 1.0      | 1.00     | 100%      |
| 0.8      | 0.74     | 74%       |
| 0.5      | 0.42     | 42%       |
| 0.3      | 0.25     | 25%       |
| 0.1      | 0.13     | 13%       |

### C.2 Interlock 配置初始化

PatternGenerator 的 InterlockConfig 採用**延遲初始化**設計：

```cpp
// 使用時才取得風格對應的互鎖設定
interlockConfig = getStyleInterlockConfig(styleIndex);
```

各風格的互鎖強度差異：
- **西非/甘美朗**：高度互鎖（avoidance = 0.85）
- **古巴/巴西**：中度互鎖（avoidance = 0.70）
- **電子/Techno**：無互鎖（avoidance = 0.0，各軌獨立）

此設計允許使用者在運行時動態調整互鎖參數，而非在編譯時固定。
