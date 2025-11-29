#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <numeric>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Asymmetric Grouping Engine (Aksak)
// ========================================
// Implements asymmetric/additive meter patterns found in:
// - Balkan music (Aksak rhythms: 7/8, 9/8, 11/8, etc.)
// - Turkish music (Usul patterns with uneven beats)
// - Greek music (Kalamatianos 7/8, Tsamikos 3/4)
// - Bulgarian folk (Rachenitsa 7/8, Daichovo 9/8, Kopanitsa 11/8)
//
// Key concept: Beats are grouped into 2s and 3s
// - 7/8 = 2+2+3 or 3+2+2 or 2+3+2
// - 9/8 = 2+2+2+3 or 2+3+2+2 or 3+2+2+2
// - 11/8 = 2+2+3+2+2 or 3+2+2+2+2, etc.

// ========================================
// Grouping pattern types
// ========================================
enum class GroupingType {
    // 7/8 variations
    SEVEN_223,      // 2+2+3 (Rachenitsa)
    SEVEN_232,      // 2+3+2 (Lesnoto)
    SEVEN_322,      // 3+2+2

    // 9/8 variations
    NINE_2223,      // 2+2+2+3 (Daichovo)
    NINE_2232,      // 2+2+3+2
    NINE_2322,      // 2+3+2+2 (Karsilama)
    NINE_3222,      // 3+2+2+2

    // 11/8 variations
    ELEVEN_22322,   // 2+2+3+2+2 (Kopanitsa)
    ELEVEN_23222,   // 2+3+2+2+2
    ELEVEN_32222,   // 3+2+2+2+2

    // 5/8 variations
    FIVE_23,        // 2+3
    FIVE_32,        // 3+2

    // 15/8 variations (Bulgarian)
    FIFTEEN_22223322, // Complex Bulgarian pattern

    // Turkish Usul patterns
    USUL_AKSAK,     // 9/8 Turkish aksak
    USUL_CURCUNA,   // 10/8 Çürçüna

    CUSTOM          // User-defined grouping
};

// ========================================
// Grouping configuration
// ========================================
struct GroupingConfig {
    GroupingType type;
    std::vector<int> groupSizes;    // e.g., {2, 2, 3} for 7/8
    int totalSteps;                  // Sum of groupSizes (7 for 7/8)
    int stepsPerSmallBeat;          // Typically 2 (eighth note = 2 steps at 16th resolution)

    // Accent pattern within the grouping
    std::vector<float> groupAccents; // Accent strength per group (0.0-1.0)

    // Optional: secondary accents within groups
    bool useSecondaryAccents;
    float secondaryAccentStrength;

    // Calculate total pattern length in steps
    int getPatternLength() const {
        return totalSteps * stepsPerSmallBeat;
    }

    // Get beat positions (start of each group)
    std::vector<int> getBeatPositions() const {
        std::vector<int> positions;
        int pos = 0;
        for (int size : groupSizes) {
            positions.push_back(pos * stepsPerSmallBeat);
            pos += size;
        }
        return positions;
    }

