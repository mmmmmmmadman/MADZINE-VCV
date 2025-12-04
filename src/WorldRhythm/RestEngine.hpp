#pragma once

#include <vector>
#include <random>
#include <cmath>
#include "PatternGenerator.hpp"
#include "StyleProfiles.hpp"

namespace WorldRhythm {

// ========================================
// Position-Weighted Rest Engine
// ========================================
// Based on module_design.md Section 7
// Rest creates rhythmic silence by "punching holes" in patterns
// Different from density: density decides how many onsets to generate,
// rest decides which onsets to silence

struct RestProfile {
    float roleMaxRest[4];       // Maximum rest per role: Timeline, Foundation, Groove, Lead
    float strongBeatProtection; // Multiplier for strong beat rest probability (0.3 = 70% protection)
    float weakBeatBoost;        // Multiplier for weak beat rest probability (1.5 = 50% more likely)
    float accentProtection;     // Multiplier for accented notes (0.5 = 50% protection)
    float clusterProbability;   // Probability of consecutive rests
};

// ========================================
// Style-Specific Rest Profiles
// ========================================
inline RestProfile createWestAfricanRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.15f;    // Timeline very protected
    p.roleMaxRest[FOUNDATION] = 0.30f;
    p.roleMaxRest[GROOVE] = 0.80f;
    p.roleMaxRest[LEAD] = 1.00f;        // Lead can have full rest
    p.strongBeatProtection = 0.2f;
    p.weakBeatBoost = 1.5f;
    p.accentProtection = 0.4f;
    p.clusterProbability = 0.3f;        // Some clustering for call-response space
    return p;
}

inline RestProfile createAfroCubanRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.05f;    // Clave is sacred
    p.roleMaxRest[FOUNDATION] = 0.35f;
    p.roleMaxRest[GROOVE] = 0.70f;
    p.roleMaxRest[LEAD] = 1.00f;
    p.strongBeatProtection = 0.25f;
    p.weakBeatBoost = 1.4f;
    p.accentProtection = 0.3f;          // Clave positions very protected
    p.clusterProbability = 0.25f;
    return p;
}

inline RestProfile createBrazilianRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.20f;
    p.roleMaxRest[FOUNDATION] = 0.25f;  // Surdo anchor important
    p.roleMaxRest[GROOVE] = 0.75f;
    p.roleMaxRest[LEAD] = 0.90f;
    p.strongBeatProtection = 0.3f;
    p.weakBeatBoost = 1.3f;
    p.accentProtection = 0.4f;
    p.clusterProbability = 0.2f;
    return p;
}

inline RestProfile createBalkanRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.20f;
    p.roleMaxRest[FOUNDATION] = 0.35f;
    p.roleMaxRest[GROOVE] = 0.70f;
    p.roleMaxRest[LEAD] = 0.85f;
    p.strongBeatProtection = 0.25f;     // Group boundaries important
    p.weakBeatBoost = 1.4f;
    p.accentProtection = 0.35f;
    p.clusterProbability = 0.3f;
    return p;
}

inline RestProfile createIndianRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.25f;
    p.roleMaxRest[FOUNDATION] = 0.40f;
    p.roleMaxRest[GROOVE] = 0.65f;
    p.roleMaxRest[LEAD] = 0.80f;
    p.strongBeatProtection = 0.15f;     // Sam/Tali very protected
    p.weakBeatBoost = 1.6f;             // Khali positions more likely to rest
    p.accentProtection = 0.3f;
    p.clusterProbability = 0.4f;        // Tihai-like phrase boundaries
    return p;
}

inline RestProfile createGamelanRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.15f;    // Colotomic structure protected
    p.roleMaxRest[FOUNDATION] = 0.10f;  // Gong very important
    p.roleMaxRest[GROOVE] = 0.60f;
    p.roleMaxRest[LEAD] = 0.70f;
    p.strongBeatProtection = 0.2f;
    p.weakBeatBoost = 1.3f;
    p.accentProtection = 0.25f;
    p.clusterProbability = 0.5f;        // Angsel-style synchronized rest
    return p;
}

inline RestProfile createJazzRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.25f;
    p.roleMaxRest[FOUNDATION] = 0.45f;
    p.roleMaxRest[GROOVE] = 0.80f;
    p.roleMaxRest[LEAD] = 1.00f;
    p.strongBeatProtection = 0.35f;
    p.weakBeatBoost = 1.2f;             // Less predictable
    p.accentProtection = 0.5f;
    p.clusterProbability = 0.2f;        // Random, not clustered
    return p;
}

