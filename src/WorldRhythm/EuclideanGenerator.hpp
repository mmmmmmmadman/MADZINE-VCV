#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Euclidean Rhythm Generator
// ========================================
// Based on Bjorklund's algorithm for generating maximally even rhythms
// Reference: unified_rhythm_analysis.md Section 8
//
// Key insight from research:
// - Euclidean is a STARTING POINT tool, not a complete solution
// - Can generate: Tresillo E(3,8), Cinquillo E(5,8), Standard Bell E(7,12)
// - Cannot generate: asymmetric claves, Amen variations, swing/microtiming

struct EuclideanPattern {
    int onsets;         // k: number of hits
    int steps;          // n: total steps
    int rotation;       // Starting offset
    std::vector<bool> pattern;
    std::string matchesTraditional;  // Name of matching traditional pattern
};

// ========================================
// Bjorklund's Algorithm Implementation
// ========================================
inline std::vector<bool> bjorklund(int k, int n) {
    if (k >= n) {
        return std::vector<bool>(n, true);
    }
    if (k <= 0) {
        return std::vector<bool>(n, false);
    }

    // Initialize groups
    std::vector<std::vector<bool>> groups;
    for (int i = 0; i < k; i++) {
        groups.push_back({true});
    }
    for (int i = k; i < n; i++) {
        groups.push_back({false});
    }

    // Iteratively distribute remainders
    while (true) {
        int numOnes = 0;
        int numZeros = 0;

        for (const auto& g : groups) {
            if (g.size() > 0 && g[0]) numOnes++;
            else numZeros++;
        }

        if (numZeros <= 1) break;

        int minSize = std::min(numOnes, numZeros);

        std::vector<std::vector<bool>> newGroups;

        // Combine ones with zeros
        for (int i = 0; i < minSize; i++) {
            std::vector<bool> combined = groups[i];
            combined.insert(combined.end(),
                          groups[numOnes + i].begin(),
                          groups[numOnes + i].end());
            newGroups.push_back(combined);
        }

        // Add remaining groups
        for (int i = minSize; i < numOnes; i++) {
            newGroups.push_back(groups[i]);
        }
        for (int i = numOnes + minSize; i < static_cast<int>(groups.size()); i++) {
            newGroups.push_back(groups[i]);
        }

        groups = newGroups;
    }

    // Flatten groups into result
    std::vector<bool> result;
    for (const auto& g : groups) {
        for (bool b : g) {
            result.push_back(b);
        }
    }

    return result;
}

// ========================================
// Euclidean Generator Class
// ========================================
class EuclideanGenerator {
private:
    std::mt19937 rng;

    // Known traditional pattern matches
    struct TraditionalMatch {
        int k, n;
        std::string name;
    };

    std::vector<TraditionalMatch> traditionalMatches = {
        {3, 8, "Tresillo"},
        {5, 8, "Cinquillo"},
        {7, 12, "Standard Bell"},
        {5, 16, "Bossa Nova"},
        {4, 9, "Aksak (Turkish)"},
        {3, 4, "Cumbia"},
        {5, 6, "Bendir"},
        {7, 8, "Tuareg"},
        {9, 16, "West African"},
        {11, 16, "Dense African"},
        {4, 12, "Fume Fume"},
        {5, 12, "South African"},
    };

public:
    EuclideanGenerator() : rng(std::random_device{}()) {}

    void seed(unsigned int s) { rng.seed(s); }

    // ========================================
    // Generate basic Euclidean rhythm
    // ========================================
    EuclideanPattern generate(int k, int n, int rotation = 0) {
        EuclideanPattern ep;
        ep.onsets = k;
        ep.steps = n;
        ep.rotation = rotation;

        // Generate base pattern
        ep.pattern = bjorklund(k, n);

        // Apply rotation
        if (rotation != 0) {
            std::vector<bool> rotated(n);
            for (int i = 0; i < n; i++) {
                int srcIdx = (i - rotation + n) % n;
                rotated[i] = ep.pattern[srcIdx];
            }
            ep.pattern = rotated;
        }

        // Check for traditional match
        ep.matchesTraditional = findTraditionalMatch(k, n);

        return ep;
    }

    // ========================================
    // Convert Euclidean pattern to Pattern object
    // ========================================
    Pattern toPattern(const EuclideanPattern& ep, int targetLength, float intensity) {
        Pattern p(targetLength);
        std::uniform_real_distribution<float> velVar(-0.05f, 0.05f);

        for (int i = 0; i < targetLength; i++) {
            // Map target position to Euclidean pattern
            int epIdx = (i * ep.steps) / targetLength;
            epIdx = epIdx % ep.steps;

            if (ep.pattern[epIdx]) {
                // Velocity based on position in pattern
                float posInCycle = static_cast<float>(i) / targetLength;
                float vel = intensity * (0.7f + 0.3f * std::cos(posInCycle * 2 * M_PI)) + velVar(rng);
                p.setOnset(i, std::clamp(vel, 0.3f, 1.0f));

                // Accent on first beat
                if (i == 0) p.accents[i] = true;
            }
        }

        return p;
    }

