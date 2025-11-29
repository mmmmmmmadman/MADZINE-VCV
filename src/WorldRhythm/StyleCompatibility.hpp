#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace WorldRhythm {

// ========================================
// Style Compatibility Matrix
// ========================================
// 基於研究論文 Section 4.3 定義的 10×10 風格相容性矩陣
// 數值範圍：0.0（完全不相容）到 1.0（完全相容）
//
// 相容性基於：
// 1. 地理/文化關聯
// 2. 節奏結構相似性（timeline, clave, 拍號）
// 3. 歷史交流（如非洲-古巴-巴西的連結）
// 4. 音樂特徵重疊（swing, polyrhythm, density）

// 風格索引：
// 0 = West African
// 1 = Afro-Cuban
// 2 = Brazilian
// 3 = Balkan
// 4 = Indian
// 5 = Gamelan
// 6 = Jazz
// 7 = Electronic
// 8 = Breakbeat
// 9 = Techno

class StyleCompatibility {
private:
    // 10×10 相容性矩陣（對稱）
    static constexpr std::array<std::array<float, 10>, 10> MATRIX = {{
        //    WA    AC    BR    BK    IN    GM    JZ    EL    BB    TC
        {{1.00f, 0.90f, 0.85f, 0.40f, 0.50f, 0.45f, 0.70f, 0.55f, 0.60f, 0.45f}}, // West African
        {{0.90f, 1.00f, 0.88f, 0.35f, 0.45f, 0.40f, 0.75f, 0.60f, 0.65f, 0.50f}}, // Afro-Cuban
        {{0.85f, 0.88f, 1.00f, 0.38f, 0.42f, 0.42f, 0.72f, 0.58f, 0.62f, 0.48f}}, // Brazilian
        {{0.40f, 0.35f, 0.38f, 1.00f, 0.55f, 0.30f, 0.45f, 0.50f, 0.48f, 0.52f}}, // Balkan
        {{0.50f, 0.45f, 0.42f, 0.55f, 1.00f, 0.60f, 0.48f, 0.40f, 0.42f, 0.38f}}, // Indian
        {{0.45f, 0.40f, 0.42f, 0.30f, 0.60f, 1.00f, 0.35f, 0.55f, 0.45f, 0.50f}}, // Gamelan
        {{0.70f, 0.75f, 0.72f, 0.45f, 0.48f, 0.35f, 1.00f, 0.65f, 0.70f, 0.55f}}, // Jazz
        {{0.55f, 0.60f, 0.58f, 0.50f, 0.40f, 0.55f, 0.65f, 1.00f, 0.85f, 0.90f}}, // Electronic
        {{0.60f, 0.65f, 0.62f, 0.48f, 0.42f, 0.45f, 0.70f, 0.85f, 1.00f, 0.80f}}, // Breakbeat
        {{0.45f, 0.50f, 0.48f, 0.52f, 0.38f, 0.50f, 0.55f, 0.90f, 0.80f, 1.00f}}  // Techno
    }};

    // 相容性閾值
    static constexpr float HIGH_COMPAT = 0.75f;    // 高度相容
    static constexpr float MEDIUM_COMPAT = 0.50f;  // 中度相容
    static constexpr float LOW_COMPAT = 0.35f;     // 低度相容

public:
    // ========================================
    // Basic Queries
    // ========================================

    // 獲取兩種風格之間的相容性
    static float getCompatibility(int style1, int style2) {
        if (style1 < 0 || style1 >= 10 || style2 < 0 || style2 >= 10) {
            return 0.5f;  // 無效索引返回中性值
        }
        return MATRIX[style1][style2];
    }

    // 獲取風格名稱
    static const char* getStyleName(int styleIndex) {
        static const char* NAMES[] = {
            "West African", "Afro-Cuban", "Brazilian", "Balkan", "Indian",
            "Gamelan", "Jazz", "Electronic", "Breakbeat", "Techno"
        };
        if (styleIndex < 0 || styleIndex >= 10) return "Unknown";
        return NAMES[styleIndex];
    }

    // ========================================
    // Compatibility Analysis
    // ========================================

