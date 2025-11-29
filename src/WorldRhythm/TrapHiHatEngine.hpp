#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Trap Hi-Hat Engine
// ========================================
// Based on fills_ornaments_research.md Section 2.7
//
// Implements trap/hip-hop hi-hat vocabulary:
// - Rhythmic subdivision changes (16th → triplet → 32nd)
// - Stutter patterns
// - Machine gun rolls
// - Pitch modulation
// - Velocity patterns

enum class TrapHiHatPattern {
    STRAIGHT_16TH = 0,  // Base 16th notes
    TRIPLET_16TH,       // 16th note triplets
    ROLL_32ND,          // 32nd note roll
    ROLL_64TH,          // 64th note (machine gun)
    STUTTER,            // 2x32nd replacing 1x16th
    MACHINE_GUN,        // 8x64th burst
    OPEN_CLOSE,         // Open-close alternation
    OFFBEAT,            // Only offbeats
    NUM_PATTERNS
};

// ========================================
// Hi-Hat Note with extended properties
// ========================================
struct TrapHiHatNote {
    float timing;       // Position within step (0.0-1.0)
    float velocity;     // 0.0-1.0
    float openness;     // 0.0=closed, 1.0=full open
    float pitchOffset;  // Semitones (-12 to +12)
    bool isAccent;
};

struct TrapHiHatStep {
    std::vector<TrapHiHatNote> notes;
    TrapHiHatPattern pattern;
};

// ========================================
// Trap Hi-Hat Engine Class
// ========================================
class TrapHiHatEngine {
private:
    std::mt19937 rng;
    float baseVelocity = 0.75f;
    float humanize = 0.1f;
    float pitchRange = 0.0f;     // 0 = no pitch mod, 12 = full octave

public:
    TrapHiHatEngine() : rng(std::random_device{}()) {}

    void seed(unsigned int s) { rng.seed(s); }

    void setBaseVelocity(float vel) {
        baseVelocity = std::clamp(vel, 0.0f, 1.0f);
    }

    void setHumanize(float h) {
        humanize = std::clamp(h, 0.0f, 1.0f);
    }

    void setPitchRange(float range) {
        pitchRange = std::clamp(range, 0.0f, 24.0f);
    }

    // ========================================
    // Generate Straight 16th Notes
    // ========================================
    TrapHiHatStep generateStraight16th(float velocity) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::STRAIGHT_16TH;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> timeVar(-0.01f, 0.01f);

        TrapHiHatNote note;
        note.timing = 0.0f + humanize * timeVar(rng);
        note.velocity = velocity + humanize * velVar(rng);
        note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
        note.openness = 0.0f;
        note.pitchOffset = 0.0f;
        note.isAccent = false;

