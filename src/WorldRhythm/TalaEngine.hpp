#pragma once

#include <vector>
#include <string>
#include <cmath>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Tala System for Indian Classical Music
// ========================================
// Implements the rhythmic cycle structure:
// - Sam: First beat (strongest, must be emphasized)
// - Tali: Clapped beats (strong accents)
// - Khali: Wave/empty beats (lighter accents)

enum class TalaType {
    TEENTAL,      // 16 beats: 4+4+4+4 (most common)
    JHAPTAAL,     // 10 beats: 2+3+2+3
    EKTAAL,       // 12 beats: 2+2+2+2+2+2
    RUPAK,        // 7 beats: 3+2+2 (starts on Khali)
    DADRA,        // 6 beats: 3+3
    KEHERWA,      // 8 beats: 4+4
    JHOOMRA,      // 14 beats: 3+4+3+4
    DHAMAR        // 14 beats: 5+2+3+4
};

struct TalaBeat {
    int position;      // Beat position (0-indexed)
    bool isSam;        // First beat of cycle
    bool isTali;       // Clapped beat
    bool isKhali;      // Wave/empty beat
    int vibhag;        // Section number
    float weight;      // Accent weight (0.0-1.0)
};

struct TalaDefinition {
    TalaType type;
    std::string name;
    int totalBeats;
    std::vector<int> vibhagLengths;  // Length of each section
    std::vector<bool> vibhagTali;     // true=Tali, false=Khali
    std::vector<TalaBeat> beats;
};

// ========================================
// Tala Definitions
// ========================================
inline TalaDefinition createTeental() {
    TalaDefinition t;
    t.type = TalaType::TEENTAL;
    t.name = "Teental";
    t.totalBeats = 16;
    t.vibhagLengths = {4, 4, 4, 4};
    t.vibhagTali = {true, true, false, true};  // X 2 0 3

    t.beats.resize(16);
    int pos = 0;
    for (int v = 0; v < 4; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0);
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            // Weight calculation
            if (b.isSam) b.weight = 1.0f;
            else if (b.isTali) b.weight = 0.85f;
            else if (b.isKhali) b.weight = 0.5f;
            else if (i == 0) b.weight = 0.7f;  // Vibhag start
            else b.weight = 0.4f + (i % 2 == 0 ? 0.1f : 0.0f);

            pos++;
        }
    }
    return t;
}

inline TalaDefinition createJhaptaal() {
    TalaDefinition t;
    t.type = TalaType::JHAPTAAL;
    t.name = "Jhaptaal";
    t.totalBeats = 10;
    t.vibhagLengths = {2, 3, 2, 3};
    t.vibhagTali = {true, true, false, true};  // X 2 0 3

    t.beats.resize(10);
    int pos = 0;
    for (int v = 0; v < 4; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0);
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            if (b.isSam) b.weight = 1.0f;
            else if (b.isTali) b.weight = 0.85f;
            else if (b.isKhali) b.weight = 0.5f;
            else if (i == 0) b.weight = 0.7f;
            else b.weight = 0.45f;

            pos++;
        }
    }
    return t;
}

inline TalaDefinition createEktaal() {
    TalaDefinition t;
    t.type = TalaType::EKTAAL;
    t.name = "Ektaal";
    t.totalBeats = 12;
    t.vibhagLengths = {2, 2, 2, 2, 2, 2};
    t.vibhagTali = {true, false, true, true, false, true};  // X 0 2 3 0 4

    t.beats.resize(12);
    int pos = 0;
    for (int v = 0; v < 6; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0);
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            if (b.isSam) b.weight = 1.0f;
            else if (b.isTali) b.weight = 0.8f;
            else if (b.isKhali) b.weight = 0.45f;
            else b.weight = 0.5f;

            pos++;
        }
    }
    return t;
}