    // Get group at a given step position
    // v0.18.8: 加入空陣列和除零檢查
    int getGroupAtStep(int step) const {
        // v0.18.8: 防禦性檢查
        if (groupSizes.empty() || stepsPerSmallBeat <= 0) {
            return 0;
        }
        if (step < 0) {
            return 0;
        }

        int scaledStep = step / stepsPerSmallBeat;
        int cumulative = 0;
        for (size_t i = 0; i < groupSizes.size(); i++) {
            cumulative += groupSizes[i];
            if (scaledStep < cumulative) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(groupSizes.size()) - 1;
    }

    // Check if step is on a group boundary (downbeat)
    bool isGroupDownbeat(int step) const {
        if (step % stepsPerSmallBeat != 0) return false;
        int scaledStep = step / stepsPerSmallBeat;
        int cumulative = 0;
        for (int size : groupSizes) {
            if (scaledStep == cumulative) return true;
            cumulative += size;
        }
        return false;
    }
};

// ========================================
// Predefined grouping configurations
// ========================================
const std::array<GroupingConfig, 15> GROUPING_PRESETS = {{
    // 7/8 variations
    {GroupingType::SEVEN_223, {2, 2, 3}, 7, 2, {1.0f, 0.7f, 0.85f}, false, 0.0f},
    {GroupingType::SEVEN_232, {2, 3, 2}, 7, 2, {1.0f, 0.85f, 0.7f}, false, 0.0f},
    {GroupingType::SEVEN_322, {3, 2, 2}, 7, 2, {1.0f, 0.7f, 0.7f}, false, 0.0f},

    // 9/8 variations
    {GroupingType::NINE_2223, {2, 2, 2, 3}, 9, 2, {1.0f, 0.6f, 0.7f, 0.85f}, false, 0.0f},
    {GroupingType::NINE_2232, {2, 2, 3, 2}, 9, 2, {1.0f, 0.6f, 0.85f, 0.7f}, false, 0.0f},
    {GroupingType::NINE_2322, {2, 3, 2, 2}, 9, 2, {1.0f, 0.85f, 0.7f, 0.6f}, false, 0.0f},
    {GroupingType::NINE_3222, {3, 2, 2, 2}, 9, 2, {1.0f, 0.7f, 0.6f, 0.6f}, false, 0.0f},

    // 11/8 variations
    {GroupingType::ELEVEN_22322, {2, 2, 3, 2, 2}, 11, 2, {1.0f, 0.6f, 0.85f, 0.7f, 0.6f}, false, 0.0f},
    {GroupingType::ELEVEN_23222, {2, 3, 2, 2, 2}, 11, 2, {1.0f, 0.85f, 0.7f, 0.6f, 0.6f}, false, 0.0f},
    {GroupingType::ELEVEN_32222, {3, 2, 2, 2, 2}, 11, 2, {1.0f, 0.7f, 0.6f, 0.6f, 0.6f}, false, 0.0f},

    // 5/8 variations
    {GroupingType::FIVE_23, {2, 3}, 5, 2, {1.0f, 0.8f}, false, 0.0f},
    {GroupingType::FIVE_32, {3, 2}, 5, 2, {1.0f, 0.8f}, false, 0.0f},

    // 15/8 (Bulgarian)
    {GroupingType::FIFTEEN_22223322, {2, 2, 2, 2, 3, 3, 2, 2}, 18, 2,
     {1.0f, 0.5f, 0.7f, 0.5f, 0.85f, 0.75f, 0.6f, 0.5f}, true, 0.4f},

    // Turkish Usul
    {GroupingType::USUL_AKSAK, {2, 2, 2, 3}, 9, 2, {1.0f, 0.5f, 0.7f, 0.9f}, true, 0.3f},
    {GroupingType::USUL_CURCUNA, {3, 2, 2, 3}, 10, 2, {1.0f, 0.6f, 0.7f, 0.85f}, true, 0.35f}
}};

// ========================================
// Asymmetric Grouping Engine
// ========================================
class AsymmetricGroupingEngine {
private:
    GroupingConfig currentConfig;
    int currentPhase = 0;  // Current position within the grouping cycle

public:
    AsymmetricGroupingEngine() {
        // Default to 7/8 Rachenitsa
        currentConfig = GROUPING_PRESETS[0];
    }

    // ========================================
    // Set grouping type from preset
    // ========================================
    void setGroupingType(GroupingType type) {
        for (const auto& preset : GROUPING_PRESETS) {
            if (preset.type == type) {
                currentConfig = preset;
                return;
            }
        }
        // If not found, keep current
    }

    // ========================================
    // Set custom grouping
    // ========================================
    void setCustomGrouping(const std::vector<int>& groupSizes,
                           const std::vector<float>& accents = {}) {
        currentConfig.type = GroupingType::CUSTOM;
        currentConfig.groupSizes = groupSizes;
        currentConfig.totalSteps = std::accumulate(groupSizes.begin(), groupSizes.end(), 0);

        if (accents.empty()) {
            // Default accents: first group strongest
            currentConfig.groupAccents.resize(groupSizes.size());
            currentConfig.groupAccents[0] = 1.0f;
            for (size_t i = 1; i < groupSizes.size(); i++) {
                currentConfig.groupAccents[i] = 0.6f + (groupSizes[i] == 3 ? 0.2f : 0.0f);
            }
        } else {
            currentConfig.groupAccents = accents;
        }
    }

