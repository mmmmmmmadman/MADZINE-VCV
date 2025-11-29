#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "PatternGenerator.hpp"
#include "StyleProfiles.hpp"

namespace WorldRhythm {

// ========================================
// Humanization Engine
// ========================================
// Comprehensive humanization system including:
// - Groove templates (systematic microtiming)
// - Articulation types (RIM, CROSS, FLAM, DRAG)
// - Velocity layers (pppp to fff)
// - Hand dynamics (dominant/non-dominant)
// - Long-term dynamics (phrase, section, piece)
// - Error modeling (occasional misses)

// ========================================
// Articulation Types
// ========================================
enum class ArticulationType {
    NORMAL,     // Standard hit
    GHOST,      // Very soft, almost inaudible
    ACCENT,     // Emphasized hit
    RIM,        // Rim shot (snare)
    CROSS,      // Cross-stick
    FLAM,       // Grace note + main note
    DRAG,       // Two grace notes + main
    BUZZ,       // Buzz/press roll
    DEAD,       // Muted/choked
    // v0.17 新增
    RUFF,       // Three grace notes + main (3-stroke ruff)
    PARADIDDLE  // RLRR / LRLL sticking pattern
};

// ========================================
// Velocity Layers
// ========================================
enum class VelocityLayer {
    PPPP = 0,   // 0.05 - 0.12
    PPP,        // 0.12 - 0.20
    PP,         // 0.20 - 0.30
    P,          // 0.30 - 0.45
    MP,         // 0.45 - 0.55
    MF,         // 0.55 - 0.70
    F,          // 0.70 - 0.82
    FF,         // 0.82 - 0.92
    FFF,        // 0.92 - 1.00
    NUM_LAYERS
};

// ========================================
// Hand Assignment
// ========================================
enum class Hand {
    RIGHT,      // Typically dominant
    LEFT,       // Typically non-dominant
    BOTH,       // Both hands (flams, etc.)
    FOOT        // Kick/hi-hat pedal
};

// ========================================
// Extended Note Information
// ========================================
struct NoteInfo {
    float velocity;
    ArticulationType articulation;
    VelocityLayer layer;
    Hand hand;
    float microOffset;      // Microtiming offset in ms
    bool hasError;          // Is this a "mistake"
};

// ========================================
// Groove Template
// ========================================
// Each position has a systematic timing offset that defines the "feel"
struct GrooveTemplate {
    std::string name;
    float offsets[16];      // Timing offsets in ms for each 16th note position
    float velMods[16];      // Velocity modifiers (multiplier)
};

// ========================================
// Style-Specific Groove Templates
// ========================================
inline GrooveTemplate createSwingGroove() {
    GrooveTemplate g;
    g.name = "Swing";
    // Swing: upbeats pushed late
    g.offsets[0] = 0.0f;   g.velMods[0] = 1.0f;
    g.offsets[1] = 8.0f;   g.velMods[1] = 0.85f;   // Late
    g.offsets[2] = 0.0f;   g.velMods[2] = 0.9f;
    g.offsets[3] = 10.0f;  g.velMods[3] = 0.8f;    // Late
    g.offsets[4] = 0.0f;   g.velMods[4] = 0.95f;
    g.offsets[5] = 7.0f;   g.velMods[5] = 0.85f;
    g.offsets[6] = 0.0f;   g.velMods[6] = 0.9f;
    g.offsets[7] = 9.0f;   g.velMods[7] = 0.8f;
    g.offsets[8] = -2.0f;  g.velMods[8] = 1.0f;    // Beat 3 slightly early
    g.offsets[9] = 8.0f;   g.velMods[9] = 0.85f;
    g.offsets[10] = 0.0f;  g.velMods[10] = 0.9f;
    g.offsets[11] = 10.0f; g.velMods[11] = 0.8f;
    g.offsets[12] = 0.0f;  g.velMods[12] = 0.95f;
    g.offsets[13] = 7.0f;  g.velMods[13] = 0.85f;
    g.offsets[14] = 0.0f;  g.velMods[14] = 0.9f;
    g.offsets[15] = 9.0f;  g.velMods[15] = 0.8f;
    return g;
}

inline GrooveTemplate createAfricanGroove() {
    GrooveTemplate g;
    g.name = "African";
    // West African: specific positions pushed/pulled for polyrhythmic feel
    g.offsets[0] = 0.0f;   g.velMods[0] = 1.0f;
    g.offsets[1] = -3.0f;  g.velMods[1] = 0.7f;    // Early
    g.offsets[2] = 2.0f;   g.velMods[2] = 0.85f;
    g.offsets[3] = -2.0f;  g.velMods[3] = 0.9f;    // Bell pattern
    g.offsets[4] = 0.0f;   g.velMods[4] = 0.8f;
    g.offsets[5] = 3.0f;   g.velMods[5] = 0.75f;
    g.offsets[6] = -1.0f;  g.velMods[6] = 0.95f;   // Strong
    g.offsets[7] = 2.0f;   g.velMods[7] = 0.7f;
    g.offsets[8] = 0.0f;   g.velMods[8] = 0.85f;
    g.offsets[9] = -3.0f;  g.velMods[9] = 0.7f;
    g.offsets[10] = 1.0f;  g.velMods[10] = 0.9f;   // Bell
    g.offsets[11] = 0.0f;  g.velMods[11] = 0.75f;
    g.offsets[12] = 2.0f;  g.velMods[12] = 1.0f;   // Strong
    g.offsets[13] = -2.0f; g.velMods[13] = 0.7f;
    g.offsets[14] = 0.0f;  g.velMods[14] = 0.8f;
    g.offsets[15] = 3.0f;  g.velMods[15] = 0.75f;
    return g;
}

inline GrooveTemplate createLatinGroove() {
    GrooveTemplate g;
    g.name = "Latin";
    // Afro-Cuban: clave-based feel
    g.offsets[0] = 0.0f;   g.velMods[0] = 1.0f;    // Clave
    g.offsets[1] = 4.0f;   g.velMods[1] = 0.7f;
    g.offsets[2] = 0.0f;   g.velMods[2] = 0.8f;
    g.offsets[3] = -2.0f;  g.velMods[3] = 0.95f;   // Clave
    g.offsets[4] = 0.0f;   g.velMods[4] = 0.75f;
    g.offsets[5] = 5.0f;   g.velMods[5] = 0.7f;
    g.offsets[6] = -1.0f;  g.velMods[6] = 0.9f;    // Clave
    g.offsets[7] = 3.0f;   g.velMods[7] = 0.7f;
    g.offsets[8] = 0.0f;   g.velMods[8] = 0.8f;
    g.offsets[9] = 4.0f;   g.velMods[9] = 0.7f;
    g.offsets[10] = -2.0f; g.velMods[10] = 0.95f;  // Clave
    g.offsets[11] = 3.0f;  g.velMods[11] = 0.7f;
    g.offsets[12] = 0.0f;  g.velMods[12] = 0.9f;   // Clave
    g.offsets[13] = 5.0f;  g.velMods[13] = 0.7f;
    g.offsets[14] = 0.0f;  g.velMods[14] = 0.75f;
    g.offsets[15] = 4.0f;  g.velMods[15] = 0.7f;
    return g;
}

inline GrooveTemplate createStraightGroove() {
    GrooveTemplate g;
    g.name = "Straight";
    // Machine-like but with subtle humanization
    for (int i = 0; i < 16; i++) {
        g.offsets[i] = 0.0f;
        g.velMods[i] = (i % 4 == 0) ? 1.0f : ((i % 2 == 0) ? 0.9f : 0.8f);
    }
    return g;
}

inline GrooveTemplate createLaidBackGroove() {
    GrooveTemplate g;
    g.name = "Laid Back";
    // Everything slightly behind the beat
    for (int i = 0; i < 16; i++) {
        g.offsets[i] = 4.0f + (i % 2) * 2.0f;
        g.velMods[i] = (i % 4 == 0) ? 1.0f : 0.85f;
    }
    return g;
}

inline GrooveTemplate createPushedGroove() {
    GrooveTemplate g;
    g.name = "Pushed";
    // Everything slightly ahead
    for (int i = 0; i < 16; i++) {
        g.offsets[i] = -3.0f - (i % 2) * 1.5f;
        g.velMods[i] = (i % 4 == 0) ? 1.0f : 0.9f;
    }
    return g;
}

// ========================================
// Style-Specific Timing Variance (v0.16)
// ========================================
// Based on ethnomusicological research:
// Traditional percussion: ±10-30ms (natural human variance)
// Jazz: ±5-20ms (laid back / on top feel)
// Funk: ±5-15ms (tight but human)
// House/Techno: 0-5ms (machine precision)

struct StyleTimingProfile {
    float baseVariance;     // Base timing variance in ms
    float roleMultipliers[4];  // Per-role multipliers (Timeline, Foundation, Groove, Lead)
    float swingRatioSlow;   // Swing ratio at slow tempo (<100 BPM)
    float swingRatioFast;   // Swing ratio at fast tempo (>180 BPM)
    float ghostVelocityMin; // Ghost note minimum relative velocity
    float ghostVelocityMax; // Ghost note maximum relative velocity
};

inline StyleTimingProfile getStyleTimingProfile(int styleIndex) {
    StyleTimingProfile p;

    switch (styleIndex) {
        case 0: // West African
            p.baseVariance = 25.0f;
            p.roleMultipliers[0] = 0.3f;  // Timeline: tight
            p.roleMultipliers[1] = 0.5f;  // Foundation
            p.roleMultipliers[2] = 1.0f;  // Groove
            p.roleMultipliers[3] = 1.2f;  // Lead: most free
            p.swingRatioSlow = 0.63f;     // 60-65%
            p.swingRatioFast = 0.58f;
            p.ghostVelocityMin = 0.25f;
            p.ghostVelocityMax = 0.40f;
            break;

        case 1: // Afro-Cuban
            p.baseVariance = 18.0f;
            p.roleMultipliers[0] = 0.2f;  // Clave: very tight
            p.roleMultipliers[1] = 0.6f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.3f;
            p.swingRatioSlow = 0.60f;     // 55-65%
            p.swingRatioFast = 0.55f;
            p.ghostVelocityMin = 0.30f;
            p.ghostVelocityMax = 0.45f;
            break;

        case 2: // Brazilian
            p.baseVariance = 15.0f;
            p.roleMultipliers[0] = 0.4f;
            p.roleMultipliers[1] = 0.5f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.2f;
            p.swingRatioSlow = 0.58f;     // 55-60%
            p.swingRatioFast = 0.54f;
            p.ghostVelocityMin = 0.28f;
            p.ghostVelocityMax = 0.42f;
            break;

        case 3: // Balkan
            p.baseVariance = 12.0f;
            p.roleMultipliers[0] = 0.5f;
            p.roleMultipliers[1] = 0.6f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.1f;
            p.swingRatioSlow = 0.52f;     // Mostly straight
            p.swingRatioFast = 0.50f;
            p.ghostVelocityMin = 0.30f;
            p.ghostVelocityMax = 0.40f;
            break;

        case 4: // Indian
            p.baseVariance = 20.0f;
            p.roleMultipliers[0] = 0.3f;  // Theka: relatively tight
            p.roleMultipliers[1] = 0.4f;
            p.roleMultipliers[2] = 0.8f;
            p.roleMultipliers[3] = 1.5f;  // Tabla solo: very free
            p.swingRatioSlow = 0.52f;
            p.swingRatioFast = 0.50f;
            p.ghostVelocityMin = 0.25f;
            p.ghostVelocityMax = 0.38f;
            break;

        case 5: // Gamelan
            p.baseVariance = 15.0f;
            p.roleMultipliers[0] = 0.2f;  // Gong: very precise
            p.roleMultipliers[1] = 0.3f;
            p.roleMultipliers[2] = 0.8f;
            p.roleMultipliers[3] = 1.0f;
            p.swingRatioSlow = 0.50f;     // Straight
            p.swingRatioFast = 0.50f;
            p.ghostVelocityMin = 0.20f;
            p.ghostVelocityMax = 0.35f;
            break;

        case 6: // Jazz
            p.baseVariance = 15.0f;
            p.roleMultipliers[0] = 0.4f;  // Ride: somewhat tight
            p.roleMultipliers[1] = 0.6f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.4f;  // Solo: free
            p.swingRatioSlow = 0.67f;     // Heavy swing at slow tempo (65-70% per spec)
            p.swingRatioFast = 0.54f;     // Nearly straight at fast tempo (bebop)
            p.ghostVelocityMin = 0.25f;
            p.ghostVelocityMax = 0.40f;
            break;

        case 7: // Electronic
            p.baseVariance = 3.0f;        // Machine precision
            p.roleMultipliers[0] = 0.5f;
            p.roleMultipliers[1] = 0.5f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.5f;
            p.swingRatioSlow = 0.50f;     // Straight
            p.swingRatioFast = 0.50f;
            p.ghostVelocityMin = 0.30f;
            p.ghostVelocityMax = 0.45f;
            break;

        case 8: // Breakbeat
            p.baseVariance = 12.0f;
            p.roleMultipliers[0] = 0.5f;
            p.roleMultipliers[1] = 0.7f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.3f;
            p.swingRatioSlow = 0.55f;
            p.swingRatioFast = 0.52f;
            p.ghostVelocityMin = 0.35f;   // Pronounced ghost notes
            p.ghostVelocityMax = 0.50f;
            break;

        case 9: // Techno
            p.baseVariance = 2.0f;        // Extreme precision
            p.roleMultipliers[0] = 0.3f;
            p.roleMultipliers[1] = 0.3f;
            p.roleMultipliers[2] = 0.8f;
            p.roleMultipliers[3] = 1.2f;
            p.swingRatioSlow = 0.50f;
            p.swingRatioFast = 0.50f;
            p.ghostVelocityMin = 0.35f;
            p.ghostVelocityMax = 0.50f;
            break;

        default:
            p.baseVariance = 10.0f;
            p.roleMultipliers[0] = 0.5f;
            p.roleMultipliers[1] = 0.6f;
            p.roleMultipliers[2] = 1.0f;
            p.roleMultipliers[3] = 1.2f;
            p.swingRatioSlow = 0.55f;
            p.swingRatioFast = 0.52f;
            p.ghostVelocityMin = 0.30f;
            p.ghostVelocityMax = 0.40f;
    }

    return p;
}

// ========================================
// Humanize Engine Class
// ========================================
class HumanizeEngine {
private:
    // v0.18.6: 使用 mutable 允許 const 成員函數修改 rng 狀態
    mutable std::mt19937 rng;

