#pragma once

#include "PatternGenerator.hpp"
#include "StyleProfiles.hpp"
#include "FillGenerator.hpp"
#include "PhraseAnalyzer.hpp"
#include "CallResponseEngine.hpp"

namespace WorldRhythm {

struct GroupParams {
    int length = 16;
    float density = 0.5f;
    float variation = 0.0f;
    float rest = 0.0f;
    float fillProbability = 0.5f;
    float fillIntensity = 0.5f;
    float callResponseProbability = 0.5f;  // Call-and-response likelihood
    int styleIndex = -1;  // v0.16: -1 = use global style, 0-9 = per-role style
};

// ========================================
// Style Compatibility Matrix (v0.16)
// ========================================
// Defines how well different styles work together
// Based on shared pulse structures and cultural connections

struct StyleCompatibility {
    // Compatibility score 0.0-1.0 (1.0 = perfect match)
    static float getCompatibility(int styleA, int styleB) {
        if (styleA == styleB) return 1.0f;

        // Ensure styleA < styleB for lookup
        if (styleA > styleB) std::swap(styleA, styleB);

        // Compatibility matrix (symmetric, only upper triangle stored)
        // Based on shared pulse structures:
        // - 12/8 family: West African, Afro-Cuban (partial), Brazilian
        // - 4/4 family: Jazz, Electronic, Breakbeat, Techno
        // - Odd meter family: Balkan, Indian (some talas)
        // - Colotomic: Gamelan

        static const float matrix[10][10] = {
            //  WA    AC    BR    BK    IN    GM    JZ    EL    BB    TC
            {1.0f, 0.8f, 0.7f, 0.4f, 0.5f, 0.6f, 0.6f, 0.5f, 0.6f, 0.4f},  // West African
            {0.8f, 1.0f, 0.8f, 0.5f, 0.5f, 0.5f, 0.7f, 0.6f, 0.7f, 0.5f},  // Afro-Cuban
            {0.7f, 0.8f, 1.0f, 0.4f, 0.4f, 0.5f, 0.7f, 0.6f, 0.6f, 0.5f},  // Brazilian
            {0.4f, 0.5f, 0.4f, 1.0f, 0.7f, 0.5f, 0.5f, 0.4f, 0.4f, 0.4f},  // Balkan
            {0.5f, 0.5f, 0.4f, 0.7f, 1.0f, 0.6f, 0.5f, 0.4f, 0.4f, 0.3f},  // Indian
            {0.6f, 0.5f, 0.5f, 0.5f, 0.6f, 1.0f, 0.4f, 0.5f, 0.4f, 0.5f},  // Gamelan
            {0.6f, 0.7f, 0.7f, 0.5f, 0.5f, 0.4f, 1.0f, 0.7f, 0.8f, 0.6f},  // Jazz
            {0.5f, 0.6f, 0.6f, 0.4f, 0.4f, 0.5f, 0.7f, 1.0f, 0.8f, 0.9f},  // Electronic
            {0.6f, 0.7f, 0.6f, 0.4f, 0.4f, 0.4f, 0.8f, 0.8f, 1.0f, 0.7f},  // Breakbeat
            {0.4f, 0.5f, 0.5f, 0.4f, 0.3f, 0.5f, 0.6f, 0.9f, 0.7f, 1.0f}   // Techno
        };

        return matrix[styleA][styleB];
    }

    // Get recommended interlock strength based on style combination
    static float getInterlockStrength(int styleA, int styleB) {
        float compat = getCompatibility(styleA, styleB);

        // High compatibility = less interlock needed (they naturally fit)
        // Low compatibility = more interlock needed (avoid clashes)
        if (compat > 0.7f) return 0.3f;
        if (compat > 0.5f) return 0.5f;
        return 0.7f;
    }

    // Get style family for swing consistency
    enum StyleFamily {
        FAMILY_12_8,      // West African, Afro-Cuban, Brazilian
        FAMILY_4_4,       // Jazz, Electronic, Breakbeat, Techno
        FAMILY_ODD,       // Balkan, Indian
        FAMILY_COLOTOMIC  // Gamelan
    };

