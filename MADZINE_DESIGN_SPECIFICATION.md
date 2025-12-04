# MADZINE VCV Rack 模組設計規範文件

版本：2.3.3
更新日期：2025-01-06

根據分析 Observer, ChaosRecorder, QQ, NIGOQ, Quantizer, MADDY, MADDYPlus, U8 等代表性模組，整理出以下設計規範。

---

## 1. 文字標籤系統 (EnhancedTextLabel)

### 1.1 基本實作

```cpp
struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    EnhancedTextLabel(Vec pos, Vec size, std::string text,
                      float fontSize = 12.f,
                      NVGcolor color = nvgRGB(255, 255, 255),
                      bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);

        if (bold) {
            float offset = 0.3f;
            nvgText(args.vg, box.size.x / 2.f - offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f + offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f - offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f + offset, text.c_str(), NULL);
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};
```

### 1.2 字體大小規範

| 用途 | 字體大小 | 範例 |
|------|---------|------|
| **模組標題** | 12.f | "Observer", "Q_Q", "M A D D Y" |
| **品牌標籤** | 10.f | "MADZINE" |
| **區段標題** | 7-8.f | "DECAY", "FILL", "D/M", "LEVEL" |
| **小型標籤** | 6-7.f | "REC", "PLAY", "CLR", "T1", "T2" |
| **超小型標籤** | 6.f | "QUTQ", 輸出端口標籤 |

### 1.3 Bold 參數使用規則

```cpp
// 標題層級：bold = true
addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20),
    "Observer", 12.f, nvgRGB(255, 200, 0), true));

// 品牌標籤：bold = false (較細)
addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20),
    "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

// 控制標籤：bold = true (一般情況)
addChild(new EnhancedTextLabel(Vec(5, 55), Vec(20, 20),
    "DECAY", 8.f, nvgRGB(255, 255, 255), true));
```

### 1.4 顏色使用規範

| 情境 | 顏色值 | RGB | 使用時機 |
|------|--------|-----|----------|
| **橘色 (品牌色)** | `nvgRGB(255, 200, 0)` | #FFC800 | 模組標題、品牌標誌、特殊功能標籤 |
| **白色** | `nvgRGB(255, 255, 255)` | #FFFFFF | 一般控制標籤（暗色背景上） |
| **黑色** | `nvgRGB(0, 0, 0)` | #000000 | 白色區域內的標籤 |
| **紅色/粉紅** | `nvgRGB(255, 133, 133)` | #FF8585 | 輸出端口標籤、強調元素 |

**顏色決策樹：**
```
標籤位置在哪？
├─ Y < 330 (暗色背景)
│  ├─ 是模組標題/品牌？ → 橘色 (255, 200, 0)
│  ├─ 是特殊功能？ → 橘色 (255, 200, 0)
│  └─ 一般控制？ → 白色 (255, 255, 255)
│
└─ Y >= 330 (白色背景)
   ├─ 是輸出？ → 粉紅色 (255, 133, 133)
   └─ 是輸入/其他？ → 黑色 (0, 0, 0)
```

### 1.5 標籤與控制元件的 Y 偏移

```cpp
// 標準間距模式
float controlY = 145;

// 標籤位置：控制元件 Y - 23
addChild(new EnhancedTextLabel(Vec(15, 122), Vec(30, 15),
    "GAIN", 7.f, nvgRGB(255, 255, 255), true));

// 控制元件位置
addParam(createParamCentered<StandardBlackKnob26>(Vec(30, controlY),
    module, MODULE::GAIN_PARAM));

// Y offset 規則：
// 小型控制 (26px knob): Y_label = Y_control - 23
// 中型控制 (30px knob): Y_label = Y_control - 25
// 大型控制 (37px knob): Y_label = Y_control - 30
```

---

## 2. 旋鈕種類使用規範

### 2.1 旋鈕尺寸與使用場景

