#pragma once

#include <vector>
#include <string>
#include <cmath>
#include "PatternGenerator.hpp"

namespace WorldRhythm {

// ========================================
// Irregular Meter Support
// ========================================
// Handles odd time signatures: 7/8, 9/8, 11/8, etc.
// With proper accent groupings (e.g., 7/8 = 2+2+3 or 3+2+2)

enum class MeterType {
    REGULAR_4_4,     // Standard 4/4
    METER_7_8_A,     // 7/8: 2+2+3 (Balkan)
    METER_7_8_B,     // 7/8: 3+2+2 (Rupak-style)
    METER_9_8_A,     // 9/8: 2+2+2+3 (Aksak)
    METER_9_8_B,     // 9/8: 3+3+3 (Compound triple)
    METER_11_8_A,    // 11/8: 2+2+3+2+2
    METER_11_8_B,    // 11/8: 3+2+2+2+2
    METER_5_4,       // 5/4: 3+2 or 2+3
    METER_12_8       // 12/8: 3+3+3+3 (Compound)
};

struct MeterDefinition {
    MeterType type;
    std::string name;
    int totalEighths;     // Total eighth notes per bar
    std::vector<int> groupings;  // Beat groupings
    std::vector<float> weights;  // Weight per eighth note position
};

// ========================================
// Meter Definitions
// ========================================
inline MeterDefinition createMeter_4_4() {
    MeterDefinition m;
    m.type = MeterType::REGULAR_4_4;
    m.name = "4/4";
    m.totalEighths = 16;  // 16 sixteenth notes
    m.groupings = {4, 4, 4, 4};

    m.weights.resize(16);
    for (int i = 0; i < 16; i++) {
        if (i == 0) m.weights[i] = 1.0f;        // Beat 1
        else if (i == 8) m.weights[i] = 0.85f;  // Beat 3
        else if (i == 4 || i == 12) m.weights[i] = 0.7f;  // Beats 2, 4
        else if (i % 2 == 0) m.weights[i] = 0.5f;  // Eighth notes
        else m.weights[i] = 0.35f;  // Sixteenths
    }
    return m;
}

inline MeterDefinition createMeter_7_8_A() {
    // Balkan style: 2+2+3 (short-short-long)
    MeterDefinition m;
    m.type = MeterType::METER_7_8_A;
    m.name = "7/8 (2+2+3)";
    m.totalEighths = 7;
    m.groupings = {2, 2, 3};

    m.weights = {
        1.0f,   // Group 1 start (strongest)
        0.4f,
        0.8f,   // Group 2 start
        0.4f,
        0.85f,  // Group 3 start (long group, strong)
        0.5f,
        0.45f
    };
    return m;
}

inline MeterDefinition createMeter_7_8_B() {
    // Rupak style: 3+2+2
    MeterDefinition m;
    m.type = MeterType::METER_7_8_B;
    m.name = "7/8 (3+2+2)";
    m.totalEighths = 7;
    m.groupings = {3, 2, 2};

    m.weights = {
        1.0f,   // Group 1 start
        0.45f,
        0.5f,
        0.85f,  // Group 2 start
        0.4f,
        0.8f,   // Group 3 start
        0.4f
    };
    return m;
}

inline MeterDefinition createMeter_9_8_A() {
    // Aksak style: 2+2+2+3
    MeterDefinition m;
    m.type = MeterType::METER_9_8_A;
    m.name = "9/8 (2+2+2+3)";
    m.totalEighths = 9;
    m.groupings = {2, 2, 2, 3};

    m.weights = {
        1.0f,   // Group 1
        0.4f,
        0.75f,  // Group 2
        0.4f,
        0.8f,   // Group 3
        0.4f,
        0.85f,  // Group 4 (long)
        0.5f,
        0.45f
    };
    return m;
}

inline MeterDefinition createMeter_9_8_B() {
    // Compound triple: 3+3+3
    MeterDefinition m;
    m.type = MeterType::METER_9_8_B;
    m.name = "9/8 (3+3+3)";
    m.totalEighths = 9;
    m.groupings = {3, 3, 3};

    m.weights = {
        1.0f,   // Beat 1
        0.4f,
        0.45f,
        0.8f,   // Beat 2
        0.4f,
        0.45f,
        0.75f,  // Beat 3
        0.4f,
        0.45f
    };
    return m;
}

inline MeterDefinition createMeter_11_8_A() {
    // 11/8: 2+2+3+2+2
    MeterDefinition m;
    m.type = MeterType::METER_11_8_A;
    m.name = "11/8 (2+2+3+2+2)";
    m.totalEighths = 11;
    m.groupings = {2, 2, 3, 2, 2};

    m.weights = {
        1.0f, 0.4f,      // Group 1
        0.75f, 0.4f,     // Group 2
        0.85f, 0.5f, 0.45f,  // Group 3 (long)
        0.8f, 0.4f,      // Group 4
        0.7f, 0.4f       // Group 5
    };
    return m;
}

inline MeterDefinition createMeter_11_8_B() {
    // 11/8: 3+2+2+2+2
    MeterDefinition m;
    m.type = MeterType::METER_11_8_B;
    m.name = "11/8 (3+2+2+2+2)";
    m.totalEighths = 11;
    m.groupings = {3, 2, 2, 2, 2};

    m.weights = {
        1.0f, 0.45f, 0.5f,   // Group 1 (long)
        0.8f, 0.4f,          // Group 2
        0.75f, 0.4f,         // Group 3
        0.8f, 0.4f,          // Group 4
        0.7f, 0.4f           // Group 5
    };
    return m;
}

inline MeterDefinition createMeter_5_4() {
    // 5/4: 3+2
    MeterDefinition m;
    m.type = MeterType::METER_5_4;
    m.name = "5/4 (3+2)";
    m.totalEighths = 10;  // 10 eighth notes
    m.groupings = {3, 2};

    m.weights = {
        1.0f, 0.4f,      // Beat 1
        0.6f, 0.35f,     // Beat 2
        0.7f, 0.4f,      // Beat 3
        0.85f, 0.45f,    // Beat 4 (start of 2-group)
        0.6f, 0.35f      // Beat 5
    };
    return m;
}

inline MeterDefinition createMeter_12_8() {
    // 12/8 compound: 3+3+3+3
    MeterDefinition m;
    m.type = MeterType::METER_12_8;
    m.name = "12/8";
    m.totalEighths = 12;
    m.groupings = {3, 3, 3, 3};

    m.weights = {
        1.0f, 0.35f, 0.4f,    // Beat 1
        0.75f, 0.35f, 0.4f,   // Beat 2
        0.85f, 0.35f, 0.4f,   // Beat 3
        0.7f, 0.35f, 0.4f     // Beat 4
    };
    return m;
}

// ========================================
// Meter Engine
// ========================================
class MeterEngine {
private:
    MeterDefinition currentMeter;
    std::vector<MeterDefinition> availableMeters;

public:
    MeterEngine() {
        availableMeters.push_back(createMeter_4_4());
        availableMeters.push_back(createMeter_7_8_A());
        availableMeters.push_back(createMeter_7_8_B());
        availableMeters.push_back(createMeter_9_8_A());
        availableMeters.push_back(createMeter_9_8_B());
        availableMeters.push_back(createMeter_11_8_A());
        availableMeters.push_back(createMeter_11_8_B());
        availableMeters.push_back(createMeter_5_4());
        availableMeters.push_back(createMeter_12_8());

        currentMeter = availableMeters[0];
    }

