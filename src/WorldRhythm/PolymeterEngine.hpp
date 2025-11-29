#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace WorldRhythm {

// ========================================
// Polymeter Engine
// ========================================
// 實作多拍號系統，允許每個角色使用不同長度的循環
// 支援 3 vs 4, 5 vs 4, 7 vs 8 等傳統世界音樂複節奏
//
// 與 Cross-rhythm 的區別：
// - Cross-rhythm: 相同循環長度內的不同分割（如 3:2 hemiola）
// - Polymeter: 不同長度的循環同時進行（如 3 vs 4 拍子）

// ========================================
// Polymeter Configuration
// ========================================
struct PolymeterConfig {
    int timelineLength = 16;    // Timeline 循環長度（steps）
    int foundationLength = 16;  // Foundation 循環長度
    int grooveLength = 16;      // Groove 循環長度
    int leadLength = 16;        // Lead 循環長度

    // 計算最小公倍數（LCM）- 整體循環何時回到同步點
    // v0.18.5: 使用 long long 防止溢出，並限制最大值
    int getLCM() const {
        long long lcm = static_cast<long long>(timelineLength);
        lcm = std::lcm(lcm, static_cast<long long>(foundationLength));
        lcm = std::lcm(lcm, static_cast<long long>(grooveLength));
        lcm = std::lcm(lcm, static_cast<long long>(leadLength));

        // 限制最大值，防止極端配置導致過大循環
        static constexpr long long MAX_LCM = 1024;
        if (lcm > MAX_LCM) {
            lcm = MAX_LCM;
        }

        return static_cast<int>(lcm);
    }

    // 獲取角色長度
    int getLengthForRole(int roleIndex) const {
        switch (roleIndex) {
            case 0: return timelineLength;
            case 1: return foundationLength;
            case 2: return grooveLength;
            case 3: return leadLength;
            default: return 16;
        }
    }

    // 設定角色長度
    void setLengthForRole(int roleIndex, int length) {
        length = std::clamp(length, 3, 64);  // 限制範圍
        switch (roleIndex) {
            case 0: timelineLength = length; break;
            case 1: foundationLength = length; break;
            case 2: grooveLength = length; break;
            case 3: leadLength = length; break;
        }
    }
};

// ========================================
// Common Polymeter Types
// ========================================
enum class PolymeterType {
    UNISON,         // 全部相同長度（16）
    THREE_VS_FOUR,  // 3 vs 4（西非、古巴）
    FIVE_VS_FOUR,   // 5 vs 4（印度、巴爾幹）
    SEVEN_VS_EIGHT, // 7 vs 8（巴爾幹 Aksak）
    AFRICAN_BELL,   // 12 vs 16（西非 bell pattern）
    GAMELAN,        // 8 vs 16（甘美朗 colotomic）
    CUSTOM          // 自訂
};

// ========================================
// v0.18.2: Reset Behavior Options
// ========================================
// 控制接收到 Reset 信號時的行為
enum class PolymeterResetBehavior {
    FULL_RESET,     // 所有角色同時重置到 step 0（標準行為）
    PHASE_PRESERVE, // 保持相位關係，只重置主計數器
    GRADUAL_SYNC,   // 逐步同步到下一個自然同步點
    MASTER_ONLY,    // 只重置主角色（Timeline），其他保持
    CUSTOM_ANCHOR   // 自訂錨點重置
};

// Reset 配置結構
struct PolymeterResetConfig {
    PolymeterResetBehavior behavior = PolymeterResetBehavior::FULL_RESET;

    // GRADUAL_SYNC 相關設定
    int gradualSyncSteps = 8;    // 幾步內完成同步
    float gradualSyncCurve = 0.5f;  // 同步曲線（0=線性, 1=指數）

    // CUSTOM_ANCHOR 相關設定
    int anchorRole = 0;          // 錨點角色（其他角色相對於此）
    bool preserveTension = false; // 是否保持張力狀態

    // 通用設定
    bool allowPartialReset = true;  // 允許部分角色重置
    std::array<bool, 4> roleResetEnabled = {true, true, true, true};  // 各角色是否參與重置
};

// ========================================
// Polymeter Phase Tracker
// ========================================
// 追蹤每個角色的相位，用於即時播放
struct PolymeterPhase {
    int globalStep = 0;         // 全局步進計數器
    int masterLength = 16;      // 主循環長度（通常是 LCM）

    // 獲取角色在當前全局步進的本地位置
    int getLocalStep(int globalStep, int roleLength) const {
        return globalStep % roleLength;
    }

    // 檢查角色是否在循環起點（用於同步事件）
    bool isAtStart(int globalStep, int roleLength) const {
        return (globalStep % roleLength) == 0;
    }

