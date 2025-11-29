#pragma once

#include <vector>
#include <random>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Clave Direction and Variant Engine
// ========================================
// Implements various clave patterns with direction (2-3 vs 3-2)
// Based on unified_rhythm_analysis.md

enum class ClaveType {
    SON_3_2,        // Son Clave 3-2: X..X..X...X.X...
    SON_2_3,        // Son Clave 2-3: ...X.X...X..X..X
    RUMBA_3_2,      // Rumba Clave 3-2: X..X...X..X.X...
    RUMBA_2_3,      // Rumba Clave 2-3: ...X.X..X...X..X
    BOSSA_NOVA,     // Bossa Nova Clave: X..X..X...X..X..
    BRAZILIAN_3_2,  // Brazilian adaptation of 3-2
    AFRO_6_8,       // 6/8 Afro-Cuban: X.X.XX.X.X.X
    NUM_CLAVE_TYPES
};

struct ClaveDefinition {
    ClaveType type;
    const char* name;
    int length;                     // Pattern length (typically 16)
    std::vector<int> positions;     // Onset positions
    std::vector<float> weights;     // Accent weights per position
};

// ========================================
// Clave Pattern Definitions
// ========================================

// Son Clave 3-2: The most common clave
// 3-side: X..X..X. (hits on 1, 4, 7)
// 2-side: ..X.X... (hits on 11, 13)
inline ClaveDefinition createSonClave_3_2() {
    ClaveDefinition c;
    c.type = ClaveType::SON_3_2;
    c.name = "Son Clave 3-2";
    c.length = 16;
    c.positions = {0, 3, 6, 10, 12};
    c.weights = {1.0f, 0.8f, 0.9f, 0.85f, 0.9f};
    return c;
}

// Son Clave 2-3: Reversed direction
// 2-side first: ..X.X... then 3-side: X..X..X.
inline ClaveDefinition createSonClave_2_3() {
    ClaveDefinition c;
    c.type = ClaveType::SON_2_3;
    c.name = "Son Clave 2-3";
    c.length = 16;
    c.positions = {2, 4, 8, 11, 14};
    c.weights = {0.85f, 0.9f, 1.0f, 0.8f, 0.9f};
    return c;
}

// Rumba Clave 3-2: Shifted third beat
// 3-side: X..X...X (hits on 1, 4, 8)
// 2-side: ..X.X... (hits on 11, 13)
inline ClaveDefinition createRumbaClave_3_2() {
    ClaveDefinition c;
    c.type = ClaveType::RUMBA_3_2;
    c.name = "Rumba Clave 3-2";
    c.length = 16;
    c.positions = {0, 3, 7, 10, 12};
    c.weights = {1.0f, 0.8f, 0.85f, 0.85f, 0.9f};
    return c;
}

// Rumba Clave 2-3
inline ClaveDefinition createRumbaClave_2_3() {
    ClaveDefinition c;
    c.type = ClaveType::RUMBA_2_3;
    c.name = "Rumba Clave 2-3";
    c.length = 16;
    c.positions = {2, 4, 8, 11, 15};
    c.weights = {0.85f, 0.9f, 1.0f, 0.8f, 0.85f};
    return c;
}

// Bossa Nova Clave (Brazilian adaptation)
inline ClaveDefinition createBossaNovaClave() {
    ClaveDefinition c;
    c.type = ClaveType::BOSSA_NOVA;
    c.name = "Bossa Nova Clave";
    c.length = 16;
    c.positions = {0, 3, 6, 10, 13};
    c.weights = {1.0f, 0.75f, 0.85f, 0.8f, 0.75f};
    return c;
}

// Brazilian 3-2 (Samba clave)
inline ClaveDefinition createBrazilian_3_2() {
    ClaveDefinition c;
    c.type = ClaveType::BRAZILIAN_3_2;
    c.name = "Brazilian 3-2";
    c.length = 16;
    c.positions = {0, 3, 6, 10, 12};
    c.weights = {1.0f, 0.7f, 0.85f, 0.8f, 0.85f};
    return c;
}

// 6/8 Afro-Cuban (12 pulses mapped to 16)
inline ClaveDefinition createAfro_6_8() {
    ClaveDefinition c;
    c.type = ClaveType::AFRO_6_8;
    c.name = "6/8 Afro-Cuban";
    c.length = 16;
    // Mapped from 12-pulse: 1,3,5,6,8,10,12
    c.positions = {0, 2, 5, 6, 8, 11, 13};
    c.weights = {1.0f, 0.7f, 0.8f, 0.9f, 0.75f, 0.8f, 0.85f};
    return c;
}

// ========================================
// Clave Engine Class
// ========================================
class ClaveEngine {
private:
    std::mt19937 rng;
    std::vector<ClaveDefinition> claves;
    ClaveType currentClave = ClaveType::SON_3_2;

public:
    ClaveEngine() : rng(std::random_device{}()) {
        claves.push_back(createSonClave_3_2());
        claves.push_back(createSonClave_2_3());
        claves.push_back(createRumbaClave_3_2());
        claves.push_back(createRumbaClave_2_3());
        claves.push_back(createBossaNovaClave());
        claves.push_back(createBrazilian_3_2());
        claves.push_back(createAfro_6_8());
    }

    void seed(unsigned int s) { rng.seed(s); }

    void setClave(ClaveType type) { currentClave = type; }

