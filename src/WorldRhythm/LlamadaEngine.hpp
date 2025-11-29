#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Llamada Engine - Afro-Cuban Call Signals
// ========================================
// Based on fills_ornaments_research.md Section 2.2
//
// Llamada: Signal phrase to change sections in Afro-Cuban music
// - All instruments respond together (similar to West African break)
// - Used to transition between sections (montuno, mambo, etc.)
// - Typically 2-4 beat phrases that align with clave
//
// Types:
// - Standard Llamada: Simple call-response
// - Montuno Entry: Signal to enter montuno section
// - Mambo Call: Signal for mambo break
// - Coro Entry: Signal for chorus entry
// - Diablo: Intense climactic call

enum class LlamadaType {
    STANDARD = 0,       // Basic transition call
    MONTUNO_ENTRY,      // Entry to montuno section
    MAMBO_CALL,         // Mambo break signal
    CORO_ENTRY,         // Chorus entry signal
    DIABLO,             // Climactic "devil" call
    CIERRE,             // Closing/ending signal
    NUM_TYPES
};

struct LlamadaDefinition {
    LlamadaType type;
    std::string name;
    std::string description;
    int durationBeats;          // Typical duration in beats
    bool requiresClaveAlign;    // Must align with clave
    float intensity;            // 0-1, affects velocity
    std::vector<int> callPattern;    // Call rhythm (16th note positions in 16-grid)
    std::vector<int> responsePattern; // Response rhythm
};

// ========================================
// Llamada Type Definitions
// ========================================

inline LlamadaDefinition createStandardLlamada() {
    LlamadaDefinition l;
    l.type = LlamadaType::STANDARD;
    l.name = "Standard Llamada";
    l.description = "Basic call-response for section transitions";
    l.durationBeats = 2;
    l.requiresClaveAlign = true;
    l.intensity = 0.8f;
    // Call: syncopated phrase (positions in 16-grid for 2 beats = 8 steps)
    l.callPattern = {0, 3, 5, 7};  // "1 . . & . & . &"
    // Response: unison hit
    l.responsePattern = {0};       // All hit together on downbeat
    return l;
}

inline LlamadaDefinition createMontunoEntry() {
    LlamadaDefinition l;
    l.type = LlamadaType::MONTUNO_ENTRY;
    l.name = "Montuno Entry";
    l.description = "Signal to enter montuno section";
    l.durationBeats = 4;
    l.requiresClaveAlign = true;
    l.intensity = 0.85f;
    // More elaborate call phrase
    l.callPattern = {0, 3, 4, 7, 8, 11, 14, 15};  // Syncopated run
    l.responsePattern = {0, 4};   // Two accents
    return l;
}

inline LlamadaDefinition createMamboCall() {
    LlamadaDefinition l;
    l.type = LlamadaType::MAMBO_CALL;
    l.name = "Mambo Call";
    l.description = "Signal for mambo break section";
    l.durationBeats = 2;
    l.requiresClaveAlign = true;
    l.intensity = 0.9f;
    // Driving repeated pattern
    l.callPattern = {0, 2, 4, 6};  // Straight 8ths
    l.responsePattern = {0, 3, 6}; // Tresillo-like response
    return l;
}

inline LlamadaDefinition createCoroEntry() {
    LlamadaDefinition l;
    l.type = LlamadaType::CORO_ENTRY;
    l.name = "Coro Entry";
    l.description = "Signal for chorus entry";
    l.durationBeats = 2;
    l.requiresClaveAlign = true;
    l.intensity = 0.75f;
    // Simple but clear
    l.callPattern = {0, 4, 6};     // "1 . . . 3 . & ."
    l.responsePattern = {0};       // Single unison
    return l;
}

inline LlamadaDefinition createDiablo() {
    LlamadaDefinition l;
    l.type = LlamadaType::DIABLO;
    l.name = "Diablo";
    l.description = "Intense climactic call (the devil)";
    l.durationBeats = 4;
    l.requiresClaveAlign = false;  // Can override clave for intensity
    l.intensity = 1.0f;
    // Dense, aggressive pattern
    l.callPattern = {0, 1, 2, 3, 4, 6, 8, 10, 12, 13, 14, 15};
    l.responsePattern = {0, 2, 4, 6};  // Driving response
    return l;
}