    // 計算到下一個全局同步點的距離
    int stepsToSync(int globalStep, int lcm) const {
        return lcm - (globalStep % lcm);
    }

    // 計算兩個角色的相位差（以 steps 為單位）
    float getPhaseDifference(int globalStep, int length1, int length2) const {
        float phase1 = static_cast<float>(globalStep % length1) / length1;
        float phase2 = static_cast<float>(globalStep % length2) / length2;
        float diff = phase1 - phase2;
        // 正規化到 [-0.5, 0.5]
        if (diff > 0.5f) diff -= 1.0f;
        if (diff < -0.5f) diff += 1.0f;
        return diff;
    }
};

// ========================================
// Polymeter Engine Class
// ========================================
class PolymeterEngine {
private:
    PolymeterConfig config;
    PolymeterPhase phase;
    PolymeterResetConfig resetConfig;

    // v0.18.2: Gradual sync state
    bool gradualSyncActive = false;
    int gradualSyncRemaining = 0;
    std::array<int, 4> gradualSyncTargets = {0, 0, 0, 0};
    std::array<int, 4> roleOffsets = {0, 0, 0, 0};  // 用於 PHASE_PRESERVE

public:
    PolymeterEngine() = default;

    // ========================================
    // Configuration
    // ========================================
    void setConfig(const PolymeterConfig& cfg) {
        config = cfg;
        phase.masterLength = config.getLCM();
    }

    const PolymeterConfig& getConfig() const {
        return config;
    }

    // 使用預設的 Polymeter 類型
    void setPolymeterType(PolymeterType type) {
        switch (type) {
            case PolymeterType::UNISON:
                config = {16, 16, 16, 16};
                break;
            case PolymeterType::THREE_VS_FOUR:
                // 3 vs 4：Timeline=12, Foundation=16
                // 西非/古巴的核心 polymeter
                config = {12, 16, 16, 12};
                break;
            case PolymeterType::FIVE_VS_FOUR:
                // 5 vs 4：Foundation=20, 其他=16
                // 印度/巴爾幹常見
                config = {16, 20, 16, 20};
                break;
            case PolymeterType::SEVEN_VS_EIGHT:
                // 7 vs 8：巴爾幹 Aksak
                config = {14, 16, 14, 16};
                break;
            case PolymeterType::AFRICAN_BELL:
                // 12 vs 16：西非 bell pattern
                config = {12, 16, 12, 16};
                break;
            case PolymeterType::GAMELAN:
                // 8 vs 16：甘美朗 colotomic cycle
                config = {16, 16, 8, 16};
                break;
            case PolymeterType::CUSTOM:
                // 保持現有配置
                break;
        }
        phase.masterLength = config.getLCM();
    }

    // ========================================
    // Phase Management
    // ========================================

    // v0.18.2: 設定 Reset 行為
    void setResetConfig(const PolymeterResetConfig& cfg) {
        resetConfig = cfg;
    }

    const PolymeterResetConfig& getResetConfig() const {
        return resetConfig;
    }

    void setResetBehavior(PolymeterResetBehavior behavior) {
        resetConfig.behavior = behavior;
    }

    // v0.18.2: 根據配置進行 reset
    void reset() {
        switch (resetConfig.behavior) {
            case PolymeterResetBehavior::FULL_RESET:
                resetFull();
                break;
            case PolymeterResetBehavior::PHASE_PRESERVE:
                resetPhasePreserve();
                break;
            case PolymeterResetBehavior::GRADUAL_SYNC:
                resetGradualSync();
                break;
            case PolymeterResetBehavior::MASTER_ONLY:
                resetMasterOnly();
                break;
            case PolymeterResetBehavior::CUSTOM_ANCHOR:
                resetCustomAnchor();
                break;
            default:
                resetFull();
        }
    }

    // 完整重置（原始行為）
    void resetFull() {
        phase.globalStep = 0;
        roleOffsets.fill(0);
        gradualSyncActive = false;
    }

    // 保持相位關係重置
    void resetPhasePreserve() {
        // 記錄當前各角色的相位
        for (int i = 0; i < 4; i++) {
            if (resetConfig.roleResetEnabled[i]) {
                int roleLength = config.getLengthForRole(i);
                roleOffsets[i] = phase.globalStep % roleLength;
            }
        }
        // 重置主計數器但保持 offset
        phase.globalStep = 0;
    }