| 旋鈕類型 | 尺寸 | 用途 | 代表模組 |
|---------|------|------|---------|
| **StandardBlackKnob** | 30×30 | 主要參數控制 | PPaTTTerning, QQ |
| **StandardBlackKnob26** | 26×26 | 緊湊佈局的主要參數 | ChaosRecorder, Pyramid |
| **SmallBlackKnob** | 21×21 | 已棄用 | (改用 SmallGrayKnob) |
| **LargeBlackKnob** | 37×37 | 罕見 | (改用 LargeWhiteKnob) |
| **WhiteKnob** | 30×30 | CV/調製參數 | MADDY, MADDYPlus |
| **MediumGrayKnob** | 26×26 | 次要參數、序列參數 | MADDY, Multiverse |
| **SmallGrayKnob** | 21×21 | 微小空間的精細控制 | NIGOQ |
| **LargeWhiteKnob** | 37×37 | 重要的調製參數 | NIGOQ |
| **SmallWhiteKnob** | 26×26 | Multiverse 特殊用途 | Multiverse |
| **SnapKnob** | 26×26 | 離散值選擇 | ChaosRecorder, EuclideanRhythm |
| **MADDYSnapKnob** | 26×26 | MADDY 系列 Div/Mult | MADDY, MADDYPlus |
| **MicrotuneKnob** | 20×20 | 微調參數 | Quantizer |
| **TechnoStandardBlackKnob** | 45×45 | Techno 風格大型控制 | U8 |

### 2.2 旋鈕選擇決策樹

```
需要什麼類型的控制？
├─ 離散值 (整數、選項)
│  └─ SnapKnob / MADDYSnapKnob (26×26)
│
├─ 連續值
│  ├─ 主要參數
│  │  ├─ 標準佈局 → StandardBlackKnob (30×30)
│  │  └─ 緊湊佈局 → StandardBlackKnob26 (26×26)
│  │
│  ├─ CV/調製參數
│  │  ├─ 標準 → WhiteKnob (30×30)
│  │  ├─ 大型 → LargeWhiteKnob (37×37)
│  │  └─ 小型 → SmallWhiteKnob (26×26)
│  │
│  ├─ 次要參數/序列
│  │  └─ MediumGrayKnob (26×26)
│  │
│  ├─ 微調控制
│  │  └─ MicrotuneKnob (20×20)
│  │
│  └─ Techno 風格
│     └─ TechnoStandardBlackKnob (45×45)
│
└─ 隱藏控制
   ├─ Scope 時間 → HiddenTimeKnob 系列
   └─ CV 衰減 → HiddenAttenuatorKnob
```

### 2.3 程式碼範例

```cpp
// 標準黑色旋鈕 (30×30) - 主要參數
addParam(createParamCentered<StandardBlackKnob>(
    Vec(30, 85), module, Module::DECAY_PARAM));

// 標準黑色旋鈕 (26×26) - 緊湊佈局
addParam(createParamCentered<StandardBlackKnob26>(
    Vec(30, 145), module, Module::GAIN_PARAM));

// 白色旋鈕 (30×30) - CV 參數
addParam(createParamCentered<WhiteKnob>(
    Vec(60, 154), module, Module::DENSITY_PARAM));

// 灰色旋鈕 (26×26) - 次要參數
addParam(createParamCentered<MediumGrayKnob>(
    Vec(20, 52), module, Module::LENGTH_PARAM));

// Snap 旋鈕 - 離散值
addParam(createParamCentered<SnapKnob>(
    Vec(30, 195), module, Module::CHAOS_TYPE_PARAM));

// 微調旋鈕 (20×20) - 精細控制
addParam(createParamCentered<MicrotuneKnob>(
    Vec(15, 310), module, Module::C_MICROTUNE_PARAM));
```

---

## 3. Y=330 白色區塊邏輯

### 3.1 WhiteBottomPanel 實作

