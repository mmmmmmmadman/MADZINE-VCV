#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include "PatternGenerator.hpp"
#include "StyleProfiles.hpp"

namespace WorldRhythm {

// ========================================
// Amen Break Engine - Breakbeat Pattern Library
// ========================================
// The Amen Break is a 4-bar drum break from "Amen, Brother" (1969)
// that became the foundation of jungle, drum & bass, and breakbeat.
//
// This engine provides:
// - Original Amen pattern and variations
// - Classic "chop" patterns (Think, Funky Drummer, Apache, etc.)
// - Algorithmic slice rearrangement
// - Time-stretch feel simulation
// - Density-controlled pattern generation using StyleProfile weights

// ========================================
// Break Types
// ========================================

enum class BreakType {
    AMEN_ORIGINAL = 0,    // The original Amen break
    AMEN_CHOPPED,         // Standard chop rearrangement
    AMEN_HALF_TIME,       // Half-time feel
    AMEN_DOUBLE_TIME,     // Double-time feel
    THINK_BREAK,          // "Think" by Lyn Collins
    FUNKY_DRUMMER,        // James Brown's Funky Drummer
    APACHE,               // Incredible Bongo Band
    SKULL_SNAPS,          // "It's a New Day"
    HOT_PANTS,            // James Brown
    SYNTHETIC_SUBS,       // Synthetic Substitution
    NUM_TYPES
};

// ========================================
// Slice Definition
// ========================================

struct BreakSlice {
    int startStep;        // Start position in original (0-63 for 4 bars of 16th)
    int length;           // Length in steps
    float velocity;       // Base velocity
    bool hasKick;
    bool hasSnare;
    bool hasHihat;
    bool isGhostNote;
};

// ========================================
// Break Pattern Definition
// ========================================

struct BreakPattern {
    BreakType type;
    std::string name;
    std::string source;           // Original song/artist
    int originalBPM;
    int stepsPerBar;              // Usually 16
    std::vector<BreakSlice> slices;

    // Voice separation patterns (16 steps = 1 bar)
    std::vector<int> kickPattern;
    std::vector<int> snarePattern;
    std::vector<int> hihatPattern;
    std::vector<float> kickVelocities;
    std::vector<float> snareVelocities;
    std::vector<float> hihatVelocities;
};

// ========================================
// Break Pattern Definitions
// ========================================

inline BreakPattern createAmenOriginal() {
    BreakPattern bp;
    bp.type = BreakType::AMEN_ORIGINAL;
    bp.name = "Amen Break";
    bp.source = "The Winstons - Amen, Brother (1969)";
    bp.originalBPM = 136;
    bp.stepsPerBar = 16;

    // Original Amen pattern (1 bar, can be looped)
    // K = kick, S = snare, H = hi-hat, G = ghost snare
    // Position: 1 e & a 2 e & a 3 e & a 4 e & a
    //           0 1 2 3 4 5 6 7 8 9 A B C D E F

    bp.kickPattern = {0, 10};           // 1, 3&
    bp.snarePattern = {4, 7, 12, 15};   // 2, 2a, 4, 4a (with ghost on 2a)
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};  // 8th notes

    bp.kickVelocities = {1.0f, 0.85f};
    bp.snareVelocities = {1.0f, 0.5f, 1.0f, 0.5f};  // Ghosts at 50%
    bp.hihatVelocities = {0.7f, 0.5f, 0.7f, 0.5f, 0.7f, 0.5f, 0.7f, 0.5f};

    return bp;
}

inline BreakPattern createAmenChopped() {
    BreakPattern bp;
    bp.type = BreakType::AMEN_CHOPPED;
    bp.name = "Amen Chopped";
    bp.source = "Classic jungle/DnB rearrangement";
    bp.originalBPM = 170;
    bp.stepsPerBar = 16;

    // Classic chop: rearranged slices for more syncopation
    bp.kickPattern = {0, 6, 10};        // More syncopated kick
    bp.snarePattern = {4, 8, 12, 14};   // Snare hits shifted
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.8f, 0.9f};
    bp.snareVelocities = {1.0f, 0.7f, 1.0f, 0.6f};
    bp.hihatVelocities = {0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f};

    return bp;
}