inline RestProfile createElectronicRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.30f;
    p.roleMaxRest[FOUNDATION] = 0.20f;  // Four-on-floor protected
    p.roleMaxRest[GROOVE] = 0.50f;
    p.roleMaxRest[LEAD] = 0.90f;
    p.strongBeatProtection = 0.2f;
    p.weakBeatBoost = 1.5f;
    p.accentProtection = 0.4f;
    p.clusterProbability = 0.4f;        // Stutter/glitch effect
    return p;
}

inline RestProfile createBreakbeatRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.35f;
    p.roleMaxRest[FOUNDATION] = 0.40f;
    p.roleMaxRest[GROOVE] = 0.70f;
    p.roleMaxRest[LEAD] = 0.85f;
    p.strongBeatProtection = 0.3f;
    p.weakBeatBoost = 1.4f;
    p.accentProtection = 0.45f;
    p.clusterProbability = 0.35f;
    return p;
}

inline RestProfile createTechnoRestProfile() {
    RestProfile p;
    p.roleMaxRest[TIMELINE] = 0.25f;
    p.roleMaxRest[FOUNDATION] = 0.15f;  // Kick is king
    p.roleMaxRest[GROOVE] = 0.45f;
    p.roleMaxRest[LEAD] = 0.80f;
    p.strongBeatProtection = 0.15f;
    p.weakBeatBoost = 1.6f;
    p.accentProtection = 0.35f;
    p.clusterProbability = 0.45f;       // Minimal aesthetic
    return p;
}

// ========================================
// Rest Engine Class
// ========================================
class RestEngine {
private:
    std::mt19937 rng;
    std::vector<RestProfile> profiles;
    int currentProfileIndex = 0;

public:
    RestEngine() : rng(std::random_device{}()) {
        profiles.push_back(createWestAfricanRestProfile());
        profiles.push_back(createAfroCubanRestProfile());
        profiles.push_back(createBrazilianRestProfile());
        profiles.push_back(createBalkanRestProfile());
        profiles.push_back(createIndianRestProfile());
        profiles.push_back(createGamelanRestProfile());
        profiles.push_back(createJazzRestProfile());
        profiles.push_back(createElectronicRestProfile());
        profiles.push_back(createBreakbeatRestProfile());
        profiles.push_back(createTechnoRestProfile());
    }

    void seed(unsigned int s) { rng.seed(s); }

    void setStyle(int styleIndex) {
        if (styleIndex >= 0 && styleIndex < static_cast<int>(profiles.size())) {
            currentProfileIndex = styleIndex;
        }
    }

    const RestProfile& getCurrentProfile() const {
        return profiles[currentProfileIndex];
    }

    // ========================================
    // Calculate rest probability for a position
    // ========================================
    float getRestProbability(int position, int patternLength, Role role,
                             float restAmount, bool isAccented) const {
        const RestProfile& profile = profiles[currentProfileIndex];

        // Base probability from user control
        float prob = restAmount;

        // Apply role maximum
        float roleMax = profile.roleMaxRest[role];

        // Strong beat detection (positions 0, 4, 8, 12 in 16-grid)
        int pos16 = (position * 16) / patternLength;
        bool isStrongBeat = (pos16 % 4 == 0);
        bool isWeakSubdivision = (pos16 % 2 == 1);

        // Apply position modifiers
        if (isStrongBeat) {
            prob *= profile.strongBeatProtection;
        }
        if (isWeakSubdivision) {
            prob *= profile.weakBeatBoost;
        }

        // Apply accent protection
        if (isAccented) {
            prob *= profile.accentProtection;
        }

        // Clamp to role maximum
        return std::min(prob, roleMax);
    }

