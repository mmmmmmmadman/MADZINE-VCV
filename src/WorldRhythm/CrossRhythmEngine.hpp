#pragma once

#include <vector>
#include <cmath>
#include <random>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Cross-Rhythm Engine
// ========================================
// Generates independent polyrhythmic layers (3:2, 4:3, 5:4, etc.)
// Different from polyrhythm (different lengths) - cross-rhythm uses
// the SAME length but different subdivisions

enum class CrossRhythmType {
    NONE,
    CR_3_2,      // 3 against 2 (hemiola)
    CR_4_3,      // 4 against 3
    CR_5_4,      // 5 against 4
    CR_5_3,      // 5 against 3
    CR_7_4,      // 7 against 4
    CR_6_4       // 6 against 4 (dotted quarter vs quarter)
};

struct CrossRhythmLayer {
    int numerator;      // Number of beats in this layer
    int denominator;    // Against this many base beats
    std::vector<float> positions;  // Normalized positions (0.0-1.0)
    std::vector<float> weights;    // Accent weights
};

// ========================================
// Cross-Rhythm Definitions
// ========================================
inline CrossRhythmLayer createCrossRhythm_3_2() {
    CrossRhythmLayer layer;
    layer.numerator = 3;
    layer.denominator = 2;

    // 3 evenly spaced hits over 2 beats
    // Positions: 0, 1/3, 2/3 (normalized over 1.0)
    layer.positions = {0.0f, 0.333f, 0.667f};
    layer.weights = {1.0f, 0.7f, 0.8f};

    return layer;
}

inline CrossRhythmLayer createCrossRhythm_4_3() {
    CrossRhythmLayer layer;
    layer.numerator = 4;
    layer.denominator = 3;

    // 4 evenly spaced hits over 3 beats
    layer.positions = {0.0f, 0.25f, 0.5f, 0.75f};
    layer.weights = {1.0f, 0.6f, 0.8f, 0.65f};

    return layer;
}

inline CrossRhythmLayer createCrossRhythm_5_4() {
    CrossRhythmLayer layer;
    layer.numerator = 5;
    layer.denominator = 4;

    // 5 evenly spaced hits over 4 beats
    layer.positions = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};
    layer.weights = {1.0f, 0.55f, 0.7f, 0.6f, 0.75f};

    return layer;
}

inline CrossRhythmLayer createCrossRhythm_5_3() {
    CrossRhythmLayer layer;
    layer.numerator = 5;
    layer.denominator = 3;

    layer.positions = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};
    layer.weights = {1.0f, 0.5f, 0.65f, 0.55f, 0.7f};

    return layer;
}

inline CrossRhythmLayer createCrossRhythm_7_4() {
    CrossRhythmLayer layer;
    layer.numerator = 7;
    layer.denominator = 4;

    // 7 evenly spaced hits over 4 beats
    float step = 1.0f / 7.0f;
    for (int i = 0; i < 7; i++) {
        layer.positions.push_back(i * step);
        layer.weights.push_back(i == 0 ? 1.0f : (i % 2 == 0 ? 0.65f : 0.5f));
    }

    return layer;
}

inline CrossRhythmLayer createCrossRhythm_6_4() {
    CrossRhythmLayer layer;
    layer.numerator = 6;
    layer.denominator = 4;

    // 6 evenly spaced (dotted quarter feel)
    float step = 1.0f / 6.0f;
    for (int i = 0; i < 6; i++) {
        layer.positions.push_back(i * step);
        layer.weights.push_back(i == 0 ? 1.0f : (i == 3 ? 0.85f : 0.6f));
    }

    return layer;
}

// ========================================
// Cross-Rhythm Engine
// ========================================
class CrossRhythmEngine {
private:
    std::mt19937 rng;
    std::vector<CrossRhythmLayer> availableLayers;

public:
    CrossRhythmEngine() : rng(std::random_device{}()) {
        availableLayers.push_back(CrossRhythmLayer{});  // NONE
        availableLayers.push_back(createCrossRhythm_3_2());
        availableLayers.push_back(createCrossRhythm_4_3());
        availableLayers.push_back(createCrossRhythm_5_4());
        availableLayers.push_back(createCrossRhythm_5_3());
        availableLayers.push_back(createCrossRhythm_7_4());
        availableLayers.push_back(createCrossRhythm_6_4());
    }

    void seed(unsigned int s) { rng.seed(s); }

    // ========================================
    // Get cross-rhythm layer by type
    // ========================================
    const CrossRhythmLayer& getLayer(CrossRhythmType type) const {
        int idx = static_cast<int>(type);
        if (idx >= 0 && idx < static_cast<int>(availableLayers.size())) {
            return availableLayers[idx];
        }
        return availableLayers[0];
    }

