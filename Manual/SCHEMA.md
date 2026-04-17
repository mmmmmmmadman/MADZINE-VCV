# Manual YAML Schema

單一真相來源。每個模組一個 `modules/{slug}.yaml`，取代原本的 TSV + JSON 雙軌。

## 檔案位置

```
Manual/
  modules/
    WeiiiDocumenta.yaml      # 新格式（slug 為檔名，一目了然）
    ...
  generate.py                # YAML → hpp + HTML + website HTML
  template/
    manual.html.j2           # HTML template（OKLCH 配色）
```

## Schema

```yaml
slug: WeiiiDocumenta            # 與 plugin.json slug 一致
name: weiii documenta           # 顯示名稱
hp: 12                          # HP 寬度（數字）
wide: false                     # 是否寬卡
alt: "weiii documenta - 12HP sampler"  # <img alt>

description:                    # 短描述（卡片 + 模組簡介）
  en: "Recording sampler..."
  zh: "錄音 Sampler..."
  ja: "ノーインプット..."

features:                       # 卡片 feature list
  en:
    - "Stereo recording sampler with 60s buffer"
    - "Real-time transient slicing"
  zh: [...]
  ja: [...]

manual:                         # 長 HTML 說明（HTML manual 頁用）
  en:
    - heading: "Overview"
      html: |
        <p>Recording sampler with chaotic...</p>
    - heading: "Recording and Slicing"
      list:
        - "<b>REC</b>: Toggle recording..."
        - "<b>Transient Detection</b>: Triggers..."
  zh: [...]
  ja: [...]

controls:                       # 參數/輸入/輸出（hover tooltip 用）
  - name: "Slice Scan"          # 與 configParam 的 name 完全一致
    type: param                 # param | input | output
    desc:
      en: "Scan position through recorded slices (0-100%). CV modulatable."
      zh: "Slice 掃描位置（0-100%）。可 CV 調變。"
      ja: "スライススキャン位置（0-100%）。CVモジュレート可能。"
  - name: "Audio L"
    type: input
    desc: {...}
```

## Generator 輸出

### 1. `src/ManualHelpData.hpp`（VCV tooltip）

從 `description` + `controls` 生成。格式不變，與現有消費端 `Manual.cpp` 相容。

### 2. `Manual/madzine_modules_compact_v{N}.html`（VCV 內附 HTML 說明書）

從 `name/hp/description/features/manual` 生成。用 OKLCH 低彩度紫主題。

### 3. `madzine-website/modules.html`（網站）

同樣 template，輸出到網站資料夾。單一資料源，避免分岔。

## 配色（OKLCH）

```
主色     oklch(0.65 0.06 295)   低彩度紫
背景 1   oklch(0.22 0.005 265)
背景 2   oklch(0.20 0.005 265)
邊框     oklch(0.30 0.008 265)
文字主   oklch(0.93 0.01 80)
文字副   oklch(0.83 0.015 80)
```

禁止 `#ff6b9d` / `#FFC800` 等直接鮮豔色。

## 三語規則

- `zh` 為主（繁體中文，使用者維護）
- `en` / `ja` 由 Claude Code 從 zh 翻譯
- 空欄位 fallback 到 zh
- 專有名詞保留英文：Euclidean、Clock、CV、Envelope、Pattern
- 數學公式用自然語言描述，不嵌 `<code>` 公式

## 工作流程

```bash
# 編輯 YAML
vim Manual/modules/WeiiiDocumenta.yaml

# 生成全部輸出
python3 Manual/generate.py

# 驗證
python3 Manual/verify.py

# 編譯 + 安裝
make && make install
```

## Migration

一次性：`migrate_to_yaml.py`
- 讀 `src/ManualHelpData.hpp`（controls 來源）
- 讀 `Manual/modules/*.json`（manual HTML 來源）
- 合併輸出到 `Manual/modules/*.yaml`
- 舊 TSV/JSON 改到 `Manual/_archive/`

## 優先順序

1. Phase C：schema + generator + migration + OKLCH template
2. Phase B：按空白嚴重度分批 agent 審計補齊 YAML 內容
