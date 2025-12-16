#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <set>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Kotekan Engine - Balinese Interlocking Patterns
// ========================================
// Based on fills_ornaments_research.md Section 2.4
//
// Kotekan: Two-part interlocking technique from Balinese Gamelan
// - Polos: On-beat tendency (simpler part)
// - Sangsih: Off-beat tendency (complementary part)
// - Combined: Creates illusion of single melody faster than one can play
//
// Types:
// - Nyog Cag: Strict alternation
// - Norot: Both parts anticipate melody note
// - Kotekan Telu: 3-pitch pattern
// - Kotekan Empat: 4-pitch pattern

enum class KotekanType {
    NYOG_CAG = 0,   // Strict alternation (every other note)
    NOROT,          // Anticipation pattern
    KOTEKAN_TELU,   // 3-pitch shared pattern
    KOTEKAN_EMPAT,  // 4-pitch shared pattern
    UBIT_UBITAN,    // Fast interlocking figuration
    NUM_TYPES
};

struct KotekanDefinition {
    KotekanType type;
    std::string name;
    std::string description;
    int minSubdivision;     // Minimum subdivisions per beat
    float polosDensity;     // Typical density for polos
    float sangsihDensity;   // Typical density for sangsih
};

// ========================================
// Kotekan Type Definitions
// ========================================
inline KotekanDefinition createNyogCag() {
    KotekanDefinition k;
    k.type = KotekanType::NYOG_CAG;
    k.name = "Nyog Cag";
    k.description = "Strict alternation, every other note";
    k.minSubdivision = 4;
    k.polosDensity = 0.5f;
    k.sangsihDensity = 0.5f;
    return k;
}

inline KotekanDefinition createNorot() {
    KotekanDefinition k;
    k.type = KotekanType::NOROT;
    k.name = "Norot";
    k.description = "Both parts anticipate next melody note";
    k.minSubdivision = 4;
    k.polosDensity = 0.6f;
    k.sangsihDensity = 0.4f;
    return k;
}

inline KotekanDefinition createKotekanTelu() {
    KotekanDefinition k;
    k.type = KotekanType::KOTEKAN_TELU;
    k.name = "Kotekan Telu";
    k.description = "3-pitch pattern shared between parts";
    k.minSubdivision = 6;
    k.polosDensity = 0.55f;
    k.sangsihDensity = 0.45f;
    return k;
}

inline KotekanDefinition createKotekanEmpat() {
    KotekanDefinition k;
    k.type = KotekanType::KOTEKAN_EMPAT;
    k.name = "Kotekan Empat";
    k.description = "4-pitch pattern shared between parts";
    k.minSubdivision = 8;
    k.polosDensity = 0.5f;
    k.sangsihDensity = 0.5f;
    return k;
}

inline KotekanDefinition createUbitUbitan() {
    KotekanDefinition k;
    k.type = KotekanType::UBIT_UBITAN;
    k.name = "Ubit-ubitan";
    k.description = "Fast interlocking figuration";
    k.minSubdivision = 8;
    k.polosDensity = 0.6f;
    k.sangsihDensity = 0.6f;
    return k;
}

// ========================================
// Kotekan Pair Result
// ========================================
struct KotekanPair {
    Pattern polos;
    Pattern sangsih;
    Pattern combined;   // What it sounds like together
    KotekanType type;
    float density;
};

// ========================================
// Kotekan Engine Class
// ========================================
class KotekanEngine {
private:
    std::mt19937 rng;
    std::vector<KotekanDefinition> types;
    KotekanType currentType = KotekanType::NYOG_CAG;
    float intensityMultiplier = 1.0f;

public:
    KotekanEngine() : rng(std::random_device{}()) {
        types.push_back(createNyogCag());
        types.push_back(createNorot());
        types.push_back(createKotekanTelu());
        types.push_back(createKotekanEmpat());
        types.push_back(createUbitUbitan());
    }

    void seed(unsigned int s) { rng.seed(s); }