    // ========================================
    // Generate cross-rhythm pattern
    // ========================================
    Pattern generateCrossRhythmPattern(CrossRhythmType type, int length, float intensity) {
        Pattern p(length);

        if (type == CrossRhythmType::NONE) {
            return p;
        }

        const CrossRhythmLayer& layer = getLayer(type);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        for (size_t i = 0; i < layer.positions.size(); i++) {
            // Convert normalized position to step
            int step = static_cast<int>(layer.positions[i] * length);
            if (step >= length) step = length - 1;

            float vel = layer.weights[i] * intensity + velVar(rng);
            p.setOnset(step, std::clamp(vel, 0.3f, 1.0f));

            if (layer.weights[i] > 0.7f) {
                p.accents[step] = true;
            }
        }

        return p;
    }

    // ========================================
    // Apply cross-rhythm overlay to existing pattern
    // ========================================
    void applyCrossRhythmOverlay(Pattern& p, CrossRhythmType type, float intensity, float blend) {
        if (type == CrossRhythmType::NONE || blend <= 0.0f) {
            return;
        }

        // Don't add cross-rhythm to empty patterns (respect density=0)
        bool hasAnyOnset = false;
        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                hasAnyOnset = true;
                break;
            }
        }
        if (!hasAnyOnset) return;

        const CrossRhythmLayer& layer = getLayer(type);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (size_t i = 0; i < layer.positions.size(); i++) {
            int step = static_cast<int>(layer.positions[i] * p.length);
            if (step >= p.length) step = p.length - 1;

            float crossVel = layer.weights[i] * intensity;
            float existing = p.getVelocity(step);

            // Blend: either add where empty or boost where exists
            if (existing < 0.1f) {
                // Empty position: add cross-rhythm hit
                float vel = crossVel * blend + velVar(rng);
                p.setOnset(step, std::clamp(vel, 0.3f, 0.9f));
            } else {
                // Existing hit: boost velocity
                float vel = existing + (crossVel - existing) * blend * 0.5f + velVar(rng);
                p.setOnset(step, std::clamp(vel, existing, 1.0f));
            }
        }
    }

    // ========================================
    // Generate complementary cross-rhythm pair
    // One layer plays the main rhythm, other plays cross-rhythm
    // ========================================
    std::pair<Pattern, Pattern> generateCrossRhythmPair(
        CrossRhythmType type, int length, float intensity) {

        const CrossRhythmLayer& layer = getLayer(type);

        // Main pattern: plays on denominator grid
        Pattern main(length);
        int mainSteps = layer.denominator;
        float mainInterval = static_cast<float>(length) / mainSteps;

        for (int i = 0; i < mainSteps; i++) {
            int step = static_cast<int>(i * mainInterval);
            float vel = (i == 0) ? 0.9f : 0.7f;
            main.setOnset(step, vel * intensity);
            if (i == 0) main.accents[step] = true;
        }

        // Cross pattern: plays on numerator grid
        Pattern cross = generateCrossRhythmPattern(type, length, intensity);

        return {main, cross};
    }

    // ========================================
    // Get recommended cross-rhythm for style
    // ========================================
    CrossRhythmType getStyleCrossRhythm(int styleIndex) const {
        switch (styleIndex) {
            case 0:  // West African - 3:2 is fundamental
                return CrossRhythmType::CR_3_2;
            case 1:  // Afro-Cuban - 3:2 clave-based
                return CrossRhythmType::CR_3_2;
            case 2:  // Brazilian - 3:2 samba feel
                return CrossRhythmType::CR_3_2;
            case 3:  // Balkan - often 7:4 or 5:4
                return CrossRhythmType::CR_7_4;
            case 4:  // Indian - 5:4 or 7:4
                return CrossRhythmType::CR_5_4;
            case 5:  // Gamelan - 4:3
                return CrossRhythmType::CR_4_3;
            case 6:  // Jazz - 3:2 or 4:3
                return CrossRhythmType::CR_4_3;
            case 7:  // Electronic - 6:4
                return CrossRhythmType::CR_6_4;
            case 8:  // Breakbeat - 5:4
                return CrossRhythmType::CR_5_4;
            case 9:  // Techno - minimal, 6:4
                return CrossRhythmType::CR_6_4;
            default:
                return CrossRhythmType::CR_3_2;
        }
    }

    // ========================================
    // Get cross-rhythm name
    // ========================================
    static const char* getCrossRhythmName(CrossRhythmType type) {
        switch (type) {
            case CrossRhythmType::NONE: return "None";
            case CrossRhythmType::CR_3_2: return "3:2 (Hemiola)";
            case CrossRhythmType::CR_4_3: return "4:3";
            case CrossRhythmType::CR_5_4: return "5:4";
            case CrossRhythmType::CR_5_3: return "5:3";
            case CrossRhythmType::CR_7_4: return "7:4";
            case CrossRhythmType::CR_6_4: return "6:4 (Dotted)";
            default: return "Unknown";
        }
    }

    int getNumTypes() const { return 7; }  // Including NONE

    // ========================================
    // 明確 Cross-Rhythm 演算法（v0.17）
    // 將 cross-rhythm 精確映射到 16-step pattern
    // ========================================

    /**
     * 生成精確的 cross-rhythm 位置
     * @param type Cross-rhythm 類型
     * @param patternLength Pattern 長度（通常為 16）
     * @param cycles 完整循環數（對於 3:2，1 cycle = 2 beats）
     * @return 精確的 step 位置列表和對應權重
     */
    struct CrossRhythmHit {
        int step;           // 位置（0-based）
        float weight;       // 重音權重（0.0-1.0）
        bool isDownbeat;    // 是否為該層的強拍
    };

    std::vector<CrossRhythmHit> calculatePreciseCrossRhythm(
        CrossRhythmType type, int patternLength, int baseBeatSubdivision = 4) {

        std::vector<CrossRhythmHit> hits;
        if (type == CrossRhythmType::NONE) return hits;

        const CrossRhythmLayer& layer = getLayer(type);
        int num = layer.numerator;
        int denom = layer.denominator;

        // 計算每個 cross-rhythm 擊點的精確位置
        int totalSubdivisions = patternLength;

        for (int i = 0; i < num; i++) {
            CrossRhythmHit hit;
            // 精確計算位置（使用整數運算避免浮點誤差）
            hit.step = (i * totalSubdivisions * denom) / (num * denom);
            hit.step = hit.step % patternLength;

            // 權重：首音最強，根據位置遞減
            hit.weight = layer.weights[std::min(i, static_cast<int>(layer.weights.size()) - 1)];
            hit.isDownbeat = (i == 0);

            hits.push_back(hit);
        }

        return hits;
    }

    /**
     * 計算兩個節奏層的衝突點（用於互鎖避免）
     */
    std::vector<int> findRhythmCollisions(
        const std::vector<CrossRhythmHit>& layer1,
        const std::vector<CrossRhythmHit>& layer2,
        int tolerance = 1) {

        std::vector<int> collisions;

        for (const auto& h1 : layer1) {
            for (const auto& h2 : layer2) {
                if (std::abs(h1.step - h2.step) <= tolerance) {
                    collisions.push_back(h1.step);
                    break;
                }
            }
        }

        return collisions;
    }

    /**
     * 生成互補的 cross-rhythm pattern pair
     * 確保兩層不會同時擊打（互鎖原則）
     */
    struct CrossRhythmPairResult {
        Pattern baseLayer;      // 基礎節奏（denominator）
        Pattern crossLayer;     // Cross-rhythm（numerator）
        std::vector<int> syncPoints;  // 同步點（兩層同時）
    };

    CrossRhythmPairResult generateInterlockingCrossRhythm(
        CrossRhythmType type, int length, float intensity, bool allowSync = false) {

        CrossRhythmPairResult result;
        result.baseLayer = Pattern(length);
        result.crossLayer = Pattern(length);

        if (type == CrossRhythmType::NONE) return result;

        const CrossRhythmLayer& layer = getLayer(type);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        // Base layer: denominator 網格
        int baseSteps = layer.denominator;
        float baseInterval = static_cast<float>(length) / baseSteps;

        std::vector<int> basePositions;
        for (int i = 0; i < baseSteps; i++) {
            int step = static_cast<int>(i * baseInterval);
            if (step >= length) step = length - 1;
            basePositions.push_back(step);

            float vel = (i == 0) ? 0.95f : 0.75f;
            result.baseLayer.setOnset(step, std::clamp(vel * intensity + velVar(rng), 0.5f, 1.0f));
            if (i == 0) result.baseLayer.accents[step] = true;
        }

        // Cross layer: numerator 網格
        auto crossHits = calculatePreciseCrossRhythm(type, length);

        for (const auto& hit : crossHits) {
            // 檢查是否與 base layer 衝突
            bool collision = false;
            for (int basePos : basePositions) {
                if (std::abs(hit.step - basePos) <= 1) {
                    collision = true;
                    if (hit.step == basePos) {
                        result.syncPoints.push_back(hit.step);
                    }
                    break;
                }
            }

            if (!collision || allowSync) {
                float vel = hit.weight * intensity + velVar(rng);
                result.crossLayer.setOnset(hit.step, std::clamp(vel, 0.4f, 0.95f));
                if (hit.isDownbeat) {
                    result.crossLayer.accents[hit.step] = true;
                }
            }
        }

        return result;
    }

    /**
     * 獲取風格的 cross-rhythm 強度建議
     */
    float getStyleCrossRhythmIntensity(int styleIndex) const {
        switch (styleIndex) {
            case 0:  // West African - 3:2 非常強烈
                return 0.85f;
            case 1:  // Afro-Cuban - 3:2 中等
                return 0.70f;
            case 2:  // Brazilian - 較輕
                return 0.55f;
            case 3:  // Balkan - 複雜節拍
                return 0.60f;
            case 4:  // Indian - 較輕
                return 0.50f;
            case 5:  // Gamelan - 4:3 明顯
                return 0.65f;
            case 6:  // Jazz - 微妙
                return 0.45f;
            case 7:  // Electronic - 極輕
                return 0.30f;
            case 8:  // Breakbeat - 中等
                return 0.50f;
            case 9:  // Techno - 極輕
                return 0.25f;
            default:
                return 0.50f;
        }
    }
};

} // namespace WorldRhythm
