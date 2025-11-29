#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Metric Modulation Engine
// ========================================
// Based on fills_ornaments_research.md Section 2.6 and unified_rhythm_analysis.md
//
// Metric Modulation: Changing the perceived tempo/feel without changing actual BPM
//
// Types:
// - Half-time: Pattern feels half as fast (DnB drop, trap breakdown)
// - Double-time: Pattern feels twice as fast (bebop, EDM buildup)
// - Irama shifts: Gamelan-style density changes
// - Polymetric modulation: Shift between different groupings

// ========================================
// Modulation Types
// ========================================

enum class ModulationType {
    HALF_TIME = 0,       // Feel half as fast
    DOUBLE_TIME,         // Feel twice as fast
    TRIPLET_FEEL,        // Shift to triplet subdivision
    STRAIGHT_FEEL,       // Shift from triplet to straight
    HALF_TIME_SHUFFLE,   // Half-time with swing
    DOUBLE_TIME_SWING,   // Double-time jazz feel
    IRAMA_UP,            // Gamelan: increase density level
    IRAMA_DOWN,          // Gamelan: decrease density level
    NUM_TYPES
};

struct ModulationDefinition {
    ModulationType type;
    std::string name;
    std::string description;
    float tempoMultiplier;     // Perceived tempo change
    float densityMultiplier;   // Pattern density change
    float swingAdjustment;     // Swing feel adjustment
};

inline ModulationDefinition getModulationDef(ModulationType type) {
    switch (type) {
        case ModulationType::HALF_TIME:
            return {type, "Half-Time", "Feel half as fast", 0.5f, 0.5f, 0.0f};
        case ModulationType::DOUBLE_TIME:
            return {type, "Double-Time", "Feel twice as fast", 2.0f, 1.5f, 0.0f};
        case ModulationType::TRIPLET_FEEL:
            return {type, "Triplet Feel", "Shift to triplet subdivision", 1.0f, 1.0f, 0.67f};
        case ModulationType::STRAIGHT_FEEL:
            return {type, "Straight Feel", "Remove swing, straight 8ths/16ths", 1.0f, 1.0f, 0.5f};
        case ModulationType::HALF_TIME_SHUFFLE:
            return {type, "Half-Time Shuffle", "Half-time with heavy swing", 0.5f, 0.6f, 0.65f};
        case ModulationType::DOUBLE_TIME_SWING:
            return {type, "Double-Time Swing", "Fast bebop feel", 2.0f, 1.2f, 0.55f};
        case ModulationType::IRAMA_UP:
            return {type, "Irama Up", "Increase density (Gamelan)", 1.0f, 2.0f, 0.0f};
        case ModulationType::IRAMA_DOWN:
            return {type, "Irama Down", "Decrease density (Gamelan)", 1.0f, 0.5f, 0.0f};
        default:
            return {ModulationType::HALF_TIME, "Half-Time", "", 0.5f, 0.5f, 0.0f};
    }
}

// ========================================
// Transition Type
// ========================================

enum class TransitionType {
    INSTANT = 0,         // Immediate change
    GRADUAL_1_BAR,       // Transition over 1 bar
    GRADUAL_2_BAR,       // Transition over 2 bars
    GRADUAL_4_BAR,       // Transition over 4 bars
    FILL_TRIGGERED,      // Change after fill
    NUM_TYPES
};

// ========================================
// Modulation Result
// ========================================

struct ModulatedPattern {
    Pattern pattern;
    float effectiveSwing;      // Resulting swing amount
    float perceivedTempo;      // Multiplier for perceived tempo
    int originalLength;
    int modulatedLength;
    ModulationType appliedModulation;
};

// ========================================
// Metric Modulation Engine
// ========================================

class MetricModulationEngine {
public:
    MetricModulationEngine() : gen(std::random_device{}()) {}

    // ========================================
    // Core Modulation Functions
    // ========================================