    // Groove templates by style
    std::vector<GrooveTemplate> grooveTemplates;
    int currentGrooveIndex = 0;

    // Style-specific timing (v0.16)
    int currentStyleIndex = 0;
    float currentBPM = 120.0f;
    StyleTimingProfile currentTimingProfile;

    // Manual swing override (0-1, where 0.5 = straight, higher = more swing)
    float manualSwingAmount = -1.0f;  // -1 = use automatic BPM-based swing

    // Previous note tracking for relative ghost velocity
    float previousVelocity = 0.7f;

    // Hand dynamics
    float dominantHandBoost = 0.08f;    // +8% for dominant hand
    float nonDominantVariance = 0.05f;  // More variance for non-dominant

    // Error parameters
    float errorProbability = 0.015f;    // 1.5% chance of error
    float missProbability = 0.005f;     // 0.5% chance of complete miss
    float flamProbability = 0.01f;      // 1% accidental flam

    // Long-term dynamics state
    float sectionDynamicMod = 1.0f;
    float pieceDynamicMod = 1.0f;
    int currentSection = 0;
    int totalSections = 4;

public:
    HumanizeEngine() : rng(std::random_device{}()) {
        grooveTemplates.push_back(createStraightGroove());
        grooveTemplates.push_back(createSwingGroove());
        grooveTemplates.push_back(createAfricanGroove());
        grooveTemplates.push_back(createLatinGroove());
        grooveTemplates.push_back(createLaidBackGroove());
        grooveTemplates.push_back(createPushedGroove());

        // Initialize timing profile
        currentTimingProfile = getStyleTimingProfile(0);
    }

