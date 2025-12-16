#pragma once

namespace WorldRhythm {

// ========================================
// Swing Ratio Academic Reference (v0.20)
// ========================================
// Friberg, A., & Sundström, A. (2002). Swing Ratios and Ensemble
// Timing in Jazz Performance: Evidence for a Common Rhythmic Pattern.
// Music Perception, 19(3), 333-349.
//
// Key findings:
// - Slow tempo (~120 BPM): swing ratio up to 3.5:1 (0.78)
// - Medium tempo: 2.0:1 (triplet feel, 0.67)
// - Fast tempo (300+ BPM): approaches 1.0:1 (straight, 0.50)
// - Short note absolute duration ~100ms at medium-fast tempos
//
// Swing ratio values below follow these research findings,
// with style-specific adjustments based on ethnomusicological practice.

struct StyleProfile {
    const char* name;
    float swing;  // 0.5 = straight, 0.67 = triplet (Friberg & Sundström 2002)

    // 16-position weights for each role (0.0 - 1.0)
    float timeline[16];
    float foundation[16];
    float groove[16];
    float lead[16];

    // Density ranges per role
    float timelineDensityMin, timelineDensityMax;
    float foundationDensityMin, foundationDensityMax;
    float grooveDensityMin, grooveDensityMax;
    float leadDensityMin, leadDensityMax;