inline BreakPattern createThinkBreak() {
    BreakPattern bp;
    bp.type = BreakType::THINK_BREAK;
    bp.name = "Think Break";
    bp.source = "Lyn Collins - Think (About It) (1972)";
    bp.originalBPM = 104;
    bp.stepsPerBar = 16;

    // Think break - funkier, less busy than Amen
    bp.kickPattern = {0, 10, 14};
    bp.snarePattern = {4, 12};
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.8f, 0.7f};
    bp.snareVelocities = {1.0f, 1.0f};
    bp.hihatVelocities = {0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f};

    return bp;
}

inline BreakPattern createFunkyDrummer() {
    BreakPattern bp;
    bp.type = BreakType::FUNKY_DRUMMER;
    bp.name = "Funky Drummer";
    bp.source = "James Brown - Funky Drummer (1970)";
    bp.originalBPM = 102;
    bp.stepsPerBar = 16;

    // Clyde Stubblefield's legendary pattern
    bp.kickPattern = {0, 7, 10};
    bp.snarePattern = {4, 12};
    bp.hihatPattern = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; // 16ths

    bp.kickVelocities = {1.0f, 0.7f, 0.85f};
    bp.snareVelocities = {1.0f, 1.0f};
    // Hi-hat pattern with accents
    bp.hihatVelocities = {0.8f, 0.3f, 0.5f, 0.3f, 0.7f, 0.3f, 0.5f, 0.3f,
                          0.8f, 0.3f, 0.5f, 0.3f, 0.7f, 0.3f, 0.5f, 0.3f};

    return bp;
}

inline BreakPattern createApache() {
    BreakPattern bp;
    bp.type = BreakType::APACHE;
    bp.name = "Apache";
    bp.source = "Incredible Bongo Band (1973)";
    bp.originalBPM = 110;
    bp.stepsPerBar = 16;

    // Apache - foundation of hip-hop
    bp.kickPattern = {0, 8};
    bp.snarePattern = {4, 12};
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.9f};
    bp.snareVelocities = {1.0f, 1.0f};
    bp.hihatVelocities = {0.8f, 0.5f, 0.8f, 0.5f, 0.8f, 0.5f, 0.8f, 0.5f};

    return bp;
}

inline BreakPattern createSkullSnaps() {
    BreakPattern bp;
    bp.type = BreakType::SKULL_SNAPS;
    bp.name = "It's a New Day";
    bp.source = "Skull Snaps (1973)";
    bp.originalBPM = 100;
    bp.stepsPerBar = 16;

    bp.kickPattern = {0, 6, 10};
    bp.snarePattern = {4, 12, 14};
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.75f, 0.85f};
    bp.snareVelocities = {1.0f, 1.0f, 0.6f};
    bp.hihatVelocities = {0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f};

    return bp;
}

inline BreakPattern createHotPants() {
    BreakPattern bp;
    bp.type = BreakType::HOT_PANTS;
    bp.name = "Hot Pants";
    bp.source = "James Brown - Hot Pants (1971)";
    bp.originalBPM = 114;
    bp.stepsPerBar = 16;

    bp.kickPattern = {0, 10};
    bp.snarePattern = {4, 7, 12, 15};
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.8f};
    bp.snareVelocities = {1.0f, 0.5f, 1.0f, 0.5f};
    bp.hihatVelocities = {0.7f, 0.5f, 0.7f, 0.5f, 0.7f, 0.5f, 0.7f, 0.5f};

    return bp;
}

inline BreakPattern createSyntheticSubs() {
    BreakPattern bp;
    bp.type = BreakType::SYNTHETIC_SUBS;
    bp.name = "Synthetic Substitution";
    bp.source = "Melvin Bliss (1973)";
    bp.originalBPM = 98;
    bp.stepsPerBar = 16;

    bp.kickPattern = {0, 10, 14};
    bp.snarePattern = {4, 12};
    bp.hihatPattern = {0, 2, 4, 6, 8, 10, 12, 14};

    bp.kickVelocities = {1.0f, 0.85f, 0.7f};
    bp.snareVelocities = {1.0f, 1.0f};
    bp.hihatVelocities = {0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f};

    return bp;
}

// ========================================
// Chop Patterns (slice rearrangements)
// ========================================

struct ChopPattern {
    std::string name;
    std::vector<int> sliceOrder;  // Which slice plays at each position
    float intensity;
};

