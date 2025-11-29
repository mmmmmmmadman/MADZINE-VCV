#pragma once

#include <vector>
#include <string>
#include <cmath>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Irama (Tempo Density) System
// ========================================
// Implements Javanese Gamelan concept of Irama
// Based on unified_rhythm_analysis.md Section 3 Type C
//
// Irama is NOT tempo change - it's density change at same tempo
// As irama level increases, more subdivisions fill the same beat
//
// Level   Name          Pulses/Beat   Feel
// I       Lancar        1             Sparse
// II      Tanggung      2             Standard
// III     Dados         4             Ornate
// IV      Wiled         8             Very ornate
// V       Rangkep       16            Extremely dense

enum class IramaLevel {
    LANCAR = 0,     // 1 pulse per beat (sparse)
    TANGGUNG,       // 2 pulses per beat (standard)
    DADOS,          // 4 pulses per beat (ornate)
    WILED,          // 8 pulses per beat (very ornate)
    RANGKEP,        // 16 pulses per beat (extremely dense)
    NUM_LEVELS
};

struct IramaDefinition {
    IramaLevel level;
    std::string name;
    int pulsesPerBeat;
    float densityMultiplier;    // How much busier patterns become
    float velocityRange;        // Dynamic range in this irama
    std::string description;
};

// ========================================
// Irama Level Definitions
// ========================================
inline IramaDefinition createIramaLancar() {
    IramaDefinition i;
    i.level = IramaLevel::LANCAR;
    i.name = "Lancar";
    i.pulsesPerBeat = 1;
    i.densityMultiplier = 0.25f;
    i.velocityRange = 0.4f;     // Limited dynamics
    i.description = "Sparse, one note per beat, processional";
    return i;
}

inline IramaDefinition createIramaTanggung() {
    IramaDefinition i;
    i.level = IramaLevel::TANGGUNG;
    i.name = "Tanggung";
    i.pulsesPerBeat = 2;
    i.densityMultiplier = 0.5f;
    i.velocityRange = 0.5f;
    i.description = "Standard density, two subdivisions per beat";
    return i;
}

inline IramaDefinition createIramaDados() {
    IramaDefinition i;
    i.level = IramaLevel::DADOS;
    i.name = "Dados";
    i.pulsesPerBeat = 4;
    i.densityMultiplier = 1.0f;
    i.velocityRange = 0.65f;
    i.description = "Ornate, four subdivisions, full patterns";
    return i;
}

inline IramaDefinition createIramaWiled() {
    IramaDefinition i;
    i.level = IramaLevel::WILED;
    i.name = "Wiled";
    i.pulsesPerBeat = 8;
    i.densityMultiplier = 1.5f;
    i.velocityRange = 0.8f;
    i.description = "Very ornate, eight subdivisions, elaborate";
    return i;
}

inline IramaDefinition createIramaRangkep() {
    IramaDefinition i;
    i.level = IramaLevel::RANGKEP;
    i.name = "Rangkep";
    i.pulsesPerBeat = 16;
    i.densityMultiplier = 2.0f;
    i.velocityRange = 1.0f;     // Full dynamic range
    i.description = "Extremely dense, sixteen subdivisions, virtuosic";
    return i;
}

// ========================================
// Irama Engine Class
// ========================================
class IramaEngine {
private:
    std::vector<IramaDefinition> levels;
    IramaLevel currentLevel = IramaLevel::DADOS;  // Default to middle level
    float transitionProgress = 0.0f;  // For smooth transitions
    IramaLevel targetLevel = IramaLevel::DADOS;

public:
    IramaEngine() {
        levels.push_back(createIramaLancar());
        levels.push_back(createIramaTanggung());
        levels.push_back(createIramaDados());
        levels.push_back(createIramaWiled());
        levels.push_back(createIramaRangkep());
    }

    void setLevel(IramaLevel level) {
        currentLevel = level;
        targetLevel = level;
        transitionProgress = 1.0f;
    }