```cpp
struct WhiteBottomPanel : TransparentWidget {
    void draw(const DrawArgs& args) override {
        // Draw white background from Y=330 to bottom
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 330, box.size.x, box.size.y - 330);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

// 使用方式（在 Widget 建構函式中）
WhiteBottomPanel* whitePanel = new WhiteBottomPanel();
whitePanel->box.size = box.size;
addChild(whitePanel);
```

### 3.2 白色區域元素規則

**應該放在白色區域的元素：**
- ✅ 所有輸入端口 (Input Ports)
- ✅ 所有輸出端口 (Output Ports)
- ✅ Chain 輸入/輸出 (用於模組串聯)
- ✅ 最終訊號路徑的 I/O

**不應該放在白色區域：**
- ❌ 控制旋鈕
- ❌ CV 輸入 (除非是最終輸出相關)
- ❌ 按鈕開關

### 3.3 白色區域文字顏色

```cpp
// Y >= 330 的標籤統一使用黑色文字
addChild(new EnhancedTextLabel(Vec(15, 320), Vec(30, 15),
    "IN L", 7.f, nvgRGB(0, 0, 0), true));

// 輸出標籤可使用粉紅色強調
addChild(new EnhancedTextLabel(Vec(5, 335), Vec(20, 15),
    "OUT", 8.f, nvgRGB(255, 133, 133), true));
```

### 3.4 白色區域端口排列規則

**4HP 模組 (60px 寬)：**
```cpp
// 雙列排列
// 第一行 Y=343, 第二行 Y=368
float row1Y = 343;
float row2Y = 368;

// 左右分佈
addInput(createInputCentered<PJ301MPort>(Vec(15, row1Y), ...));
addOutput(createOutputCentered<PJ301MPort>(Vec(45, row1Y), ...));
```

**8HP 模組 (120px 寬)：**
```cpp
// Observer 範例：4x2 網格
float spacing = 30; // 30px 間距
for (int i = 0; i < 4; i++) {
    float x = 15 + i * spacing;
    addInput(createInputCentered<PJ301MPort>(Vec(x, 343), ...)); // 上排
    addInput(createInputCentered<PJ301MPort>(Vec(x, 368), ...)); // 下排
}
```

**12HP 模組 (180px 寬)：**
```cpp
// ChaosRecorder 範例：每行 3 個端口
float centerX = box.size.x / 2;  // 90px

// 第一行 (Y=343)
addInput(createInputCentered<PJ301MPort>(Vec(30, 343), ...));
addInput(createInputCentered<PJ301MPort>(Vec(centerX, 343), ...));
addInput(createInputCentered<PJ301MPort>(Vec(box.size.x - 30, 343), ...));

// 第二行 (Y=373)
addInput(createInputCentered<PJ301MPort>(Vec(30, 373), ...));
addInput(createInputCentered<PJ301MPort>(Vec(centerX, 373), ...));
addInput(createInputCentered<PJ301MPort>(Vec(box.size.x - 30, 373), ...));
```

### 3.5 標籤位置數值參考

```cpp
// 標籤在端口上方：Y_label = Y_port - 23
addChild(new EnhancedTextLabel(Vec(15, 320), Vec(30, 15),
    "IN L", 7.f, nvgRGB(0, 0, 0), true));
addInput(createInputCentered<PJ301MPort>(Vec(30, 343), ...));

// 標籤在端口側邊 (節省空間，用於緊湊模組)
addChild(new EnhancedTextLabel(Vec(-2, 337), Vec(20, 15),
    "T1", 6.f, nvgRGB(255, 133, 133), true));
addOutput(createOutputCentered<PJ301MPort>(Vec(24, 343), ...));
```

---

## 4. 佈局模式

### 4.1 12HP 模組典型佈局

**垂直區域劃分 (380px 總高度)：**