    // ========================================
    // Generate Pattern directly
    // ========================================
    Pattern generatePattern(int k, int n, int targetLength,
                           float intensity, int rotation = 0) {
        EuclideanPattern ep = generate(k, n, rotation);
        return toPattern(ep, targetLength, intensity);
    }

    // ========================================
    // Find traditional pattern match
    // ========================================
    std::string findTraditionalMatch(int k, int n) const {
        for (const auto& tm : traditionalMatches) {
            if (tm.k == k && tm.n == n) {
                return tm.name;
            }
        }
        return "";
    }

    // ========================================
    // Get common Euclidean patterns
    // ========================================
    std::vector<std::pair<int, int>> getCommonPatterns() const {
        return {
            {3, 8},   // Tresillo
            {5, 8},   // Cinquillo
            {7, 12},  // Standard Bell
            {5, 16},  // Bossa Nova
            {4, 12},  // Fume Fume
            {9, 16},  // Dense African
            {3, 4},   // Cumbia
            {7, 8},   // Tuareg
        };
    }

    // ========================================
    // Generate complementary Euclidean pair
    // (useful for interlocking patterns)
    // ========================================
    std::pair<Pattern, Pattern> generateComplementaryPair(int k, int n,
                                                          int targetLength,
                                                          float intensity) {
        EuclideanPattern ep1 = generate(k, n, 0);

        // Complementary: hits where the first is empty
        std::vector<bool> complement(n);
        for (int i = 0; i < n; i++) {
            complement[i] = !ep1.pattern[i];
        }

        EuclideanPattern ep2;
        ep2.onsets = n - k;
        ep2.steps = n;
        ep2.rotation = 0;
        ep2.pattern = complement;

        return {
            toPattern(ep1, targetLength, intensity),
            toPattern(ep2, targetLength, intensity * 0.7f)
        };
    }

    // ========================================
    // Apply Euclidean constraint to existing pattern
    // ========================================
    void applyEuclideanConstraint(Pattern& p, int k, int n, float strength) {
        EuclideanPattern ep = generate(k, n);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < p.length; i++) {
            int epIdx = (i * n) / p.length;
            epIdx = epIdx % n;
            bool shouldHaveOnset = ep.pattern[epIdx];

            if (p.hasOnsetAt(i)) {
                if (!shouldHaveOnset) {
                    // Has onset but Euclidean says no: reduce velocity
                    float vel = p.getVelocity(i);
                    p.setOnset(i, vel * (1.0f - strength * 0.5f));
                }
            } else {
                if (shouldHaveOnset && dist(rng) < strength * 0.5f) {
                    // No onset but Euclidean says yes: maybe add
                    p.setOnset(i, 0.6f * strength);
                }
            }
        }
    }

    // ========================================
    // Get recommended Euclidean for style
    // ========================================
    std::pair<int, int> getStyleEuclidean(int styleIndex) const {
        switch (styleIndex) {
            case 0:  // West African
                return {7, 12};  // Standard Bell
            case 1:  // Afro-Cuban
                return {5, 8};   // Cinquillo
            case 2:  // Brazilian
                return {5, 16};  // Bossa Nova
            case 3:  // Balkan
                return {4, 9};   // Aksak
            case 4:  // Indian
                return {7, 16};  // Tabla-like
            case 5:  // Gamelan
                return {4, 12};  // Fume Fume
            case 6:  // Jazz
                return {5, 8};   // Cinquillo base
            case 7:  // Electronic
                return {4, 16};  // Four on floor
            case 8:  // Breakbeat
                return {9, 16};  // Dense
            case 9:  // Techno
                return {4, 16};  // Four on floor
            default:
                return {3, 8};   // Tresillo
        }
    }

    // ========================================
    // Visualize pattern as string
    // ========================================
    std::string visualize(const EuclideanPattern& ep) const {
        std::string result;
        for (bool b : ep.pattern) {
            result += b ? "X" : ".";
        }
        return result;
    }

    // ========================================
    // Calculate density
    // ========================================
    float getDensity(int k, int n) const {
        return static_cast<float>(k) / n;
    }

    // ========================================
    // Generate variations
    // ========================================
    std::vector<Pattern> generateVariations(int k, int n, int targetLength,
                                           float intensity, int numVariations) {
        std::vector<Pattern> variations;

        for (int i = 0; i < numVariations; i++) {
            int rotation = (i * n) / numVariations;
            variations.push_back(generatePattern(k, n, targetLength, intensity, rotation));
        }

        return variations;
    }
};

} // namespace WorldRhythm