inline std::vector<ChopPattern> getStandardChops() {
    return {
        {"Original", {0, 1, 2, 3, 4, 5, 6, 7}, 0.8f},
        {"Reverse", {7, 6, 5, 4, 3, 2, 1, 0}, 0.9f},
        {"Jungle 1", {0, 1, 0, 3, 4, 5, 4, 7}, 0.85f},
        {"Jungle 2", {0, 3, 2, 1, 4, 7, 6, 5}, 0.9f},
        {"Stutter", {0, 0, 2, 2, 4, 4, 6, 6}, 0.95f},
        {"Roll End", {0, 1, 2, 3, 6, 6, 7, 7}, 0.9f},
        {"Skip", {0, 2, 4, 6, 1, 3, 5, 7}, 0.85f},
        {"Tension", {0, 1, 0, 1, 4, 5, 4, 5}, 0.9f}
    };
}

// ========================================
// Amen Break Engine
// ========================================

class AmenBreakEngine {
public:
    AmenBreakEngine() : currentType(BreakType::AMEN_ORIGINAL), gen(std::random_device{}()) {
        patterns[BreakType::AMEN_ORIGINAL] = createAmenOriginal();
        patterns[BreakType::AMEN_CHOPPED] = createAmenChopped();
        patterns[BreakType::THINK_BREAK] = createThinkBreak();
        patterns[BreakType::FUNKY_DRUMMER] = createFunkyDrummer();
        patterns[BreakType::APACHE] = createApache();
        patterns[BreakType::SKULL_SNAPS] = createSkullSnaps();
        patterns[BreakType::HOT_PANTS] = createHotPants();
        patterns[BreakType::SYNTHETIC_SUBS] = createSyntheticSubs();

        chops = getStandardChops();
    }

    // ========================================
    // Type Selection
    // ========================================

    void setType(BreakType type) {
        currentType = type;
    }

