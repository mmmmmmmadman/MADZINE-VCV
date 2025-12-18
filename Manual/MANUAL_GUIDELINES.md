# MADZINE Manual 製作準則

## 語言風格

- **平實、冷調**：使用客觀事實性語言，避免華麗或情感性用詞
- **注重事實**：記載參數值、範圍、行為，不加修飾
- **去除數學公式**：不使用 `<code>` 標籤顯示計算公式，改用平實語言描述行為和效果

## 術語規範

### 避免使用
- 「調色盤」→ 改用「預設值」或「選項」
- 「魔法般」「神奇」等修飾詞
- 數學公式（如 `output = sin(phase) × amplitude`）

### 正確用法
- 「K1-K5 定義五個預設電壓值」（非「音符調色盤」）
- 「將 Hit 均勻分佈於 Pattern 長度」（非「floor(i × length / fill)」）
- 「Envelope 衰減速度隨力度增加」（非「decay = baseDecay × velocity^1.5」）

## 三語一致性

- 英文 (manual)、中文 (manual_zh)、日文 (manual_ja) 內容結構必須對應
- 同一概念在三語中使用對等的描述深度
- 專有名詞保持英文：Euclidean、Clock、CV、Envelope、Pattern 等

## 內容結構

1. **Overview / 概述**：模組用途與主要功能
2. **Signal Flow / 訊號流程**：輸入到輸出的處理路徑
3. **Controls / 控制**：各旋鈕、按鈕的功能與範圍
4. **I/O**：輸入輸出端口說明
5. **Technical Specifications / 技術規格**：電壓範圍、處理精度等

## 版本紀錄

- 2025-12-17：建立準則文件，確立平實語言風格與去除公式原則
