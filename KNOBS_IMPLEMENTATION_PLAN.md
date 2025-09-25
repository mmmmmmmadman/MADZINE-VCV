# MADZINE VCV Rack 旋鈕重構計畫
## Knobs HPP Implementation Plan

**版本**: 2.3.0
**日期**: 2025-01-22
**作者**: MADZINE

---

## 📋 執行摘要

本計畫旨在將 MADZINE VCV Rack 插件中的 22 種自定義旋鈕類別重構為統一的 header 檔案架構，解決 undo/redo 功能問題，並提升程式碼的可維護性。

### 🎯 主要目標
1. ✅ 為所有自定義旋鈕添加完整的 undo/redo 支援
2. ✅ 減少 80% 的重複程式碼
3. ✅ 保持現有的使用體驗和外觀不變
4. ✅ 建立可擴展的旋鈕類別架構

---

## 📊 現況分析

### 旋鈕類型統計

| 類別 | 旋鈕類型 | 使用模組數 | 尺寸 | 優先級 |
|------|----------|------------|------|--------|
| **核心旋鈕** | StandardBlackKnob | 11 | 38×38px | P1 |
| | TechnoStandardBlackKnob | 4 | 45×45px | P1 |
| | HiddenTimeKnob | 4 | 隱藏 | P1 |
| | SmallGrayKnob | 3 | 21×21px | P1 |
| **Snap旋鈕** | TechnoSnapKnob | 2 | 變動 | P2 |
| | SnapKnob | 1 | 26×26px | P2 |
| | MADDYSnapKnob | 1 | 變動 | P2 |
| | TWNCLightSnapKnob | 1 | 變動 | P2 |
| **特殊旋鈕** | SmallWhiteKnob | 2 | 28×28px | P3 |
| | WhiteKnob | 2 | 變動 | P3 |
| | MediumGrayKnob | 2 | 變動 | P3 |
| | RandomizedKnob | 1 | 特殊 | P3 |
| | MicrotuneKnob | 1 | 20×20px | P3 |

### 受影響模組列表
- **ADGenerator**: StandardBlackKnob
- **DECAPyramid**: StandardBlackKnob
- **EllenRipley**: StandardBlackKnob
- **EuclideanRhythm**: StandardBlackKnob, SnapKnob
- **KIMO**: TechnoStandardBlackKnob, TechnoSnapKnob
- **MADDY/MADDYPlus**: 多種自定義旋鈕
- **NIGOQ**: LargeWhiteKnob, SmallWhiteKnob, SmallGrayKnob, HiddenTimeKnob
- **Observer/Obserfour**: HiddenTimeKnob
- **PPaTTTerning**: StandardBlackKnob
- **Pinpple**: RandomizedKnob
- **Pyramid**: StandardBlackKnob
- **QQ**: StandardBlackKnob, HiddenTimeKnob, HiddenAttenuatorKnob
- **Quantizer**: StandardBlackKnob, MicrotuneKnob
- **SwingLFO**: StandardBlackKnob
- **TWNC/TWNC2/TWNCLight**: Techno系列旋鈕
- **U8**: TechnoStandardBlackKnob
- **YAMANOTE**: StandardBlackKnob

---

## 🏗️ 架構設計

### 檔案結構
```
MADZINE/
├── src/
│   ├── widgets/
│   │   ├── Knobs.hpp           // 主要旋鈕定義
│   │   ├── KnobBase.hpp        // 基礎類別與 undo/redo
│   │   └── KnobStyles.hpp      // 樣式常數定義
│   ├── plugin.hpp               // 包含 widgets/Knobs.hpp
│   └── backups_20250122/        // 原始檔案備份
│       └── *.cpp.backup
```

### 類別繼承架構

```cpp
app::Knob (VCV Rack 內建)
    ↓
BaseCustomKnob (基礎類別 - 處理 undo/redo)
    ├─ StandardKnob (標準旋鈕基礎)
    │   ├─ StandardBlackKnob
    │   ├─ TechnoStandardBlackKnob
    │   └─ SmallGrayKnob
    ├─ SnapKnob (整數跳躍基礎)
    │   ├─ TechnoSnapKnob
    │   └─ MADDYSnapKnob
    └─ SpecialKnob (特殊旋鈕基礎)
        ├─ HiddenTimeKnob
        └─ RandomizedKnob
```

---

## 📝 實施步驟

### Phase 0: 準備工作 (Day 1)
- [x] 建立本計畫文件
- [ ] 備份所有 .cpp 檔案到 backups_20250122/
- [ ] 建立 widgets/ 目錄結構
- [ ] 建立版本控制分支 `feature/knobs-refactor`

### Phase 1: 基礎架構 (Day 2-3)
- [ ] 實作 KnobBase.hpp
  - [ ] BaseCustomKnob 類別
  - [ ] Undo/redo 機制
  - [ ] 基本繪製函數
