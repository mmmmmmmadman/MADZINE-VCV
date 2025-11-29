#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Jazz Brush/Stick Engine
// ========================================
// Based on fills_ornaments_research.md Section 2.5
//
// Jazz drumming techniques:
// - Brush patterns (ballads, medium swing)
// - Stick techniques (bebop, up-tempo)
// - Comping vocabulary
// - "Dropping bombs" (bass drum accents)

// ========================================
// Technique Types
// ========================================

enum class JazzTechnique {
    BRUSHES_BALLAD = 0,    // Slow brush circles
    BRUSHES_MEDIUM,        // Medium swing brushes
    BRUSHES_FAST,          // Fast brush sweeps
    STICKS_SWING,          // Standard stick swing
    STICKS_BEBOP,          // Fast bebop comping
    STICKS_BOMBS,          // Aggressive accent style
    STICKS_BLAKEY,         // Art Blakey style
    STICKS_ROACH,          // Max Roach melodic style
    NUM_TECHNIQUES
};

struct TechniqueDefinition {
    JazzTechnique technique;
    std::string name;
    std::string description;
    float typicalBPMMin;
    float typicalBPMMax;
    float swingAmount;
    float rideIntensity;     // How prominent the ride is
    float snareActivity;     // Comping density
    float kickActivity;      // Bomb frequency
    bool usesBrushes;
};

inline TechniqueDefinition getTechniqueDef(JazzTechnique t) {
    switch (t) {
        case JazzTechnique::BRUSHES_BALLAD:
            return {t, "Brushes (Ballad)", "Slow brush circles and sweeps",
                    40.0f, 80.0f, 0.62f, 0.6f, 0.2f, 0.1f, true};
        case JazzTechnique::BRUSHES_MEDIUM:
            return {t, "Brushes (Medium)", "Medium swing brush pattern",
                    80.0f, 140.0f, 0.60f, 0.7f, 0.35f, 0.15f, true};
        case JazzTechnique::BRUSHES_FAST:
            return {t, "Brushes (Fast)", "Fast brush sweeps",
                    140.0f, 200.0f, 0.55f, 0.8f, 0.4f, 0.2f, true};
        case JazzTechnique::STICKS_SWING:
            return {t, "Sticks (Swing)", "Standard swing ride pattern",
                    100.0f, 180.0f, 0.60f, 0.9f, 0.4f, 0.25f, false};
        case JazzTechnique::STICKS_BEBOP:
            return {t, "Sticks (Bebop)", "Fast bebop comping",
                    180.0f, 300.0f, 0.53f, 1.0f, 0.5f, 0.35f, false};
        case JazzTechnique::STICKS_BOMBS:
            return {t, "Sticks (Bombs)", "Aggressive accent style",
                    120.0f, 220.0f, 0.58f, 0.85f, 0.45f, 0.6f, false};
        case JazzTechnique::STICKS_BLAKEY:
            return {t, "Art Blakey Style", "Driving, aggressive, rolling",
                    140.0f, 240.0f, 0.58f, 0.95f, 0.55f, 0.5f, false};
        case JazzTechnique::STICKS_ROACH:
            return {t, "Max Roach Style", "Melodic, supportive, musical",
                    100.0f, 200.0f, 0.60f, 0.85f, 0.4f, 0.3f, false};
        default:
            return {JazzTechnique::STICKS_SWING, "Sticks (Swing)", "",
                    100.0f, 180.0f, 0.60f, 0.9f, 0.4f, 0.25f, false};
    }
}

// ========================================
// Ride Pattern Types
// ========================================

enum class RidePattern {
    STANDARD_SWING = 0,    // Ding-ding-a-ding
    STRAIGHT_8THS,         // Straight 8th notes
    FOUR_ON_FLOOR,         // Quarter notes
    BEBOP_RIDE,            // Continuous 8ths with accent
    BROKEN,                // Interrupted pattern
    NUM_PATTERNS
};

// ========================================
// Comping Event
// ========================================

struct CompEvent {
    int position;
    float velocity;
    bool isAccent;
    bool isBomb;           // Bass drum "bomb"
    bool isRimshot;
    bool isCrossStick;
};

// ========================================
// Jazz Pattern Result
// ========================================

struct JazzPatternResult {
    Pattern ride;          // Ride cymbal pattern
    Pattern hihat;         // Hi-hat (foot) pattern
    Pattern snare;         // Snare comping
    Pattern kick;          // Bass drum bombs
    Pattern combined;      // All combined