    static StyleFamily getFamily(int styleIndex) {
        switch (styleIndex) {
            case 0: case 1: case 2: return FAMILY_12_8;
            case 3: case 4: return FAMILY_ODD;
            case 5: return FAMILY_COLOTOMIC;
            case 6: case 7: case 8: case 9: return FAMILY_4_4;
            default: return FAMILY_4_4;
        }
    }

    // Check if styles are in same family
    static bool sameFamily(int styleA, int styleB) {
        return getFamily(styleA) == getFamily(styleB);
    }
};

// ========================================
// Humanization parameters
// ========================================
struct HumanizeParams {
    float bpm = 120.0f;
    float swingOverride = -1.0f;  // -1 = use style default
    float microtimingAmount = 0.5f;  // 0-1 intensity
    int phraseLength = 4;  // bars per phrase
};

struct BarPattern {
    Pattern patterns[4][3];  // [role][voice]
    bool hasFill;
    FillType fillType;
    int fillStartStep;
    int fillLengthSteps;
    bool hasCallResponse;
    CallResponsePair callResponse;
};

class RhythmEngine {
private:
    PatternGenerator generator;
    FillGenerator fillGen;
    CallResponseEngine crEngine;
    PhraseAnalyzer cvAnalyzer;
    mutable std::mt19937 humanRng{std::random_device{}()};
    float cvAdaptAmount = 0.0f;  // 0 = no adaptation, 1 = full

    // 4 groups, each with up to 3 voices
    Pattern patterns[4][3];
    int voicesPerGroup[4] = {2, 2, 3, 2};  // Timeline, Foundation, Groove, Lead

    // Multi-bar structure
    std::vector<BarPattern> bars;
    int numBars = 4;

    int styleIndex = 0;
    int roleStyles[4] = {-1, -1, -1, -1};  // v0.16: per-role style override

public:
    // ========================================
    // Get effective style for a role (v0.16)
    // ========================================
    int getEffectiveStyle(Role role, const GroupParams& params) const {
        // Priority: GroupParams > roleStyles > global styleIndex
        if (params.styleIndex >= 0 && params.styleIndex < NUM_STYLES) {
            return params.styleIndex;
        }
        if (roleStyles[role] >= 0 && roleStyles[role] < NUM_STYLES) {
            return roleStyles[role];
        }
        return styleIndex;
    }

    const StyleProfile& getEffectiveStyleProfile(Role role, const GroupParams& params) const {
        return *STYLES[getEffectiveStyle(role, params)];
    }