inline LlamadaDefinition createCierre() {
    LlamadaDefinition l;
    l.type = LlamadaType::CIERRE;
    l.name = "Cierre";
    l.description = "Closing/ending signal";
    l.durationBeats = 2;
    l.requiresClaveAlign = true;
    l.intensity = 0.85f;
    // Decisive ending phrase
    l.callPattern = {0, 3, 6, 7};  // Short decisive call
    l.responsePattern = {0, 7};    // Final accent with pickup
    return l;
}

// ========================================
// Llamada Result Structure
// ========================================

struct LlamadaResult {
    Pattern callPattern;          // The call phrase
    Pattern responsePattern;      // The response phrase
    std::vector<Pattern> allPartsCall;     // All voices during call
    std::vector<Pattern> allPartsResponse; // All voices during response
    int totalLength;              // Total duration in steps
    float callVelocity;
    float responseVelocity;
};

// ========================================
// Llamada Engine
// ========================================

class LlamadaEngine {
public:
    LlamadaEngine() : currentType(LlamadaType::STANDARD), gen(std::random_device{}()) {
        definitions.push_back(createStandardLlamada());
        definitions.push_back(createMontunoEntry());
        definitions.push_back(createMamboCall());
        definitions.push_back(createCoroEntry());
        definitions.push_back(createDiablo());
        definitions.push_back(createCierre());
    }

    // ========================================
    // Type Selection
    // ========================================

    void setType(LlamadaType type) {
        currentType = type;
    }

