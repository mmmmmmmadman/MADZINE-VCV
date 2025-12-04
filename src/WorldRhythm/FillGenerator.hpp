#pragma once

#include <vector>
#include <random>
#include <cmath>
#include "StyleProfiles.hpp"

namespace WorldRhythm {

enum class FillType {
    NONE = 0,
    ROLL,           // Rapid repetition, increasing density
    TIHAI,          // Phrase x 3, lands on downbeat
    BUILDUP,        // Density increase toward target
    BREAK,          // Synchronized silence
    SIGNAL,         // Lead voice phrase
    // Extended roll types (v0.13)
    ROLL_ACCELERANDO,   // 16th -> 32nd -> triplet
    ROLL_PITCHED,       // Pitch rises during roll
    ROLL_STUTTER,       // Trap-style stutter
    ROLL_TRIPLET,       // Triplet feel roll
    // Gamelan-specific (v0.17)
    ANGSEL,             // Gamelan coordinated break
    // Afro-Cuban-specific (v0.19)
    LLAMADA             // Afro-Cuban call phrase (all roles respond in unison)
};

// ========================================
// Roll Subdivision Type
// ========================================
enum class RollSubdivision {
    SIXTEENTH,      // Standard 16th notes
    THIRTY_SECOND,  // 32nd notes (double density)
    TRIPLET,        // 16th note triplets
    MIXED           // Accelerating: 16th -> 32nd -> triplet
};

struct FillEvent {
    FillType type;
    int startStep;      // Start position within bar
    int lengthBeats;    // 1, 2, 4, or 8 beats
    float intensity;    // 0.0 - 1.0
};

class FillGenerator {
private:
    std::mt19937 rng;

public:
    FillGenerator() : rng(std::random_device{}()) {}

    void seed(unsigned int s) { rng.seed(s); }

    // ========================================
    // Determine if fill should occur at bar position
    // ========================================
    bool shouldFill(int barNumber, float fillProbability) {
        // Base probability from research (Section 4.1)
        // barNumber is 1-indexed
        float baseProbability;

        int barInPhrase = barNumber % 4;  // 0, 1, 2, 3

        if (barInPhrase == 0) {  // Bar 4, 8, 12, 16...
            baseProbability = 0.85f;
        } else if (barInPhrase == 3) {  // Bar 3, 7, 11, 15...
            baseProbability = 0.15f;
        } else if (barInPhrase == 2) {  // Bar 2, 6, 10, 14...
            baseProbability = 0.10f;
        } else {  // Bar 1, 5, 9, 13...
            baseProbability = 0.05f;
        }

        // Modulate by user probability
        float finalProb = baseProbability * fillProbability;

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < finalProb;
    }

    // ========================================
    // Determine fill length based on intensity (Section 4.2)
    // ========================================
    int getFillLengthBeats(float intensity) {
        if (intensity < 0.25f) return 1;
        if (intensity < 0.50f) return 2;
        if (intensity < 0.75f) return 4;
        return 8;
    }

    // ========================================
    // Select fill type based on style
    // BREAK is rare (full stop) - most fills should be ROLL or BUILDUP
    // ========================================
    FillType selectFillType(int styleIndex, Role role) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float r = dist(rng);

