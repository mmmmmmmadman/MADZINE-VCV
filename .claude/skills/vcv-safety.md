# /vcv-safety

檢查 VCV 模組程式碼的潛在崩潰風險

## 使用方式
```
/vcv-safety [模組名稱或檔案路徑]
/vcv-safety all  # 檢查所有模組
```

## 範例
```
/vcv-safety UniversalRhythm
/vcv-safety src/Quantizer.cpp
/vcv-safety all
```

## 檢查項目
1. **陣列越界**: 檢查所有 `[]` 存取
2. **空指標**: 檢查 module 指標存取
3. **除零錯誤**: 檢查除法運算
4. **未初始化變數**: 檢查變數宣告
5. **向量存取**: 檢查 std::vector 存取

## 輸出
- 風險等級 (HIGH/MEDIUM/LOW)
- 問題行號和描述
- 建議的修復程式碼
- 預防措施建議

## Agent
使用 Crash Prevention Agent 執行此技能