    // ========================================
    // Get current configuration
    // ========================================
    const GroupingConfig& getConfig() const {
        return currentConfig;
    }

    // ========================================
    // Generate accent pattern for pattern
    // ========================================
    std::vector<float> generateAccentPattern(int patternLength) const {
        std::vector<float> accents(patternLength, 0.0f);
        int configPatternLen = currentConfig.getPatternLength();

        for (int step = 0; step < patternLength; step++) {
            int cyclicStep = step % configPatternLen;

            if (currentConfig.isGroupDownbeat(cyclicStep)) {
                int group = currentConfig.getGroupAtStep(cyclicStep);
                // v0.18.5: 加入負索引檢查
                if (group >= 0 && group < static_cast<int>(currentConfig.groupAccents.size())) {
                    accents[step] = currentConfig.groupAccents[group];
                }
            } else if (currentConfig.useSecondaryAccents) {
                // Secondary accents on off-beats within groups
                int scaledStep = cyclicStep / currentConfig.stepsPerSmallBeat;
                if (scaledStep % 2 == 1) {
                    accents[step] = currentConfig.secondaryAccentStrength;
                }
            }
        }

        return accents;
    }

    // ========================================
    // Apply asymmetric feel to existing pattern
    // ========================================
    void applyToPattern(Pattern& p, float intensity = 1.0f) const {
        std::vector<float> accents = generateAccentPattern(p.length);
        std::vector<int> beatPositions = currentConfig.getBeatPositions();

        for (int step = 0; step < p.length; step++) {
            float vel = p.getVelocity(step);
            if (vel > 0.0f) {
                float accent = accents[step % accents.size()];
                if (accent > 0.0f) {
                    // Boost velocity on accented positions
                    float boost = 1.0f + (accent - 0.5f) * intensity * 0.4f;
                    vel = std::clamp(vel * boost, 0.0f, 1.0f);
                    p.setOnset(step, vel);
                    p.accents[step] = (accent >= 0.8f);
                } else {
                    // Slightly reduce velocity on non-accented positions
                    vel = std::clamp(vel * (1.0f - intensity * 0.15f), 0.0f, 1.0f);
                    p.setOnset(step, vel);
                }
            }
        }
    }

    // ========================================
    // Generate a pattern following the grouping
    // ========================================
    Pattern generateGroupingPattern(int patternLength, float density, float baseVelocity) const {
        Pattern p(patternLength);
        std::vector<float> accents = generateAccentPattern(patternLength);
        int configPatternLen = currentConfig.getPatternLength();

        for (int step = 0; step < patternLength; step++) {
            int cyclicStep = step % configPatternLen;
            float accent = accents[step];

            // Always hit on group downbeats
            if (currentConfig.isGroupDownbeat(cyclicStep)) {
                float vel = baseVelocity * (0.85f + accent * 0.15f);
                p.setOnset(step, std::clamp(vel, 0.0f, 1.0f));
                p.accents[step] = (accent >= 0.8f);
            }
            // Optionally add notes based on density
            else if (density > 0.5f) {
                // Add secondary hits within groups for higher density
                int scaledStep = cyclicStep / currentConfig.stepsPerSmallBeat;
                int group = currentConfig.getGroupAtStep(cyclicStep);
                // v0.18.8: 加入陣列邊界檢查
                if (group < 0 || group >= static_cast<int>(currentConfig.groupSizes.size())) {
                    continue;
                }
                int groupSize = currentConfig.groupSizes[group];

                // More likely to add notes in longer groups (3s)
                float addProb = (density - 0.5f) * 2.0f;
                if (groupSize == 3) addProb *= 1.3f;

                if (scaledStep % groupSize != 0) {
                    // This is inside a group
                    bool shouldAdd = (step % 2 == 1 && addProb > 0.3f) ||
                                    (step % 4 == 2 && addProb > 0.6f);
                    if (shouldAdd) {
                        float vel = baseVelocity * 0.6f * density;
                        p.setOnset(step, std::clamp(vel, 0.0f, 1.0f));
                    }
                }
            }
        }

        return p;
    }