    // 判斷相容性等級
    enum class CompatLevel {
        HIGH,      // >= 0.75
        MEDIUM,    // >= 0.50
        LOW,       // >= 0.35
        CONFLICT   // < 0.35
    };

    static CompatLevel getCompatLevel(int style1, int style2) {
        float c = getCompatibility(style1, style2);
        if (c >= HIGH_COMPAT) return CompatLevel::HIGH;
        if (c >= MEDIUM_COMPAT) return CompatLevel::MEDIUM;
        if (c >= LOW_COMPAT) return CompatLevel::LOW;
        return CompatLevel::CONFLICT;
    }

    // 獲取與指定風格最相容的風格列表
    static std::array<int, 3> getMostCompatible(int styleIndex) {
        std::array<std::pair<float, int>, 10> scores;
        for (int i = 0; i < 10; i++) {
            scores[i] = {getCompatibility(styleIndex, i), i};
        }

        // 排序（不包括自己）
        std::sort(scores.begin(), scores.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        std::array<int, 3> result;
        int idx = 0;
        for (const auto& [score, style] : scores) {
            if (style != styleIndex && idx < 3) {
                result[idx++] = style;
            }
        }
        return result;
    }

    // ========================================
    // Interlock Strength Calculation
    // ========================================
    // 根據相容性動態調整互鎖強度

    struct InterlockParams {
        float avoidanceStrength;    // 避開另一角色的強度 [0, 1]
        float complementBoost;      // 互補位置的加權 [1, 2]
        bool strictInterlock;       // 是否使用嚴格互鎖
    };

    static InterlockParams calculateInterlockParams(int style1, int style2) {
        float compat = getCompatibility(style1, style2);
        InterlockParams params;

        if (compat >= HIGH_COMPAT) {
            // 高相容：輕度互鎖，允許重疊
            params.avoidanceStrength = 0.3f;
            params.complementBoost = 1.2f;
            params.strictInterlock = false;
        } else if (compat >= MEDIUM_COMPAT) {
            // 中相容：適度互鎖
            params.avoidanceStrength = 0.6f;
            params.complementBoost = 1.5f;
            params.strictInterlock = false;
        } else if (compat >= LOW_COMPAT) {
            // 低相容：強互鎖，減少衝突
            params.avoidanceStrength = 0.8f;
            params.complementBoost = 1.8f;
            params.strictInterlock = true;
        } else {
            // 衝突：最強互鎖，嚴格分離
            params.avoidanceStrength = 1.0f;
            params.complementBoost = 2.0f;
            params.strictInterlock = true;
        }

        return params;
    }

    // ========================================
    // Blend Weights Calculation
    // ========================================
    // 計算混合兩種風格時各自的權重

    struct BlendWeights {
        float weight1;  // 風格1的權重
        float weight2;  // 風格2的權重
        float overlap;  // 重疊區的處理方式（0=避開, 1=允許）
    };

    static BlendWeights calculateBlendWeights(int style1, int style2,
                                              float balance = 0.5f) {
        float compat = getCompatibility(style1, style2);
        BlendWeights weights;

        // 基本權重由 balance 決定
        weights.weight1 = 1.0f - balance;
        weights.weight2 = balance;

        // 相容性影響重疊處理
        weights.overlap = compat;

        // 低相容時調整權重，讓主導風格更明顯
        if (compat < MEDIUM_COMPAT) {
            if (balance < 0.5f) {
                weights.weight1 *= (1.0f + (MEDIUM_COMPAT - compat));
            } else {
                weights.weight2 *= (1.0f + (MEDIUM_COMPAT - compat));
            }
            // 正規化
            float total = weights.weight1 + weights.weight2;
            weights.weight1 /= total;
            weights.weight2 /= total;
        }

        return weights;
    }

    // ========================================
    // Fill/Ornament Compatibility
    // ========================================
    // 某些 Fill 類型只適合特定風格組合

    enum class FillCompatibility {
        RECOMMENDED,   // 推薦使用
        ACCEPTABLE,    // 可以使用
        AVOID          // 應避免
    };

    // 檢查 Tihai 是否適合當前風格組合
    static FillCompatibility checkTihaiCompatibility(int primaryStyle, int secondaryStyle = -1) {
        // Tihai 主要適合印度風格
        if (primaryStyle == 4) return FillCompatibility::RECOMMENDED;

        // 與印度的相容性
        if (secondaryStyle >= 0) {
            float compatWithIndian = std::max(
                getCompatibility(primaryStyle, 4),
                getCompatibility(secondaryStyle, 4)
            );
            if (compatWithIndian >= MEDIUM_COMPAT) return FillCompatibility::ACCEPTABLE;
        }

        // 一般情況下可接受但不推薦
        return FillCompatibility::ACCEPTABLE;
    }

    // 檢查 Angsel 是否適合當前風格組合
    static FillCompatibility checkAngselCompatibility(int primaryStyle, int secondaryStyle = -1) {
        // Angsel 主要適合 Gamelan 風格
        if (primaryStyle == 5) return FillCompatibility::RECOMMENDED;

        // 與 Gamelan 的相容性
        if (secondaryStyle >= 0) {
            float compatWithGamelan = std::max(
                getCompatibility(primaryStyle, 5),
                getCompatibility(secondaryStyle, 5)
            );
            if (compatWithGamelan >= MEDIUM_COMPAT) return FillCompatibility::ACCEPTABLE;
        }

        // 電子/Techno 可能覺得 Angsel 的靜默太突兀
        if (primaryStyle == 7 || primaryStyle == 9) return FillCompatibility::AVOID;

        return FillCompatibility::ACCEPTABLE;
    }

    // ========================================
    // Cross-Rhythm Compatibility
    // ========================================
    // 某些 cross-rhythm 更適合特定風格

    static float getCrossRhythmAffinity(int styleIndex, int crossRhythmNum, int crossRhythmDen) {
        // 3:2 - 非洲、古巴、巴西的核心
        if (crossRhythmNum == 3 && crossRhythmDen == 2) {
            if (styleIndex <= 2) return 1.0f;  // WA, AC, BR
            if (styleIndex == 6) return 0.8f;  // Jazz
            return 0.5f;
        }

        // 4:3 - 較為通用
        if (crossRhythmNum == 4 && crossRhythmDen == 3) {
            return 0.7f;  // 大多數風格都可以
        }

        // 5:4 - 印度、巴爾幹
        if (crossRhythmNum == 5 && crossRhythmDen == 4) {
            if (styleIndex == 4 || styleIndex == 3) return 1.0f;  // IN, BK
            return 0.4f;
        }

        // 7:4 - 巴爾幹
        if (crossRhythmNum == 7 && crossRhythmDen == 4) {
            if (styleIndex == 3) return 1.0f;  // Balkan
            if (styleIndex == 4) return 0.7f;  // Indian
            return 0.3f;
        }

        return 0.5f;  // 預設
    }

    // ========================================
    // Style Family Groups
    // ========================================
    // 風格家族（用於 UI 分組或推薦）

    enum class StyleFamily {
        AFRICAN_DIASPORA,  // West African, Afro-Cuban, Brazilian
        EASTERN,           // Indian, Gamelan
        EUROPEAN,          // Balkan
        WESTERN_MODERN,    // Jazz, Electronic, Breakbeat, Techno
    };

    static StyleFamily getStyleFamily(int styleIndex) {
        switch (styleIndex) {
            case 0: case 1: case 2: return StyleFamily::AFRICAN_DIASPORA;
            case 4: case 5: return StyleFamily::EASTERN;
            case 3: return StyleFamily::EUROPEAN;
            case 6: case 7: case 8: case 9: return StyleFamily::WESTERN_MODERN;
            default: return StyleFamily::WESTERN_MODERN;
        }
    }

    // 同家族的風格通常更相容
    static bool areSameFamily(int style1, int style2) {
        return getStyleFamily(style1) == getStyleFamily(style2);
    }

    // ========================================
    // Debug / Visualization
    // ========================================
    static void printMatrix() {
        const char* abbrev[] = {"WA", "AC", "BR", "BK", "IN", "GM", "JZ", "EL", "BB", "TC"};

        // 可用於 debug 輸出
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                // printf("%.2f ", MATRIX[i][j]);
            }
            // printf("\n");
        }
    }
};

} // namespace WorldRhythm