    void setType(KotekanType type) { currentType = type; }
    void setTypeByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(KotekanType::NUM_TYPES)) {
            currentType = static_cast<KotekanType>(index);
        }
    }

    KotekanType getCurrentType() const { return currentType; }
    const KotekanDefinition& getCurrentDefinition() const {
        return types[static_cast<int>(currentType)];
    }

    int getNumTypes() const { return static_cast<int>(types.size()); }

    void setIntensity(float intensity) {
        intensityMultiplier = std::clamp(intensity, 0.0f, 2.0f);
    }

    // ========================================
    // Generate Nyog Cag - Strict Alternation
    // ========================================
    KotekanPair generateNyogCag(int length, float baseVelocity, float density = 1.0f) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = KotekanType::NYOG_CAG;

        // Density 0 = empty patterns
        if (density < 0.01f) {
            result.density = 0.0f;
            return result;
        }

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> densityDist(0.0f, 1.0f);

        for (int i = 0; i < length; i++) {
            // Apply density as probability filter
            if (densityDist(rng) > density) {
                continue;  // Skip this position based on density
            }

            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            if (i % 2 == 0) {
                // Polos on even positions
                result.polos.setOnset(i, vel);
            } else {
                // Sangsih on odd positions
                result.sangsih.setOnset(i, vel * 0.9f);
            }
            result.combined.setOnset(i, vel);
        }

        result.density = density;
        return result;
    }

    // ========================================
    // Generate Norot - Anticipation Pattern
    // ========================================
    KotekanPair generateNorot(int length, float baseVelocity,
                              const std::vector<int>& melodyPositions, float density = 1.0f) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = KotekanType::NOROT;

        // Density 0 = empty patterns
        if (density < 0.01f) {
            result.density = 0.0f;
            return result;
        }

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> densityDist(0.0f, 1.0f);

        // Convert melody positions to set for fast lookup
        std::set<int> melodySet(melodyPositions.begin(), melodyPositions.end());

        for (int i = 0; i < length; i++) {
            bool isMelodyPos = melodySet.count(i) > 0;
            bool isBeforeMelody = melodySet.count((i + 1) % length) > 0;

            // Melody positions are always included if density > 0
            // Other positions are filtered by density
            bool isImportantPos = isMelodyPos || isBeforeMelody;
            if (!isImportantPos && densityDist(rng) > density) {
                continue;
            }

            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            if (isMelodyPos) {
                // Both play on melody notes (unison)
                result.polos.setOnset(i, vel);
                result.sangsih.setOnset(i, vel * 0.85f);
                result.combined.setOnset(i, vel);
            } else if (isBeforeMelody) {
                // Anticipation: sangsih plays before melody
                result.sangsih.setOnset(i, vel * 0.7f);
                result.combined.setOnset(i, vel * 0.7f);
            } else if (i % 2 == 0) {
                // Fill with alternation
                result.polos.setOnset(i, vel * 0.6f);
                result.combined.setOnset(i, vel * 0.6f);
            } else {
                result.sangsih.setOnset(i, vel * 0.5f);
                result.combined.setOnset(i, vel * 0.5f);
            }
        }

        result.density = density * 0.8f;
        return result;
    }

    // ========================================
    // Generate Kotekan Telu - 3-Pitch Pattern
    // ========================================
    KotekanPair generateKotekanTelu(int length, float baseVelocity, float density = 1.0f) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = KotekanType::KOTEKAN_TELU;

        // Density 0 = empty patterns
        if (density < 0.01f) {
            result.density = 0.0f;
            return result;
        }

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> densityDist(0.0f, 1.0f);

        // 3-note cycle: P-S-P, S-P-S pattern
        // Creates interlocking triplet feel
        for (int i = 0; i < length; i++) {
            int phase = i % 6;
            bool isStrongBeat = (phase == 0 || phase == 3);

            // Strong beats are more likely to be included
            float effectiveDensity = isStrongBeat ? std::min(1.0f, density * 1.5f) : density;
            if (densityDist(rng) > effectiveDensity) {
                continue;
            }

            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            switch (phase) {
                case 0: // Polos strong
                    result.polos.setOnset(i, vel);
                    result.polos.accents[i] = true;
                    break;
                case 1: // Sangsih
                    result.sangsih.setOnset(i, vel * 0.8f);
                    break;
                case 2: // Polos
                    result.polos.setOnset(i, vel * 0.7f);
                    break;
                case 3: // Sangsih strong
                    result.sangsih.setOnset(i, vel);
                    result.sangsih.accents[i] = true;
                    break;
                case 4: // Polos
                    result.polos.setOnset(i, vel * 0.8f);
                    break;
                case 5: // Sangsih
                    result.sangsih.setOnset(i, vel * 0.7f);
                    break;
            }
            result.combined.setOnset(i, vel * 0.85f);
        }

        result.density = density;
        return result;
    }

    // ========================================
    // Generate Kotekan Empat - 4-Pitch Pattern
    // ========================================
    KotekanPair generateKotekanEmpat(int length, float baseVelocity, float density = 1.0f) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = KotekanType::KOTEKAN_EMPAT;

        // Density 0 = empty patterns
        if (density < 0.01f) {
            result.density = 0.0f;
            return result;
        }

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> densityDist(0.0f, 1.0f);

        // 4-note cycle with interlock
        for (int i = 0; i < length; i++) {
            int phase = i % 8;
            bool isAccentBeat = (phase == 0 || phase == 4);

            // Accent beats are more likely to be included
            float effectiveDensity = isAccentBeat ? std::min(1.0f, density * 1.5f) : density;
            if (densityDist(rng) > effectiveDensity) {
                continue;
            }

            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            switch (phase) {
                case 0: // Polos accent
                    result.polos.setOnset(i, vel);
                    result.polos.accents[i] = true;
                    break;
                case 1: // Sangsih
                    result.sangsih.setOnset(i, vel * 0.75f);
                    break;
                case 2: // Polos
                    result.polos.setOnset(i, vel * 0.8f);
                    break;
                case 3: // Sangsih
                    result.sangsih.setOnset(i, vel * 0.7f);
                    break;
                case 4: // Sangsih accent
                    result.sangsih.setOnset(i, vel);
                    result.sangsih.accents[i] = true;
                    break;
                case 5: // Polos
                    result.polos.setOnset(i, vel * 0.75f);
                    break;
                case 6: // Sangsih
                    result.sangsih.setOnset(i, vel * 0.8f);
                    break;
                case 7: // Polos
                    result.polos.setOnset(i, vel * 0.7f);
                    break;
            }
            result.combined.setOnset(i, vel * 0.85f);
        }

        result.density = density;
        return result;
    }

    // ========================================
    // Generate Ubit-ubitan - Fast Figuration
    // ========================================
    KotekanPair generateUbitUbitan(int length, float baseVelocity, float density) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = KotekanType::UBIT_UBITAN;

        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

        // More complex interlocking with variable density
        for (int i = 0; i < length; i++) {
            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            // Base alternation
            bool isPolosPos = (i % 2 == 0);

            // Add some complexity based on position in phrase
            int phrasePos = i % 8;
            float localDensity = density;

            // Increase density at phrase boundaries
            if (phrasePos == 0 || phrasePos == 4) {
                localDensity *= 1.2f;
            }

            if (probDist(rng) < localDensity) {
                if (isPolosPos) {
                    result.polos.setOnset(i, vel);
                    // Occasional sangsih doubling
                    if (probDist(rng) < 0.15f) {
                        result.sangsih.setOnset(i, vel * 0.5f);
                    }
                } else {
                    result.sangsih.setOnset(i, vel * 0.9f);
                    // Occasional polos doubling
                    if (probDist(rng) < 0.15f) {
                        result.polos.setOnset(i, vel * 0.5f);
                    }
                }
                result.combined.setOnset(i, vel * 0.9f);
            }
        }

        result.density = density;
        return result;
    }

    // ========================================
    // Generate Kotekan by Current Type
    // ========================================
    KotekanPair generate(int length, float baseVelocity, float density = 0.8f) {
        // Density 0 = empty patterns for all types
        if (density < 0.01f) {
            KotekanPair result;
            result.polos = Pattern(length);
            result.sangsih = Pattern(length);
            result.combined = Pattern(length);
            result.type = currentType;
            result.density = 0.0f;
            return result;
        }

        switch (currentType) {
            case KotekanType::NYOG_CAG:
                return generateNyogCag(length, baseVelocity, density);
            case KotekanType::NOROT: {
                // Generate default melody positions (every 4 steps)
                std::vector<int> melodyPos;
                for (int i = 0; i < length; i += 4) {
                    melodyPos.push_back(i);
                }
                return generateNorot(length, baseVelocity, melodyPos, density);
            }
            case KotekanType::KOTEKAN_TELU:
                return generateKotekanTelu(length, baseVelocity, density);
            case KotekanType::KOTEKAN_EMPAT:
                return generateKotekanEmpat(length, baseVelocity, density);
            case KotekanType::UBIT_UBITAN:
                return generateUbitUbitan(length, baseVelocity, density);
            default:
                return generateNyogCag(length, baseVelocity, density);
        }
    }

    // ========================================
    // Apply Kotekan to Existing Pattern
    // ========================================
    KotekanPair applyKotekan(const Pattern& source, float intensity) {
        KotekanPair result;
        result.polos = Pattern(source.length);
        result.sangsih = Pattern(source.length);
        result.combined = Pattern(source.length);
        result.type = currentType;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (int i = 0; i < source.length; i++) {
            if (source.hasOnsetAt(i)) {
                float vel = source.getVelocity(i) * intensity + velVar(rng);
                vel = std::clamp(vel, 0.3f, 1.0f);

                // Distribute to polos/sangsih based on position
                if (i % 2 == 0) {
                    result.polos.setOnset(i, vel);
                    result.polos.accents[i] = source.accents[i];
                } else {
                    result.sangsih.setOnset(i, vel * 0.9f);
                    result.sangsih.accents[i] = source.accents[i];
                }
                result.combined.setOnset(i, vel);
            }
        }

        return result;
    }

    // ========================================
    // Generate Complementary Pair from Pattern
    // ========================================
    KotekanPair generateComplementary(const Pattern& base, float sangsihIntensity) {
        KotekanPair result;
        result.polos = base;  // Copy base as polos
        result.sangsih = Pattern(base.length);
        result.combined = Pattern(base.length);
        result.type = KotekanType::NYOG_CAG;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (int i = 0; i < base.length; i++) {
            if (base.hasOnsetAt(i)) {
                result.combined.setOnset(i, base.getVelocity(i));
            } else {
                // Sangsih fills gaps
                float vel = sangsihIntensity + velVar(rng);
                vel = std::clamp(vel, 0.3f, 1.0f);
                result.sangsih.setOnset(i, vel);
                result.combined.setOnset(i, vel);
            }
        }

        return result;
    }

    // ========================================
    // Get Type Name
    // ========================================
    const char* getTypeName(KotekanType type) const {
        return types[static_cast<int>(type)].name.c_str();
    }

    const char* getCurrentTypeName() const {
        return getTypeName(currentType);
    }

    // ========================================
    // Get Recommended Type for Style
    // ========================================
    KotekanType getRecommendedType(int styleIndex) const {
        switch (styleIndex) {
            case 0:  // West African
                return KotekanType::NYOG_CAG;  // Hocketing
            case 1:  // Afro-Cuban
                return KotekanType::NOROT;     // Tumbao interlock
            case 2:  // Brazilian
                return KotekanType::UBIT_UBITAN;  // Batucada weave
            case 3:  // Balkan
                return KotekanType::KOTEKAN_TELU;  // Asymmetric feel
            case 4:  // Indian
                return KotekanType::NOROT;     // Tihai-like anticipation
            case 5:  // Gamelan
                return KotekanType::KOTEKAN_EMPAT;  // Traditional
            case 6:  // Jazz
                return KotekanType::UBIT_UBITAN;  // Conversation
            case 7:  // Electronic
                return KotekanType::NYOG_CAG;  // Clean interlock
            case 8:  // Breakbeat
                return KotekanType::UBIT_UBITAN;  // Complex layers
            case 9:  // Techno
                return KotekanType::NYOG_CAG;  // Mechanical precision
            default:
                return KotekanType::NYOG_CAG;
        }
    }

    // ========================================
    // Calculate Combined Density
    // ========================================
    float getCombinedDensity(const KotekanPair& pair) const {
        int onsets = 0;
        for (int i = 0; i < pair.combined.length; i++) {
            if (pair.combined.hasOnsetAt(i)) onsets++;
        }
        return static_cast<float>(onsets) / pair.combined.length;
    }

    // ========================================
    // v0.18.2: Polos-Sangsih 互鎖驗證與修正
    // ========================================

    /**
     * 驗證 Polos 和 Sangsih 是否正確互鎖
     * 真正的 Kotekan 要求：
     * 1. 互補性：在同一步不應該都有強音
     * 2. 連續性：合成後應該形成連續的旋律線
     * 3. 平衡性：兩者音符數量應相近
     */
    struct InterlockValidation {
        bool isValid;
        float complementarity;   // 互補性分數 (0-1)
        float continuity;        // 連續性分數 (0-1)
        float balance;           // 平衡性分數 (0-1)
        int conflictCount;       // 衝突步數
        int gapCount;            // 空白步數
        std::string message;
    };

    InterlockValidation validateInterlock(const KotekanPair& pair) const {
        InterlockValidation result;
        result.conflictCount = 0;
        result.gapCount = 0;

        int polosCount = 0;
        int sangsihCount = 0;
        int totalSteps = pair.combined.length;

        for (int i = 0; i < totalSteps; i++) {
            bool hasPolos = pair.polos.hasOnsetAt(i) && pair.polos.getVelocity(i) > 0.3f;
            bool hasSangsih = pair.sangsih.hasOnsetAt(i) && pair.sangsih.getVelocity(i) > 0.3f;

            if (hasPolos) polosCount++;
            if (hasSangsih) sangsihCount++;

            if (hasPolos && hasSangsih) {
                // 兩者同時有強音 = 衝突
                result.conflictCount++;
            } else if (!hasPolos && !hasSangsih) {
                // 都沒有 = 空白
                result.gapCount++;
            }
        }

        // 計算分數
        result.complementarity = 1.0f - (static_cast<float>(result.conflictCount) / totalSteps);
        result.continuity = 1.0f - (static_cast<float>(result.gapCount) / totalSteps);

        // v0.18.5: 強化除零保護
        int maxCount = std::max(polosCount, sangsihCount);
        int minCount = std::min(polosCount, sangsihCount);
        result.balance = (maxCount > 0 && minCount >= 0)
            ? static_cast<float>(minCount) / static_cast<float>(maxCount)
            : 0.0f;

        // 綜合判定
        // NOTE: These thresholds (80%, 60%, 60%) are operational definitions
        // by the author, not derived from Tenzer or other ethnomusicological
        // literature. Tenzer's analyses use qualitative descriptions, not
        // percentage metrics. These values provide reasonable quality control
        // but should not be cited as academic standards.
        result.isValid = (result.complementarity >= 0.8f) &&
                        (result.continuity >= 0.6f) &&
                        (result.balance >= 0.6f);

        // 生成訊息
        if (result.isValid) {
            result.message = "Interlock OK";
        } else {
            result.message = "";
            if (result.complementarity < 0.8f) {
                result.message += "Too many conflicts. ";
            }
            if (result.continuity < 0.6f) {
                result.message += "Too many gaps. ";
            }
            if (result.balance < 0.6f) {
                result.message += "Parts unbalanced. ";
            }
        }

        return result;
    }

    /**
     * 自動修正互鎖問題
     * 確保 Polos 和 Sangsih 真正互補
     */
    KotekanPair enforceInterlock(const KotekanPair& input) {
        KotekanPair result = input;
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (int i = 0; i < input.combined.length; i++) {
            float polosVel = input.polos.getVelocity(i);
            float sangsihVel = input.sangsih.getVelocity(i);
            bool hasPolos = polosVel > 0.1f;
            bool hasSangsih = sangsihVel > 0.1f;

            if (hasPolos && hasSangsih) {
                // 衝突：根據位置決定保留哪個
                // 偶數位置優先 Polos，奇數優先 Sangsih
                if (i % 2 == 0) {
                    result.sangsih.setOnset(i, 0.0f);
                    result.sangsih.accents[i] = false;
                } else {
                    result.polos.setOnset(i, 0.0f);
                    result.polos.accents[i] = false;
                }
            } else if (!hasPolos && !hasSangsih) {
                // 空白：填充一個音
                float fillVel = 0.6f + velVar(rng);
                fillVel = std::clamp(fillVel, 0.4f, 0.8f);

                if (i % 2 == 0) {
                    result.polos.setOnset(i, fillVel);
                } else {
                    result.sangsih.setOnset(i, fillVel);
                }
            }

            // 更新 combined
            float finalVel = std::max(result.polos.getVelocity(i),
                                      result.sangsih.getVelocity(i));
            result.combined.setOnset(i, finalVel);
        }

        return result;
    }

    /**
     * 生成保證互鎖的 Kotekan 對
     * 直接從互補原則開始，而非後修正
     */
    KotekanPair generateGuaranteedInterlock(int length, float baseVelocity,
                                            float polosBias = 0.5f) {
        KotekanPair result;
        result.polos = Pattern(length);
        result.sangsih = Pattern(length);
        result.combined = Pattern(length);
        result.type = currentType;

        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);
        std::uniform_real_distribution<float> biasDist(0.0f, 1.0f);

        // 根據 kotekan 類型決定分配模式
        for (int i = 0; i < length; i++) {
            float vel = baseVelocity * intensityMultiplier + velVar(rng);
            vel = std::clamp(vel, 0.3f, 1.0f);

            bool assignToPolos;

            switch (currentType) {
                case KotekanType::NYOG_CAG:
                    // 嚴格交替
                    assignToPolos = (i % 2 == 0);
                    break;

                case KotekanType::KOTEKAN_TELU:
                    // 3 週期模式
                    assignToPolos = ((i % 3) != 1);  // 0,2 → Polos, 1 → Sangsih
                    break;

                case KotekanType::KOTEKAN_EMPAT:
                    // 4 週期模式
                    {
                        int phase = i % 4;
                        assignToPolos = (phase == 0 || phase == 2);
                    }
                    break;

                case KotekanType::NOROT:
                case KotekanType::UBIT_UBITAN:
                default:
                    // 帶隨機偏差的交替
                    {
                        float bias = (i % 2 == 0) ? polosBias : (1.0f - polosBias);
                        assignToPolos = (biasDist(rng) < bias);
                    }
                    break;
            }

            // 根據分配設定速度
            if (assignToPolos) {
                result.polos.setOnset(i, vel);
                // 強拍加重音
                if (i % 4 == 0) {
                    result.polos.accents[i] = true;
                }
            } else {
                result.sangsih.setOnset(i, vel * 0.9f);  // Sangsih 略低
                if (i % 4 == 2) {
                    result.sangsih.accents[i] = true;
                }
            }

            result.combined.setOnset(i, vel);
        }

        result.density = 1.0f;
        return result;
    }

    /**
     * 從現有 Pattern 分割成互鎖的 Polos 和 Sangsih
     */
    KotekanPair splitIntoKotekan(const Pattern& source, float sangsihRatio = 0.5f) {
        KotekanPair result;
        result.polos = Pattern(source.length);
        result.sangsih = Pattern(source.length);
        result.combined = source;  // 複製來源
        result.type = currentType;

        std::uniform_real_distribution<float> splitDist(0.0f, 1.0f);

        for (int i = 0; i < source.length; i++) {
            if (source.hasOnsetAt(i)) {
                float vel = source.getVelocity(i);

                // 決定分配
                bool toSangsih;
                if (currentType == KotekanType::NYOG_CAG) {
                    toSangsih = (i % 2 == 1);
                } else {
                    // 使用隨機分配，但偏向適當的位置
                    float adjustedRatio = sangsihRatio;
                    if (i % 2 == 1) adjustedRatio += 0.2f;  // 奇數位置偏向 Sangsih
                    toSangsih = (splitDist(rng) < adjustedRatio);
                }

                if (toSangsih) {
                    result.sangsih.setOnset(i, vel);
                    result.sangsih.accents[i] = source.accents[i];
                } else {
                    result.polos.setOnset(i, vel);
                    result.polos.accents[i] = source.accents[i];
                }
            }
        }

        // 確保有足夠的互鎖
        auto validation = validateInterlock(result);
        if (!validation.isValid) {
            result = enforceInterlock(result);
        }

        return result;
    }
};

} // namespace WorldRhythm