        switch (styleIndex) {
            case 0: // West African - Roll dominant, occasional signal
                if (role == LEAD) return FillType::SIGNAL;
                if (r < 0.5f) return FillType::ROLL;
                if (r < 0.8f) return FillType::BUILDUP;
                return FillType::ROLL_ACCELERANDO;

            case 1: // Afro-Cuban - Buildup, Roll, and Llamada
                if (role == LEAD && r < 0.25f) return FillType::LLAMADA;
                if (r < 0.2f) return FillType::LLAMADA;  // All roles can play llamada
                if (r < 0.5f) return FillType::BUILDUP;
                if (r < 0.75f) return FillType::ROLL;
                return FillType::ROLL_TRIPLET;

            case 2: // Brazilian - Roll heavy
                if (r < 0.5f) return FillType::ROLL;
                if (r < 0.8f) return FillType::ROLL_ACCELERANDO;
                return FillType::BUILDUP;

            case 3: // Balkan - Signal and Roll
                if (r < 0.3f) return FillType::SIGNAL;
                if (r < 0.6f) return FillType::ROLL;
                if (r < 0.9f) return FillType::ROLL_TRIPLET;
                return FillType::BUILDUP;

            case 4: // Indian - Tihai dominant
                if (r < 0.5f) return FillType::TIHAI;
                if (r < 0.8f) return FillType::BUILDUP;
                return FillType::ROLL;

            case 5: // Gamelan - Angsel + Roll
                if (r < 0.35f) return FillType::ANGSEL;
                if (r < 0.6f) return FillType::ROLL;
                if (r < 0.85f) return FillType::SIGNAL;
                return FillType::BUILDUP;

            case 6: // Jazz - Triplet feel
                if (r < 0.4f) return FillType::ROLL_TRIPLET;
                if (r < 0.7f) return FillType::SIGNAL;
                return FillType::BUILDUP;

            case 7: // Electronic - Roll variants
                if (r < 0.3f) return FillType::ROLL_STUTTER;
                if (r < 0.6f) return FillType::ROLL_ACCELERANDO;
                if (r < 0.85f) return FillType::BUILDUP;
                return FillType::ROLL;

            case 8: // Breakbeat - Buildup + Stutter
                if (r < 0.35f) return FillType::BUILDUP;
                if (r < 0.6f) return FillType::ROLL_STUTTER;
                if (r < 0.85f) return FillType::ROLL;
                return FillType::ROLL_ACCELERANDO;

            case 9: // Techno - Minimal roll variants
                if (r < 0.4f) return FillType::ROLL_PITCHED;
                if (r < 0.7f) return FillType::ROLL_ACCELERANDO;
                return FillType::BUILDUP;

            default:
                return FillType::ROLL;
        }
    }

    // ========================================
    // Generate Roll fill pattern (Section 4.4)
    // Increasing density toward end
    // Returns velocity values (0.0 = no hit, >0 = velocity)
    // ========================================
    std::vector<float> generateRoll(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Accelerando: start sparse, end dense
        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;
            float density = 0.3f + progress * 0.7f * intensity;

            if (dist(rng) < density) {
                // Velocity increases with progress (crescendo)
                float vel = 0.5f + progress * 0.4f * intensity + velVar(rng);
                pattern[i] = std::clamp(vel, 0.3f, 1.0f);
            }
        }

        // Ensure last hit for resolution (strong)
        pattern[lengthSteps - 1] = std::min(1.0f, 0.9f + velVar(rng));

