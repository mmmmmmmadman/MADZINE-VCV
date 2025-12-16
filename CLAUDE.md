# MADZINE VCV Rack 開發指南

版本：2.3.6
更新日期：2025-12-14

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
