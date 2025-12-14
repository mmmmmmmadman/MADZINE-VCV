#pragma once
#include "HumanizeEngine.hpp"

namespace WorldRhythm {

// ============================================================================
// Articulation Profile System
// ============================================================================
// Based on ethnomusicological research of drumming traditions worldwide.
// Each style and role combination has specific articulation tendencies.
//
// References:
// - West African: Djembe uses flams, rolls, slaps/tones. Solo passages feature
//   rapid rolls and flams. (afrodrumming.com, snarestory.com)
// - Afro-Cuban: Conga techniques include slap, open tone, muffled tone,
//   heel-toe. European military rudiments (paradiddle, rolls) integrated.
//   (marcdedouvan.com, franciscocrow.com)
// - Brazilian: Repinique uses rebound, double rebound, rim-shot, slap.
//   Bebop drummers inspired by repinique double-rebound. (marcdedouvan.com)
// - Balkan: Tapan uses heavy stick (bass) + light switch (treble). Complex
//   accents in aksak asymmetric meters. Non-dominant hand expresses melody.
//   (Wikipedia, ResearchGate)
// - Indian: Tabla has 16+ bols with distinct articulations. Flams, tihai
//   (phrase repeated 3x). Gharana styles differ in technique. (tablalegacy.com)
// - Gamelan: Kotekan interlocking (polos/sangsih). Kendang has 14 stroke types.
//   Angsel (dramatic breaks) led by kendang signals. (gamelan.org.nz)
// - Jazz: Rudiments from military tradition. Flams, drags, brush sweeps.
//   Ghost notes essential for swing feel. Bebop "dropping bombs". (hudsonmusic.com)
// - Electronic/Techno: Accent on quarter notes. Hi-hat rolls (3 or 5 16ths).
//   Velocity variation critical. (studiobrootle.com, native-instruments.com)
// - Breakbeat: Amen break features ghost notes between backbeats. Chopping
//   creates syncopation. Flams add weight. (amen-break.com, drumeo.com)
// ============================================================================

// Articulation probability entry for a specific style/role combination
struct ArticulationEntry {
    ArticulationType type;
    float baseProbability;  // Base probability when articulation amount = 1.0
    bool onAccentsOnly;     // If true, only apply to accented notes
    bool onStrongBeats;     // If true, prioritize strong beat positions
};

// Maximum articulations per style/role combination
constexpr int MAX_ARTICULATIONS_PER_PROFILE = 4;

struct ArticulationProfile {
    ArticulationEntry entries[MAX_ARTICULATIONS_PER_PROFILE];
    int numEntries;
};

// ============================================================================
// Style Index Reference:
// 0 = West African, 1 = Afro-Cuban, 2 = Brazilian, 3 = Balkan
// 4 = Indian, 5 = Gamelan, 6 = Jazz, 7 = Electronic
// 8 = Breakbeat, 9 = Techno
//
// Role Index Reference:
// 0 = Timeline, 1 = Foundation, 2 = Groove, 3 = Lead
// ============================================================================

// Articulation profiles: [style][role]
const ArticulationProfile ARTICULATION_PROFILES[10][4] = {
    // ========================================================================
    // STYLE 0: West African
    // ========================================================================
    // Djembe tradition: flams on solos, rolls on sustained passages
    // Timeline (bell): minimal ornamentation, steady pulse
    // Foundation (dununba): occasional flams on strong beats
    // Groove (sangban/kenkeni): flams, some rolls
    // Lead (djembe solo): heavy use of flams, ruffs, rolls
    {
        // Timeline - bell pattern, minimal articulation
        { { {ArticulationType::FLAM, 0.05f, true, true} }, 1 },
        // Foundation - dununba, occasional flams
        { { {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::DRAG, 0.05f, true, false} }, 2 },
        // Groove - sangban, moderate flams
        { { {ArticulationType::FLAM, 0.25f, false, true},
            {ArticulationType::DRAG, 0.10f, true, false} }, 2 },
        // Lead - djembe solo, heavy ornamentation
        { { {ArticulationType::FLAM, 0.40f, false, false},
            {ArticulationType::RUFF, 0.20f, true, false},
            {ArticulationType::DRAG, 0.15f, false, false},
            {ArticulationType::BUZZ, 0.10f, false, false} }, 4 }
    },

    // ========================================================================
    // STYLE 1: Afro-Cuban
    // ========================================================================
    // Conga tradition: slap/open/muffled tones, European rudiment influence
    // Timeline (clave): very clean, no ornamentation
    // Foundation (tumbao ponche): occasional flams for emphasis
    // Groove (conga tumbao): heel-toe patterns (modeled as drags), flams
    // Lead (quinto): improvisational flams, ruffs
    {
        // Timeline - clave is clean
        { { {ArticulationType::NORMAL, 1.0f, false, false} }, 1 },
        // Foundation - tumbao, occasional flams
        { { {ArticulationType::FLAM, 0.12f, true, true} }, 1 },
        // Groove - conga, heel-toe (drag), flams
        { { {ArticulationType::DRAG, 0.20f, false, false},
            {ArticulationType::FLAM, 0.18f, true, true} }, 2 },
        // Lead - quinto improvisation
        { { {ArticulationType::FLAM, 0.35f, false, false},
            {ArticulationType::RUFF, 0.15f, true, false},
            {ArticulationType::DRAG, 0.12f, false, false} }, 3 }
    },

    // ========================================================================
    // STYLE 2: Brazilian Samba
    // ========================================================================
    // Batucada tradition: double-rebound, rim-shots, rolls
    // Timeline (agogô): clean strokes
    // Foundation (surdo): occasional rim emphasis
    // Groove (caixa/tamborim): buzz rolls, rim shots common
    // Lead (repinique): heavy double-rebound (buzz), flams, calls
    {
        // Timeline - agogô, clean
        { { {ArticulationType::FLAM, 0.05f, true, true} }, 1 },
        // Foundation - surdo, rim for emphasis
        { { {ArticulationType::RIM, 0.10f, true, true},
            {ArticulationType::FLAM, 0.08f, true, true} }, 2 },
        // Groove - caixa, buzz rolls characteristic
        { { {ArticulationType::BUZZ, 0.30f, false, false},
            {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::RIM, 0.12f, false, false} }, 3 },
        // Lead - repinique, double-rebound essential
        { { {ArticulationType::BUZZ, 0.40f, false, false},
            {ArticulationType::FLAM, 0.25f, false, false},
            {ArticulationType::RUFF, 0.15f, true, false},
            {ArticulationType::RIM, 0.10f, false, false} }, 4 }
    },

    // ========================================================================
    // STYLE 3: Balkan Aksak
    // ========================================================================
    // Tapan/davul tradition: heavy beater + light switch
    // Complex accents in asymmetric meters, ornamental fills
    // Timeline: clean asymmetric pulse
    // Foundation: strong downbeats, occasional flams
    // Groove: fills between downbeats, drags common
    // Lead: ornamental, ruffs and drags
    {
        // Timeline - aksak pulse, clean
        { { {ArticulationType::FLAM, 0.08f, true, true} }, 1 },
        // Foundation - strong downbeats
        { { {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::DRAG, 0.08f, true, false} }, 2 },
        // Groove - fills, drags common
        { { {ArticulationType::DRAG, 0.25f, false, false},
            {ArticulationType::FLAM, 0.18f, true, true},
            {ArticulationType::RUFF, 0.10f, false, false} }, 3 },
        // Lead - ornamental
        { { {ArticulationType::RUFF, 0.30f, false, false},
            {ArticulationType::DRAG, 0.25f, false, false},
            {ArticulationType::FLAM, 0.20f, true, false} }, 3 }
    },

    // ========================================================================
    // STYLE 4: Indian Tala
    // ========================================================================
    // Tabla tradition: 16+ bols, distinct articulations per gharana
    // Flams common, tihai structures
    // Timeline (theka): some ornamentation based on gharana
    // Foundation (bayan): bass strokes, occasional flams
    // Groove (dayan): moderate ornamentation
    // Lead (solo): heavy ornamentation, tihai feel (ruffs/drags)
    {
        // Timeline - theka, moderate ornamentation
        { { {ArticulationType::FLAM, 0.12f, true, true},
            {ArticulationType::DRAG, 0.08f, false, false} }, 2 },
        // Foundation - bayan, bass emphasis
        { { {ArticulationType::FLAM, 0.10f, true, true} }, 1 },
        // Groove - dayan elaboration
        { { {ArticulationType::FLAM, 0.22f, false, true},
            {ArticulationType::DRAG, 0.15f, false, false},
            {ArticulationType::RUFF, 0.10f, true, false} }, 3 },
        // Lead - solo, heavy ornamentation for tihai feel
        { { {ArticulationType::RUFF, 0.35f, false, false},
            {ArticulationType::FLAM, 0.30f, false, false},
            {ArticulationType::DRAG, 0.20f, false, false},
            {ArticulationType::BUZZ, 0.08f, false, false} }, 4 }
    },

    // ========================================================================
    // STYLE 5: Gamelan
    // ========================================================================
    // Kotekan interlocking: polos/sangsih complementary
    // Kendang: 14 stroke types, angsel (dramatic breaks)
    // Timeline (colotomic): very clean punctuation
    // Foundation (gong): clean, resonant
    // Groove (kotekan polos): some flams for angsel
    // Lead (kotekan sangsih): flams at phrase boundaries
    {
        // Timeline - colotomic, clean
        { { {ArticulationType::NORMAL, 1.0f, false, false} }, 1 },
        // Foundation - gong, clean and resonant
        { { {ArticulationType::FLAM, 0.05f, true, true} }, 1 },
        // Groove - kotekan polos, angsel flams
        { { {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::DRAG, 0.08f, true, false} }, 2 },
        // Lead - kotekan sangsih, phrase boundary ornaments
        { { {ArticulationType::FLAM, 0.20f, true, true},
            {ArticulationType::DRAG, 0.12f, true, false},
            {ArticulationType::RUFF, 0.08f, true, false} }, 3 }
    },

    // ========================================================================
    // STYLE 6: Jazz Swing
    // ========================================================================
    // Military rudiment heritage: flams, drags, paradiddles
    // Ghost notes essential, brush work, bebop "bombs"
    // Timeline (ride): clean swing pattern
    // Foundation (kick): sparse, occasional flam "bomb"
    // Groove (snare comping): ghost notes, brush sweeps, flams
    // Lead (fills): ruffs, drags, bebop complexity
    {
        // Timeline - ride cymbal, clean
        { { {ArticulationType::FLAM, 0.05f, true, true} }, 1 },
        // Foundation - kick, occasional bomb (flam)
        { { {ArticulationType::FLAM, 0.12f, true, false} }, 1 },
        // Groove - snare comping, ghost notes separate, flams/drags
        { { {ArticulationType::FLAM, 0.20f, true, true},
            {ArticulationType::DRAG, 0.15f, false, false},
            {ArticulationType::BUZZ, 0.08f, false, false} }, 3 },
        // Lead - fills, bebop complexity
        { { {ArticulationType::DRAG, 0.30f, false, false},
            {ArticulationType::FLAM, 0.25f, false, false},
            {ArticulationType::RUFF, 0.18f, true, false},
            {ArticulationType::BUZZ, 0.10f, false, false} }, 4 }
    },

    // ========================================================================
    // STYLE 7: Electronic
    // ========================================================================
    // Machine precision, velocity-based articulation
    // Hi-hat rolls, accent on quarter notes
    // Minimal traditional articulation, focus on velocity/filter
    // Timeline (hi-hat): rolls possible
    // Foundation (kick): clean four-on-floor
    // Groove (snare/clap): clean backbeat
    // Lead (perc): some flams for emphasis
    {
        // Timeline - hi-hat, occasional rolls
        { { {ArticulationType::BUZZ, 0.15f, false, false},
            {ArticulationType::FLAM, 0.05f, true, true} }, 2 },
        // Foundation - kick, very clean
        { { {ArticulationType::FLAM, 0.03f, true, true} }, 1 },
        // Groove - snare/clap, clean
        { { {ArticulationType::FLAM, 0.08f, true, true} }, 1 },
        // Lead - percussion, flams for emphasis
        { { {ArticulationType::FLAM, 0.18f, true, true},
            {ArticulationType::DRAG, 0.10f, true, false} }, 2 }
    },

    // ========================================================================
    // STYLE 8: Breakbeat
    // ========================================================================
    // Amen break tradition: ghost notes, syncopation, chopped feel
    // Flams add weight, ghost notes essential (separate param)
    // Timeline (hat): clean or with flams
    // Foundation (kick): occasional flams for weight
    // Groove (snare): ghost notes (separate), flams on backbeat
    // Lead (chops): flams, drags for chopped feel
    {
        // Timeline - hat pattern
        { { {ArticulationType::FLAM, 0.10f, true, true} }, 1 },
        // Foundation - kick, flams for weight
        { { {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::DRAG, 0.05f, true, false} }, 2 },
        // Groove - snare, flams on backbeat
        { { {ArticulationType::FLAM, 0.22f, true, true},
            {ArticulationType::DRAG, 0.12f, false, false} }, 2 },
        // Lead - chops, more complex
        { { {ArticulationType::FLAM, 0.30f, false, false},
            {ArticulationType::DRAG, 0.20f, false, false},
            {ArticulationType::RUFF, 0.12f, true, false} }, 3 }
    },

    // ========================================================================
    // STYLE 9: Techno
    // ========================================================================
    // Driving, mechanical precision
    // Minimal articulation, accent-based dynamics
    // Timeline (hi-hat): rolls for buildup
    // Foundation (kick): perfectly clean
    // Groove (clap): clean
    // Lead (industrial perc): occasional flams
    {
        // Timeline - hi-hat, rolls for tension
        { { {ArticulationType::BUZZ, 0.20f, false, false} }, 1 },
        // Foundation - kick, machine precision
        { { {ArticulationType::NORMAL, 1.0f, false, false} }, 1 },
        // Groove - clap, clean
        { { {ArticulationType::FLAM, 0.05f, true, true} }, 1 },
        // Lead - industrial, sparse flams
        { { {ArticulationType::FLAM, 0.15f, true, true},
            {ArticulationType::DRAG, 0.08f, true, false} }, 2 }
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

// Get the articulation profile for a style and role
inline const ArticulationProfile& getArticulationProfile(int styleIndex, int roleIndex) {
    styleIndex = std::clamp(styleIndex, 0, 9);
    roleIndex = std::clamp(roleIndex, 0, 3);
    return ARTICULATION_PROFILES[styleIndex][roleIndex];
}

// Select an articulation type based on profile and amount parameter
// Returns NORMAL if no articulation should be applied
inline ArticulationType selectArticulation(int styleIndex, int roleIndex,
                                           float amount, bool isAccent,
                                           bool isStrongBeat) {
    if (amount < 0.01f) return ArticulationType::NORMAL;

    const ArticulationProfile& profile = getArticulationProfile(styleIndex, roleIndex);

    // Calculate total probability for normalization
    float totalProb = 0.0f;
    for (int i = 0; i < profile.numEntries; i++) {
        const ArticulationEntry& entry = profile.entries[i];
        if (entry.type == ArticulationType::NORMAL) continue;

        // Skip if constraints not met
        if (entry.onAccentsOnly && !isAccent) continue;

        // Calculate effective probability
        float prob = entry.baseProbability * amount;

        // Boost if on strong beat and entry prefers strong beats
        if (entry.onStrongBeats && isStrongBeat) {
            prob *= 1.5f;
        }

        totalProb += prob;
    }

    // Random selection
    float r = static_cast<float>(rand()) / RAND_MAX;

    // If random > total probability, return NORMAL (no articulation)
    if (r > totalProb) return ArticulationType::NORMAL;

    // Otherwise, select an articulation proportionally
    float cumulative = 0.0f;
    for (int i = 0; i < profile.numEntries; i++) {
        const ArticulationEntry& entry = profile.entries[i];
        if (entry.type == ArticulationType::NORMAL) continue;
        if (entry.onAccentsOnly && !isAccent) continue;

        float prob = entry.baseProbability * amount;
        if (entry.onStrongBeats && isStrongBeat) {
            prob *= 1.5f;
        }

        cumulative += prob;
        if (r <= cumulative) {
            return entry.type;
        }
    }

    return ArticulationType::NORMAL;
}

// Get articulation name for display
inline const char* getArticulationName(ArticulationType type) {
    switch (type) {
        case ArticulationType::NORMAL: return "Normal";
        case ArticulationType::GHOST: return "Ghost";
        case ArticulationType::ACCENT: return "Accent";
        case ArticulationType::RIM: return "Rim";
        case ArticulationType::CROSS: return "Cross";
        case ArticulationType::FLAM: return "Flam";
        case ArticulationType::DRAG: return "Drag";
        case ArticulationType::BUZZ: return "Buzz";
        case ArticulationType::DEAD: return "Dead";
        case ArticulationType::RUFF: return "Ruff";
        case ArticulationType::PARADIDDLE: return "Paradiddle";
        default: return "Normal";
    }
}

} // namespace WorldRhythm
