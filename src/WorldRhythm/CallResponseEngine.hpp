#pragma once

#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <algorithm>
#include "PatternGenerator.hpp"
#include "StyleProfiles.hpp"

namespace WorldRhythm {

// ========================================
// Call-and-Response Engine
// ========================================
// Implements traditional call-and-response patterns found in:
// - West African drumming (lead djembe calls, ensemble responds)
// - Afro-Cuban music (coro-pregon structure)
// - Brazilian batucada (repinique calls, surdo/caixa respond)
// - Jazz (trading fours, comping responses)

enum class CallType {
    PHRASE,      // Full melodic phrase (2-4 beats)
    ACCENT,      // Single strong accent
    BREAK,       // Rhythmic break/pause
    SIGNAL       // Cue for section change
};

enum class ResponseType {
    ECHO,        // Repeat call pattern
    ANSWER,      // Complementary pattern
    UNISON,      // All voices together
    LAYERED      // Staggered entry
};

struct CallEvent {
    CallType type;
    int startStep;       // Position in pattern
    int lengthSteps;     // Duration of call
    float intensity;     // 0.0 - 1.0
    std::vector<float> velocities;  // Call pattern data
};

struct ResponseEvent {
    ResponseType type;
    int startStep;       // Response start (after call ends)
    int lengthSteps;     // Response duration
    float intensityScale; // Relative to call (typically 0.7-0.9)
    std::vector<float> velocities;  // Response pattern data
    bool crossBar = false;  // true if response wraps to next bar (v0.18)
    int overflowSteps = 0;  // Steps that overflow to next bar (v0.18)
};

struct CallResponsePair {
    CallEvent call;
    ResponseEvent response;
    int callerRole;      // Usually LEAD
    int responderRole;   // Usually FOUNDATION or GROOVE
};

// ========================================
// Style-specific call-response profiles
// ========================================
struct CallResponseProfile {
    // Probability of call-response occurring per phrase
    float callProbability;

    // Typical call length (beats)
    int minCallBeats;
    int maxCallBeats;

    // Gap between call end and response start (steps)
    int responseDelay;

    // Response characteristics
    float responseIntensityScale;  // 0.7 = response is 70% of call intensity
    bool responseCanOverlap;       // Allow response while call continues

    // Preferred types
    CallType preferredCallType;
    ResponseType preferredResponseType;

    // Role assignments
    int primaryCaller;     // Role index
    int primaryResponder;  // Role index
    bool groupResponse;    // All non-caller roles respond
};

// Default profiles per style
const CallResponseProfile CR_PROFILES[] = {
    // West African - Lead calls, ensemble responds with unison accents
    {0.7f, 2, 4, 2, 0.85f, false, CallType::PHRASE, ResponseType::UNISON, LEAD, GROOVE, true},

    // Afro-Cuban - Pregon/coro pattern, echo response
    {0.6f, 2, 3, 4, 0.80f, false, CallType::PHRASE, ResponseType::ECHO, LEAD, GROOVE, false},

    // Brazilian - Repinique signals, layered surdo response
    {0.5f, 1, 2, 2, 0.90f, true, CallType::SIGNAL, ResponseType::LAYERED, LEAD, FOUNDATION, true},

    // Balkan - Accent-based calls
    {0.4f, 1, 2, 1, 0.85f, false, CallType::ACCENT, ResponseType::ANSWER, LEAD, GROOVE, false},

    // Indian - Tihai-like calls with precise responses
    {0.5f, 3, 4, 0, 0.75f, false, CallType::PHRASE, ResponseType::ECHO, LEAD, GROOVE, false},

    // Gamelan - Signal for angsel (coordinated break)
    {0.6f, 1, 2, 0, 1.0f, false, CallType::BREAK, ResponseType::UNISON, TIMELINE, GROOVE, true},

    // Jazz - Trading phrases, answer responses
    {0.5f, 4, 8, 0, 0.85f, false, CallType::PHRASE, ResponseType::ANSWER, LEAD, GROOVE, false},

    // Electronic - Build signals, layered response
    {0.3f, 2, 4, 4, 0.90f, true, CallType::SIGNAL, ResponseType::LAYERED, LEAD, GROOVE, false},

    // Breakbeat - Break calls, unison drops
    {0.4f, 2, 4, 0, 1.0f, false, CallType::BREAK, ResponseType::UNISON, LEAD, FOUNDATION, true},

    // Techno - Minimal call-response
    {0.2f, 1, 2, 4, 0.80f, true, CallType::ACCENT, ResponseType::LAYERED, LEAD, GROOVE, false}
};

class CallResponseEngine {
private:
    std::mt19937 rng;
    std::vector<CallResponsePair> activePairs;