- [ ] 實作 KnobStyles.hpp
  - [ ] 顏色定義
  - [ ] 尺寸常數
  - [ ] 偏移值定義
- [ ] 測試基礎類別編譯

### Phase 2: 核心旋鈕實作 (Day 4-5)
- [ ] StandardBlackKnob (11 個模組)
- [ ] TechnoStandardBlackKnob (4 個模組)
- [ ] HiddenTimeKnob (4 個模組)
- [ ] SmallGrayKnob (3 個模組)
- [ ] 在 U8 模組測試

### Phase 3: Snap 旋鈕實作 (Day 6)
- [ ] BaseSnapKnob 基礎類別
- [ ] TechnoSnapKnob
- [ ] SnapKnob
- [ ] 其他 Snap 變體
- [ ] 在 KIMO 模組測試

### Phase 4: 特殊旋鈕實作 (Day 7)
- [ ] RandomizedKnob
- [ ] MicrotuneKnob
- [ ] 白色旋鈕系列
- [ ] Hidden 系列完善

### Phase 5: 整合與測試 (Day 8-9)
- [ ] 更新所有模組的 #include
- [ ] 移除舊的旋鈕定義
- [ ] 完整編譯測試
- [ ] Undo/redo 功能測試
- [ ] 載入舊專案測試

### Phase 6: 優化與文件 (Day 10)
- [ ] 效能優化
- [ ] 程式碼註解
- [ ] 使用文件撰寫
- [ ] 清理備份檔案

---

## 🔧 技術規範

### Undo/Redo 實作
```cpp
class BaseCustomKnob : public app::Knob {
protected:
    float oldValue = 0.0f;

public:
    void onDragStart(const event::DragStart& e) override {
        if (ParamQuantity* pq = getParamQuantity()) {
            oldValue = pq->getValue();
        }
        app::Knob::onDragStart(e);
    }

    void onDragEnd(const event::DragEnd& e) override {
        if (ParamQuantity* pq = getParamQuantity()) {
            float newValue = pq->getValue();
            if (oldValue != newValue) {
                // History is handled by app::Knob
            }
        }
        app::Knob::onDragEnd(e);
    }
};
```

### 靈敏度對應表
| 原始 ParamWidget | app::Knob speed |
|------------------|-----------------|
| sensitivity 0.002f | speed 0.5f |
| sensitivity 0.003f | speed 0.75f |
| sensitivity 0.001f | speed 0.25f |

---

## ⚠️ 風險管理

### 已識別風險
1. **靈敏度改變**: 需要仔細調校每個旋鈕的 speed 值
2. **Snap 行為**: 累積邏輯需要特別處理
3. **編譯時間**: 單一 hpp 可能增加編譯時間
4. **相容性**: 舊專案載入可能有問題

### 緩解措施
1. **完整備份**: 所有原始檔案備份在 backups_20250122/
2. **漸進測試**: 每個 Phase 完成後立即測試
3. **版本控制**: 使用 Git 分支，可隨時回滾
4. **A/B 測試**: 新舊版本並行比較

---

## 📊 成功指標

- [ ] 所有旋鈕支援 Ctrl+Z/Ctrl+Shift+Z
- [ ] 程式碼行數減少 > 60%
- [ ] 編譯時間增加 < 10%
- [ ] 使用者無法察覺操作差異
- [ ] 所有模組正常運作
- [ ] 舊專案可正常載入

---

## 📅 時程追蹤

| 日期 | 任務 | 狀態 | 備註 |
|------|------|------|------|
| 2025-01-22 | 計畫制定 | ✅ 完成 | |
| 2025-01-22 | Phase 0 準備 | 🔄 進行中 | |
| 2025-01-23 | Phase 1 基礎架構 | ⏳ 待開始 | |
| 2025-01-24 | Phase 2 核心旋鈕 | ⏳ 待開始 | |
| 2025-01-25 | Phase 3 Snap旋鈕 | ⏳ 待開始 | |
| 2025-01-26 | Phase 4 特殊旋鈕 | ⏳ 待開始 | |
| 2025-01-27 | Phase 5 整合測試 | ⏳ 待開始 | |
| 2025-01-28 | Phase 6 優化文件 | ⏳ 待開始 | |

---

## 📚 參考資源

- [VCV Rack API Documentation](https://vcvrack.com/docs-v2/)
- [VCV Rack Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial)
- [MADZINE Component Y Offset Reference](COMPONENT_Y_OFFSET_REFERENCE.txt)

---

## 🗒️ 備註

- 本計畫為活文件，將隨實施進度更新
- 所有變更需經過測試後才能合併到主分支
- 保持與使用者的溝通，收集回饋

---

最後更新: 2025-01-22