    void setClaveByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(claves.size())) {
            currentClave = static_cast<ClaveType>(index);
        }
    }

    const ClaveDefinition& getCurrentClave() const {
        return claves[static_cast<int>(currentClave)];
    }

    const ClaveDefinition& getClave(ClaveType type) const {
        return claves[static_cast<int>(type)];
    }

    int getNumClaves() const { return static_cast<int>(claves.size()); }

    // ========================================
    // Check if position is on clave
    // ========================================
    bool isOnClave(int position, int patternLength) const {
        const ClaveDefinition& clave = getCurrentClave();

        // Map position to clave grid
        int mappedPos = (position * clave.length) / patternLength;
        mappedPos = mappedPos % clave.length;

        for (int clavePos : clave.positions) {
            if (mappedPos == clavePos) return true;
        }
        return false;
    }

    // ========================================
    // Get clave weight for position
    // ========================================
    float getClaveWeight(int position, int patternLength) const {
        const ClaveDefinition& clave = getCurrentClave();

        int mappedPos = (position * clave.length) / patternLength;
        mappedPos = mappedPos % clave.length;

        for (size_t i = 0; i < clave.positions.size(); i++) {
            if (mappedPos == clave.positions[i]) {
                return clave.weights[i];
            }
        }
        return 0.0f;
    }

    // ========================================
    // Generate clave pattern
    // ========================================
    Pattern generateClavePattern(int patternLength, float intensity) {
        Pattern p(patternLength);
        const ClaveDefinition& clave = getCurrentClave();
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (size_t i = 0; i < clave.positions.size(); i++) {
            // Map clave position to pattern length
            int step = (clave.positions[i] * patternLength) / clave.length;
            if (step >= patternLength) step = patternLength - 1;

            float vel = clave.weights[i] * intensity + velVar(rng);
            p.setOnset(step, std::clamp(vel, 0.5f, 1.0f));

            if (clave.weights[i] > 0.85f) {
                p.accents[step] = true;
            }
        }

        return p;
    }

    // ========================================
    // Apply clave constraint to pattern
    // Ensure pattern respects clave alignment
    // ========================================
    void applyClaveConstraint(Pattern& p, float claveStrength) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < p.length; i++) {
            bool onClave = isOnClave(i, p.length);
            float claveWeight = getClaveWeight(i, p.length);

            if (p.hasOnsetAt(i)) {
                if (onClave) {
                    // On clave: boost velocity
                    float vel = p.getVelocity(i);
                    float boost = claveWeight * claveStrength * 0.2f;
                    p.setOnset(i, std::min(vel + boost, 1.0f));
                    if (claveWeight > 0.85f) {
                        p.accents[i] = true;
                    }
                } else {
                    // Off clave: reduce velocity based on claveStrength
                    float vel = p.getVelocity(i);
                    float reduction = (1.0f - claveWeight) * claveStrength * 0.3f;
                    p.setOnset(i, std::max(vel - reduction, 0.3f));
                }
            } else {
                // No onset: maybe add one on clave position
                if (onClave && dist(rng) < claveWeight * claveStrength * 0.5f) {
                    p.setOnset(i, claveWeight * 0.7f);
                }
            }
        }
    }

    // ========================================
    // Generate clave-aware position weights
    // ========================================
    std::vector<float> generateClaveWeights(int patternLength) const {
        std::vector<float> weights(patternLength, 0.3f);  // Base weight
        const ClaveDefinition& clave = getCurrentClave();

        for (size_t i = 0; i < clave.positions.size(); i++) {
            int step = (clave.positions[i] * patternLength) / clave.length;
            if (step < patternLength) {
                weights[step] = clave.weights[i];
            }
        }

        return weights;
    }

    // ========================================
    // Check clave alignment score
    // Returns 0.0-1.0 how well pattern aligns with clave
    // ========================================
    float getClaveAlignmentScore(const Pattern& p) const {
        const ClaveDefinition& clave = getCurrentClave();
        float score = 0.0f;
        float maxScore = 0.0f;

        for (size_t i = 0; i < clave.positions.size(); i++) {
            int step = (clave.positions[i] * p.length) / clave.length;
            if (step < p.length) {
                maxScore += clave.weights[i];
                if (p.hasOnsetAt(step)) {
                    score += clave.weights[i] * p.getVelocity(step);
                }
            }
        }

        return maxScore > 0.0f ? score / maxScore : 0.0f;
    }

    // ========================================
    // Get recommended clave for style
    // ========================================
    ClaveType getStyleClave(int styleIndex) const {
        switch (styleIndex) {
            case 1:  // Afro-Cuban
                return ClaveType::SON_3_2;
            case 2:  // Brazilian
                return ClaveType::BRAZILIAN_3_2;
            case 0:  // West African
                return ClaveType::AFRO_6_8;
            default:
                return ClaveType::SON_3_2;
        }
    }

    // ========================================
    // Flip clave direction (3-2 <-> 2-3)
    // ========================================
    void flipClaveDirection() {
        switch (currentClave) {
            case ClaveType::SON_3_2:
                currentClave = ClaveType::SON_2_3;
                break;
            case ClaveType::SON_2_3:
                currentClave = ClaveType::SON_3_2;
                break;
            case ClaveType::RUMBA_3_2:
                currentClave = ClaveType::RUMBA_2_3;
                break;
            case ClaveType::RUMBA_2_3:
                currentClave = ClaveType::RUMBA_3_2;
                break;
            default:
                // No flip for non-directional claves
                break;
        }
    }

    // ========================================
    // Get clave name
    // ========================================
    const char* getClaveName(ClaveType type) const {
        return claves[static_cast<int>(type)].name;
    }

    const char* getCurrentClaveName() const {
        return getClaveName(currentClave);
    }
};

} // namespace WorldRhythm