    // ========================================
    // Generate all patterns in sequence (v0.16 mixed styles)
    // ========================================
    void generateAll(const GroupParams params[4]) {
        // Get effective styles for each role
        int styles[4];
        for (int r = 0; r < 4; r++) {
            styles[r] = getEffectiveStyle(static_cast<Role>(r), params[r]);
        }

        // 1. Timeline first (no reference)
        const StyleProfile& timelineStyle = *STYLES[styles[TIMELINE]];
        generateGroupWithStyle(TIMELINE, timelineStyle, params[0], styles[TIMELINE]);

        // 2. Foundation - interlock with Timeline using cross-style strength
        const StyleProfile& foundationStyle = *STYLES[styles[FOUNDATION]];
        float interlockStrength = StyleCompatibility::getInterlockStrength(
            styles[TIMELINE], styles[FOUNDATION]);

        // Apply interlock if either style requests it, or if styles are different
        bool needsInterlock = foundationStyle.avoidFoundationOnTimeline ||
                             (styles[TIMELINE] != styles[FOUNDATION]);

        if (needsInterlock) {
            generateGroupWithCrossStyleInterlock(FOUNDATION, foundationStyle, params[1],
                                                  patterns[TIMELINE][0], interlockStrength,
                                                  styles[FOUNDATION]);
        } else {
            generateGroupWithStyle(FOUNDATION, foundationStyle, params[1], styles[FOUNDATION]);
        }

        // 3. Groove complements Foundation
        const StyleProfile& grooveStyle = *STYLES[styles[GROOVE]];
        float grooveInterlock = StyleCompatibility::getInterlockStrength(
            styles[FOUNDATION], styles[GROOVE]);

        bool grooveNeedsInterlock = grooveStyle.grooveComplementsFoundation ||
                                    (styles[FOUNDATION] != styles[GROOVE]);

        if (grooveNeedsInterlock) {
            generateGroupWithCrossStyleInterlock(GROOVE, grooveStyle, params[2],
                                                  patterns[FOUNDATION][0], grooveInterlock,
                                                  styles[GROOVE]);
        } else {
            generateGroupWithStyle(GROOVE, grooveStyle, params[2], styles[GROOVE]);
        }

        // 4. Lead - optionally avoids Groove for Jazz-like styles
        const StyleProfile& leadStyle = *STYLES[styles[LEAD]];
        bool leadAvoidsGroove = (styles[LEAD] == 6);  // Jazz

        if (leadAvoidsGroove) {
            float leadInterlock = StyleCompatibility::getInterlockStrength(
                styles[GROOVE], styles[LEAD]);
            generateGroupWithCrossStyleInterlock(LEAD, leadStyle, params[3],
                                                  patterns[GROOVE][0], leadInterlock * 0.5f,
                                                  styles[LEAD]);
        } else {
            generateGroupWithStyle(LEAD, leadStyle, params[3], styles[LEAD]);
        }
    }

    // ========================================
    // Generate group with specific style (v0.16)
    // ========================================
    void generateGroupWithStyle(Role role, const StyleProfile& style,
                                const GroupParams& params, int effectiveStyleIndex) {
        int numVoices = voicesPerGroup[role];

        for (int v = 0; v < numVoices; v++) {
            // Kotekan for Gamelan groove
            if (role == GROOVE && effectiveStyleIndex == 5 && v < 2) {
                generateKotekan(params.length, params.density, v);
            } else {
                patterns[role][v] = generator.generate(role, style, params.length,
                                                       params.density, params.variation);
                generator.generateAccents(patterns[role][v], role, style);
                generator.applyRest(patterns[role][v], role, params.rest);

                if (role == GROOVE && v == 0) {
                    generator.addGhostNotes(patterns[role][v], style, 0.6f);
                }
            }
        }
    }

    // ========================================
    // Generate with cross-style interlock (v0.16)
    // ========================================
    void generateGroupWithCrossStyleInterlock(Role role, const StyleProfile& style,
                                               const GroupParams& params,
                                               const Pattern& reference,
                                               float interlockStrength,
                                               int effectiveStyleIndex) {
        int numVoices = voicesPerGroup[role];

        for (int v = 0; v < numVoices; v++) {
            // Use custom interlock with specified strength
            patterns[role][v] = generateWithVariableInterlock(
                role, style, params.length, params.density, params.variation,
                reference, interlockStrength);
            generator.generateAccents(patterns[role][v], role, style);
            generator.applyRest(patterns[role][v], role, params.rest);

            if (role == GROOVE && v == 0) {
                generator.addGhostNotes(patterns[role][v], style, 0.6f);
            }
        }
    }