    void setLevelByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(IramaLevel::NUM_LEVELS)) {
            setLevel(static_cast<IramaLevel>(index));
        }
    }

    IramaLevel getCurrentLevel() const { return currentLevel; }
    const IramaDefinition& getCurrentDefinition() const {
        return levels[static_cast<int>(currentLevel)];
    }

    int getNumLevels() const { return static_cast<int>(levels.size()); }

    // ========================================
    // Start transition to new irama level
    // ========================================
    void startTransition(IramaLevel target) {
        if (target != currentLevel) {
            targetLevel = target;
            transitionProgress = 0.0f;
        }
    }

    // ========================================
    // Update transition progress
    // ========================================
    void updateTransition(float deltaProgress) {
        if (transitionProgress < 1.0f) {
            transitionProgress += deltaProgress;
            if (transitionProgress >= 1.0f) {
                transitionProgress = 1.0f;
                currentLevel = targetLevel;
            }
        }
    }

    bool isTransitioning() const {
        return transitionProgress < 1.0f;
    }

    float getTransitionProgress() const {
        return transitionProgress;
    }

    // ========================================
    // Get effective density multiplier (with transition)
    // ========================================
    float getEffectiveDensityMultiplier() const {
        float currentMult = levels[static_cast<int>(currentLevel)].densityMultiplier;
        float targetMult = levels[static_cast<int>(targetLevel)].densityMultiplier;

        // Smooth interpolation
        return currentMult + (targetMult - currentMult) * transitionProgress;
    }

    // ========================================
    // Get effective pulses per beat (with transition)
    // ========================================
    int getEffectivePulsesPerBeat() const {
        int currentPulses = levels[static_cast<int>(currentLevel)].pulsesPerBeat;
        int targetPulses = levels[static_cast<int>(targetLevel)].pulsesPerBeat;

        // During transition, use maximum to avoid note loss
        if (isTransitioning()) {
            return std::max(currentPulses, targetPulses);
        }
        return currentPulses;
    }

    // ========================================
    // Apply irama to pattern
    // Adjusts density based on current irama level
    // v0.18.3: 修正除零風險
    // v0.18.7: 加強極端長度檢查
    // ========================================
    void applyIrama(Pattern& p, float /*baseDensity*/) {
        const IramaDefinition& irama = getCurrentDefinition();

        // v0.18.7: 安全檢查：確保 pattern 長度有效且足夠進行 irama 處理
        // 當 length < 4 時，無法正確計算 beatsPerPattern，直接返回
        if (p.length < 4) return;

        // Adjust pattern based on irama
        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                // Check if this position should exist at current irama

                // In lower irama levels, remove subdivisions
                if (irama.pulsesPerBeat < 4) {
                    // Only keep beats that fall on main pulses
                    // v0.18.3: 修正除零風險
                    // 計算每拍的 step 數（至少為 1）
                    int beatsPerPattern = std::max(1, p.length / 4);
                    int pulsesInPattern = std::max(1, beatsPerPattern * irama.pulsesPerBeat);
                    int stepPerPulse = std::max(1, p.length / pulsesInPattern);

                    if (i % stepPerPulse != 0) {
                        // Reduce velocity for off-pulse notes in sparse irama
                        float vel = p.getVelocity(i) * 0.5f;
                        if (vel < 0.25f) {
                            p.clearOnset(i);
                        } else {
                            p.setOnset(i, vel);
                        }
                    }
                }

                // Adjust velocity range based on irama
                float vel = p.getVelocity(i);
                float minVel = 0.3f + (1.0f - irama.velocityRange) * 0.3f;
                float maxVel = minVel + irama.velocityRange * 0.7f;
                vel = minVel + (vel - 0.3f) / 0.7f * (maxVel - minVel);
                p.setOnset(i, std::clamp(vel, minVel, maxVel));
            }
        }
    }

    // ========================================
    // Generate irama-appropriate pattern length
    // ========================================
    int getRecommendedLength(int baseLength) const {
        const IramaDefinition& irama = getCurrentDefinition();

        // Higher irama = longer patterns to fit more subdivisions
        switch (irama.level) {
            case IramaLevel::LANCAR:
                return baseLength / 4;
            case IramaLevel::TANGGUNG:
                return baseLength / 2;
            case IramaLevel::DADOS:
                return baseLength;
            case IramaLevel::WILED:
                return baseLength * 2;
            case IramaLevel::RANGKEP:
                return baseLength * 4;
            default:
                return baseLength;
        }
    }

    // ========================================
    // Generate colotomic structure for irama
    // (Gamelan gong pattern)
    // v0.18.8: 加入 cycleLength 最小值檢查
    // ========================================
    std::vector<int> getColotomicStructure(int cycleLength) const {
        std::vector<int> structure;
        const IramaDefinition& irama = getCurrentDefinition();

        // v0.18.8: 確保 cycleLength 足夠大，避免負數索引
        if (cycleLength < 8) {
            // 對於極短的循環，只返回末尾位置
            return {std::max(0, cycleLength - 1)};
        }

        // Gong positions depend on irama density
        // Lancar: gong at end only
        // Higher irama: more intermediate gongs

        switch (irama.level) {
            case IramaLevel::LANCAR:
                structure = {cycleLength - 1};  // Gong at end
                break;
            case IramaLevel::TANGGUNG:
                structure = {cycleLength / 2 - 1, cycleLength - 1};
                break;
            case IramaLevel::DADOS:
                structure = {cycleLength / 4 - 1, cycleLength / 2 - 1,
                            cycleLength * 3 / 4 - 1, cycleLength - 1};
                break;
            case IramaLevel::WILED:
            case IramaLevel::RANGKEP:
                // More frequent punctuation
                for (int i = 1; i <= 8; i++) {
                    structure.push_back(cycleLength * i / 8 - 1);
                }
                break;
            default:
                structure = {cycleLength - 1};
        }

        return structure;
    }

    // ========================================
    // Get kotekan density for irama
    // (Interlocking pattern density)
    // ========================================
    float getKotekanDensity() const {
        const IramaDefinition& irama = getCurrentDefinition();

        // Kotekan becomes denser at higher irama
        switch (irama.level) {
            case IramaLevel::LANCAR:
                return 0.0f;    // No kotekan at this level
            case IramaLevel::TANGGUNG:
                return 0.3f;    // Light kotekan
            case IramaLevel::DADOS:
                return 0.6f;    // Standard kotekan
            case IramaLevel::WILED:
                return 0.8f;    // Dense kotekan
            case IramaLevel::RANGKEP:
                return 1.0f;    // Maximum kotekan
            default:
                return 0.5f;
        }
    }

    // ========================================
    // Get level name
    // ========================================
    const char* getLevelName(IramaLevel level) const {
        return levels[static_cast<int>(level)].name.c_str();
    }

    const char* getCurrentLevelName() const {
        return getLevelName(currentLevel);
    }

    // ========================================
    // Calculate next irama (for automatic progression)
    // ========================================
    IramaLevel getNextLevel(bool ascending) const {
        int current = static_cast<int>(currentLevel);
        if (ascending) {
            return static_cast<IramaLevel>(
                std::min(current + 1, static_cast<int>(IramaLevel::NUM_LEVELS) - 1));
        } else {
            return static_cast<IramaLevel>(std::max(current - 1, 0));
        }
    }

    // ========================================
    // Get irama for style and intensity
    // ========================================
    IramaLevel getRecommendedIrama(int styleIndex, float intensity) const {
        // Base irama from intensity
        int baseLevel;
        if (intensity < 0.2f) baseLevel = 0;
        else if (intensity < 0.4f) baseLevel = 1;
        else if (intensity < 0.6f) baseLevel = 2;
        else if (intensity < 0.8f) baseLevel = 3;
        else baseLevel = 4;

        // Adjust for style
        if (styleIndex == 5) {  // Gamelan
            // Gamelan uses full range
            return static_cast<IramaLevel>(baseLevel);
        } else if (styleIndex == 7 || styleIndex == 9) {  // Electronic/Techno
            // Electronic tends to stay in middle range
            return static_cast<IramaLevel>(std::clamp(baseLevel, 1, 3));
        } else {
            // Other styles: map to equivalent density
            return static_cast<IramaLevel>(std::clamp(baseLevel, 1, 3));
        }
    }
};

