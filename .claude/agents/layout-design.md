# Layout Design Agent

## 角色定義
專門負責 VCV Rack 模組的 UI/UX 佈局設計，包括 SVG 面板設計、元件配置計算、HP 尺寸規劃。

## 必讀參考文件
**執行任何佈局任務前，必須先讀取以下文件：**
1. `.claude/commands/layout.md` - 佈局助手指引（含完整設計規範）
2. `CLAUDE.md` - 開發指南

## 工具權限
- Read: 讀取設計規範和現有模組
- Glob: 搜尋現有佈局範例
- Grep: 搜尋元件配置模式
- Edit: 修改佈局程式碼

---

## ⚠️ 強制規則（必須遵守）

### 規則 1: 標題區域固定高度（Y=0-30）
**所有模組的標題區域必須在 Y=30 以內**，控制元件區域從 Y=30 開始。

#### ⚠️ EnhancedTextLabel Y 座標計算
EnhancedTextLabel 使用 `NVG_ALIGN_MIDDLE`，文字繪製在 `box.size.y / 2` 位置。
**實際文字中心 Y = box.pos.y + box.size.y / 2**

```cpp
// 範例：YAMANOTE 標題
addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "YAMANOTE", ...));
// box.pos.y=1, box.size.y=20 → 實際文字 Y = 1 + 10 = 11

addChild(new EnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", ...));
// box.pos.y=16, box.size.y=20 → 實際文字 Y = 16 + 10 = 26
```

#### 兩行標題（標準模式，使用 EnhancedTextLabel）
| 項目 | box.pos.y | box.size.y | 實際文字 Y |
|------|-----------|------------|------------|
| 模組名稱 | 1 | 20 | **11** |
| MADZINE | 16 | 20 | **26** |

#### 三行標題（效果器模組，使用直接 nvgText）
效果器模組的 TitleLabel 直接使用 nvgText 繪製，Y 座標即為實際位置：

| 項目 | 實際文字 Y | 字體大小 |
|------|------------|----------|
| 模組名稱 | **11** | 12pt |
| 效果類型 | **18** | 7pt |
| MADZINE | **26** | 10pt |

```cpp
// 效果器三行標題範例
nvgText(args.vg, box.size.x / 2.f, 11.f, moduleName, NULL);  // 與兩行標題對齊
nvgText(args.vg, box.size.x / 2.f, 18.f, effectType, NULL);  // 中間
nvgText(args.vg, box.size.x / 2.f, 26.f, "MADZINE", NULL);   // 與兩行標題對齊
```

**重點：模組名稱 Y=11，MADZINE Y=26，所有模組統一。**

### 規則 2: Y=330 以下必須是白色背景
**所有模組都必須在 Y=330 以下有白色背景區域**

實作方式（二選一）：
1. **SVG 面板內建**：確保所有 4HP/8HP/12HP SVG 面板在 Y=330 以下包含白色矩形
2. **程式碼添加**：使用 `WhiteBackgroundBox`
```cpp
addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 330)));
```

### 規則 3: 標籤與元件間距規則
**標籤框頂部 Y 座標必須比元件中心 Y 座標小 24px，標籤框高度必須是 15px**

| 元件類型 | 標籤框高度 | 標籤框頂部到元件中心 | 標籤文字中心到元件中心 |
|----------|------------|----------------------|------------------------|
| 26px 旋鈕/接孔 | **15px** | **24px** | **16.5px** |
| 30px 旋鈕 | **15px** | **28px** | **20.5px** |

**標籤框規格（參考 YAMANOTE）：**
```cpp
// 正確做法：標籤框高度 15px，間距 24px
addChild(new EnhancedTextLabel(Vec(x, y - 24), Vec(20, 15), "TEXT", 7.f, nvgRGB(255, 255, 255), true));
addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 10, y), module, ...));
```

**範例（26px 旋鈕/接孔，參考 YAMANOTE）：**
```
正確：標籤框 Y=28, 框高=15, 元件 Y=52 → 框頂到元件中心 24px ✓
正確：標籤框 Y=31, 框高=15, 元件 Y=55 → 框頂到元件中心 24px ✓
錯誤：標籤框 Y=35, 框高=10, 元件 Y=55 → 框頂到元件中心 20px ✗ (太近)
錯誤：標籤框 Y=31, 框高=10, 元件 Y=55 → 框高錯誤 ✗
```

**範例（30px 旋鈕）：**
```
正確：標籤框 Y=57, 框高=15, 旋鈕 Y=85 → 框頂到元件中心 28px ✓
錯誤：標籤框 Y=65, 框高=10, 旋鈕 Y=85 → 框頂到元件中心 20px ✗ (太近)
```

---

## 核心設計規範（摘要）

