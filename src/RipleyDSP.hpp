#pragma once
#include "plugin.hpp"

// ============================================================
// ChaosGenerator - Lorenz Attractor 混沌系統
// 從 EllenRipley.cpp 提取，供衍生模組共用
// ============================================================
struct ChaosGenerator {
    float x = 0.1f;
    float y = 0.1f;
    float z = 0.1f;

    void reset() {
        x = 0.1f;
        y = 0.1f;
        z = 0.1f;
    }

    float process(float rate) {
        float dt = rate * 0.001f;

        float dx = 7.5f * (y - x);
        float dy = x * (30.9f - z) - y;
        float dz = x * y - 1.02f * z;

        x += dx * dt;
        y += dy * dt;
        z += dz * dt;

        // Prevent numerical explosion
        if (std::isnan(x) || std::isnan(y) || std::isnan(z) ||
            std::abs(x) > 100.0f || std::abs(y) > 100.0f || std::abs(z) > 100.0f) {
            reset();
        }

        return clamp(x * 0.1f, -1.0f, 1.0f);
    }
};

// ============================================================
// ReverbProcessor - Freeverb 風格 (8 comb + 4 allpass)
// 從 EllenRipley.cpp 提取
// ============================================================
struct ReverbProcessor {
    // Freeverb-style parallel comb filters + series allpass
    static constexpr int COMB_1_SIZE = 1557;
    static constexpr int COMB_2_SIZE = 1617;
    static constexpr int COMB_3_SIZE = 1491;
    static constexpr int COMB_4_SIZE = 1422;
    static constexpr int COMB_5_SIZE = 1277;
    static constexpr int COMB_6_SIZE = 1356;
    static constexpr int COMB_7_SIZE = 1188;
    static constexpr int COMB_8_SIZE = 1116;

    float combBuffer1[COMB_1_SIZE];
    float combBuffer2[COMB_2_SIZE];
    float combBuffer3[COMB_3_SIZE];
    float combBuffer4[COMB_4_SIZE];
    float combBuffer5[COMB_5_SIZE];
    float combBuffer6[COMB_6_SIZE];
    float combBuffer7[COMB_7_SIZE];
    float combBuffer8[COMB_8_SIZE];

    int combIndex1 = 0, combIndex2 = 0, combIndex3 = 0, combIndex4 = 0;
    int combIndex5 = 0, combIndex6 = 0, combIndex7 = 0, combIndex8 = 0;

    float combLp1 = 0.0f, combLp2 = 0.0f, combLp3 = 0.0f, combLp4 = 0.0f;
    float combLp5 = 0.0f, combLp6 = 0.0f, combLp7 = 0.0f, combLp8 = 0.0f;

    float hpState = 0.0f;

    static constexpr int ALLPASS_1_SIZE = 556;
    static constexpr int ALLPASS_2_SIZE = 441;
    static constexpr int ALLPASS_3_SIZE = 341;
    static constexpr int ALLPASS_4_SIZE = 225;

    float allpassBuffer1[ALLPASS_1_SIZE];
    float allpassBuffer2[ALLPASS_2_SIZE];
    float allpassBuffer3[ALLPASS_3_SIZE];
    float allpassBuffer4[ALLPASS_4_SIZE];

    int allpassIndex1 = 0, allpassIndex2 = 0, allpassIndex3 = 0, allpassIndex4 = 0;

    ReverbProcessor() { reset(); }

    void reset() {
        for (int i = 0; i < COMB_1_SIZE; i++) combBuffer1[i] = 0.0f;
        for (int i = 0; i < COMB_2_SIZE; i++) combBuffer2[i] = 0.0f;
        for (int i = 0; i < COMB_3_SIZE; i++) combBuffer3[i] = 0.0f;
        for (int i = 0; i < COMB_4_SIZE; i++) combBuffer4[i] = 0.0f;
        for (int i = 0; i < COMB_5_SIZE; i++) combBuffer5[i] = 0.0f;
        for (int i = 0; i < COMB_6_SIZE; i++) combBuffer6[i] = 0.0f;
        for (int i = 0; i < COMB_7_SIZE; i++) combBuffer7[i] = 0.0f;
        for (int i = 0; i < COMB_8_SIZE; i++) combBuffer8[i] = 0.0f;

        for (int i = 0; i < ALLPASS_1_SIZE; i++) allpassBuffer1[i] = 0.0f;
        for (int i = 0; i < ALLPASS_2_SIZE; i++) allpassBuffer2[i] = 0.0f;
        for (int i = 0; i < ALLPASS_3_SIZE; i++) allpassBuffer3[i] = 0.0f;
        for (int i = 0; i < ALLPASS_4_SIZE; i++) allpassBuffer4[i] = 0.0f;

        combIndex1 = combIndex2 = combIndex3 = combIndex4 = 0;
        combIndex5 = combIndex6 = combIndex7 = combIndex8 = 0;
        allpassIndex1 = allpassIndex2 = allpassIndex3 = allpassIndex4 = 0;

        combLp1 = combLp2 = combLp3 = combLp4 = 0.0f;
        combLp5 = combLp6 = combLp7 = combLp8 = 0.0f;
        hpState = 0.0f;
    }

