#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Batucada Engine - Brazilian Samba Percussion
// ========================================
// Based on unified_rhythm_analysis.md Brazilian section
//
// Batucada: Large ensemble samba percussion from Rio de Janeiro
// Characterized by interlocking Surdo patterns and dense texture
//
// Key instruments:
// - Surdo (3 sizes: primeira, segunda, terceira)
// - Caixa (snare drum)
// - Repinique (lead/call drum)
// - Tamborim
// - Agogô (double bell)
// - Ganzá/Chocalho (shakers)
// - Cuíca (friction drum)

// ========================================
// Instrument Types
// ========================================

enum class BatucadaInstrument {
    SURDO_PRIMEIRA = 0,  // Lowest, beat 2
    SURDO_SEGUNDA,       // Middle, beat 1 (answer)
    SURDO_TERCEIRA,      // Highest, fills/variations
    CAIXA,               // Snare, continuous 16ths
    REPINIQUE,           // Lead drum, calls/breaks
    TAMBORIM,            // Small frame drum, timeline
    AGOGO,               // Double bell, timeline
    GANZA,               // Shaker, continuous texture
    CUICA,               // Friction drum, melodic accent
    NUM_INSTRUMENTS
};

// ========================================
// Surdo Type Definition
// ========================================

enum class SurdoType {
    PRIMEIRA = 0,   // "First" - lowest, plays on beat 2
    SEGUNDA,        // "Second" - middle, plays on beat 1
    TERCEIRA,       // "Third" - highest, plays variations
    NUM_TYPES
};

struct SurdoDefinition {
    SurdoType type;
    std::string name;
    std::string description;
    int pitchOffset;        // Relative pitch (0 = lowest)
    float defaultVelocity;
    std::vector<int> basePattern;    // 16-step pattern
    std::vector<float> velocities;
    bool allowsVariation;
};

inline SurdoDefinition createSurdoPrimeiraDef() {
    SurdoDefinition s;
    s.type = SurdoType::PRIMEIRA;
    s.name = "Surdo Primeira";
    s.description = "Lowest surdo, anchors beat 2";
    s.pitchOffset = 0;
    s.defaultVelocity = 1.0f;
    // Classic primeira: strong hit on beat 2, sometimes ghost on 4
    // Position: 1 e & a 2 e & a 3 e & a 4 e & a
    s.basePattern = {4};  // Beat 2 only (position 4 in 16th grid)
    s.velocities = {1.0f};
    s.allowsVariation = false;  // Primeira stays solid
    return s;
}

inline SurdoDefinition createSurdoSegundaDef() {
    SurdoDefinition s;
    s.type = SurdoType::SEGUNDA;
    s.name = "Surdo Segunda";
    s.description = "Middle surdo, answers on beat 1";
    s.pitchOffset = 5;  // ~perfect 4th higher
    s.defaultVelocity = 0.9f;
    // Segunda answers primeira: hit on beat 1
    s.basePattern = {0};  // Beat 1
    s.velocities = {0.9f};
    s.allowsVariation = false;
    return s;
}

inline SurdoDefinition createSurdoTerceiraDef() {
    SurdoDefinition s;
    s.type = SurdoType::TERCEIRA;
    s.name = "Surdo Terceira";
    s.description = "Highest surdo, plays variations";
    s.pitchOffset = 7;  // ~perfect 5th higher
    s.defaultVelocity = 0.85f;
    // Terceira fills between primeira and segunda
    s.basePattern = {2, 6, 10, 14};  // Offbeats
    s.velocities = {0.7f, 0.85f, 0.7f, 0.85f};
    s.allowsVariation = true;  // Terceira has freedom
    return s;
}

// ========================================
// Batucada Pattern Definitions
// ========================================

struct BatucadaPattern {
    BatucadaInstrument instrument;
    std::string name;
    std::vector<int> pattern;
    std::vector<float> velocities;
    bool isTimeline;
};

inline BatucadaPattern createCaixaPattern() {
    BatucadaPattern p;
    p.instrument = BatucadaInstrument::CAIXA;
    p.name = "Caixa";
    // Continuous 16th notes with accents
    p.pattern = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    p.velocities = {0.9f, 0.4f, 0.6f, 0.4f, 0.9f, 0.4f, 0.6f, 0.4f,
                    0.9f, 0.4f, 0.6f, 0.4f, 0.9f, 0.4f, 0.6f, 0.4f};
    p.isTimeline = false;
    return p;
}