// ========================================
// Metric Modulation Types (v0.18)
// ========================================
// 通用的節拍調變系統，適用於所有風格
enum class MetricModulationType {
    NONE,           // 無調變
    HALF_TIME,      // 半速 (2x 時值)
    DOUBLE_TIME,    // 倍速 (0.5x 時值)
    TRIPLET_FEEL,   // 三連音感
    DOTTED_FEEL,    // 附點感 (3:2 比例)
    SWING_TO_STRAIGHT,  // Swing → Straight
    STRAIGHT_TO_SWING,  // Straight → Swing
    INDIAN_LAYA_VILAMBIT,  // 印度慢速 (Vilambit)
    INDIAN_LAYA_MADHYA,    // 印度中速 (Madhya)
    INDIAN_LAYA_DRUT       // 印度快速 (Drut)
};

// ========================================
// Metric Modulation Engine (v0.18)
// ========================================
class MetricModulationEngine {
private:
    MetricModulationType currentModulation = MetricModulationType::NONE;
    MetricModulationType targetModulation = MetricModulationType::NONE;
    float transitionProgress = 1.0f;
    float modulationStrength = 1.0f;  // 調變強度 (0-1)

public:
    MetricModulationEngine() = default;

    // ========================================
    // Set modulation
    // ========================================
    void setModulation(MetricModulationType type, float strength = 1.0f) {
        currentModulation = type;
        targetModulation = type;
        modulationStrength = std::clamp(strength, 0.0f, 1.0f);
        transitionProgress = 1.0f;
    }