    // ========================================
    // Apply rest to pattern with clustering
    // ========================================
    void applyRest(Pattern& p, Role role, float restAmount) {
        if (restAmount <= 0.0f) return;

        const RestProfile& profile = profiles[currentProfileIndex];
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        bool previousWasRest = false;

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            // Calculate base probability
            float prob = getRestProbability(i, p.length, role, restAmount, p.accents[i]);

            // Apply clustering
            if (previousWasRest && restAmount > 0.3f) {
                float clusterBoost = profile.clusterProbability *
                                    (restAmount > 0.6f ? 1.5f : 1.0f);
                prob = std::min(prob + clusterBoost, profile.roleMaxRest[role]);
            }

            // Decide if rest
            if (dist(rng) < prob) {
                p.clearOnset(i);
                previousWasRest = true;
            } else {
                previousWasRest = false;
            }
        }
    }

    // ========================================
    // Apply synchronized rest (Angsel-style)
    // All patterns rest at the same positions
    // ========================================
    std::vector<int> generateSynchronizedRestPositions(int patternLength,
                                                        float restAmount,
                                                        int numPositions) {
        std::vector<int> positions;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Prefer weak beats for synchronized rest
        std::vector<int> candidates;
        for (int i = 0; i < patternLength; i++) {
            int pos16 = (i * 16) / patternLength;
            if (pos16 % 4 != 0) {  // Not on strong beats
                candidates.push_back(i);
            }
        }

        // Randomly select positions
        int toSelect = std::min(numPositions, static_cast<int>(candidates.size()));
        for (int i = 0; i < toSelect; i++) {
            if (dist(rng) < restAmount) {
                int idx = static_cast<int>(dist(rng) * candidates.size());
                idx = std::min(idx, static_cast<int>(candidates.size()) - 1);
                positions.push_back(candidates[idx]);
                candidates.erase(candidates.begin() + idx);
            }
        }

        return positions;
    }

    // ========================================
    // Apply breakdown rest pattern
    // Groove/Lead increase rest, Timeline/Foundation maintain
    // ========================================
    void applyBreakdown(Pattern& timeline, Pattern& foundation,
                        Pattern& groove, Pattern& lead,
                        float breakdownIntensity) {
        // Timeline: minimal rest
        applyRest(timeline, TIMELINE, breakdownIntensity * 0.1f);

        // Foundation: light rest
        applyRest(foundation, FOUNDATION, breakdownIntensity * 0.2f);

        // Groove: moderate rest
        applyRest(groove, GROOVE, breakdownIntensity * 0.6f);

        // Lead: heavy rest
        applyRest(lead, LEAD, breakdownIntensity * 0.8f);
    }

    // ========================================
    // Apply buildup rest pattern (reverse of breakdown)
    // Start with high rest, decrease toward climax
    // ========================================
    float getBuildupRestAmount(float baseRest, int currentBar, int totalBars) {
        float progress = static_cast<float>(currentBar) / totalBars;
        // Exponential decrease
        return baseRest * std::pow(1.0f - progress, 2.0f);
    }

    // ========================================
    // Call-Response rest pattern
    // Alternate high/low rest between two groups
    // ========================================
    void applyCallResponse(Pattern& caller, Pattern& responder,
                           float restAmount, bool callerActive) {
        if (callerActive) {
            applyRest(caller, LEAD, restAmount * 0.2f);      // Caller active
            applyRest(responder, GROOVE, restAmount * 0.8f); // Responder quiet
        } else {
            applyRest(caller, LEAD, restAmount * 0.8f);      // Caller quiet
            applyRest(responder, GROOVE, restAmount * 0.2f); // Responder active
        }
    }

    // ========================================
    // Humanize rest (5-15% for natural breathing)
    // ========================================
    void applyHumanizeRest(Pattern& p, Role role) {
        std::uniform_real_distribution<float> dist(0.05f, 0.15f);
        float humanRest = dist(rng);
        applyRest(p, role, humanRest);
    }

    // ========================================
    // Enhanced Clustering Rest (v0.14)
    // Creates more natural phrase-based rest clusters
    // ========================================
    void applyClusteredRest(Pattern& p, Role role, float restAmount,
                            int minClusterSize = 2, int maxClusterSize = 4) {
        if (restAmount <= 0.0f) return;

        const RestProfile& profile = profiles[currentProfileIndex];
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_int_distribution<int> clusterDist(minClusterSize, maxClusterSize);

        int i = 0;
        while (i < p.length) {
            if (!p.hasOnsetAt(i)) {
                i++;
                continue;
            }

            // Calculate base probability
            float prob = getRestProbability(i, p.length, role, restAmount, p.accents[i]);

            // Decide if starting a rest cluster
            if (dist(rng) < prob) {
                int clusterSize = clusterDist(rng);
                int restCount = 0;
                int actualProcessed = 0;  // v0.19: 追蹤實際處理的位置數

                // Apply rest cluster
                for (int j = i; j < std::min(i + clusterSize, p.length); j++) {
                    actualProcessed++;  // 記錄已處理的位置

                    if (!p.hasOnsetAt(j)) continue;

                    // Strong beats can break cluster
                    int pos16 = (j * 16) / p.length;
                    if (pos16 % 4 == 0 && restCount > 0) {
                        // 30% chance to break cluster at strong beat
                        if (dist(rng) < 0.3f) break;
                    }

                    // Accented notes harder to rest
                    if (p.accents[j] && dist(rng) > profile.accentProtection) {
                        continue;  // Skip this note in cluster
                    }

                    p.clearOnset(j);
                    restCount++;
                }

                // v0.19: 使用實際處理的數量來前進，確保至少前進 1 步
                i += std::max(1, actualProcessed);
            } else {
                i++;
            }
        }
    }

    // ========================================
    // Angsel Synchronized Rest (v0.14)
    // Gamelan-style coordinated silence across all patterns
    // ========================================
    void applyAngselRest(std::vector<Pattern*>& patterns,
                         const std::vector<Role>& roles,
                         float angselIntensity,
                         int angselPosition) {
        if (patterns.size() != roles.size()) return;

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Angsel affects a window around the position
        int windowSize = static_cast<int>(angselIntensity * 4) + 1;  // 1-5 steps

        for (size_t p = 0; p < patterns.size(); p++) {
            Pattern* pat = patterns[p];
            Role role = roles[p];

            for (int i = angselPosition; i < std::min(angselPosition + windowSize, pat->length); i++) {
                if (!pat->hasOnsetAt(i)) continue;

                // Role-based angsel participation
                float participationProb;
                switch (role) {
                    case TIMELINE:
                        participationProb = 0.3f;  // Timeline often maintains
                        break;
                    case FOUNDATION:
                        participationProb = 0.7f;  // Foundation participates more
                        break;
                    case GROOVE:
                    case LEAD:
                        participationProb = 0.9f;  // Full participation
                        break;
                    default:
                        participationProb = 0.8f;
                }

                if (dist(rng) < participationProb * angselIntensity) {
                    pat->clearOnset(i);
                }
            }
        }
    }

    // ========================================
    // Phrase Boundary Rest (v0.14)
    // Creates natural breathing space at phrase boundaries
    // ========================================
    void applyPhraseBoundaryRest(Pattern& p, Role role, float restAmount,
                                  int phraseLength = 8) {
        if (restAmount <= 0.0f) return;

        const RestProfile& profile = profiles[currentProfileIndex];
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            // Position within phrase
            int phrasePos = i % phraseLength;

            // Last 1-2 positions of phrase more likely to rest
            float boundaryBoost = 1.0f;
            if (phrasePos == phraseLength - 1) {
                boundaryBoost = 2.0f;  // Last position
            } else if (phrasePos == phraseLength - 2) {
                boundaryBoost = 1.5f;  // Second to last
            }

            float prob = getRestProbability(i, p.length, role, restAmount, p.accents[i]);
            prob *= boundaryBoost;
            prob = std::min(prob, profile.roleMaxRest[role]);

            if (dist(rng) < prob) {
                p.clearOnset(i);
            }
        }
    }

    // ========================================
    // Density-Aware Rest (v0.14)
    // Applies more rest to denser areas, less to sparse areas
    // ========================================
    void applyDensityAwareRest(Pattern& p, Role role, float restAmount) {
        if (restAmount <= 0.0f) return;

        const RestProfile& profile = profiles[currentProfileIndex];
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Calculate local density for each position
        int windowSize = 4;
        std::vector<float> localDensity(p.length, 0.0f);

        for (int i = 0; i < p.length; i++) {
            int count = 0;
            for (int j = -windowSize; j <= windowSize; j++) {
                int idx = (i + j + p.length) % p.length;
                if (p.hasOnsetAt(idx)) count++;
            }
            localDensity[i] = static_cast<float>(count) / (windowSize * 2 + 1);
        }

        // Apply rest based on local density
        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            // Higher density = more likely to rest
            float densityMultiplier = 0.5f + localDensity[i];
            float prob = getRestProbability(i, p.length, role, restAmount, p.accents[i]);
            prob *= densityMultiplier;
            prob = std::min(prob, profile.roleMaxRest[role]);

            if (dist(rng) < prob) {
                p.clearOnset(i);
            }
        }
    }

    // ========================================
    // Get Rest Statistics
    // ========================================
    float getRestPercentage(const Pattern& p) const {
        int total = 0;

        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                total++;
            }
        }

        // Compare with a theoretical "full" pattern
        float density = static_cast<float>(total) / p.length;
        return 1.0f - density;
    }
};

} // namespace WorldRhythm