inline BatucadaPattern createTamborimPattern() {
    BatucadaPattern p;
    p.instrument = BatucadaInstrument::TAMBORIM;
    p.name = "Tamborim";
    // Classic teleco-teco pattern
    p.pattern = {0, 3, 4, 6, 8, 11, 12, 14};
    p.velocities = {0.9f, 0.7f, 0.9f, 0.7f, 0.9f, 0.7f, 0.9f, 0.7f};
    p.isTimeline = true;
    return p;
}

inline BatucadaPattern createAgogoPattern() {
    BatucadaPattern p;
    p.instrument = BatucadaInstrument::AGOGO;
    p.name = "Agogo";
    // Partido alto pattern
    p.pattern = {0, 3, 4, 7, 8, 10, 12};
    p.velocities = {0.9f, 0.6f, 0.9f, 0.7f, 0.9f, 0.6f, 0.8f};
    p.isTimeline = true;
    return p;
}

inline BatucadaPattern createGanzaPattern() {
    BatucadaPattern p;
    p.instrument = BatucadaInstrument::GANZA;
    p.name = "Ganza";
    // Continuous 16ths, softer accents
    p.pattern = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    p.velocities = {0.6f, 0.4f, 0.5f, 0.4f, 0.6f, 0.4f, 0.5f, 0.4f,
                    0.6f, 0.4f, 0.5f, 0.4f, 0.6f, 0.4f, 0.5f, 0.4f};
    p.isTimeline = false;
    return p;
}

inline BatucadaPattern createRepiniquePattern() {
    BatucadaPattern p;
    p.instrument = BatucadaInstrument::REPINIQUE;
    p.name = "Repinique";
    // Call pattern (can vary)
    p.pattern = {0, 2, 4, 7, 8, 10, 12, 14};
    p.velocities = {1.0f, 0.7f, 0.9f, 0.8f, 1.0f, 0.7f, 0.9f, 0.7f};
    p.isTimeline = false;
    return p;
}

// ========================================
// Samba Style Variations
// ========================================

enum class SambaStyle {
    SAMBA_ENREDO = 0,    // Escola de samba parade style
    SAMBA_REGGAE,        // Salvador Bahia style
    PARTIDO_ALTO,        // Pagode/roda de samba
    SAMBA_FUNK,          // Modern fusion
    BOSSA_NOVA,          // Quiet, brushes
    NUM_STYLES
};

struct SambaStyleProfile {
    SambaStyle style;
    std::string name;
    float surdoDensity;      // How much terceira fills
    float caixaIntensity;    // Caixa ghost note level
    float swing;             // Swing amount (50-60%)
    bool useTerceira;
    bool useRepiniqueCalls;
};

inline SambaStyleProfile createSambaEnredo() {
    return {SambaStyle::SAMBA_ENREDO, "Samba Enredo", 0.6f, 0.9f, 0.55f, true, true};
}

inline SambaStyleProfile createSambaReggae() {
    return {SambaStyle::SAMBA_REGGAE, "Samba Reggae", 0.7f, 0.7f, 0.52f, true, true};
}

inline SambaStyleProfile createPartidoAltoStyle() {
    return {SambaStyle::PARTIDO_ALTO, "Partido Alto", 0.4f, 0.6f, 0.58f, false, false};
}

inline SambaStyleProfile createSambaFunk() {
    return {SambaStyle::SAMBA_FUNK, "Samba Funk", 0.8f, 1.0f, 0.50f, true, true};
}

inline SambaStyleProfile createBossaNova() {
    return {SambaStyle::BOSSA_NOVA, "Bossa Nova", 0.2f, 0.3f, 0.55f, false, false};
}

// ========================================
// Batucada Result Structure
// ========================================

struct BatucadaResult {
    Pattern surdoPrimeira;
    Pattern surdoSegunda;
    Pattern surdoTerceira;
    Pattern caixa;
    Pattern repinique;
    Pattern tamborim;
    Pattern agogo;
    Pattern ganza;

    // Combined patterns for different voice counts
    Pattern combinedLow;     // Surdos combined
    Pattern combinedMid;     // Caixa + Repinique
    Pattern combinedHigh;    // Tamborim + Agogo + Ganza
    Pattern combinedAll;
};

// ========================================
// Batucada Engine
// ========================================

class BatucadaEngine {
public:
    BatucadaEngine() : currentStyle(SambaStyle::SAMBA_ENREDO), gen(std::random_device{}()) {
        surdoDefs.push_back(createSurdoPrimeiraDef());
        surdoDefs.push_back(createSurdoSegundaDef());
        surdoDefs.push_back(createSurdoTerceiraDef());

        styleProfiles.push_back(createSambaEnredo());
        styleProfiles.push_back(createSambaReggae());
        styleProfiles.push_back(createPartidoAltoStyle());
        styleProfiles.push_back(createSambaFunk());
        styleProfiles.push_back(createBossaNova());
    }

