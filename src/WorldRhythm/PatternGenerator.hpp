#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include "StyleProfiles.hpp"

namespace WorldRhythm {

enum Role {
    TIMELINE = 0,
    FOUNDATION = 1,
    GROOVE = 2,
    LEAD = 3
};

struct Pattern {
    std::vector<float> velocities;  // 0.0 = no onset, 0.01-1.0 = velocity
    std::vector<bool> accents;
    int length;

    // v0.18.9: 確保 length 至少為 1，防止除零錯誤
    Pattern(int len = 16) :
        velocities(std::max(1, len), 0.0f),
        accents(std::max(1, len), false),
        length(std::max(1, len)) {}

    void clear() {
        std::fill(velocities.begin(), velocities.end(), 0.0f);
        std::fill(accents.begin(), accents.end(), false);
    }

    // v0.18.9: 加入 length > 0 防禦檢查（理論上建構函數已保證，但多一層保護）
    bool hasOnsetAt(int pos) const {
        if (length <= 0) return false;
        return velocities[pos % length] > 0.0f;
    }

    float getVelocity(int pos) const {
        if (length <= 0) return 0.0f;
        return velocities[pos % length];
    }

    void setOnset(int pos, float velocity = 0.7f) {
        if (length <= 0) return;
        velocities[pos % length] = std::clamp(velocity, 0.01f, 1.0f);
    }

    void clearOnset(int pos) {
        if (length <= 0) return;
        velocities[pos % length] = 0.0f;
    }
};

class PatternGenerator {
private:
    std::mt19937 rng;

public:
    PatternGenerator() : rng(std::random_device{}()) {}

    void seed(unsigned int s) {
        rng.seed(s);
    }

    // ========================================
    // Core: Weighted Selection
    // ========================================
    Pattern generate(Role role, const StyleProfile& style, int length, float density, float variation) {
        // Foundation uses skeleton-based generation
        if (role == FOUNDATION) {
            return generateFoundation(style, length, density, variation);
        }

        Pattern p(length);

        // If density is 0, return empty pattern (complete silence)
        if (density < 0.01f) {
            return p;
        }

        std::vector<float> weights(length);

        // 1. Map style weights to pattern length
        // v0.18.3: 使用浮點計算後四捨五入，避免精度損失
        const float* styleWeights = getWeightsForRole(role, style);
        for (int i = 0; i < length; i++) {
            int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
            weights[i] = styleWeights[mapped];

            // 2. Apply variation (blend with uniform)
            weights[i] = weights[i] * (1.0f - variation) + variation;
        }

        // 3. For high density (>0.5), expand available positions while preserving style
        // Add weight to positions adjacent to existing weighted positions
        // This maintains style character while allowing higher density
        if (density > 0.5f) {
            float expansionFactor = (density - 0.5f) * 2.0f;  // 0 at 50%, 1.0 at 100%
            std::vector<float> expandedWeights = weights;

            for (int i = 0; i < length; i++) {
                if (weights[i] < 0.01f) {
                    // Check neighbors for style-appropriate expansion
                    int prev = (i - 1 + length) % length;
                    int next = (i + 1) % length;
                    float neighborWeight = std::max(weights[prev], weights[next]);

                    if (neighborWeight > 0.1f) {
                        // Adjacent to weighted position: inherit reduced weight
                        expandedWeights[i] = neighborWeight * 0.4f * expansionFactor;
                    } else {
                        // Not adjacent: very small weight for extreme density only
                        expandedWeights[i] = 0.1f * expansionFactor;
                    }
                }
            }
            weights = expandedWeights;
        }

        // 4. Calculate target onsets (allow 0 for complete silence)
        int targetOnsets = static_cast<int>(std::round(length * density));

        // 5. Weighted random selection
        weightedSelect(p, weights, targetOnsets);

        return p;
    }

