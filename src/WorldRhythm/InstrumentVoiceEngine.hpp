#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Instrument Voice Engine
// ========================================
// Provides specific articulation/voice types for traditional instruments
// Based on module_design.md section on instrument-specific configurations
//
// Instruments covered:
// - Djembe (West African): bass, tone, slap
// - Tabla (Indian): complete Bol system
// - Conga (Afro-Cuban): open, muff, slap, heel-toe

// ========================================
// Djembe Voice System
// ========================================

enum class DjembeVoice {
    BASS = 0,    // Deep center hit, low pitch
    TONE,        // Edge hit, mid pitch, open
    SLAP,        // Edge hit with fingers, high/sharp
    MUFFLED,     // Dampened tone
    FLAM,        // Double hit (ghost + main)
    NUM_VOICES
};

struct DjembeVoiceDefinition {
    DjembeVoice voice;
    std::string name;
    std::string notation;     // Traditional notation symbol
    float pitchOffset;        // Relative pitch (0 = bass)
    float decay;              // Note length multiplier
    float typicalVelocity;
};

inline DjembeVoiceDefinition getDjembeVoice(DjembeVoice v) {
    switch (v) {
        case DjembeVoice::BASS:
            return {v, "Bass", "B", 0.0f, 1.0f, 0.9f};
        case DjembeVoice::TONE:
            return {v, "Tone", "T", 7.0f, 0.8f, 0.8f};
        case DjembeVoice::SLAP:
            return {v, "Slap", "S", 12.0f, 0.4f, 1.0f};
        case DjembeVoice::MUFFLED:
            return {v, "Muffled", "M", 5.0f, 0.3f, 0.6f};
        case DjembeVoice::FLAM:
            return {v, "Flam", "F", 7.0f, 0.8f, 0.95f};
        default:
            return {DjembeVoice::TONE, "Tone", "T", 7.0f, 0.8f, 0.8f};
    }
}

// ========================================
// Tabla Bol System
// ========================================

// Tabla has two drums: Dayan (right, high) and Bayan (left, low)
enum class TablaBol {
    // Dayan (right hand) bols
    NA = 0,      // Ring finger strike on edge
    TA,          // Index finger strike
    TIN,         // Ring finger, open sound
    TE,          // Index finger, slight mute
    TI,          // Quick light stroke
    RE,          // Flick with middle finger

    // Bayan (left hand) bols
    GE,          // Open bass stroke
    KA,          // Flat palm strike (closed)
    GHE,         // Bass with resonance
    KAT,         // Quick dampened bass

    // Combined bols (both hands)
    DHA,         // GE + NA simultaneously
    DHIN,        // GE + TIN
    DHI,         // GE + TI
    TUN,         // Deep bass note (special)
    TETE,        // Double stroke TA-TA

    NUM_BOLS
};

struct TablaBolDefinition {
    TablaBol bol;
    std::string name;
    std::string devanagari;   // Hindi script (optional)
    bool usesDayan;           // Right drum
    bool usesBayan;           // Left drum
    float pitchDayan;         // Right pitch offset
    float pitchBayan;         // Left pitch offset
    float velocity;
    float duration;           // Relative duration
};