    // ========================================
    // Style Selection
    // ========================================

    void setStyle(SambaStyle style) {
        currentStyle = style;
    }

    void setStyleByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(SambaStyle::NUM_STYLES)) {
            currentStyle = static_cast<SambaStyle>(index);
        }
    }

    SambaStyle getStyle() const { return currentStyle; }

    const SambaStyleProfile& getCurrentProfile() const {
        return styleProfiles[static_cast<int>(currentStyle)];
    }

    std::string getCurrentStyleName() const {
        return styleProfiles[static_cast<int>(currentStyle)].name;
    }

    // ========================================
    // Individual Instrument Generation
    // ========================================

    Pattern generateSurdoPrimeira(int length, float velocity) {
        const auto& def = surdoDefs[0];
        return generateFromDefinition(def, length, velocity);
    }

    Pattern generateSurdoSegunda(int length, float velocity) {
        const auto& def = surdoDefs[1];
        return generateFromDefinition(def, length, velocity);
    }

    Pattern generateSurdoTerceira(int length, float velocity, float variation = 0.5f) {
        const auto& def = surdoDefs[2];
        const auto& profile = getCurrentProfile();

        Pattern p = generateFromDefinition(def, length, velocity * 0.85f);

        if (profile.useTerceira && variation > 0.0f) {
            // Add variation fills based on style
            addTerceiraVariation(p, variation * profile.surdoDensity);
        }

        return p;
    }

    Pattern generateCaixa(int length, float velocity) {
        const auto& profile = getCurrentProfile();
        auto def = createCaixaPattern();

        Pattern p(length);
        for (size_t i = 0; i < def.pattern.size() && i < def.velocities.size(); i++) {
            int pos = def.pattern[i];
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                float v = velocity * def.velocities[i] * profile.caixaIntensity;
                p.setOnset(mappedPos, v);
            }
        }

        return p;
    }

    Pattern generateTamborim(int length, float velocity) {
        auto def = createTamborimPattern();
        return generateFromBatucadaPattern(def, length, velocity);
    }

    Pattern generateAgogo(int length, float velocity) {
        auto def = createAgogoPattern();
        return generateFromBatucadaPattern(def, length, velocity);
    }

    Pattern generateGanza(int length, float velocity) {
        auto def = createGanzaPattern();
        return generateFromBatucadaPattern(def, length, velocity * 0.6f);
    }

    Pattern generateRepinique(int length, float velocity, bool includeCall = false) {
        auto def = createRepiniquePattern();
        Pattern p = generateFromBatucadaPattern(def, length, velocity);

        if (includeCall && getCurrentProfile().useRepiniqueCalls) {
            addRepiniqueCall(p, velocity);
        }

        return p;
    }

    // ========================================
    // Complete Batucada Generation
    // ========================================

    BatucadaResult generateComplete(int length, float velocity, float variation = 0.5f) {
        BatucadaResult result;

        result.surdoPrimeira = generateSurdoPrimeira(length, velocity);
        result.surdoSegunda = generateSurdoSegunda(length, velocity);
        result.surdoTerceira = generateSurdoTerceira(length, velocity, variation);
        result.caixa = generateCaixa(length, velocity);
        result.repinique = generateRepinique(length, velocity, variation > 0.7f);
        result.tamborim = generateTamborim(length, velocity);
        result.agogo = generateAgogo(length, velocity);
        result.ganza = generateGanza(length, velocity);

        // Generate combined patterns
        result.combinedLow = combinePatterns({result.surdoPrimeira, result.surdoSegunda, result.surdoTerceira});
        result.combinedMid = combinePatterns({result.caixa, result.repinique});
        result.combinedHigh = combinePatterns({result.tamborim, result.agogo, result.ganza});
        result.combinedAll = combinePatterns({result.combinedLow, result.combinedMid, result.combinedHigh});

        return result;
    }

    // ========================================
    // Surdo Interlock Pattern
    // ========================================

    // Generate the classic primeira + segunda interlock
    std::pair<Pattern, Pattern> generateSurdoInterlock(int length, float velocity) {
        Pattern primeira = generateSurdoPrimeira(length, velocity);
        Pattern segunda = generateSurdoSegunda(length, velocity);

        // Ensure they don't overlap (shouldn't by definition, but verify)
        for (int i = 0; i < length; i++) {
            if (primeira.hasOnsetAt(i) && segunda.hasOnsetAt(i)) {
                // Primeira takes priority
                segunda.setOnset(i, 0.0f);
            }
        }

        return {primeira, segunda};
    }

    // Generate all three surdos interlocking
    std::tuple<Pattern, Pattern, Pattern> generateTripleSurdo(int length, float velocity, float variation = 0.5f) {
        Pattern primeira = generateSurdoPrimeira(length, velocity);
        Pattern segunda = generateSurdoSegunda(length, velocity * 0.95f);
        Pattern terceira = generateSurdoTerceira(length, velocity * 0.85f, variation);

        // Resolve overlaps: primeira > segunda > terceira
        for (int i = 0; i < length; i++) {
            if (primeira.hasOnsetAt(i)) {
                segunda.setOnset(i, 0.0f);
                terceira.setOnset(i, 0.0f);
            } else if (segunda.hasOnsetAt(i)) {
                terceira.setOnset(i, 0.0f);
            }
        }

        return {primeira, segunda, terceira};
    }

    // ========================================
    // Swing Application
    // ========================================

    float getSwingAmount() const {
        return getCurrentProfile().swing;
    }

    // ========================================
    // Repinique Calls
    // ========================================

    Pattern generateRepiniqueCall(int length, float velocity) {
        Pattern p(length);

        // Classic call patterns
        std::vector<std::vector<int>> callPatterns = {
            {0, 2, 4, 6, 7},           // Run up
            {0, 3, 4, 7, 8, 11, 12},   // Syncopated
            {0, 4, 6, 8, 10, 12, 14},  // Steady build
            {0, 2, 4, 8, 10, 12, 14, 15} // Dense call
        };

        std::uniform_int_distribution<int> dist(0, static_cast<int>(callPatterns.size()) - 1);
        const auto& pattern = callPatterns[dist(gen)];

        for (int pos : pattern) {
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                float v = velocity * (0.8f + 0.2f * (static_cast<float>(pos) / 16.0f));
                p.setOnset(mappedPos, v);
                p.accents[mappedPos] = true;
            }
        }

        return p;
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getInstrumentName(BatucadaInstrument inst) {
        switch (inst) {
            case BatucadaInstrument::SURDO_PRIMEIRA: return "Surdo Primeira";
            case BatucadaInstrument::SURDO_SEGUNDA: return "Surdo Segunda";
            case BatucadaInstrument::SURDO_TERCEIRA: return "Surdo Terceira";
            case BatucadaInstrument::CAIXA: return "Caixa";
            case BatucadaInstrument::REPINIQUE: return "Repinique";
            case BatucadaInstrument::TAMBORIM: return "Tamborim";
            case BatucadaInstrument::AGOGO: return "Agogo";
            case BatucadaInstrument::GANZA: return "Ganza";
            case BatucadaInstrument::CUICA: return "Cuica";
            default: return "Unknown";
        }
    }

    int getNumStyles() const {
        return static_cast<int>(SambaStyle::NUM_STYLES);
    }

