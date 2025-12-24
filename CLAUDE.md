# MADZINE VCV Rack 開發指南

版本：2.3.7
更新日期：2025-12-24

---

## 編譯指令

### 標準流程
```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK
make clean && make && make install
```

### 常用指令
```bash
# 只編譯（不安裝）
make

# 增量編譯並安裝
make && make install

# 檢查錯誤
make 2>&1 | grep -A5 "error:"

# 發佈版本
make dist
```

---

## 工作資料夾

- **開發**: `/Users/madzine/Documents/VCV-Dev/MADZINE`
- **上傳**: `/Users/madzine/Documents/Github/VCV-Dev/MADZINE`

完成修改後需同步至上傳資料夾。

---

## 專案結構

```
MADZINE/
├── src/
│   ├── plugin.hpp              # 主要標頭檔
│   ├── plugin.cpp              # 模組註冊
│   ├── widgets/
│   │   ├── KnobBase.hpp        # 旋鈕基類
│   │   ├── Knobs.hpp           # 旋鈕定義
│   │   └── PanelTheme.hpp      # 主題系統
│   └── [ModuleName].cpp        # 各模組實作
├── res/                        # SVG 面板
├── plugin.json                 # 模組清單
├── Makefile
└── MADZINE_DESIGN_SPECIFICATION.md
```

---

## 新增模組

1. **建立模組檔案**
   ```bash
   cp src/ExistingModule.cpp src/NewModule.cpp
   ```

2. **註冊模組**
   - `src/plugin.hpp`: `extern Model* modelNewModule;`
   - `src/plugin.cpp`: `p->addModel(modelNewModule);`

3. **更新 plugin.json**
   ```json
   {
     "slug": "NewModule",
     "name": "New Module",
     "description": "描述",
     "tags": ["Tag1"]
   }
   ```

4. **建立 SVG 面板**
   ```bash
   cp res/ExistingModule.svg res/NewModule.svg
   ```

5. **編譯測試**
   ```bash
   make && make install
   ```

---

## 常見問題

### 找不到 rack.hpp
```bash
export RACK_DIR=~/Documents/VCV-Dev/Rack-SDK
```

### 面板主題不切換
確認實作：
- Module: `panelTheme` 變數、`dataToJson/dataFromJson`
- Widget: `PanelThemeHelper`、`step()`、`appendContextMenu`

### 旋鈕雙擊歸零到錯誤值
參考 `MADZINE_DESIGN_SPECIFICATION.md` 第 3 節「自定義 defaultValue」

---

## 依賴函式庫

- **Rack SDK**: VCV Rack 核心 API
- **SST Filters**: Surge XT 濾波器（oversampling）
- **SST Basic Blocks**: Surge XT 基礎元件

---

## 版本號規則

`Major.Minor.Patch` (例: 2.3.6)
- Major: 重大功能改變
- Minor: 新增模組或功能
- Patch: Bug 修正

更新位置：`plugin.json` 的 `"version"` 欄位

---

## 發佈流程

1. 測試所有功能
2. 更新 `plugin.json` 版本號
3. `make dist`
4. 推送到 GitHub
5. 等待 VCV Library 自動建置

---

## 參考資料

### 內部文件
- `MADZINE_DESIGN_SPECIFICATION.md` - 設計規範（含旋鈕、標籤、佈局）

