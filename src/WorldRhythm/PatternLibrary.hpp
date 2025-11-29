#pragma once

#include <vector>
#include <string>
#include <random>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Pattern Library
// ========================================
// Collection of fundamental rhythm patterns from world music traditions
// Based on fills_ornaments_research.md and unified_rhythm_analysis.md

enum class PatternType {
    // Latin/Afro-Cuban fundamentals
    TRESILLO,           // 3+3+2: X..X..X.
    HABANERA,           // Tresillo with extra beat: X..X.XX.
    CINQUILLO,          // 5-note pattern: X.XX.XX.
    MONTUNO,            // Piano comping pattern
    CASCARA,            // Shell pattern on timbale

    // West African fundamentals
    STANDARD_BELL,      // 7-stroke: X.X.XX.X.X.X (12 pulses)
    AGBEKOR_BELL,       // Ewe pattern
    KPANLOGO,           // Ga pattern

    // Brazilian fundamentals
    PARTIDO_ALTO,       // Samba partido alto
    TAMBORIM,           // Tamborim pattern
    SURDO_PRIMEIRA,     // First surdo (beat 2)
    SURDO_SEGUNDA,      // Second surdo (beat 1)

    // Electronic fundamentals
    FOUR_ON_FLOOR,      // X...X...X...X...
    OFFBEAT_HAT,        // .X.X.X.X.X.X.X.X
    BREAKBEAT_KICK,     // Syncopated kick
    AMEN_BREAK,         // Classic break pattern

    // Jazz fundamentals
    RIDE_PATTERN,       // Swing ride
    HI_HAT_2_4,         // Hi-hat on 2 and 4

    NUM_PATTERN_TYPES
};

struct PatternDefinition {
    PatternType type;
    std::string name;
    std::string tradition;
    int length;
    std::vector<int> positions;
    std::vector<float> velocities;
    float swingAmount;          // 0.5 = straight, 0.67 = triplet
};

// ========================================
// Latin/Afro-Cuban Patterns
// ========================================

// Tresillo: 3+3+2 grouping (the DNA of Afro-Cuban music)
inline PatternDefinition createTresillo() {
    PatternDefinition p;
    p.type = PatternType::TRESILLO;
    p.name = "Tresillo";
    p.tradition = "Afro-Cuban";
    p.length = 8;
    p.positions = {0, 3, 6};
    p.velocities = {1.0f, 0.85f, 0.9f};
    p.swingAmount = 0.55f;
    return p;
}

// Habanera: Tresillo with added note
inline PatternDefinition createHabanera() {
    PatternDefinition p;
    p.type = PatternType::HABANERA;
    p.name = "Habanera";
    p.tradition = "Cuban";
    p.length = 8;
    p.positions = {0, 3, 4, 6};
    p.velocities = {1.0f, 0.8f, 0.7f, 0.85f};
    p.swingAmount = 0.55f;
    return p;
}

// Cinquillo: 5-note pattern
inline PatternDefinition createCinquillo() {
    PatternDefinition p;
    p.type = PatternType::CINQUILLO;
    p.name = "Cinquillo";
    p.tradition = "Afro-Cuban";
    p.length = 8;
    p.positions = {0, 2, 3, 5, 6};
    p.velocities = {1.0f, 0.75f, 0.85f, 0.75f, 0.8f};
    p.swingAmount = 0.55f;
    return p;
}

// Montuno: Piano comping pattern
inline PatternDefinition createMontuno() {
    PatternDefinition p;
    p.type = PatternType::MONTUNO;
    p.name = "Montuno";
    p.tradition = "Afro-Cuban";
    p.length = 16;
    p.positions = {0, 3, 4, 6, 8, 10, 12, 14};
    p.velocities = {1.0f, 0.7f, 0.8f, 0.75f, 0.85f, 0.7f, 0.8f, 0.7f};
    p.swingAmount = 0.58f;
    return p;
}

// Cascara: Timbale shell pattern
inline PatternDefinition createCascara() {
    PatternDefinition p;
    p.type = PatternType::CASCARA;
    p.name = "Cascara";
    p.tradition = "Afro-Cuban";
    p.length = 16;
    p.positions = {0, 3, 4, 6, 8, 10, 11, 12, 14};
    p.velocities = {1.0f, 0.75f, 0.85f, 0.7f, 0.9f, 0.7f, 0.8f, 0.85f, 0.7f};
    p.swingAmount = 0.55f;
    return p;
}

