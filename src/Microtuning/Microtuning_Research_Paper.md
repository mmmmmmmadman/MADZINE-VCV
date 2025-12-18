# 世界微分音系統研究
## Microtuning Systems of the World: A Technical Analysis for Digital Implementation

**作者：MADZINE**
**日期：2024-12**
**版本：1.0**

---

## 摘要 Abstract

本論文研究世界各地傳統音樂中的微分音（microtuning）系統，分析其音程結構、文化背景，以及在數位音樂合成器中實現的技術考量。特別關注哪些系統需要兩個八度的獨立微分音設定，以及上行/下行音階的差異性。

---

## 目錄

1. [導論](#1-導論)
2. [西方調音系統](#2-西方調音系統)
3. [中東音階系統](#3-中東音階系統)
4. [南亞音階系統](#4-南亞音階系統)
5. [東亞音階系統](#5-東亞音階系統)
6. [東南亞音階系統](#6-東南亞音階系統)
7. [技術實現分析](#7-技術實現分析)
8. [結論與建議](#8-結論與建議)
9. [參考資料](#9-參考資料)

---

## 1. 導論

### 1.1 什麼是微分音？

微分音（Microtone）指的是小於西方十二平均律（12-TET）半音（100 音分/cents）的音程。在世界各地的傳統音樂中，許多文化發展出獨特的音階系統，其音程並不符合西方標準。

### 1.2 音分（Cents）系統

音分是測量音程的對數單位：
- 1 八度 = 1200 cents
- 1 半音（12-TET）= 100 cents
- 1 音分 = 1/100 半音

計算公式：
```
cents = 1200 × log₂(f₂/f₁)
```

### 1.3 研究目的

本研究旨在：
1. 系統性整理世界主要微分音系統
2. 分析各系統的音程結構
3. 確定哪些系統需要兩個八度的獨立設定
4. 為 VCV Rack Quantizer 模組提供實現依據

---

## 2. 西方調音系統

### 2.1 十二平均律 (12-TET / Equal Temperament)

現代西方標準調音，將八度等分為 12 個半音。

| 音符 | Cents | 頻率比 |
|------|-------|--------|
| C | 0 | 1.0000 |
| C# | 100 | 1.0595 |
| D | 200 | 1.1225 |
| D# | 300 | 1.1892 |
| E | 400 | 1.2599 |
| F | 500 | 1.3348 |
| F# | 600 | 1.4142 |
| G | 700 | 1.4983 |
| G# | 800 | 1.5874 |
| A | 900 | 1.6818 |
| A# | 1000 | 1.7818 |
| B | 1100 | 1.8877 |

**特點**：所有調性等效，便於轉調，但犧牲了純律的和諧性。

**需要兩個八度**：❌ 否

### 2.2 純律 (Just Intonation)

基於簡單整數比的調音系統，產生最和諧的音程。

| 音符 | 頻率比 | Cents | 與 12-TET 差異 |
|------|--------|-------|----------------|
| C | 1/1 | 0 | 0 |
| C# | 16/15 | 111.7 | +11.7 |
| D | 9/8 | 203.9 | +3.9 |
| D# | 6/5 | 315.6 | +15.6 |
| E | 5/4 | 386.3 | -13.7 |
| F | 4/3 | 498.0 | -2.0 |
| F# | 45/32 | 590.2 | -9.8 |
| G | 3/2 | 702.0 | +2.0 |
| G# | 8/5 | 813.7 | +13.7 |
| A | 5/3 | 884.4 | -15.6 |
| A# | 9/5 | 1017.6 | +17.6 |
| B | 15/8 | 1088.3 | -11.7 |

**特點**：和聲純淨，但不同調性間差異大。

**需要兩個八度**：❌ 否

### 2.3 畢達哥拉斯調音 (Pythagorean Tuning)

僅使用純五度（3:2）堆疊產生的調音。

| 音符 | 頻率比 | Cents | 與 12-TET 差異 |
|------|--------|-------|----------------|
| C | 1/1 | 0 | 0 |
| D | 9/8 | 203.9 | +3.9 |
| E | 81/64 | 407.8 | +7.8 |
| F | 4/3 | 498.0 | -2.0 |
| G | 3/2 | 702.0 | +2.0 |
| A | 27/16 | 905.9 | +5.9 |
| B | 243/128 | 1109.8 | +9.8 |

**特點**：五度完美，但三度偏高（畢氏音差 Pythagorean comma = 23.46 cents）。

**需要兩個八度**：❌ 否

### 2.4 四分音 (Quarter Tone / 24-TET)

將八度等分為 24 個四分音。

**特點**：常用於 20 世紀現代音樂，也是阿拉伯音樂的近似系統。

**需要兩個八度**：❌ 否

---

## 3. 中東音階系統

### 3.1 阿拉伯 Maqam 系統

#### 3.1.1 概述

Maqam（複數 Maqamat）是阿拉伯音樂的調式系統，使用 24-TET 作為近似。傳統上，24 個音符跨越兩個八度，從 Yakah（低 G）到 Ramal Tuti（高 G）。

#### 3.1.2 主要 Maqam 及其音程

**Maqam Rast（拉斯特）**
```
音符：C  D  E½♭  F  G  A  B½♭  C
音程：0  200  350  500  700  900  1050  1200
```
E½♭ 和 B½♭ 表示降低約 50 cents 的音符。

**Maqam Bayati（巴亞提）**
```
音符：D  E½♭  F  G  A  B♭  C  D
音程：0  150  300  500  700  850  1000  1200
```

**Maqam Hijaz（希賈茲）**
```
音符：D  E♭  F#  G  A  B♭  C  D
音程：0  100  400  500  700  800  1000  1200
```

#### 3.1.3 兩個八度的差異

在某些 Maqam 中，**上八度的音程可能與下八度不同**：

**Maqam Sikah 示例**：
- 下八度：E½♭ - F - G - A - B½♭ - C - D
- 上八度：可能使用不同的四分音位置

**需要兩個八度**：✅ 是（部分 Maqam）

### 3.2 土耳其 Makam 系統

#### 3.2.1 概述

土耳其 Makam 系統使用 53-TET（53 等分律），將八度分為 53 個 comma（逗號）。

- 大二度 = 9 commas（約 204 cents）
- 小二度 = 4 commas（約 90 cents）
- 1 comma ≈ 22.6 cents

#### 3.2.2 浮動音符概念

土耳其音樂的關鍵特徵是「浮動音符」（floating notes）：

- **上行時**：某些音符會稍微升高（約 1 comma）
- **下行時**：相同音符會稍微降低

**Makam Rast 示例**：
```
上行：G - A - B(+1) - C - D - E - F#(Irak) - G
下行：G - F(Acem) - E - D - C - B(-1) - A - G
```

注意：上行使用 F#（Irak），下行使用 F（Acem）。

#### 3.2.3 Seyir（旋律方向）

Makam 有三種旋律方向：
1. **Çıkıcı（上行型）**：從主音開始向上
2. **İnici（下行型）**：從高音開始向下
3. **İnici-Çıkıcı（混合型）**：結合兩者

**需要兩個八度**：✅ 是（上行/下行差異）

### 3.3 波斯 Dastgah 系統

#### 3.3.1 概述

波斯音樂使用 Dastgah（調式系統），包含 7 個主要 Dastgah 和 5 個輔助 Avaz。

#### 3.3.2 音程特徵

波斯音樂的特徵音程包括：
- **Koron（↓）**：降低約 50-60 cents
- **Sori（↑）**：升高約 50-60 cents

**Dastgah Shur 示例**：
```
G - A(koron) - B♭ - C - D - E♭ - F - G
音程：0 - ~150 - 300 - 500 - 700 - 800 - 1000 - 1200
```

#### 3.3.3 上行/下行差異

類似土耳其系統，波斯音樂也有上行/下行的微妙差異。

**需要兩個八度**：✅ 是

---

## 4. 南亞音階系統

### 4.1 印度 Raga 系統

#### 4.1.1 概述

印度古典音樂使用 Raga（旋律模式）和 22 Shruti（微分音）系統。

#### 4.1.2 七個基本音符（Swaras）

| Swara | 名稱 | 近似西方音符 |
|-------|------|--------------|
| Sa | Shadja | C（固定）|
| Re | Rishabh | D |
| Ga | Gandhar | E |
| Ma | Madhyam | F |
| Pa | Pancham | G（固定）|
| Dha | Dhaivat | A |
| Ni | Nishad | B |

#### 4.1.3 Shruti 系統

傳統上八度分為 22 個 Shruti（不等分）：

| Shruti | Cents（約）| 對應 |
|--------|-----------|------|
| 1 | 0 | Sa |
| 2 | 90 | Komal Re |
| 3 | 112 | |
| 4 | 182 | Shuddha Re |
| 5 | 204 | |
| 6 | 294 | Komal Ga |
| 7 | 316 | |
| 8 | 386 | Shuddha Ga |
| 9 | 408 | |
| 10 | 498 | Shuddha Ma |
| 11 | 520 | |
| 12 | 590 | Tivra Ma |
| 13 | 612 | |
| 14 | 702 | Pa |
| 15 | 792 | Komal Dha |
| 16 | 814 | |
| 17 | 884 | Shuddha Dha |
| 18 | 906 | |
| 19 | 996 | Komal Ni |
| 20 | 1018 | |
| 21 | 1088 | Shuddha Ni |
| 22 | 1110 | |

#### 4.1.4 Aroha 與 Avaroha

印度 Raga 最重要的特徵是 **Aroha（上行）和 Avaroha（下行）可以使用完全不同的音符**。

**Raga Yaman 示例**：
```
Aroha（上行）：N S G M# D N S'
Avaroha（下行）：S' N D P M# G R S
```
注意：上行省略 Re(R) 和 Pa(P)，下行使用所有音符。

**Raga Bhairav 示例**：
```
Aroha：S r G M P d N S'
Avaroha：S' N d P M G r S
```
使用 Komal Re(r) 和 Komal Dha(d)。

#### 4.1.5 三個八度

印度音樂跨越三個八度：
- **Mandra Saptak**（低八度）
- **Madhya Saptak**（中八度）
- **Tar Saptak**（高八度）

**需要兩個八度**：✅ 是（Aroha/Avaroha 差異 + 不同八度可能有微妙差異）

---

## 5. 東亞音階系統

### 5.1 日本音階系統

#### 5.1.1 上原六四郎的理論（1895）

明治時代，上原六四郎在《俗樂旋律考》中提出：
1. 俗樂的音階是五音音階
2. 分為含半音的**陰旋**和不含半音的**陽旋**
3. **兩者都有上行形和下行形**

#### 5.1.2 主要音階類型與上行/下行差異

**都節音階（Miyako-bushi）- 陰旋法**

具有**下行性格**，上行時音符會改變：

```
下行形（基本）：E - F - A - B - C - E
上行形（異向形）：E - F - A - B - D - E
                              ↑
                     短6度(C)→短7度(D)
```

**律音階（Ritsu）**

同樣具有下行性格：
```
下行形：6度音使用 A♭
上行形：6度音上升為 B♭
```

**陽音階（Yo Scale）**
```
音符：D - E - G - A - B
音程：0 - 200 - 500 - 700 - 900
```
用於雅樂（Gagaku）。具有**上行性格**，較為穩定。

**民謠音階（Min'yo）**

具有**上行性格**，中間音浮動較少，上行/下行差異不大。

#### 5.1.3 小泉文夫的理論

小泉文夫將日本音階分為：
- **下行性音階**：都節、律（上行時音會變化）
- **上行性音階**：民謠、琉球（較穩定）

核心概念是**テトラコルド（四度音列）**，完全四度內的中間音會根據旋律方向浮動。

#### 5.1.4 微分音特徵

箏（Koto）調音傾向使用純律：
- 大三度：386 cents（純律）vs 400 cents（12-TET）
- 差異約 14 cents

**需要兩個八度**：✅ 是（都節/律音階有上行/下行差異）

### 5.2 琉球音階（Ryukyu Scale）

#### 5.2.1 結構

```
五聲版：C - E - F - G - B
音程：0 - 400 - 500 - 700 - 1100
結構：大三度 - 小二度 - 大二度 - 大三度 - 小二度

六聲版：C - D - E - F - G - B
音程：0 - 200 - 400 - 500 - 700 - 1100
```

#### 5.2.2 上行/下行差異

根據研究，琉球音階在下行時會加入**經過音「レ」（D）**：

```
上行：C - E - F - G - B - C（五聲）
下行：C - B - G - F - E - D - C（加入 D 作為經過音）
```

「てぃんさぐぬ花」等樂曲中，「レ」多用於**下行時使旋律更滑順**。

#### 5.2.3 特徵

- 特徵音程：B→C（小二度）是識別沖繩音樂的關鍵
- 與陽音階相關，但音高升高
- 八重山民謠有微妙的音高變化
- 屬於小泉理論中的「上行性音階」

**需要兩個八度**：✅ 是（下行時加入經過音）

### 5.3 中國音階系統

#### 5.3.1 五聲音階（宮商角徵羽）

```
宮：C - D - E - G - A
音程：0 - 200 - 400 - 700 - 900
```

#### 5.3.2 微分音

中國傳統音樂主要使用純律，特別是古琴音樂中有複雜的泛音系統。

**需要兩個八度**：❌ 否

---

## 6. 東南亞音階系統

### 6.1 印尼甘美朗（Gamelan）

#### 6.1.1 Slendro 音階

五聲音階，音程接近等分（約 240 cents 每級）：

```
近似音程：0 - 240 - 480 - 720 - 960
```

但實際上每組甘美朗樂器都有獨特的調音，沒有標準化。

#### 6.1.2 Pelog 音階

七聲音階，音程不均：

```
一種常見形式：0 - 120 - 270 - 540 - 670 - 800 - 1050
```

特徵是包含非常窄和非常寬的音程混合。

#### 6.1.3 微分音特徵

- 每組甘美朗有獨特調音
- Ombak（顫音）：成對樂器故意調成微小差異產生顫動效果
- 與西方 12-TET 差異可達 ±50 cents

**需要兩個八度**：❌ 否（但高度個別化）

### 6.2 泰國音階

#### 6.2.1 七平均律

泰國傳統音樂使用 7-TET（七等分律）：

```
音程：0 - 171 - 343 - 514 - 686 - 857 - 1029
每級約 171 cents
```

**需要兩個八度**：❌ 否

---

## 7. 技術實現分析

### 7.1 需要兩個八度的系統總結

| 系統 | 需要兩個八度 | 原因 |
|------|-------------|------|
| Arabic Maqam | ✅ | 上下八度音程可能不同 |
| Turkish Makam | ✅ | 浮動音符（上行/下行差異）|
| Persian Dastgah | ✅ | 類似 Turkish |
| Indian Raga | ✅ | Aroha/Avaroha 完全不同 |
| **Japanese 都節/律** | ✅ | 上行時短6度→短7度 |
| **Ryukyu** | ✅ | 下行時加入經過音 |
| Japanese 陽/民謠 | ❌ | 上行性，較穩定 |
| 12-TET | ❌ | 八度等價 |
| Just Intonation | ❌ | 八度等價 |
| Pythagorean | ❌ | 八度等價 |
| Chinese | ❌ | 八度等價 |
| Gamelan | ❌ | 個別化但無上下行差異 |
| Thai 7-TET | ❌ | 八度等價 |

### 7.2 實現方案比較

#### 方案 A：24 個微分音參數（兩個八度）

```cpp
enum MicrotuneParam {
    // 第一個八度
    C1_MICROTUNE, CS1_MICROTUNE, D1_MICROTUNE, ..., B1_MICROTUNE,
    // 第二個八度
    C2_MICROTUNE, CS2_MICROTUNE, D2_MICROTUNE, ..., B2_MICROTUNE,
};
```

**優點**：
- 完全支援所有系統
- 上下八度獨立控制

**缺點**：
- 需要 24 個旋鈕（UI 空間問題）
- 對於不需要兩個八度的系統是多餘的

#### 方案 B：12 + 12（上行/下行分開）

```cpp
struct MicrotunePreset {
    float ascending[12];   // 上行微分音
    float descending[12];  // 下行微分音
};
```

**優點**：
- 精確對應中東/印度系統的需求
- 可以根據音高移動方向自動切換

**缺點**：
- 需要追蹤音高移動方向
- 實現較複雜

#### 方案 C：混合方案（推薦）

```cpp
struct MicrotuneSystem {
    float base[12];        // 基本微分音（一個八度）
    float octave2[12];     // 第二個八度（可選）
    bool useTwoOctaves;    // 是否啟用兩個八度
    bool useAscDesc;       // 是否區分上行/下行
    float ascending[12];   // 上行修正（可選）
    float descending[12];  // 下行修正（可選）
};
```

**優點**：
- 靈活支援所有系統
- 向後兼容現有的 12 音符系統
- 可根據預設自動配置

### 7.3 預設值建議

#### 中東系統預設（需要兩個八度/上下行）

**Turkish Makam Rast**：
```cpp
// 上行
float rast_asc[12] = {0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 8};
// 下行（F# → F）
float rast_desc[12] = {0, 0, 0, 0, 8, 0, -100, 0, 0, 0, 0, 8};
```

**Arabic Maqam Bayati**：
```cpp
float bayati[12] = {0, 0, -50, 0, 0, 0, 0, 0, 0, -50, 0, 0};
```

#### 印度系統預設

**22 Shruti 近似**：
```cpp
float shruti[12] = {0, -10, 14, -16, 14, 2, -10, -2, -14, 16, -18, 12};
```

### 7.4 音高方向追蹤算法

```cpp
class PitchDirectionTracker {
    float lastPitch = 0.f;
    bool ascending = true;

    void update(float currentPitch) {
        if (currentPitch > lastPitch + threshold) {
            ascending = true;
        } else if (currentPitch < lastPitch - threshold) {
            ascending = false;
        }
        lastPitch = currentPitch;
    }

    float getMicrotune(int note, const MicrotuneSystem& sys) {
        if (sys.useAscDesc) {
            return ascending ? sys.ascending[note] : sys.descending[note];
        }
        return sys.base[note];
    }
};
```

---

## 8. 結論與建議

### 8.1 主要發現

1. **中東和南亞系統**需要兩個八度或上行/下行區分
2. **東亞系統**（日本、中國、琉球）不需要兩個八度
3. **實現複雜度**：混合方案可以同時滿足簡單和複雜需求

### 8.2 建議實現步驟

1. **Phase 1**：保持現有 12 個微分音參數
2. **Phase 2**：新增「Two-Octave Mode」開關和第二組 12 個參數
3. **Phase 3**：新增「Ascending/Descending」模式和音高方向追蹤
4. **Phase 4**：實現完整的預設庫

### 8.3 預設庫建議

**優先實現**：
1. Equal Temperament（12-TET）
2. Just Intonation
3. Pythagorean
4. Arabic Maqam Rast
5. Arabic Maqam Bayati
6. Turkish Makam Rast（含上下行）
7. Indian Shruti（22 音近似）
8. Gamelan Slendro
9. Gamelan Pelog
10. Japanese In/Yo
11. Ryukyu
12. Quarter Tone

---

## 9. 參考資料

### 學術資源
- Touma, H. H. (1996). *The Music of the Arabs*
- Powers, H. S. (1980). "Mode" in *New Grove Dictionary of Music*
- Zonis, E. (1973). *Classical Persian Music*

### 線上資源
- [Wikipedia - Arabic maqam](https://en.wikipedia.org/wiki/Arabic_maqam)
- [Wikipedia - Turkish makam](https://en.wikipedia.org/wiki/Turkish_makam)
- [Wikipedia - Shruti](https://en.wikipedia.org/wiki/Shruti_(music))
- [Maqam World](https://www.maqamworld.com/)
- [TAQS.IM Scale Guide](https://taqs.im/scales/)

### 技術參考
- Scala Scale Archive (http://www.huygens-fokker.org/scala/)
- Xenharmonic Wiki (https://en.xen.wiki/)

---

## 附錄 A：完整微分音數據表

### A.1 Just Intonation（純律）

| 音符 | 比率 | Cents | 與 12-TET 差 |
|------|------|-------|-------------|
| C | 1/1 | 0.00 | 0.00 |
| C# | 16/15 | 111.73 | +11.73 |
| D | 9/8 | 203.91 | +3.91 |
| D# | 6/5 | 315.64 | +15.64 |
| E | 5/4 | 386.31 | -13.69 |
| F | 4/3 | 498.04 | -1.96 |
| F# | 45/32 | 590.22 | -9.78 |
| G | 3/2 | 701.96 | +1.96 |
| G# | 8/5 | 813.69 | +13.69 |
| A | 5/3 | 884.36 | -15.64 |
| A# | 9/5 | 1017.60 | +17.60 |
| B | 15/8 | 1088.27 | -11.73 |

### A.2 Pythagorean（畢達哥拉斯）

| 音符 | 比率 | Cents | 與 12-TET 差 |
|------|------|-------|-------------|
| C | 1/1 | 0.00 | 0.00 |
| C# | 2187/2048 | 113.69 | +13.69 |
| D | 9/8 | 203.91 | +3.91 |
| D# | 32/27 | 294.13 | -5.87 |
| E | 81/64 | 407.82 | +7.82 |
| F | 4/3 | 498.04 | -1.96 |
| F# | 729/512 | 611.73 | +11.73 |
| G | 3/2 | 701.96 | +1.96 |
| G# | 6561/4096 | 815.64 | +15.64 |
| A | 27/16 | 905.87 | +5.87 |
| A# | 16/9 | 996.09 | -3.91 |
| B | 243/128 | 1109.78 | +9.78 |

### A.3 22 Shruti（印度）

| Shruti # | 名稱 | Cents |
|----------|------|-------|
| 1 | Chandovati | 0 |
| 2 | Dayavati | 90 |
| 3 | Ranjani | 112 |
| 4 | Ratika | 182 |
| 5 | Raudri | 204 |
| 6 | Krodha | 294 |
| 7 | Vajrika | 316 |
| 8 | Prasarini | 386 |
| 9 | Priti | 408 |
| 10 | Marjani | 498 |
| 11 | Kshiti | 520 |
| 12 | Rakta | 590 |
| 13 | Sandipani | 612 |
| 14 | Alapini | 702 |
| 15 | Madanti | 792 |
| 16 | Rohini | 814 |
| 17 | Ramya | 884 |
| 18 | Ugra | 906 |
| 19 | Kshobhini | 996 |
| 20 | Tivra | 1018 |
| 21 | Kumudvati | 1088 |
| 22 | Manda | 1110 |

---

## 附錄 B：預設實現代碼參考

```cpp
// Microtuning preset data structure
struct MicrotunePreset {
    const char* name;
    float cents[12];
    bool needsTwoOctaves;
    float cents_octave2[12];  // 第二個八度（如需要）
};

// 預設定義
const MicrotunePreset PRESETS[] = {
    {"Equal Temperament", {0,0,0,0,0,0,0,0,0,0,0,0}, false, {}},
    {"Just Intonation", {0,11.7,3.9,15.6,-13.7,-2.0,-9.8,2.0,13.7,-15.6,17.6,-11.7}, false, {}},
    {"Pythagorean", {0,13.7,3.9,-5.9,7.8,-2.0,11.7,2.0,15.6,5.9,-3.9,9.8}, false, {}},
    {"Arabic Bayati", {0,0,-50,0,0,0,0,0,0,-50,0,0}, false, {}},
    {"Turkish Rast", {0,0,0,0,8,0,0,0,0,0,0,8}, false, {}},
    // ... 更多預設
};
```

---

**文件結束**

*本研究為 MADZINE VCV Rack Quantizer 模組開發的基礎資料。*