private:
    SambaStyle currentStyle;
    std::vector<SurdoDefinition> surdoDefs;
    std::vector<SambaStyleProfile> styleProfiles;
    std::mt19937 gen;

    Pattern generateFromDefinition(const SurdoDefinition& def, int length, float velocity) {
        Pattern p(length);

        for (size_t i = 0; i < def.basePattern.size() && i < def.velocities.size(); i++) {
            int pos = def.basePattern[i];
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                p.setOnset(mappedPos, velocity * def.velocities[i]);
            }
        }

        return p;
    }

    Pattern generateFromBatucadaPattern(const BatucadaPattern& bp, int length, float velocity) {
        Pattern p(length);

        for (size_t i = 0; i < bp.pattern.size() && i < bp.velocities.size(); i++) {
            int pos = bp.pattern[i];
            int mappedPos = (pos * length) / 16;
            if (mappedPos < length) {
                p.setOnset(mappedPos, velocity * bp.velocities[i]);
            }
        }

        return p;
    }

    void addTerceiraVariation(Pattern& p, float amount) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Add fills on weak beats
        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) {
                bool isOffbeat = ((i * 16 / p.length) % 4) != 0;
                if (isOffbeat && dist(gen) < amount * 0.3f) {
                    p.setOnset(i, 0.5f + dist(gen) * 0.3f);
                }
            }
        }
    }

    void addRepiniqueCall(Pattern& p, float velocity) {
        // Add call accent at end of pattern
        int callStart = std::max(0, p.length - 4);
        for (int i = callStart; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) {
                p.setOnset(i, velocity * 0.8f);
                p.accents[i] = true;
            }
        }
    }

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