    // ========================================
    // Foundation: Skeleton + Variation
    // Ensures strong downbeats with optional variations
    // ========================================
    Pattern generateFoundation(const StyleProfile& style, int length, float density, float /*variation*/) {
        Pattern p(length);

        // If density is 0, return empty pattern (complete silence)
        if (density < 0.01f) {
            return p;
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Skeleton positions (must-hit downbeats)
        // Position 0 (beat 1) is almost always present
        // Position 8 (beat 3) is common in most styles
        std::vector<int> skeleton;

        // Beat 1 - very high probability
        if (dist(rng) < 0.95f) {
            skeleton.push_back(0);
        }

        // Beat 3 - style dependent
        float beat3Prob = (style.swing > 0.55f) ? 0.7f : 0.85f;  // Swung styles less rigid
        if (dist(rng) < beat3Prob) {
            skeleton.push_back(8);
        }

        // Place skeleton hits with strong velocity
        for (int pos : skeleton) {
            if (pos < length) {
                float vel = 0.85f + velVar(rng);  // Strong: 0.75-0.95
                p.setOnset(pos, std::clamp(vel, 0.75f, 1.0f));
                p.accents[pos] = true;
            }
        }

        // Add variation hits based on density
        // Use style's density range if available, otherwise use provided density
        const float* styleWeights = style.foundation;
        int skeletonCount = static_cast<int>(skeleton.size());
        // Foundation should be sparse - use density directly without boost
        int targetTotal = std::max(skeletonCount, static_cast<int>(std::round(length * density)));
        int additionalHits = targetTotal - skeletonCount;

        // Build effective weights with expansion for high density
        std::vector<float> weights(length);
        for (int i = 0; i < length; i++) {
            int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
            weights[i] = styleWeights[mapped];
        }

        // For high density (>0.5), expand to adjacent positions
        if (density > 0.5f) {
            float expansionFactor = (density - 0.5f) * 2.0f;
            for (int i = 0; i < length; i++) {
                if (weights[i] < 0.01f) {
                    int prev = (i - 1 + length) % length;
                    int next = (i + 1) % length;
                    float neighborWeight = std::max(weights[prev], weights[next]);
                    if (neighborWeight > 0.1f) {
                        weights[i] = neighborWeight * 0.3f * expansionFactor;
                    } else {
                        weights[i] = 0.08f * expansionFactor;
                    }
                }
            }
        }

        for (int n = 0; n < additionalHits; n++) {
            // Find best available position
            float bestWeight = 0.0f;
            int bestPos = -1;

            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) continue;

                float w = weights[i];

                // Reduce weight for positions right after existing hits
                int prev = (i - 1 + length) % length;
                if (p.hasOnsetAt(prev)) w *= 0.3f;

                // Add randomness
                w *= (0.7f + dist(rng) * 0.6f);

                if (w > bestWeight) {
                    bestWeight = w;
                    bestPos = i;
                }
            }

            if (bestPos >= 0 && bestWeight > 0.05f) {
                // Variation hits are softer than skeleton
                float vel = 0.55f + weights[bestPos] * 0.25f + velVar(rng);
                p.setOnset(bestPos, std::clamp(vel, 0.45f, 0.8f));
            }
        }

        return p;
    }

    // ========================================
    // Generate with interlock (avoids reference pattern)
    // ========================================
    Pattern generateWithInterlock(Role role, const StyleProfile& style, int length,
                                   float density, float variation, const Pattern& reference) {
        Pattern p(length);

        // If density is 0, return empty pattern (complete silence)
        if (density < 0.01f) {
            return p;
        }

        std::vector<float> weights(length);

        // v0.18.3: 使用浮點計算後四捨五入，避免精度損失
        const float* styleWeights = getWeightsForRole(role, style);
        for (int i = 0; i < length; i++) {
            int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
            weights[i] = styleWeights[mapped];
            weights[i] = weights[i] * (1.0f - variation) + variation;

            // Interlock: reduce weight where reference has onset
            if (reference.hasOnsetAt(i)) {
                weights[i] *= 0.2f;
            }
            // Boost adjacent positions
            int prev = (i - 1 + length) % length;
            int next = (i + 1) % length;
            if (reference.hasOnsetAt(prev) || reference.hasOnsetAt(next)) {
                weights[i] *= 1.3f;
            }
        }

        // For high density (>0.5), expand to adjacent positions while maintaining interlock
        if (density > 0.5f) {
            float expansionFactor = (density - 0.5f) * 2.0f;
            std::vector<float> expandedWeights = weights;

            for (int i = 0; i < length; i++) {
                if (weights[i] < 0.01f && !reference.hasOnsetAt(i)) {
                    // Only expand to positions not occupied by reference (preserve interlock)
                    int prev = (i - 1 + length) % length;
                    int next = (i + 1) % length;
                    float neighborWeight = std::max(weights[prev], weights[next]);

                    if (neighborWeight > 0.1f) {
                        expandedWeights[i] = neighborWeight * 0.4f * expansionFactor;
                    } else if (!reference.hasOnsetAt(prev) && !reference.hasOnsetAt(next)) {
                        // Only add small weight if neighbors also free
                        expandedWeights[i] = 0.1f * expansionFactor;
                    }
                }
            }
            weights = expandedWeights;
        }

        // Allow 0 for complete silence
        int targetOnsets = static_cast<int>(std::round(length * density));
        weightedSelect(p, weights, targetOnsets);

        return p;
    }