    float processComb(float input, float* buffer, int size, int& index, float feedback, float& lp, float damping) {
        float output = buffer[index];
        lp = lp + (output - lp) * damping;
        buffer[index] = input + lp * feedback;
        index = (index + 1) % size;
        return output;
    }

    float processAllpass(float input, float* buffer, int size, int& index, float gain) {
        float delayed = buffer[index];
        float output = -input * gain + delayed;
        buffer[index] = input + delayed * gain;
        index = (index + 1) % size;
        return output;
    }

    float process(float inputL, float inputR, float roomSize, float damping, float decay,
                  bool isLeftChannel, bool chaosEnabled, float chaosOutput, float sampleRate) {

        float input = isLeftChannel ? inputL : inputR;

        float feedback = 0.5f + decay * 0.485f;
        if (chaosEnabled) {
            feedback += chaosOutput * 0.5f;
            feedback = clamp(feedback, 0.0f, 0.995f);
        }

        float dampingCoeff = 0.05f + damping * 0.9f;
        float roomScale = 0.3f + roomSize * 1.4f;

        float combOut = 0.0f;

        if (isLeftChannel) {
            int roomOffset1 = std::max(0, (int)(roomSize * 400 + chaosOutput * 50));
            int roomOffset2 = std::max(0, (int)(roomSize * 350 + chaosOutput * 40));

            int readIdx1 = ((combIndex1 - roomOffset1) % COMB_1_SIZE + COMB_1_SIZE) % COMB_1_SIZE;
            int readIdx2 = ((combIndex2 - roomOffset2) % COMB_2_SIZE + COMB_2_SIZE) % COMB_2_SIZE;

            float roomInput = input * roomScale;
            combOut += processComb(roomInput, combBuffer1, COMB_1_SIZE, combIndex1, feedback, combLp1, dampingCoeff);
            combOut += processComb(roomInput, combBuffer2, COMB_2_SIZE, combIndex2, feedback, combLp2, dampingCoeff);
            combOut += processComb(roomInput, combBuffer3, COMB_3_SIZE, combIndex3, feedback, combLp3, dampingCoeff);
            combOut += processComb(roomInput, combBuffer4, COMB_4_SIZE, combIndex4, feedback, combLp4, dampingCoeff);

            combOut += combBuffer1[readIdx1] * roomSize * 0.15f;
            combOut += combBuffer2[readIdx2] * roomSize * 0.12f;
        } else {
            int roomOffset5 = std::max(0, (int)(roomSize * 380 + chaosOutput * 45));
            int roomOffset6 = std::max(0, (int)(roomSize * 420 + chaosOutput * 55));

            int readIdx5 = ((combIndex5 - roomOffset5) % COMB_5_SIZE + COMB_5_SIZE) % COMB_5_SIZE;
            int readIdx6 = ((combIndex6 - roomOffset6) % COMB_6_SIZE + COMB_6_SIZE) % COMB_6_SIZE;

            float roomInput = input * roomScale;
            combOut += processComb(roomInput, combBuffer5, COMB_5_SIZE, combIndex5, feedback, combLp5, dampingCoeff);
            combOut += processComb(roomInput, combBuffer6, COMB_6_SIZE, combIndex6, feedback, combLp6, dampingCoeff);
            combOut += processComb(roomInput, combBuffer7, COMB_7_SIZE, combIndex7, feedback, combLp7, dampingCoeff);
            combOut += processComb(roomInput, combBuffer8, COMB_8_SIZE, combIndex8, feedback, combLp8, dampingCoeff);

            combOut += combBuffer5[readIdx5] * roomSize * 0.13f;
            combOut += combBuffer6[readIdx6] * roomSize * 0.11f;
        }

        combOut *= 0.25f;

        float diffused = combOut;
        diffused = processAllpass(diffused, allpassBuffer1, ALLPASS_1_SIZE, allpassIndex1, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer2, ALLPASS_2_SIZE, allpassIndex2, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer3, ALLPASS_3_SIZE, allpassIndex3, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer4, ALLPASS_4_SIZE, allpassIndex4, 0.5f);

        float hpCutoff = 100.0f / (sampleRate * 0.5f);
        hpCutoff = clamp(hpCutoff, 0.001f, 0.1f);
        hpState += (diffused - hpState) * hpCutoff;
        float hpOutput = diffused - hpState;

        return hpOutput;
    }
};