    void setTypeByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(BreakType::NUM_TYPES)) {
            currentType = static_cast<BreakType>(index);
        }
    }

    BreakType getType() const { return currentType; }

    const BreakPattern& getCurrentPattern() const {
        auto it = patterns.find(currentType);
        if (it != patterns.end()) {
            return it->second;
        }
        return patterns.at(BreakType::AMEN_ORIGINAL);
    }

    std::string getCurrentName() const {
        return getCurrentPattern().name;
    }

    // ========================================
    // Pattern Generation
    // ========================================
    //
    // Density behavior:
    // - 0.0: Empty pattern (silence)
    // - 0.0-0.3: Sparse - only strongest hits from original pattern
    // - 0.3-0.5: Original pattern (classic break feel)
    // - 0.5-1.0: Original + additional hits using breakbeat-appropriate weights
    //
    // This ensures Breakbeat patterns scale properly with density like other styles.

    // Internal weights for adding extra hits (more positions available than StyleProfile)
    // These allow density to actually add more hits
    static constexpr float KICK_EXTRA_WEIGHTS[16] = {
        0.0f, 0.3f, 0.5f, 0.4f, 0.2f, 0.4f, 0.6f, 0.5f,
        0.0f, 0.4f, 0.0f, 0.5f, 0.3f, 0.5f, 0.0f, 0.4f
    };
    static constexpr float SNARE_EXTRA_WEIGHTS[16] = {
        0.2f, 0.5f, 0.4f, 0.6f, 0.0f, 0.5f, 0.4f, 0.7f,
        0.3f, 0.6f, 0.4f, 0.5f, 0.0f, 0.6f, 0.5f, 0.7f
    };
    static constexpr float HIHAT_EXTRA_WEIGHTS[16] = {
        0.8f, 0.6f, 0.8f, 0.5f, 0.8f, 0.6f, 0.8f, 0.5f,
        0.8f, 0.6f, 0.8f, 0.5f, 0.8f, 0.6f, 0.8f, 0.5f
    };

    // Generate kick pattern with density control
    // density: 0.0 = empty, 0.4 = original pattern, 1.0 = full with extras
    Pattern generateKick(int length, float density) {
        const auto& bp = getCurrentPattern();
        Pattern p(length);

        if (density < 0.01f) {
            return p;  // Empty pattern
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Phase 1: Add original pattern hits (probability based on density)
        // At density 0.4, all original hits are included
        float originalProb = std::min(1.0f, density * 2.5f);  // 0->0, 0.4->1.0, 1.0->1.0

        for (size_t i = 0; i < bp.kickPattern.size(); i++) {
            float hitProb = originalProb * (0.5f + 0.5f * bp.kickVelocities[i]);

            if (dist(gen) < hitProb) {
                int pos = bp.kickPattern[i];
                int mappedPos = (pos * length) / 16;
                if (mappedPos < length) {
                    float vel = bp.kickVelocities[i];
                    p.setOnset(mappedPos, vel);
                    p.accents[mappedPos] = (vel > 0.7f);
                }
            }
        }

        // Phase 2: Add extra hits when density > 0.4
        if (density > 0.4f) {
            float extraDensity = (density - 0.4f) / 0.6f;  // 0.4->0, 1.0->1.0
            // Target: at density 1.0, aim for ~50% of positions (8 hits in 16 steps)
            int targetTotal = static_cast<int>(std::round(length * density * 0.5f));
            int currentHits = 0;
            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) currentHits++;
            }
            int extraHits = std::max(0, targetTotal - currentHits);

            for (int n = 0; n < extraHits; n++) {
                float bestWeight = 0.0f;
                int bestPos = -1;

                for (int i = 0; i < length; i++) {
                    if (p.hasOnsetAt(i)) continue;

                    int mapped = (i * 16) / length;
                    float w = KICK_EXTRA_WEIGHTS[mapped % 16];

                    // Reduce weight for positions adjacent to existing hits (avoid flamming)
                    int prev = (i - 1 + length) % length;
                    int next = (i + 1) % length;
                    if (p.hasOnsetAt(prev) || p.hasOnsetAt(next)) w *= 0.5f;

                    // Add randomness scaled by extraDensity
                    w *= (0.5f + dist(gen) * 0.5f);
                    w *= extraDensity;

                    if (w > bestWeight) {
                        bestWeight = w;
                        bestPos = i;
                    }
                }

                if (bestPos >= 0) {
                    float vel = 0.6f + extraDensity * 0.3f + velVar(gen);
                    p.setOnset(bestPos, std::clamp(vel, 0.5f, 0.9f));
                }
            }
        }

        return p;
    }

    // Generate snare pattern with density control
    Pattern generateSnare(int length, float density) {
        const auto& bp = getCurrentPattern();
        Pattern p(length);

        if (density < 0.01f) {
            return p;  // Empty pattern
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Phase 1: Add original pattern hits
        float originalProb = std::min(1.0f, density * 2.5f);

        for (size_t i = 0; i < bp.snarePattern.size(); i++) {
            float hitProb = originalProb * (0.5f + 0.5f * bp.snareVelocities[i]);

            if (dist(gen) < hitProb) {
                int pos = bp.snarePattern[i];
                int mappedPos = (pos * length) / 16;
                if (mappedPos < length) {
                    float vel = bp.snareVelocities[i];
                    p.setOnset(mappedPos, vel);
                    p.accents[mappedPos] = (vel > 0.6f);
                }
            }
        }

        // Phase 2: Add extra hits (ghost notes) when density > 0.4
        if (density > 0.4f) {
            float extraDensity = (density - 0.4f) / 0.6f;
            // Target: at density 1.0, aim for ~45% of positions
            int targetTotal = static_cast<int>(std::round(length * density * 0.45f));
            int currentHits = 0;
            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) currentHits++;
            }
            int extraHits = std::max(0, targetTotal - currentHits);

            for (int n = 0; n < extraHits; n++) {
                float bestWeight = 0.0f;
                int bestPos = -1;

                for (int i = 0; i < length; i++) {
                    if (p.hasOnsetAt(i)) continue;

                    int mapped = (i * 16) / length;
                    float w = SNARE_EXTRA_WEIGHTS[mapped % 16];

                    // Prefer off-beat positions for ghost notes
                    if (i % 2 == 1) w *= 1.2f;

                    // Reduce weight near existing hits
                    int prev = (i - 1 + length) % length;
                    if (p.hasOnsetAt(prev)) w *= 0.6f;

                    w *= (0.5f + dist(gen) * 0.5f);
                    w *= extraDensity;

                    if (w > bestWeight) {
                        bestWeight = w;
                        bestPos = i;
                    }
                }

                if (bestPos >= 0) {
                    // Extra snare hits are ghost notes (softer)
                    float vel = 0.35f + extraDensity * 0.25f + velVar(gen);
                    p.setOnset(bestPos, std::clamp(vel, 0.3f, 0.65f));
                    p.accents[bestPos] = false;  // Ghost note
                }
            }
        }

        return p;
    }

    // Generate hi-hat pattern with density control
    Pattern generateHihat(int length, float density) {
        const auto& bp = getCurrentPattern();
        Pattern p(length);

        if (density < 0.01f) {
            return p;  // Empty pattern
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Phase 1: Add original pattern hits
        float originalProb = std::min(1.0f, density * 2.5f);

        for (size_t i = 0; i < bp.hihatPattern.size(); i++) {
            if (dist(gen) < originalProb) {
                int pos = bp.hihatPattern[i];
                int mappedPos = (pos * length) / 16;
                if (mappedPos < length) {
                    float vel = bp.hihatVelocities[i];
                    p.setOnset(mappedPos, vel);
                }
            }
        }

        // Phase 2: Add extra hits when density > 0.4
        if (density > 0.4f) {
            float extraDensity = (density - 0.4f) / 0.6f;
            // Target: at density 1.0, aim for ~85% of positions (hi-hats are dense)
            int targetTotal = static_cast<int>(std::round(length * density * 0.85f));
            int currentHits = 0;
            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) currentHits++;
            }
            int extraHits = std::max(0, targetTotal - currentHits);

            for (int n = 0; n < extraHits; n++) {
                float bestWeight = 0.0f;
                int bestPos = -1;

                for (int i = 0; i < length; i++) {
                    if (p.hasOnsetAt(i)) continue;

                    int mapped = (i * 16) / length;
                    float w = HIHAT_EXTRA_WEIGHTS[mapped % 16];

                    // Add some randomness
                    w *= (0.5f + dist(gen) * 0.5f);
                    w *= extraDensity;

                    if (w > bestWeight) {
                        bestWeight = w;
                        bestPos = i;
                    }
                }

                if (bestPos >= 0) {
                    float vel = 0.45f + extraDensity * 0.35f + velVar(gen);
                    p.setOnset(bestPos, std::clamp(vel, 0.35f, 0.85f));
                }
            }
        }

        return p;
    }

    // Generate combined pattern with density control
    Pattern generateCombined(int length, float density) {
        if (density < 0.01f) {
            return Pattern(length);  // Empty pattern
        }

        Pattern kick = generateKick(length, density);
        Pattern snare = generateSnare(length, density);
        Pattern hihat = generateHihat(length, density * 0.7f);

        Pattern combined(length);
        for (int i = 0; i < length; i++) {
            float maxVel = 0.0f;
            if (kick.hasOnsetAt(i)) maxVel = std::max(maxVel, kick.getVelocity(i));
            if (snare.hasOnsetAt(i)) maxVel = std::max(maxVel, snare.getVelocity(i));
            if (hihat.hasOnsetAt(i)) maxVel = std::max(maxVel, hihat.getVelocity(i));

            if (maxVel > 0) {
                combined.setOnset(i, maxVel);
                combined.accents[i] = kick.hasOnsetAt(i) || snare.accents[i];
            }
        }

        return combined;
    }

    // ========================================
    // Chop / Slice Operations
    // ========================================

    // Apply a chop pattern to rearrange slices
    Pattern applyChop(const Pattern& original, int chopIndex) {
        if (chopIndex < 0 || chopIndex >= static_cast<int>(chops.size())) {
            return original;
        }

        const auto& chop = chops[chopIndex];
        int sliceSize = original.length / 8;  // 8 slices per bar
        Pattern result(original.length);

        for (int i = 0; i < 8; i++) {
            int sourceSlice = chop.sliceOrder[i];
            int destStart = i * sliceSize;
            int srcStart = sourceSlice * sliceSize;

            for (int j = 0; j < sliceSize; j++) {
                int srcPos = srcStart + j;
                int destPos = destStart + j;

                if (srcPos < original.length && destPos < result.length) {
                    if (original.hasOnsetAt(srcPos)) {
                        result.setOnset(destPos, original.getVelocity(srcPos) * chop.intensity);
                        result.accents[destPos] = original.accents[srcPos];
                    }
                }
            }
        }

        return result;
    }

    // Random chop generation with density control
    Pattern generateRandomChop(int length, float density, float chopIntensity) {
        if (density < 0.01f) {
            return Pattern(length);  // Empty pattern
        }

        Pattern base = generateCombined(length, density);

        if (chopIntensity < 0.1f) return base;

        // Create random slice order
        std::vector<int> sliceOrder = {0, 1, 2, 3, 4, 5, 6, 7};

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Shuffle based on intensity
        for (int i = 7; i > 0; i--) {
            if (dist(gen) < chopIntensity) {
                std::uniform_int_distribution<int> swapDist(0, i);
                int j = swapDist(gen);
                std::swap(sliceOrder[i], sliceOrder[j]);
            }
        }

        // Apply the random chop
        int sliceSize = length / 8;
        Pattern result(length);

        for (int i = 0; i < 8; i++) {
            int sourceSlice = sliceOrder[i];
            int destStart = i * sliceSize;
            int srcStart = sourceSlice * sliceSize;

            for (int j = 0; j < sliceSize; j++) {
                int srcPos = srcStart + j;
                int destPos = destStart + j;

                if (srcPos < base.length && destPos < result.length) {
                    if (base.hasOnsetAt(srcPos)) {
                        result.setOnset(destPos, base.getVelocity(srcPos));
                        result.accents[destPos] = base.accents[srcPos];
                    }
                }
            }
        }

        return result;
    }

    // ========================================
    // Time-Stretch Simulation
    // ========================================

    // Generate half-time feel with density control
    Pattern generateHalfTime(int length, float density) {
        if (density < 0.01f) {
            return Pattern(length);  // Empty pattern
        }

        Pattern original = generateCombined(length * 2, density);
        Pattern result(length);

        // Take every other slice (effectively halving tempo feel)
        for (int i = 0; i < length; i++) {
            int srcPos = i * 2;
            if (srcPos < original.length && original.hasOnsetAt(srcPos)) {
                result.setOnset(i, original.getVelocity(srcPos));
                result.accents[i] = original.accents[srcPos];
            }
        }

        return result;
    }

    // Generate double-time feel with density control
    Pattern generateDoubleTime(int length, float density) {
        if (density < 0.01f) {
            return Pattern(length);  // Empty pattern
        }

        Pattern original = generateCombined(length / 2, density);
        Pattern result(length);

        // Duplicate pattern
        for (int i = 0; i < length; i++) {
            int srcPos = i % (length / 2);
            if (original.hasOnsetAt(srcPos)) {
                result.setOnset(i, original.getVelocity(srcPos));
                result.accents[i] = original.accents[srcPos];
            }
        }

        return result;
    }

    // ========================================
    // Ghost Note Enhancement
    // ========================================

    Pattern addGhostNotes(const Pattern& original, float density, float ghostVelocity) {
        Pattern result = original;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < result.length; i++) {
            // Don't add ghosts where there's already a hit
            if (!result.hasOnsetAt(i)) {
                // Higher chance for ghost notes on weak subdivisions
                bool isWeakBeat = (i % 2 == 1);  // Off 8ths
                float prob = isWeakBeat ? density * 1.5f : density;

                if (dist(gen) < prob) {
                    result.setOnset(i, ghostVelocity);
                }
            }
        }

        return result;
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getTypeName(BreakType type) {
        switch (type) {
            case BreakType::AMEN_ORIGINAL: return "Amen";
            case BreakType::AMEN_CHOPPED: return "Amen Chopped";
            case BreakType::AMEN_HALF_TIME: return "Amen Half";
            case BreakType::AMEN_DOUBLE_TIME: return "Amen Double";
            case BreakType::THINK_BREAK: return "Think";
            case BreakType::FUNKY_DRUMMER: return "Funky Drummer";
            case BreakType::APACHE: return "Apache";
            case BreakType::SKULL_SNAPS: return "Skull Snaps";
            case BreakType::HOT_PANTS: return "Hot Pants";
            case BreakType::SYNTHETIC_SUBS: return "Synthetic Subs";
            default: return "Unknown";
        }
    }

    int getNumTypes() const {
        return static_cast<int>(BreakType::NUM_TYPES);
    }

    int getNumChops() const {
        return static_cast<int>(chops.size());
    }

    std::string getChopName(int index) const {
        if (index >= 0 && index < static_cast<int>(chops.size())) {
            return chops[index].name;
        }
        return "Unknown";
    }

private:
    BreakType currentType;
    std::map<BreakType, BreakPattern> patterns;
    std::vector<ChopPattern> chops;
    std::mt19937 gen;
};

} // namespace WorldRhythm
