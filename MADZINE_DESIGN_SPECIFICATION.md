# MADZINE VCV Rack 模組設計規範

版本：2.4.3
更新日期：2026-01-21

---

## 1. 文字標籤規範

### 字體大小
| 用途 | 字體大小 | Bold | 備註 |
|------|---------|------|------|
| 模組標題 | 14.f | true | 標準尺寸 |
| 模組標題（較長名稱） | 10.f | true | 如 PPaTTTerning |
| 品牌標籤 (MADZINE) | 10.f | false | |
| 功能標籤 | 8.f | true | Y < 330 區域 |
| 輸出區域標籤 | 7.f | true | Y >= 330 區域 |
| 背景裝飾文字 | 32.f | true | 如 Pyramid X/Y/Z |

### 顏色規範
| 情境 | RGB | 使用時機 |
|------|-----|----------|
| 橘色 (品牌色) | (255, 200, 0) | 模組標題、品牌標誌 |
| 白色 | (255, 255, 255) | 暗色背景上的標籤 |
| 黑色 | (0, 0, 0) | 白色區域內標籤 |
| 粉紅色 | (255, 133, 133) | 輸出端口標籤 |
| 淡灰色 | (160, 160, 160) | 背景裝飾文字 |

### 顏色決策
```
Y < 330 (暗色背景)
├─ 標題/品牌 → 橘色
├─ 一般控制 → 白色
└─ 背景裝飾 → 淡灰色 (160,160,160)

Y >= 330 (白色背景)
├─ 輸出 → 粉紅色
└─ 其他 → 黑色
```

### 背景裝飾標籤（Pyramid/DECAPyramid X/Y/Z）

**Pyramid:**
```
字體大小: 32.f
顏色: nvgRGB(160, 160, 160)
粗體: true
添加順序: 標籤在旋鈕之前添加，讓旋鈕渲染在上層
```

**DECAPyramid:**
```
字體大小: 80.f
顏色: nvgRGB(160, 160, 160)
外框: 黑色 2px (使用 OutlinedTextLabel)
Y 座標: X=80, Y=145, Z=215
添加順序: 標籤在旋鈕之前添加，讓旋鈕渲染在上層
```

---

## 2. 旋鈕規範

### 旋鈕類型
| 類型 | 尺寸 | 半徑 | 標籤偏移 | 用途 |
|------|------|------|----------|------|
| LargeWhiteKnob | 37px | 18.5px | 32px | EQ 大旋鈕 |
| StandardBlackKnob | 30px | 15px | 28px | 主要參數 |
| StandardBlackKnob26 | 26px | 13px | 26px | 緊湊主要參數 |
| WhiteKnob | 30px | 15px | 28px | CV/調製參數 |
| MediumGrayKnob | 26px | 13px | 26px | 次要參數 |
| SmallGrayKnob | 21px | 10.5px | 24px | 微小空間 |
| SnapKnob | 26px | 13px | 26px | 離散值選擇 |
| MicrotuneKnob | 20px | 10px | 23px | 微調控制 |

### 標籤偏移公式
```
標籤到旋鈕偏移 = 旋鈕半徑 + 標籤高度(10) + 間隙(3)
標籤框 X = 旋鈕中心 X - (標籤框寬度 / 2)
```

### 旋鈕選擇決策樹
```
控制類型？
├─ 離散值 → SnapKnob / MADDYSnapKnob
├─ 連續值
│  ├─ 主要參數 → StandardBlackKnob / StandardBlackKnob26
│  ├─ CV/調製 → WhiteKnob / LargeWhiteKnob
│  ├─ 次要參數 → MediumGrayKnob
│  └─ 微調 → MicrotuneKnob
└─ 隱藏控制 → HiddenKnob 系列
```

---

## 3. 自定義 defaultValue（雙擊歸零）

當參數需要非標準的 defaultValue 時（如 Speed=0.5 代表 1x），必須：