inline TablaBolDefinition getTablaBol(TablaBol b) {
    switch (b) {
        // Dayan only
        case TablaBol::NA:
            return {b, "Na", "ना", true, false, 12.0f, 0.0f, 0.85f, 0.5f};
        case TablaBol::TA:
            return {b, "Ta", "ता", true, false, 10.0f, 0.0f, 0.9f, 0.4f};
        case TablaBol::TIN:
            return {b, "Tin", "तीं", true, false, 14.0f, 0.0f, 0.8f, 0.7f};
        case TablaBol::TE:
            return {b, "Te", "ते", true, false, 11.0f, 0.0f, 0.7f, 0.3f};
        case TablaBol::TI:
            return {b, "Ti", "ति", true, false, 13.0f, 0.0f, 0.6f, 0.25f};
        case TablaBol::RE:
            return {b, "Re", "रे", true, false, 15.0f, 0.0f, 0.5f, 0.2f};

        // Bayan only
        case TablaBol::GE:
            return {b, "Ge", "गे", false, true, 0.0f, 0.0f, 0.85f, 0.8f};
        case TablaBol::KA:
            return {b, "Ka", "का", false, true, 0.0f, 3.0f, 0.7f, 0.3f};
        case TablaBol::GHE:
            return {b, "Ghe", "घे", false, true, 0.0f, -2.0f, 0.9f, 1.0f};
        case TablaBol::KAT:
            return {b, "Kat", "कट", false, true, 0.0f, 2.0f, 0.6f, 0.2f};

        // Combined
        case TablaBol::DHA:
            return {b, "Dha", "धा", true, true, 12.0f, 0.0f, 1.0f, 0.7f};
        case TablaBol::DHIN:
            return {b, "Dhin", "धिं", true, true, 14.0f, 0.0f, 0.95f, 0.8f};
        case TablaBol::DHI:
            return {b, "Dhi", "धि", true, true, 13.0f, -1.0f, 0.85f, 0.5f};
        case TablaBol::TUN:
            return {b, "Tun", "तूं", false, true, 0.0f, -5.0f, 0.9f, 1.2f};
        case TablaBol::TETE:
            return {b, "Tete", "तेते", true, false, 10.0f, 0.0f, 0.8f, 0.3f};

        default:
            return {TablaBol::NA, "Na", "ना", true, false, 12.0f, 0.0f, 0.85f, 0.5f};
    }
}

// ========================================
// Conga Voice System
// ========================================

enum class CongaVoice {
    OPEN = 0,    // Open tone
    MUFF,        // Muffled tone (heel on head)
    SLAP,        // Sharp slap
    BASS,        // Full palm in center
    HEEL,        // Heel of palm
    TIP,         // Fingertips
    TOUCH,       // Light ghost touch
    NUM_VOICES
};

struct CongaVoiceDefinition {
    CongaVoice voice;
    std::string name;
    float pitchOffset;
    float decay;
    float typicalVelocity;
};

inline CongaVoiceDefinition getCongaVoice(CongaVoice v) {
    switch (v) {
        case CongaVoice::OPEN:
            return {v, "Open", 0.0f, 1.0f, 0.85f};
        case CongaVoice::MUFF:
            return {v, "Muff", -2.0f, 0.3f, 0.7f};
        case CongaVoice::SLAP:
            return {v, "Slap", 5.0f, 0.4f, 1.0f};
        case CongaVoice::BASS:
            return {v, "Bass", -5.0f, 0.8f, 0.9f};
        case CongaVoice::HEEL:
            return {v, "Heel", -3.0f, 0.5f, 0.6f};
        case CongaVoice::TIP:
            return {v, "Tip", 3.0f, 0.3f, 0.5f};
        case CongaVoice::TOUCH:
            return {v, "Touch", 0.0f, 0.2f, 0.3f};
        default:
            return {CongaVoice::OPEN, "Open", 0.0f, 1.0f, 0.85f};
    }
}

// ========================================
// Voice Event Structure
// ========================================

struct VoiceEvent {
    int position;
    float velocity;
    float pitchOffset;
    float duration;
    int voiceType;        // Enum value of specific voice
    bool isAccent;
};

// ========================================
// Pattern with Voice Information
// ========================================

struct VoicedPattern {
    int length;
    std::vector<VoiceEvent> events;

    VoicedPattern(int len = 16) : length(len) {}

    void addEvent(const VoiceEvent& e) {
        events.push_back(e);
    }

    // Convert to basic Pattern (loses voice info)
    Pattern toBasicPattern() const {
        Pattern p(length);
        for (const auto& e : events) {
            if (e.position < length) {
                p.setOnset(e.position, e.velocity);
                p.accents[e.position] = e.isAccent;
            }
        }
        return p;
    }

    // Get events at specific position
    std::vector<VoiceEvent> getEventsAt(int pos) const {
        std::vector<VoiceEvent> result;
        for (const auto& e : events) {
            if (e.position == pos) {
                result.push_back(e);
            }
        }
        return result;
    }
};

// ========================================
// Instrument Voice Engine
// ========================================

class InstrumentVoiceEngine {
public:
    InstrumentVoiceEngine() : gen(std::random_device{}()) {}

    // ========================================
    // Djembe Pattern Generation
    // ========================================