### 外部資源
- [VCV Rack 官方文件](https://vcvrack.com/manual/)
- [Plugin 開發教學](https://vcvrack.com/manual/PluginDevelopmentTutorial)

---

## 聯絡資訊

- GitHub: https://github.com/mmmmmmmadman/MADZINE-VCV
- Email: madzinetw@gmail.com

---

## 已知問題與注意事項

### Manual HTML 點擊功能消失問題

**問題描述**：修改 `madzine_modules_compact_v2.html` 時，點擊小卡開啟大卡的功能會因 JavaScript 語法錯誤而失效。

**根本原因**：
1. 刪除 HTML 元素後，對應的 JavaScript 仍引用該元素，導致 `null` 錯誤
2. 刪除程式碼區塊時，可能遺漏函數的開頭或結尾大括號 `{}`
3. 翻譯字串中的多行內容必須使用 `\n` 而非實際換行

**預防措施**：
- 修改 HTML 結構後，必須同步修改對應的 JavaScript
- 每次修改後執行 JavaScript 語法驗證：
  ```bash
  awk 'NR>=3122 && NR<=3887' madzine_modules_compact_v2.html > /tmp/js.js && node --check /tmp/js.js
  ```
- 翻譯字串必須保持單行格式

### Manual HTML 雙卡片上下層切換 CSS 問題

**問題描述**：Manual（紅色）與 Quick Reference（藍色）雙卡片切換時，底層卡片的大小和 hover 效果不一致。

**根本原因**：
1. **CSS 特異性問題**：`.card-stack.has-quickref .stacked-card.card-front` (0,0,4,0) 高於 `.stacked-card.card-front.inactive` (0,0,3,0)，導致樣式被覆蓋
2. **transform 屬性互斥**：hover 時如果只設定 `translate()` 而缺少 `scale()`，會覆蓋基本狀態的縮放，造成卡片突然變大

**解決方案**：
1. 紅色卡片（card-front）在底層時：
   - 基本狀態：`transform: scale(0.92) translate(3px, 3px)`
   - hover 狀態：`transform: scale(0.92) translate(25px, 25px)`
   - 必須使用 `.card-stack.has-quickref .stacked-card.card-front.inactive` 選擇器以提高特異性

2. 藍色卡片（card-back）在底層時：
   - 基本狀態：`transform: translate(5px, 5px)`
   - hover 狀態：`transform: translate(15px, 15px)`

**注意事項**：
- 修改 hover 的 transform 時，必須保留基本狀態的所有 transform 函數（scale、translate 等）
- 兩張卡片的 hover 位移量可以不同，視覺上紅色（25px）比藍色（15px）大是正常的

### Manual 版本號更新

**重要**：每次修改 Manual 內容後，必須更新版本號以避免覆蓋舊版本。

**更新位置**：`Manual/build_manual.py` 檔案末尾
```python
with open('madzine_modules_compact_vX.X.html', 'w', encoding='utf-8') as f:
    ...
print("已生成: madzine_modules_compact_vX.X.html")
```

**版本號規則**：
- 格式：`vMajor.Minor`（例：v3.8）
- Minor 版本：每次內容修改時遞增
- Major 版本：重大結構改變時遞增

### UniversalRhythm 陣列越界崩潰問題（v2.3.7 修復）

**問題描述**：VCV Rack 啟動後約 4 秒崩潰，錯誤類型為 `EXC_BAD_ACCESS (SIGBUS)` / `KERN_PROTECTION_FAILURE`。

**崩潰堆疊**：
```
Thread 12 Crashed:
0  plugin.dylib  UniversalRhythm::process(...) + 676
1  plugin.dylib  UniversalRhythm::process(...) + 656
2  libRack.dylib rack::engine::Module::doProcess(...) + 156
```

**根本原因**：
1. **`accents[useStep]` 直接索引越界**：當 `fillActive = true` 時，`useStep` 可能超出 `accents` 向量的實際大小
2. **`fillStep` 計算使用錯誤的 pattern 長度**：使用固定的 `fillPatterns.patterns[0].length` 而非當前 role 的 pattern 長度

**錯誤程式碼**：
```cpp
// 問題 1：直接索引可能越界
bool accent = primaryPattern.accents[useStep];

// 問題 2：使用固定 patterns[0] 長度
int fillStep = fillActive ? (static_cast<int>(fillPatterns.patterns[0].length) - fillStepsRemaining) : step;
```

**修復方案**：
```cpp
// 修復 1：使用安全索引
bool accent = primaryPattern.accents[useStep % primaryPattern.length];

// 修復 2：使用對應 role 的 pattern 長度 + 邊界檢查
int fillPatternLen = static_cast<int>(fillPatterns.patterns[voiceBase].length);
int fillStep = fillActive ? (fillPatternLen - fillStepsRemaining) : step;
if (fillActive && fillStep < 0) fillStep = 0;
if (fillActive && fillStep >= fillPatternLen) fillStep = fillPatternLen - 1;
```

**預防措施**：
- 訪問 `std::vector` 時，若索引來自外部計算，務必使用 `% length` 或邊界檢查
- 當有多個不同長度的 pattern 時，確保使用正確的 pattern 長度進行索引計算
- Fill pattern 的長度可能與正常 pattern 不同，需特別注意