    // 漸進同步重置
    void resetGradualSync() {
        gradualSyncActive = true;
        gradualSyncRemaining = resetConfig.gradualSyncSteps;

        // 計算每個角色需要到達的目標位置
        for (int i = 0; i < 4; i++) {
            int roleLength = config.getLengthForRole(i);
            int currentPos = phase.globalStep % roleLength;
            // 目標：最近的循環起點
            gradualSyncTargets[i] = (currentPos > roleLength / 2) ? roleLength : 0;
        }
    }

    // 只重置主角色
    void resetMasterOnly() {
        // 計算 Timeline 需要調整的量
        int timelineLength = config.timelineLength;
        int timelinePos = phase.globalStep % timelineLength;

        // 調整 globalStep 使 Timeline 回到 0
        phase.globalStep -= timelinePos;
        if (phase.globalStep < 0) {
            phase.globalStep += phase.masterLength;
        }
    }

    // 自訂錨點重置
    void resetCustomAnchor() {
        int anchorRole = resetConfig.anchorRole;
        int anchorLength = config.getLengthForRole(anchorRole);
        int anchorPos = phase.globalStep % anchorLength;

        if (resetConfig.preserveTension) {
            // 保持張力：只調整錨點角色
            phase.globalStep -= anchorPos;
            if (phase.globalStep < 0) {
                phase.globalStep += phase.masterLength;
            }
        } else {
            // 完全重置但從錨點角色的視角
            for (int i = 0; i < 4; i++) {
                if (resetConfig.roleResetEnabled[i]) {
                    roleOffsets[i] = 0;
                } else {
                    // 保持此角色的相對位置
                    int roleLength = config.getLengthForRole(i);
                    roleOffsets[i] = phase.globalStep % roleLength;
                }
            }
            phase.globalStep = 0;
        }
    }

    void advance() {
        phase.globalStep++;

        // 處理漸進同步
        if (gradualSyncActive && gradualSyncRemaining > 0) {
            gradualSyncRemaining--;
            if (gradualSyncRemaining == 0) {
                gradualSyncActive = false;
                // 完成同步：所有角色回到 0
                phase.globalStep = 0;
                roleOffsets.fill(0);
            }
        }

        // 在 LCM 處環繞（防止溢出）
        if (phase.globalStep >= phase.masterLength * 100) {
            phase.globalStep = phase.globalStep % phase.masterLength;
        }
    }

    int getGlobalStep() const {
        return phase.globalStep;
    }

    int getLCM() const {
        return config.getLCM();
    }

    // ========================================
    // Role Position Queries
    // ========================================
    int getLocalStep(int roleIndex) const {
        int roleLength = config.getLengthForRole(roleIndex);
        // v0.18.2: 考慮 PHASE_PRESERVE 時的 offset
        int effectiveStep = phase.globalStep + roleOffsets[roleIndex];
        return effectiveStep % roleLength;
    }

    // v0.18.2: 檢查是否在漸進同步中
    bool isGradualSyncActive() const {
        return gradualSyncActive;
    }

    int getGradualSyncRemaining() const {
        return gradualSyncRemaining;
    }

    // v0.18.2: 獲取角色的 offset（用於 debug/visualization）
    int getRoleOffset(int roleIndex) const {
        if (roleIndex >= 0 && roleIndex < 4) {
            return roleOffsets[roleIndex];
        }
        return 0;
    }

    bool isRoleAtStart(int roleIndex) const {
        int roleLength = config.getLengthForRole(roleIndex);
        return phase.isAtStart(phase.globalStep, roleLength);
    }

    // 檢查是否所有角色都在循環起點（全局同步點）
    bool isAtGlobalSync() const {
        return (phase.globalStep % config.getLCM()) == 0;
    }

    // ========================================
    // Pattern Stretching/Mapping
    // v0.18.8: 加入輸入驗證
    // ========================================
    // 將標準 16-step pattern 映射到任意長度
    std::vector<float> mapPatternToLength(const std::vector<float>& pattern16,
                                          int targetLength) const {
        // v0.18.8: 輸入驗證
        if (pattern16.empty() || targetLength <= 0) {
            return std::vector<float>(std::max(1, targetLength), 0.0f);
        }
        if (targetLength == 16 || pattern16.size() != 16) {
            return pattern16;  // 無需轉換
        }

        std::vector<float> result(targetLength, 0.0f);

        // 使用線性插值映射
        for (int i = 0; i < targetLength; i++) {
            // 計算原始 pattern 中的對應位置
            float srcPos = static_cast<float>(i) * 16.0f / targetLength;
            int srcIdx = static_cast<int>(srcPos);
            float frac = srcPos - srcIdx;

            if (srcIdx >= 15) {
                result[i] = pattern16[15];
            } else {
                // 線性插值
                result[i] = pattern16[srcIdx] * (1.0f - frac) +
                           pattern16[srcIdx + 1] * frac;
            }
        }

        return result;
    }