```
Y=0-30:   頂部標題區 (品牌 + 模組名)
Y=30-330: 主控制區 (旋鈕、按鈕、顯示器)
Y=330-380: 白色 I/O 區 (輸入/輸出端口)
```

**ChaosRecorder 參考佈局：**
```cpp
box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);  // 180×380

// 頂部品牌
addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20),
    "w e i i i  d o c u m e n t a", 12.f, nvgRGB(255, 200, 0), true));
addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20),
    "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

// 波形顯示 (40-95)
WaveformDisplay* display = new WaveformDisplay();
display->box.pos = Vec(5, 40);
display->box.size = Vec(170, 55);

// 按鈕行 (105)
float buttonY = 105;
addParam(createParamCentered<VCVButton>(Vec(25, buttonY), ...));
addParam(createParamCentered<VCVButton>(Vec(90, buttonY), ...));
addParam(createParamCentered<VCVButton>(Vec(155, buttonY), ...));

// 旋鈕區 (145-290)，每行間距約 38-43px
float yPos = 145;
addParam(createParamCentered<StandardBlackKnob26>(Vec(30, yPos), ...));
yPos += 38;
addParam(createParamCentered<StandardBlackKnob26>(Vec(30, yPos), ...));

// 白色 I/O 區 (330-380)
addChild(new WhiteBottomPanel());
```

### 4.2 8HP 模組佈局

**Observer 參考佈局：**
```cpp
box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);  // 120×380

// 頂部標題 (0-30)
addChild(new EnhancedTextLabel(Vec(0, 1), Vec(120, 20),
    "Observer", 12.f, nvgRGB(255, 200, 0), true));

// Scope 顯示 (30-330)
ObserverScopeDisplay* scopeDisplay = new ObserverScopeDisplay();
scopeDisplay->box.pos = Vec(0, 30);
scopeDisplay->box.size = Vec(120, 300);

// 隱藏控制旋鈕 (覆蓋 Scope)
addParam(createParam<HiddenTimeKnobObserver>(Vec(0, 30), ...));

// 白色 I/O 區 (330-380)：4×2 網格
```

### 4.3 4HP 模組佈局

**QQ 參考佈局：**
```cpp
box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);  // 60×380

// 緊湊垂直排列
// 標題 (0-30)
// Track 1 控制 (45-120)
// Track 2 控制 (125-200)
// Track 3 控制 (205-280)
// Scope 顯示 (279-330)
// I/O 區 (330-380)
```

### 4.4 輸入/輸出排列方式

**對稱排列 (常見於音訊模組)：**
```cpp
// 左側輸入，右側輸出
addInput(createInputCentered<PJ301MPort>(Vec(15, 343), ...));
addOutput(createOutputCentered<PJ301MPort>(Vec(105, 343), ...));
```

**網格排列 (多通道模組)：**
```cpp
// Observer: 8 輸入，4×2 網格
for (int i = 0; i < 4; i++) {
    float x = 15 + i * 30;
    addInput(createInputCentered<PJ301MPort>(Vec(x, 343), ...));  // 上排
    addInput(createInputCentered<PJ301MPort>(Vec(x, 368), ...));  // 下排
}
```

**群組排列 (功能分組)：**
```cpp
// MADDY: 按功能分組
// Track outputs (上排)
addOutput(createOutputCentered<PJ301MPort>(Vec(24, 343), ...));  // T1
addOutput(createOutputCentered<PJ301MPort>(Vec(64, 343), ...));  // T2
addOutput(createOutputCentered<PJ301MPort>(Vec(102, 343), ...)); // T3

// Chain outputs (下排)
addOutput(createOutputCentered<PJ301MPort>(Vec(24, 368), ...));  // 12
addOutput(createOutputCentered<PJ301MPort>(Vec(64, 368), ...));  // 23
addOutput(createOutputCentered<PJ301MPort>(Vec(102, 368), ...)); // 123
```

### 4.5 控制元件分組邏輯