    void startTransition(MetricModulationType target, float strength = 1.0f) {
        if (target != currentModulation) {
            targetModulation = target;
            modulationStrength = std::clamp(strength, 0.0f, 1.0f);
            transitionProgress = 0.0f;
        }
    }

    void updateTransition(float delta) {
        if (transitionProgress < 1.0f) {
            transitionProgress += delta;
            if (transitionProgress >= 1.0f) {
                transitionProgress = 1.0f;
                currentModulation = targetModulation;
            }
        }
    }

    bool isTransitioning() const { return transitionProgress < 1.0f; }
    MetricModulationType getCurrentModulation() const { return currentModulation; }
    float getStrength() const { return modulationStrength; }

    // ========================================
    // Get timing multiplier
    // ========================================
    float getTimingMultiplier() const {
        float mult = 1.0f;

        switch (currentModulation) {
            case MetricModulationType::HALF_TIME:
                mult = 2.0f;
                break;
            case MetricModulationType::DOUBLE_TIME:
                mult = 0.5f;
                break;
            case MetricModulationType::TRIPLET_FEEL:
                mult = 2.0f / 3.0f;  // 將 8 分音符變成三連音
                break;
            case MetricModulationType::DOTTED_FEEL:
                mult = 1.5f;  // 附點
                break;
            case MetricModulationType::INDIAN_LAYA_VILAMBIT:
                mult = 2.0f;  // 慢速
                break;
            case MetricModulationType::INDIAN_LAYA_DRUT:
                mult = 0.5f;  // 快速
                break;
            default:
                mult = 1.0f;
        }

        // 應用強度
        return 1.0f + (mult - 1.0f) * modulationStrength;
    }

    // ========================================
    // Get density multiplier
    // ========================================
    float getDensityMultiplier() const {
        switch (currentModulation) {
            case MetricModulationType::HALF_TIME:
                return 0.5f * modulationStrength + (1.0f - modulationStrength);
            case MetricModulationType::DOUBLE_TIME:
                return 2.0f * modulationStrength + (1.0f - modulationStrength);
            case MetricModulationType::TRIPLET_FEEL:
                return 1.5f * modulationStrength + (1.0f - modulationStrength);
            default:
                return 1.0f;
        }
    }