    // Interlock rules
    bool avoidFoundationOnTimeline;
    bool grooveComplementsFoundation;
};

// ============================================================
// STYLE 0: West African 12/8
// ============================================================
// Based on Standard Bell (Gankogui): X.X.XX.X.X.X (12-pulse)
// 12-pulse positions: 1,3,5,6,8,10,12 mapped to 16-grid
// Hemiola 3:2: 3-feel every 4 pulse (1,5,9,13), 2-feel every 6 pulse (1,7,13)
// Swing: 0.62 (between straight and triplet, 60-65% range)

const StyleProfile WEST_AFRICAN = {
    "West African",
    0.62f,

    // Timeline: Standard Bell mapped to 16-grid
    // 12→16 mapping: pos_16 = round(pos_12 × 16/12)
    // Bell hits: 1→1, 3→4, 5→7, 6→8, 8→11, 10→13, 12→16
    {1.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.9f, 1.0f,
     0.0f, 0.0f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 0.9f},

    // Foundation: Dununba - beat 1 dominant, very sparse (1-2 per cycle)
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

    // Groove: Sangban/Kenkeni with Hemiola 3:2 structure
    // 3-feel accents: positions 1,5,9,13 (every 4 pulses)
    // 2-feel accents: positions 1,7,13 (every 6 pulses in 12→16)
    // Density target: 35-45% (6-7 high weight positions)
    {0.8f, 0.3f, 0.2f, 0.4f, 0.8f, 0.2f, 0.7f, 0.3f,
     0.8f, 0.2f, 0.3f, 0.3f, 0.7f, 0.3f, 0.2f, 0.2f},

    // Lead: Djembe slap/tone, responds to hemiola tension
    {0.4f, 0.5f, 0.6f, 0.4f, 0.7f, 0.5f, 0.6f, 0.5f,
     0.7f, 0.5f, 0.4f, 0.6f, 0.5f, 0.6f, 0.5f, 0.4f},

    // Density ranges
    0.40f, 0.50f,  // Timeline: 40-50% (7 bell hits)
    0.05f, 0.10f,  // Foundation: 5-10% (1-2 per cycle)
    0.35f, 0.45f,  // Groove: 35-45%
    0.20f, 0.35f,  // Lead: 20-35%

    // Interlock
    true,   // Foundation avoids Timeline
    true    // Groove complements Foundation
};

// ============================================================
// STYLE 1: Afro-Cuban
// ============================================================
// Based on Son Clave 3-2: X..X..X...X.X... (16-pulse)
// Clave positions: 1, 4, 7, 11, 13
// Tumbao ponche MUST align to clave positions
// Swing: 0.58 (55-65% range)

const StyleProfile AFRO_CUBAN = {
    "Afro-Cuban",
    0.58f,

    // Timeline: Clave 3-2 positions (1, 4, 7, 11, 13)
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Foundation: Tumbao ponche - clave-aligned (2-4 per cycle)
    // Only hit on clave positions: 1, 4, 7, 11
    // Density: 4/16 = 25%
    {0.9f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

    // Groove: Conga tumbao - syncopated around clave
    // High weight on off-clave positions for tension
    // Density: 35-50% (6-8 positions with high weight)
    {0.2f, 0.7f, 0.6f, 0.2f, 0.8f, 0.4f, 0.2f, 0.7f,
     0.6f, 0.4f, 0.2f, 0.7f, 0.2f, 0.6f, 0.4f, 0.3f},

    // Lead: Quinto improvisation, free positions
    {0.5f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f, 0.5f, 0.6f,
     0.5f, 0.6f, 0.5f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f},

    // Density ranges
    0.30f, 0.35f,  // Timeline: exactly 5 hits (clave)
    0.20f, 0.30f,  // Foundation: 20-30% (clave-aligned)
    0.35f, 0.50f,  // Groove: 35-50%
    0.15f, 0.35f,  // Lead: 15-35%

    // Interlock
    true,   // Foundation (Tumbao) avoids non-clave positions
    true    // Groove complements Foundation
};

// ============================================================
// STYLE 2: Brazilian Samba
// ============================================================
// Based on Surdo pattern and Agogô timeline
// Surdo: beat 2 (position 5) emphasis (Brazilian "1" feel)
// Batucada weave: multi-layer interlock between Caixa/Tamborim
// Swing: 0.57 (55-60% range)

const StyleProfile BRAZILIAN = {
    "Brazilian",
    0.57f,

    // Timeline: Agogô pattern (4-8 strokes)
    {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.7f, 0.0f},

    // Foundation: Surdo - beat 2 (position 5) is king, 1 per bar
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f},

    // Groove: Caixa/Tamborim - busy but controlled (45-60%)
    // Reduced density: 9/16 positions active = 56%
    {0.3f, 0.7f, 0.4f, 0.7f, 0.2f, 0.6f, 0.5f, 0.7f,
     0.3f, 0.7f, 0.4f, 0.7f, 0.2f, 0.6f, 0.5f, 0.7f},

    // Lead: Repinique calls - sparse, call patterns only
    // Density: 20-35% (4-5 high weight positions for calls)
    {0.3f, 0.2f, 0.3f, 0.2f, 0.5f, 0.2f, 0.3f, 0.2f,
     0.3f, 0.2f, 0.3f, 0.2f, 0.5f, 0.2f, 0.3f, 0.2f},

    // Density ranges
    0.25f, 0.35f,  // Timeline: 25-35%
    0.10f, 0.15f,  // Foundation: 10-15% (1 per bar)
    0.45f, 0.55f,  // Groove: 45-55% (busy but controlled)
    0.20f, 0.35f,  // Lead: 20-35%

    // Interlock
    true,   // Foundation avoids Timeline
    true    // Groove complements Foundation
};

// ============================================================
// STYLE 3: Balkan Aksak
// ============================================================
// Asymmetric groupings: 7/8 = 2+2+3 (short-short-long)
// 7→16 mapping: pulse×16/7 = positions
//   Group 1 (2 pulses): 1 → pos 1, 2 → pos 5 (2×16/7≈4.6)
//   Group 2 (2 pulses): 3 → pos 7 (3×16/7≈6.9), 4 → pos 10 (4×16/7≈9.1)
//   Group 3 (3 pulses): 5 → pos 12, 6 → pos 14, 7 → pos 16
// Downbeats: 1, 5, 10 (start of each group)
// Swing: 0.50 (straight, asymmetry is in grouping)

const StyleProfile BALKAN = {
    "Balkan",
    0.50f,

    // Timeline: Asymmetric 2+2+3 downbeats
    // Group boundaries: pos 1 (group1), pos 5 (group2), pos 10 (group3)
    {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

    // Foundation: Downbeats of each 2+2+3 group
    {1.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

    // Groove: Fill within each group (off-downbeat positions)
    // Group1 fill: 2-4, Group2 fill: 6-9, Group3 fill: 11-16
    // Density: 40-50% (6-8 positions with high weight)
    {0.2f, 0.6f, 0.5f, 0.0f, 0.2f, 0.6f, 0.5f, 0.6f,
     0.0f, 0.2f, 0.6f, 0.5f, 0.6f, 0.0f, 0.5f, 0.0f},

    // Lead: Ornamental, responds to asymmetric pulse
    // Density: 25-40% (4-6 high weight positions)
    {0.3f, 0.4f, 0.5f, 0.0f, 0.3f, 0.4f, 0.5f, 0.0f,
     0.5f, 0.3f, 0.4f, 0.5f, 0.0f, 0.5f, 0.0f, 0.4f},

    // Density ranges
    0.15f, 0.25f,  // Timeline: 15-25% (3 downbeats)
    0.15f, 0.20f,  // Foundation: 15-20%
    0.40f, 0.50f,  // Groove: 40-50%
    0.25f, 0.40f,  // Lead: 25-40%

    // Interlock
    true,   // Foundation avoids Timeline
    true    // Groove complements Foundation
};

// ============================================================
// STYLE 4: Indian Tala
// ============================================================
// Based on Teental (16 beats): Dha Dhin Dhin Dha | Dha Dhin Dhin Dha |
//                              Dha Dhin Dhin Dha | Dha Dhin Dhin Dha
// Sam (beat 1) heavily emphasized, Khali (beat 9) is empty/light
// Dha positions: 1, 4, 5, 8, 9(khali), 12, 13, 16 (stronger bass)
// Dhin positions: 2, 3, 6, 7, 10, 11, 14, 15 (lighter, clear)
// Swing: 0.50 (straight)

const StyleProfile INDIAN = {
    "Indian",
    0.50f,

    // Timeline: Teental theka - clear Dha/Dhin contrast
    // Dha (bass): 1(Sam), 4, 5(Tali), 8, 9(Khali-light), 12, 13(Tali), 16
    // Dhin (clear): 2, 3, 6, 7, 10, 11, 14, 15
    {1.0f, 0.5f, 0.5f, 0.8f, 0.9f, 0.5f, 0.5f, 0.8f,
     0.1f, 0.5f, 0.5f, 0.7f, 0.9f, 0.5f, 0.5f, 0.8f},

    // Foundation: Bayan - Sam and Tali points (2-4 per cycle)
    // Sam=1, Tali=5,13
    {1.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f},

    // Groove: Dayan elaboration - follows theka
    {0.3f, 0.6f, 0.6f, 0.4f, 0.3f, 0.6f, 0.6f, 0.4f,
     0.2f, 0.5f, 0.5f, 0.4f, 0.3f, 0.6f, 0.6f, 0.4f},

    // Lead: Tihai preparation, phrase endings (builds toward Sam)
    {0.4f, 0.4f, 0.4f, 0.5f, 0.4f, 0.4f, 0.5f, 0.5f,
     0.3f, 0.4f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f, 0.8f},

    // Density ranges
    0.50f, 0.60f,  // Timeline: 50-60% (theka is busy)
    0.15f, 0.20f,  // Foundation: 15-20% (Sam + Tali)
    0.35f, 0.45f,  // Groove: 35-45%
    0.25f, 0.40f,  // Lead: 25-40%

    // Interlock
    true,   // Foundation avoids Timeline
    true    // Groove (Dayan) complements Foundation (Bayan)
};

// ============================================================
// STYLE 5: Gamelan
// ============================================================
// Colotomic structure: nested gong cycles
// Kotekan interlocking between voices
// Swing: 0.50 (straight)

const StyleProfile GAMELAN = {
    "Gamelan",
    0.50f,

    // Timeline: Colotomic punctuation - sparse gongs only
    // Gong ageng at end (16), Kempul at 5, 13 - total 3 positions
    // Density: 20-30% (sparse punctuation)
    {0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 1.0f},

    // Foundation: Gong ageng - only cycle end
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},

    // Groove: Kotekan polos (on-beat tendency)
    {0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.2f,
     0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.3f},

    // Lead: Kotekan sangsih (off-beat tendency)
    {0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.8f,
     0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.7f},

    // Density ranges
    0.20f, 0.30f,  // Timeline: 20-30% (sparse punctuation)
    0.05f, 0.10f,  // Foundation: 5-10% (very sparse)
    0.40f, 0.55f,  // Groove: 40-55%
    0.40f, 0.55f,  // Lead: 40-55% (kotekan pair)

    // Interlock
    false,  // Foundation is independent (gong ageng)
    true    // Groove/Lead do kotekan interlock (polos-sangsih complement)
};

// ============================================================
// STYLE 6: Jazz Swing
// ============================================================
// Triplet feel, ride cymbal pattern (1, 2&, 3, 4&)
// Kick sparse on 1, 3 only (2-4 per bar)
// Snare comping responds to ride
// Swing: 0.65 (strong swing, 65-70% range)

const StyleProfile JAZZ = {
    "Jazz",
    0.65f,

    // Timeline: Ride pattern (1, 2&, 3, 4&) = positions 1, 4, 5, 8, 9, 12, 13
    // Position 16 should NOT have weight (not part of ride pattern)
    {1.0f, 0.0f, 0.0f, 0.8f, 1.0f, 0.0f, 0.0f, 0.8f,
     1.0f, 0.0f, 0.0f, 0.8f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Foundation: Kick on 1 and 3 ONLY, very sparse (2-4 per bar = 12-25%)
    // Occasional bomb probability kept very low
    {0.9f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
     0.8f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f},

    // Groove: Snare comping, upbeat emphasis - sparse (20-35%)
    // Jazz comping is conversational, not constant
    // High weight only on 2& and 4& (positions 4, 8, 12, 16)
    {0.1f, 0.2f, 0.2f, 0.5f, 0.1f, 0.2f, 0.2f, 0.5f,
     0.1f, 0.2f, 0.2f, 0.5f, 0.1f, 0.2f, 0.2f, 0.4f},

    // Lead: Bombs, fills, interactive - very sparse (10-25%)
    // Jazz lead is minimal, only occasional accents
    {0.2f, 0.2f, 0.2f, 0.3f, 0.2f, 0.2f, 0.2f, 0.3f,
     0.2f, 0.2f, 0.2f, 0.3f, 0.2f, 0.3f, 0.3f, 0.4f},

    // Density ranges (Jazz should be sparse and free)
    0.35f, 0.45f,  // Timeline: 35-45% (ride cymbal)
    0.12f, 0.20f,  // Foundation: 12-20% (kick on 1,3 only)
    0.20f, 0.35f,  // Groove: 20-35% (snare comping)
    0.10f, 0.25f,  // Lead: 10-25% (fills, accents)

    // Interlock
    false,  // Jazz is conversational, not avoidance
    false   // Free interplay
};

// ============================================================
// STYLE 7: Electronic
// ============================================================
// Four-on-floor kick, hi-hat 8ths/16ths
// Snare/clap on 2 and 4
// Swing: 0.50 (straight)

const StyleProfile ELECTRONIC = {
    "Electronic",
    0.50f,

    // Timeline: Hi-hat 8ths or 16ths
    {1.0f, 0.6f, 1.0f, 0.6f, 1.0f, 0.6f, 1.0f, 0.6f,
     1.0f, 0.6f, 1.0f, 0.6f, 1.0f, 0.6f, 1.0f, 0.6f},

    // Foundation: Four-on-floor kick
    {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Groove: Snare/clap on 2 and 4
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Lead: Percussion, syncopation
    // Density: 20-40% (4-6 high weight positions)
    {0.2f, 0.4f, 0.3f, 0.5f, 0.1f, 0.3f, 0.2f, 0.6f,
     0.2f, 0.4f, 0.3f, 0.5f, 0.1f, 0.3f, 0.2f, 0.5f},

    // Density ranges
    0.50f, 0.65f,  // Timeline: 50-65%
    0.25f, 0.30f,  // Foundation: exactly 4 (four-on-floor)
    0.10f, 0.15f,  // Groove: 10-15% (just 2 and 4)
    0.20f, 0.40f,  // Lead: 20-40%

    // Interlock
    false,  // Electronic is grid-locked
    false   // Fixed pattern, no complement
};

// ============================================================
// STYLE 8: Breakbeat
// ============================================================
// Syncopated breaks, amen-style patterns
// DnB 2-step kick: 1, 1a, 3& (positions 1, 4, 11 in 16-grid)
// Swing: 0.52 (nearly straight with slight push)

const StyleProfile BREAKBEAT = {
    "Breakbeat",
    0.52f,

    // Timeline: Syncopated hat pattern - sparse, not all positions
    // Classic breakbeat hats: downbeats + syncopated offbeats
    // 8-10 positions with weight, not 16
    {1.0f, 0.0f, 0.8f, 0.0f, 1.0f, 0.0f, 0.7f, 0.0f,
     1.0f, 0.0f, 0.8f, 0.0f, 1.0f, 0.0f, 0.7f, 0.0f},

    // Foundation: DnB 2-step kick pattern - 3 positions for 15-20%
    // Classic 2-step: positions 1, 9, 15 (drop position 7)
    // X  .  .  .  .  .  .  .  X  .  .  .  .  .  X  .
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f},

    // Groove: Snare on 2 and 4
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Lead: Breakbeat chops, ghost notes
    // Density: 25-40% (4-6 high weight positions)
    {0.2f, 0.4f, 0.3f, 0.5f, 0.1f, 0.4f, 0.3f, 0.6f,
     0.3f, 0.5f, 0.2f, 0.4f, 0.1f, 0.5f, 0.3f, 0.6f},

    // Density ranges
    0.50f, 0.65f,  // Timeline: 50-65%
    0.15f, 0.20f,  // Foundation: 15-20% (2-3 kicks per bar)
    0.10f, 0.15f,  // Groove: 10-15% (snare on 2,4)
    0.25f, 0.40f,  // Lead: 25-40%

    // Interlock
    false,
    false
};

// ============================================================
// STYLE 9: Techno
// ============================================================
// Driving four-on-floor, minimal variation
// Timeline: Hi-hat dense but not 100% (60-75%)
// Lead: Sparse industrial perc (15-30%, NOT 0%)
// Swing: 0.50 (perfectly straight, 0ms humanization)

const StyleProfile TECHNO = {
    "Techno",
    0.50f,

    // Timeline: Hi-hat pattern - dense but with gaps (60-75%)
    // 12/16 positions active = 75%
    {1.0f, 0.8f, 1.0f, 0.0f, 1.0f, 0.8f, 1.0f, 0.0f,
     1.0f, 0.8f, 1.0f, 0.0f, 1.0f, 0.8f, 1.0f, 0.0f},

    // Foundation: Solid four-on-floor
    {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Groove: Minimal clap on 2 and 4
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

    // Lead: Sparse industrial perc (15-25% density)
    // 2-4 syncopated hits per cycle - very sparse
    // Only 3-4 positions with high weight, rest near zero
    {0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f},

    // Density ranges
    0.60f, 0.75f,  // Timeline: 60-75% (dense but not 100%)
    0.25f, 0.25f,  // Foundation: exactly 4 kicks
    0.10f, 0.15f,  // Groove: minimal
    0.15f, 0.25f,  // Lead: sparse industrial (15-25%)

    // Interlock
    false,
    false
};

// ============================================================
// Style Array
// ============================================================

const StyleProfile* const STYLES[] = {
    &WEST_AFRICAN,
    &AFRO_CUBAN,
    &BRAZILIAN,
    &BALKAN,
    &INDIAN,
    &GAMELAN,
    &JAZZ,
    &ELECTRONIC,
    &BREAKBEAT,
    &TECHNO
};

const int NUM_STYLES = 10;

} // namespace WorldRhythm