    void seed(unsigned int s) { rng.seed(s); }

    // ========================================
    // Style and BPM Configuration (v0.16)
    // ========================================
    void setStyle(int styleIndex) {
        currentStyleIndex = styleIndex;
        currentTimingProfile = getStyleTimingProfile(styleIndex);
        setGrooveForStyle(styleIndex);
    }

    void setBPM(float bpm) {
        currentBPM = std::clamp(bpm, 40.0f, 300.0f);
    }

    // Set manual swing amount (0.0 = straight, 0.5 = default, 1.0 = maximum swing)
    // Pass -1 to use automatic BPM-based swing
    void setSwing(float amount) {
        if (amount < 0.0f) {
            manualSwingAmount = -1.0f;  // Use automatic
        } else {
            manualSwingAmount = std::clamp(amount, 0.0f, 1.0f);
        }
    }

    // Get current swing amount (for display)
    float getSwingAmount() const {
        if (manualSwingAmount >= 0.0f) {
            return manualSwingAmount;
        }
        // Convert ratio to 0-1 range: 0.5 (straight) = 0.0, 0.67 (max swing) = 1.0
        float ratio = getDynamicSwingRatio();
        return (ratio - 0.5f) * 2.0f / 0.34f;  // 0.34 = 0.67 - 0.5 - 0.17/2 (typical range)
    }

    // ========================================
    // v0.18.2: 完整的 BPM-Dependent Swing 系統
    // ========================================

    // Swing 曲線類型（不同風格有不同的 BPM-swing 關係）
    enum class SwingCurveType {
        LINEAR,      // 線性衰減（標準）
        EXPONENTIAL, // 指數衰減（Jazz - 快速轉直）
        STEPPED,     // 階梯式（傳統音樂 - 在特定 BPM 跳變）
        PLATEAU,     // 平台式（維持 swing 到高 BPM 才下降）
        CUSTOM       // 自訂曲線
    };