    // ========================================
    // Variable strength interlock generation (v0.16)
    // ========================================
    Pattern generateWithVariableInterlock(Role role, const StyleProfile& style, int length,
                                          float density, float variation,
                                          const Pattern& reference, float interlockStrength) {
        Pattern p(length);
        std::vector<float> weights(length);
        std::uniform_real_distribution<float> velVar(-0.12f, 0.12f);

        const float* styleWeights;
        switch (role) {
            case TIMELINE:   styleWeights = style.timeline; break;
            case FOUNDATION: styleWeights = style.foundation; break;
            case GROOVE:     styleWeights = style.groove; break;
            case LEAD:       styleWeights = style.lead; break;
            default:         styleWeights = style.groove;
        }

        for (int i = 0; i < length; i++) {
            int mapped = (i * 16) / length;
            weights[i] = styleWeights[mapped];
            weights[i] = weights[i] * (1.0f - variation) + variation;

            // Apply interlock with variable strength
            if (reference.hasOnsetAt(i)) {
                weights[i] *= (1.0f - interlockStrength * 0.8f);
            }

            // Complement boost for adjacent positions
            int prev = (i - 1 + length) % length;
            int next = (i + 1) % length;
            if (reference.hasOnsetAt(prev) || reference.hasOnsetAt(next)) {
                weights[i] *= (1.0f + interlockStrength * 0.5f);
            }
        }

        // Weighted selection
        int targetOnsets = std::max(1, static_cast<int>(std::round(length * density)));

        for (int n = 0; n < targetOnsets; n++) {
            float available = 0.0f;
            for (int i = 0; i < length; i++) {
                if (!p.hasOnsetAt(i)) available += weights[i];
            }
            if (available <= 0.0f) break;

            std::uniform_real_distribution<float> dist(0.0f, available);
            float r = dist(humanRng);

            float cumulative = 0.0f;
            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) continue;
                cumulative += weights[i];
                if (r <= cumulative) {
                    float baseVel = 0.25f + weights[i] * 0.5f;
                    if ((i % (length / 4)) == 0) baseVel += 0.2f;
                    float velocity = std::clamp(baseVel + velVar(humanRng), 0.2f, 1.0f);
                    p.setOnset(i, velocity);
                    break;
                }
            }
        }

