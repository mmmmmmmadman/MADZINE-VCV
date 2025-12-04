# MADZINE VCV Rack 開發指南

版本：2.3.3
更新日期：2025-01-06

---

## 目錄

1. [編譯指令](#編譯指令)
2. [設計規範](#設計規範)
3. [開發工具](#開發工具)
4. [專案結構](#專案結構)

---

## 編譯指令

### 標準編譯流程

```bash
# 設定 Rack SDK 路徑
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK

# 清理舊的編譯檔案
make clean

# 編譯模組
make

# 安裝到 VCV Rack
make install
```

### 完整編譯與安裝（一行指令）

```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK && make clean && make && make install
```

### 只編譯不安裝

```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK && make
```

### 只重新編譯變更的檔案

```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK && make && make install
```

### 檢查編譯錯誤

```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK && make 2>&1 | grep -A5 "error:"
```

---

## 設計規範

### 使用 Layout Agent

設計新模組或檢查現有模組佈局時，使用 `/layout` Slash Command：

```bash
# 檢查現有模組佈局是否符合規範
/layout check ModuleName

# 設計新的 12HP 模組
/layout new 12HP MyNewModule

# 修正佈局問題
/layout fix ModuleName
```

### 設計規範文件

詳細規範請參考：`MADZINE_DESIGN_SPECIFICATION.md`

包含：
- 文字標籤系統（顏色、字體、位置）
- 旋鈕選擇決策樹
- Y=330 白色區塊規範
- 佈局模式參考
- 右鍵選單實作
- 完整程式碼範例

### 快速檢查清單

新模組設計時必須檢查：

- [ ] 頂部標題使用橘色 (255, 200, 0)，12.f，bold=true
- [ ] 品牌標籤 "MADZINE" 使用橘色，10.f，bold=false
- [ ] Y=330 使用 WhiteBottomPanel
- [ ] 白色區域內標籤使用黑色文字
- [ ] 主要參數使用 StandardBlackKnob26 (26px)
- [ ] CV 參數使用 WhiteKnob 系列
- [ ] 端口標籤位置正確 (Y_label = Y_port - 23)
- [ ] 右鍵選單包含 addPanelThemeMenu()
- [ ] 所有 I/O 都在白色區域內

---

## 開發工具

### 檔案結構

```
MADZINE/
├── src/                          # 原始碼
│   ├── plugin.hpp                # 主要標頭檔
│   ├── plugin.cpp                # 模組註冊
│   ├── widgets/                  # 自定義 Widget
│   │   ├── Knobs.hpp            # 旋鈕定義
│   │   └── PanelTheme.hpp       # 主題系統
│   └── [ModuleName].cpp          # 各模組實作
├── res/                          # 資源檔
│   └── [ModuleName].svg          # 面板 SVG
├── plugin.json                   # 模組清單
├── Makefile                      # 編譯設定
├── MADZINE_DESIGN_SPECIFICATION.md  # 設計規範
├── COMPONENT_Y_OFFSET_REFERENCE.txt # Y 偏移參考
└── .claude/commands/layout.md    # Layout Agent 指令
```

### 新增模組步驟

1. **建立模組檔案**
   ```bash
   # 複製範本或從頭開始
   cp src/ExistingModule.cpp src/NewModule.cpp
   ```

2. **使用 Layout Agent 設計佈局**
   ```
   /layout new 12HP NewModule
   ```

3. **建立 SVG 面板**
   ```bash
   # 複製現有面板作為範本
   cp res/ExistingModule.svg res/NewModule.svg
   cp res/ExistingModule_W.svg res/NewModule_W.svg
   ```

4. **註冊模組**

   在 `src/plugin.hpp` 加入：
   ```cpp
   extern Model* modelNewModule;
   ```

   在 `src/plugin.cpp` 加入：
   ```cpp
   p->addModel(modelNewModule);
   ```

5. **更新 plugin.json**
   ```json
   {
     "slug": "NewModule",
     "name": "New Module",
     "description": "Module description",
     "tags": ["Tag1", "Tag2"]
   }
   ```

6. **編譯測試**
   ```bash
   export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK && make && make install
   ```

7. **佈局檢查**
   ```
   /layout check NewModule
   ```

### 常見問題排除

#### 編譯錯誤：找不到 rack.hpp

```bash
# 確認 RACK_DIR 路徑正確
echo $RACK_DIR
# 應該輸出：/Users/madzine/Documents/VCV-Dev/Rack-SDK

# 重新設定環境變數
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK
```

#### 編譯錯誤：undeclared identifier

- 檢查 `#include` 是否正確
- 檢查 namespace 是否正確
- 參考其他正常運作的模組

#### 佈局問題：標籤被遮住

```
/layout check YourModule
```

會自動檢查並修正標籤位置。

#### 面板主題不切換

確認實作了：
```cpp
// Module 類別
int panelTheme = 0;

json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    json_t* themeJ = json_object_get(rootJ, "panelTheme");
    if (themeJ) panelTheme = json_integer_value(themeJ);
}

// Widget 類別
PanelThemeHelper panelThemeHelper;

MyModuleWidget(MyModule* module) {
    // ...
    panelThemeHelper.init(this, "ModuleName");
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
```

---

## 專案結構

### 依賴函式庫

- **Rack SDK**: VCV Rack 核心 API
- **SST Filters**: Surge XT 濾波器庫（用於 oversampling）
- **SST Basic Blocks**: Surge XT 基礎元件

### 自定義元件

#### 旋鈕系列
- `StandardBlackKnob` (30×30)
- `StandardBlackKnob26` (26×26)
- `WhiteKnob` (30×30)
- `MediumGrayKnob` (26×26)
- `SmallGrayKnob` (21×21)
- `SnapKnob` (26×26)
- `MicrotuneKnob` (20×20)

#### Widget 元件
- `EnhancedTextLabel` - 自定義文字標籤（支援 bold）
- `WhiteBottomPanel` - Y=330 白色背景
- `PanelThemeHelper` - 主題切換系統

### 編譯設定

#### Makefile 重點

```makefile
# 編譯器標準
CXXFLAGS += -std=c++20

# Include 路徑
CXXFLAGS += -Isst-filters/include -Isst-basic-blocks/include

# 優化等級
CXXFLAGS += -O3 -funsafe-math-optimizations
```

---

## 版本控制

### Git 工作流程

```bash
# 查看變更
git status

# 加入新模組
git add src/NewModule.cpp res/NewModule*.svg plugin.json

# 提交
git commit -m "Add NewModule: description"

# 推送
git push
```

### 版本號規則

- `2.3.1` = Major.Minor.Patch
- Major: 重大功能改變
- Minor: 新增模組或功能
- Patch: Bug 修正、小改進

更新版本號時需要修改：
- `plugin.json` 中的 `"version"` 欄位

---

## 效能優化

### Oversampling

新的音訊處理模組應該使用 2x oversampling：

```cpp
#include <sst/filters/HalfRateFilter.h>

// Module 類別中
static constexpr int BLOCK_SIZE = 8;
int oversampleRate = 2;
sst::filters::HalfRate::HalfRateFilter downFilterL{6, true};
sst::filters::HalfRate::HalfRateFilter downFilterR{6, true};

float outputBufferL[BLOCK_SIZE * 2];
float outputBufferR[BLOCK_SIZE * 2];
```

參考實作：`ChaosRecorder.cpp`, `NIGOQ.cpp`

---

## 測試

### 手動測試檢查清單

- [ ] 所有旋鈕功能正常
- [ ] 所有輸入/輸出工作正常
- [ ] CV 輸入響應正確
- [ ] 右鍵選單可開啟
- [ ] 主題切換正常
- [ ] 參數保存/載入正常
- [ ] 音訊沒有爆音或失真
- [ ] CPU 使用率合理

### 音訊測試

- 測試極端參數值
- 測試高頻輸入 (20kHz)
- 測試 DC 偏移
- 測試靜音輸入
- 長時間運行穩定性

---

## 發佈流程

1. **完成開發**
   - 所有功能測試通過
   - 佈局符合設計規範 (`/layout check`)
   - 程式碼清理完成

2. **更新文件**
   - 更新 `plugin.json` 描述
   - 更新版本號
   - 提交 Git

3. **編譯發佈版本**
   ```bash
   export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK
   make clean
   make dist
   ```

4. **測試安裝包**
   ```bash
   make install
   # 在 VCV Rack 中測試所有功能
   ```

5. **發佈到 VCV Library**
   - 推送到 GitHub
   - 等待自動建置
   - 檢查 VCV Library 狀態

---

## 參考資料

### 內部文件
- `MADZINE_DESIGN_SPECIFICATION.md` - 完整設計規範
- `COMPONENT_Y_OFFSET_REFERENCE.txt` - Y 偏移數值表
- `.claude/commands/layout.md` - Layout Agent 說明

### 外部資源
- [VCV Rack 官方文件](https://vcvrack.com/manual/)
- [VCV Rack Plugin 開發教學](https://vcvrack.com/manual/PluginDevelopmentTutorial)
- [Rack SDK API](https://vcvrack.com/docs/)

---

## 聯絡資訊

- GitHub: https://github.com/mmmmmmmadman/MADZINE-VCV
- Email: madzinetw@gmail.com
- Patreon: https://www.patreon.com/c/madzinetw

---

**最後更新**: 2025-01-06
**維護者**: MAD (MADZINE)