    // ========================================
    // Get swing ratio adjustment
    // ========================================
    float getSwingAdjustment(float baseSwing) const {
        switch (currentModulation) {
            case MetricModulationType::SWING_TO_STRAIGHT:
                // 逐漸趨向 50%（straight）
                return baseSwing * (1.0f - modulationStrength) + 0.5f * modulationStrength;
            case MetricModulationType::STRAIGHT_TO_SWING:
                // 增加 swing
                return baseSwing + (0.67f - baseSwing) * modulationStrength;
            case MetricModulationType::TRIPLET_FEEL:
                // 三連音感：2/3 swing
                return baseSwing * (1.0f - modulationStrength) + 0.667f * modulationStrength;
            default:
                return baseSwing;
        }
    }

    // ========================================
    // Apply modulation to pattern step
    // v0.18.2: 修正 HALF_TIME 精度問題，使用四捨五入
    // ========================================
    int mapStep(int originalStep, int patternLength) const {
        switch (currentModulation) {
            case MetricModulationType::HALF_TIME:
                // 每個音符延伸到兩個位置
                // 使用四捨五入避免奇數步精度損失
                return static_cast<int>(std::round(originalStep * 0.5f));
            case MetricModulationType::DOUBLE_TIME:
                // 壓縮到一半
                return (originalStep * 2) % patternLength;
            case MetricModulationType::TRIPLET_FEEL:
                // 三連音映射：將 8 分音符位置映射到三連音位置
                // 0→0, 1→1, 2→1, 3→2, 4→3, 5→3, 6→4, ...
                return static_cast<int>(std::round(originalStep * 2.0f / 3.0f));
            case MetricModulationType::DOTTED_FEEL:
                // 附點映射：3:2 比例
                return static_cast<int>(std::round(originalStep * 2.0f / 3.0f));
            default:
                return originalStep;
        }
    }

    // ========================================
    // Reverse map step (from modulated to original)
    // 用於從調變後的位置還原到原始位置
    // ========================================
    int reverseMapStep(int modulatedStep, int patternLength) const {
        switch (currentModulation) {
            case MetricModulationType::HALF_TIME:
                return std::min(modulatedStep * 2, patternLength - 1);
            case MetricModulationType::DOUBLE_TIME:
                return modulatedStep / 2;
            case MetricModulationType::TRIPLET_FEEL:
            case MetricModulationType::DOTTED_FEEL:
                return static_cast<int>(std::round(modulatedStep * 3.0f / 2.0f));
            default:
                return modulatedStep;
        }
    }

    // ========================================
    // Get modulation name
    // ========================================
    const char* getModulationName() const {
        switch (currentModulation) {
            case MetricModulationType::NONE: return "None";
            case MetricModulationType::HALF_TIME: return "Half-Time";
            case MetricModulationType::DOUBLE_TIME: return "Double-Time";
            case MetricModulationType::TRIPLET_FEEL: return "Triplet Feel";
            case MetricModulationType::DOTTED_FEEL: return "Dotted Feel";
            case MetricModulationType::SWING_TO_STRAIGHT: return "Swing→Straight";
            case MetricModulationType::STRAIGHT_TO_SWING: return "Straight→Swing";
            case MetricModulationType::INDIAN_LAYA_VILAMBIT: return "Vilambit (Slow)";
            case MetricModulationType::INDIAN_LAYA_MADHYA: return "Madhya (Medium)";
            case MetricModulationType::INDIAN_LAYA_DRUT: return "Drut (Fast)";
            default: return "Unknown";
        }
    }

    // ========================================
    // Suggest modulation for style
    // ========================================
    static MetricModulationType suggestForStyle(int styleIndex, float intensity) {
        // 根據風格和強度建議適合的調變
        if (intensity < 0.3f) {
            // 低強度：傾向放慢
            if (styleIndex == 4) return MetricModulationType::INDIAN_LAYA_VILAMBIT;
            if (styleIndex == 6) return MetricModulationType::HALF_TIME;  // Jazz
            return MetricModulationType::NONE;
        } else if (intensity > 0.7f) {
            // 高強度：傾向加快
            if (styleIndex == 4) return MetricModulationType::INDIAN_LAYA_DRUT;
            if (styleIndex == 7 || styleIndex == 9) return MetricModulationType::DOUBLE_TIME;  // Electronic/Techno
            return MetricModulationType::NONE;
        }
        return MetricModulationType::NONE;
    }
};

} // namespace WorldRhythm