// ============================================================
// GrainProcessor - 16 Grains 粒子處理器
// 從 EllenRipley.cpp 提取
// ============================================================
struct GrainProcessor {
    static constexpr int GRAIN_BUFFER_SIZE = 8192;
    float grainBuffer[GRAIN_BUFFER_SIZE];
    int grainWriteIndex = 0;

    struct Grain {
        bool active = false;
        float position = 0.0f;
        float size = 0.0f;
        float envelope = 0.0f;
        float direction = 1.0f;
        float pitch = 1.0f;
    };

    static constexpr int MAX_GRAINS = 16;
    Grain grains[MAX_GRAINS];

    float phase = 0.0f;

    GrainProcessor() { reset(); }

    void reset() {
        for (int i = 0; i < GRAIN_BUFFER_SIZE; i++) {
            grainBuffer[i] = 0.0f;
        }
        grainWriteIndex = 0;

        for (int i = 0; i < MAX_GRAINS; i++) {
            grains[i].active = false;
        }
        phase = 0.0f;
    }

    float process(float input, float grainSize, float density, float position,
                  bool chaosEnabled, float chaosOutput, float sampleRate) {

        grainBuffer[grainWriteIndex] = input;
        grainWriteIndex = (grainWriteIndex + 1) % GRAIN_BUFFER_SIZE;

        float grainSizeMs = grainSize * 99.0f + 1.0f;
        float grainSamples = (grainSizeMs / 1000.0f) * sampleRate;

        float densityValue = density;
        if (chaosEnabled) {
            densityValue += chaosOutput * 0.3f;
        }
        densityValue = clamp(densityValue, 0.0f, 1.0f);

        float triggerRate = densityValue * 50.0f + 1.0f;
        phase += triggerRate / sampleRate;

        if (phase >= 1.0f) {
            phase -= 1.0f;

            for (int i = 0; i < MAX_GRAINS; i++) {
                if (!grains[i].active) {
                    grains[i].active = true;
                    grains[i].size = grainSamples;
                    grains[i].envelope = 0.0f;

                    float pos = position;
                    if (chaosEnabled) {
                        pos += chaosOutput * 20.0f;
                        if (random::uniform() < 0.3f) {
                            grains[i].direction = -1.0f;
                        } else {
                            grains[i].direction = 1.0f;
                        }

                        if (densityValue > 0.7f && random::uniform() < 0.2f) {
                            grains[i].pitch = random::uniform() < 0.5f ? 0.5f : 2.0f;
                        } else {
                            grains[i].pitch = 1.0f;
                        }
                    } else {
                        grains[i].direction = 1.0f;
                        grains[i].pitch = 1.0f;
                    }

                    pos = clamp(pos, 0.0f, 1.0f);
                    grains[i].position = pos * GRAIN_BUFFER_SIZE;
                    break;
                }
            }
        }

        float output = 0.0f;
        int activeGrains = 0;

        for (int i = 0; i < MAX_GRAINS; i++) {
            if (grains[i].active) {
                float envPhase = grains[i].envelope / grains[i].size;

                if (envPhase >= 1.0f) {
                    grains[i].active = false;
                    continue;
                }

                float env = 0.5f * (1.0f - cos(envPhase * 2.0f * M_PI));

                int readPos = (int)grains[i].position;
                readPos = ((readPos % GRAIN_BUFFER_SIZE) + GRAIN_BUFFER_SIZE) % GRAIN_BUFFER_SIZE;

                float sample = grainBuffer[readPos];
                output += sample * env;

                grains[i].position += grains[i].direction * grains[i].pitch;

                while (grains[i].position >= GRAIN_BUFFER_SIZE) {
                    grains[i].position -= GRAIN_BUFFER_SIZE;
                }
                while (grains[i].position < 0) {
                    grains[i].position += GRAIN_BUFFER_SIZE;
                }

                grains[i].envelope += 1.0f;
                activeGrains++;
            }
        }

        if (activeGrains > 0) {
            output /= sqrt(activeGrains);
        }

        return output;
    }
};