// ========================================
// West African Patterns
// ========================================

// Standard Bell (Gankogui): 7-stroke pattern mapped to 16
inline PatternDefinition createStandardBell() {
    PatternDefinition p;
    p.type = PatternType::STANDARD_BELL;
    p.name = "Standard Bell";
    p.tradition = "West African";
    p.length = 16;
    // Original 12-pulse: 1,3,5,6,8,10,12 mapped to 16
    p.positions = {0, 2, 5, 6, 8, 11, 13};
    p.velocities = {1.0f, 0.7f, 0.8f, 0.95f, 0.75f, 0.85f, 0.9f};
    p.swingAmount = 0.62f;
    return p;
}

// Agbekor Bell: Ewe rhythm
inline PatternDefinition createAgbekorBell() {
    PatternDefinition p;
    p.type = PatternType::AGBEKOR_BELL;
    p.name = "Agbekor Bell";
    p.tradition = "West African (Ewe)";
    p.length = 16;
    p.positions = {0, 2, 4, 6, 8, 10, 12, 14};
    p.velocities = {1.0f, 0.6f, 0.8f, 0.6f, 0.85f, 0.6f, 0.75f, 0.6f};
    p.swingAmount = 0.60f;
    return p;
}

// Kpanlogo: Ga tradition
inline PatternDefinition createKpanlogo() {
    PatternDefinition p;
    p.type = PatternType::KPANLOGO;
    p.name = "Kpanlogo";
    p.tradition = "West African (Ga)";
    p.length = 16;
    p.positions = {0, 3, 6, 8, 10, 13};
    p.velocities = {1.0f, 0.75f, 0.85f, 0.7f, 0.8f, 0.75f};
    p.swingAmount = 0.58f;
    return p;
}

// ========================================
// Brazilian Patterns
// ========================================

// Partido Alto: Samba partido alto clave
inline PatternDefinition createPartidoAlto() {
    PatternDefinition p;
    p.type = PatternType::PARTIDO_ALTO;
    p.name = "Partido Alto";
    p.tradition = "Brazilian";
    p.length = 16;
    p.positions = {0, 3, 6, 7, 10, 12};
    p.velocities = {1.0f, 0.7f, 0.85f, 0.75f, 0.8f, 0.85f};
    p.swingAmount = 0.57f;
    return p;
}

// Tamborim: Samba tamborim pattern
inline PatternDefinition createTamborim() {
    PatternDefinition p;
    p.type = PatternType::TAMBORIM;
    p.name = "Tamborim";
    p.tradition = "Brazilian";
    p.length = 16;
    p.positions = {0, 2, 3, 4, 6, 8, 10, 11, 12, 14};
    p.velocities = {1.0f, 0.6f, 0.7f, 0.85f, 0.75f, 0.9f, 0.6f, 0.7f, 0.8f, 0.65f};
    p.swingAmount = 0.57f;
    return p;
}

// Surdo Primeira: First surdo on beat 2
inline PatternDefinition createSurdoPrimeira() {
    PatternDefinition p;
    p.type = PatternType::SURDO_PRIMEIRA;
    p.name = "Surdo Primeira";
    p.tradition = "Brazilian";
    p.length = 16;
    p.positions = {4, 12};  // Beat 2 of each half
    p.velocities = {1.0f, 0.95f};
    p.swingAmount = 0.50f;
    return p;
}

// Surdo Segunda: Second surdo on beat 1
inline PatternDefinition createSurdoSegunda() {
    PatternDefinition p;
    p.type = PatternType::SURDO_SEGUNDA;
    p.name = "Surdo Segunda";
    p.tradition = "Brazilian";
    p.length = 16;
    p.positions = {0, 8};  // Beat 1 of each half
    p.velocities = {1.0f, 0.9f};
    p.swingAmount = 0.50f;
    return p;
}

// ========================================
// Electronic Patterns
// ========================================

