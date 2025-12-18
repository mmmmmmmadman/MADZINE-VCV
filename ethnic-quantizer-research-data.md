# 民族風格 Quantizer 模組 - 精確數據研究報告

此文檔整理了各民族音樂系統的精確調音數據，用於 VCV Rack Quantizer 模組的實現。

## 目錄
1. [Arabic Maqam 系統](#arabic-maqam-system)
2. [Turkish Makam 系統](#turkish-makam-system)
3. [Indian Raga 系統](#indian-raga-system)
4. [Japanese 音階系統](#japanese-scales)
5. [Gamelan 系統](#gamelan-system)

---

## Arabic Maqam System

### 基本理論框架

**24-TET 系統 (理論基礎):**
- 八度分為 24 個等分
- 四分音 (Quarter Tone) = 50 cents
- 半音 = 100 cents
- 四分之三音 (Neutral Second) = 150 cents

**重要注意事項:**
24-TET 僅為記譜慣例，實際演奏中的音高會根據旋律流動和前後音符關係而有微妙變化。傳統演奏者透過口傳心授掌握更精細的微分音細節。

### 主要 Maqam 的 Cents 值

#### 1. Maqam Rast (راست)
**情緒:** 自豪、高尚

**音階結構 (以 C 為根音):**
```
Lower Tetrachord (Rast on C): C - D - E½♭ - F
Upper Tetrachord (Rast on G): G - A - B½♭ - C
```

**Cents 值 (相對於根音):**
- C (Sa) = 0 cents
- D (Re) = ~200-204 cents (大全音)
- E½♭ (Ga) = ~350-360 cents (中性三度)
- F (Ma) = 498 cents (純四度)
- G (Pa) = 702 cents (純五度)
- A (Dha) = ~900-906 cents
- B½♭ (Ni) = ~1050-1060 cents (中性七度)
- C (Sa') = 1200 cents

**Turkish Rast 變體 (53-TET):**
- 第三音 = 377.5 cents (17 commas)
- E½♭ 和 B½♭ 在 Turkish 版本中 B½♭ 相對較高

**特徵:**
- 兩個 Rast 四音組
- 上行下行無顯著差異
- 使用 "half-flat" (半降) 記號

---

#### 2. Maqam Bayati (بياتي)
**情緒:** 活力、喜悅、女性特質

**音階結構 (以 D 為根音):**
```
D - E½♭ - F - G - A - B♭ - C - D
```

**Cents 值:**
- D = 0 cents
- E½♭ = ~140-150 cents (小於四分之三音)
- F = 350 cents
- G = 700 cents (純五度，但傳統演奏可能略高於畢達哥拉斯調音)
- A = 900 cents
- B♭ = 1000 cents
- C = 1150 cents
- D' = 1200 cents

**特徵:**
- 五度音經常高於畢達哥拉斯調音
- 第二音 (E½♭) 是識別特徵

---

#### 3. Maqam Hijaz (حجاز)
**情緒:** 遙遠的沙漠、神秘

**音階結構:**
```
Jins Hijaz: 1 - ♭2 - 3 - 4
典型: D - E♭ - F# - G
```

**Cents 值:**
- D = 0 cents
- E♭ = ~100 cents (小半音)
- F# = ~400-408 cents (增二度，約 300-308 cents 跳躍)
- G = 498 cents

**Turkish Hicaz (53-TET):**
- [4, 13, 5] commas = [~91, 295, 113] cents
- 或 [5, 12, 5] commas = [~113, 272, 113] cents

**特徵:**
- 特徵性的增二度音程 (約 1.5 全音)
- 某些音在下行時降低約 25 cents
- 第三音下行時可能降低至 -25 cents

---

#### 4. Maqam Saba (صبا)
**情緒:** 悲傷、痛苦、敏感

**音階結構 (前四音):**
```
D - E¼♭ - F - G♭
```

**Cents 值 (精確測量):**
- D = 0 cents
- E¼♭ = ~140 cents (小於四分之三音的中性二度)
- F = ~300 cents (160 cents 跨度)
- G♭ = ~395 cents (95 cents 小半音)

**特徵:**
- 兩個不等的中性二度: 160 cents 和 140 cents
- 小半音: 95 cents
- E¼♭ 和 G♭ 可能根據情感需要而略有變化

---

#### 5. Maqam Nahawand (نهاوند)
**情緒:** 憂鬱 (類似西方小調)

**音階結構:**
類似西方自然小調和和聲小調的混合

**形式 1 - 自然小調型:**
```
D - E - F - G - A - B♭ - C - D
```
**Cents:** 0, 200, 300, 500, 700, 1000, 1100, 1200

**形式 2 - 和聲小調型:**
```
D - E - F - G - A - B♭ - C# - D
```
**Cents:** 0, 200, 300, 500, 700, 1000, 1100, 1200

**上行/下行差異:**
- 上行使用較低的 A♭
- 下行使用較高的 A♭
- Jins Kurd (下行) 使用低小二度
- Jins Hijaz (上行) 使用高小二度

**特徵:**
- 可用 12-TET 鋼琴演奏 (不包含四分音)
- 第七音有兩種形式

---

#### 6. Maqam Kurd (كرد)
**情緒:** (類似 Phrygian)

**音階結構:**
等同於西方 Phrygian 音階

```
D - E♭ - F - G - A - B♭ - C - D
```

**Cents:** 0, 100, 300, 500, 700, 900, 1100, 1200

**特徵:**
- Jins Kurd 傾向使用低小二度
- 可用 12-TET 演奏
- 與 Meantone Phrygian 等同

---

#### 7. Maqam Ajam (عجم)
**情緒:** (類似西方大調)

**音階結構:**
等同於西方大調音階

```
B♭ - C - D - E♭ - F - G - A - B♭
或 C - D - E - F - G - A - B - C
```

**Cents:** 0, 200, 400, 500, 700, 900, 1100, 1200

**Turkish 53-TET 版本細節:**
- 純五度 = 31 commas = 701.9 cents
- 大三度 (較大) = 18 commas = 407.5 cents
- 大三度 (較小) = 17 commas = 384.9 cents
- B♭ to D 和 F to A: 9+8 commas
- E♭ to G: 9+9 commas (寬一個 comma)

**特徵:**
- 可用 12-TET 演奏
- Ajam = "波斯" (阿拉伯語)
- 通常從 B♭ 開始，但也可從 C 或 F

---

### 常用 Half-Flat 音符

最常用的 half-flat 音符:
- **E½♭** (E half-flat): ~350 cents
- **B½♭** (B half-flat): ~1050 cents
- **A½♭** (A half-flat): ~850 cents (較少使用)

---

### Jins (四音組) 基礎音程

Arabic Maqam 由更小的音階片段 "Ajnas" (單數: Jins) 構成:

**9 個主要 Jins 家族:**
1. **Jins Rast:** 大全音 - 中性三度跳躍 - 小全音
2. **Jins Bayati:** 四分之三音 - 小全音 - 大全音
3. **Jins Saba:** 中性 - 中性 - 小半音
4. **Jins Hijaz:** 小半音 - 增二度 - 小半音
5. **Jins Nahawand:** 大全音 - 小半音 - 大全音
6. **Jins Kurd:** 小半音 - 大全音 - 大全音
7. **Jins Ajam:** 大全音 - 大全音 - 小半音

---

## Turkish Makam System

### 基本理論框架: 53-TET Comma 系統

**基礎定義:**
- 八度分為 **53 個等分 Commas** (Holdrian Comma)
- 1 Comma = **1200/53 = 22.6415 cents**
- 大全音 = 9 commas = 203.77 cents
- 小半音 = 4 commas = 90.57 cents
- 小半音 (另一型) = 5 commas = 113.21 cents
- 純五度 = 31 commas = 701.89 cents

**實際使用:**
理論上有 53 個音，但實際只使用其中 24 個。

**歷史:**
20 世紀初期，53-TET 成為 Ottoman 古典音樂最常見的調音形式，取代了更早的不等律調音。

---

### Comma 與 Cents 換算表

| Commas | Cents | 音程類型 |
|--------|-------|----------|
| 1 | 22.64 | Comma |
| 4 | 90.57 | 小半音 (Küçük Mücenneb) |
| 5 | 113.21 | 小半音 (浮動音) |
| 8 | 181.13 | 大半音 |
| 9 | 203.77 | 大全音 (Tanini) |
| 12 | 271.70 | 中性三度 |
| 13 | 294.34 | 中性三度 (大) |
| 17 | 384.91 | 大三度 (小) |
| 18 | 407.55 | 大三度 (大) |
| 22 | 498.11 | 純四度 |
| 31 | 701.89 | 純五度 |

---

### 六大基礎四音組 (Tetrachords)

Turkish Makam 的構成單元是四音組，跨越純四度 (22 commas):

#### 1. Çargah (چارگاه)
**Comma 結構:** [9, 9, 4]
**Cents:** [204, 204, 91] = 大全音 - 大全音 - 小半音
**等同於:** 西方大調四音組
**情緒:** 明亮

---

#### 2. Rast (راست)
**Comma 結構:** [9, 8, 5]
**Cents:** [204, 181, 113]
**特徵:** 不等的全音
**Turkish Rast 第三音:** 377.5 cents

**Makam Rast:**
- 根音: G (Rast)
- 屬音: D (Neva)
- 導音: F# (Irak)
- 下行時 F# 降低 4 commas 成為 F (Acem)

---

#### 3. Hicaz (حجاز)
**Comma 結構:** [4, 13, 5] 或 [5, 12, 5]
**Cents:** [91, 295, 113] 或 [113, 272, 113]
**特徵:** 特徵性增二度
**等同於:** Arabic Hijaz

**Makam Hicaz 特色:**
- 12-comma 的增二度非常接近 Just Intonation 的 septimal minor third (7/6)

---

#### 4. Uşşak (عشّاق)
**Comma 結構:** [9, 5, 8] 或 [8, 5, 9]
**Cents:** [204, 113, 181] 或 [181, 113, 204]
**特徵:** 微分音降低的第二音
**情緒:** 內省、溫暖 (類似小調但更溫暖)

**Makam Uşşak:**
- 根音: A (Dügâh)
- 屬音 (實為下屬音): D (Neva)
- 導音 (實為下中音): G (Rast)

**記譜注意:**
- Segah 變音記號通常代表降低 1 comma
- 但在 Uşşak Makam 中，要降低 2-3 commas

---

#### 5. Bûselik (بوسليك)
**Comma 結構:** [5, 9, 8]
**Cents:** [113, 204, 181]
**等同於:** 西方小調四音組 (但有微分音差異)

**從 D 開始的 Bûselik:**
基本上是西方小調四音組模式

---

#### 6. Segâh (سيگاه)
**Comma 結構:** [5, 4, 13]
**Cents:** [113, 91, 295]
**特徵:** 獨特的微分音降低
**情緒:** 極度平和 (尤其解決回微分音降 E 時)

**Turkish vs Arabic:**
- Turkish: 第四音也微分音降低
- Arabic (Sika): 延伸方式不同

---

### Pentachords (五音組)

五音組跨越純五度，共 31 commas:

**Rast Pentachord:**
- 包含 Rast Tetrachord 音程
- 加上額外的 9 commas 大全音

---

### 主要 Makam 示例

#### Makam Rast (完整音階)
```
G - A - B♭-4commas - C - D - E - F#-4commas(下行時為F自然) - G
```
**Commas:** 0, 9, 17, 22, 31, 40, 49 (上行), 45 (下行), 53

**結構:**
- 下四音組: Rast on G (G-A-B-C, commas: 9-8-5)
- 上四音組: Rast on D (上行) / Bûselik on D (下行)

---

#### Makam Hüseyni (حسيني)
**變體 of Rast Tetrachord:** [8, 5, 9]
**情緒:** 類似西方小調，但第二和第六音有微分音
**類似於:** Western Minor (但有微分音在第二和第六音)

---

### 實作注意事項

**浮動音符 (Floating Notes):**
半音在 Turkish Maqam 中:
- 4 commas (上行方向)
- 5 commas (下行方向)
這取決於 "浮動" 或 "不定" 音符的概念

**調音實踐:**
- 具體降低多少 (例如 "少於半音") 取決於傳統、品味、地域和藝術表達
- 聆聽和實驗是最好的指南

---

## Indian Raga System

### 基本理論框架: 22 Shruti 系統

**歷史源頭:**
- Bharata 的 Natya Shastra (約公元前 200 - 公元 200)
- 八度分為 **22 個微分音程 (Shrutis)**
- 提供比西方 12-TET 更精細的音高控制

**重要爭議:**
學術界對 22 Shruti 存在三種觀點:
1. **均等四分音說:** 每個 Shruti 約 53 cents (1200/22 ≈ 54.5 cents)
2. **不均等間距說:** 基於 Just Intonation 比例
3. **可變系統說:** 沒有固定的 22 個音，而是依上下文變化

**現代實踐:**
當代 Carnatic 和 Hindustani 音樂演奏中，並不明確使用 22 個固定音高。中間音 (intermediate tones) 的數量可能少於或多於 22。

---

### 三種基本 Shruti 音程

根據 Natya Shastra 和後續研究，22 Shrutis 由三種基本音程組成:

#### 1. Pramana Shruti (標準)
- **比例:** 81/80 (Syntonic Comma)
- **Cents:** ~22 cents
- **位置:** 共 10 個
- **分布在:** Shrutis 1-2, 3-4, 5-6, 7-8, 9-10, 11-12, 14-15, 16-17, 18-19, 20-21

#### 2. Nyuna Shruti (小)
- **比例:** 25/24 (Just Chromatic Semitone)
- **Cents:** ~70 cents
- **位置:** 共 5 個
- **分布在:** Shrutis 2-3, 6-7, 10-11, 15-16, 19-20

#### 3. Purna Shruti (大)
- **比例:** 256/243 (Pythagorean Limma)
- **Cents:** ~90 cents
- **位置:** 共 7 個
- **分布在:** Shrutis 0-1, 4-5, 8-9, 12-13, 13-14, 17-18, 21-22

**驗證:**
(81/80)^10 × (256/243)^7 × (25/24)^5 = 2 (八度)

---

### 22 Shruti Just Intonation 比例表

**Shadja Grama Scale (基礎音階):**

| Shruti # | Note Name | Ratio | Cents | 音名 (Sargam) |
|----------|-----------|-------|-------|---------------|
| 0 | Shadja (Sa) | 1/1 | 0 | S |
| 1 | | 256/243 | 90 | |
| 2 | | | 112 | |
| 3 | Komal Rishabh | 16/15 | 182 | r |
| 4 | | | 204 | |
| 5 | Shuddha Rishabh | 9/8 | 204 | R |
| 6 | | | 274 | |
| 7 | | | 294 | |
| 8 | Komal Gandhar | 32/27 | 294 | g |
| 9 | Shuddha Gandhar | 81/64 | 408 | G |
| 10 | | | 430 | |
| 11 | | 5/4 | 386 | |
| 12 | | | 498 | |
| 13 | Shuddha Madhyam | 4/3 | 498 | M |
| 14 | | | 520 | |
| 15 | Tivra Madhyam | 45/32 or 729/512 | 590/612 | M# |
| 16 | Pancham | 3/2 | 702 | P |
| 17 | | | 792 | |
| 18 | Komal Dhaivat | 128/81 | 792 | d |
| 19 | Shuddha Dhaivat | 5/3 or 27/16 | 884/906 | D |
| 20 | | | 928 | |
| 21 | Komal Nishad | 16/9 | 996 | n |
| 22 | Shuddha Nishad | 15/8 or 243/128 | 1088/1110 | N |

**主要 Swara 位置 (7 音):**
- Shadja (S): Shruti 0 (1/1) = 0 cents
- Rishabh (R): Shruti 5 (9/8) = 204 cents
- Gandhar (g): Shruti 5 (未明確)
- Madhyam (M): Shruti 9 (4/3) = 498 cents
- Pancham (P): Shruti 13 (3/2) = 702 cents
- Dhaivat (D): Shruti 16 (未明確)
- Nishad (n): Shruti 18 (未明確)
- Shadja (S'): Shruti 22 (2/1) = 1200 cents

**Swara 之間的 Shruti 間隔:**
3, 2, 4, 4, 3, 2, 4 Shrutis

---

### Chatur-Shruti Rishabha (四 Shruti Rishabh)

**重要音程計算:**
Purna + Pramana + Nyuna + Pramana = 256/243 × 81/80 × 25/24 × 81/80 = 9/8

**Cents:**
90 + 22 + 70 + 22 = 204 cents

這就是 **Chatursruti Rishabha** (四 Shruti Rishabh) 的名稱由來，Ri (9/8) 距離 Sa 四個 Shrutis。

---

### 主要 Raga 的 Shruti 使用

#### Raga Bhairav (भैरव)
**Time:** 清晨 (Dawn Raga)
**Thaat:** Bhairav
**情緒:** 肅穆、虔誠、力量

**Swaras:**
```
S r G M P d N S'
```
- S (Sa) = 1/1 = 0 cents
- r (Komal Re) = 256/243 = 90 cents
- G (Shuddha Ga) = 81/64 = 408 cents
- M (Shuddha Ma) = 4/3 = 498 cents
- P (Pa) = 3/2 = 702 cents
- d (Komal Dha) = 128/81 = 792 cents
- N (Shuddha Ni) = 15/8 = 1088 cents
- S' (Sa) = 2/1 = 1200 cents

**特徵 Shruti:**
- Komal Rishabh (r): 使用 **Ati-Komal** (極降) 的 Shruti
- 在 Bhairav 中的 Re 搖擺 (Andolan) 從 Re2 到 Re4
- 等同於 Western Double Harmonic Major Scale

**Aroha (上行):** S r G M P d N S'
**Avaroha (下行):** S' N d P M G r S

**Vadi (主音):** Madhyam (Ma)
**Samvadi (副主音):** Shadja (Sa)

---

#### Raga Yaman (यमन)
**Time:** 黃昏 / 晚上第一 Prahar
**Thaat:** Kalyan
**情緒:** 平靜、浪漫、奉獻

**Swaras:**
```
S R G M# P D N S'
```
- S = 0 cents
- R (Shuddha Re) = 9/8 = 204 cents
- G (Shuddha Ga) = 81/64 = 408 cents
- M# (Tivra Ma) = 729/512 = 612 cents (或 45/32 = 590 cents)
- P = 3/2 = 702 cents
- D (Shuddha Dha) = 27/16 = 906 cents (或 5/3 = 884 cents)
- N (Shuddha Ni) = 243/128 = 1110 cents (或 15/8 = 1088 cents)
- S' = 1200 cents

**特徵:**
- 使用 Tivra Madhyam (升 Ma)
- 所有其他音都是 Shuddh (自然)
- ShrutiSense 準確率: 91.1%

**Aroha:** N̤ R G M♯ P D N S'
**Avaroha:** S' N D P M♯ G R S

**Vadi:** Gandhar (Ga)
**Samvadi:** Nishad (Ni)

---

#### Raga Bhairavi (भैरवी)
**Time:** 清晨結束時 / 音樂會結束曲
**Thaat:** Bhairavi
**情緒:** 悲傷、虔誠、全面情感

**Swaras (基礎):**
```
S r g M P d n S'
```
- S = 0 cents
- r (Komal Re) = 256/243 = 90 cents
- g (Komal Ga) = 32/27 = 294 cents
- M (Shuddha Ma) = 4/3 = 498 cents
- P = 3/2 = 702 cents
- d (Komal Dha) = 128/81 = 792 cents
- n (Komal Ni) = 16/9 = 996 cents
- S' = 1200 cents

**特殊性質:**
- **Mishra Bhairavi:** 可使用全部半音，包括自然音和變化音
- Re 和 Dha 常使用 Andolit (搖擺) 唱法
- ShrutiSense 準確率: 90.7%
- 等同於 Western Phrygian Mode 的 "all-komal" 音階

**Aroha:** S r g M P d n S'
**Avaroha:** S' n d P M g r S (但常有變化)

**特徵 Gamakas:**
- Rishabh 和 Dhaivat 使用極低的微分音位置

---

#### Raga Kafi (काफी)
**Time:** 晚上第一 Prahar / 清晨結束時
**Thaat:** Kafi
**情緒:** 輕鬆、田園、喜悅

**Swaras:**
```
S R g M P D n S'
```
- S = 0 cents
- R (Shuddha Re) = 9/8 = 204 cents
- g (Komal Ga) = 32/27 = 294 cents
- M (Shuddha Ma) = 4/3 = 498 cents
- P = 3/2 = 702 cents
- D (Shuddha Dha) = 27/16 = 906 cents
- n (Komal Ni) = 16/9 = 996 cents
- S' = 1200 cents

**特徵:**
- 類似 Dorian Mode
- Gandhar 和 Nishad 為 Komal
- 常用於半古典和民謠風格

**Aroha:** S R g M P D n S'
**Avaroha:** S' n D P M g R S

**Vadi:** Pa (Pancham)
**Samvadi:** Sa (Shadja)

---

#### Raga Asavari (आसावरी)
**Time:** 清晨後期 (Late Morning)
**Thaat:** Asavari (或 Bhairavi，取決於版本)
**情緒:** 嚴肅、虔誠、內省

**三種版本:**

**1. Komal Re Only (r-only Asavari) - 古老 Dhrupad 版本**
```
S r g M P d n S'
```
- S = 0 cents
- r (Komal Re) = 256/243 = 90 cents
- g (Komal Ga) = 32/27 = 294 cents
- M (Shuddha Ma) = 4/3 = 498 cents
- P = 3/2 = 702 cents
- d (Komal Dha, **Ati-Komal**) = 略低於 128/81 = <792 cents
- n (Komal Ni) = 16/9 = 996 cents
- S' = 1200 cents

**2. Shuddha Re Only (R-only Asavari) - 現代 Gwalior 派版本**
```
S R g M P d n S'
```
- S = 0 cents
- R (Shuddha Re) = 9/8 = 204 cents
- g (Komal Ga) = 32/27 = 294 cents
- (其餘同上)

**3. 混合版本 (Both r and R)**
使用 r 和 R 兩者

**特徵:**
- Dhaivat 常調音為 **Ati-Komal** (極降) Shruti
- 需要複雜的連接動作和表現性 Alankar (裝飾音) 環繞 Dha
- 列於 Lakshanagranthas 中作為 Malkauns 的 Ragini

**Aroha (r-only):** S r g M P d n S'
**Avaroha (r-only):** S' n d P M g r S

**Aroha (R-only):** S R g M P d n S'
**Avaroha (R-only):** S' n d P M g R S

---

### 實作注意事項

**靈活性:**
Indian Classical Music 中的 Shruti 與固定頻率點的概念不同。研究證實存在微分音性，但提供的是 **靈活性調音 (Flexible Intonation)** 的證據，而非固定音高點。

**旋律上下文影響:**
音高受旋律上下文影響的證據清晰可見。當代 Shruti 的含義更多與 **旋律形狀 (Ornamentation)** 和 **旋律上下文** 相關。

**Shruti 處理 (Shruti Treatment):**
基於不同類型的 Gamaks、Andolans 等的使用。在相同 Shrutis 的 Ragas 中，使用不同的 'Shruti-treatment' 給 Raga Swara 完全不同的色彩。

**範例:**
- **Sri Raga:** Re 的 Andolan/Gamak 從 Sa 到 Re2
- **Bhairav Raga:** Andolan 從 Re2 到 Re4

**Swaras 使用頻率範圍:**
Swaras 使用的是 "頻率範圍" (Range of Frequencies)，而不是固定的聲音頻率。

**實測數據:**
- 大二度平均約 204 cents (接近 9/8)
- 半音作為獨立音程，通常約 95 cents
- 大三度和其他音程可能與 Equal Temperament 有顯著差異

**軟體實作:**
- K-means 聚類證實存在 22 個不同的 Shrutis
- 音高比例與歷史來源 (如 Deval 的研究) 密切吻合，平均差異約 10 cents
- ShrutiSense (2025) 模型達到 91.3% 的 Shruti 分類準確率

---

## Japanese Scales

### 基本理論框架: Tetrachord Theory

**Fumio Koizumi (小泉文夫) 的四音組理論:**
- 基於 **純四度 (Perfect Fourth)** 的音階
- 使用兩個穩定的外框音和一個不穩定的中間音
- 重複構成四種不同音階: Min-Yo, Miyako-bushi, Ritsu, Ryukyu

**文化意義:**
- 五音五聲音階的音符被賦予男性和女性特徵
- 代表五種基本元素: 土、水、火、木、金
- 與西方不同，根音不是第一個音，而是 **中央的第三音**，更符合平衡概念

---

### 1. Miyako-bushi Scale (都節音階) / In Scale

**名稱:**
- Miyako-bushi (都節)
- In Scale (陰音階)
- Sakura Scale (因日本民謠「櫻花櫻花」而得名)

**用途:**
- Koto (箏) 和 Shamisen (三味線) 音樂
- 日本民謠 (Folk Music)

**音階結構 (從 D 開始):**
```
D - E♭ - G - A - B♭ - D
```

**音階結構 (從 C 開始):**
```
C - D♭ - F - G - A♭ - C
```

**音程 (半音):**
```
1 - 4 - 2 - 2 - 3
```

**Cents 值 (12-TET):**
- Root = 0 cents
- ♭2 = 100 cents (小二度)
- 4 = 500 cents (純四度)
- 5 = 700 cents (純五度)
- ♭6 = 800 cents (小六度)
- Octave = 1200 cents

**間隔:**
- 0 → 100: **100 cents** (半音)
- 100 → 500: **400 cents** (大三度)
- 500 → 700: **200 cents** (大全音)
- 700 → 800: **100 cents** (半音)
- 800 → 1200: **400 cents** (大三度)

**特徵:**
- Hemitonic Pentatonic (含半音的五聲音階)
- 強調 **半音音程 (Half-step Interval)** 的黑暗色彩
- 不含大三度或小三度，屬於 **模糊音階 (Ambiguous Scale)**
- 可產生模糊或神秘的音樂效果

---

### 2. In-sen Scale (陰旋音階)

**與 In Scale 的關係:**
In-sen 與 In Scale 非常相似，但使用 ♭7 而非 ♭6

**音階結構:**
```
C - D♭ - F - G - B♭ - C
```

**音程 (半音):**
```
1 - 4 - 2 - 3 - 2
```

**Cents 值:**
- Root = 0 cents
- ♭2 = 100 cents
- 4 = 500 cents
- 5 = 700 cents
- ♭7 = 1000 cents
- Octave = 1200 cents

**間隔:**
- 0 → 100: **100 cents** (半音)
- 100 → 500: **400 cents**
- 500 → 700: **200 cents**
- 700 → 1000: **300 cents** (小三度)
- 1000 → 1200: **200 cents**

**特徵:**
- 與 Phrygian Mode 共享降二度特徵
- 與 Iwato Scale 有共同點

---

### 3. Hirajoshi Scale (平調子)

**名稱:**
- Hirajōshi (平調子)
- Hira-chōshi
- 也稱: Kata-kumoi

**來源:**
由 Yatsuhashi Kengyō (八橋検校) 從三味線音樂改編用於箏的調弦音階

**重要注意:**
"Hirajoshi, Kumoijoshi, 和 Kokinjoshi '音階' 是西方對同名箏調弦的衍生物。這些音階被尋求'新'聲音的搖滾和爵士吉他手使用。"

**西方使用中的多個版本:**

**版本 1 (Burrows):**
```
C - E - F# - G - B - C
```
**Semitones:** 4 - 2 - 1 - 4 - 1
**Cents:** 0, 400, 600, 700, 1100, 1200

**版本 2 (Sachs / Slonimsky):**
```
C - D♭ - F - G♭ - B♭ - C
```
**Semitones:** 1 - 4 - 1 - 4 - 2
**Cents:** 0, 100, 500, 600, 1000, 1200

**版本 3 (Speed / Kostka & Payne):**
```
C - D - E♭ - G - A♭ - C
```
**Semitones:** 2 - 1 - 4 - 1 - 4
**Cents:** 0, 200, 300, 700, 800, 1200

**共同模式:**
所有版本都是 **Hemitonic Pentatonic** (含半音的五聲音階)，屬於相同音程模式的不同 modes:
**音程模式:** 2 - 1 - 4 - 1 - 4 (半音單位)

**特徵:**
- 更寬的音程與半音結合
- 產生明顯的東方聲音，與西方五聲音階不同

---

### 4. Kumoi Scale (雲井音階)

**與 Hirajoshi 的關係:**
Kumoi 是 Hirajoshi Scale 的一個 Mode

**音程公式:**
```
1 - 2 - ♭3 - 5 - ♭6
```
或
```
1 - 3 - 4 - 6 - 7
```

**從 C 開始:**
```
C - E - F - A - B - C
```
**Cents:** 0, 400, 500, 900, 1100, 1200

**從 A 開始:**
```
A - C# - D - F# - G# - A
```

**特徵:**
- 更寬的音程與半音結合
- 明顯的東方聲音

---

### 5. Yo Scale (陽音階)

**名稱:** Yo (陽) = "陽性"

**與五聲音階的關係:**
可視為 **Major Pentatonic Scale 的第四 Mode**

**音程公式:**
```
1 - 2 - 4 - 5 - 6
```

**半音模式:**
```
2 - 3 - 2 - 2 - 3
```

**Cents 值:**
- Root = 0 cents
- 2 = 200 cents (大全音)
- 4 = 500 cents (純四度)
- 5 = 700 cents (純五度)
- 6 = 900 cents (大六度)
- Octave = 1200 cents

**間隔:**
- 0 → 200: **200 cents** (大全音)
- 200 → 500: **300 cents** (小三度)
- 500 → 700: **200 cents** (大全音)
- 700 → 900: **200 cents** (大全音)
- 900 → 1200: **300 cents** (小三度)

**學習方法:**
可以視為 **大調去掉第三和第七音**

**特徵:**
- **Anhemitonic** (不含半音)
- 與 In Scale 相對 (Yo = 陽, In = 陰)
- 不含小音符，較為明亮

**相關音階:**
Hirajoshi, In-sen, Iwato, Kumoi (Dorian Pentatonic)

---

### 6. Ritsu Scale (律音階)

**名稱:** Ritsu (律)

**用途:**
用於日本佛教詠唱 **Shōmyō (聲明)**

**音階類型:**
**Anhemitonic Pentatonic** (無半音五聲音階)

**音程構成:**
```
大二度 - 小三度 - 大二度 - 大二度 - 小三度
```

**度數記號:**
```
[1 / 2 / 4 / 5 / 6]
```

**Cents 值 (從 E 開始，可移調到 E 和 B):**
- E = 0 cents
- F# = 200 cents (大全音)
- A = 500 cents (純四度)
- B = 700 cents (純五度)
- C# = 900 cents (大六度)
- E' = 1200 cents

**間隔:**
- 0 → 200: **200 cents** (大全音)
- 200 → 500: **300 cents** (小三度)
- 500 → 700: **200 cents** (大全音)
- 700 → 900: **200 cents** (大全音)
- 900 → 1200: **300 cents** (小三度)

**特徵:**
- 不精確符合 Equal Temperament
- 但可移調到 E 和 B
- 與 Yo Scale 音程相同

---

### 7. Ryukyu Scale (琉球音階)

**名稱:** Ryūkyū (琉球) = Okinawa (沖繩)

**著名曲目:**
Okinawan 民謠 "Tinsagu Nu Hana" (てぃんさぐぬ花)

**音階結構:**
```
[1 / 3 / 4 / 5 / 7]
```

**從 C 開始:**
```
C - E - F - G - B - C
```

**Cents 值:**
- C = 0 cents
- E = 400 cents (大三度)
- F = 500 cents (純四度)
- G = 700 cents (純五度)
- B = 1100 cents (大七度)
- C' = 1200 cents

**間隔:**
- 0 → 400: **400 cents** (大三度)
- 400 → 500: **100 cents** (半音)
- 500 → 700: **200 cents** (大全音)
- 700 → 1100: **400 cents** (大三度)
- 1100 → 1200: **100 cents** (半音)

**特徵:**
- 前四個音 [1, 3, 4, 5] 基本上是 "Oh When the Saints" 的音型
- 第五音 [7] 是 "曲線球" (Curve Ball)
- 獨特的魅力來自前四音與第五音之間的跳躍
- 既有東方又有南太平洋風格的聲音
- 與其他日本音階有不同的韻味

---

### 8. Min-Yo Scale (民謠音階)

**音程公式:**
```
[1 / ♭3 / 4 / 5 / ♭7]
```

**Cents 值:**
- 1 = 0 cents
- ♭3 = 300 cents
- 4 = 500 cents
- 5 = 700 cents
- ♭7 = 1000 cents
- Octave = 1200 cents

**間隔:**
- 0 → 300: **300 cents** (小三度)
- 300 → 500: **200 cents** (大全音)
- 500 → 700: **200 cents** (大全音)
- 700 → 1000: **300 cents** (小三度)
- 1000 → 1200: **200 cents** (大全音)

---

### 9. Iwato Scale (岩戸音階)

**音程公式:**
```
[1 / ♭2 / 4 / ♭5 / ♭7]
```

**從 C 開始:**
```
C - D♭ - F - G♭ - B♭ - C
```

**Cents 值:**
- C = 0 cents
- D♭ = 100 cents
- F = 500 cents
- G♭ = 600 cents
- B♭ = 1000 cents
- C' = 1200 cents

**間隔:**
- 0 → 100: **100 cents** (半音)
- 100 → 500: **400 cents**
- 500 → 600: **100 cents** (半音)
- 600 → 1000: **400 cents**
- 1000 → 1200: **200 cents**

**特徵:**
- 與 In-sen 和 Phrygian Mode 共享降二度

---

### 四種四音組結合形成的五聲音階

當兩個相同類型的四音組以分離方式 (Disjunct) 結合時，在八度框架內產生四種五聲音階:
1. **Miyako-bushi Scale** (都節音階)
2. **Min-Yo Scale** (民謠音階)
3. **Ritsu Scale** (律音階)
4. **Ryūkyū Scale** (琉球音階)

---

### 實作注意事項

**調音實踐:**
日本傳統音階實際上 **不精確符合 Equal Temperament**。這些音階在傳統樂器 (如箏、三味線、尺八) 上的調音可能與 12-TET 有細微差異。

**上行/下行差異:**
搜索結果未提供日本音階有明確的上行/下行音程差異的證據 (與 Arabic Maqam 或某些 Indian Ragas 不同)。

**現代應用:**
這些音階被現代音樂家 (特別是搖滾和爵士吉他手) 採用，尋求 "新" 或 "異國情調" 的聲音。

---

## Gamelan System

### 基本理論框架

**兩大調音系統:**
1. **Slendro (Sléndro):** 五音系統
2. **Pelog (Pélog):** 七音系統

**重要哲學:**
Gamelan 調音的概念與西方音樂理論基於八度純淨性的科學測量系統 **根本不同**。在 Gamelan 中，八度 **不** 基於音高頻率的數學加倍和減半。

**獨特性:**
- 每個 Gamelan 樂團有其獨特的調音
- 固定音高樂器的調音具有極大靈活性
- 不同樂團的絕對音高差異很大
- 即使在同一樂團內，不同樂器產生的同名音也在音高上有很大變化

**變化原因:**
- 樂器製造中使用的金屬逐漸變化
- 對單一樂團樂器間產生 **聲學拍頻 (Acoustical Beats)** 的高度重視

---

### "Embat" 現象

**定義:**
Gamelan 調音系統中的 "Embat" 指音程結構總是略有不同 (少於 50 cents)，甚至難以確定，因為某些金屬樂器 (青銅/鐵) 具有不穩定的音色和不同的泛音結構。

**含義:**
一個樂器的音程結構可能與另一個樂器不完全相同。

---

### Slendro 系統

#### 基本特徵

**定義:**
Slendro 是一個 **五音音階 (Pentatonic)** 調音系統，每個音高與下一個音高大致等距。

**理論近似:**
有些人認為 Slendro 聽起來像 **5-EDO** (Equal Division of the Octave into 5)。

**5-EDO 理論值:**
- 每個音程 = 1200/5 = **240 cents**

#### 實測數據

**Jaap Kunst 的測量:**
Jaap Kunst 測量了來自不同地區的 8 個 Slendro 音階 (由 Larry Polansky 在 "Interval Sizes in Javanese Slendro" 中呈現)。

**音程範圍:**
測量值都接近 5-EDO 的 240 cents 步進，但範圍從 **206 到 268 cents**。

**區域變異:**
- **Central Java (中爪哇):** Slendro 在不同 Gamelan 之間的變化較小
- **Bali (峇里島):** 即使來自同一村莊的樂團，調音也可能非常不同

#### Slendro 典型 Cents 值範圍

**基於多項研究的統計偏好:**

**音程 (相鄰音符之間):**
- 小間隔: ~206-230 cents
- 中間隔: ~230-250 cents
- 大間隔: ~250-268 cents

**五個音的近似 Cents (從根音開始):**
- 音 1 = 0 cents
- 音 2 = ~220-240 cents
- 音 3 = ~460-480 cents
- 音 4 = ~700-720 cents
- 音 5 = ~940-960 cents
- 音 1' (八度) = ~1200-1220 cents (拉伸八度)

**重要注意:**
這些是統計近似值。實際調音在不同 Gamelan 之間差異很大。

#### Sundanese Slendro (Salendro)

**特徵:**
- Sundanese Gamelan Salendro 的音高 **略低於** Javanese Gamelan Slendro
- Sundanese Salendro 中五個音之間的距離 **更均等** (compared to Javanese Slendro)

**聲樂音高:**
Sundanese 音樂學家 Raden Machjar Angga Koesoemadinata 識別出 Slendro 中使用的 **17 個聲樂音高**。

#### 實例測量

**Universitas Pendidikan Indonesia 的 Gamelan:**
- **Central Javanese-style Pelog Salendro Gamelan**
  - 基音 (Tugu) = 472 Hz

---

### Pelog 系統

#### 基本特徵

**定義:**
Pelog 是一個 **七音音階 (Heptatonic)** 調音系統，從中構建五音 Modes。

**變異性:**
- 沒有單一的 Pelog 音階定義
- Gamelan 之間有相當大的變化
- 尤其在不同地區之間 (特別是 Java, Bali, Sunda)

**理論近似:**
可以粗略地將 Central Javanese Pelog 的七個音高表示為 **9-EDO 的子集**。

**9-EDO 理論值:**
- 每個音程 = 1200/9 = **133.33 cents**

#### 實測數據

**Surjodiningrat 的研究 (1972):**
Surjodiningrat, Sudarjana 和 Susanto 對 **76 個 Javanese Gamelans** 進行了廣泛的調音測量。

**關鍵出版物:**
Surjodiningrat, Wasisto, et al. (1972). *Tone Measurements of Outstanding Javanese Gamelans in Jogakarta and Surakarta*. Jogjakarta: Gadjah Mada University Press.

**主要發現:**
- 對 27 個 Central Javanese Gamelans 的分析顯示對 9-EDO 系統的統計偏好
- 研究人員將五種理論模型擬合到頻率數據，發現 **指數模型 (Exponential Models)** 最適合 Slendro 和 Pelog 音階
- 相對於西方音階的輕微拉伸，Javanese Slendro 和 Pelog 都 **相當拉伸 (Considerably Stretched)**

#### Pelog 音程結構

**小音程 (Single Steps):**
- 約 **100-160 cents**

**大音程 (Double Steps):**
- 約 **240-300 cents**

**特徵:**
- 音階具有 **靈活、不等距的特性**
- 為樂團共鳴和微妙拍頻效果提供貢獻

#### Pelog 理論 Cents 值 (Just Intonation 提議)

**一位研究者提出的 Just Pelog 調音:**
```
[0c, 128c, 267c, 563c, 702c, 814c, 979c, 1214c]
```

**9-EDO 近似:**
相對音階度數接近 9-EDO 的一步或兩步:
- 1 步 = 133 cents
- 2 步 = 267 cents

#### Pelog 典型 Cents 值範圍

**七個音的近似 Cents (從根音開始):**
- 音 1 = 0 cents
- 音 2 = ~120-140 cents
- 音 3 = ~260-280 cents
- 音 4 = ~550-570 cents
- 音 5 = ~690-710 cents
- 音 6 = ~800-820 cents
- 音 7 = ~960-990 cents
- 音 1' (八度) = ~1210-1220 cents (拉伸八度)

**五音子集:**
從七個音中選擇五個音來演奏。

---

### 八度拉伸 (Octave Stretching)

#### 定義

Gamelan 的八度 **不是精確的 1200 cents**，而是 **拉伸** 的，經常超過 1200 cents。

#### 典型值

**八度拉伸範圍:**
- **1200-1220 cents** (有時高達 1220 cents)

#### 原因

**1. 高不諧性 (High Inharmonicity):**
- Gamelan 樂器 (金屬共鳴板) 固有的高不諧性

**2. Ombak (拍頻效果) - Balinese 傳統:**
- 在 Balinese Gamelan 中，樂器 **成對演奏**，調音略有差異
- 產生 **干涉拍頻 (Interference Beating)**
- 理想情況下，所有音符對在所有音域中的拍頻速度一致
- 產生拉伸八度作為結果
- 貢獻於 Gamelan 樂團非常 "忙碌" 和 "閃爍" 的聲音

**Javanese vs. Balinese:**
- **Central Javanese Gamelan:** 不使用成對調音和故意拍頻
- **Balinese Gamelan:** 使用成對調音產生 Ombak

#### 學術研究

**重要文獻:**
- Carterette, Edward C., and Roger A. Kendall. "On the Tuning and Stretched Octave of Javanese Gamelans." *Leonardo Music Journal*, Vol. 4 (1994), pp. 59-68.
- Sethares, W. A., & Vitale, W. (2020). "Ombak and octave stretching in Balinese gamelan." *Journal of Mathematics and Music*, 16(1), 1–17.

---

### "Tumbuk" (共享音)

**定義:**
Pelog 和 Slendro 音階同時使用時，會共享一個音，稱為 **Tumbuk**。

**最常見的 Tumbuk:**
- Tumbuk **5**
- Tumbuk **6**
- Tumbuk **2** (偶爾)

---

### Sundanese Gamelan 調音

#### Tuning Systems (Laras)

**Sundanese Gamelan 有自己的 Pelog 調音:**
- **Javanese-like Pelog**
- **Sundanese Pelog (Degung)**
兩者在 Sundanese 音樂中共存。

#### 實例測量

**Universitas Pendidikan Indonesia:**
- **Gamelan Degung Surupan "57"**
  - Da = 401 Hz

---

### 實作建議

#### 1. 提供多種預設

由於 Gamelan 調音的極大變異性，建議提供:
- **Central Javanese Slendro** (較均勻)
- **Balinese Slendro** (較多變異)
- **Central Javanese Pelog** (9-EDO 近似)
- **Sundanese Pelog (Degung)**
- **Balinese Pelog** (with Ombak beating)

#### 2. 八度拉伸參數

允許用戶調整八度拉伸量 (1200-1220 cents)。

#### 3. "Embat" 變異

考慮添加輕微隨機變異 (±10-20 cents) 來模擬 Embat 現象。

#### 4. Ombak (拍頻) 模擬

對於 Balinese 風格:
- 成對的聲音，調音略有差異 (±5-10 cents)
- 產生拍頻效果

#### 5. 不要過度精確化

記住 Gamelan 調音的哲學是 **每個樂團的獨特調音**，而不是標準化的音階。提供 "精確" 的 Cents 值與 Gamelan 精神相悖。

---

## 數據來源與學術參考

### Arabic Maqam
- [Arabic maqam - Wikipedia](https://en.wikipedia.org/wiki/Arabic_maqam)
- [Section 7.9: Maqamat - Offtonic Theory](https://offtonic.com/theory/book/7-9.html)
- [Makams and Cents - Baba Yaga Music](https://babayagamusic.com/Music/Makams-and-Cents.htm)
- [Microtonal Theory - Makams and Maqamat](https://www.microtonaltheory.com/microtonal-ethnography/makams-and-maqamat)
- Sami Abu Shumays. "Maqam Analysis: A Primer." Society for Music Theory.
- [Arab tone system - Wikipedia](https://en.wikipedia.org/wiki/Arab_tone_system)

### Turkish Makam
- [Turkish makam - Wikipedia](https://en.wikipedia.org/wiki/Turkish_makam)
- [53 equal temperament - Wikipedia](https://en.wikipedia.org/wiki/53_equal_temperament)
- [Microtonal Theory - Turkish Makams](https://www.microtonaltheory.com/microtonal-ethnography/turkish-makams)
- Ozan Yarman. "79-tone Tuning & Theory for Turkish Maqam Music." Ph.D. dissertation, Istanbul Technical University, 2008.
- Ozan Yarman. "YARMAN-36 Makam Tone-System for Turkish Art Music."
- Barış Bozkurt. "Features for Analysis of Makam Music."

### Indian Raga
- [Shruti (music) - Wikipedia](https://en.wikipedia.org/wiki/Shruti_(music))
- [22 SHRUTI](https://22shruti.com/)
- [Raga intonation - Bol Processor](https://bolprocessor.org/raga-intonation/)
- [Derivation of the 22 Srutis - Carnatic Corner](http://www.carnaticcorner.com/articles/22_srutis.htm)
- [515 RESONANCE - The Notion of Twenty-Two Shrutis (IAS)](https://www.ias.ac.in/article/fulltext/reso/020/06/0515-0531)
- "ShrutiSense: Microtonal Modeling and Correction in Indian Classical Music" (2025)
- [Bharat and Sarang Dev's 22 Shrutis - PureTones](https://puretones.sadharani.com/)

### Japanese Scales
- [In scale - Wikipedia](https://en.wikipedia.org/wiki/In_scale)
- [Hirajōshi scale - Wikipedia](https://en.wikipedia.org/wiki/Hiraj%C5%8Dshi_scale)
- [Japanese musical scales - Wikipedia](https://en.wikipedia.org/wiki/Japanese_musical_scales)
- [Ritsu and ryo scales - Wikipedia](https://en.wikipedia.org/wiki/Ritsu_and_ryo_scales)
- [Piano Scales - Japanese Scales](https://www.pianoscales.org/)
- Fumio Koizumi. Tetrachord Theory.

### Gamelan
- [Slendro - Wikipedia](https://en.wikipedia.org/wiki/Slendro)
- [Pelog - Wikipedia](https://en.wikipedia.org/wiki/Pelog)
- [Microtonal Theory - Indonesian Gamelan](https://www.microtonaltheory.com/microtonal-ethnography/indonesian-gamelan)
- [A Guide to the Sundanese Gamelan Tuning - Ableton](https://tuning.ableton.com/sundanese-gamelan/intro-to-sundanese-gamelan/)
- Surjodiningrat, Wasisto, et al. (1972). *Tone Measurements of Outstanding Javanese Gamelans in Jogjakarta and Surakarta*. Gadjah Mada University Press.
- Carterette, E. C., & Kendall, R. A. (1994). "On the Tuning and Stretched Octave of Javanese Gamelans." *Leonardo Music Journal*, 4, 59-68.
- Sethares, W. A., & Vitale, W. (2020). "Ombak and octave stretching in Balinese gamelan." *Journal of Mathematics and Music*, 16(1), 1–17.
- Larry Polansky. "Interval Sizes in Javanese Slendro" and "Notes on The Tunings Of Three Central Javanese Slendro/Pelog Pairs."
- Jaap Kunst. Ethnomusicological measurements of Indonesian Gamelan.

---

## 實作建議總結

### 1. 數據精度的哲學

**Arabic Maqam & Turkish Makam:**
- 提供理論值 (24-TET / 53-TET)
- 注明這些是記譜慣例，實際演奏有變化
- 允許用戶微調 (±10-20 cents)

**Indian Raga:**
- 提供 Just Intonation 比例作為基礎
- 強調靈活性和上下文依賴性
- 某些音符 (如 Ati-Komal) 需要額外降低選項

**Japanese Scales:**
- 使用 12-TET 近似值
- 較為標準化，變異較小

**Gamelan:**
- 提供範圍而非精確值
- 包含八度拉伸參數
- 提供區域變體 (Javanese vs. Balinese vs. Sundanese)

### 2. 上行/下行差異

**需要實作的系統:**
- **Arabic Maqam:** Nahawand, Hijaz (某些形式)
- **Turkish Makam:** Rast (導音變化)
- **Indian Raga:** 某些 Ragas 的 Aroha/Avaroha 差異

**不需要的系統:**
- Japanese Scales
- Gamelan (上行下行相同)

### 3. 用戶介面建議

**預設系統:**
- 主要 Maqam/Makam/Raga 作為預設
- 地區變體選擇
- "Traditional" vs. "Theory" 模式

**微調選項:**
- 個別音符微調 (±50 cents)
- 八度拉伸控制 (Gamelan)
- Ombak 拍頻強度 (Balinese Gamelan)

### 4. 教育與文檔

**為每個音階提供:**
- 文化背景
- 情緒/時間關聯 (Ragas)
- 音程特徵
- 使用注意事項

---

## 結論

此研究報告收集了五大民族音樂系統的可用精確數據:

1. **Arabic Maqam:** 基於 24-TET 理論，但實踐中有靈活性
2. **Turkish Makam:** 53-TET Comma 系統，提供高精度微分音
3. **Indian Raga:** 22 Shruti 基於 Just Intonation，強調靈活性
4. **Japanese Scales:** 相對標準化的五聲音階，基於 12-TET
5. **Gamelan:** 高度個性化的調音，強調獨特性而非標準化

**關鍵發現:**
- 西方的 "精確 cents 值" 概念與許多民族音樂傳統的哲學 **相悖**
- 這些系統強調 **口傳心授**、**上下文依賴** 和 **個人/區域變異**
- 最佳實作應提供 **理論框架** + **靈活調整能力**

**下一步:**
將此數據轉換為 C++ 代碼結構，為 VCV Rack Quantizer 模組實作。