**按功能分組：**
```cpp
// ChaosRecorder 範例
// 預處理組 (Y=145-183)
GAIN, HPF, THRESHOLD

// 混沌引擎組 (Y=183-226, 橘色標籤)
TYPE, CHAOS, RATE

// 漸變組 (Y=226-264)
MORPH TIME, MORPH CURVE

// 回饋組 (Y=264-302)
FEEDBACK AMOUNT, FEEDBACK DELAY
```

**視覺分隔 (可選)：**
```cpp
// MADDY 使用線條分隔區段
struct VerticalLine : TransparentWidget {
    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, 0);
        nvgLineTo(args.vg, 0, box.size.y);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 40));
        nvgStroke(args.vg);
    }
};

addChild(new VerticalLine(Vec(39, 55), Vec(1, 242)));
```

---

## 5. 右鍵選單內容

### 5.1 PanelTheme 選單標準實作

```cpp
void appendContextMenu(ui::Menu* menu) override {
    MyModule* module = dynamic_cast<MyModule*>(this->module);
    if (!module) return;

    // 必備：主題切換
    addPanelThemeMenu(menu, module);

    // 可選：其他功能...
}
```

### 5.2 常見選單項目

**Attack Time 滑桿 (QQ, MADDY)：**
```cpp
menu->addChild(new MenuSeparator);
menu->addChild(createMenuLabel("Attack Time"));

struct AttackTimeSlider : ui::Slider {
    struct AttackTimeQuantity : Quantity {
        MyModule* module;
        int trackIndex;

        void setValue(float value) override {
            float attackTime = rescale(value, 0.0f, 1.0f, 0.0005f, 0.020f);
            module->tracks[trackIndex].attackTime = attackTime;
        }

        float getValue() override {
            return rescale(module->tracks[trackIndex].attackTime,
                          0.0005f, 0.020f, 0.0f, 1.0f);
        }

        std::string getDisplayValueString() override {
            return string::f("%.2f ms",
                module->tracks[trackIndex].attackTime * 1000.0f);
        }

        std::string getUnit() override { return ""; }
    };

    AttackTimeSlider(MyModule* module, int trackIndex) {
        box.size.x = 200.0f;
        quantity = new AttackTimeQuantity();
        quantity->module = module;
        quantity->trackIndex = trackIndex;
    }
};

menu->addChild(new AttackTimeSlider(module, 0));
```

**Preset 子選單 (Quantizer)：**
```cpp
menu->addChild(createSubmenuItem("Scale Presets", "", [=](Menu* menu) {
    std::string scaleNames[] = {
        "Chromatic", "Major (Ionian)", "Minor (Aeolian)",
        "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
        "Harmonic Minor", "Melodic Minor", "Pentatonic Major",
        "Pentatonic Minor", "Blues", "Whole Tone",
        "Augmented", "Diminished"
    };

    for (int i = 0; i < 16; i++) {
        menu->addChild(createMenuItem(scaleNames[i], "", [=]() {
            module->applyScalePreset(i);
        }));
    }
}));
```

**Toggle 選項 (QQ Retrigger)：**
```cpp
menu->addChild(new MenuSeparator);

struct RetriggerItem : ui::MenuItem {
    MyModule* module;

    void onAction(const event::Action& e) override {
        module->retriggerEnabled = !module->retriggerEnabled;
    }
};

RetriggerItem* item = new RetriggerItem;
item->text = "Retrigger";
item->rightText = CHECKMARK(module->retriggerEnabled);
item->module = module;
menu->addChild(item);
```