    // 獲取風格的 swing 曲線類型
    SwingCurveType getSwingCurveType(int styleIndex) const {
        switch (styleIndex) {
            case 0:  // West African - 維持 swing feel 到高速
                return SwingCurveType::PLATEAU;
            case 1:  // Afro-Cuban - 傳統階梯式
                return SwingCurveType::STEPPED;
            case 2:  // Brazilian - 線性
                return SwingCurveType::LINEAR;
            case 6:  // Jazz - 經典指數衰減（bebop 特徵）
                return SwingCurveType::EXPONENTIAL;
            case 8:  // Breakbeat - 線性
                return SwingCurveType::LINEAR;
            default:
                return SwingCurveType::LINEAR;
        }
    }

    // Get BPM-aware swing ratio（v0.18.2 增強版）
    float getDynamicSwingRatio() const {
        // Check for manual swing override
        if (manualSwingAmount >= 0.0f) {
            // Convert 0-1 range to swing ratio: 0.0 = 0.5 (straight), 1.0 = 0.67 (max swing)
            return 0.5f + manualSwingAmount * 0.17f;
        }

        SwingCurveType curveType = getSwingCurveType(currentStyleIndex);

        // 定義 BPM 閾值
        float slowThreshold = 100.0f;
        float mediumThreshold = 140.0f;
        float fastThreshold = 180.0f;

        float slowSwing = currentTimingProfile.swingRatioSlow;
        float fastSwing = currentTimingProfile.swingRatioFast;

        if (currentBPM <= slowThreshold) {
            return slowSwing;
        }
        if (currentBPM >= fastThreshold) {
            return fastSwing;
        }

        // 根據曲線類型計算 swing ratio
        float t = (currentBPM - slowThreshold) / (fastThreshold - slowThreshold);
        float swingRange = slowSwing - fastSwing;

        switch (curveType) {
            case SwingCurveType::LINEAR:
                // 標準線性插值
                return slowSwing - t * swingRange;

            case SwingCurveType::EXPONENTIAL:
                // 指數衰減：Jazz bebop 風格
                // 在中速就已經接近直拍
                {
                    float expT = 1.0f - std::exp(-3.0f * t);  // 快速衰減
                    return slowSwing - expT * swingRange;
                }

            case SwingCurveType::STEPPED:
                // 階梯式：Afro-Cuban 傳統
                // < 120: 完整 swing
                // 120-160: 中等 swing
                // > 160: 接近直拍
                if (currentBPM < 120.0f) {
                    return slowSwing;
                } else if (currentBPM < 160.0f) {
                    return slowSwing - swingRange * 0.4f;  // 中間值
                } else {
                    return fastSwing;
                }

            case SwingCurveType::PLATEAU:
                // 平台式：West African 風格
                // 維持高 swing 到 160 BPM 才開始下降
                if (currentBPM < mediumThreshold) {
                    return slowSwing * 0.95f;  // 稍微減少
                } else {
                    float plateauT = (currentBPM - mediumThreshold) / (fastThreshold - mediumThreshold);
                    return slowSwing - plateauT * swingRange;
                }

            case SwingCurveType::CUSTOM:
            default:
                return slowSwing - t * swingRange;
        }
    }

    // 獲取更精確的 swing timing offset（毫秒）
    float getSwingTimingOffset(int step, float bpm) const {
        float swingRatio = getDynamicSwingRatio();

        // 只有 off-beat 位置有 swing
        if (step % 2 == 0) {
            return 0.0f;  // On-beat: 無偏移
        }

        // 計算一個 8 分音符的時長
        float eighthNoteDuration = 60000.0f / bpm / 2.0f;  // ms

        // Swing ratio 轉換為時間偏移
        // 0.5 = straight (0ms offset)
        // 0.67 = triplet swing (~1/3 of eighth note late)
        float swingOffset = (swingRatio - 0.5f) * 2.0f * eighthNoteDuration;

        return swingOffset;
    }

    // 獲取帶有 BPM 感知的完整 microtiming
    float getSwingAwareMicrotiming(int step, Role role, float amount, float bpm) const {
        const GrooveTemplate& groove = grooveTemplates[currentGrooveIndex];
        int pos = step % 16;

        // 基礎 groove 偏移
        float baseOffset = groove.offsets[pos] * amount;

        // BPM-aware swing offset
        float swingOffset = getSwingTimingOffset(step, bpm) * amount;

        // 將 swing 添加到 off-beat 位置
        if (pos % 2 == 1) {
            baseOffset += swingOffset;
        }

        // Style-specific variance
        float styleVariance = getStyleTimingVariance(role);

        // v0.18.6: 移除 const_cast，rng 已宣告為 mutable
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float randomOffset = dist(rng) * styleVariance * amount * 0.5f;

        return baseOffset + randomOffset;
    }

    // 檢查當前 BPM 是否在「swing 活躍區」
    bool isSwingActive() const {
        float swingRatio = getDynamicSwingRatio();
        // 當 swing ratio 與 0.5（straight）差距小於 0.03 時，視為無 swing
        return std::abs(swingRatio - 0.5f) >= 0.03f;
    }

    // 獲取 swing 強度描述（用於 UI 顯示）
    std::string getSwingIntensityDescription() const {
        float swingRatio = getDynamicSwingRatio();

        if (swingRatio >= 0.65f) return "Heavy";
        if (swingRatio >= 0.58f) return "Medium";
        if (swingRatio >= 0.53f) return "Light";
        return "Straight";
    }

    // 舊版相容：原始的簡單線性 swing ratio
    float getDynamicSwingRatioLegacy() const {
        float slowThreshold = 100.0f;
        float fastThreshold = 180.0f;

        if (currentBPM <= slowThreshold) {
            return currentTimingProfile.swingRatioSlow;
        }
        if (currentBPM >= fastThreshold) {
            return currentTimingProfile.swingRatioFast;
        }

        float t = (currentBPM - slowThreshold) / (fastThreshold - slowThreshold);
        return currentTimingProfile.swingRatioSlow +
               t * (currentTimingProfile.swingRatioFast - currentTimingProfile.swingRatioSlow);
    }