// Four on the floor: House/Techno kick
inline PatternDefinition createFourOnFloor() {
    PatternDefinition p;
    p.type = PatternType::FOUR_ON_FLOOR;
    p.name = "Four on Floor";
    p.tradition = "Electronic";
    p.length = 16;
    p.positions = {0, 4, 8, 12};
    p.velocities = {1.0f, 0.95f, 0.97f, 0.93f};
    p.swingAmount = 0.50f;
    return p;
}

// Offbeat hi-hat: Classic house hat
inline PatternDefinition createOffbeatHat() {
    PatternDefinition p;
    p.type = PatternType::OFFBEAT_HAT;
    p.name = "Offbeat Hi-Hat";
    p.tradition = "Electronic";
    p.length = 16;
    p.positions = {2, 6, 10, 14};
    p.velocities = {0.85f, 0.8f, 0.85f, 0.8f};
    p.swingAmount = 0.50f;
    return p;
}

// Breakbeat kick: Syncopated
inline PatternDefinition createBreakbeatKick() {
    PatternDefinition p;
    p.type = PatternType::BREAKBEAT_KICK;
    p.name = "Breakbeat Kick";
    p.tradition = "Electronic";
    p.length = 16;
    p.positions = {0, 6, 8, 14};
    p.velocities = {1.0f, 0.85f, 0.9f, 0.8f};
    p.swingAmount = 0.52f;
    return p;
}

// Amen Break: The classic
inline PatternDefinition createAmenBreak() {
    PatternDefinition p;
    p.type = PatternType::AMEN_BREAK;
    p.name = "Amen Break";
    p.tradition = "Breakbeat";
    p.length = 16;
    // Simplified Amen: kick on 1, 1a, 3&; snare on 2, 4
    p.positions = {0, 2, 4, 7, 8, 12, 14};
    p.velocities = {1.0f, 0.7f, 0.95f, 0.75f, 0.85f, 0.9f, 0.7f};
    p.swingAmount = 0.52f;
    return p;
}

// ========================================
// Jazz Patterns
// ========================================

// Ride pattern: Basic swing ride
inline PatternDefinition createRidePattern() {
    PatternDefinition p;
    p.type = PatternType::RIDE_PATTERN;
    p.name = "Swing Ride";
    p.tradition = "Jazz";
    p.length = 16;
    p.positions = {0, 3, 4, 7, 8, 11, 12, 15};
    p.velocities = {1.0f, 0.7f, 0.9f, 0.7f, 0.95f, 0.7f, 0.9f, 0.7f};
    p.swingAmount = 0.65f;
    return p;
}

// Hi-hat on 2 and 4
inline PatternDefinition createHiHat_2_4() {
    PatternDefinition p;
    p.type = PatternType::HI_HAT_2_4;
    p.name = "Hi-Hat 2 & 4";
    p.tradition = "Jazz";
    p.length = 16;
    p.positions = {4, 12};
    p.velocities = {0.9f, 0.85f};
    p.swingAmount = 0.65f;
    return p;
}

// ========================================
// Pattern Library Class
// ========================================
class PatternLibrary {
private:
    std::mt19937 rng;
    std::vector<PatternDefinition> patterns;

public:
    PatternLibrary() : rng(std::random_device{}()) {
        // Latin/Afro-Cuban
        patterns.push_back(createTresillo());
        patterns.push_back(createHabanera());
        patterns.push_back(createCinquillo());
        patterns.push_back(createMontuno());
        patterns.push_back(createCascara());

        // West African
        patterns.push_back(createStandardBell());
        patterns.push_back(createAgbekorBell());
        patterns.push_back(createKpanlogo());

        // Brazilian
        patterns.push_back(createPartidoAlto());
        patterns.push_back(createTamborim());
        patterns.push_back(createSurdoPrimeira());
        patterns.push_back(createSurdoSegunda());

        // Electronic
        patterns.push_back(createFourOnFloor());
        patterns.push_back(createOffbeatHat());
        patterns.push_back(createBreakbeatKick());
        patterns.push_back(createAmenBreak());

        // Jazz
        patterns.push_back(createRidePattern());
        patterns.push_back(createHiHat_2_4());
    }

    void seed(unsigned int s) { rng.seed(s); }

    int getNumPatterns() const { return static_cast<int>(patterns.size()); }

    const PatternDefinition& getPattern(PatternType type) const {
        for (const auto& p : patterns) {
            if (p.type == type) return p;
        }
        return patterns[0];
    }