**Per-Track 設定 (MADDY Shift)：**
```cpp
menu->addChild(new MenuSeparator);
menu->addChild(createMenuLabel("Shift Settings"));

for (int trackId = 0; trackId < 3; trackId++) {
    struct TrackShiftMenu : MenuItem {
        MyModule* module;
        int trackIndex;

        Menu* createChildMenu() override {
            Menu* menu = new Menu();

            for (int shift = 0; shift <= 4; shift++) {
                struct ShiftMenuItem : MenuItem {
                    MyModule* module;
                    int trackIndex;
                    int shiftValue;

                    void onAction(const event::Action& e) override {
                        module->tracks[trackIndex].shift = shiftValue;
                    }
                };

                ShiftMenuItem* item = new ShiftMenuItem();
                item->module = module;
                item->trackIndex = trackIndex;
                item->shiftValue = shift;
                item->text = string::f("Shift %d", shift);
                item->rightText = CHECKMARK(module->tracks[trackIndex].shift == shift);
                menu->addChild(item);
            }
            return menu;
        }
    };

    TrackShiftMenu* trackMenu = new TrackShiftMenu();
    trackMenu->module = module;
    trackMenu->trackIndex = trackId;
    trackMenu->text = string::f("Track %d", trackId + 1);
    trackMenu->rightText = RIGHT_ARROW;
    menu->addChild(trackMenu);
}
```

---

## 6. 設計檢查清單

### 新模組設計時檢查：

**視覺設計：**
- [ ] 頂部標題使用橘色 (255, 200, 0)，12.f，bold=true
- [ ] 品牌標籤 "MADZINE" 使用橘色，10.f，bold=false
- [ ] Y=330 使用 WhiteBottomPanel
- [ ] 白色區域內標籤使用黑色文字
- [ ] 白色區域輸出標籤可選用粉紅色 (255, 133, 133)

**控制元件：**
- [ ] 主要參數使用 StandardBlackKnob (30px) 或 StandardBlackKnob26 (26px)
- [ ] CV 參數使用 WhiteKnob 系列
- [ ] 離散值參數使用 SnapKnob
- [ ] 端口標籤位置正確 (Y_label = Y_port - 23)

**程式碼：**
- [ ] 右鍵選單包含 addPanelThemeMenu()
- [ ] 實作 dataToJson/dataFromJson 儲存 panelTheme
- [ ] Widget 有 step() 呼叫 panelThemeHelper.step()
- [ ] 建構函式呼叫 panelThemeHelper.init()

**佈局：**
- [ ] 所有 I/O 都在白色區域內
- [ ] 控制元件都在暗色區域內
- [ ] 標籤都在控制元件上方（非下方）
- [ ] 間距符合標準 (38-43px 旋鈕垂直間距)

---

## 7. 數值速查表

### Y 座標參考
```
0-1:    模組標題起始
13:     品牌標籤起始
30:     主控制區起始 (典型)
330:    白色區域起始 (固定)
343:    I/O 第一行中心
368-373: I/O 第二行中心
380:    模組底部 (固定)
```

### X 座標參考 (4HP=60px)
```
15:     左側端口
30:     中心
45:     右側端口
```

### X 座標參考 (8HP=120px)
```
15:     最左
45:     中心偏左
60:     中心
75:     中心偏右
105:    最右
```

### X 座標參考 (12HP=180px)
```
30:     左側
90:     中心 (box.size.x / 2)
150:    右側
```

### 間距參考
```
標籤-控制垂直間距: 20-23px
旋鈕垂直間距: 38-43px (標準)
端口水平間距: 30px (8HP), 60px (12HP)
端口垂直間距: 25-30px (343 → 368-373)
模組標題與品牌標籤: 12px (Y=1 → Y=13)
```

---

## 8. 完整範例：典型 12HP 模組