    // Get style-specific timing variance for a role
    float getStyleTimingVariance(Role role) const {
        int roleIndex = static_cast<int>(role);
        if (roleIndex < 0 || roleIndex > 3) roleIndex = 2;
        return currentTimingProfile.baseVariance * currentTimingProfile.roleMultipliers[roleIndex];
    }

    // ========================================
    // Groove Template Management
    // ========================================
    void setGrooveTemplate(int index) {
        if (index >= 0 && index < static_cast<int>(grooveTemplates.size())) {
            currentGrooveIndex = index;
        }
    }

    void setGrooveForStyle(int styleIndex) {
        switch (styleIndex) {
            case 0: currentGrooveIndex = 2; break;  // West African
            case 1: currentGrooveIndex = 3; break;  // Afro-Cuban
            case 2: currentGrooveIndex = 3; break;  // Brazilian
            case 3: currentGrooveIndex = 0; break;  // Balkan (straight)
            case 4: currentGrooveIndex = 0; break;  // Indian (straight)
            case 5: currentGrooveIndex = 0; break;  // Gamelan (straight)
            case 6: currentGrooveIndex = 1; break;  // Jazz (swing)
            case 7: currentGrooveIndex = 0; break;  // Electronic (straight)
            case 8: currentGrooveIndex = 4; break;  // Breakbeat (laid back)
            case 9: currentGrooveIndex = 0; break;  // Techno (straight)
            default: currentGrooveIndex = 0;
        }
    }

    const GrooveTemplate& getCurrentGroove() const {
        return grooveTemplates[currentGrooveIndex];
    }

    // ========================================
    // Microtiming with Groove Template (v0.16 enhanced)
    // ========================================
    float getGrooveMicrotiming(int step, Role role, float amount) const {
        const GrooveTemplate& groove = grooveTemplates[currentGrooveIndex];
        int pos = step % 16;

        // Base offset from groove template
        float baseOffset = groove.offsets[pos] * amount;

        // Apply BPM-aware swing to off-beat positions
        float swingRatio = getDynamicSwingRatio();
        if (pos % 2 == 1) {  // Off-beat positions (e, a)
            // Convert swing ratio to timing offset
            // 0.5 = straight (0ms), 0.67 = triplet (~20ms late at 120bpm)
            float swingOffset = (swingRatio - 0.5f) * 40.0f * amount;
            baseOffset += swingOffset;
        }

        // Style-specific variance (v0.16)
        float styleVariance = getStyleTimingVariance(role);

        // v0.18.6: 移除 const_cast，rng 已宣告為 mutable
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float randomOffset = dist(rng) * styleVariance * amount * 0.5f;

        return baseOffset + randomOffset;
    }

    // Get swing-aware timing for a specific step (for external use)
    float getSwingTiming(int step) const {
        float swingRatio = getDynamicSwingRatio();
        if (step % 2 == 1) {
            return swingRatio;
        }
        return 0.5f;  // On-beat is always at 50%
    }

    // ========================================
    // Velocity Layer Conversion
    // ========================================
    static VelocityLayer velocityToLayer(float velocity) {
        if (velocity < 0.12f) return VelocityLayer::PPPP;
        if (velocity < 0.20f) return VelocityLayer::PPP;
        if (velocity < 0.30f) return VelocityLayer::PP;
        if (velocity < 0.45f) return VelocityLayer::P;
        if (velocity < 0.55f) return VelocityLayer::MP;
        if (velocity < 0.70f) return VelocityLayer::MF;
        if (velocity < 0.82f) return VelocityLayer::F;
        if (velocity < 0.92f) return VelocityLayer::FF;
        return VelocityLayer::FFF;
    }

    static float layerToVelocity(VelocityLayer layer) {
        switch (layer) {
            case VelocityLayer::PPPP: return 0.08f;
            case VelocityLayer::PPP:  return 0.16f;
            case VelocityLayer::PP:   return 0.25f;
            case VelocityLayer::P:    return 0.37f;
            case VelocityLayer::MP:   return 0.50f;
            case VelocityLayer::MF:   return 0.62f;
            case VelocityLayer::F:    return 0.76f;
            case VelocityLayer::FF:   return 0.87f;
            case VelocityLayer::FFF:  return 0.96f;
            default: return 0.5f;
        }
    }

    // ========================================
    // Hand Dynamics
    // ========================================
    Hand assignHand(int step, Role role) const {
        if (role == FOUNDATION) {
            return Hand::FOOT;  // Kick drum
        }

        // Alternate hands, right hand on strong beats
        bool isStrongBeat = (step % 4 == 0);
        if (isStrongBeat) {
            return Hand::RIGHT;
        }
        return (step % 2 == 0) ? Hand::RIGHT : Hand::LEFT;
    }

    float applyHandDynamics(float velocity, Hand hand) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        switch (hand) {
            case Hand::RIGHT:  // Dominant
                return velocity * (1.0f + dominantHandBoost + dist(rng) * 0.02f);
            case Hand::LEFT:   // Non-dominant
                return velocity * (1.0f - dominantHandBoost * 0.5f + dist(rng) * nonDominantVariance);
            case Hand::FOOT:
                return velocity * (1.0f + dist(rng) * 0.03f);
            case Hand::BOTH:
                return velocity * (1.0f + dominantHandBoost * 0.5f);
            default:
                return velocity;
        }
    }

    // ========================================
    // Articulation Selection (v0.16 enhanced)
    // ========================================
    ArticulationType selectArticulation(float velocity, Role role, int /*step*/) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float r = dist(rng);

        // Ghost notes - use relative threshold based on previous velocity
        float ghostThreshold = previousVelocity * currentTimingProfile.ghostVelocityMax;
        if (velocity < ghostThreshold && velocity < 0.35f) {
            return ArticulationType::GHOST;
        }