inline TalaDefinition createRupak() {
    TalaDefinition t;
    t.type = TalaType::RUPAK;
    t.name = "Rupak";
    t.totalBeats = 7;
    t.vibhagLengths = {3, 2, 2};
    t.vibhagTali = {false, true, true};  // 0 X 2 (starts on Khali!)

    t.beats.resize(7);
    int pos = 0;
    for (int v = 0; v < 3; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0) && !b.isSam;
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            // Rupak is unique: Sam is on Khali (lighter)
            if (b.isSam) b.weight = 0.7f;  // Sam but Khali
            else if (b.isTali) b.weight = 0.9f;
            else if (b.isKhali) b.weight = 0.5f;
            else b.weight = 0.45f;

            pos++;
        }
    }
    return t;
}

inline TalaDefinition createDadra() {
    TalaDefinition t;
    t.type = TalaType::DADRA;
    t.name = "Dadra";
    t.totalBeats = 6;
    t.vibhagLengths = {3, 3};
    t.vibhagTali = {true, false};  // X 0

    t.beats.resize(6);
    int pos = 0;
    for (int v = 0; v < 2; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0);
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            if (b.isSam) b.weight = 1.0f;
            else if (b.isTali) b.weight = 0.85f;
            else if (b.isKhali) b.weight = 0.5f;
            else b.weight = 0.5f;

            pos++;
        }
    }
    return t;
}

inline TalaDefinition createKeherwa() {
    TalaDefinition t;
    t.type = TalaType::KEHERWA;
    t.name = "Keherwa";
    t.totalBeats = 8;
    t.vibhagLengths = {4, 4};
    t.vibhagTali = {true, false};  // X 0

    t.beats.resize(8);
    int pos = 0;
    for (int v = 0; v < 2; v++) {
        for (int i = 0; i < t.vibhagLengths[v]; i++) {
            TalaBeat& b = t.beats[pos];
            b.position = pos;
            b.vibhag = v;
            b.isSam = (pos == 0);
            b.isTali = t.vibhagTali[v] && (i == 0);
            b.isKhali = !t.vibhagTali[v] && (i == 0);

            if (b.isSam) b.weight = 1.0f;
            else if (b.isTali) b.weight = 0.85f;
            else if (b.isKhali) b.weight = 0.5f;
            else if (i == 2) b.weight = 0.6f;  // Half-beat accent
            else b.weight = 0.45f;

            pos++;
        }
    }
    return t;
}

// ========================================
// Tala Engine
// ========================================
class TalaEngine {
private:
    TalaDefinition currentTala;
    std::vector<TalaDefinition> availableTalas;

public:
    TalaEngine() {
        // Initialize all talas
        availableTalas.push_back(createTeental());
        availableTalas.push_back(createJhaptaal());
        availableTalas.push_back(createEktaal());
        availableTalas.push_back(createRupak());
        availableTalas.push_back(createDadra());
        availableTalas.push_back(createKeherwa());

        currentTala = availableTalas[0];  // Default to Teental
    }

    void setTala(TalaType type) {
        for (const auto& t : availableTalas) {
            if (t.type == type) {
                currentTala = t;
                return;
            }
        }
    }