    VoicedPattern generateDjembePattern(int length, float density, float velocity) {
        VoicedPattern vp(length);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Traditional djembe patterns favor specific positions
        std::vector<float> positionWeights = {
            1.0f, 0.3f, 0.6f, 0.3f,   // Beat 1
            0.8f, 0.3f, 0.7f, 0.4f,   // Beat 2
            0.9f, 0.3f, 0.6f, 0.3f,   // Beat 3
            0.8f, 0.4f, 0.7f, 0.5f    // Beat 4
        };

        int targetHits = static_cast<int>(length * density);
        int hitCount = 0;

        for (int i = 0; i < length && hitCount < targetHits; i++) {
            int weightIdx = (i * 16) / length;
            float prob = positionWeights[weightIdx % 16] * density;

            if (dist(gen) < prob) {
                DjembeVoice voice = selectDjembeVoice(i, length);
                auto def = getDjembeVoice(voice);

                VoiceEvent e;
                e.position = i;
                e.velocity = velocity * def.typicalVelocity;
                e.pitchOffset = def.pitchOffset;
                e.duration = def.decay;
                e.voiceType = static_cast<int>(voice);
                e.isAccent = (weightIdx % 4 == 0);

                vp.addEvent(e);
                hitCount++;
            }
        }

        return vp;
    }

    // Generate specific djembe rhythm pattern
    VoicedPattern generateDjembeRhythm(const std::string& rhythm, int length, float velocity) {
        VoicedPattern vp(length);

        // Parse rhythm string (e.g., "B.T.S.T.B.T.S.T.")
        int pos = 0;
        for (char c : rhythm) {
            if (pos >= length) break;

            DjembeVoice voice;
            bool hasHit = true;

            switch (c) {
                case 'B': case 'b': voice = DjembeVoice::BASS; break;
                case 'T': case 't': voice = DjembeVoice::TONE; break;
                case 'S': case 's': voice = DjembeVoice::SLAP; break;
                case 'M': case 'm': voice = DjembeVoice::MUFFLED; break;
                case 'F': case 'f': voice = DjembeVoice::FLAM; break;
                case '.': case '-': hasHit = false; break;
                default: hasHit = false; break;
            }

            if (hasHit) {
                auto def = getDjembeVoice(voice);
                VoiceEvent e;
                e.position = pos;
                e.velocity = velocity * def.typicalVelocity;
                e.pitchOffset = def.pitchOffset;
                e.duration = def.decay;
                e.voiceType = static_cast<int>(voice);
                e.isAccent = (voice == DjembeVoice::SLAP || voice == DjembeVoice::BASS);
                vp.addEvent(e);
            }

            pos++;
        }

        return vp;
    }

    // ========================================
    // Tabla Theka Generation
    // ========================================

    // Generate standard Teental theka (16 beats)
    VoicedPattern generateTeentalTheka(int length, float velocity) {
        VoicedPattern vp(length);

        // Classic Teental: Dha Dhin Dhin Dha | Dha Dhin Dhin Dha | Dha Tin Tin Ta | Ta Dhin Dhin Dha
        std::vector<TablaBol> theka = {
            TablaBol::DHA, TablaBol::DHIN, TablaBol::DHIN, TablaBol::DHA,
            TablaBol::DHA, TablaBol::DHIN, TablaBol::DHIN, TablaBol::DHA,
            TablaBol::DHA, TablaBol::TIN, TablaBol::TIN, TablaBol::TA,
            TablaBol::TA, TablaBol::DHIN, TablaBol::DHIN, TablaBol::DHA
        };

        int stepsPerBol = length / 16;
        if (stepsPerBol < 1) stepsPerBol = 1;

        for (size_t i = 0; i < theka.size(); i++) {
            int pos = static_cast<int>(i) * stepsPerBol;
            if (pos >= length) break;

            auto def = getTablaBol(theka[i]);
            VoiceEvent e;
            e.position = pos;
            e.velocity = velocity * def.velocity;
            e.pitchOffset = def.usesDayan ? def.pitchDayan : def.pitchBayan;
            e.duration = def.duration;
            e.voiceType = static_cast<int>(theka[i]);
            e.isAccent = (i == 0 || i == 8);  // Sam and khali

            vp.addEvent(e);
        }

        return vp;
    }