    // Apply half-time feel
    ModulatedPattern applyHalfTime(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::HALF_TIME;
        result.perceivedTempo = 0.5f;
        result.effectiveSwing = 0.5f;

        Pattern p(input.length);

        // Half-time: only use hits on even positions, spread them out
        for (int i = 0; i < input.length; i++) {
            int sourcePos = (i * 2) % input.length;
            if (input.hasOnsetAt(sourcePos)) {
                // Only keep strong beat hits
                if (i % 2 == 0) {
                    p.setOnset(i, input.getVelocity(sourcePos));
                    p.accents[i] = input.accents[sourcePos];
                }
            }
        }

        // Ensure snare-type hits are on beat 3 (half-time signature)
        // In 16-step: position 8 is beat 3
        if (input.length >= 16) {
            // Find highest velocity hit and move accent to half-time snare position
            float maxVel = 0.0f;
            for (int i = 4; i < 12; i++) {  // Look in middle section
                if (input.hasOnsetAt(i)) {
                    maxVel = std::max(maxVel, input.getVelocity(i));
                }
            }
            if (maxVel > 0 && !p.hasOnsetAt(8)) {
                p.setOnset(8, maxVel);
                p.accents[8] = true;
            }
        }

        result.pattern = p;
        return result;
    }

    // Apply double-time feel
    ModulatedPattern applyDoubleTime(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::DOUBLE_TIME;
        result.perceivedTempo = 2.0f;
        result.effectiveSwing = 0.52f;  // Slight swing for bebop feel

        Pattern p(input.length);

        // Double-time: compress pattern and repeat
        int halfLength = input.length / 2;

        for (int i = 0; i < input.length; i++) {
            int sourcePos = i % halfLength;
            int mappedSource = sourcePos * 2;

            if (mappedSource < input.length && input.hasOnsetAt(mappedSource)) {
                p.setOnset(i, input.getVelocity(mappedSource) * 0.9f);
                p.accents[i] = input.accents[mappedSource];
            }
        }

        // Add subdivision hits for busier feel
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int i = 1; i < input.length; i += 2) {
            if (!p.hasOnsetAt(i) && dist(gen) < 0.3f) {
                p.setOnset(i, 0.4f);  // Ghost notes
            }
        }