    void setTalaByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(availableTalas.size())) {
            currentTala = availableTalas[index];
        }
    }

    const TalaDefinition& getCurrentTala() const { return currentTala; }
    int getTotalBeats() const { return currentTala.totalBeats; }
    int getNumTalas() const { return static_cast<int>(availableTalas.size()); }

    // ========================================
    // Get weight for a specific step position
    // Maps pattern steps to tala beats
    // ========================================
    float getWeightForStep(int step, int patternLength) const {
        // Map step to tala beat
        int beat = (step * currentTala.totalBeats) / patternLength;
        beat = beat % currentTala.totalBeats;

        if (beat >= 0 && beat < static_cast<int>(currentTala.beats.size())) {
            return currentTala.beats[beat].weight;
        }
        return 0.5f;
    }

    // ========================================
    // Check if step is Sam (first beat)
    // ========================================
    bool isSam(int step, int patternLength) const {
        int beat = (step * currentTala.totalBeats) / patternLength;
        return (beat % currentTala.totalBeats) == 0;
    }

    // ========================================
    // Check if step is Tali (clapped beat)
    // ========================================
    bool isTali(int step, int patternLength) const {
        int beat = (step * currentTala.totalBeats) / patternLength;
        beat = beat % currentTala.totalBeats;

        if (beat >= 0 && beat < static_cast<int>(currentTala.beats.size())) {
            return currentTala.beats[beat].isTali;
        }
        return false;
    }

    // ========================================
    // Check if step is Khali (wave beat)
    // ========================================
    bool isKhali(int step, int patternLength) const {
        int beat = (step * currentTala.totalBeats) / patternLength;
        beat = beat % currentTala.totalBeats;

        if (beat >= 0 && beat < static_cast<int>(currentTala.beats.size())) {
            return currentTala.beats[beat].isKhali;
        }
        return false;
    }

    // ========================================
    // Apply Tala constraints to pattern
    // Enforces Sam, Tali accents, Khali lighter
    // ========================================
    void applyTalaConstraints(Pattern& p) const {
        for (int i = 0; i < p.length; i++) {
            float talaWeight = getWeightForStep(i, p.length);

            // Sam position: ensure strong hit
            if (isSam(i, p.length)) {
                if (!p.hasOnsetAt(i)) {
                    p.setOnset(i, 0.9f);
                } else {
                    float vel = std::max(p.getVelocity(i), 0.85f);
                    p.setOnset(i, vel);
                }
                p.accents[i] = true;
            }
            // Tali positions: emphasize if present
            else if (isTali(i, p.length)) {
                if (p.hasOnsetAt(i)) {
                    float vel = std::max(p.getVelocity(i), 0.7f);
                    p.setOnset(i, vel);
                    p.accents[i] = true;
                }
            }
            // Khali positions: reduce if present
            else if (isKhali(i, p.length)) {
                if (p.hasOnsetAt(i)) {
                    float vel = std::min(p.getVelocity(i), 0.6f);
                    p.setOnset(i, vel);
                    p.accents[i] = false;
                }
            }
        }
    }

    // ========================================
    // Generate Tala-aware weights for pattern generation
    // ========================================
    std::vector<float> generateTalaWeights(int patternLength) const {
        std::vector<float> weights(patternLength);

        for (int i = 0; i < patternLength; i++) {
            weights[i] = getWeightForStep(i, patternLength);
        }

        return weights;
    }

    // ========================================
    // Generate Tihai ending pattern
    // Tihai: phrase repeated 3 times, landing on Sam
    // ========================================
    std::vector<float> generateTihai(int phraseLength, int totalSteps, float intensity) const {
        std::vector<float> pattern(totalSteps, 0.0f);

        // Calculate spacing to land on Sam
        // Formula: 3 * phrase + 2 * gap = totalSteps
        int gapLength = (totalSteps - 3 * phraseLength) / 2;
        if (gapLength < 0) {
            phraseLength = totalSteps / 4;
            gapLength = (totalSteps - 3 * phraseLength) / 2;
        }

        int pos = totalSteps - (3 * phraseLength + 2 * gapLength);
        if (pos < 0) pos = 0;

        // Generate three repetitions
        for (int rep = 0; rep < 3; rep++) {
            float repIntensity = 0.6f + rep * 0.15f;  // Each louder

            for (int i = 0; i < phraseLength && pos < totalSteps; i++) {
                // Accent on first note of each phrase
                float vel = (i == 0) ? repIntensity + 0.2f : repIntensity;
                pattern[pos++] = std::clamp(vel * intensity, 0.3f, 1.0f);
            }

            if (rep < 2) {
                pos += gapLength;
            }
        }

        // Ensure last hit is strong (landing on Sam)
        if (totalSteps > 0) {
            pattern[totalSteps - 1] = std::clamp(0.95f * intensity, 0.85f, 1.0f);
        }

        return pattern;
    }
};

} // namespace WorldRhythm