    // ========================================
    // Map standard 4/4 pattern to asymmetric meter
    // v0.18.8: 加入除零和邊界檢查
    // ========================================
    Pattern mapFromStandardMeter(const Pattern& source, int targetLength) const {
        Pattern target(targetLength);
        int srcLen = source.length;
        int configPatternLen = currentConfig.getPatternLength();

        // v0.18.8: 防止除零和無效輸入
        if (srcLen <= 0 || configPatternLen <= 0 || targetLength <= 0) {
            return target;
        }

        // Map source positions to target positions proportionally
        // but snap to group boundaries
        for (int srcStep = 0; srcStep < srcLen; srcStep++) {
            float vel = source.getVelocity(srcStep);
            if (vel > 0.0f) {
                // Proportional mapping
                float ratio = static_cast<float>(srcStep) / static_cast<float>(srcLen);
                int rawTargetStep = static_cast<int>(ratio * configPatternLen);

                // Snap to nearest group boundary for strong beats
                int targetStep = rawTargetStep;
                if (srcStep % 4 == 0) {  // Strong beat in 4/4
                    // Find nearest group downbeat
                    int minDist = configPatternLen;
                    auto beatPositions = currentConfig.getBeatPositions();
                    for (int pos : beatPositions) {
                        int dist = std::abs(rawTargetStep - pos);
                        if (dist < minDist) {
                            minDist = dist;
                            targetStep = pos;
                        }
                    }
                }

                // Apply to all cycles in target
                for (int cycle = 0; cycle * configPatternLen < targetLength; cycle++) {
                    int pos = cycle * configPatternLen + targetStep;
                    if (pos < targetLength) {
                        float existing = target.getVelocity(pos);
                        target.setOnset(pos, std::max(existing, vel));
                        target.accents[pos] = source.accents[srcStep];
                    }
                }
            }
        }

        return target;
    }

    // ========================================
    // Get time signature string for display
    // ========================================
    std::string getTimeSignature() const {
        return std::to_string(currentConfig.totalSteps) + "/8";
    }

    // ========================================
    // Get grouping description for display
    // ========================================
    std::string getGroupingDescription() const {
        std::string desc;
        for (size_t i = 0; i < currentConfig.groupSizes.size(); i++) {
            if (i > 0) desc += "+";
            desc += std::to_string(currentConfig.groupSizes[i]);
        }
        return desc;
    }

    // ========================================
    // Phase tracking
    // ========================================
    void advancePhase(int steps) {
        currentPhase = (currentPhase + steps) % currentConfig.getPatternLength();
    }

    void resetPhase() {
        currentPhase = 0;
    }

    int getCurrentGroup() const {
        return currentConfig.getGroupAtStep(currentPhase);
    }

    bool isOnGroupDownbeat() const {
        return currentConfig.isGroupDownbeat(currentPhase);
    }

    // ========================================
    // Style-specific presets
    // ========================================
    static GroupingType getStyleDefaultGrouping(int styleIndex) {
        switch (styleIndex) {
            case 3:  // Balkan
                return GroupingType::SEVEN_223;  // Rachenitsa default
            case 4:  // Indian (some Carnatic talas use asymmetric)
                return GroupingType::SEVEN_232;
            case 5:  // Gamelan (adapted)
                return GroupingType::NINE_2322;
            default:
                return GroupingType::SEVEN_223;
        }
    }

    // ========================================
    // Generate traditional Balkan dance pattern
    // ========================================
    Pattern generateBalkanDancePattern(GroupingType type, int repetitions = 1) {
        setGroupingType(type);
        int singleLen = currentConfig.getPatternLength();
        int totalLen = singleLen * repetitions;

        Pattern p(totalLen);

        for (int rep = 0; rep < repetitions; rep++) {
            int offset = rep * singleLen;
            auto beats = currentConfig.getBeatPositions();

            // Traditional pattern: hit on all group downbeats
            for (size_t i = 0; i < beats.size(); i++) {
                int pos = offset + beats[i];
                float accent = currentConfig.groupAccents[i];
                p.setOnset(pos, accent);
                p.accents[pos] = (accent >= 0.8f);

                // Add upbeats for longer groups (3s)
                if (currentConfig.groupSizes[i] == 3) {
                    int upbeatPos = pos + currentConfig.stepsPerSmallBeat;
                    if (upbeatPos < totalLen) {
                        p.setOnset(upbeatPos, accent * 0.5f);
                    }
                }
            }
        }

        return p;
    }
};

} // namespace WorldRhythm