        result.pattern = p;
        return result;
    }

    // Apply triplet feel
    ModulatedPattern applyTripletFeel(const Pattern& input, float swingAmount = 0.67f) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::TRIPLET_FEEL;
        result.perceivedTempo = 1.0f;
        result.effectiveSwing = swingAmount;

        // Triplet feel is mainly about timing, which we represent via swing
        // Pattern structure stays similar but with triplet-friendly positions
        Pattern p(input.length);

        for (int i = 0; i < input.length; i++) {
            if (input.hasOnsetAt(i)) {
                // Quantize to triplet-friendly positions
                int tripletPos = quantizeToTriplet(i, input.length);
                if (!p.hasOnsetAt(tripletPos)) {
                    p.setOnset(tripletPos, input.getVelocity(i));
                    p.accents[tripletPos] = input.accents[i];
                }
            }
        }

        result.pattern = p;
        return result;
    }

    // Apply straight feel (remove swing)
    ModulatedPattern applyStraightFeel(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::STRAIGHT_FEEL;
        result.perceivedTempo = 1.0f;
        result.effectiveSwing = 0.5f;  // Perfectly straight

        // Straighten by quantizing to strict grid
        Pattern p(input.length);

        for (int i = 0; i < input.length; i++) {
            if (input.hasOnsetAt(i)) {
                int straightPos = quantizeToStraight(i, input.length);
                if (!p.hasOnsetAt(straightPos)) {
                    p.setOnset(straightPos, input.getVelocity(i));
                    p.accents[straightPos] = input.accents[i];
                }
            }
        }

        result.pattern = p;
        return result;
    }

    // Apply half-time shuffle (like Purdie shuffle)
    ModulatedPattern applyHalfTimeShuffle(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::HALF_TIME_SHUFFLE;
        result.perceivedTempo = 0.5f;
        result.effectiveSwing = 0.65f;

        Pattern p(input.length);

        // Half-time shuffle: sparse kick/snare with ghost notes
        for (int i = 0; i < input.length; i++) {
            if (input.hasOnsetAt(i)) {
                // Keep strong beats
                if (i % 4 == 0) {
                    p.setOnset(i, input.getVelocity(i));
                    p.accents[i] = true;
                }
                // Add ghost notes on shuffle positions
                else if (i % 2 == 1) {
                    p.setOnset(i, input.getVelocity(i) * 0.35f);  // Quiet ghosts
                }
            }
        }

        // Ensure backbeat
        if (input.length >= 16 && !p.hasOnsetAt(8)) {
            p.setOnset(8, 0.9f);
            p.accents[8] = true;
        }

        result.pattern = p;
        return result;
    }

    // Apply double-time swing (bebop feel)
    ModulatedPattern applyDoubleTimeSwing(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::DOUBLE_TIME_SWING;
        result.perceivedTempo = 2.0f;
        result.effectiveSwing = 0.55f;  // Light swing at fast tempo

        Pattern p = applyDoubleTime(input).pattern;

        // Bebop style: ride pattern emphasis
        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                // Accent on beats 1 and 3
                if ((i * 4 / p.length) % 2 == 0) {
                    p.accents[i] = true;
                    float v = p.getVelocity(i);
                    p.setOnset(i, std::min(1.0f, v * 1.1f));
                }
            }
        }

        result.pattern = p;
        result.effectiveSwing = 0.55f;
        return result;
    }

    // ========================================
    // Irama-style Density Modulation
    // ========================================

    // Increase density (subdivide existing hits)
    ModulatedPattern applyIramaUp(const Pattern& input, int factor = 2) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::IRAMA_UP;
        result.perceivedTempo = 1.0f;
        result.effectiveSwing = 0.5f;

        Pattern p(input.length);

        for (int i = 0; i < input.length; i++) {
            if (input.hasOnsetAt(i)) {
                // Keep original hit
                p.setOnset(i, input.getVelocity(i));
                p.accents[i] = input.accents[i];

                // Add subdivisions
                for (int j = 1; j < factor; j++) {
                    int subPos = i + (j * input.length / (factor * 16));
                    if (subPos < input.length && !p.hasOnsetAt(subPos)) {
                        p.setOnset(subPos, input.getVelocity(i) * 0.6f);
                    }
                }
            }
        }

        result.pattern = p;
        return result;
    }

    // Decrease density (remove subdivisions)
    ModulatedPattern applyIramaDown(const Pattern& input) {
        ModulatedPattern result;
        result.originalLength = input.length;
        result.modulatedLength = input.length;
        result.appliedModulation = ModulationType::IRAMA_DOWN;
        result.perceivedTempo = 1.0f;
        result.effectiveSwing = 0.5f;

        Pattern p(input.length);

        // Only keep hits on strong beats
        for (int i = 0; i < input.length; i++) {
            if (input.hasOnsetAt(i)) {
                // Only keep if on strong position
                int pos16 = (i * 16) / input.length;
                if (pos16 % 4 == 0) {  // Quarter notes only
                    p.setOnset(i, input.getVelocity(i));
                    p.accents[i] = input.accents[i];
                }
            }
        }

        result.pattern = p;
        return result;
    }

    // ========================================
    // Transition Generation
    // ========================================

    // Generate transition pattern between two feels
    std::vector<Pattern> generateTransition(const Pattern& from, const Pattern& to,
                                            TransitionType transType, int /*barsPerPattern*/ = 1) {
        std::vector<Pattern> result;

        int numTransitionPatterns;
        switch (transType) {
            case TransitionType::INSTANT:
                numTransitionPatterns = 0;
                break;
            case TransitionType::GRADUAL_1_BAR:
                numTransitionPatterns = 1;
                break;
            case TransitionType::GRADUAL_2_BAR:
                numTransitionPatterns = 2;
                break;
            case TransitionType::GRADUAL_4_BAR:
                numTransitionPatterns = 4;
                break;
            case TransitionType::FILL_TRIGGERED:
                numTransitionPatterns = 1;
                break;
            default:
                numTransitionPatterns = 0;
        }

        if (numTransitionPatterns == 0) {
            result.push_back(to);
            return result;
        }

        // Generate crossfaded patterns
        for (int i = 0; i < numTransitionPatterns; i++) {
            float blend = static_cast<float>(i + 1) / (numTransitionPatterns + 1);
            Pattern blended = blendPatterns(from, to, blend);
            result.push_back(blended);
        }

        result.push_back(to);
        return result;
    }

    // ========================================
    // Unified Apply Function
    // ========================================

    ModulatedPattern apply(const Pattern& input, ModulationType type) {
        switch (type) {
            case ModulationType::HALF_TIME:
                return applyHalfTime(input);
            case ModulationType::DOUBLE_TIME:
                return applyDoubleTime(input);
            case ModulationType::TRIPLET_FEEL:
                return applyTripletFeel(input);
            case ModulationType::STRAIGHT_FEEL:
                return applyStraightFeel(input);
            case ModulationType::HALF_TIME_SHUFFLE:
                return applyHalfTimeShuffle(input);
            case ModulationType::DOUBLE_TIME_SWING:
                return applyDoubleTimeSwing(input);
            case ModulationType::IRAMA_UP:
                return applyIramaUp(input);
            case ModulationType::IRAMA_DOWN:
                return applyIramaDown(input);
            default:
                ModulatedPattern result;
                result.pattern = input;
                result.originalLength = input.length;
                result.modulatedLength = input.length;
                result.appliedModulation = type;
                result.perceivedTempo = 1.0f;
                result.effectiveSwing = 0.5f;
                return result;
        }
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getTypeName(ModulationType type) {
        return getModulationDef(type).name;
    }

    static std::string getTypeDescription(ModulationType type) {
        return getModulationDef(type).description;
    }

    int getNumTypes() const {
        return static_cast<int>(ModulationType::NUM_TYPES);
    }

private:
    std::mt19937 gen;

    int quantizeToTriplet(int pos, int length) {
        // Triplet positions in 16-step: 0, 2.67, 5.33, 8, 10.67, 13.33
        // Approximate to: 0, 3, 5, 8, 11, 13
        std::vector<int> tripletPositions;
        for (int i = 0; i < 6; i++) {
            tripletPositions.push_back((i * length * 2) / 12);
        }

        // Find nearest triplet position
        int nearest = 0;
        int minDist = length;
        for (int tp : tripletPositions) {
            int dist = std::abs(pos - tp);
            if (dist < minDist) {
                minDist = dist;
                nearest = tp;
            }
        }
        return nearest;
    }

    int quantizeToStraight(int pos, int /*length*/) {
        // Straight 16th note positions: 0, 1, 2, 3, 4...
        // Just round to nearest position (effectively no change if already quantized)
        return pos;
    }

    Pattern blendPatterns(const Pattern& a, const Pattern& b, float blend) {
        int length = std::max(a.length, b.length);
        Pattern result(length);

        for (int i = 0; i < length; i++) {
            bool hasA = (i < a.length) && a.hasOnsetAt(i);
            bool hasB = (i < b.length) && b.hasOnsetAt(i);

            if (hasA && hasB) {
                // Both have hit: blend velocities
                float velA = a.getVelocity(i);
                float velB = b.getVelocity(i);
                result.setOnset(i, velA * (1.0f - blend) + velB * blend);
                result.accents[i] = (blend < 0.5f) ? a.accents[i] : b.accents[i];
            } else if (hasA && !hasB) {
                // Only A: fade out
                if ((1.0f - blend) > 0.3f) {
                    result.setOnset(i, a.getVelocity(i) * (1.0f - blend));
                    result.accents[i] = a.accents[i];
                }
            } else if (!hasA && hasB) {
                // Only B: fade in
                if (blend > 0.3f) {
                    result.setOnset(i, b.getVelocity(i) * blend);
                    result.accents[i] = b.accents[i];
                }
            }
        }

        return result;
    }
};

} // namespace WorldRhythm