```cpp
#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

struct MyModule : Module {
    enum ParamId {
        PARAM1_PARAM,
        PARAM2_PARAM,
        SPECIAL_TYPE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_L,
        INPUT_R,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    int panelTheme = 0;

    MyModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PARAM1_PARAM, 0.f, 1.f, 0.5f, "Parameter 1");
        configParam(PARAM2_PARAM, 0.f, 1.f, 0.5f, "Parameter 2");
        configSwitch(SPECIAL_TYPE_PARAM, 0.f, 3.f, 0.f, "Type",
            {"Type A", "Type B", "Type C", "Type D"});
        configInput(INPUT_L, "Left Input");
        configInput(INPUT_R, "Right Input");
        configOutput(OUTPUT, "Output");
    }

    void process(const ProcessArgs& args) override {
        // 處理邏輯...
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);
    }
};

struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    EnhancedTextLabel(Vec pos, Vec size, std::string text,
                      float fontSize = 12.f,
                      NVGcolor color = nvgRGB(255, 255, 255),
                      bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);

        if (bold) {
            float offset = 0.3f;
            nvgText(args.vg, box.size.x / 2.f - offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f + offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f - offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f + offset, text.c_str(), NULL);
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

struct WhiteBottomPanel : TransparentWidget {
    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 330, box.size.x, box.size.y - 330);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

struct MyModuleWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    MyModuleWidget(MyModule* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");

        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Add white background panel for bottom section
        WhiteBottomPanel* whitePanel = new WhiteBottomPanel();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // === 頂部標題 ===
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20),
            "My Module", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20),
            "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // === 主控制區 ===
        float centerX = box.size.x / 2;
        float yPos = 145;

        // 第一行：主要參數
        addChild(new EnhancedTextLabel(Vec(15, 122), Vec(30, 15),
            "PARAM1", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(30, yPos),
            module, MyModule::PARAM1_PARAM));

        addChild(new EnhancedTextLabel(Vec(centerX - 15, 122), Vec(30, 15),
            "PARAM2", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(centerX, yPos),
            module, MyModule::PARAM2_PARAM));

        yPos += 38;

        // 第二行：特殊功能 (橘色標籤)
        addChild(new EnhancedTextLabel(Vec(15, 160), Vec(30, 15),
            "SPECIAL", 7.f, nvgRGB(255, 200, 0), true));
        addParam(createParamCentered<SnapKnob>(Vec(30, yPos),
            module, MyModule::SPECIAL_TYPE_PARAM));

        // === 白色 I/O 區 ===
        // 第一行輸入/輸出
        addChild(new EnhancedTextLabel(Vec(15, 320), Vec(30, 15),
            "IN L", 7.f, nvgRGB(0, 0, 0), true));
        addInput(createInputCentered<PJ301MPort>(Vec(30, 343),
            module, MyModule::INPUT_L));

        addChild(new EnhancedTextLabel(Vec(centerX - 15, 320), Vec(30, 15),
            "IN R", 7.f, nvgRGB(0, 0, 0), true));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX, 343),
            module, MyModule::INPUT_R));

        addChild(new EnhancedTextLabel(Vec(box.size.x - 45, 320), Vec(30, 15),
            "OUT", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 30, 343),
            module, MyModule::OUTPUT));
    }

    void step() override {
        MyModule* module = dynamic_cast<MyModule*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        MyModule* module = dynamic_cast<MyModule*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelMyModule = createModel<MyModule, MyModuleWidget>("MyModule");
```

---

## 9. 總結

這份規範基於 MADZINE 系列模組的實際實作，提供了：

1. **一致的視覺語言**：橘色品牌色、白色 I/O 區、標準化標籤
2. **模組化的元件系統**：統一的旋鈕庫、標籤系統
3. **可預測的佈局**：固定的 Y 分區、標準間距
4. **完整的程式碼範例**：即學即用的實作模板

遵循這些規範可確保新模組與現有系列保持一致的外觀和使用體驗。

---

## 附錄：相關檔案

- `COMPONENT_Y_OFFSET_REFERENCE.txt` - 控制元件 Y 偏移數值參考
- `src/widgets/PanelTheme.hpp` - 主題切換系統
- `src/widgets/Knobs.hpp` - 自定義旋鈕定義
- 參考模組：Observer, QQ, NIGOQ, ChaosRecorder, MADDY, MADDYPlus