    const PatternDefinition& getPatternByIndex(int index) const {
        if (index >= 0 && index < static_cast<int>(patterns.size())) {
            return patterns[index];
        }
        return patterns[0];
    }

    // ========================================
    // Generate pattern from definition
    // ========================================
    Pattern generatePattern(PatternType type, int targetLength, float intensity) {
        const PatternDefinition& def = getPattern(type);
        Pattern p(targetLength);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (size_t i = 0; i < def.positions.size(); i++) {
            // Map definition position to target length
            int step = (def.positions[i] * targetLength) / def.length;
            if (step >= targetLength) step = targetLength - 1;

            float vel = def.velocities[i] * intensity + velVar(rng);
            p.setOnset(step, std::clamp(vel, 0.3f, 1.0f));

            if (def.velocities[i] > 0.85f) {
                p.accents[step] = true;
            }
        }

        return p;
    }

    // ========================================
    // Get patterns by tradition
    // ========================================
    std::vector<PatternType> getPatternsByTradition(const std::string& tradition) const {
        std::vector<PatternType> result;
        for (const auto& p : patterns) {
            if (p.tradition.find(tradition) != std::string::npos) {
                result.push_back(p.type);
            }
        }
        return result;
    }

    // ========================================
    // Get recommended patterns for style
    // ========================================
    std::vector<PatternType> getStylePatterns(int styleIndex) const {
        std::vector<PatternType> result;

        switch (styleIndex) {
            case 0:  // West African
                result = {PatternType::STANDARD_BELL, PatternType::AGBEKOR_BELL, PatternType::KPANLOGO};
                break;
            case 1:  // Afro-Cuban
                result = {PatternType::TRESILLO, PatternType::CINQUILLO, PatternType::CASCARA, PatternType::MONTUNO};
                break;
            case 2:  // Brazilian
                result = {PatternType::PARTIDO_ALTO, PatternType::TAMBORIM, PatternType::SURDO_PRIMEIRA, PatternType::SURDO_SEGUNDA};
                break;
            case 6:  // Jazz
                result = {PatternType::RIDE_PATTERN, PatternType::HI_HAT_2_4};
                break;
            case 7:  // Electronic
            case 9:  // Techno
                result = {PatternType::FOUR_ON_FLOOR, PatternType::OFFBEAT_HAT};
                break;
            case 8:  // Breakbeat
                result = {PatternType::AMEN_BREAK, PatternType::BREAKBEAT_KICK};
                break;
            default:
                result = {PatternType::TRESILLO};
        }

        return result;
    }

    // ========================================
    // Combine two patterns (layering)
    // ========================================
    Pattern combinePatterns(PatternType type1, PatternType type2,
                           int targetLength, float intensity,
                           float blend) {
        Pattern p1 = generatePattern(type1, targetLength, intensity);
        Pattern p2 = generatePattern(type2, targetLength, intensity * blend);

        // Merge patterns
        for (int i = 0; i < targetLength; i++) {
            if (p2.hasOnsetAt(i)) {
                if (p1.hasOnsetAt(i)) {
                    // Both have onset: use maximum velocity
                    float vel = std::max(p1.getVelocity(i), p2.getVelocity(i));
                    p1.setOnset(i, vel);
                } else {
                    p1.setOnset(i, p2.getVelocity(i));
                }
                p1.accents[i] = p1.accents[i] || p2.accents[i];
            }
        }

        return p1;
    }

    // ========================================
    // Rotate pattern
    // ========================================
    Pattern rotatePattern(PatternType type, int targetLength,
                         float intensity, int rotation) {
        Pattern p = generatePattern(type, targetLength, intensity);

        // Create rotated pattern
        Pattern rotated(targetLength);
        for (int i = 0; i < targetLength; i++) {
            int srcPos = (i - rotation + targetLength) % targetLength;
            if (p.hasOnsetAt(srcPos)) {
                rotated.setOnset(i, p.getVelocity(srcPos));
                rotated.accents[i] = p.accents[srcPos];
            }
        }

        return rotated;
    }

    // ========================================
    // Get pattern swing amount
    // ========================================
    float getPatternSwing(PatternType type) const {
        return getPattern(type).swingAmount;
    }
};

} // namespace WorldRhythm