    // v0.18.2: Call 位置歷史追蹤（用於動態預測 nextCallStart）
    static constexpr int CALL_HISTORY_SIZE = 16;
    std::array<int, CALL_HISTORY_SIZE> callStartHistory{};  // 歷史 call 起始位置
    int callHistoryCount = 0;
    int lastBarNumber = -1;

public:
    CallResponseEngine() : rng(std::random_device{}()) {
        callStartHistory.fill(-1);
    }

    void seed(unsigned int s) { rng.seed(s); }

    // ========================================
    // v0.18.2: 動態預測下一個 call 的起始位置
    // 基於風格特性和歷史模式
    // ========================================
    int predictNextCallStart(int styleIndex, int patternLength, int currentBarNumber) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        // 策略 1：基於風格 profile 的典型 call 行為
        // 某些風格偏好在特定拍點發起 call
        int stylePreferredStart = 0;

        switch (styleIndex) {
            case 0:  // West African - call 通常在 beat 1
                stylePreferredStart = 0;
                break;
            case 1:  // Afro-Cuban - call 可能在 beat 1 或 beat 3
                stylePreferredStart = (currentBarNumber % 2 == 0) ? 0 : patternLength / 2;
                break;
            case 2:  // Brazilian - repinique call 常在 beat 4 (準備下一小節)
                stylePreferredStart = (patternLength * 3) / 4;
                break;
            case 3:  // Balkan - 受不對稱拍號影響，偏好強拍
                stylePreferredStart = 0;
                break;
            case 4:  // Indian - call 在樂句開頭，準備 tihai
                stylePreferredStart = 0;
                break;
            case 5:  // Gamelan - angsel signal 在 beat 3-4
                stylePreferredStart = patternLength / 2;
                break;
            case 6:  // Jazz - trading 通常在 beat 1
                stylePreferredStart = 0;
                break;
            case 7:  // Electronic - build 可能從中段開始
                stylePreferredStart = patternLength / 4;
                break;
            case 8:  // Breakbeat - break call 在 beat 1
                stylePreferredStart = 0;
                break;
            case 9:  // Techno - minimal call 在 beat 1 或 3
                stylePreferredStart = (currentBarNumber % 4 == 3) ? patternLength / 2 : 0;
                break;
            default:
                stylePreferredStart = 0;
        }

        // 策略 2：基於歷史統計
        // 分析過去 call 的起始位置分布
        if (callHistoryCount >= 4) {
            // 計算最常見的起始位置（量化到 beat 級別）
            std::array<int, 4> beatCounts = {0, 0, 0, 0};  // 4 beats per bar
            // v0.18.3: 防止除零，確保 stepsPerBeat 至少為 1
            int stepsPerBeat = std::max(1, patternLength / 4);

            for (int i = 0; i < callHistoryCount && i < CALL_HISTORY_SIZE; i++) {
                if (callStartHistory[i] >= 0) {
                    int beat = (callStartHistory[i] / stepsPerBeat) % 4;
                    beatCounts[beat]++;
                }
            }

            // 找出最常見的 beat
            int maxBeat = 0;
            int maxCount = beatCounts[0];
            for (int b = 1; b < 4; b++) {
                if (beatCounts[b] > maxCount) {
                    maxCount = beatCounts[b];
                    maxBeat = b;
                }
            }

            // 如果歷史模式強烈（出現次數超過 50%），採用歷史模式
            if (maxCount > callHistoryCount / 2) {
                return maxBeat * stepsPerBeat;
            }
        }

        // 策略 3：根據樂句結構調整
        // 在 4 小節樂句中，不同位置有不同的 call 傾向
        int barInPhrase = currentBarNumber % 4;
        if (barInPhrase == 3) {
            // 樂句結尾前，call 可能較晚發起（為轉接做準備）
            stylePreferredStart = std::min(stylePreferredStart + patternLength / 4, patternLength - 4);
        } else if (barInPhrase == 0) {
            // 樂句開頭，call 通常在 beat 1
            stylePreferredStart = 0;
        }

        return stylePreferredStart;
    }

