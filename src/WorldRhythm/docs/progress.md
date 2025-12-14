# Universal Rhythm - 開發進度

## 專案狀態

**開始日期**: 2024-11-27
**最後更新**: 2025-12-14
**目前階段**: 已完成核心功能，持續優化中

---

## 已完成功能

### 核心引擎
- [x] 10 種風格的權重矩陣
- [x] 加權選擇演算法
- [x] Pattern 儲存與管理
- [x] 4 角色 × 2 聲部架構

### 互鎖系統
- [x] Timeline → Foundation 閃避邏輯
- [x] Groove 補位邏輯
- [x] Lead 協商邏輯
- [x] 風格相容性矩陣

### 人性化處理
- [x] 時序微偏移（風格特異性）
- [x] BPM 相依 Swing
- [x] Ghost Notes 生成
- [x] Accent 增強（單向增加，優先強拍）

### Articulation Profile 系統
- [x] ArticulationProfiles.hpp 建立
- [x] 10 風格 × 4 角色對照表
- [x] Flam / Drag / Ruff / Buzz 技法實作
- [x] 單一旋鈕機率控制
- [x] 角色獨立風格讀取

### 合成器與音訊
- [x] 8 聲部內建合成器
- [x] 外部音訊輸入 VCA 處理
- [x] Mix 參數（內建/外部混合）
- [x] Mix L/R 輸出
- [x] Spread 立體聲擺位（Role-based panning）

### UI 介面
- [x] 40HP 面板設計
- [x] Pattern 視覺化顯示
- [x] 4 角色區塊佈局
- [x] 全域控制區

### 輸入/輸出
- [x] Clock / Reset / Regen 處理
- [x] CV 輸入（Freq, Decay, Style, Density）
- [x] 8 Audio + 8 Gate + 8 CV + 8 Accent 輸出
- [x] PPQN 選擇（右鍵選單）

---

## 近期變更（2025-12）

### 新功能
- [x] Spread 立體聲擺位功能
  - Foundation 置中（低頻規則）
  - Timeline 略右（+0.20, +0.25）
  - Groove 左右分離（-0.30, +0.30）
  - Lead 略左（-0.40, -0.50）
  - 基於混音研究的擺位比例

### UI 調整
- [x] 上排旋鈕改為 MediumGrayKnob
- [x] FILL/Articulation/GHOST/ACCENT/SPREAD 標籤改白色
- [x] 控制區間距調整優化

---

## 歷史變更（2024-12）

### 參數調整
- [x] 刪除 Groove Template 參數（內建 Auto）
- [x] Accent 改為單向控制（只增加不減少）
- [x] Articulation 從 0-7 段改為 0-1 連續

### Bug 修復
- [x] MIX 標籤對齊修復
- [x] REST 功能修復（originalPatterns 儲存）
- [x] Articulation 角色獨立風格讀取

### 文件更新
- [x] HTML 文件更新（TW/EN/JP）
- [x] Algorithm Paper 更新
- [x] 開發文件更新

---

## 待完成

### 優化
- [ ] 權重表根據聽感微調
- [ ] 更多風格變體

---

## 開發筆記

### Widget draw() 安全檢查（2025-12-14）

VCV Rack 的 widget `draw()` 可能在模組完全初始化前被呼叫，導致崩潰。

**必須加入的檢查：**
```cpp
void draw(const DrawArgs& args) override {
    if (!module) return;
    if (module->params.empty()) return;
    // 存取前檢查索引邊界
    if (paramIdx >= static_cast<int>(module->params.size())) return;
}
```

---

## 檔案狀態

| 檔案 | 說明 | 狀態 |
|------|------|------|
| UniversalRhythm.cpp | 模組主程式 | 完成 |
| WorldRhythm.hpp | 核心引擎 | 完成 |
| ArticulationProfiles.hpp | Articulation 對照表 | 完成 |
| WorldRhythm_TW.html | 繁體中文文件 | 完成 |
| WorldRhythm_EN.html | 英文文件 | 完成 |
| WorldRhythm_JP.html | 日文文件 | 完成 |
| WorldRhythm_Algorithm_Paper.md | 演算法論文 | 完成 |
