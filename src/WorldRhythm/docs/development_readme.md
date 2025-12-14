# Universal Rhythm - 開發說明

## 專案概述

VCV Rack 模組：基於世界節奏傳統的 generative rhythm sequencer

**核心特色：**
- Weighted probability 演算法（非 Euclidean）
- 4 角色分組 × 每角色 2 聲部 = 8 聲部
- 10 種風格 preset 影響生成權重
- 角色間互鎖邏輯
- 內建合成器 + 外部音訊輸入混合
- Articulation Profile 系統（基於風格×角色的裝飾技法）

---

## 模組規格

| 項目 | 規格 |
|------|------|
| 尺寸 | 40 HP |
| 分組 | Timeline (2), Foundation (2), Groove (2), Lead (2) = 8 聲部 |
| 每組參數 | Style, Density, Length, Freq, Decay, Mix |
| 全域參數 | Variation, Humanize, Swing, Rest, Fill, Articulation, Ghost, Accent |
| CV inputs | Style, Density, Freq, Decay per role + Clock + Reset + Regen + Rest + Fill |
| Outputs | 8 Audio + 8 Gate + 8 CV + 8 Accent + Mix L/R |

---

## 風格系統

### 10 種內建風格

| ID | Style | Swing | 特色 |
|----|-------|-------|------|
| 0 | West African | 0.62 | 12/8 複節奏、強互鎖 |
| 1 | Afro-Cuban | 0.58 | Clave 導向、切分豐富 |
| 2 | Brazilian | 0.57 | Samba 搖擺、Surdo 對話 |
| 3 | Balkan | 0.50 | 不規則拍、非對稱重音 |
| 4 | Indian | 0.50 | Tala 循環、Tihai 終止式 |
| 5 | Gamelan | 0.50 | Colotomic 結構、Irama 層次 |
| 6 | Jazz | 0.65 | 三連音律動、BPM 相依搖擺 |
| 7 | Electronic | 0.50 | 機械精準、四拍底鼓 |
| 8 | Breakbeat | 0.55 | 取樣切片、節奏斷裂 |
| 9 | Techno | 0.50 | 極簡、催眠式重複 |

---

## 核心系統

### 1. 四層角色系統

| 角色 | 功能 | 音樂類比 |
|------|------|----------|
| Timeline | 提供恆定參考框架 | 西非鐵鈴、古巴 Clave |
| Foundation | 建立低頻基礎 | 大鼓、Surdo |
| Groove | 填充中頻空間 | 康加、小鼓 |
| Lead | 即興與裝飾 | 獨奏樂器 |

### 2. 互鎖機制

- **閃避（Avoidance）**：Foundation 避開 Timeline 的強拍位置
- **補位（Complement）**：Groove 填補 Timeline 與 Foundation 之間的空隙
- **協商**：Lead 聲部綜合考量所有其他聲部

### 3. 人性化處理

- **時序變異**：風格特異性的微時序偏移（西非 ±25ms、爵士 ±15ms、電子 ±2ms）
- **BPM 相依 Swing**：慢速時接近 2:1（68%），快速時趨近直拍（54%）
- **Ghost Notes**：低力度的幽靈音符填充節奏空隙
- **Accent 增強**：優先在強拍（位置 0, 4, 8, 12）增加重音

### 4. Articulation Profile 系統

基於民族音樂學研究的裝飾技法系統：

**技法類型：**
- Flam（雙擊）：前置裝飾音
- Drag（拖曳）：雙前置音
- Ruff（滾奏）：三前置音
- Buzz（蜂鳴）：連續細碎擊點

**風格×角色對應：**
- 每種風格的每個角色都有專屬的 Articulation 設定
- 一個旋鈕控制發生機率
- 技法類型由風格與角色自動決定

---

## 檔案結構

```
/src/WorldRhythm/
  WorldRhythm.hpp              - 主要標頭檔
  ArticulationProfiles.hpp     - Articulation Profile 對照表

/src/WorldRhythm/docs/
  WorldRhythm_Algorithm_Paper.md  - 演算法論文
  WorldRhythm_TW.html            - 繁體中文文件
  WorldRhythm_EN.html            - 英文文件
  WorldRhythm_JP.html            - 日文文件
  unified_rhythm_analysis.md     - 節奏系統統一分析（研究）
  fills_ornaments_research.md    - 過門與裝飾音研究（研究）

/src/
  UniversalRhythm.cpp           - 模組主程式
```

---

## 參考資源

### 民族音樂學文獻
- Simha Arom (1991): 非洲複節奏與互鎖結構
- Gerhard Kubik (2010): 時間線理論
- Fernando Benadon (2006): 爵士微時序研究
- Godfried Toussaint (2013): 歐幾里得節奏
- Michael Tenzer (2000): 峇里島 Kotekan 理論

### Articulation 研究來源
- Afrodrumming.com: 西非打擊樂的 Flam 與 Drag 技法
- Marc Dédouvan: 古巴打擊樂的裝飾音傳統
- Gamelan.org.nz: 甘美朗音樂的 Kotekan 交織技法

### VCV Rack
- VCV Rack SDK: https://vcvrack.com/manual/PluginDevelopmentTutorial