    // ========================================
    // 記錄 call 起始位置到歷史
    // ========================================
    void recordCallStart(int startStep, int barNumber) {
        if (barNumber != lastBarNumber) {
            // 新的小節，記錄歷史
            callStartHistory[callHistoryCount % CALL_HISTORY_SIZE] = startStep;
            callHistoryCount++;
            lastBarNumber = barNumber;
        }
    }

    // ========================================
    // 清除歷史（用於風格切換時）
    // ========================================
    void clearCallHistory() {
        callStartHistory.fill(-1);
        callHistoryCount = 0;
        lastBarNumber = -1;
    }

    // ========================================
    // Determine if call should occur at bar position
    // ========================================
    bool shouldCall(int barNumber, int styleIndex, float userProbability) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        // Base probability modulated by phrase position
        float baseProbability = profile.callProbability;

        // Calls more likely at phrase boundaries (bar 4, 8, etc.)
        int barInPhrase = barNumber % 4;
        if (barInPhrase == 3) {  // Bar before phrase end
            baseProbability *= 1.5f;
        } else if (barInPhrase == 0) {  // First bar of phrase
            baseProbability *= 0.5f;  // Less likely at start
        }

        float finalProb = std::min(1.0f, baseProbability * userProbability);

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < finalProb;
    }

    // ========================================
    // Generate call pattern
    // ========================================
    CallEvent generateCall(int styleIndex, int startStep, int patternLength, float intensity) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        CallEvent call;
        call.type = profile.preferredCallType;
        call.startStep = startStep;
        call.intensity = intensity;

        // Determine call length
        std::uniform_int_distribution<int> lenDist(profile.minCallBeats, profile.maxCallBeats);
        int callBeats = lenDist(rng);
        call.lengthSteps = callBeats * 4;  // Assuming 4 steps per beat

        // Ensure call fits in pattern
        if (call.startStep + call.lengthSteps > patternLength) {
            call.lengthSteps = patternLength - call.startStep;
        }

        // Generate call velocities based on type
        call.velocities = generateCallPattern(call.type, call.lengthSteps, intensity);

        return call;
    }

    // ========================================
    // Generate response pattern
    // v0.18.2: 新增 barNumber 參數用於動態 nextCallStart 計算
    // ========================================
    ResponseEvent generateResponse(const CallEvent& call, int styleIndex, int patternLength, int barNumber = 0) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        ResponseEvent response;
        response.type = profile.preferredResponseType;
        response.startStep = call.startStep + call.lengthSteps + profile.responseDelay;
        response.intensityScale = profile.responseIntensityScale;
        response.crossBar = false;
        response.overflowSteps = 0;

        // Response length matches or is shorter than call
        response.lengthSteps = call.lengthSteps;

        // v0.18.7: 重構跨小節邏輯，統一處理所有邊界情況
        if (response.startStep >= patternLength) {
            // Response 起始點超出當前小節
            response.crossBar = true;
            response.startStep = response.startStep % patternLength;

            // v0.18.2: 動態預測下一個 call 的起始位置
            int nextCallStart = predictNextCallStart(styleIndex, patternLength, barNumber + 1);

            // v0.18.7: 確保 nextCallStart 在有效範圍內
            nextCallStart = std::clamp(nextCallStart, 0, patternLength - 1);

            // 計算安全的最大 response 長度
            int safeMaxLength;
            if (nextCallStart > response.startStep) {
                // 下一個 call 在 response 之後，可用空間 = nextCallStart - startStep
                safeMaxLength = nextCallStart - response.startStep;
            } else if (nextCallStart == response.startStep) {
                // call 和 response 同時開始，完全放棄跨小節
                safeMaxLength = 0;
            } else {
                // nextCallStart < response.startStep（環繞情況）
                // response 可以延伸到小節末，但不能環繞回去與 call 重疊
                safeMaxLength = patternLength - response.startStep;
            }

            // 應用長度限制
            const int MIN_RESPONSE_LENGTH = 2;
            if (safeMaxLength < MIN_RESPONSE_LENGTH) {
                // 空間不足，放棄跨小節，改為當前小節末尾
                response.crossBar = false;
                response.startStep = std::max(0, patternLength - call.lengthSteps);
                response.lengthSteps = patternLength - response.startStep;
            } else {
                response.lengthSteps = std::min(response.lengthSteps, safeMaxLength);
            }
        } else if (response.startStep + response.lengthSteps > patternLength) {
            // Response 起始在當前小節內，但會延伸到下一小節
            response.crossBar = true;
            response.overflowSteps = (response.startStep + response.lengthSteps) - patternLength;

            // 選項 1：截短 response 使其不超出
            // 選項 2：允許 overflow，由 applyResponseToPattern 處理
            // 這裡選擇選項 2，保持完整性

            // 但如果 overflow 太多（超過 response 的 50%），改為截短
            if (response.overflowSteps > response.lengthSteps / 2) {
                response.lengthSteps = patternLength - response.startStep;
                response.overflowSteps = 0;
                response.crossBar = false;
            }
        }

        // Generate response velocities based on type
        response.velocities = generateResponsePattern(
            response.type, call.velocities, response.lengthSteps,
            call.intensity * response.intensityScale);

        return response;
    }

    // ========================================
    // Generate call-response pair
    // ========================================
    CallResponsePair generatePair(int styleIndex, int barNumber, int patternLength, float intensity) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        CallResponsePair pair;
        pair.callerRole = profile.primaryCaller;
        pair.responderRole = profile.primaryResponder;

        // Determine call start position
        // Calls typically start on strong beats (quarter note positions)
        std::vector<int> validStarts;
        for (int pos = 0; pos < patternLength; pos += 4) {
            validStarts.push_back(pos);
        }
        if (validStarts.empty()) validStarts.push_back(0);

        std::uniform_int_distribution<int> startDist(0, static_cast<int>(validStarts.size()) - 1);
        int startStep = validStarts[startDist(rng)];

        // Generate call
        pair.call = generateCall(styleIndex, startStep, patternLength, intensity);

        // v0.18.2: 記錄 call 起始位置到歷史，用於未來預測
        recordCallStart(pair.call.startStep, barNumber);

        // Generate response (傳入 barNumber 用於動態 nextCallStart 計算)
        pair.response = generateResponse(pair.call, styleIndex, patternLength, barNumber);

        return pair;
    }

    // ========================================
    // Apply call to pattern
    // ========================================
    void applyCallToPattern(Pattern& p, const CallEvent& call) {
        for (int i = 0; i < call.lengthSteps && (call.startStep + i) < p.length; i++) {
            int pos = call.startStep + i;
            float vel = call.velocities[i];

            if (vel > 0.0f) {
                // Call overrides existing pattern with strong presence
                p.setOnset(pos, vel);
                p.accents[pos] = (vel > 0.7f);
            }
        }
    }

    // ========================================
    // Apply response to pattern
    // v0.18.1: 處理 crossBar overflow
    // ========================================
    void applyResponseToPattern(Pattern& p, const ResponseEvent& response) {
        // 計算當前小節內可應用的步數
        int stepsInCurrentBar = response.lengthSteps;
        if (response.crossBar && response.overflowSteps > 0) {
            // 如果有 overflow，只處理當前小節內的部分
            stepsInCurrentBar = response.lengthSteps - response.overflowSteps;
        }

        // 應用當前小節內的 response
        for (int i = 0; i < stepsInCurrentBar && (response.startStep + i) < p.length; i++) {
            int pos = response.startStep + i;
            float vel = response.velocities[i];

            if (vel > 0.0f) {
                // Response blends with existing pattern
                float existing = p.getVelocity(pos);
                float blended = std::max(existing, vel);
                p.setOnset(pos, blended);
            }
        }
    }

    // ========================================
    // Apply response overflow to next bar's pattern
    // v0.18.1: 新增函數處理跨小節 response
    // ========================================
    void applyResponseOverflowToPattern(Pattern& p, const ResponseEvent& response) {
        if (!response.crossBar || response.overflowSteps <= 0) {
            return;  // 沒有 overflow，不需處理
        }

        // overflow 部分從 pattern 開頭開始
        int overflowStart = response.lengthSteps - response.overflowSteps;

        for (int i = 0; i < response.overflowSteps && i < p.length; i++) {
            int velocityIdx = overflowStart + i;
            if (velocityIdx >= static_cast<int>(response.velocities.size())) break;

            float vel = response.velocities[velocityIdx];

            if (vel > 0.0f) {
                float existing = p.getVelocity(i);
                float blended = std::max(existing, vel);
                p.setOnset(i, blended);
            }
        }
    }

    // ========================================
    // Check if response has overflow for next bar
    // ========================================
    bool hasResponseOverflow(const ResponseEvent& response) const {
        return response.crossBar && response.overflowSteps > 0;
    }

    // ========================================
    // Apply response to multiple voices (group response)
    // ========================================
    void applyGroupResponse(Pattern patterns[4][3], const ResponseEvent& response,
                           int styleIndex, const int voicesPerGroup[4]) {
        const CallResponseProfile& profile = CR_PROFILES[styleIndex];

        if (!profile.groupResponse) {
            // Single responder only
            for (int v = 0; v < voicesPerGroup[profile.primaryResponder]; v++) {
                applyResponseToPattern(patterns[profile.primaryResponder][v], response);
            }
            return;
        }

        // All roles except caller respond
        for (int role = 0; role < 4; role++) {
            if (role == profile.primaryCaller) continue;

            // Stagger entry for LAYERED response
            int delay = 0;
            if (response.type == ResponseType::LAYERED) {
                delay = role;  // Each role enters 1 step later
            }

            for (int v = 0; v < voicesPerGroup[role]; v++) {
                ResponseEvent adjusted = response;
                adjusted.startStep = (response.startStep + delay) % patterns[role][v].length;

                // Scale intensity by role
                float roleScale = 1.0f;
                switch (role) {
                    case TIMELINE:   roleScale = 0.6f; break;
                    case FOUNDATION: roleScale = 1.0f; break;
                    case GROOVE:     roleScale = 0.9f; break;
                    case LEAD:       roleScale = 0.7f; break;
                }

                for (float& vel : adjusted.velocities) {
                    vel *= roleScale;
                }

                applyResponseToPattern(patterns[role][v], adjusted);
            }
        }
    }

    // ========================================
    // Get profile for style
    // ========================================
    const CallResponseProfile& getProfile(int styleIndex) const {
        return CR_PROFILES[std::clamp(styleIndex, 0, 9)];
    }

    // ========================================
    // Check if style uses call-response prominently
    // ========================================
    bool styleUsesCallResponse(int styleIndex) const {
        return CR_PROFILES[styleIndex].callProbability >= 0.4f;
    }

