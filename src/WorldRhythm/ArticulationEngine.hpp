#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Articulation Engine - Ornament Timing Expansion
// ========================================
// Based on fills_ornaments_research.md Section 3
//
// Expands single hits into multi-hit ornaments with proper timing:
// - Flam: Two strokes nearly simultaneous
// - Drag: Two grace notes before main
// - Ruff: Three grace notes before main
// - Buzz: Multiple bounces per stroke
// - Roll: Sustained rapid alternation

enum class OrnamentType {
    NONE = 0,
    FLAM,           // Two hits nearly simultaneous
    DRAG,           // Two grace notes + main
    RUFF,           // Three grace notes + main (4-stroke)
    BUZZ,           // Multiple bounces (press roll)
    SINGLE_ROLL,    // LR alternation
    DOUBLE_ROLL,    // LLRR alternation
    DIDDLE,         // RR or LL double stroke
    PARADIDDLE,     // RLRR LRLL pattern
    FLAM_TAP,       // Flam + tap
    NUM_TYPES
};

// ========================================
// Expanded Note Structure
// ========================================
struct ExpandedNote {
    float timing;       // Relative timing (0.0 = on beat)
    float velocity;     // 0.0-1.0
    bool isGrace;       // Is this a grace note?
    bool isAccent;      // Is this accented?
    int hand;           // 0=right, 1=left (for alternation)
    float pitchOffset;  // Semitones offset (for buzz rolls)
};

struct ExpandedHit {
    std::vector<ExpandedNote> notes;
    OrnamentType ornament;
    int originalPosition;
};

// ========================================
// Ornament Timing Parameters
// ========================================
struct OrnamentTiming {
    float flamOffset;       // Seconds before main hit (default: 0.025-0.040)
    float dragOffset;       // Seconds per grace note (default: 0.030-0.050)
    float buzzInterval;     // Seconds between buzz bounces
    float rollSpeed;        // Hits per second for rolls
    float graceVelocity;    // Velocity multiplier for grace notes (0.3-0.5)
};

inline OrnamentTiming getDefaultTiming() {
    OrnamentTiming t;
    t.flamOffset = 0.030f;      // 30ms
    t.dragOffset = 0.040f;      // 40ms per note
    t.buzzInterval = 0.015f;    // 15ms between buzzes
    t.rollSpeed = 15.0f;        // 15 hits per second
    t.graceVelocity = 0.4f;     // 40% of main velocity
    return t;
}

// ========================================
// Articulation Engine Class
// ========================================
class ArticulationEngine {
private:
    std::mt19937 rng;
    OrnamentTiming timing;
    float humanizeAmount = 0.1f;    // 0-1, adds timing variation

public:
    ArticulationEngine() : rng(std::random_device{}()) {
        timing = getDefaultTiming();
    }

    void seed(unsigned int s) { rng.seed(s); }

    void setTiming(const OrnamentTiming& t) { timing = t; }
    OrnamentTiming& getTiming() { return timing; }

    void setHumanize(float amount) {
        humanizeAmount = std::clamp(amount, 0.0f, 1.0f);
    }

    // ========================================
    // Generate Flam
    // ========================================
    ExpandedHit generateFlam(float velocity, bool rightHandLead = true) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::FLAM;

        std::uniform_real_distribution<float> humanize(-0.005f, 0.005f);
        float timeVar = humanizeAmount * humanize(rng);

        // Grace note
        ExpandedNote grace;
        grace.timing = -(timing.flamOffset + timeVar);
        grace.velocity = velocity * timing.graceVelocity;
        grace.isGrace = true;
        grace.isAccent = false;
        grace.hand = rightHandLead ? 1 : 0;  // Opposite hand leads
        grace.pitchOffset = 0.0f;
        hit.notes.push_back(grace);

        // Main note
        ExpandedNote main;
        main.timing = 0.0f;
        main.velocity = velocity;
        main.isGrace = false;
        main.isAccent = true;
        main.hand = rightHandLead ? 0 : 1;
        main.pitchOffset = 0.0f;
        hit.notes.push_back(main);