    // Generate Jhaptaal theka (10 beats)
    VoicedPattern generateJhaptaalTheka(int length, float velocity) {
        VoicedPattern vp(length);

        // Jhaptaal: Dhi Na | Dhi Dhi Na | Ti Na | Dhi Dhi Na
        std::vector<TablaBol> theka = {
            TablaBol::DHI, TablaBol::NA,
            TablaBol::DHI, TablaBol::DHI, TablaBol::NA,
            TablaBol::TI, TablaBol::NA,
            TablaBol::DHI, TablaBol::DHI, TablaBol::NA
        };

        int stepsPerBol = length / 10;
        if (stepsPerBol < 1) stepsPerBol = 1;

        for (size_t i = 0; i < theka.size(); i++) {
            int pos = static_cast<int>(i) * stepsPerBol;
            if (pos >= length) break;

            auto def = getTablaBol(theka[i]);
            VoiceEvent e;
            e.position = pos;
            e.velocity = velocity * def.velocity;
            e.pitchOffset = def.usesDayan ? def.pitchDayan : def.pitchBayan;
            e.duration = def.duration;
            e.voiceType = static_cast<int>(theka[i]);
            e.isAccent = (i == 0 || i == 5);

            vp.addEvent(e);
        }

        return vp;
    }

    // Generate custom bol sequence
    VoicedPattern generateBolSequence(const std::vector<TablaBol>& bols, int length, float velocity) {
        VoicedPattern vp(length);

        int stepsPerBol = length / static_cast<int>(bols.size());
        if (stepsPerBol < 1) stepsPerBol = 1;

        for (size_t i = 0; i < bols.size(); i++) {
            int pos = static_cast<int>(i) * stepsPerBol;
            if (pos >= length) break;

            auto def = getTablaBol(bols[i]);
            VoiceEvent e;
            e.position = pos;
            e.velocity = velocity * def.velocity;
            e.pitchOffset = def.usesDayan ? def.pitchDayan : def.pitchBayan;
            e.duration = def.duration;
            e.voiceType = static_cast<int>(bols[i]);
            e.isAccent = (i == 0);

            vp.addEvent(e);
        }

        return vp;
    }

    // ========================================
    // Conga Tumbao Generation
    // ========================================

    VoicedPattern generateCongaTumbao(int length, float velocity) {
        VoicedPattern vp(length);

        // Classic tumbao pattern with voice variety
        // Position:  1 e & a 2 e & a 3 e & a 4 e & a
        std::vector<std::pair<int, CongaVoice>> tumbao = {
            {0, CongaVoice::HEEL},
            {2, CongaVoice::OPEN},
            {4, CongaVoice::MUFF},
            {6, CongaVoice::SLAP},
            {7, CongaVoice::OPEN},
            {10, CongaVoice::OPEN},
            {14, CongaVoice::SLAP},
            {15, CongaVoice::OPEN}
        };

        for (const auto& [pos16, voice] : tumbao) {
            int mappedPos = (pos16 * length) / 16;
            if (mappedPos >= length) continue;

            auto def = getCongaVoice(voice);
            VoiceEvent e;
            e.position = mappedPos;
            e.velocity = velocity * def.typicalVelocity;
            e.pitchOffset = def.pitchOffset;
            e.duration = def.decay;
            e.voiceType = static_cast<int>(voice);
            e.isAccent = (voice == CongaVoice::SLAP);

            vp.addEvent(e);
        }

        return vp;
    }

    // ========================================
    // Utility
    // ========================================

    static std::string getDjembeVoiceName(DjembeVoice v) {
        return getDjembeVoice(v).name;
    }

    static std::string getTablaBolName(TablaBol b) {
        return getTablaBol(b).name;
    }

    static std::string getCongaVoiceName(CongaVoice v) {
        return getCongaVoice(v).name;
    }

private:
    std::mt19937 gen;

    DjembeVoice selectDjembeVoice(int position, int length) {
        // Voice selection based on position
        int pos16 = (position * 16) / length;

        // Strong beats favor bass
        if (pos16 % 4 == 0) {
            return DjembeVoice::BASS;
        }
        // Offbeats favor slaps/tones
        if (pos16 % 2 == 1) {
            std::uniform_int_distribution<int> dist(0, 1);
            return dist(gen) == 0 ? DjembeVoice::SLAP : DjembeVoice::TONE;
        }
        // Others are tones
        return DjembeVoice::TONE;
    }
};

} // namespace WorldRhythm