    // ========================================
    // v0.16: Enhanced Interlock Rules
    // ========================================
    // Based on module_design.md specifications:
    // - avoid_foundation_on_timeline: Foundation avoids Timeline positions
    // - groove_complements_foundation: Groove fills between Foundation hits

    struct InterlockConfig {
        bool avoidFoundationOnTimeline;
        bool grooveComplementsFoundation;
        bool leadAvoidsGroove;
        float avoidanceStrength;
        float complementBoost;

        InterlockConfig()
            : avoidFoundationOnTimeline(true)
            , grooveComplementsFoundation(true)
            , leadAvoidsGroove(false)
            , avoidanceStrength(0.2f)
            , complementBoost(1.5f) {}
    };

    // Generate all four roles with proper interlock relationships
    struct RolePatterns {
        Pattern timeline;
        Pattern foundation;
        Pattern groove;
        Pattern lead;
    };

    RolePatterns generateInterlocked(const StyleProfile& style, int length,
                                     float density, float variation,
                                     const InterlockConfig& config) {
        RolePatterns patterns;

        // 1. Timeline first (always the reference)
        patterns.timeline = generate(TIMELINE, style, length, density * 0.8f, variation);

        // 2. Foundation with Timeline avoidance
        if (config.avoidFoundationOnTimeline) {
            patterns.foundation = generateFoundationWithInterlock(style, length, density,
                                                                  variation, patterns.timeline,
                                                                  config.avoidanceStrength);
        } else {
            patterns.foundation = generateFoundation(style, length, density, variation);
        }

        // 3. Groove complementing Foundation
        if (config.grooveComplementsFoundation) {
            patterns.groove = generateGrooveWithComplement(style, length, density,
                                                          variation, patterns.foundation,
                                                          patterns.timeline, config);
        } else {
            patterns.groove = generate(GROOVE, style, length, density, variation);
        }

        // 4. Lead with optional Groove avoidance
        if (config.leadAvoidsGroove) {
            patterns.lead = generateWithInterlock(LEAD, style, length, density * 0.6f,
                                                  variation, patterns.groove);
        } else {
            patterns.lead = generate(LEAD, style, length, density * 0.6f, variation);
        }

        return patterns;
    }

