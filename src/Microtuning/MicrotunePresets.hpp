#pragma once
#include <string>

namespace Microtuning {

// Microtune presets in cents (C C# D D# E F F# G G# A A# B)
// Format: {C, C#, D, D#, E, F, F#, G, G#, A, A#, B}

const float PRESETS[][12] = {
    // Western (0-3)
    {0,0,0,0,0,0,0,0,0,0,0,0},                           // 0: Equal Temperament
    {0,11.73,3.91,15.64,-13.69,-1.96,-9.78,1.96,13.69,-15.64,17.60,-11.73}, // 1: Just Intonation
    {0,13.69,3.91,-5.87,7.82,-1.96,11.73,1.96,15.64,5.87,-3.91,9.78},       // 2: Pythagorean
    {0,50,0,50,0,0,50,0,50,0,50,0},                      // 3: Quarter-tone
    // Arabic Maqam (4-9)
    {0,0,0,0,-50,0,0,0,0,0,0,-50},                       // 4: Rast
    {0,0,0,0,-50,0,0,0,0,-50,0,0},                       // 5: Bayati
    {0,0,0,0,-100,0,0,0,0,0,-100,0},                     // 6: Hijaz
    {0,0,0,-50,-50,0,-100,0,0,-50,0,0},                  // 7: Saba
    {0,0,0,0,0,0,0,0,0,0,0,0},                           // 8: Nahawand
    {0,0,0,0,0,0,0,0,0,0,0,0},                           // 9: Kurd
    // Turkish Makam (10-13)
    {0,0,0,0,-12,0,0,0,0,0,0,-12},                       // 10: Rast
    {0,0,0,-45,0,0,0,0,0,-45,0,0},                       // 11: Ussak
    {0,0,0,0,-90,0,12,0,0,0,-90,0},                      // 12: Hicaz
    {0,0,0,-45,0,0,0,0,-45,0,0,0},                       // 13: Segah
    // Persian (14-15)
    {0,0,0,-55,0,0,0,0,-55,0,-100,0},                    // 14: Shur
    {0,0,0,-55,0,0,0,0,-55,0,0,0},                       // 15: Segah
    // Indian (16-19)
    {0,-10,14,-16,14,2,-10,-2,-14,16,-18,12},            // 16: Shruti
    {0,-10,0,0,8,-2,0,2,-8,0,0,-12},                     // 17: Bhairav
    {0,0,4,0,8,0,12,2,0,6,0,10},                         // 18: Yaman
    {0,-10,0,-6,0,-2,0,2,-8,0,-4,0},                     // 19: Bhairavi
    // Japanese (20-23)
    {0,0,4,0,-14,-2,0,2,0,-16,0,-12},                    // 20: Gagaku
    {0,-14,0,0,0,-2,0,2,-14,0,0,0},                      // 21: In Scale
    {0,0,4,0,0,-2,0,2,0,-16,0,0},                        // 22: Yo Scale
    {0,0,0,0,-14,-2,0,2,0,0,0,-12},                      // 23: Ryukyu
    // Southeast Asian (24-26)
    {0,0,40,0,-20,20,0,0,40,0,-20,20},                   // 24: Slendro
    {0,20,-30,0,40,-30,0,0,30,-20,0,50},                 // 25: Pelog
    {0,0,-29,0,-57,14,0,-14,0,-43,0,29},                 // 26: Thai 7-TET
    // Chinese (27)
    {0,0,4,0,-14,0,0,2,0,-16,0,0}                        // 27: Pentatonic
};

// Directional presets: {ascending, descending}
const float DIRECTIONAL[][2][12] = {
    // Turkish Rast: ascending uses F#, descending uses F
    {{0,0,0,0,-12,0,0,0,0,0,0,-12}, {0,0,0,0,-12,0,-100,0,0,0,0,-12}},
    // Arabic Hijaz: descending lowers F#
    {{0,0,0,0,-100,0,0,0,0,0,-100,0}, {0,0,0,0,-100,0,-25,0,0,0,-100,0}},
    // Japanese Miyako-bushi: ascending D, descending C
    {{0,-14,0,0,0,-2,0,2,-14,0,0,0}, {0,-14,-100,0,0,-2,0,2,-14,0,0,0}}
};

const int PRESET_COUNT = 28;
const int DIRECTIONAL_COUNT = 3;

inline const float* getPreset(int i) { return (i >= 0 && i < PRESET_COUNT) ? PRESETS[i] : PRESETS[0]; }
inline const float* getAscending(int i) { return (i >= 0 && i < DIRECTIONAL_COUNT) ? DIRECTIONAL[i][0] : PRESETS[0]; }
inline const float* getDescending(int i) { return (i >= 0 && i < DIRECTIONAL_COUNT) ? DIRECTIONAL[i][1] : PRESETS[0]; }

} // namespace Microtuning
