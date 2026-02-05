# MADZINE VCV Rack UI 規範

版本：1.1
更新日期：2026-01-21

---

## 1. 基本常數

```cpp
// VCV Rack 標準常數
RACK_GRID_WIDTH  = 15.24px  // 每 HP 寬度
RACK_GRID_HEIGHT = 380px    // 模組標準高度

// HP 對應像素寬度
4HP  = 60.96px
8HP  = 121.92px
12HP = 182.88px
16HP = 243.84px
32HP = 487.68px
40HP = 609.6px
```

---

## 2. 標籤與元件的相對位置

### 核心規則

```
標籤 Y = 元件 Y - 24px（標籤在元件上方 24px）

例如：
旋鈕 Y = 123 → 標籤 Y = 99
Port Y = 59  → 標籤 Y = 35
```

### 垂直間距

| 關係 | 間距 |
|------|------|
| 標籤到旋鈕 | 24px |
| 標籤到 Port | 24px |
| 旋鈕到 CV Input | 38px |
| Port 到 Port（垂直） | 25px |
| Row 1 到 Row 2 | 25px |

### 水平間距

| 元件 | 間距 |
|------|------|
| 左右 Port（同軌）| 30px |
| 旋鈕到 CV Input | 28px |
| 軌道間 | trackWidth |

---

## 3. 白色輸出區域規範

```
Y 開始: 330
高度: 50-60px
Y 結束: 380 (RACK_GRID_HEIGHT)

Row 1: Y = 343
Row 2: Y = 368
間距: 25px
```

背景顏色：`nvgRGB(255, 255, 255)` 白色

---

## 4. 標題區域規範

```
模組標題: Y = 1-2, fontSize = 14.f (標準) 或 12.f (長名稱)
副標題 "MADZINE": Y = 13-16, fontSize = 10.f, color = #FFC800 (金黃色)
```

---

## 5. 字體大小規範

| 用途 | 字體大小 | Bold | 備註 |
|------|----------|------|------|
| 模組標題 | 14.f | true | 標準尺寸 |
| 模組標題（長名稱）| 10.f | true | 如 PPaTTTerning |
| 副標題 MADZINE | 10.f | false | |
| 功能標籤 | 8.f | true | Y < 330 區域 |
| Output 區域標籤 | 7.f | true | Y >= 330 區域，粉紅色 |
| 背景裝飾文字 | 32.f | true | 如 Pyramid X/Y/Z，淡灰色 |

### 背景裝飾標籤規範（Pyramid/DECAPyramid）

**Pyramid（32.f 無外框）:**
```cpp
// 大型背景文字，旋鈕渲染在上層
addChild(new TechnoEnhancedTextLabel(Vec(7, 75), Vec(50, 10), "X", 32.f, nvgRGB(160, 160, 160), true));
// 之後添加旋鈕
addParam(createParamCentered<StandardBlackKnob26>(Vec(17, 95), module, X_PARAM));
```

**DECAPyramid（80.f 帶黑色外框）:**
```cpp
// 使用 OutlinedTextLabel 繪製帶黑色外框的大型背景文字
addChild(new OutlinedTextLabel(Vec(7, 80), Vec(50, 10), "X", 80.f, nvgRGB(160, 160, 160), 2.f));
addChild(new OutlinedTextLabel(Vec(7, 145), Vec(50, 10), "Y", 80.f, nvgRGB(160, 160, 160), 2.f));
addChild(new OutlinedTextLabel(Vec(7, 215), Vec(50, 10), "Z", 80.f, nvgRGB(160, 160, 160), 2.f));
// 之後添加旋鈕
addParam(createParamCentered<StandardBlackKnob26>(Vec(30, 95), module, X_PARAM));
```

---

## 6. 各 HP 尺寸的軌道設計

### 4HP (60.96px)

```
中心 X = 30
左 Port: X = 15
右 Port: X = 45
水平間距: 30px
```

### 8HP (121.92px)

```
2 軌設計: 每軌 60.96px
軌道 1 中心: X = 30
軌道 2 中心: X = 91
```

### 16HP (243.84px)

```
4 軌設計: 每軌 60.96px
軌道中心 X: 30, 91, 152, 213

或緊湊設計: 每軌 55px
軌道中心 X: 30, 85, 140, 195
```

### 32HP (487.68px)

```
8 軌設計: 每軌 60.96px
軌道中心 X = 30 + t * 60.96
```

### 40HP (609.6px)