    // Foundation that avoids Timeline positions
    Pattern generateFoundationWithInterlock(const StyleProfile& style, int length,
                                            float density, float /*variation*/,
                                            const Pattern& timeline,
                                            float avoidanceStrength) {
        Pattern p(length);

        // If density is 0, return empty pattern (complete silence)
        if (density < 0.01f) {
            return p;
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        // Skeleton positions
        std::vector<int> skeleton;

        // Beat 1 - check Timeline first
        float beat1Prob = 0.95f;
        if (timeline.hasOnsetAt(0)) {
            beat1Prob *= (1.0f - avoidanceStrength * 0.5f);  // Less avoidance on beat 1
        }
        if (dist(rng) < beat1Prob) {
            skeleton.push_back(0);
        }

        // Beat 3
        int beat3Pos = length / 2;
        float beat3Prob = (style.swing > 0.55f) ? 0.7f : 0.85f;
        if (timeline.hasOnsetAt(beat3Pos)) {
            beat3Prob *= (1.0f - avoidanceStrength);
        }
        if (dist(rng) < beat3Prob) {
            skeleton.push_back(beat3Pos);
        }

        // Place skeleton
        for (int pos : skeleton) {
            if (pos < length) {
                float vel = 0.85f + velVar(rng);
                p.setOnset(pos, std::clamp(vel, 0.75f, 1.0f));
                p.accents[pos] = true;
            }
        }

        // Add variation hits avoiding Timeline
        const float* styleWeights = style.foundation;
        int skeletonCount = static_cast<int>(skeleton.size());
        // v0.18.6: 移除 * 2.5f 乘數，Foundation 應保持稀疏特性
        int targetTotal = std::max(skeletonCount, static_cast<int>(std::round(length * density)));
        int additionalHits = targetTotal - skeletonCount;

        for (int n = 0; n < additionalHits; n++) {
            float bestWeight = 0.0f;
            int bestPos = -1;

            for (int i = 0; i < length; i++) {
                if (p.hasOnsetAt(i)) continue;

                // v0.18.3: 使用浮點計算後四捨五入，避免精度損失
                int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
                float w = styleWeights[mapped];

                // Reduce weight where Timeline has onset
                if (timeline.hasOnsetAt(i)) {
                    w *= (1.0f - avoidanceStrength);
                }

                // Reduce weight for positions right after existing hits
                int prev = (i - 1 + length) % length;
                if (p.hasOnsetAt(prev)) w *= 0.3f;

                w *= (0.7f + dist(rng) * 0.6f);

                if (w > bestWeight) {
                    bestWeight = w;
                    bestPos = i;
                }
            }

            if (bestPos >= 0 && bestWeight > 0.1f) {
                float vel = 0.55f + bestWeight * 0.25f + velVar(rng);
                p.setOnset(bestPos, std::clamp(vel, 0.45f, 0.8f));
            }
        }

        return p;
    }

    // Groove that complements Foundation
    Pattern generateGrooveWithComplement(const StyleProfile& style, int length,
                                         float density, float variation,
                                         const Pattern& foundation,
                                         const Pattern& timeline,
                                         const InterlockConfig& config) {
        Pattern p(length);

        // If density is 0, return empty pattern (complete silence)
        if (density < 0.01f) {
            return p;
        }

        std::vector<float> weights(length);

        // v0.18.3: 使用浮點計算後四捨五入，避免精度損失
        const float* styleWeights = style.groove;
        for (int i = 0; i < length; i++) {
            int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
            weights[i] = styleWeights[mapped];
            weights[i] = weights[i] * (1.0f - variation) + variation;

            // Avoid Foundation positions
            if (foundation.hasOnsetAt(i)) {
                weights[i] *= config.avoidanceStrength;
            }

            // Boost positions between Foundation hits (complementary)
            int prev = (i - 1 + length) % length;
            int next = (i + 1) % length;
            if (foundation.hasOnsetAt(prev) || foundation.hasOnsetAt(next)) {
                weights[i] *= config.complementBoost;
            }

            // Slight avoidance of Timeline (but less than Foundation)
            if (timeline.hasOnsetAt(i)) {
                weights[i] *= 0.7f;
            }
        }

        // Allow 0 for complete silence
        int targetOnsets = static_cast<int>(std::round(length * density));
        weightedSelect(p, weights, targetOnsets);

        return p;
    }

    // Get interlock config for a specific style
    static InterlockConfig getStyleInterlockConfig(int styleIndex) {
        InterlockConfig config;

        switch (styleIndex) {
            case 0: // West African - strong interlock
                config.avoidFoundationOnTimeline = true;
                config.grooveComplementsFoundation = true;
                config.avoidanceStrength = 0.8f;
                config.complementBoost = 1.6f;
                break;

            case 1: // Afro-Cuban - clave-based
                config.avoidFoundationOnTimeline = true;
                config.grooveComplementsFoundation = true;
                config.avoidanceStrength = 0.7f;
                config.complementBoost = 1.5f;
                break;

            case 2: // Brazilian - layered
                config.avoidFoundationOnTimeline = false;
                config.grooveComplementsFoundation = true;
                config.avoidanceStrength = 0.3f;
                config.complementBoost = 1.4f;
                break;

            case 3: // Balkan - less strict
                config.avoidFoundationOnTimeline = false;
                config.grooveComplementsFoundation = false;
                config.avoidanceStrength = 0.2f;
                break;

            case 4: // Indian - tabla independent
                config.avoidFoundationOnTimeline = false;
                config.grooveComplementsFoundation = false;
                config.leadAvoidsGroove = true;
                break;

            case 5: // Gamelan - strict interlock
                config.avoidFoundationOnTimeline = true;
                config.grooveComplementsFoundation = true;
                config.avoidanceStrength = 0.9f;
                config.complementBoost = 1.8f;
                break;

            case 6: // Jazz - conversation
                config.avoidFoundationOnTimeline = true;
                config.grooveComplementsFoundation = true;
                config.leadAvoidsGroove = true;
                config.avoidanceStrength = 0.5f;
                break;

            case 7: // Electronic - grid
                config.avoidFoundationOnTimeline = false;
                config.grooveComplementsFoundation = false;
                config.avoidanceStrength = 0.0f;
                break;

            case 8: // Breakbeat - layered
                config.avoidFoundationOnTimeline = true;
                config.grooveComplementsFoundation = true;
                config.avoidanceStrength = 0.4f;
                break;

            case 9: // Techno - grid
                config.avoidFoundationOnTimeline = false;
                config.grooveComplementsFoundation = false;
                config.avoidanceStrength = 0.0f;
                break;

            default:
                break;
        }

        return config;
    }

    // ========================================
    // Kotekan Pair Generation (v0.19)
    // 生成 Gamelan Kotekan 互鎖 pattern pair
    // Polos: 強拍傾向（偶數位置）
    // Sangsih: 弱拍傾向（奇數位置）
    // 兩個 pattern 完美互補，無重疊
    // ========================================
    void generateKotekanPair(Pattern& polos, Pattern& sangsih, int length, float density, const StyleProfile& style) {
        polos.clear();
        sangsih.clear();

        // Density 0 = empty patterns (complete silence)
        if (density < 0.01f) {
            return;
        }

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        // 基礎權重（使用 style 的 groove weights）
        const float* weights = style.groove;

        // Kotekan 核心：交替式分配
        // Polos 傾向偶數位置（強拍），Sangsih 傾向奇數位置（弱拍）
        for (int i = 0; i < length; i++) {
            int mapped = static_cast<int>(std::round((i * 16.0f) / length)) % 16;
            float baseWeight = weights[mapped];

            bool isEvenPos = (i % 2 == 0);
            bool isStrongBeat = (i % 4 == 0);

            // 決定該位置由 Polos 還是 Sangsih 佔據
            // 強拍：Polos 優先；弱拍：Sangsih 優先
            float polosProb = baseWeight;
            if (isEvenPos) {
                polosProb *= 1.5f;  // Polos 偏好偶數位置
            } else {
                polosProb *= 0.3f;  // Polos 避免奇數位置
            }

            float sangsihProb = baseWeight;
            if (!isEvenPos) {
                sangsihProb *= 1.5f;  // Sangsih 偏好奇數位置
            } else {
                sangsihProb *= 0.3f;  // Sangsih 避免偶數位置
            }

            // 確保互補：同一位置不能同時有音
            if (dist(rng) < polosProb * density) {
                // Polos 佔據此位置
                float vel = 0.6f + baseWeight * 0.3f + velVar(rng);
                if (isStrongBeat) vel += 0.1f;  // 強拍加強
                polos.setOnset(i, std::clamp(vel, 0.4f, 1.0f));
            } else if (dist(rng) < sangsihProb * density) {
                // Sangsih 佔據此位置（僅在 Polos 未佔據時）
                if (!polos.hasOnsetAt(i)) {
                    float vel = 0.55f + baseWeight * 0.25f + velVar(rng);
                    sangsih.setOnset(i, std::clamp(vel, 0.35f, 0.9f));
                }
            }
        }

        // 後處理：確保完美互補（移除任何重疊）
        for (int i = 0; i < length; i++) {
            if (polos.hasOnsetAt(i) && sangsih.hasOnsetAt(i)) {
                // 重疊：偶數位置保留 Polos，奇數位置保留 Sangsih
                if (i % 2 == 0) {
                    sangsih.clearOnset(i);
                } else {
                    polos.clearOnset(i);
                }
            }
        }

        // 確保至少有一些音符
        int polosCount = 0, sangsihCount = 0;
        for (int i = 0; i < length; i++) {
            if (polos.hasOnsetAt(i)) polosCount++;
            if (sangsih.hasOnsetAt(i)) sangsihCount++;
        }

        // 如果某個 pattern 太空，補充最少音符
        if (polosCount == 0) {
            polos.setOnset(0, 0.8f);  // 至少在起始強拍有音
        }
        if (sangsihCount == 0) {
            sangsih.setOnset(1, 0.7f);  // 至少在第二位置有音
        }
    }

    // ========================================
    // Add Ghost Notes (for Groove/snare patterns)
    // Ghost notes are very soft hits on weak beats
    // ========================================
    void addGhostNotes(Pattern& p, const StyleProfile& style, float amount) {
        if (amount <= 0.0f) return;

        // Don't add ghost notes to empty patterns (respect density=0)
        bool hasAnyOnset = false;
        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                hasAnyOnset = true;
                break;
            }
        }
        if (!hasAnyOnset) return;

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.03f, 0.03f);

        const float* weights = style.groove;

        for (int i = 0; i < p.length; i++) {
            // Skip if already has onset
            if (p.hasOnsetAt(i)) continue;

            // Skip strong beats (ghosts go on weak positions)
            if (isStrongBeat(i, p.length)) continue;

            // Check if adjacent to existing hit (ghosts fill gaps)
            int prev = (i - 1 + p.length) % p.length;
            int next = (i + 1) % p.length;
            bool nearHit = p.hasOnsetAt(prev) || p.hasOnsetAt(next);

            // Ghost probability based on weight and proximity
            // v0.18.8: 統一使用四捨五入計算，與其他位置保持一致
            int mapped = static_cast<int>(std::round((i * 16.0f) / p.length)) % 16;
            float prob = weights[mapped] * amount * 0.4f;
            if (nearHit) prob *= 1.5f;  // More likely near other hits

            if (dist(rng) < prob) {
                // Ghost notes: 25-32% of normal velocity
                // Reference: Matsuo & Sakaguchi (2024) 1:4 amplitude ratio = 25%
                //           Cheng et al. (2022) 10dB difference = ~32%
                float ghostVel = 0.25f + dist(rng) * 0.07f + velVar(rng);
                p.setOnset(i, std::clamp(ghostVel, 0.20f, 0.35f));
            }
        }
    }

    // ========================================
    // Apply Rest
    // ========================================
    void applyRest(Pattern& p, Role role, float restAmount) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        float roleMax;
        switch (role) {
            case TIMELINE:   roleMax = 0.2f; break;
            case FOUNDATION: roleMax = 0.4f; break;
            case GROOVE:     roleMax = 1.0f; break;
            case LEAD:       roleMax = 1.0f; break;
            default:         roleMax = 1.0f; break;
        }

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            float prob = restAmount;

            // Strong beats resist rest
            if (isStrongBeat(i, p.length)) {
                prob *= 0.3f;
            }

            // Accented positions resist rest
            if (p.accents[i]) {
                prob *= 0.5f;
            }

            prob = std::min(prob, roleMax);

            if (dist(rng) < prob) {
                p.clearOnset(i);
            }
        }
    }

    // ========================================
    // Generate Accents (modifies velocity based on accent)
    // ========================================
    void generateAccents(Pattern& p, Role role, const StyleProfile& style) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        const float* weights = getWeightsForRole(role, style);

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) continue;

            // v0.18.8: 統一使用四捨五入計算，與其他位置保持一致
            int mapped = static_cast<int>(std::round((i * 16.0f) / p.length)) % 16;
            bool accent = false;

            switch (role) {
                case TIMELINE:
                    accent = (i == 0) || (weights[mapped] > 0.8f);
                    break;
                case FOUNDATION:
                    accent = true;  // All foundation hits are accented
                    break;
                case GROOVE:
                    accent = dist(rng) < weights[mapped];
                    break;
                case LEAD:
                    accent = dist(rng) < 0.3f;
                    break;
                default:
                    accent = false;
                    break;
            }

            p.accents[i] = accent;

            // Modify velocity based on accent
            float vel = p.getVelocity(i);
            if (accent) {
                vel = std::min(1.0f, vel + 0.2f);
            } else {
                vel = std::max(0.15f, vel - 0.05f);
            }
            p.setOnset(i, vel);
        }

        // Apply velocity smoothing for natural flow
        smoothVelocities(p);
    }

    // ========================================
    // Smooth velocities for natural phrase feeling
    // ========================================
    void smoothVelocities(Pattern& p) {
        if (p.length < 3) return;

        std::vector<float> smoothed(p.length);

        for (int i = 0; i < p.length; i++) {
            if (!p.hasOnsetAt(i)) {
                smoothed[i] = 0.0f;
                continue;
            }

            float vel = p.getVelocity(i);
            float neighborSum = 0.0f;
            int neighborCount = 0;

            // Look at nearby onsets
            for (int offset = -2; offset <= 2; offset++) {
                if (offset == 0) continue;
                int idx = (i + offset + p.length) % p.length;
                if (p.hasOnsetAt(idx)) {
                    float weight = (offset == -1 || offset == 1) ? 0.3f : 0.15f;
                    neighborSum += p.getVelocity(idx) * weight;
                    neighborCount++;
                }
            }

            if (neighborCount > 0) {
                // v0.18.5: 確保使用浮點除法
                float neighborAvg = neighborSum / static_cast<float>(neighborCount);
                smoothed[i] = vel * 0.7f + neighborAvg * 0.3f;
            } else {
                smoothed[i] = vel;
            }
        }

        for (int i = 0; i < p.length; i++) {
            if (smoothed[i] > 0.0f) {
                p.setOnset(i, std::clamp(smoothed[i], 0.12f, 1.0f));
            }
        }
    }