private:
    // ========================================
    // Generate call pattern based on type
    // ========================================
    std::vector<float> generateCallPattern(CallType type, int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        switch (type) {
            case CallType::PHRASE:
                // Melodic phrase: start strong, develop, end accent
                pattern[0] = std::clamp(0.9f * intensity + velVar(rng), 0.7f, 1.0f);
                for (int i = 1; i < lengthSteps - 1; i++) {
                    // Syncopated internal rhythm
                    if (i % 2 == 1 || dist(rng) < 0.6f * intensity) {
                        pattern[i] = std::clamp(0.6f * intensity + velVar(rng), 0.4f, 0.85f);
                    }
                }
                // Strong ending
                pattern[lengthSteps - 1] = std::clamp(0.85f * intensity + velVar(rng), 0.75f, 1.0f);
                break;

            case CallType::ACCENT:
                // Single strong accent at start
                pattern[0] = std::clamp(0.95f * intensity + velVar(rng), 0.85f, 1.0f);
                // Possible secondary accent
                if (lengthSteps > 2 && dist(rng) < 0.5f) {
                    pattern[lengthSteps / 2] = std::clamp(0.7f * intensity + velVar(rng), 0.5f, 0.85f);
                }
                break;

            case CallType::BREAK:
                // Silence with surrounding accents
                pattern[0] = std::clamp(0.9f * intensity + velVar(rng), 0.8f, 1.0f);
                // Rest in middle (pattern stays 0)
                pattern[lengthSteps - 1] = std::clamp(0.95f * intensity + velVar(rng), 0.85f, 1.0f);
                break;

            case CallType::SIGNAL:
                // Distinctive signal pattern (short-short-long)
                if (lengthSteps >= 4) {
                    pattern[0] = std::clamp(0.8f * intensity + velVar(rng), 0.7f, 0.95f);
                    pattern[1] = std::clamp(0.75f * intensity + velVar(rng), 0.6f, 0.9f);
                    pattern[3] = std::clamp(0.9f * intensity + velVar(rng), 0.8f, 1.0f);
                } else {
                    pattern[0] = std::clamp(0.9f * intensity + velVar(rng), 0.8f, 1.0f);
                }
                break;
        }

        return pattern;
    }

    // ========================================
    // Generate response pattern based on type
    // ========================================
    std::vector<float> generateResponsePattern(ResponseType type,
                                                const std::vector<float>& callPattern,
                                                int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> velVar(-0.1f, 0.1f);

        switch (type) {
            case ResponseType::ECHO:
                // Repeat call pattern (possibly truncated)
                for (int i = 0; i < lengthSteps && i < static_cast<int>(callPattern.size()); i++) {
                    pattern[i] = callPattern[i] * intensity / 0.9f;  // Adjust for intensity scale
                    pattern[i] = std::clamp(pattern[i], 0.0f, 0.95f);
                }
                break;

            case ResponseType::ANSWER:
                // Complementary pattern (fill gaps)
                for (int i = 0; i < lengthSteps; i++) {
                    int callIdx = i < static_cast<int>(callPattern.size()) ? i : 0;
                    if (callPattern[callIdx] < 0.3f) {
                        // Answer where call is silent
                        pattern[i] = std::clamp(0.7f * intensity + velVar(rng), 0.5f, 0.9f);
                    }
                }
                // Ensure at least some response
                if (pattern[0] < 0.1f) {
                    pattern[0] = std::clamp(0.6f * intensity + velVar(rng), 0.4f, 0.8f);
                }
                break;

            case ResponseType::UNISON:
                // All voices hit together on key points
                pattern[0] = std::clamp(0.85f * intensity + velVar(rng), 0.75f, 1.0f);
                if (lengthSteps > 1) {
                    pattern[lengthSteps - 1] = std::clamp(0.8f * intensity + velVar(rng), 0.7f, 0.95f);
                }
                break;

            case ResponseType::LAYERED:
                // Staggered entry effect (each voice enters progressively)
                for (int i = 0; i < lengthSteps; i++) {
                    float progress = static_cast<float>(i) / lengthSteps;
                    // Build up density
                    if (dist(rng) < 0.3f + progress * 0.5f) {
                        pattern[i] = std::clamp((0.4f + progress * 0.4f) * intensity + velVar(rng),
                                               0.3f, 0.9f);
                    }
                }
                // Strong final hit
                pattern[lengthSteps - 1] = std::clamp(0.85f * intensity + velVar(rng), 0.75f, 1.0f);
                break;
        }

        return pattern;
    }