        return p;
    }

    // ========================================
    // Generate single group (legacy, uses global style)
    // ========================================
    void generateGroup(Role role, const StyleProfile& style, const GroupParams& params) {
        generateGroupWithStyle(role, style, params, styleIndex);
    }

    // ========================================
    // Generate with interlock reference (legacy)
    // ========================================
    void generateGroupWithInterlock(Role role, const StyleProfile& style,
                                     const GroupParams& params, const Pattern& reference) {
        generateGroupWithCrossStyleInterlock(role, style, params, reference, 0.5f, styleIndex);
    }

    // ========================================
    // Per-role style setters (v0.16)
    // ========================================
    void setRoleStyle(Role role, int style) {
        roleStyles[role] = std::clamp(style, -1, NUM_STYLES - 1);
    }

    int getRoleStyle(Role role) const {
        return roleStyles[role];
    }

    void clearRoleStyles() {
        for (int i = 0; i < 4; i++) {
            roleStyles[i] = -1;
        }
    }

    // Get compatibility between current role styles
    float getStyleCompatibility(Role roleA, Role roleB) const {
        GroupParams defaultParams;
        int styleA = getEffectiveStyle(roleA, defaultParams);
        int styleB = getEffectiveStyle(roleB, defaultParams);
        return StyleCompatibility::getCompatibility(styleA, styleB);
    }

    // Check if all roles are using compatible styles
    bool areStylesCompatible(float threshold = 0.5f) const {
        GroupParams defaultParams;
        for (int a = 0; a < 4; a++) {
            for (int b = a + 1; b < 4; b++) {
                float compat = getStyleCompatibility(static_cast<Role>(a), static_cast<Role>(b));
                if (compat < threshold) return false;
            }
        }
        return true;
    }

    // Get dominant swing for mixed styles (use Timeline's family)
    float getMixedSwing(float bpm) const {
        GroupParams defaultParams;
        int timelineStyle = getEffectiveStyle(TIMELINE, defaultParams);
        return getEffectiveSwingForStyle(bpm, timelineStyle);
    }

    float getEffectiveSwingForStyle(float bpm, int style) const {
        float baseSwing = STYLES[style]->swing;

        float tempoFactor;
        if (bpm < 100.0f) {
            tempoFactor = 1.0f + (100.0f - bpm) / 200.0f;
        } else if (bpm > 150.0f) {
            tempoFactor = 1.0f - (bpm - 150.0f) / 200.0f;
        } else {
            tempoFactor = 1.0f;
        }

        float adjusted = 0.5f + (baseSwing - 0.5f) * tempoFactor;
        return std::clamp(adjusted, 0.5f, 0.75f);
    }

    // ========================================
    // Kotekan generation (polos/sangsih pair)
    // ========================================
    void generateKotekan(int length, float density, int voice) {
        Pattern& p = patterns[GROOVE][voice];
        p = Pattern(length);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);
        std::mt19937 localRng(std::random_device{}());

        for (int i = 0; i < length; i++) {
            bool isEven = (i % 2 == 0);
            // Polos: even positions, Sangsih: odd positions
            if ((voice == 0 && isEven) || (voice == 1 && !isEven)) {
                // Apply density
                if (static_cast<float>(rand()) / RAND_MAX < density) {
                    float vel = 0.7f + velVar(localRng);
                    p.setOnset(i, vel);
                }
            }
        }
    }

    // ========================================
    // Accessors
    // ========================================
    void setStyle(int index) {
        styleIndex = std::clamp(index, 0, NUM_STYLES - 1);
    }

    int getStyle() const {
        return styleIndex;
    }

    const StyleProfile& getCurrentStyle() const {
        return *STYLES[styleIndex];
    }

    bool getOnset(Role role, int voice, int step) const {
        return patterns[role][voice].hasOnsetAt(step);
    }

    float getVelocity(Role role, int voice, int step) const {
        return patterns[role][voice].getVelocity(step);
    }

    bool getAccent(Role role, int voice, int step) const {
        int s = step % patterns[role][voice].length;
        return patterns[role][voice].accents[s];
    }

    int getLength(Role role) const {
        return patterns[role][0].length;
    }

    void seed(unsigned int s) {
        generator.seed(s);
        fillGen.seed(s + 1);
        crEngine.seed(s + 2);
        humanRng.seed(s + 3);
    }

    // ========================================
    // CV Input Adaptation
    // ========================================
    void processCVInput(float voltage, float velocity = 1.0f) {
        cvAnalyzer.process(voltage, velocity);
    }

    void setCVAdaptAmount(float amount) {
        cvAdaptAmount = std::clamp(amount, 0.0f, 1.0f);
    }

    float getCVAdaptAmount() const { return cvAdaptAmount; }

    // Get adapted weights for pattern generation
    std::vector<float> getAdaptedWeights(Role role) const {
        if (cvAdaptAmount <= 0.0f) {
            return std::vector<float>();  // No adaptation
        }

        const StyleProfile& style = *STYLES[styleIndex];
        const float* styleWeights;

        switch (role) {
            case TIMELINE:   styleWeights = style.timeline; break;
            case FOUNDATION: styleWeights = style.foundation; break;
            case GROOVE:     styleWeights = style.groove; break;
            case LEAD:       styleWeights = style.lead; break;
            default:         styleWeights = style.groove;
        }

        return cvAnalyzer.blendWithStyle(styleWeights, cvAdaptAmount);
    }

    // Get complementary weights (play in gaps)
    std::vector<float> getComplementWeights() const {
        return cvAnalyzer.getComplementWeights();
    }

    float getCVDensity() const {
        return cvAnalyzer.getDetectedDensity();
    }

    int getCVPeriod() const {
        return cvAnalyzer.getDetectedPeriod();
    }

    void resetCVAnalyzer() {
        cvAnalyzer.reset();
    }

    // ========================================
    // Swing calculation with BPM scaling
    // ========================================
    // Research: swing ratio decreases at higher tempos
    // Slow (<100): up to 3.5:1 (0.78 ratio)
    // Medium (100-150): 2:1 (0.67 ratio)
    // Fast (>150): nearly straight (0.52-0.55 ratio)
    float getEffectiveSwing(float bpm) const {
        float baseSwing = STYLES[styleIndex]->swing;

        // Scale swing based on tempo
        float tempoFactor;
        if (bpm < 100.0f) {
            // Slow: amplify swing
            tempoFactor = 1.0f + (100.0f - bpm) / 200.0f;
        } else if (bpm > 150.0f) {
            // Fast: reduce swing toward straight
            tempoFactor = 1.0f - (bpm - 150.0f) / 200.0f;
        } else {
            tempoFactor = 1.0f;
        }

        // Apply tempo factor but keep within bounds
        float adjusted = 0.5f + (baseSwing - 0.5f) * tempoFactor;
        return std::clamp(adjusted, 0.5f, 0.75f);
    }

    // ========================================
    // Microtiming offset for a step
    // Returns offset in milliseconds
    // ========================================
    float getMicrotiming(int step, Role role, float amount) const {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        // Base offset range by role (from research)
        float rangeMs;
        switch (role) {
            case TIMELINE: rangeMs = 3.0f; break;   // Most stable
            case FOUNDATION: rangeMs = 5.0f; break; // Bass slightly behind
            case GROOVE: rangeMs = 10.0f; break;    // More variation
            case LEAD: rangeMs = 15.0f; break;      // Most expressive
            default: rangeMs = 5.0f;
        }

        // Scale by amount parameter
        float offset = dist(humanRng) * rangeMs * amount;

        // Foundation tends to play slightly behind
        if (role == FOUNDATION) {
            offset += 3.0f * amount;
        }

        return offset;
    }

    // ========================================
    // Phrase-aware velocity modifier
    // Applies crescendo toward phrase end
    // ========================================
    float getPhraseVelocityMod(int bar, int step, int phraseLength) const {
        // Position within phrase (0.0 to 1.0)
        int barInPhrase = bar % phraseLength;
        float barProgress = static_cast<float>(barInPhrase) / phraseLength;
        float stepProgress = static_cast<float>(step) / 16.0f;
        float totalProgress = barProgress + stepProgress / phraseLength;

        // Crescendo toward phrase end
        // Last bar of phrase: +5-15% velocity
        if (barInPhrase == phraseLength - 1) {
            return 1.0f + stepProgress * 0.15f;
        }

        // Slight build throughout phrase
        return 1.0f + totalProgress * 0.05f;
    }

    // ========================================
    // Check if position is phrase boundary
    // ========================================
    bool isPhraseEnd(int bar, int step, int phraseLength) const {
        return (bar % phraseLength == phraseLength - 1) && (step >= 12);
    }

    // ========================================
    // Generate multi-bar sequence with fills and call-response
    // Uses base pattern repetition for musical coherence
    // ========================================
    void generateBars(const GroupParams params[4], int totalBars) {
        numBars = totalBars;
        bars.clear();
        bars.resize(numBars);

        const StyleProfile& style = *STYLES[styleIndex];

        // First pass: determine fill and call-response positions
        for (int bar = 0; bar < numBars; bar++) {
            BarPattern& bp = bars[bar];

            // Check for fill
            bp.hasFill = fillGen.shouldFill(bar + 1, params[0].fillProbability);

            if (bp.hasFill) {
                int fillBeats = fillGen.getFillLengthBeats(params[0].fillIntensity);
                bp.fillLengthSteps = fillBeats * 4;
                bp.fillStartStep = params[0].length - bp.fillLengthSteps;
                if (bp.fillStartStep < 0) bp.fillStartStep = 0;
                bp.fillType = fillGen.selectFillType(styleIndex, GROOVE);
            }

            // Check for call-response (not on fill bars, not two in a row)
            bp.hasCallResponse = false;
            if (!bp.hasFill && crEngine.styleUsesCallResponse(styleIndex)) {
                bool prevHadCR = (bar > 0) && bars[bar - 1].hasCallResponse;
                if (!prevHadCR && crEngine.shouldCall(bar + 1, styleIndex, params[0].callResponseProbability)) {
                    bp.hasCallResponse = true;
                    bp.callResponse = crEngine.generatePair(styleIndex, bar, params[0].length,
                                                            params[0].fillIntensity);
                }
            }
        }

        // Generate ONE base pattern for the entire sequence
        Pattern basePattern[4][3];

        for (int role = 0; role < 4; role++) {
            Role r = static_cast<Role>(role);
            int numVoices = voicesPerGroup[role];

            for (int v = 0; v < numVoices; v++) {
                if (role == GROOVE && styleIndex == 5 && v < 2) {
                    // Kotekan for Gamelan
                    basePattern[role][v] = Pattern(params[role].length);
                    std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);
                    for (int i = 0; i < params[role].length; i++) {
                        bool isEven = (i % 2 == 0);
                        if ((v == 0 && isEven) || (v == 1 && !isEven)) {
                            if (static_cast<float>(rand()) / RAND_MAX < params[role].density) {
                                float vel = 0.7f + velVar(humanRng);
                                basePattern[role][v].setOnset(i, vel);
                            }
                        }
                    }
                } else if (role == FOUNDATION && style.avoidFoundationOnTimeline) {
                    basePattern[role][v] = generator.generateWithInterlock(
                        r, style, params[role].length, params[role].density,
                        params[role].variation, basePattern[TIMELINE][0]);
                } else if (role == GROOVE && style.grooveComplementsFoundation) {
                    basePattern[role][v] = generator.generateWithInterlock(
                        r, style, params[role].length, params[role].density,
                        params[role].variation, basePattern[FOUNDATION][0]);
                } else {
                    basePattern[role][v] = generator.generate(
                        r, style, params[role].length, params[role].density, params[role].variation);
                }

                generator.generateAccents(basePattern[role][v], r, style);
                generator.applyRest(basePattern[role][v], r, params[role].rest);

                if (role == GROOVE && v == 0) {
                    generator.addGhostNotes(basePattern[role][v], style, 0.6f);
                }
            }
        }

        // Second pass: copy base pattern to each bar with ornamentation
        for (int bar = 0; bar < numBars; bar++) {
            BarPattern& bp = bars[bar];

            for (int role = 0; role < 4; role++) {
                Role r = static_cast<Role>(role);
                int numVoices = voicesPerGroup[role];

                for (int v = 0; v < numVoices; v++) {
                    // Copy base pattern
                    bp.patterns[role][v] = basePattern[role][v];

                    // Apply ornamentation to EVERY bar (including first)
                    if (!bp.hasFill) {
                        applyOrnamentation(bp.patterns[role][v], r, bar, params[role].variation);
                    }

                    // Apply fill if needed
                    if (bp.hasFill && fillGen.shouldRoleFill(r, bp.fillType)) {
                        applyFillToPattern(bp.patterns[role][v], r, bp, params[role].fillIntensity);
                    }
                }
            }

            // Apply call-response after base patterns are set
            if (bp.hasCallResponse) {
                applyCallResponse(bp, params);
            }
        }
    }

    // ========================================
    // Apply call-response to bar patterns
    // ========================================
    void applyCallResponse(BarPattern& bp, const GroupParams params[4]) {
        const CallResponseProfile& profile = crEngine.getProfile(styleIndex);

        // Apply call to caller role
        int callerRole = bp.callResponse.callerRole;
        for (int v = 0; v < voicesPerGroup[callerRole]; v++) {
            crEngine.applyCallToPattern(bp.patterns[callerRole][v], bp.callResponse.call);
        }

        // Apply response (single or group)
        crEngine.applyGroupResponse(bp.patterns, bp.callResponse.response,
                                    styleIndex, voicesPerGroup);
    }

    // ========================================
    // Apply ornamentation to pattern
    // Each bar gets unique decorations while keeping base intact
    // ========================================
    void applyOrnamentation(Pattern& p, Role role, int barIndex, float amount) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Use barIndex to seed variation (different ornaments each bar)
        unsigned int localSeed = barIndex * 31 + static_cast<int>(role) * 7;

        for (int i = 0; i < p.length; i++) {
            // Deterministic but varied per bar/position
            float hash = std::sin(localSeed + i * 0.7f) * 0.5f + 0.5f;

            // Don't touch skeleton beats for Foundation
            if (role == FOUNDATION && (i == 0 || i == 8)) {
                // Only slight velocity variation on skeleton
                if (p.hasOnsetAt(i)) {
                    float vel = p.getVelocity(i) + velVar(humanRng) * 0.5f;
                    p.setOnset(i, std::clamp(vel, 0.7f, 1.0f));
                }
                continue;
            }

            if (p.hasOnsetAt(i)) {
                // Velocity variation on existing hits
                float vel = p.getVelocity(i) + velVar(humanRng);
                p.setOnset(i, std::clamp(vel, 0.12f, 1.0f));

                // Occasional ghost: turn a normal hit quieter
                if (hash < amount * 0.2f && vel > 0.4f && !p.accents[i]) {
                    p.setOnset(i, 0.2f + dist(humanRng) * 0.15f);
                }
            } else {
                // Add ornamental ghost notes
                float ghostProb = amount * 0.25f;

                // Higher probability for Groove, lower for Timeline
                if (role == GROOVE) ghostProb *= 1.5f;
                if (role == TIMELINE) ghostProb *= 0.5f;
                if (role == FOUNDATION) ghostProb *= 0.3f;

                if (hash < ghostProb) {
                    // Ghost note: very soft
                    float ghostVel = 0.12f + dist(humanRng) * 0.12f;
                    p.setOnset(i, ghostVel);
                }
            }
        }
    }

    // ========================================
    // Apply fill to pattern
    // ========================================
    void applyFillToPattern(Pattern& p, Role role, const BarPattern& bp, float intensity) {
        float roleIntensity = fillGen.getRoleFillIntensity(role, intensity);
        std::vector<float> fillPattern = fillGen.generateFillPattern(
            bp.fillType, bp.fillLengthSteps, roleIntensity);

        // Replace pattern from fillStartStep onwards
        for (int i = 0; i < bp.fillLengthSteps && (bp.fillStartStep + i) < p.length; i++) {
            int pos = bp.fillStartStep + i;
            float vel = fillPattern[i];

            if (vel > 0.0f) {
                p.setOnset(pos, vel);
                p.accents[pos] = true;
            } else {
                p.clearOnset(pos);
            }
        }
    }

    // ========================================
    // Multi-bar accessors
    // ========================================
    bool getBarOnset(int bar, Role role, int voice, int step) const {
        if (bar < 0 || bar >= numBars) return false;
        return bars[bar].patterns[role][voice].hasOnsetAt(step);
    }

    float getBarVelocity(int bar, Role role, int voice, int step) const {
        if (bar < 0 || bar >= numBars) return 0.0f;
        return bars[bar].patterns[role][voice].getVelocity(step);
    }

    bool getBarAccent(int bar, Role role, int voice, int step) const {
        if (bar < 0 || bar >= numBars) return false;
        int s = step % bars[bar].patterns[role][voice].length;
        return bars[bar].patterns[role][voice].accents[s];
    }

    bool barHasFill(int bar) const {
        if (bar < 0 || bar >= numBars) return false;
        return bars[bar].hasFill;
    }

    FillType getBarFillType(int bar) const {
        if (bar < 0 || bar >= numBars) return FillType::NONE;
        return bars[bar].fillType;
    }

    int getNumBars() const { return numBars; }

    // ========================================
    // Call-Response accessors
    // ========================================
    bool barHasCallResponse(int bar) const {
        if (bar < 0 || bar >= numBars) return false;
        return bars[bar].hasCallResponse;
    }

    const CallResponsePair& getBarCallResponse(int bar) const {
        static CallResponsePair empty;
        if (bar < 0 || bar >= numBars) return empty;
        return bars[bar].callResponse;
    }

    bool styleUsesCallResponse() const {
        return crEngine.styleUsesCallResponse(styleIndex);
    }

    const CallResponseProfile& getCallResponseProfile() const {
        return crEngine.getProfile(styleIndex);
    }
};

} // namespace WorldRhythm