private:
    const float* getWeightsForRole(Role role, const StyleProfile& style) {
        switch (role) {
            case TIMELINE:   return style.timeline;
            case FOUNDATION: return style.foundation;
            case GROOVE:     return style.groove;
            case LEAD:       return style.lead;
        }
        return style.timeline;
    }

    void weightedSelect(Pattern& p, std::vector<float>& weights, int targetOnsets) {
        std::uniform_real_distribution<float> velVar(-0.12f, 0.12f);

        for (int n = 0; n < targetOnsets; n++) {
            // Recalculate sum of available positions
            float available = 0.0f;
            for (int i = 0; i < p.length; i++) {
                if (!p.hasOnsetAt(i)) available += weights[i];
            }
            // Stop if no weighted positions available (preserve style character)
            if (available <= 0.0f) break;

            // Random selection
            std::uniform_real_distribution<float> dist(0.0f, available);
            float r = dist(rng);

            float cumulative = 0.0f;
            for (int i = 0; i < p.length; i++) {
                if (p.hasOnsetAt(i)) continue;
                cumulative += weights[i];
                if (r <= cumulative) {
                    // Expanded velocity range: 0.25-0.95 base
                    float baseVel = 0.25f + weights[i] * 0.5f;

                    // Strong beat bonus
                    if (isStrongBeat(i, p.length)) {
                        baseVel += 0.2f;
                    }

                    // Add random variation
                    float velocity = std::clamp(baseVel + velVar(rng), 0.2f, 1.0f);
                    p.setOnset(i, velocity);
                    break;
                }
            }
        }
    }

    bool isStrongBeat(int pos, int length) {
        // Quarter note positions
        int quarterInterval = length / 4;
        if (quarterInterval <= 0) quarterInterval = 1;
        return (pos % quarterInterval) == 0;
    }
};

} // namespace WorldRhythm