    void setMeter(MeterType type) {
        for (const auto& m : availableMeters) {
            if (m.type == type) {
                currentMeter = m;
                return;
            }
        }
    }

    void setMeterByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(availableMeters.size())) {
            currentMeter = availableMeters[index];
        }
    }

    const MeterDefinition& getCurrentMeter() const { return currentMeter; }
    int getTotalEighths() const { return currentMeter.totalEighths; }
    int getNumMeters() const { return static_cast<int>(availableMeters.size()); }
    const std::vector<int>& getGroupings() const { return currentMeter.groupings; }

    // ========================================
    // Get weight for a specific step
    // ========================================
    float getWeightForStep(int step) const {
        int pos = step % currentMeter.totalEighths;
        if (pos >= 0 && pos < static_cast<int>(currentMeter.weights.size())) {
            return currentMeter.weights[pos];
        }
        return 0.5f;
    }

    // ========================================
    // Check if step is at group boundary
    // ========================================
    bool isGroupStart(int step) const {
        int pos = step % currentMeter.totalEighths;
        int cumulative = 0;
        for (int groupLen : currentMeter.groupings) {
            if (pos == cumulative) return true;
            cumulative += groupLen;
        }
        return false;
    }

    // ========================================
    // Get group index for a step
    // ========================================
    int getGroupIndex(int step) const {
        int pos = step % currentMeter.totalEighths;
        int cumulative = 0;
        int groupIdx = 0;
        for (int groupLen : currentMeter.groupings) {
            if (pos < cumulative + groupLen) return groupIdx;
            cumulative += groupLen;
            groupIdx++;
        }
        return 0;
    }

    // ========================================
    // Generate meter-aware weights for any pattern length
    // ========================================
    std::vector<float> generateMeterWeights(int patternLength) const {
        std::vector<float> weights(patternLength);

        for (int i = 0; i < patternLength; i++) {
            // Map pattern position to meter position
            int meterPos = (i * currentMeter.totalEighths) / patternLength;
            meterPos = meterPos % currentMeter.totalEighths;

            weights[i] = currentMeter.weights[meterPos];
        }

        return weights;
    }

    // ========================================
    // Apply meter constraints to pattern
    // ========================================
    void applyMeterConstraints(Pattern& p) const {
        std::vector<float> meterWeights = generateMeterWeights(p.length);

        for (int i = 0; i < p.length; i++) {
            if (p.hasOnsetAt(i)) {
                // Adjust velocity based on meter weight
                float vel = p.getVelocity(i);
                float meterWeight = meterWeights[i];

                // Boost strong beats, reduce weak beats
                if (meterWeight > 0.7f) {
                    vel = std::max(vel, meterWeight * 0.9f);
                    p.accents[i] = true;
                } else if (meterWeight < 0.45f) {
                    vel = std::min(vel, 0.6f);
                }

                p.setOnset(i, vel);
            }
        }

        // Ensure downbeat (position 0) has a hit
        if (!p.hasOnsetAt(0)) {
            p.setOnset(0, 0.85f);
            p.accents[0] = true;
        }
    }

    // ========================================
    // Get recommended pattern length for meter
    // ========================================
    int getRecommendedLength() const {
        return currentMeter.totalEighths;
    }

    // ========================================
    // Check if a pattern length is compatible
    // ========================================
    bool isCompatibleLength(int length) const {
        // Compatible if length is a multiple of meter or divisible evenly
        return (length % currentMeter.totalEighths == 0) ||
               (currentMeter.totalEighths % length == 0);
    }
};

} // namespace WorldRhythm