    // 將標準 16-step weights 映射到任意長度
    std::vector<float> mapWeightsToLength(const float* weights16,
                                          int targetLength) const {
        if (targetLength == 16) {
            return std::vector<float>(weights16, weights16 + 16);
        }

        std::vector<float> result(targetLength, 0.0f);

        for (int i = 0; i < targetLength; i++) {
            float srcPos = static_cast<float>(i) * 16.0f / targetLength;
            int srcIdx = static_cast<int>(srcPos);
            float frac = srcPos - srcIdx;

            if (srcIdx >= 15) {
                result[i] = weights16[15];
            } else {
                result[i] = weights16[srcIdx] * (1.0f - frac) +
                           weights16[srcIdx + 1] * frac;
            }
        }

        return result;
    }

    // ========================================
    // Phase Relationship Analysis
    // ========================================
    // 計算兩個角色之間的相位關係
    float getPhaseDifference(int role1, int role2) const {
        int len1 = config.getLengthForRole(role1);
        int len2 = config.getLengthForRole(role2);
        return phase.getPhaseDifference(phase.globalStep, len1, len2);
    }

    // 獲取「張力」值：相位差越大張力越高
    // 用於動態調整其他參數（如 intensity, fill probability）
    float getPolymeterTension() const {
        float totalTension = 0.0f;
        int comparisons = 0;

        // 計算所有角色對之間的相位差
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                float diff = std::abs(getPhaseDifference(i, j));
                totalTension += diff;
                comparisons++;
            }
        }

        // 正規化（最大值為 0.5 × 6 = 3.0）
        return comparisons > 0 ? totalTension / (comparisons * 0.5f) : 0.0f;
    }

    // ========================================
    // Sync Point Detection
    // ========================================
    // 檢查特定角色對是否即將同步
    bool isPairApproachingSync(int role1, int role2, int lookAhead = 4) const {
        int len1 = config.getLengthForRole(role1);
        int len2 = config.getLengthForRole(role2);
        int pairLCM = std::lcm(len1, len2);

        int stepsToSync = pairLCM - (phase.globalStep % pairLCM);
        return stepsToSync <= lookAhead;
    }

    // 獲取到下一個全局同步點的步數
    int getStepsToGlobalSync() const {
        int lcm = config.getLCM();
        return lcm - (phase.globalStep % lcm);
    }

    // ========================================
    // Style-Specific Polymeter Suggestions
    // ========================================
    static PolymeterType suggestForStyle(int styleIndex) {
        // 根據風格建議適合的 Polymeter 類型
        switch (styleIndex) {
            case 0:  // West African
                return PolymeterType::AFRICAN_BELL;  // 12 vs 16
            case 1:  // Afro-Cuban
                return PolymeterType::THREE_VS_FOUR;  // 3 vs 4 是古巴音樂的核心
            case 2:  // Brazilian
                return PolymeterType::UNISON;  // Samba 通常同步
            case 3:  // Balkan
                return PolymeterType::SEVEN_VS_EIGHT;  // Aksak 不規則拍
            case 4:  // Indian
                return PolymeterType::FIVE_VS_FOUR;  // Tala 系統的複雜分割
            case 5:  // Gamelan
                return PolymeterType::GAMELAN;  // Colotomic cycle
            case 6:  // Jazz
                return PolymeterType::UNISON;  // 標準 4/4
            case 7:  // Electronic
                return PolymeterType::UNISON;  // 四拍為主
            case 8:  // Breakbeat
                return PolymeterType::UNISON;  // 基於 4 bar loop
            case 9:  // Techno
                return PolymeterType::UNISON;  // 嚴格 4/4
            default:
                return PolymeterType::UNISON;
        }
    }

    // ========================================
    // Visualization Data
    // ========================================
    struct VisualizationData {
        std::array<float, 4> rolePhases;     // 每個角色的相位 [0, 1)
        std::array<bool, 4> roleAtStart;     // 每個角色是否在循環起點
        float tension;                        // 整體張力
        int stepsToSync;                     // 到全局同步的步數
        bool isAtGlobalSync;                 // 是否在全局同步點
    };

    VisualizationData getVisualizationData() const {
        VisualizationData data;

        for (int i = 0; i < 4; i++) {
            int len = config.getLengthForRole(i);
            data.rolePhases[i] = static_cast<float>(phase.globalStep % len) / len;
            data.roleAtStart[i] = isRoleAtStart(i);
        }

        data.tension = getPolymeterTension();
        data.stepsToSync = getStepsToGlobalSync();
        data.isAtGlobalSync = isAtGlobalSync();

        return data;
    }
};

} // namespace WorldRhythm