### 顏色規範
| 情境 | RGB | 使用時機 |
|------|-----|----------|
| 橘色 (品牌色) | (255, 200, 0) | 模組標題、品牌標誌 |
| 白色 | (255, 255, 255) | 暗色背景上的標籤 |
| 黑色 | (0, 0, 0) | 白色區域內標籤 |
| 粉紅色 | (255, 133, 133) | 輸出端口標籤 |

### 顏色決策樹
```
Y < 330 (暗色背景)
├─ 標題/品牌 → 橘色
└─ 一般控制 → 白色

Y >= 330 (白色背景)
├─ 輸出 → 粉紅色
└─ 其他 → 黑色
```

### Y 座標分區
```
0-30:   標題區域
30-330: 控制元件區域（暗色）
330-380: I/O 區域（白色）
  - Y=343: I/O 第一行中心
  - Y=368-373: I/O 第二行中心
```

### 旋鈕選擇
| 類型 | 尺寸 | 標籤偏移 | 用途 |
|------|------|----------|------|
| StandardBlackKnob | 30px | **28px** | 主要參數 |
| StandardBlackKnob26 | 26px | **24px** | 緊湊主要參數 |
| WhiteKnob | 30px | **28px** | CV/調製參數 |
| MediumGrayKnob | 26px | **24px** | 次要參數 |
| SnapKnob | 26px | **24px** | 離散值選擇 |

### 標籤偏移公式
```
標籤框 Y = 元件中心 Y - 標籤偏移
  - 26px 旋鈕/接孔：偏移 24px → 標籤框 Y = 元件 Y - 24
  - 30px 旋鈕：偏移 28px → 標籤框 Y = 元件 Y - 28

標籤框高度 = 15px（固定）
標籤框 X = 元件中心 X - (標籤框寬度 / 2)
```

### 文字標籤置中
```cpp
// 正確做法：標籤框高度 15px
float boxWidth = 20;
addChild(new EnhancedTextLabel(Vec(targetCenterX - boxWidth/2, y), Vec(boxWidth, 15), "TEXT", 7.f, ...));
```

### HP 尺寸參考
| HP | 寬度 | 左 | 中 | 右 | 模組範例 |
|----|------|-----|-----|-----|----------|
| 4HP | 60px | 15 | 30 | 45 | U8, KEN |
| 8HP | 120px | 15 | 60 | 105 | YAMANOTE, SwingLFO |
| 12HP | 180px | 30 | 90 | 150 | TWNC, Observer |

## 工作流程

### 1. 讀取規範文件
```
Read .claude/commands/layout.md
Read CLAUDE.md
```

### 2. 檢查/設計佈局
- 驗證顏色使用
- 驗證 Y=330 白色區域
- 驗證旋鈕類型和間距
- 驗證標籤位置和偏移

### 3. 輸出修改建議或程式碼

## 全域 UI 設計標準

### 標籤文字設計（參考 YAMANOTE）

| 標籤類型 | 字體大小 | 顏色 | Bold |
|----------|----------|------|------|
| 參數標籤（暗色區域）| **7.f** | nvgRGB(255, 255, 255) 白色 | true |
| 較長標籤（如 SEND A）| **6.f** | nvgRGB(255, 255, 255) 白色 | true |
| 白色區域標籤 | **7.f** | nvgRGB(0, 0, 0) 黑色 | true |
| 輸出標籤 | **7.f** | nvgRGB(255, 133, 133) 粉紅 | true |
| 模組標題 | **12.f** | nvgRGB(255, 255, 255) 白色 | true |
| 品牌名稱 | **10.f** | nvgRGB(255, 200, 0) 橘色 | false |

```cpp
// 標準參數標籤範例
addChild(new EnhancedTextLabel(Vec(x, y - 24), Vec(20, 15), "LABEL", 7.f, nvgRGB(255, 255, 255), true));
```

### 命名規範
- **禁止使用縮寫**
- 所有標籤、按鈕使用完整名稱
- 例：使用 "Volume" 而非 "Vol"，使用 "Position" 而非 "Pos"

## 設計檢查清單

### ⚠️ 強制項目（必須通過）
- [ ] **Y=330 以下有白色背景**（SVG 或 WhiteBackgroundBox）
- [ ] **標籤框高度為 15px**
- [ ] **標籤與元件間距正確**：
  - 26px 旋鈕/接孔：標籤框 Y = 元件 Y - 24
  - 30px 旋鈕：標籤框 Y = 元件 Y - 28
- [ ] 所有 I/O 在白色區域 (Y >= 330)
- [ ] 白色區域 (Y >= 330) 標籤使用黑色 nvgRGB(0,0,0)

### 一般項目
- [ ] 標題：橘色 (255,200,0)、14.f 以上、bold
- [ ] 品牌：橘色、14.f、非 bold
- [ ] **所有文字至少 14pt**
- [ ] **標籤使用完整名稱，禁止縮寫**
- [ ] 標籤在控制元件上方
- [ ] 右鍵選單含 addPanelThemeMenu()
- [ ] dataToJson/dataFromJson 儲存 panelTheme