        return pattern;
    }

    // ========================================
    // Generate Accelerando Roll (16th -> 32nd -> triplet)
    // Based on fills_ornaments_research.md Section 2.6
    // ========================================
    std::vector<float> generateRollAccelerando(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        // Divide into three sections: 16th, 32nd, triplet
        int section1End = lengthSteps / 3;
        int section2End = lengthSteps * 2 / 3;

        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;
            float vel = 0.5f + progress * 0.45f * intensity + velVar(rng);
            bool shouldHit = false;

            if (i < section1End) {
                // Section 1: 16th notes (every 4th position in 64-step grid)
                shouldHit = (i % 4 == 0);
            } else if (i < section2End) {
                // Section 2: 32nd notes (every 2nd position)
                shouldHit = (i % 2 == 0);
            } else {
                // Section 3: triplets / continuous (every position)
                shouldHit = true;
            }

            if (shouldHit) {
                pattern[i] = std::clamp(vel, 0.4f, 1.0f);
            }
        }

        // Strong resolution
        pattern[lengthSteps - 1] = std::clamp(0.95f + velVar(rng), 0.9f, 1.0f);

        return pattern;
    }

    // ========================================
    // Generate Triplet Roll (swing feel)
    // ========================================
    std::vector<float> generateRollTriplet(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        // Triplet feel: hits on positions 0, 2, 3, 5, 6, 8... (every 3rd skipped)
        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;
            int tripletPos = i % 3;

            // Hit on first two of each triplet group
            bool shouldHit = (tripletPos != 2);

            // Increase density toward end
            if (!shouldHit && progress > 0.7f) {
                shouldHit = true;  // Fill in gaps near end
            }

            if (shouldHit) {
                float vel = 0.5f + progress * 0.4f * intensity + velVar(rng);
                // Accent first of each triplet
                if (tripletPos == 0) vel += 0.1f;
                pattern[i] = std::clamp(vel, 0.35f, 1.0f);
            }
        }

        pattern[lengthSteps - 1] = std::clamp(0.95f + velVar(rng), 0.9f, 1.0f);

        return pattern;
    }

    // ========================================
    // Generate Stutter Roll (Trap-style)
    // Based on fills_ornaments_research.md Section 2.7
    // ========================================
    std::vector<float> generateRollStutter(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Base 16th note pattern with occasional stutters
        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;

            // 16th note base
            if (i % 4 == 0) {
                float vel = 0.6f + progress * 0.3f * intensity + velVar(rng);
                pattern[i] = std::clamp(vel, 0.4f, 1.0f);
            }

            // Stutter: 32nd note pairs at phrase boundaries
            if (i % 8 == 6 || i % 8 == 7) {
                if (progress > 0.3f && dist(rng) < intensity * 0.7f) {
                    // v0.19: 修正缺少的 intensity 乘數
                    float vel = 0.5f + progress * 0.35f * intensity + velVar(rng);
                    pattern[i] = std::clamp(vel, 0.35f, 0.9f);
                }
            }

            // Machine gun burst at end (64th notes)
            if (progress > 0.85f && i % 2 == 0) {
                float vel = 0.7f + (progress - 0.85f) * 2.0f * intensity + velVar(rng);
                pattern[i] = std::clamp(vel, 0.5f, 1.0f);
            }
        }

        pattern[lengthSteps - 1] = 1.0f;

        return pattern;
    }

    // ========================================
    // Generate Pitched Roll (pitch rises)
    // Returns velocity + pitch offset data
    // ========================================
    struct PitchedRollNote {
        float velocity;
        float pitchOffset;  // In semitones, 0-12
    };

    std::vector<PitchedRollNote> generateRollPitched(int lengthSteps, float intensity) {
        std::vector<PitchedRollNote> pattern(lengthSteps);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;

            // Hit every position in latter half, sparser in first half
            bool shouldHit = (i % 2 == 0) || (progress > 0.5f);

            if (shouldHit) {
                pattern[i].velocity = std::clamp(
                    0.5f + progress * 0.45f * intensity + velVar(rng),
                    0.4f, 1.0f);
                // Pitch rises from 0 to 12 semitones
                pattern[i].pitchOffset = progress * 12.0f * intensity;
            } else {
                pattern[i].velocity = 0.0f;
                pattern[i].pitchOffset = 0.0f;
            }
        }

        pattern[lengthSteps - 1].velocity = 1.0f;
        pattern[lengthSteps - 1].pitchOffset = 12.0f * intensity;

        return pattern;
    }

    // ========================================
    // Generate Tihai fill pattern (Section 4.3)
    // 精確數學公式：Total = (Phrase × 3) + (Gap × 2)
    // 最後一擊必須落在 Sam (beat 1 = pattern 結尾)
    // ========================================
    std::vector<float> generateTihai(int lengthSteps, float intensity) {
        // 最小長度檢查：Tihai 至少需要 8 步才能形成有意義的結構
        // 最小組合 (P=2, G=1) = 2*3 + 1*2 = 8 steps
        static constexpr int TIHAI_MIN_LENGTH = 8;

        if (lengthSteps < TIHAI_MIN_LENGTH) {
            // 太短，降級為簡單的 Roll
            return generateRoll(lengthSteps, intensity);
        }

        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        // 使用精確 Tihai 公式計算 phrase 和 gap
        // Total = 3P + 2G，且最後一擊落在 Sam
        // 常見組合：(P=3, G=1) -> 11 steps, (P=5, G=2) -> 19 steps
        //          (P=4, G=1) -> 14 steps, (P=5, G=1) -> 17 steps

        int phraseLength, gapLength;

        // 選擇最適合的 phrase/gap 組合
        if (lengthSteps >= 19) {
            phraseLength = 5; gapLength = 2;  // 19 steps
        } else if (lengthSteps >= 17) {
            phraseLength = 5; gapLength = 1;  // 17 steps
        } else if (lengthSteps >= 14) {
            phraseLength = 4; gapLength = 1;  // 14 steps
        } else if (lengthSteps >= 11) {
            phraseLength = 3; gapLength = 1;  // 11 steps
        } else {
            // 8-10 steps：最小有效 Tihai
            phraseLength = 2; gapLength = 1;  // 8 steps
        }

        int totalTihaiLength = phraseLength * 3 + gapLength * 2;

        // 計算起始位置，使最後一擊落在 Sam (pattern 最後一步)
        int samPosition = lengthSteps - 1;
        int startPos = samPosition - totalTihaiLength + 1;
        if (startPos < 0) startPos = 0;

        // 生成 Tihai phrase 模式（可自訂，這裡用簡單的節奏型）
        std::vector<float> phrasePattern = generateTihaiPhrase(phraseLength, intensity);

        int pos = startPos;

        // 三次重複，力度遞增（傳統 Tihai 特徵）
        for (int rep = 0; rep < 3; rep++) {
            float repScale = 0.7f + rep * 0.15f;  // 70%, 85%, 100%

            for (int i = 0; i < phraseLength && pos < lengthSteps; i++) {
                float vel = phrasePattern[i] * repScale;
                pattern[pos++] = std::clamp(vel + velVar(rng), 0.3f, 1.0f);
            }

            // Gap（靜默）
            if (rep < 2) {
                pos += gapLength;
            }
        }

        // 確保最後一擊（Sam）是最強音
        if (samPosition < lengthSteps) {
            pattern[samPosition] = std::clamp(1.0f + velVar(rng), 0.95f, 1.0f);
        }

        return pattern;
    }

    // ========================================
    // 生成 Tihai 樂句模式
    // 傳統 Tabla Tihai 常用 Bol：Dha Dhin Dhin / Ta Tin Tin
    // ========================================
    std::vector<float> generateTihaiPhrase(int length, float intensity) {
        std::vector<float> phrase(length, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        // 傳統 Tihai 樂句結構：強-中-弱 或 強-弱-中-弱
        for (int i = 0; i < length; i++) {
            float baseVel;
            if (i == 0) {
                baseVel = 0.95f;  // 首音最強（Dha/Ta）
            } else if (i == length - 1) {
                baseVel = 0.75f;  // 尾音中等
            } else {
                baseVel = 0.6f + (static_cast<float>(i) / length) * 0.15f;
            }
            phrase[i] = std::clamp(baseVel * intensity + velVar(rng), 0.4f, 1.0f);
        }

        return phrase;
    }

    // ========================================
    // Generate Buildup fill pattern
    // Gradual density increase
    // ========================================
    std::vector<float> generateBuildup(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        for (int i = 0; i < lengthSteps; i++) {
            float progress = static_cast<float>(i) / lengthSteps;
            // Exponential density increase
            float density = 0.1f + std::pow(progress, 2.0f) * 0.9f * intensity;

            if (dist(rng) < density) {
                // Velocity also builds up
                float vel = 0.4f + progress * 0.5f * intensity + velVar(rng);
                pattern[i] = std::clamp(vel, 0.3f, 1.0f);
            }
        }

        return pattern;
    }

    // ========================================
    // Generate Break pattern (silence with accent)
    // ========================================
    std::vector<float> generateBreak(int lengthSteps) {
        std::vector<float> pattern(lengthSteps, 0.0f);

        // Only hit on first and last beat (strong accents)
        pattern[0] = 0.95f;
        if (lengthSteps > 1) {
            pattern[lengthSteps - 1] = 1.0f;  // Final hit strongest
        }

        return pattern;
    }

    // ========================================
    // Generate Angsel pattern (Gamelan coordinated break)
    // Angsel = 同步中斷點，所有樂器協調停止後重新進入
    // 結構：[信號音] -> [靜默] -> [齊奏重音]
    // ========================================
    struct AngselPattern {
        std::vector<float> velocities;
        int silenceStart;       // 靜默開始位置
        int silenceEnd;         // 靜默結束位置
        bool isUnison;          // 是否齊奏（供多角色協調用）
    };

    AngselPattern generateAngsel(int lengthSteps, float intensity) {
        // 最小長度檢查：Angsel 需要至少 8 步才能形成有意義的三段結構
        // 信號區 2 步 + 靜默區 4 步 + 重入區 2 步 = 8 步
        static constexpr int ANGSEL_MIN_LENGTH = 8;

        AngselPattern angsel;
        angsel.velocities.resize(lengthSteps, 0.0f);
        angsel.isUnison = true;

        if (lengthSteps < ANGSEL_MIN_LENGTH) {
            // 太短，降級為簡單的信號音 + 結尾強音
            std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
            angsel.velocities[0] = std::clamp(0.85f * intensity + velVar(rng), 0.7f, 1.0f);
            if (lengthSteps > 1) {
                angsel.velocities[lengthSteps - 1] = std::clamp(1.0f + velVar(rng), 0.95f, 1.0f);
            }
            angsel.silenceStart = 1;
            angsel.silenceEnd = lengthSteps - 1;
            return angsel;
        }

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        // Angsel 結構比例（基於傳統 Gamelan）
        // 信號區：前 25%，靜默區：中間 50%，重入區：後 25%
        int signalEnd = lengthSteps / 4;
        int silenceStart = signalEnd;
        int silenceEnd = lengthSteps * 3 / 4;
        int reentryStart = silenceEnd;

        angsel.silenceStart = silenceStart;
        angsel.silenceEnd = silenceEnd;

        // 信號音：短促的節奏型態引導進入 Angsel
        // 傳統 Gamelan 使用特定的 cue pattern
        if (signalEnd >= 2) {
            // 雙音信號（傳統 Angsel cue）
            angsel.velocities[0] = std::clamp(0.85f * intensity + velVar(rng), 0.7f, 1.0f);
            angsel.velocities[signalEnd - 1] = std::clamp(0.9f * intensity + velVar(rng), 0.75f, 1.0f);
        } else if (signalEnd >= 1) {
            angsel.velocities[0] = std::clamp(0.9f * intensity + velVar(rng), 0.8f, 1.0f);
        }

        // 靜默區：完全靜音（核心特徵）
        // velocities 已初始化為 0，無需處理

        // 重入區：齊奏強音
        // 傳統 Gamelan 在 Angsel 結束時所有樂器同時奏出強音
        if (reentryStart < lengthSteps) {
            // 重入首音（所有角色同步）
            angsel.velocities[reentryStart] = std::clamp(1.0f + velVar(rng), 0.95f, 1.0f);

            // 後續漸弱（回歸正常）
            for (int i = reentryStart + 1; i < lengthSteps; i++) {
                float decay = 1.0f - (static_cast<float>(i - reentryStart) / (lengthSteps - reentryStart));
                if (i % 2 == 0) {  // 每隔一步
                    angsel.velocities[i] = std::clamp(0.7f * decay * intensity + velVar(rng), 0.4f, 0.85f);
                }
            }
        }

        return angsel;
    }

    // 簡化版本（只返回 velocity 向量，供 generateFillPattern 使用）
    std::vector<float> generateAngselSimple(int lengthSteps, float intensity) {
        return generateAngsel(lengthSteps, intensity).velocities;
    }

    // ========================================
    // Generate Signal pattern (lead phrase)
    // ========================================
    std::vector<float> generateSignal(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Start with strong accent
        pattern[0] = std::clamp(0.9f + velVar(rng), 0.8f, 1.0f);

        // Fill with syncopated hits
        for (int i = 1; i < lengthSteps - 1; i++) {
            // Prefer off-beats
            float prob = (i % 2 == 1) ? 0.6f * intensity : 0.3f * intensity;
            if (dist(rng) < prob) {
                float vel = 0.5f + intensity * 0.3f + velVar(rng);
                pattern[i] = std::clamp(vel, 0.3f, 0.85f);
            }
        }

        // End with accent
        pattern[lengthSteps - 1] = std::clamp(0.95f + velVar(rng), 0.85f, 1.0f);

        return pattern;
    }

    // ========================================
    // Generate Llamada pattern (Afro-Cuban call phrase, v0.19)
    // Llamada = 呼喚短語，所有角色同步回應
    // 結構：強重音開頭 + clave-aligned 短語 + 強重音結尾
    // ========================================
    std::vector<float> generateLlamada(int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        // 最小長度檢查：Llamada 至少需要 4 步
        if (lengthSteps < 4) {
            // 降級為簡單的開頭 + 結尾重音
            pattern[0] = std::clamp(0.95f * intensity + velVar(rng), 0.85f, 1.0f);
            if (lengthSteps > 1) {
                pattern[lengthSteps - 1] = std::clamp(1.0f + velVar(rng), 0.95f, 1.0f);
            }
            return pattern;
        }

        // Llamada 典型結構（基於 3-2 Son Clave）
        // 開頭強重音（呼喚）
        pattern[0] = std::clamp(0.95f * intensity + velVar(rng), 0.85f, 1.0f);

        // 中段：Clave-aligned 短語
        // 3-2 Son Clave 位置：0, 3, 6, 10, 12（對應 16 分音符網格）
        // 根據 lengthSteps 映射這些位置
        std::vector<int> clavePositions;
        if (lengthSteps >= 16) {
            clavePositions = {0, 3, 6, 10, 12};
        } else if (lengthSteps >= 8) {
            // 壓縮版本（8-15 steps）
            clavePositions = {0, 2, 4, 6, 7};
        } else {
            // 簡化版本（4-7 steps）
            clavePositions = {0, 1, 3};
        }

        // 在 clave 位置放置音符（力度遞增，趨向高潮）
        for (int i = 1; i < lengthSteps - 1; i++) {
            // 映射到 clave 網格
            int mappedPos = static_cast<int>(std::round((i * 16.0f) / lengthSteps));

            // 檢查是否在 clave 位置附近
            bool isClavePos = false;
            for (int clavePos : clavePositions) {
                if (std::abs(mappedPos - clavePos) <= 1) {
                    isClavePos = true;
                    break;
                }
            }

            if (isClavePos) {
                // Clave 位置：放置音符
                float progress = static_cast<float>(i) / lengthSteps;
                float vel = (0.7f + progress * 0.2f) * intensity + velVar(rng);
                pattern[i] = std::clamp(vel, 0.6f, 0.95f);
            }
        }

        // 結尾強重音（解決）- 所有角色齊奏
        pattern[lengthSteps - 1] = std::clamp(1.0f + velVar(rng), 0.95f, 1.0f);

        // 確保至少有 3-4 個音符（呼喚特徵）
        int noteCount = 0;
        for (int i = 0; i < lengthSteps; i++) {
            if (pattern[i] > 0.0f) noteCount++;
        }

        // 如果太稀疏，補充音符
        if (noteCount < 3 && lengthSteps >= 8) {
            int midPoint = lengthSteps / 2;
            if (pattern[midPoint] < 0.01f) {
                pattern[midPoint] = std::clamp(0.75f * intensity + velVar(rng), 0.65f, 0.9f);
            }
        }

        return pattern;
    }

    // ========================================
    // Generate fill pattern based on type
    // Returns velocity values (0.0 = no hit, >0 = velocity)
    // ========================================
    std::vector<float> generateFillPattern(FillType type, int lengthSteps, float intensity) {
        switch (type) {
            case FillType::ROLL:
                return generateRoll(lengthSteps, intensity);
            case FillType::TIHAI:
                return generateTihai(lengthSteps, intensity);
            case FillType::BUILDUP:
                return generateBuildup(lengthSteps, intensity);
            case FillType::BREAK:
                return generateBreak(lengthSteps);
            case FillType::SIGNAL:
                return generateSignal(lengthSteps, intensity);
            case FillType::ROLL_ACCELERANDO:
                return generateRollAccelerando(lengthSteps, intensity);
            case FillType::ROLL_TRIPLET:
                return generateRollTriplet(lengthSteps, intensity);
            case FillType::ROLL_STUTTER:
                return generateRollStutter(lengthSteps, intensity);
            case FillType::ROLL_PITCHED: {
                // Convert pitched roll to velocity-only for compatibility
                auto pitched = generateRollPitched(lengthSteps, intensity);
                std::vector<float> result(lengthSteps);
                for (int i = 0; i < lengthSteps; i++) {
                    result[i] = pitched[i].velocity;
                }
                return result;
            }
            case FillType::ANGSEL:
                return generateAngselSimple(lengthSteps, intensity);
            case FillType::LLAMADA:
                return generateLlamada(lengthSteps, intensity);
            default:
                return std::vector<float>(lengthSteps, 0.0f);
        }
    }

    // ========================================
    // Select extended roll type for electronic styles
    // ========================================
    FillType selectExtendedRollType(int styleIndex) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float r = dist(rng);

        switch (styleIndex) {
            case 7:  // Electronic
                if (r < 0.3f) return FillType::ROLL_ACCELERANDO;
                if (r < 0.6f) return FillType::ROLL_PITCHED;
                return FillType::ROLL;

            case 8:  // Breakbeat
                if (r < 0.4f) return FillType::ROLL_STUTTER;
                if (r < 0.7f) return FillType::ROLL_ACCELERANDO;
                return FillType::ROLL;

            case 9:  // Techno
                if (r < 0.5f) return FillType::ROLL_PITCHED;
                return FillType::ROLL_ACCELERANDO;

            case 6:  // Jazz
                if (r < 0.6f) return FillType::ROLL_TRIPLET;
                return FillType::ROLL;

            default:
                return FillType::ROLL;
        }
    }

    // ========================================
    // Get role-specific fill behavior (Section 7 of research)
    // Timeline: Maintains during others' fills OR plays signal only
    // Foundation: Sparse fills, phrase boundaries only (BUILDUP)
    // Groove: Most active fills, interlock with foundation
    // Lead: Extended improvisation, tihai-style endings
    // ========================================
    bool shouldRoleFill(Role role, FillType fillType) {
        switch (role) {
            case TIMELINE:
                // Timeline maintains pattern OR plays signal - very limited fills
                return (fillType == FillType::SIGNAL);

            case FOUNDATION:
                // Foundation: sparse fills at phrase boundaries only
                return (fillType == FillType::BUILDUP);

            case GROOVE:
                // Groove: most active fills - all types except BREAK (too silent)
                return (fillType != FillType::BREAK);

            case LEAD:
                // Lead: extended improvisation, tihai endings - all types except BREAK
                return (fillType != FillType::BREAK);

            default:
                return true;
        }
    }

    // ========================================
    // Modify role fill intensity (Section 7)
    // ========================================
    float getRoleFillIntensity(Role role, float baseIntensity) {
        switch (role) {
            case TIMELINE:
                return baseIntensity * 0.3f;  // Subtle
            case FOUNDATION:
                return baseIntensity * 0.5f;  // Moderate
            case GROOVE:
                return baseIntensity * 1.0f;  // Full
            case LEAD:
                return baseIntensity * 1.2f;  // Enhanced
            default:
                return baseIntensity;
        }
    }
};

} // namespace WorldRhythm