        return hit;
    }

    // ========================================
    // Generate Drag (Two grace notes)
    // ========================================
    ExpandedHit generateDrag(float velocity, bool rightHandLead = true) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::DRAG;

        std::uniform_real_distribution<float> humanize(-0.005f, 0.005f);

        // Two grace notes (diddle)
        for (int i = 0; i < 2; i++) {
            float timeVar = humanizeAmount * humanize(rng);
            ExpandedNote grace;
            grace.timing = -timing.dragOffset * (2 - i) + timeVar;
            grace.velocity = velocity * timing.graceVelocity * (0.8f + 0.2f * i);
            grace.isGrace = true;
            grace.isAccent = false;
            grace.hand = rightHandLead ? 1 : 0;  // Same hand for both grace
            grace.pitchOffset = 0.0f;
            hit.notes.push_back(grace);
        }

        // Main note
        ExpandedNote main;
        main.timing = 0.0f;
        main.velocity = velocity;
        main.isGrace = false;
        main.isAccent = true;
        main.hand = rightHandLead ? 0 : 1;
        main.pitchOffset = 0.0f;
        hit.notes.push_back(main);

        return hit;
    }

    // ========================================
    // Generate Ruff (Three grace notes)
    // ========================================
    ExpandedHit generateRuff(float velocity, bool rightHandLead = true) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::RUFF;

        std::uniform_real_distribution<float> humanize(-0.003f, 0.003f);

        // Three grace notes
        for (int i = 0; i < 3; i++) {
            float timeVar = humanizeAmount * humanize(rng);
            ExpandedNote grace;
            grace.timing = -timing.dragOffset * (3 - i) * 0.8f + timeVar;
            grace.velocity = velocity * timing.graceVelocity * (0.6f + 0.15f * i);
            grace.isGrace = true;
            grace.isAccent = false;
            // Alternate hands for ruff: L-R-L or R-L-R
            grace.hand = (i % 2 == 0) ? (rightHandLead ? 1 : 0) : (rightHandLead ? 0 : 1);
            grace.pitchOffset = 0.0f;
            hit.notes.push_back(grace);
        }

        // Main note
        ExpandedNote main;
        main.timing = 0.0f;
        main.velocity = velocity;
        main.isGrace = false;
        main.isAccent = true;
        main.hand = rightHandLead ? 0 : 1;
        main.pitchOffset = 0.0f;
        hit.notes.push_back(main);

        return hit;
    }

    // ========================================
    // Generate Buzz Roll
    // ========================================
    ExpandedHit generateBuzz(float velocity, float duration, int bounces = 4) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::BUZZ;

        std::uniform_real_distribution<float> humanize(-0.002f, 0.002f);
        std::uniform_real_distribution<float> velVar(0.9f, 1.1f);

        float interval = duration / bounces;

        for (int i = 0; i < bounces; i++) {
            float timeVar = humanizeAmount * humanize(rng);
            ExpandedNote note;
            note.timing = -duration + interval * i + timeVar;
            // Velocity decreases naturally for buzz
            float decay = 1.0f - (static_cast<float>(bounces - 1 - i) / bounces) * 0.5f;
            note.velocity = velocity * decay * velVar(rng);
            note.isGrace = (i < bounces - 1);
            note.isAccent = (i == bounces - 1);
            note.hand = 0;  // Same hand for buzz
            note.pitchOffset = 0.0f;
            hit.notes.push_back(note);
        }

        return hit;
    }

    // ========================================
    // Generate Single Stroke Roll
    // ========================================
    ExpandedHit generateSingleRoll(float velocity, float duration) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::SINGLE_ROLL;

        std::uniform_real_distribution<float> humanize(-0.003f, 0.003f);
        std::uniform_real_distribution<float> velVar(0.95f, 1.05f);

        int numStrokes = static_cast<int>(duration * timing.rollSpeed);
        numStrokes = std::max(2, numStrokes);
        float interval = duration / numStrokes;

        for (int i = 0; i < numStrokes; i++) {
            float timeVar = humanizeAmount * humanize(rng);
            ExpandedNote note;
            note.timing = -duration + interval * i + timeVar;
            note.velocity = velocity * velVar(rng);
            note.isGrace = (i < numStrokes - 1);
            note.isAccent = (i == numStrokes - 1);
            note.hand = i % 2;  // Alternate R-L-R-L
            note.pitchOffset = 0.0f;
            hit.notes.push_back(note);
        }

        return hit;
    }

    // ========================================
    // Generate Double Stroke Roll
    // ========================================
    ExpandedHit generateDoubleRoll(float velocity, float duration) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::DOUBLE_ROLL;

        std::uniform_real_distribution<float> humanize(-0.003f, 0.003f);
        std::uniform_real_distribution<float> velVar(0.9f, 1.1f);

        int numStrokes = static_cast<int>(duration * timing.rollSpeed * 1.5f);
        numStrokes = std::max(4, (numStrokes / 4) * 4);  // Round to multiple of 4
        float interval = duration / numStrokes;

        for (int i = 0; i < numStrokes; i++) {
            float timeVar = humanizeAmount * humanize(rng);
            ExpandedNote note;
            note.timing = -duration + interval * i + timeVar;

            // Second stroke of double is slightly softer
            float doubleDecay = (i % 2 == 1) ? 0.85f : 1.0f;
            note.velocity = velocity * doubleDecay * velVar(rng);

            note.isGrace = (i < numStrokes - 1);
            note.isAccent = (i == numStrokes - 1);
            note.hand = (i / 2) % 2;  // RR-LL-RR-LL
            note.pitchOffset = 0.0f;
            hit.notes.push_back(note);
        }

        return hit;
    }

    // ========================================
    // Generate Paradiddle
    // ========================================
    ExpandedHit generateParadiddle(float velocity, float duration) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::PARADIDDLE;

        std::uniform_real_distribution<float> humanize(-0.003f, 0.003f);
        std::uniform_real_distribution<float> velVar(0.95f, 1.05f);

        // RLRR LRLL pattern
        const int pattern[] = {0, 1, 0, 0, 1, 0, 1, 1};  // 0=R, 1=L
        const float accents[] = {1.0f, 0.7f, 0.8f, 0.75f, 1.0f, 0.7f, 0.8f, 0.75f};
        const int patternLen = 8;

        int numCycles = std::max(1, static_cast<int>(duration * timing.rollSpeed / patternLen));
        float interval = duration / (numCycles * patternLen);

        for (int cycle = 0; cycle < numCycles; cycle++) {
            for (int i = 0; i < patternLen; i++) {
                float timeVar = humanizeAmount * humanize(rng);
                ExpandedNote note;
                note.timing = -duration + interval * (cycle * patternLen + i) + timeVar;
                note.velocity = velocity * accents[i] * velVar(rng);
                note.isGrace = false;
                note.isAccent = (i == 0 || i == 4);  // Accent on first of each group
                note.hand = pattern[i];
                note.pitchOffset = 0.0f;
                hit.notes.push_back(note);
            }
        }

        return hit;
    }

    // ========================================
    // Generate Flam Tap
    // ========================================
    ExpandedHit generateFlamTap(float velocity, bool rightHandLead = true) {
        ExpandedHit hit;
        hit.ornament = OrnamentType::FLAM_TAP;

        std::uniform_real_distribution<float> humanize(-0.005f, 0.005f);

        // Flam
        ExpandedNote grace;
        grace.timing = -(timing.flamOffset + humanizeAmount * humanize(rng));
        grace.velocity = velocity * timing.graceVelocity;
        grace.isGrace = true;
        grace.isAccent = false;
        grace.hand = rightHandLead ? 1 : 0;
        grace.pitchOffset = 0.0f;
        hit.notes.push_back(grace);

        // Main note
        ExpandedNote main;
        main.timing = 0.0f;
        main.velocity = velocity;
        main.isGrace = false;
        main.isAccent = true;
        main.hand = rightHandLead ? 0 : 1;
        main.pitchOffset = 0.0f;
        hit.notes.push_back(main);

        // Tap (softer follow-up)
        ExpandedNote tap;
        tap.timing = timing.dragOffset * 2 + humanizeAmount * humanize(rng);
        tap.velocity = velocity * 0.6f;
        tap.isGrace = false;
        tap.isAccent = false;
        tap.hand = rightHandLead ? 0 : 1;  // Same hand as main
        tap.pitchOffset = 0.0f;
        hit.notes.push_back(tap);

        return hit;
    }

    // ========================================
    // Expand Pattern with Articulations
    // ========================================
    std::vector<ExpandedHit> expandPattern(const Pattern& p,
                                           const std::vector<OrnamentType>& ornaments,
                                           float stepDuration) {
        std::vector<ExpandedHit> result;

        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                float vel = p.getVelocity(i);
                OrnamentType orn = (i < static_cast<int>(ornaments.size()))
                                   ? ornaments[i] : OrnamentType::NONE;

                ExpandedHit hit;
                hit.originalPosition = i;

                switch (orn) {
                    case OrnamentType::FLAM:
                        hit = generateFlam(vel);
                        break;
                    case OrnamentType::DRAG:
                        hit = generateDrag(vel);
                        break;
                    case OrnamentType::RUFF:
                        hit = generateRuff(vel);
                        break;
                    case OrnamentType::BUZZ:
                        hit = generateBuzz(vel, stepDuration * 0.8f);
                        break;
                    case OrnamentType::SINGLE_ROLL:
                        hit = generateSingleRoll(vel, stepDuration);
                        break;
                    case OrnamentType::DOUBLE_ROLL:
                        hit = generateDoubleRoll(vel, stepDuration);
                        break;
                    case OrnamentType::PARADIDDLE:
                        hit = generateParadiddle(vel, stepDuration * 2);
                        break;
                    case OrnamentType::FLAM_TAP:
                        hit = generateFlamTap(vel);
                        break;
                    default: {
                        // Simple single note
                        ExpandedNote note;
                        note.timing = 0.0f;
                        note.velocity = vel;
                        note.isGrace = false;
                        note.isAccent = p.accents[i];
                        note.hand = 0;
                        note.pitchOffset = 0.0f;
                        hit.notes.push_back(note);
                        hit.ornament = OrnamentType::NONE;
                    }
                }

                hit.originalPosition = i;
                result.push_back(hit);
            }
        }

        return result;
    }

    // ========================================
    // Auto-assign Ornaments Based on Accents
    // ========================================
    std::vector<OrnamentType> autoAssignOrnaments(const Pattern& p,
                                                   float ornamentDensity,
                                                   int styleIndex) {
        std::vector<OrnamentType> ornaments(p.length, OrnamentType::NONE);
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            // Only ornament some hits based on density
            if (prob(rng) > ornamentDensity) continue;

            // Accented notes more likely to get flam
            if (p.accents[i]) {
                ornaments[i] = OrnamentType::FLAM;
            } else {
                // Choose ornament based on style
                float r = prob(rng);
                switch (styleIndex) {
                    case 0:  // West African - lots of flams
                        ornaments[i] = (r < 0.7f) ? OrnamentType::FLAM : OrnamentType::DRAG;
                        break;
                    case 1:  // Afro-Cuban
                        ornaments[i] = (r < 0.5f) ? OrnamentType::FLAM : OrnamentType::NONE;
                        break;
                    case 4:  // Indian
                        ornaments[i] = (r < 0.4f) ? OrnamentType::DRAG : OrnamentType::RUFF;
                        break;
                    case 6:  // Jazz
                        if (r < 0.3f) ornaments[i] = OrnamentType::FLAM;
                        else if (r < 0.5f) ornaments[i] = OrnamentType::BUZZ;
                        else if (r < 0.6f) ornaments[i] = OrnamentType::DRAG;
                        break;
                    case 7:  // Electronic
                    case 9:  // Techno
                        ornaments[i] = OrnamentType::NONE;  // Usually clean hits
                        break;
                    default:
                        ornaments[i] = (r < 0.5f) ? OrnamentType::FLAM : OrnamentType::NONE;
                }
            }
        }

        return ornaments;
    }

    // ========================================
    // Get Ornament Name
    // ========================================
    static const char* getOrnamentName(OrnamentType type) {
        switch (type) {
            case OrnamentType::NONE: return "None";
            case OrnamentType::FLAM: return "Flam";
            case OrnamentType::DRAG: return "Drag";
            case OrnamentType::RUFF: return "Ruff";
            case OrnamentType::BUZZ: return "Buzz";
            case OrnamentType::SINGLE_ROLL: return "Single Roll";
            case OrnamentType::DOUBLE_ROLL: return "Double Roll";
            case OrnamentType::DIDDLE: return "Diddle";
            case OrnamentType::PARADIDDLE: return "Paradiddle";
            case OrnamentType::FLAM_TAP: return "Flam Tap";
            default: return "Unknown";
        }
    }

    // ========================================
    // Calculate Total Notes in Expanded Hit
    // ========================================
    static int getNoteCount(const ExpandedHit& hit) {
        return static_cast<int>(hit.notes.size());
    }

    // ========================================
    // Get Time Range of Expanded Hit
    // ========================================
    static std::pair<float, float> getTimeRange(const ExpandedHit& hit) {
        float minTime = 0.0f, maxTime = 0.0f;
        for (const auto& note : hit.notes) {
            minTime = std::min(minTime, note.timing);
            maxTime = std::max(maxTime, note.timing);
        }
        return {minTime, maxTime};
    }
};

} // namespace WorldRhythm
