# Microtuning 開發日志

## 2024-12-18

### 研究階段開始

- 建立 `src/Microtuning/` 研究資料夾
- 完成 `Microtuning_Research_Paper.md` 初版

### 重大更新：日本音階研究修正

根據上原六四郎（1895）和小泉文夫的理論，日本音階確實有上行/下行差異：

**都節音階（Miyako-bushi）**：
- 下行形：E - F - A - B - **C** - E
- 上行形：E - F - A - B - **D** - E
- 短6度(C)在上行時變成短7度(D)

**琉球音階**：
- 下行時加入經過音「レ」(D)
- 「てぃんさぐぬ花」等樂曲可見此特徵

### 主要發現（修正版）

1. **需要兩個八度/上行下行區分的系統**：
   - Arabic Maqam（上下八度音程可能不同）
   - Turkish Makam（浮動音符 - 上行/下行差異）
   - Persian Dastgah（類似 Turkish）
   - Indian Raga（Aroha/Avaroha 完全不同）
   - **Japanese 都節/律音階**（上行時音符改變）
   - **Ryukyu 琉球音階**（下行時加入經過音）

2. **不需要兩個八度的系統**：
   - 西方系統（12-TET, Just, Pythagorean）
   - 日本 陽音階/民謠音階（上行性，較穩定）
   - 中國音階
   - 甘美朗（Gamelan）
   - 泰國 7-TET

### 待辦事項

- [ ] 收集更多 Maqam/Makam 的精確音程數據
- [ ] 研究印度 22 Shruti 的實際演奏慣例
- [ ] 設計 UI 方案（如何在有限空間顯示 24 個微分音）
- [ ] 決定是否需要音高方向追蹤算法
- [ ] 建立預設庫的完整數據

### 技術決策

**建議方案**：混合方案
- 保持現有 12 個微分音參數作為基礎
- 新增「Two-Octave Mode」開關
- 新增「Ascending/Descending Mode」開關
- 預設可自動配置需要的模式

---

## 參考連結

- [Maqam World](https://www.maqamworld.com/)
- [TAQS.IM](https://taqs.im/scales/)
- [Scala Archive](http://www.huygens-fokker.org/scala/)