public:
    // ========================================
    // v0.17: 風格特定 Call Pattern 內容增強
    // ========================================

    /**
     * 生成風格特定的 call 樂句
     * 基於各文化的傳統呼喚模式
     */
    std::vector<float> generateStyleSpecificCall(int styleIndex, int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        switch (styleIndex) {
            case 0:  // West African - Djembe lead call
                // 傳統：強-弱-強-弱 + 結尾重音
                for (int i = 0; i < lengthSteps; i++) {
                    if (i == 0) {
                        pattern[i] = 0.95f * intensity + velVar(rng);  // 開頭強
                    } else if (i == lengthSteps - 1) {
                        pattern[i] = 1.0f * intensity + velVar(rng);   // 結尾最強
                    } else if (i % 2 == 0) {
                        pattern[i] = 0.75f * intensity + velVar(rng);  // 偶數位置中等
                    } else if (i % 4 == 1) {
                        pattern[i] = 0.55f * intensity + velVar(rng);  // 部分奇數位置輕
                    }
                }
                break;

            case 1:  // Afro-Cuban - Pregón call
                // 傳統 Pregón：syncopated，強調 off-beat
                pattern[0] = 0.85f * intensity + velVar(rng);
                if (lengthSteps >= 4) {
                    pattern[1] = 0.65f * intensity + velVar(rng);   // and of 1
                    pattern[3] = 0.70f * intensity + velVar(rng);   // and of 2
                }
                if (lengthSteps >= 8) {
                    pattern[5] = 0.60f * intensity + velVar(rng);
                    pattern[7] = 0.90f * intensity + velVar(rng);   // 結尾
                }
                break;

            case 4:  // Indian - Tabla call (Sam-oriented)
                // 傳統：Bol 序列，結尾落在 Sam
                {
                    // 簡化的 Bol 序列：Dha Dhin Dhin Dha
                    std::vector<float> bolPattern = {0.9f, 0.6f, 0.55f, 0.85f};
                    for (int i = 0; i < lengthSteps; i++) {
                        int bolIdx = i % 4;
                        if (i < static_cast<int>(bolPattern.size()) * 2) {
                            pattern[i] = bolPattern[bolIdx] * intensity + velVar(rng);
                        }
                    }
                    // Sam（結尾）必須最強
                    if (lengthSteps > 0) {
                        pattern[lengthSteps - 1] = 1.0f * intensity;
                    }
                }
                break;

            case 5:  // Gamelan - Angsel signal
                // 傳統：雙音信號引導 Angsel
                pattern[0] = 0.85f * intensity + velVar(rng);
                if (lengthSteps >= 4) {
                    pattern[2] = 0.90f * intensity + velVar(rng);
                }
                // 後面靜默（Angsel 特徵）
                break;

            case 6:  // Jazz - Trading phrase
                // 即興樂句：swing feel，syncopated
                for (int i = 0; i < lengthSteps; i++) {
                    bool isUpbeat = (i % 4 == 1 || i % 4 == 3);
                    if (i == 0) {
                        pattern[i] = 0.80f * intensity + velVar(rng);
                    } else if (isUpbeat) {
                        pattern[i] = 0.70f * intensity + velVar(rng);
                    } else if (i == lengthSteps - 1) {
                        pattern[i] = 0.85f * intensity + velVar(rng);
                    }
                }
                break;

            default:
                // 默認：使用通用 call pattern
                return generateCallPattern(CallType::PHRASE, lengthSteps, intensity);
        }

        // Clamp all values
        for (float& v : pattern) {
            v = std::clamp(v, 0.0f, 1.0f);
        }

        return pattern;
    }

    /**
     * 生成風格特定的 response 樂句
     */
    std::vector<float> generateStyleSpecificResponse(int styleIndex,
                                                     const std::vector<float>& call,
                                                     int lengthSteps, float intensity) {
        std::vector<float> pattern(lengthSteps, 0.0f);
        std::uniform_real_distribution<float> velVar(-0.08f, 0.08f);

        switch (styleIndex) {
            case 0:  // West African - Ensemble unison response
                // 傳統：所有鼓同時回應，力度略低於 call
                pattern[0] = 0.85f * intensity + velVar(rng);
                if (lengthSteps >= 2) {
                    pattern[lengthSteps - 1] = 0.80f * intensity + velVar(rng);
                }
                break;

            case 1:  // Afro-Cuban - Coro response
                // 傳統 Coro：固定樂句回應
                for (int i = 0; i < lengthSteps; i++) {
                    if (i == 0 || i == lengthSteps - 1) {
                        pattern[i] = 0.75f * intensity + velVar(rng);
                    } else if (i % 2 == 0) {
                        pattern[i] = 0.60f * intensity + velVar(rng);
                    }
                }
                break;

            case 4:  // Indian - Tihai-style response
                // 傳統：重複三次的短樂句
                if (lengthSteps >= 6) {
                    int phraseLen = lengthSteps / 3;
                    for (int rep = 0; rep < 3; rep++) {
                        int startPos = rep * (phraseLen + 1);
                        if (startPos < lengthSteps) {
                            pattern[startPos] = (0.65f + rep * 0.1f) * intensity + velVar(rng);
                        }
                    }
                }
                break;

            case 5:  // Gamelan - Synchronized re-entry
                // 傳統：Angsel 後的齊奏
                pattern[0] = 1.0f * intensity;  // 齊奏首音
                for (int i = 1; i < lengthSteps; i++) {
                    if (i % 2 == 0) {
                        pattern[i] = 0.65f * intensity + velVar(rng);
                    }
                }
                break;

            default:
                // 默認：使用 ECHO 類型
                return generateResponsePattern(ResponseType::ECHO, call, lengthSteps, intensity);
        }

        for (float& v : pattern) {
            v = std::clamp(v, 0.0f, 1.0f);
        }

        return pattern;
    }

    /**
     * 完整的 call-response 流程（v0.17 增強版）
     * 使用風格特定的 pattern
     */
    CallResponsePair generateEnhancedPair(int styleIndex, int barNumber,
                                          int patternLength, float intensity) {
        CallResponsePair pair = generatePair(styleIndex, barNumber, patternLength, intensity);

        // 替換為風格特定的 pattern
        pair.call.velocities = generateStyleSpecificCall(styleIndex, pair.call.lengthSteps, intensity);
        pair.response.velocities = generateStyleSpecificResponse(
            styleIndex, pair.call.velocities, pair.response.lengthSteps,
            pair.call.intensity * pair.response.intensityScale);

        return pair;
    }
};

} // namespace WorldRhythm