        step.notes.push_back(note);
        return step;
    }

    // ========================================
    // Generate 16th Note Triplets
    // ========================================
    TrapHiHatStep generateTriplet16th(float velocity) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::TRIPLET_16TH;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> timeVar(-0.008f, 0.008f);

        // 3 notes per 16th note space
        for (int i = 0; i < 3; i++) {
            TrapHiHatNote note;
            note.timing = static_cast<float>(i) / 3.0f + humanize * timeVar(rng);
            note.velocity = velocity * (0.9f - 0.1f * i) + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;
            note.pitchOffset = 0.0f;
            note.isAccent = (i == 0);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate 32nd Note Roll
    // ========================================
    TrapHiHatStep generateRoll32nd(float velocity, bool crescendo = true) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::ROLL_32ND;

        std::uniform_real_distribution<float> velVar(-0.03f, 0.03f);
        std::uniform_real_distribution<float> timeVar(-0.005f, 0.005f);

        // 2 notes per 16th note space
        for (int i = 0; i < 2; i++) {
            TrapHiHatNote note;
            note.timing = static_cast<float>(i) / 2.0f + humanize * timeVar(rng);

            float velMod = crescendo ? (0.8f + 0.2f * i) : (1.0f - 0.1f * i);
            note.velocity = velocity * velMod + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;
            note.pitchOffset = 0.0f;
            note.isAccent = (i == 1 && crescendo);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate 64th Note Roll (Machine Gun)
    // ========================================
    TrapHiHatStep generateRoll64th(float velocity) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::ROLL_64TH;

        std::uniform_real_distribution<float> velVar(-0.02f, 0.02f);
        std::uniform_real_distribution<float> timeVar(-0.003f, 0.003f);

        // 4 notes per 16th note space
        for (int i = 0; i < 4; i++) {
            TrapHiHatNote note;
            note.timing = static_cast<float>(i) / 4.0f + humanize * timeVar(rng);
            note.velocity = velocity * (0.7f + 0.1f * i) + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;
            note.pitchOffset = 0.0f;
            note.isAccent = (i == 3);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate Stutter (2x32nd replacing 1x16th)
    // ========================================
    TrapHiHatStep generateStutter(float velocity) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::STUTTER;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);
        std::uniform_real_distribution<float> timeVar(-0.005f, 0.005f);

        // Quick double hit at the end of the step
        for (int i = 0; i < 2; i++) {
            TrapHiHatNote note;
            note.timing = 0.5f + static_cast<float>(i) * 0.25f + humanize * timeVar(rng);
            note.velocity = velocity * (0.85f + 0.15f * i) + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;
            note.pitchOffset = 0.0f;
            note.isAccent = (i == 1);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate Machine Gun (8x64th burst)
    // ========================================
    TrapHiHatStep generateMachineGun(float velocity) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::MACHINE_GUN;

        std::uniform_real_distribution<float> velVar(-0.02f, 0.02f);
        std::uniform_real_distribution<float> timeVar(-0.002f, 0.002f);

        // 8 extremely fast notes
        for (int i = 0; i < 8; i++) {
            TrapHiHatNote note;
            note.timing = static_cast<float>(i) / 8.0f + humanize * timeVar(rng);
            // Velocity ramp up then down
            float velCurve = std::sin(static_cast<float>(i) / 7.0f * M_PI);
            note.velocity = velocity * (0.6f + 0.4f * velCurve) + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;
            note.pitchOffset = 0.0f;
            note.isAccent = (i == 4 || i == 7);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate Open-Close Pattern
    // ========================================
    TrapHiHatStep generateOpenClose(float velocity, float openAmount) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::OPEN_CLOSE;

        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        // Open hit
        TrapHiHatNote open;
        open.timing = 0.0f;
        open.velocity = velocity + humanize * velVar(rng);
        open.velocity = std::clamp(open.velocity, 0.3f, 1.0f);
        open.openness = openAmount;
        open.pitchOffset = 0.0f;
        open.isAccent = true;
        step.notes.push_back(open);

        // Close hit (choke)
        TrapHiHatNote close;
        close.timing = 0.5f;
        close.velocity = velocity * 0.6f + humanize * velVar(rng);
        close.velocity = std::clamp(close.velocity, 0.3f, 1.0f);
        close.openness = 0.0f;
        close.pitchOffset = 0.0f;
        close.isAccent = false;
        step.notes.push_back(close);

        return step;
    }

    // ========================================
    // Generate Pitched Roll (rising or falling)
    // ========================================
    TrapHiHatStep generatePitchedRoll(float velocity, int numNotes,
                                       bool rising, float pitchAmount) {
        TrapHiHatStep step;
        step.pattern = TrapHiHatPattern::ROLL_32ND;

        std::uniform_real_distribution<float> velVar(-0.03f, 0.03f);
        std::uniform_real_distribution<float> timeVar(-0.005f, 0.005f);

        for (int i = 0; i < numNotes; i++) {
            TrapHiHatNote note;
            note.timing = static_cast<float>(i) / numNotes + humanize * timeVar(rng);
            note.velocity = velocity * (0.7f + 0.3f * i / (numNotes - 1)) + humanize * velVar(rng);
            note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            note.openness = 0.0f;

            // Pitch modulation
            float pitchProgress = static_cast<float>(i) / (numNotes - 1);
            note.pitchOffset = rising ? (pitchProgress * pitchAmount)
                                      : ((1.0f - pitchProgress) * pitchAmount);
            note.isAccent = (i == numNotes - 1);

            step.notes.push_back(note);
        }

        return step;
    }

    // ========================================
    // Generate Full Pattern with Variations
    // ========================================
    std::vector<TrapHiHatStep> generatePattern(int numSteps, float complexity,
                                                float rollProbability) {
        std::vector<TrapHiHatStep> pattern;

        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(0.7f, 0.9f);

        for (int i = 0; i < numSteps; i++) {
            float stepVel = baseVelocity * velVar(rng);

            // Determine pattern type based on position and probability
            TrapHiHatStep step;

            // More likely to have variation near phrase boundaries
            bool isPhraseBoundary = (i % 4 == 3) || (i % 8 == 7);
            float localRollProb = isPhraseBoundary ? rollProbability * 1.5f : rollProbability;

            // Complexity affects variation probability
            float variationProb = complexity * localRollProb;

            if (prob(rng) < variationProb) {
                // Choose a variation
                float r = prob(rng);

                if (r < 0.25f) {
                    step = generateTriplet16th(stepVel);
                } else if (r < 0.45f) {
                    step = generateRoll32nd(stepVel, true);
                } else if (r < 0.55f && complexity > 0.5f) {
                    step = generateRoll64th(stepVel);
                } else if (r < 0.70f) {
                    step = generateStutter(stepVel);
                } else if (r < 0.80f && complexity > 0.7f) {
                    step = generateMachineGun(stepVel);
                } else if (r < 0.90f) {
                    step = generateOpenClose(stepVel, 0.5f + prob(rng) * 0.5f);
                } else {
                    step = generatePitchedRoll(stepVel, 4, prob(rng) > 0.5f,
                                               pitchRange * prob(rng));
                }
            } else {
                // Basic 16th note
                step = generateStraight16th(stepVel);

                // Add accent on downbeats
                if (i % 4 == 0) {
                    step.notes[0].velocity *= 1.2f;
                    step.notes[0].velocity = std::min(step.notes[0].velocity, 1.0f);
                    step.notes[0].isAccent = true;
                }
            }

            pattern.push_back(step);
        }

        return pattern;
    }

    // ========================================
    // Apply Velocity Pattern to Steps
    // ========================================
    void applyVelocityPattern(std::vector<TrapHiHatStep>& pattern,
                              const std::string& velocityPattern) {
        // Pattern string: "H" = hard, "S" = soft, "M" = medium
        for (size_t i = 0; i < pattern.size() && i < velocityPattern.length(); i++) {
            float velMod = 1.0f;

            switch (velocityPattern[i]) {
                case 'H': velMod = 1.2f; break;
                case 'S': velMod = 0.6f; break;
                case 'M': velMod = 0.9f; break;
                default: velMod = 1.0f;
            }

            for (auto& note : pattern[i].notes) {
                note.velocity *= velMod;
                note.velocity = std::clamp(note.velocity, 0.3f, 1.0f);
            }
        }
    }

    // ========================================
    // Convert to Pattern Object
    // ========================================
    Pattern toPattern(const std::vector<TrapHiHatStep>& hiHatPattern,
                      int /*stepsPerBeat*/ = 4) {
        int totalSteps = static_cast<int>(hiHatPattern.size());
        Pattern p(totalSteps);

        for (int i = 0; i < totalSteps; i++) {
            const TrapHiHatStep& step = hiHatPattern[i];

            if (!step.notes.empty()) {
                // Use first note's velocity as the step velocity
                float maxVel = 0.0f;
                bool hasAccent = false;

                for (const auto& note : step.notes) {
                    maxVel = std::max(maxVel, note.velocity);
                    if (note.isAccent) hasAccent = true;
                }

                p.setOnset(i, maxVel);
                p.accents[i] = hasAccent;
            }
        }

        return p;
    }

    // ========================================
    // Get Pattern Subdivision Count
    // ========================================
    int getSubdivisionCount(const TrapHiHatStep& step) const {
        return static_cast<int>(step.notes.size());
    }

    // ========================================
    // Get Pattern Name
    // ========================================
    static const char* getPatternName(TrapHiHatPattern pattern) {
        switch (pattern) {
            case TrapHiHatPattern::STRAIGHT_16TH: return "Straight 16th";
            case TrapHiHatPattern::TRIPLET_16TH: return "Triplet";
            case TrapHiHatPattern::ROLL_32ND: return "32nd Roll";
            case TrapHiHatPattern::ROLL_64TH: return "64th Roll";
            case TrapHiHatPattern::STUTTER: return "Stutter";
            case TrapHiHatPattern::MACHINE_GUN: return "Machine Gun";
            case TrapHiHatPattern::OPEN_CLOSE: return "Open-Close";
            case TrapHiHatPattern::OFFBEAT: return "Offbeat";
            default: return "Unknown";
        }
    }

    // ========================================
    // Preset Patterns
    // ========================================
    std::vector<TrapHiHatStep> generatePreset(const std::string& presetName,
                                               int numSteps) {
        std::vector<TrapHiHatStep> pattern;

        if (presetName == "basic") {
            // Simple 16th notes with accent on 1
            for (int i = 0; i < numSteps; i++) {
                pattern.push_back(generateStraight16th(baseVelocity));
                if (i % 4 == 0) pattern.back().notes[0].isAccent = true;
            }
        } else if (presetName == "bounce") {
            // Alternating velocity for bounce feel
            for (int i = 0; i < numSteps; i++) {
                float vel = (i % 2 == 0) ? baseVelocity : baseVelocity * 0.6f;
                pattern.push_back(generateStraight16th(vel));
            }
        } else if (presetName == "triplet_flow") {
            // Triplets throughout
            for (int i = 0; i < numSteps; i++) {
                pattern.push_back(generateTriplet16th(baseVelocity));
            }
        } else if (presetName == "roll_end") {
            // Normal pattern with roll at end
            for (int i = 0; i < numSteps; i++) {
                if (i >= numSteps - 2) {
                    pattern.push_back(generateRoll32nd(baseVelocity, true));
                } else {
                    pattern.push_back(generateStraight16th(baseVelocity));
                }
            }
        } else if (presetName == "stutter_groove") {
            // Stutters on certain positions
            for (int i = 0; i < numSteps; i++) {
                if (i % 4 == 3) {
                    pattern.push_back(generateStutter(baseVelocity));
                } else {
                    pattern.push_back(generateStraight16th(baseVelocity));
                }
            }
        } else {
            // Default to basic
            return generatePreset("basic", numSteps);
        }

        return pattern;
    }
};

} // namespace WorldRhythm
