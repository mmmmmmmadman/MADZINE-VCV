# /vcv-feature

開發 VCV Rack 模組新功能

## 使用方式
```
/vcv-feature [功能描述]
/vcv-feature new [模組名稱] [功能描述]
/vcv-feature add [現有模組] [新功能]
```

## 範例
```
/vcv-feature new ClockDiv "4通道時鐘除頻器"
/vcv-feature add Quantizer "加入 CV 控制 root note"
/vcv-feature "實作 swing 功能"
```

## 功能
1. 分析現有 MADZINE 模組作為參考
2. 設計參數和 I/O 架構
3. 生成完整的模組程式碼
4. 整合面板主題系統
5. 提供 plugin.json 條目

## 輸出
- 完整的 .cpp 模組程式碼
- plugin.hpp 修改建議
- plugin.cpp 修改建議
- plugin.json 條目
- SVG 面板尺寸建議

## 參考模組
根據功能類型自動參考相似模組:
- 節奏類: EuclideanRhythm, UniversalRhythm
- 效果類: EllenRipley, Quantizer
- 工具類: U8, YAMANOTE
- 視覺類: Observer, Multiverse

## Agent
使用 Feature Development Agent 執行此技能