    std::vector<CompEvent> compEvents;  // Detailed comping info
    float effectiveSwing;
    JazzTechnique technique;
};

// ========================================
// Jazz Brush/Stick Engine
// ========================================

class JazzBrushEngine {
public:
    JazzBrushEngine() : currentTechnique(JazzTechnique::STICKS_SWING),
                         gen(std::random_device{}()) {}

    // ========================================
    // Technique Selection
    // ========================================

    void setTechnique(JazzTechnique tech) {
        currentTechnique = tech;
    }

    void setTechniqueByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(JazzTechnique::NUM_TECHNIQUES)) {
            currentTechnique = static_cast<JazzTechnique>(index);
        }
    }

    JazzTechnique getTechnique() const { return currentTechnique; }

    const TechniqueDefinition& getCurrentDef() const {
        static TechniqueDefinition def = getTechniqueDef(currentTechnique);
        def = getTechniqueDef(currentTechnique);
        return def;
    }

    std::string getCurrentName() const {
        return getTechniqueDef(currentTechnique).name;
    }

    bool usesBrushes() const {
        return getTechniqueDef(currentTechnique).usesBrushes;
    }

    // ========================================
    // Ride Cymbal Patterns
    // ========================================

    Pattern generateRidePattern(int length, float velocity, RidePattern type = RidePattern::STANDARD_SWING) {
        Pattern p(length);
        const auto& def = getCurrentDef();

        switch (type) {
            case RidePattern::STANDARD_SWING:
                return generateStandardSwingRide(length, velocity * def.rideIntensity);
            case RidePattern::STRAIGHT_8THS:
                return generateStraight8thRide(length, velocity * def.rideIntensity);
            case RidePattern::FOUR_ON_FLOOR:
                return generateQuarterNoteRide(length, velocity * def.rideIntensity);
            case RidePattern::BEBOP_RIDE:
                return generateBebopRide(length, velocity * def.rideIntensity);
            case RidePattern::BROKEN:
                return generateBrokenRide(length, velocity * def.rideIntensity);
            default:
                return generateStandardSwingRide(length, velocity * def.rideIntensity);
        }
    }

    // Standard swing: "ding-ding-a-ding" (1, 2&, 3, 4&)
    Pattern generateStandardSwingRide(int length, float velocity) {
        Pattern p(length);

        // In 16-step: positions 0, 5, 8, 13 (with swing feel)
        // Or thinking in 8ths: 1, 2&, 3, 4&
        std::vector<int> ridePositions = {0, 5, 8, 13};  // Swing positions

        for (int pos : ridePositions) {
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                bool isDownbeat = (pos == 0 || pos == 8);
                float vel = isDownbeat ? velocity : velocity * 0.85f;
                p.setOnset(mappedPos, vel);
                p.accents[mappedPos] = isDownbeat;
            }
        }

        return p;
    }

    // Straight 8th notes (latin jazz, early jazz)
    Pattern generateStraight8thRide(int length, float velocity) {
        Pattern p(length);

        for (int i = 0; i < length; i++) {
            int pos16 = (i * 16) / length;
            if (pos16 % 2 == 0) {  // Every 8th note
                bool isDownbeat = (pos16 % 4 == 0);
                float vel = isDownbeat ? velocity : velocity * 0.7f;
                p.setOnset(i, vel);
                p.accents[i] = isDownbeat;
            }
        }

        return p;
    }

    // Quarter note ride (slower tempos, brushes)
    Pattern generateQuarterNoteRide(int length, float velocity) {
        Pattern p(length);

        for (int i = 0; i < length; i++) {
            int pos16 = (i * 16) / length;
            if (pos16 % 4 == 0) {  // Quarter notes
                p.setOnset(i, velocity);
                p.accents[i] = (pos16 == 0 || pos16 == 8);
            }
        }

        return p;
    }

    // Bebop ride: continuous 8ths with accent pattern
    Pattern generateBebopRide(int length, float velocity) {
        Pattern p(length);

        for (int i = 0; i < length; i++) {
            int pos16 = (i * 16) / length;
            if (pos16 % 2 == 0) {  // 8th notes
                // Bebop accent pattern: heavy on 1, lighter elsewhere
                float vel;
                if (pos16 == 0) vel = velocity;
                else if (pos16 == 8) vel = velocity * 0.9f;
                else if (pos16 % 4 == 2) vel = velocity * 0.7f;  // &s
                else vel = velocity * 0.8f;

                p.setOnset(i, vel);
                p.accents[i] = (pos16 == 0);
            }
        }

        return p;
    }

    // Broken ride: intentionally sparse/interrupted
    Pattern generateBrokenRide(int length, float velocity) {
        Pattern p(length);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Start with standard swing, then remove some hits
        std::vector<int> basePositions = {0, 5, 8, 13};

        for (int pos : basePositions) {
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                // 70% chance to keep each hit (except beat 1)
                if (pos == 0 || dist(gen) < 0.7f) {
                    p.setOnset(mappedPos, velocity * 0.85f);
                }
            }
        }

        return p;
    }

    // ========================================
    // Hi-Hat (Foot) Pattern
    // ========================================

    Pattern generateHihatFoot(int length, float velocity) {
        Pattern p(length);

        // Hi-hat foot on 2 and 4
        for (int i = 0; i < length; i++) {
            int pos16 = (i * 16) / length;
            if (pos16 == 4 || pos16 == 12) {  // Beats 2 and 4
                p.setOnset(i, velocity * 0.8f);
                p.accents[i] = true;
            }
        }

        return p;
    }

    // ========================================
    // Snare Comping
    // ========================================

    Pattern generateSnareComping(int length, float velocity, float density = 0.4f) {
        Pattern p(length);
        const auto& def = getCurrentDef();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Comping positions favor offbeats (& of 2, & of 4)
        std::vector<float> compWeights = {
            0.1f, 0.2f, 0.3f, 0.4f,   // Beat 1 area (sparse)
            0.3f, 0.7f, 0.5f, 0.3f,   // Beat 2 area (& of 2 is prime)
            0.1f, 0.2f, 0.3f, 0.4f,   // Beat 3 area
            0.3f, 0.8f, 0.6f, 0.3f    // Beat 4 area (& of 4 is prime)
        };

        float actualDensity = density * def.snareActivity;

        for (int i = 0; i < length; i++) {
            int weightIdx = (i * 16) / length;
            float prob = compWeights[weightIdx % 16] * actualDensity;

            if (dist(gen) < prob) {
                // Vary velocity for ghost notes vs accents
                float vel = velocity * (0.4f + dist(gen) * 0.5f);
                bool isAccent = dist(gen) < 0.3f;
                if (isAccent) vel = velocity * 0.9f;

                p.setOnset(i, vel);
                p.accents[i] = isAccent;
            }
        }

        return p;
    }

    // ========================================
    // Bass Drum "Bombs"
    // ========================================

    Pattern generateKickBombs(int length, float velocity, float density = 0.25f) {
        Pattern p(length);
        const auto& def = getCurrentDef();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Kenny Clarke style: unpredictable accents
        // Favor syncopated positions
        std::vector<float> bombWeights = {
            0.3f, 0.4f, 0.5f, 0.6f,   // Beat 1 area
            0.2f, 0.5f, 0.7f, 0.4f,   // Beat 2 area
            0.3f, 0.4f, 0.5f, 0.7f,   // Beat 3 area
            0.2f, 0.6f, 0.8f, 0.5f    // Beat 4 area (strong on & of 4)
        };

        float actualDensity = density * def.kickActivity;

        for (int i = 0; i < length; i++) {
            int weightIdx = (i * 16) / length;
            float prob = bombWeights[weightIdx % 16] * actualDensity;

            if (dist(gen) < prob) {
                p.setOnset(i, velocity * 0.85f);
                p.accents[i] = true;  // Bombs are always accented
            }
        }

        return p;
    }

    // ========================================
    // Brush-Specific Patterns
    // ========================================

    // Brush sweep pattern (circular motion simulation)
    Pattern generateBrushSweep(int length, float velocity) {
        Pattern p(length);
        const auto& def = getCurrentDef();

        if (!def.usesBrushes) {
            // Fall back to ride
            return generateRidePattern(length, velocity);
        }

        // Brush sweeps: continuous gentle sound with accents
        for (int i = 0; i < length; i++) {
            int pos16 = (i * 16) / length;

            // Continuous low-level sound
            float baseVel = velocity * 0.3f;

            // Accent on beats 2 and 4 (sweep hits head)
            if (pos16 == 4 || pos16 == 12) {
                baseVel = velocity * 0.8f;
                p.accents[i] = true;
            }
            // Lighter accent on 1 and 3
            else if (pos16 == 0 || pos16 == 8) {
                baseVel = velocity * 0.5f;
            }

            p.setOnset(i, baseVel);
        }

        return p;
    }

    // ========================================
    // Complete Pattern Generation
    // ========================================

    JazzPatternResult generateComplete(int length, float velocity, float compDensity = 0.4f) {
        JazzPatternResult result;
        result.technique = currentTechnique;
        result.effectiveSwing = getCurrentDef().swingAmount;

        if (usesBrushes()) {
            result.ride = generateBrushSweep(length, velocity);
        } else {
            result.ride = generateRidePattern(length, velocity);
        }

        result.hihat = generateHihatFoot(length, velocity);
        result.snare = generateSnareComping(length, velocity, compDensity);
        result.kick = generateKickBombs(length, velocity, compDensity * 0.6f);

        // Combined
        result.combined = combinePatterns({result.ride, result.hihat, result.snare, result.kick});

        // Generate comp events
        for (int i = 0; i < length; i++) {
            if (result.snare.hasOnsetAt(i) || result.kick.hasOnsetAt(i)) {
                CompEvent e;
                e.position = i;
                e.velocity = std::max(
                    result.snare.hasOnsetAt(i) ? result.snare.getVelocity(i) : 0.0f,
                    result.kick.hasOnsetAt(i) ? result.kick.getVelocity(i) : 0.0f
                );
                e.isAccent = result.snare.accents[i] || result.kick.accents[i];
                e.isBomb = result.kick.hasOnsetAt(i);
                e.isRimshot = false;
                e.isCrossStick = false;
                result.compEvents.push_back(e);
            }
        }

        return result;
    }

    // ========================================
    // Style-Specific Patterns
    // ========================================

    // Art Blakey style: driving, press rolls, tom accents
    JazzPatternResult generateBlakeyStyle(int length, float velocity) {
        setTechnique(JazzTechnique::STICKS_BLAKEY);
        JazzPatternResult result = generateComplete(length, velocity, 0.55f);

        // Add characteristic Blakey press roll buildup at end of phrase
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        int rollStart = length - (length / 4);
        for (int i = rollStart; i < length; i++) {
            if (!result.snare.hasOnsetAt(i) && dist(gen) < 0.6f) {
                float v = velocity * (0.3f + 0.4f * static_cast<float>(i - rollStart) / (length - rollStart));
                result.snare.setOnset(i, v);
            }
        }

        return result;
    }

    // Max Roach style: melodic, musical phrasing
    JazzPatternResult generateRoachStyle(int length, float velocity) {
        setTechnique(JazzTechnique::STICKS_ROACH);
        JazzPatternResult result = generateComplete(length, velocity, 0.35f);

        // Roach: fewer notes but more purposeful
        // Clear the snare and add melodic figures
        result.snare = Pattern(length);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Short melodic phrases
        int phraseStart = length / 4;
        std::vector<int> melodicPositions = {phraseStart, phraseStart + 2, phraseStart + 3};
        for (int pos : melodicPositions) {
            if (pos < length) {
                result.snare.setOnset(pos, velocity * (0.6f + dist(gen) * 0.3f));
            }
        }

        return result;
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getTechniqueName(JazzTechnique tech) {
        return getTechniqueDef(tech).name;
    }

    int getNumTechniques() const {
        return static_cast<int>(JazzTechnique::NUM_TECHNIQUES);
    }

    float getSwingForCurrentTechnique() const {
        return getCurrentDef().swingAmount;
    }

private:
    JazzTechnique currentTechnique;
    std::mt19937 gen;

    Pattern combinePatterns(const std::vector<Pattern>& patterns) {
        if (patterns.empty()) return Pattern(16);

        int length = patterns[0].length;
        Pattern combined(length);

        for (int i = 0; i < length; i++) {
            float maxVel = 0.0f;
            bool hasAccent = false;

            for (const auto& p : patterns) {
                if (i < p.length && p.hasOnsetAt(i)) {
                    maxVel = std::max(maxVel, p.getVelocity(i));
                    hasAccent = hasAccent || p.accents[i];
                }
            }

            if (maxVel > 0) {
                combined.setOnset(i, maxVel);
                combined.accents[i] = hasAccent;
            }
        }

        return combined;
    }
};

} // namespace WorldRhythm