        // Accents
        if (velocity > 0.85f) {
            return ArticulationType::ACCENT;
        }

        // Role-specific articulations
        if (role == GROOVE) {
            // Snare articulations
            if (r < 0.05f && velocity > 0.6f) return ArticulationType::RIM;
            if (r < 0.08f && velocity < 0.5f) return ArticulationType::CROSS;
        }

        // Occasional flam
        if (r < flamProbability && velocity > 0.5f) {
            return ArticulationType::FLAM;
        }

        return ArticulationType::NORMAL;
    }

    // ========================================
    // Relative Ghost Velocity (v0.16, v0.18 改進)
    // ========================================
    // Ghost notes are relative to the previous note's velocity
    // v0.18: 加入最小值保護，確保 ghost note 始終可聽
    static constexpr float GHOST_VELOCITY_MIN_ABSOLUTE = 0.08f;  // 最小絕對力度

    float calculateGhostVelocity(float /*baseVelocity*/) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float ratio = currentTimingProfile.ghostVelocityMin +
                     dist(rng) * (currentTimingProfile.ghostVelocityMax - currentTimingProfile.ghostVelocityMin);
        float ghostVel = previousVelocity * ratio;

        // v0.18: 確保 ghost note 始終有最小可聽力度
        return std::max(ghostVel, GHOST_VELOCITY_MIN_ABSOLUTE);
    }

    // Update previous velocity tracking
    void updatePreviousVelocity(float velocity) {
        if (velocity > 0.1f) {  // Only update for audible notes
            previousVelocity = velocity;
        }
    }

    void resetPreviousVelocity() {
        previousVelocity = 0.7f;  // Default to mf
    }

    // ========================================
    // Error Modeling
    // ========================================
    bool shouldHaveError() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < errorProbability;
    }

    bool shouldMiss() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < missProbability;
    }

    void applyError(NoteInfo& note) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float errorType = dist(rng);

        if (errorType < 0.4f) {
            // Timing error: larger offset
            note.microOffset += (dist(rng) - 0.5f) * 20.0f;
        } else if (errorType < 0.7f) {
            // Velocity error: wrong dynamics
            note.velocity *= 0.7f + dist(rng) * 0.6f;
        } else {
            // Accidental flam
            note.articulation = ArticulationType::FLAM;
        }

        note.hasError = true;
    }

    // ========================================
    // Long-term Dynamics
    // ========================================
    void setSection(int section, int total) {
        currentSection = section;
        totalSections = total;

        // Build up through piece
        float pieceProgress = static_cast<float>(section) / total;
        pieceDynamicMod = 0.9f + pieceProgress * 0.2f;  // 0.9 -> 1.1

        // Each section has internal arc
        sectionDynamicMod = 1.0f;
    }

    float getLongTermDynamicMod(int bar, int step, int phraseLength) const {
        // Bar within phrase
        int barInPhrase = bar % phraseLength;
        float phraseProgress = static_cast<float>(barInPhrase) / phraseLength;
        float stepProgress = static_cast<float>(step) / 16.0f;
        float total = phraseProgress + stepProgress / phraseLength;

        // Crescendo toward phrase end
        float phraseMod = 1.0f + total * 0.15f;

        // Last bar of phrase: more dramatic
        if (barInPhrase == phraseLength - 1) {
            phraseMod += stepProgress * 0.1f;
        }

        // Combine all levels
        return phraseMod * sectionDynamicMod * pieceDynamicMod;
    }

    // ========================================
    // Complete Humanization Pipeline (v0.16 enhanced)
    // ========================================
    NoteInfo humanizeNote(float velocity, int step, Role role, int bar, int phraseLength) {
        NoteInfo note;

        // Check for complete miss first
        if (shouldMiss()) {
            note.velocity = 0.0f;
            note.hasError = true;
            return note;
        }

        // Base velocity with groove template modifier
        const GrooveTemplate& groove = grooveTemplates[currentGrooveIndex];
        int pos = step % 16;
        note.velocity = velocity * groove.velMods[pos];

        // Apply long-term dynamics
        note.velocity *= getLongTermDynamicMod(bar, step, phraseLength);

        // Assign hand
        note.hand = assignHand(step, role);

        // Apply hand dynamics
        note.velocity = applyHandDynamics(note.velocity, note.hand);

        // Check if this should be a ghost note (relative to previous velocity)
        ArticulationType articulationCheck = selectArticulation(note.velocity, role, step);
        if (articulationCheck == ArticulationType::GHOST) {
            // Calculate ghost velocity relative to previous note
            note.velocity = calculateGhostVelocity(note.velocity);
        }

        // Clamp velocity
        note.velocity = std::clamp(note.velocity, 0.05f, 1.0f);

        // Update previous velocity for next ghost calculation
        updatePreviousVelocity(note.velocity);

        // Determine layer
        note.layer = velocityToLayer(note.velocity);

        // Determine articulation (final)
        note.articulation = articulationCheck;

        // Calculate microtiming
        note.microOffset = getGrooveMicrotiming(step, role, 0.7f);

        // Check for error
        note.hasError = false;
        if (shouldHaveError()) {
            applyError(note);
        }

        return note;
    }

    // v0.16: Enhanced humanizeNote with BPM parameter
    NoteInfo humanizeNoteWithBPM(float velocity, int step, Role role, int bar, int phraseLength, float bpm) {
        setBPM(bpm);
        return humanizeNote(velocity, step, role, bar, phraseLength);
    }

    // ========================================
    // Apply humanization to entire pattern (v0.16 enhanced)
    // ========================================
    void humanizePattern(Pattern& p, Role role, int bar, int phraseLength) {
        resetPreviousVelocity();  // Start fresh for each pattern

        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                NoteInfo note = humanizeNote(p.getVelocity(i), i, role, bar, phraseLength);

                if (note.velocity < 0.05f) {
                    // Missed note
                    p.clearOnset(i);
                } else {
                    p.setOnset(i, note.velocity);
                    p.accents[i] = (note.articulation == ArticulationType::ACCENT ||
                                   note.articulation == ArticulationType::RIM);
                }
            }
        }
    }

    // v0.16: Humanize with style and BPM
    void humanizePatternWithContext(Pattern& p, Role role, int bar, int phraseLength,
                                    int styleIndex, float bpm) {
        setStyle(styleIndex);
        setBPM(bpm);
        humanizePattern(p, role, bar, phraseLength);
    }

    // ========================================
    // Flam Generation
    // ========================================
    struct FlamInfo {
        float graceVelocity;   // Grace note velocity (30-50% of main)
        float graceOffset;     // Grace note timing offset (-50 to -20 ms)
    };

    FlamInfo generateFlam(float mainVelocity) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        FlamInfo flam;
        flam.graceVelocity = mainVelocity * (0.3f + dist(rng) * 0.2f);
        flam.graceOffset = -50.0f + dist(rng) * 30.0f;  // -50 to -20 ms

        return flam;
    }

    // ========================================
    // Ruff Generation (v0.17)
    // 3-stroke ruff: 三個裝飾音 + 主音
    // 時序：grace1 -> grace2 -> grace3 -> main
    // ========================================
    struct RuffInfo {
        float graceVelocities[3];  // 三個裝飾音力度（遞增）
        float graceOffsets[3];     // 三個裝飾音時序偏移
        float mainVelocity;        // 主音力度
    };

    // 舊版 generateRuff（無 BPM，保留向後相容）
    RuffInfo generateRuff(float mainVelocity) {
        return generateRuffWithBPM(mainVelocity, 120.0f);  // 預設 120 BPM
    }

    // BPM-aware Ruff 生成（v0.18 改進）
    RuffInfo generateRuffWithBPM(float mainVelocity, float bpm) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        RuffInfo ruff;
        ruff.mainVelocity = mainVelocity;

        // 裝飾音力度遞增（30% -> 40% -> 50%）
        ruff.graceVelocities[0] = mainVelocity * (0.25f + dist(rng) * 0.1f) + velVar(rng);
        ruff.graceVelocities[1] = mainVelocity * (0.35f + dist(rng) * 0.1f) + velVar(rng);
        ruff.graceVelocities[2] = mainVelocity * (0.45f + dist(rng) * 0.1f) + velVar(rng);

        // Clamp velocities
        for (int i = 0; i < 3; i++) {
            ruff.graceVelocities[i] = std::clamp(ruff.graceVelocities[i], 0.1f, 1.0f);
        }

        // BPM-aware 時序計算
        // 一個 beat = 60000 / bpm ms
        // Ruff 通常佔用 32 分音符的時間
        float beatDuration = 60000.0f / bpm;
        float note32Duration = beatDuration / 8.0f;  // 32 分音符

        // 最大間距限制：Ruff 不應超過一個 16 分音符（避免與前一個音符衝突）
        float maxTotalDuration = beatDuration / 4.0f;  // 16 分音符

        // 基礎間距：根據 BPM 動態調整
        // 慢速（< 100 BPM）：使用較長間距，讓 ruff 更明顯
        // 中速（100-160 BPM）：標準間距
        // 快速（> 160 BPM）：壓縮間距，避免 ruff 延伸到前一拍
        float baseSpacing;
        if (bpm < 100.0f) {
            baseSpacing = std::min(50.0f, note32Duration * 0.8f);
        } else if (bpm > 160.0f) {
            baseSpacing = std::min(25.0f, note32Duration * 0.6f);
        } else {
            // 線性插值
            float t = (bpm - 100.0f) / 60.0f;  // 0 at 100, 1 at 160
            float lowSpacing = std::min(45.0f, note32Duration * 0.75f);
            float highSpacing = std::min(30.0f, note32Duration * 0.65f);
            baseSpacing = lowSpacing * (1.0f - t) + highSpacing * t;
        }

        // 添加隨機變化
        baseSpacing += dist(rng) * 8.0f - 4.0f;  // ±4ms 變化

        // 確保總時長不超過最大值
        float totalDuration = baseSpacing * 3.0f;
        if (totalDuration > maxTotalDuration) {
            baseSpacing = maxTotalDuration / 3.0f;
        }

        // 最小間距保護（確保 ruff 可辨識）
        baseSpacing = std::max(15.0f, baseSpacing);

        // 時序偏移（主音在 0ms，裝飾音在前）
        ruff.graceOffsets[0] = -baseSpacing * 3.0f + dist(rng) * 3.0f;
        ruff.graceOffsets[1] = -baseSpacing * 2.0f + dist(rng) * 3.0f;
        ruff.graceOffsets[2] = -baseSpacing * 1.0f + dist(rng) * 3.0f;

        return ruff;
    }

    // ========================================
    // Drag Generation (v0.19)
    // 兩個裝飾音 + 主音
    // 總時長約 40ms
    // ========================================
    struct DragInfo {
        float graceVelocities[2];  // 兩個裝飾音力度
        float graceOffsets[2];     // 兩個裝飾音時序偏移
        float mainVelocity;        // 主音力度
    };

    DragInfo generateDrag(float mainVelocity) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        DragInfo drag;
        drag.mainVelocity = mainVelocity;

        // 裝飾音力度：30-40% 和 40-50% of main
        drag.graceVelocities[0] = mainVelocity * (0.30f + dist(rng) * 0.1f);
        drag.graceVelocities[1] = mainVelocity * (0.40f + dist(rng) * 0.1f);

        // Clamp velocities
        drag.graceVelocities[0] = std::clamp(drag.graceVelocities[0], 0.1f, 1.0f);
        drag.graceVelocities[1] = std::clamp(drag.graceVelocities[1], 0.1f, 1.0f);

        // 時序：總時長約 40ms，裝飾音在前
        // Grace1: -40 to -35 ms, Grace2: -20 to -15 ms, Main: 0 ms
        drag.graceOffsets[0] = -40.0f + dist(rng) * 5.0f;  // -40 to -35 ms
        drag.graceOffsets[1] = -20.0f + dist(rng) * 5.0f;  // -20 to -15 ms

        return drag;
    }

    // ========================================
    // Buzz Generation (v0.19)
    // 4-6 個快速連續觸發（Buzz roll）
    // 每個約 8ms 間隔
    // ========================================
    struct BuzzInfo {
        int numStrokes;         // 4-6 個觸發
        float velocities[6];    // 各觸發力度（略遞增）
        float offsets[6];       // 時序偏移
    };

    BuzzInfo generateBuzz(float mainVelocity) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_int_distribution<int> numDist(4, 6);

        BuzzInfo buzz;
        buzz.numStrokes = numDist(rng);

        // 每個觸發約 8ms 間隔
        float baseInterval = 8.0f;

        for (int i = 0; i < buzz.numStrokes; i++) {
            // 力度略遞增（50% -> 65%）
            float ratio = 0.50f + (static_cast<float>(i) / buzz.numStrokes) * 0.15f;
            buzz.velocities[i] = mainVelocity * ratio + dist(rng) * 0.05f;
            buzz.velocities[i] = std::clamp(buzz.velocities[i], 0.15f, 1.0f);

            // 時序：每個間隔約 8ms ± 1ms
            buzz.offsets[i] = i * baseInterval + (dist(rng) - 0.5f) * 2.0f;
        }

        return buzz;
    }

    // ========================================
    // Paradiddle Generation (v0.17)
    // RLRR / LRLL sticking pattern
    // 四個音符，交替手順
    // ========================================
    struct ParadiddleInfo {
        Hand hands[4];         // 手順：RLRR 或 LRLL
        float velocities[4];   // 各音力度（首音最強）
        float offsets[4];      // 時序偏移（等間距）
        bool isRightStart;     // true = RLRR, false = LRLL
    };

    ParadiddleInfo generateParadiddle(float mainVelocity, bool rightStart = true) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        ParadiddleInfo para;
        para.isRightStart = rightStart;

        // 手順
        if (rightStart) {
            // RLRR
            para.hands[0] = Hand::RIGHT;
            para.hands[1] = Hand::LEFT;
            para.hands[2] = Hand::RIGHT;
            para.hands[3] = Hand::RIGHT;
        } else {
            // LRLL
            para.hands[0] = Hand::LEFT;
            para.hands[1] = Hand::RIGHT;
            para.hands[2] = Hand::LEFT;
            para.hands[3] = Hand::LEFT;
        }

        // 力度：首音重音，其他較輕
        para.velocities[0] = mainVelocity * 1.0f + velVar(rng);                      // 重音
        para.velocities[1] = mainVelocity * (0.65f + dist(rng) * 0.1f) + velVar(rng);
        para.velocities[2] = mainVelocity * (0.55f + dist(rng) * 0.1f) + velVar(rng);
        para.velocities[3] = mainVelocity * (0.60f + dist(rng) * 0.1f) + velVar(rng);

        // 時序：16 分音符間距（約 125ms @ 120BPM）
        // 這裡使用相對偏移，實際值需根據 BPM 調整
        float spacing = 62.5f;  // 32 分音符 @ 120BPM
        para.offsets[0] = 0.0f;
        para.offsets[1] = spacing + dist(rng) * 5.0f;
        para.offsets[2] = spacing * 2.0f + dist(rng) * 5.0f;
        para.offsets[3] = spacing * 3.0f + dist(rng) * 5.0f;

        return para;
    }

    // 根據 BPM 調整 Paradiddle 間距
    ParadiddleInfo generateParadiddleWithBPM(float mainVelocity, float bpm, bool rightStart = true) {
        ParadiddleInfo para = generateParadiddle(mainVelocity, rightStart);

        // 重新計算間距（32 分音符）
        float beatDuration = 60000.0f / bpm;  // ms per beat
        float spacing = beatDuration / 8.0f;   // 32nd note

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        para.offsets[0] = 0.0f;
        para.offsets[1] = spacing + dist(rng) * (spacing * 0.1f);
        para.offsets[2] = spacing * 2.0f + dist(rng) * (spacing * 0.1f);
        para.offsets[3] = spacing * 3.0f + dist(rng) * (spacing * 0.1f);

        return para;
    }

    // ========================================
    // Accessors (v0.16 enhanced)
    // ========================================
    void setDominantHandBoost(float boost) {
        dominantHandBoost = std::clamp(boost, 0.0f, 0.2f);
    }

    void setErrorProbability(float prob) {
        errorProbability = std::clamp(prob, 0.0f, 0.1f);
    }

    int getNumGrooveTemplates() const {
        return static_cast<int>(grooveTemplates.size());
    }

    const char* getGrooveTemplateName(int index) const {
        if (index >= 0 && index < static_cast<int>(grooveTemplates.size())) {
            return grooveTemplates[index].name.c_str();
        }
        return "Unknown";
    }

    // v0.16 accessors
    int getCurrentStyle() const { return currentStyleIndex; }
    float getCurrentBPM() const { return currentBPM; }
    const StyleTimingProfile& getCurrentTimingProfile() const { return currentTimingProfile; }

    // Style timing info for display
    static const char* getStyleName(int index) {
        static const char* names[] = {
            "West African", "Afro-Cuban", "Brazilian", "Balkan", "Indian",
            "Gamelan", "Jazz", "Electronic", "Breakbeat", "Techno"
        };
        if (index >= 0 && index < 10) return names[index];
        return "Unknown";
    }

    // Get timing variance description
    std::string getTimingDescription() const {
        float variance = currentTimingProfile.baseVariance;
        float swingRatio = getDynamicSwingRatio();

        std::string desc = getStyleName(currentStyleIndex);
        desc += " @ " + std::to_string(static_cast<int>(currentBPM)) + " BPM: ";

        if (variance < 5.0f) desc += "Machine precision";
        else if (variance < 10.0f) desc += "Tight";
        else if (variance < 18.0f) desc += "Human";
        else if (variance < 25.0f) desc += "Loose";
        else desc += "Very loose";

        desc += ", Swing: " + std::to_string(static_cast<int>(swingRatio * 100)) + "%";

        return desc;
    }
};

} // namespace WorldRhythm
