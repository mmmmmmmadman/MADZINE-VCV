# Manual JSON 審計修正指南

## 目標

每個 MADZINE 模組都有一個 Manual JSON 檔案，用於 Manual 模組的 hover 即時說明。這些 JSON 的內容必須與 C++ 原始碼完全一致。上一輪審計已修正大部分問題，但仍有遺漏（例如 MADDYPlus 的旋鈕描述不完整）。

## 檔案位置

| 類型 | 路徑 |
|------|------|
| C++ 原始碼 | `src/[ModuleName].cpp` |
| Manual JSON | `Manual/modules/[slug].json` |
| 自動生成的 Help 資料 | `src/ManualHelpData.hpp` |
| 生成腳本 | `Manual/generate_help_data.py` |
| Manual 模組本體 | `src/Manual.cpp` |

## JSON 檔案結構

每個 JSON 檔案包含六個欄位，三語言成對：

```json
{
  "description": "英文模組簡介",
  "description_zh": "中文模組簡介",
  "description_ja": "日文模組簡介",
  "manual": "英文詳細說明（HTML 格式）",
  "manual_zh": "中文詳細說明（HTML 格式）",
  "manual_ja": "日文詳細說明（HTML 格式）"
}
```

### manual 欄位格式

HTML 字串，用 `<br>` 換行。參數說明格式固定為：

```
<b>PARAM_NAME</b>: 說明文字<br>
```

Manual 模組透過解析 `<b>` 標籤取得參數名稱，再與 hover 偵測到的 `paramQuantity->name` 比對。

## 審計方法

### 步驟 1：讀取 C++ 原始碼

找到模組的所有 `configParam`、`configInput`、`configOutput` 呼叫，記錄：

- **參數名稱**（第二個字串參數，即 tooltip 顯示名稱）
- **範圍**（min, max, default）
- **顯示轉換**（displayBase, displayMultiplier, displayOffset）
- **單位**（unit 字串）

範例：
```cpp
configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "Frequency", " Hz", 2.f, 100.f, 0.f);
// name="Frequency", min=0, max=1, default=0.5
// displayBase=2, displayMultiplier=100, displayOffset=0
// 顯示值 = 100 * 2^param = 100 * 2^0.5 = ~141 Hz (default)
```

### 步驟 2：讀取 process() 邏輯

確認每個參數的實際行為：
- CV 調變方式（加法、乘法、指數）
- 信號流路徑
- 特殊條件（閾值、clamp、模式切換）
- 輸出電壓範圍

### 步驟 3：比對 JSON

逐一比對每個 `<b>NAME</b>` 區段，確認：

1. 參數名稱與 `configParam` 的 name 一致
2. 範圍、預設值正確
3. CV 行為描述正確
4. 特殊功能有記錄
5. 三語言內容一致（不是翻譯品質，而是技術資訊一致）

### 步驟 4：補齊遺漏

如果 C++ 有 `configParam` 但 JSON 沒有對應的 `<b>NAME</b>` 區段，需要新增。

## 已知待修正模組

以下模組在上一輪審計中可能修正不完整，需要重新檢查：

- **MADDYPlus** (`maddy_.json`) - 大量旋鈕缺少說明
- 建議對所有 37 個模組重新逐一確認

## 完整模組清單與對應檔案

| 模組 Slug | C++ 檔案 | JSON 檔案 |
|-----------|----------|-----------|
| SwingLFO | SwingLFO.cpp | swing_lfo.json |
| EuclideanRhythm | EuclideanRhythm.cpp | euclidean_rhythm.json |
| ADGenerator | ADGenerator.cpp | ad_generator.json |
| Pinpple | Pinpple.cpp | pinpple.json |
| PPaTTTerning | PPaTTTerning.cpp | ppattterning.json |
| MADDY | MADDY.cpp | maddy.json |
| TWNC | TWNC.cpp | twnc.json |
| TWNCLight | TWNCLight.cpp | twnc_light.json |
| TWNC2 | TWNC2.cpp | twnc_2.json |
| QQ | QQ.cpp | qq.json |
| Observer | Observer.cpp | observer.json |
| U8 | U8.cpp | u8.json |
| YAMANOTE | YAMANOTE.cpp | yamanote.json |
| KIMO | KIMO.cpp | kimo.json |
| Obserfour | Obserfour.cpp | obserfour.json |
| Pyramid | Pyramid.cpp | pyramid.json |
| DECAPyramid | DECAPyramid.cpp | decapyramid.json |
| KEN | KEN.cpp | ken.json |
| Quantizer | Quantizer.cpp | quantizer.json |
| EllenRipley | EllenRipley.cpp | ellen_ripley.json |
| MADDYPlus | MADDYPlus.cpp | maddy_.json |
| NIGOQ | NIGOQ.cpp | nigoq.json |
| Runshow | Runshow.cpp | runshow.json |
| EnvVCA6 | EnvVCA6.cpp | env_vca_6.json |
| WeiiiDocumenta | weiiidocumenta.cpp | weiii_documenta.json |
| UniversalRhythm | UniversalRhythm.cpp | universal_rhythm.json |
| UniRhythm | UniRhythm.cpp | uni_rhythm.json |
| SongMode | SongMode.cpp | song_mode.json |
| Launchpad | Launchpad.cpp | launchpad.json |
| Runner | Runner.cpp | runner.json |
| Facehugger | Facehugger.cpp | facehugger.json |
| Ovomorph | Ovomorph.cpp | ovomorph.json |
| ALEXANDERPLATZ | ALEXANDERPLATZ.cpp | alexanderplatz.json |
| SHINJUKU | SHINJUKU.cpp | shinjuku.json |
| Portal | Portal.cpp | portal.json |
| Drummmmmmer | Drummmmmmer.cpp | drummmmmmer.json |
| theKICK | theKICK.cpp | thekick.json |

## 修正後的流程

### 1. 修改 JSON 後重新生成 Help 資料

```bash
cd /Users/madzine/Documents/OpenSource/MADZINE-VCV
python3 Manual/generate_help_data.py
```

輸出應顯示 `37 modules, XXXX entries`。

### 2. 編譯與安裝

```bash
export RACK_DIR=~/Documents/Tools/Rack-SDK
make -j$(sysctl -n hw.ncpu) && make install
```

注意：RACK_DIR 路徑是 `~/Documents/Tools/Rack-SDK`，不是 CLAUDE.md 裡寫的 `~/Documents/VCV-Dev/Rack-SDK`。

### 3. 測試

開啟 VCV Rack，加入 Manual 模組，將游標移到目標模組的各個旋鈕和 Port 上，確認顯示的說明文字正確。

## 給 Claude 的工作指令範本

在新對話中可以使用以下指令：

```
請審計並修正 MADDYPlus 模組的 Manual JSON。

方法：
1. 讀取 src/MADDYPlus.cpp，找出所有 configParam/configInput/configOutput
2. 讀取 Manual/modules/maddy_.json
3. 逐一比對，找出遺漏或錯誤的參數說明
4. 修正 JSON（三語言同步）
5. 執行 python3 Manual/generate_help_data.py
6. 編譯測試：export RACK_DIR=~/Documents/Tools/Rack-SDK && make -j$(sysctl -n hw.ncpu) && make install

參考 Manual/AUDIT_GUIDE.md 了解檔案結構和審計方法。
```

如果要批次處理多個模組，可以使用背景 Agent 並行處理，每個 Agent 負責 3-4 個模組。