1. **使用自定義 ParamQuantity**
2. **手動設定所有欄位**
3. **onDoubleClick 使用 setValue(getDefaultValue())**

### 設定方式
```cpp
// 1. 定義自定義 ParamQuantity
struct SpeedParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        return knobToSpeed(getValue());  // 自定義顯示轉換
    }
};

// 2. 在 Module 構造函式中替換
configParam(SPEED_PARAM, 0.0f, 1.0f, 0.5f, "Speed", "x");
delete paramQuantities[SPEED_PARAM];
paramQuantities[SPEED_PARAM] = new SpeedParamQuantity;
paramQuantities[SPEED_PARAM]->module = this;
paramQuantities[SPEED_PARAM]->paramId = SPEED_PARAM;
paramQuantities[SPEED_PARAM]->minValue = 0.0f;
paramQuantities[SPEED_PARAM]->maxValue = 1.0f;
paramQuantities[SPEED_PARAM]->defaultValue = 0.5f;  // 關鍵：設定預設值
paramQuantities[SPEED_PARAM]->name = "Speed";
paramQuantities[SPEED_PARAM]->unit = "x";
```

### KnobBase.hpp onDoubleClick 實作
```cpp
void onDoubleClick(const event::DoubleClick& e) override {
    if (enableDoubleClickReset) {
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            // 重要：使用 setValue + getDefaultValue，不要用 reset()
            pq->setValue(pq->getDefaultValue());
            e.consume(this);
            return;
        }
    }
    app::Knob::onDoubleClick(e);
}
```

**注意**：不要使用 `pq->reset()`，它可能不會使用自定義的 defaultValue。

---

## 4. Y=330 白色區塊

### 規則
- Y >= 330 為白色 I/O 區域
- 所有輸入/輸出端口放置於此
- 標籤使用黑色或粉紅色

### 標準 Y 座標
```
330: 白色區域起始
343: I/O 第一行中心
368: I/O 第二行中心
380: 模組底部
```

### X 座標參考
| HP | 寬度 | 左 | 中 | 右 |
|----|------|-----|-----|-----|
| 4HP | 60px | 15 | 30 | 45 |
| 8HP | 120px | 15 | 60 | 105 |
| 12HP | 180px | 30 | 90 | 150 |

---

## 5. 設計檢查清單

### 視覺
- [ ] 標題：橘色 (255,200,0)、14.f、bold（長名稱可用 12.f）
- [ ] 品牌：橘色、10.f、非 bold
- [ ] 功能標籤：白色、8.f、bold
- [ ] 輸出區域標籤：粉紅色、7.f、bold
- [ ] Y=330 使用 WhiteBottomPanel
- [ ] 白色區域標籤使用黑色或粉紅色

### 控制元件
- [ ] 主要參數：StandardBlackKnob 系列
- [ ] CV 參數：WhiteKnob 系列
- [ ] 離散值：SnapKnob
- [ ] 標籤位置正確（使用偏移公式）

### 程式碼
- [ ] 右鍵選單含 addPanelThemeMenu()
- [ ] dataToJson/dataFromJson 儲存 panelTheme
- [ ] 自定義 defaultValue 使用正確方式

### 佈局
- [ ] 所有 I/O 在白色區域
- [ ] 控制元件在暗色區域
- [ ] 標籤在控制元件上方

---

## 6. 參考模組

| 用途 | 參考模組 |
|------|----------|
| 8HP 緊湊佈局 | MADDY, EuclideanRhythm |
| 12HP 標準佈局 | weiii documenta, NIGOQ |
| I/O 區域設計 | U8, Observer |
| 自定義 defaultValue | weiii documenta (Speed, Poly) |

---

## 附錄：相關檔案

- `src/widgets/KnobBase.hpp` - 旋鈕基類（含雙擊歸零）
- `src/widgets/Knobs.hpp` - 旋鈕定義
- `src/widgets/PanelTheme.hpp` - 主題切換系統