    void setTypeByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(LlamadaType::NUM_TYPES)) {
            currentType = static_cast<LlamadaType>(index);
        }
    }

    LlamadaType getType() const { return currentType; }

    const LlamadaDefinition& getCurrentDefinition() const {
        return definitions[static_cast<int>(currentType)];
    }

    std::string getCurrentName() const {
        return definitions[static_cast<int>(currentType)].name;
    }

    // ========================================
    // Core Generation
    // ========================================

    // Generate basic llamada call pattern
    Pattern generateCall(int length, float velocity) {
        const auto& def = getCurrentDefinition();
        Pattern p(length);

        float vel = velocity * def.intensity;

        // Map definition's call pattern to requested length
        int defLength = def.durationBeats * 4;  // 16th notes per beat

        for (int pos : def.callPattern) {
            int mappedPos = (pos * length) / defLength;
            if (mappedPos < length) {
                p.setOnset(mappedPos, vel);
                p.accents[mappedPos] = true;
            }
        }

        return p;
    }

    // Generate response pattern
    Pattern generateResponse(int length, float velocity) {
        const auto& def = getCurrentDefinition();
        Pattern p(length);

        float vel = velocity * def.intensity;

        int defLength = def.durationBeats * 4;

        for (int pos : def.responsePattern) {
            int mappedPos = (pos * length) / defLength;
            if (mappedPos < length) {
                p.setOnset(mappedPos, vel);
                p.accents[mappedPos] = true;
            }
        }

        return p;
    }

    // Generate complete llamada with all parts
    LlamadaResult generateComplete(int stepsPerBeat, float velocity, int numVoices = 4) {
        const auto& def = getCurrentDefinition();
        LlamadaResult result;

        int callLength = def.durationBeats * stepsPerBeat;
        int responseLength = stepsPerBeat * 2;  // Response typically 2 beats

        result.callPattern = generateCall(callLength, velocity);
        result.responsePattern = generateResponse(responseLength, velocity);
        result.totalLength = callLength + responseLength;
        result.callVelocity = velocity * def.intensity;
        result.responseVelocity = velocity * def.intensity * 0.9f;

        // Generate patterns for all voices
        result.allPartsCall.resize(numVoices);
        result.allPartsResponse.resize(numVoices);

        for (int v = 0; v < numVoices; v++) {
            // During call: only lead voice (last) plays the call
            // Others either rest or play sparse support
            if (v == numVoices - 1) {
                // Lead plays the full call
                result.allPartsCall[v] = result.callPattern;
            } else if (v == 0) {
                // Timeline maintains sparse pattern
                result.allPartsCall[v] = generateSparseSupport(callLength, velocity * 0.6f);
            } else {
                // Others rest during call
                result.allPartsCall[v] = Pattern(callLength);
            }

            // During response: all voices hit together (unison)
            result.allPartsResponse[v] = result.responsePattern;

            // Adjust velocity by role
            float roleVel = (v == numVoices - 1) ? 1.0f :
                           (v == 0) ? 0.8f : 0.9f;
            for (int i = 0; i < responseLength; i++) {
                if (result.allPartsResponse[v].hasOnsetAt(i)) {
                    result.allPartsResponse[v].setOnset(i,
                        result.allPartsResponse[v].getVelocity(i) * roleVel);
                }
            }
        }

        return result;
    }

    // ========================================
    // Clave-Aligned Generation
    // ========================================

    // Generate llamada that respects clave position
    LlamadaResult generateClaveAligned(int stepsPerBeat, float velocity,
                                        const Pattern& clavePattern, int clavePosition) {
        const auto& def = getCurrentDefinition();

        if (!def.requiresClaveAlign) {
            return generateComplete(stepsPerBeat, velocity);
        }

        LlamadaResult result = generateComplete(stepsPerBeat, velocity);

        // Adjust call pattern to align with clave
        // Ensure strong hits align with clave positions
        for (int i = 0; i < result.callPattern.length; i++) {
            int globalPos = (clavePosition + i) % clavePattern.length;
            if (clavePattern.hasOnsetAt(globalPos)) {
                // Boost velocity on clave positions
                if (result.callPattern.hasOnsetAt(i)) {
                    float v = result.callPattern.getVelocity(i);
                    result.callPattern.setOnset(i, std::min(1.0f, v * 1.2f));
                }
            }
        }

        return result;
    }

    // ========================================
    // Variation Generation
    // ========================================

    // Add variation to llamada
    Pattern addVariation(const Pattern& base, float variationAmount) {
        Pattern p = base;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                // Velocity variation
                float v = p.getVelocity(i);
                float variation = (dist(gen) - 0.5f) * 0.2f * variationAmount;
                p.setOnset(i, std::clamp(v + variation, 0.3f, 1.0f));
            } else if (dist(gen) < 0.1f * variationAmount) {
                // Occasional ghost note addition
                p.setOnset(i, 0.3f);
            }
        }

        return p;
    }

    // Generate pickup phrase before llamada
    Pattern generatePickup(int length, float velocity) {
        Pattern p(length);

        // Simple pickup: hits on last few positions leading into call
        int startPos = std::max(0, length - 3);
        for (int i = startPos; i < length; i++) {
            float v = velocity * (0.6f + 0.15f * (i - startPos));
            p.setOnset(i, v);
        }

        return p;
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getTypeName(LlamadaType type) {
        switch (type) {
            case LlamadaType::STANDARD: return "Standard";
            case LlamadaType::MONTUNO_ENTRY: return "Montuno Entry";
            case LlamadaType::MAMBO_CALL: return "Mambo Call";
            case LlamadaType::CORO_ENTRY: return "Coro Entry";
            case LlamadaType::DIABLO: return "Diablo";
            case LlamadaType::CIERRE: return "Cierre";
            default: return "Unknown";
        }
    }

    int getNumTypes() const {
        return static_cast<int>(LlamadaType::NUM_TYPES);
    }

private:
    LlamadaType currentType;
    std::vector<LlamadaDefinition> definitions;
    std::mt19937 gen;

    // Generate sparse support pattern (for timeline during call)
    Pattern generateSparseSupport(int length, float velocity) {
        Pattern p(length);

        // Only hit on strong beats
        for (int i = 0; i < length; i += 4) {
            p.setOnset(i, velocity);
        }

        return p;
    }
};

} // namespace WorldRhythm
