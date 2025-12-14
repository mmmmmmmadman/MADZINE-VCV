# Universal Rhythm 模組設計
## VCV Rack Module Architecture

---

## 一、模組概覽

**名稱**：Universal Rhythm

**尺寸**：40 HP

**核心概念**：
- 4 個角色分組，每組 2 聲部（共 8 聲部）
- 每組獨立風格選擇
- 全域參數控制人性化與變奏
- 內建合成器 + 外部音訊混合
- Articulation Profile 系統

---

## 二、面板佈局

```
┌───────────────────────────────────────────────────────────────────────┐
│  U N I V E R S A L  R H Y T H M                                  40HP │
│  MADZINE                                                              │
├───────────────────────────────────────────────────────────────────────┤
│  [================== Pattern Display (8 tracks) ==================]   │
├───────────────────────────────────────────────────────────────────────┤
│  CLOCK  RESET   REGEN   VARI  HUMAN SWING  REST    FILL  ARTIC GHOST ACC │
│    ○    [●] ○   [●] ○    ○     ○     ○     ○  ○    ○  ○   ○    ○    ○   │
├───────────────────────────────────────────────────────────────────────┤
│  FOUNDATION  │  TIMELINE   │   GROOVE    │    LEAD                    │
├──────────────┼─────────────┼─────────────┼────────────────────────────┤
│   STYLE      │   STYLE     │   STYLE     │   STYLE                    │
│     ○        │     ○       │     ○       │     ○                      │
│   DENSITY    │   DENSITY   │   DENSITY   │   DENSITY                  │
│     ○        │     ○       │     ○       │     ○                      │
│   LENGTH     │   LENGTH    │   LENGTH    │   LENGTH                   │
│     ○        │     ○       │     ○       │     ○                      │
│   FREQ       │   FREQ      │   FREQ      │   FREQ                     │
│     ○   CV   │     ○   CV  │     ○   CV  │     ○   CV                 │
│   DECAY      │   DECAY     │   DECAY     │   DECAY                    │
│     ○   CV   │     ○   CV  │     ○   CV  │     ○   CV                 │
│   MIX        │   MIX       │   MIX       │   MIX                      │
│     ○        │     ○       │     ○       │     ○                      │
│   IN1  IN2   │   IN1  IN2  │   IN1  IN2  │   IN1  IN2                 │
│    ○    ○    │    ○    ○   │    ○    ○   │    ○    ○                  │
├──────────────┴─────────────┴─────────────┴────────────────────────────┤
│  OUTPUT:  MIX  │ FD1 FD2 │ TL1 TL2 │ GR1 GR2 │ LD1 LD2                │
│           L R  │ AUD GATE│ AUD GATE│ AUD GATE│ AUD GATE               │
│           ○ ○  │  ○   ○  │  ○   ○  │  ○   ○  │  ○   ○                 │
│                │ CV  ACC │ CV  ACC │ CV  ACC │ CV  ACC                │
│                │  ○   ○  │  ○   ○  │  ○   ○  │  ○   ○                 │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 三、參數系統

### 全域參數

| 參數 | 範圍 | 功能 |
|------|------|------|
| VARIATION | 0-1 | 風格權重與隨機的混合比例 |
| HUMANIZE | 0-1 | 時序微偏移量 |
| SWING | 0-1 | Swing 比例（BPM 相依） |
| REST | 0-1 | 音符靜音機率 |
| FILL | 0-1 | 過門音符密度 |
| ARTICULATION | 0-1 | 裝飾技法發生機率 |
| GHOST | 0-1 | 幽靈音符添加量 |
| ACCENT | 0-1 | 重音增強量（優先強拍） |

### 每組參數

| 參數 | 範圍 | 功能 |
|------|------|------|
| STYLE | 0-9 | 風格選擇（10 種） |
| DENSITY | 0-1 | 音符密度 |
| LENGTH | 3-16 | Pattern 長度 |
| FREQ | 20-2000 Hz | 內建合成器頻率 |
| DECAY | 0-1 | 內建合成器衰減 |
| MIX | 0-1 | 內建/外部混合比例 |

---

## 四、風格系統

### 10 種內建風格

每種風格定義：
- 16 位置 × 4 角色的權重矩陣
- Swing 比例
- 互鎖規則強度
- 時序變異範圍

| ID | 風格 | 細分 | 特色 |
|----|------|------|------|
| 0 | West African | 12 | 12/8 複節奏、強互鎖 |
| 1 | Afro-Cuban | 16 | Clave 導向 |
| 2 | Brazilian | 16 | Samba 搖擺 |
| 3 | Balkan | 混合 | 不規則拍 |
| 4 | Indian | 16 | Tala 循環 |
| 5 | Gamelan | 可變 | Colotomic 結構 |
| 6 | Jazz | 12 | 三連音律動 |
| 7 | Electronic | 16 | 機械精準 |
| 8 | Breakbeat | 16 | 取樣切片 |
| 9 | Techno | 16 | 極簡循環 |

---

## 五、Articulation Profile 系統

### 裝飾技法類型

| 類型 | 說明 | 時序 |
|------|------|------|
| Flam | 雙擊（前置裝飾音） | -15ms |
| Drag | 拖曳（雙前置音） | -30ms, -15ms |
| Ruff | 滾奏（三前置音） | -45ms, -30ms, -15ms |
| Buzz | 蜂鳴滾奏 | 連續 4-8 個細碎擊點 |

### 風格×角色對應表（範例）

| 風格 | Timeline | Foundation | Groove | Lead |
|------|----------|------------|--------|------|
| 西非 | Flam 中機率 | Drag 低機率 | Flam 高機率 | Ruff 中機率 |
| 古巴 | Flam 低機率 | None | Flam 中機率 | Drag 高機率 |
| 巴西 | Flam 中機率 | Drag 低機率 | Ruff 高機率 | Buzz 中機率 |
| 爵士 | Flam 低機率 | Drag 低機率 | Buzz 中機率 | Ruff 高機率 |
| 電子 | None | None | Flam 低機率 | Flam 低機率 |

### 選擇邏輯

1. 讀取當前角色的風格設定
2. 從 Profile 表取得該風格×角色的設定
3. 計算機率 = baseProbability × ARTICULATION 旋鈕值
4. 檢查條件（onAccentsOnly, onStrongBeats）
5. 機率觸發時產生對應裝飾技法

---

## 六、互鎖機制

### 生成順序

1. Timeline（獨立生成）
2. Foundation（閃避 Timeline）
3. Groove（補位 Timeline + Foundation 空隙）
4. Lead（協商所有其他聲部）

### 互鎖強度（依風格）

| 風格 | 互鎖強度 |
|------|----------|
| 西非、甘美朗 | 高（avoidance = 0.85） |
| 古巴、巴西 | 中（avoidance = 0.70） |
| 爵士 | 中低（avoidance = 0.50） |
| 電子、Techno | 無（avoidance = 0.0） |

---

## 七、輸入/輸出規格

### Inputs

| Port | 功能 |
|------|------|
| CLOCK | Master clock |
| RESET | 重設所有 position |
| REGEN | 觸發重新生成 |
| REST CV | Rest 參數調變 |
| FILL | Fill 觸發 |
| FREQ CV ×4 | 每組頻率 CV |
| DECAY CV ×4 | 每組衰減 CV |
| AUDIO IN ×8 | 外部音訊輸入（每組 2 個） |

### Outputs

| Port | 訊號 |
|------|------|
| MIX L/R | 立體聲混合輸出 |
| AUDIO ×8 | 個別聲部音訊 |
| GATE ×8 | 個別聲部 Gate |
| CV ×8 | 個別聲部 Velocity CV |
| ACCENT ×8 | 個別聲部 Accent Gate |

---

## 八、右鍵選單

- **Clock PPQN**：1 / 2 / 4 PPQN 選擇
- **Panel Theme**：面板主題切換

---

## 九、未來擴展

- **Spread 功能**：Role-based 立體聲擺位
  - Foundation 置中
  - Timeline/Groove/Lead 依 Spread 參數展開