```
4 Role 設計: 每 Role 152px
Role 中心 X = 76 + role * 152
```

---

## 7. 標準垂直位置（以 ALEXANDERPLATZ 為基準）

```
標題區: Y = 1-18
INPUT 標籤: Y = 35
INPUT Ports: Y = 59
VU Meter: Y = 71, 79
LEVEL 標籤: Y = 89
LEVEL 旋鈕: Y = 123
LEVEL CV: Y = 161
DUCK 標籤: Y = 182
DUCK 旋鈕: Y = 216
DUCK Input: Y = 254
MUTE/SOLO 標籤: Y = 270
Button: Y = 292
Trigger Input: Y = 316
白色區開始: Y = 330
Row 1: Y = 343
Row 2: Y = 368
```

---

## 8. 元件尺寸

### Port

```
PJ301MPort: 直徑約 24px
中心對齊（createCentered）
```

### 旋鈕

```
SmallWhiteKnob: 約 22px
SmallGrayKnob: 約 22px
MediumGrayKnob: 約 26px
StandardBlackKnob: 約 30px
```

### 按鈕

```
VCVButton: 約 16px
LightLatch: 約 20px
```

---

## 9. 佈局範例

### 單軌結構（自上而下）

```cpp
float centerX = 30;  // 4HP 中心

// 標籤在元件上方 24px
addChild(new TextLabel(Vec(centerX - 15, Y - 24), "LABEL"));
addParam(createParamCentered<Knob>(Vec(centerX, Y), module, PARAM_ID));
addInput(createInputCentered<PJ301MPort>(Vec(centerX, Y + 38), module, INPUT_ID));
```

### 多軌結構

```cpp
float trackWidth = 4 * RACK_GRID_WIDTH;  // 60.96px

for (int t = 0; t < TRACKS; t++) {
    float trackX = t * trackWidth;
    float centerX = trackX + trackWidth / 2;

    // 標籤
    addChild(new TextLabel(Vec(trackX, Y - 24), "LABEL"));
    // 元件
    addParam(createParamCentered<Knob>(Vec(centerX, Y), module, PARAM + t));
}
```

### 白色輸出區

```cpp
float outputY = 330;
addChild(new WhiteBox(Vec(0, outputY), Vec(box.size.x, 50)));

float row1Y = 343;
float row2Y = 368;

// 左側輸入
addInput(createInputCentered<PJ301MPort>(Vec(15, row1Y), module, CHAIN_L));
addInput(createInputCentered<PJ301MPort>(Vec(15, row2Y), module, CHAIN_R));

// 右側輸出
addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, row1Y), module, OUT_L));
addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, row2Y), module, OUT_R));
```

---

## 10. 常見錯誤與修正

### 錯誤 1：標籤被旋鈕遮住

```
錯誤: 標籤 Y = 旋鈕 Y + 10（標籤在下方）
正確: 標籤 Y = 旋鈕 Y - 24（標籤在上方）
```

### 錯誤 2：元件超出面板

```
錯誤: X 計算未考慮邊界
正確: 確保 15 <= X <= box.size.x - 15
```

### 錯誤 3：Output 排列不一致

```
錯誤: 隨意放置輸出
正確: 使用固定的 row1Y=343, row2Y=368
```

### 錯誤 4：使用縮寫

```
錯誤: TL, FD, GR, LD, F, D
正確: TIMELINE, FOUNDATION, GROOVE, LEAD, FREQ, DECAY
不允許以空間不足為由使用縮寫
```

---

## 11. Role/Track 顯示順序

對於多 Role 模組（如 UniversalRhythm），顯示順序為：

```
UI 順序: Foundation, Timeline, Groove, Lead
Role 索引: 1, 0, 2, 3

const int roleOrder[4] = {1, 0, 2, 3};
for (int uiPos = 0; uiPos < 4; uiPos++) {
    int role = roleOrder[uiPos];
    // 使用 uiPos 計算 X 位置
    // 使用 role 索引 enum
}
```

---

## 12. 面板主題支援

所有模組必須支援面板主題：

```cpp
// Module 類
int panelTheme = -1;
float panelContrast = panelContrastDefault;

json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
    json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
    return rootJ;
}

// Widget 類
PanelThemeHelper panelThemeHelper;

// 構造函數
panelThemeHelper.init(this, "16HP", module ? &module->panelContrast : nullptr);

// step()
panelThemeHelper.step(module);

// appendContextMenu()
addPanelThemeMenu(menu, module);
```
