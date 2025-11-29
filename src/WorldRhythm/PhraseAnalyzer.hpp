#pragma once

#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <array>

namespace WorldRhythm {

// ========================================
// Response Strategy for CV Input
// ========================================
enum class ResponseStrategy {
    COMPLEMENT,    // 互補：填補空隙，避開輸入的位置
    ECHO,          // 回聲：延遲重複輸入的節奏
    ANSWER,        // 對話：呼應式回應（類似 Call-Response）
    INTERLOCK,     // 互鎖：嚴格交替，形成緊密的節奏織體
    SHADOW,        // 影子：跟隨但稍微偏移
    DENSITY_MATCH  // 密度匹配：維持相同密度但不同位置
};

// ========================================
// Response Pattern Result
// ========================================
struct ResponsePattern {
    std::vector<float> weights;      // 16 位置權重
    std::vector<float> velocities;   // 對應的力度建議
    int suggestedOffset = 0;         // 建議偏移（以 step 為單位）
    float confidence = 0.5f;         // 分析信心度
};

// ========================================
// CV Input Analyzer for phrase detection
// ========================================
// Analyzes incoming CV to detect:
// - Onset positions and density
// - Phrase structure (period, accents)
// - Pattern similarity for adaptation
// - Generate response patterns for different strategies

class PhraseAnalyzer {
private:
    // Ring buffer for onset times (in steps)
    std::deque<int> onsetHistory;
    std::deque<float> velocityHistory;
    static constexpr int MAX_HISTORY = 128;  // 8 bars of 16 steps

    // Detected phrase parameters
    int detectedPeriod = 16;  // steps
    float detectedDensity = 0.5f;
    std::vector<float> positionWeights;  // 16 positions

    // Timing
    int currentStep = 0;
    int analysisWindow = 64;  // 4 bars

    // Gate detection
    float lastVoltage = 0.0f;
    float gateThreshold = 0.5f;

public:
    PhraseAnalyzer() : positionWeights(16, 0.5f) {}

    // ========================================
    // Process incoming CV signal
    // Call this every step
    // ========================================
    void process(float voltage, float velocity = 1.0f) {
        // Rising edge detection
        bool onset = (voltage >= gateThreshold && lastVoltage < gateThreshold);
        lastVoltage = voltage;

        if (onset) {
            // Record onset
            onsetHistory.push_back(currentStep);
            velocityHistory.push_back(velocity);

            // Maintain buffer size
            while (onsetHistory.size() > MAX_HISTORY) {
                onsetHistory.pop_front();
                velocityHistory.pop_front();
            }
        }

        currentStep++;

        // v0.18.2: 防止長期運行溢出
        // 當 currentStep 超過安全閾值時，重置並調整歷史記錄
        static constexpr int STEP_OVERFLOW_THRESHOLD = 1000000;  // 約 17 小時 @ 120BPM
        if (currentStep >= STEP_OVERFLOW_THRESHOLD) {
            // 將 currentStep 重置為 analysisWindow 大小
            int resetOffset = currentStep - analysisWindow;
            currentStep = analysisWindow;

            // 調整所有歷史 onset 時間
            for (size_t i = 0; i < onsetHistory.size(); i++) {
                onsetHistory[i] = onsetHistory[i] - resetOffset;
                // 移除已經超出視窗的舊數據
                if (onsetHistory[i] < 0) {
                    onsetHistory.erase(onsetHistory.begin(), onsetHistory.begin() + i + 1);
                    velocityHistory.erase(velocityHistory.begin(), velocityHistory.begin() + i + 1);
                    break;
                }
            }
        }

        // Periodic analysis
        if (currentStep % 16 == 0) {
            analyze();
        }
    }

    // ========================================
    // Analyze recorded onsets
    // ========================================
    void analyze() {
        if (onsetHistory.size() < 4) return;

        // Calculate density
        int windowStart = std::max(0, currentStep - analysisWindow);
        int count = 0;
        for (int onset : onsetHistory) {
            if (onset >= windowStart) count++;
        }
        detectedDensity = static_cast<float>(count) / analysisWindow;

        // Update position weights with TIME DECAY
        // 使用指數衰減：較新的 onset 權重較高，較舊的逐漸衰減
        std::fill(positionWeights.begin(), positionWeights.end(), 0.1f);

        for (size_t i = 0; i < onsetHistory.size(); i++) {
            int step = onsetHistory[i];
            if (step >= windowStart) {
                int pos = step % 16;
                float vel = velocityHistory[i];

                // 計算時間衰減因子：距離現在越遠，權重越低
                // decay = e^(-age / halfLife)，halfLife = 32 steps (2 bars)
                int age = currentStep - step;
                float decayFactor = std::exp(-static_cast<float>(age) / 32.0f);

                // 應用衰減後的權重
                positionWeights[pos] += vel * 0.2f * decayFactor;
            }
        }

        // Normalize weights
        float maxWeight = *std::max_element(positionWeights.begin(), positionWeights.end());
        if (maxWeight > 0.0f) {
            for (float& w : positionWeights) {
                w = std::clamp(w / maxWeight, 0.1f, 1.0f);
            }
        }

        // Detect period via autocorrelation
        detectPeriod();
    }

    // ========================================
    // Autocorrelation for period detection
    // ========================================
    void detectPeriod() {
        if (onsetHistory.size() < 16) return;

        // Convert recent onsets to binary pattern
        std::vector<bool> pattern(analysisWindow, false);
        int windowStart = std::max(0, currentStep - analysisWindow);

        for (int onset : onsetHistory) {
            if (onset >= windowStart) {
                int idx = onset - windowStart;
                if (idx >= 0 && idx < analysisWindow) {
                    pattern[idx] = true;
                }
            }
        }

        // Autocorrelation at different lags
        int bestPeriod = 16;
        float bestCorr = 0.0f;

        for (int lag : {8, 12, 16, 24, 32}) {
            if (lag >= analysisWindow / 2) continue;

            float corr = 0.0f;
            int count = 0;
            for (int i = 0; i < analysisWindow - lag; i++) {
                if (pattern[i] == pattern[i + lag]) {
                    corr += 1.0f;
                }
                count++;
            }
            corr /= count;

            if (corr > bestCorr) {
                bestCorr = corr;
                bestPeriod = lag;
            }
        }

        if (bestCorr > 0.6f) {
            detectedPeriod = bestPeriod;
        }
    }

    // ========================================
    // Get complementary weights
    // Returns weights for positions that are NOT occupied by input
    // ========================================
    std::vector<float> getComplementWeights() const {
        std::vector<float> complement(16);
        for (int i = 0; i < 16; i++) {
            // Invert: where input is dense, output should be sparse
            complement[i] = 1.0f - positionWeights[i] * 0.7f;
        }
        return complement;
    }

    // ========================================
    // Blend detected weights with style weights
    // ========================================
    std::vector<float> blendWithStyle(const float* styleWeights, float adaptAmount) const {
        std::vector<float> blended(16);
        for (int i = 0; i < 16; i++) {
            float styleW = styleWeights[i];
            float detectedW = positionWeights[i];
            blended[i] = styleW * (1.0f - adaptAmount) + detectedW * adaptAmount;
        }
        return blended;
    }

    // ========================================
    // Accessors
    // ========================================
    int getDetectedPeriod() const { return detectedPeriod; }
    float getDetectedDensity() const { return detectedDensity; }
    const std::vector<float>& getPositionWeights() const { return positionWeights; }

    void setAnalysisWindow(int steps) {
        analysisWindow = std::clamp(steps, 16, 128);
    }

    void setGateThreshold(float threshold) {
        gateThreshold = std::clamp(threshold, 0.1f, 5.0f);
    }

    void reset() {
        onsetHistory.clear();
        velocityHistory.clear();
        currentStep = 0;
        detectedPeriod = 16;
        detectedDensity = 0.5f;
        std::fill(positionWeights.begin(), positionWeights.end(), 0.5f);
        lastVoltage = 0.0f;
    }

    // ========================================
    // Generate Response Pattern based on Strategy
    // ========================================
    ResponsePattern generateResponse(ResponseStrategy strategy) const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);

        switch (strategy) {
            case ResponseStrategy::COMPLEMENT:
                result = generateComplementResponse();
                break;
            case ResponseStrategy::ECHO:
                result = generateEchoResponse();
                break;
            case ResponseStrategy::ANSWER:
                result = generateAnswerResponse();
                break;
            case ResponseStrategy::INTERLOCK:
                result = generateInterlockResponse();
                break;
            case ResponseStrategy::SHADOW:
                result = generateShadowResponse();
                break;
            case ResponseStrategy::DENSITY_MATCH:
                result = generateDensityMatchResponse();
                break;
        }

        return result;
    }

private:
    // ========================================
    // COMPLEMENT: 互補策略
    // 在輸入沒有打擊的位置產生節奏
    // ========================================
    ResponsePattern generateComplementResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);
        result.suggestedOffset = 0;

        // 找出輸入的空隙
        for (int i = 0; i < 16; i++) {
            // 輸入權重高的地方，回應權重低
            float inputWeight = positionWeights[i];
            result.weights[i] = std::max(0.1f, 1.0f - inputWeight * 0.9f);

            // 力度：空隙處用中等力度
            result.velocities[i] = 0.5f + 0.3f * result.weights[i];
        }

        // 強調 off-beat 位置
        for (int i = 0; i < 16; i += 2) {
            if (i + 1 < 16 && positionWeights[i] > 0.5f) {
                // 如果 on-beat 有輸入，強調 off-beat
                result.weights[i + 1] *= 1.3f;
            }
        }

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence();

        return result;
    }

    // ========================================
    // ECHO: 回聲策略
    // 延遲重複輸入的節奏模式
    // ========================================
    ResponsePattern generateEchoResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);

        // 決定回聲延遲（通常是 2-4 步）
        int echoDelay = 2;
        if (detectedDensity < 0.3f) {
            echoDelay = 4;  // 稀疏輸入用較長延遲
        } else if (detectedDensity > 0.5f) {
            echoDelay = 2;  // 密集輸入用較短延遲
        }

        result.suggestedOffset = echoDelay;

        // 位移權重
        for (int i = 0; i < 16; i++) {
            int sourcePos = (i - echoDelay + 16) % 16;
            result.weights[i] = positionWeights[sourcePos] * 0.7f;  // 回聲較弱
            result.velocities[i] = 0.3f + 0.4f * positionWeights[sourcePos];
        }

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence();

        return result;
    }

    // ========================================
    // ANSWER: 對話式回應
    // 前半聆聽，後半回應
    // ========================================
    ResponsePattern generateAnswerResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);

        // 計算輸入的 "call" 特徵
        float callIntensity = 0.0f;
        int callAccentPos = 0;

        // 分析前半部（call）
        for (int i = 0; i < 8; i++) {
            if (positionWeights[i] > callIntensity) {
                callIntensity = positionWeights[i];
                callAccentPos = i;
            }
        }

        // 前半部靜默或很輕
        for (int i = 0; i < 8; i++) {
            result.weights[i] = 0.1f;
            result.velocities[i] = 0.2f;
        }

        // 後半部產生回應
        for (int i = 8; i < 16; i++) {
            // 基本權重
            result.weights[i] = 0.5f;

            // 在對應 call 重音的位置加強
            int mirrorPos = 15 - i;  // 鏡像位置
            if (positionWeights[mirrorPos] > 0.5f) {
                result.weights[i] = 0.8f;
            }

            result.velocities[i] = 0.5f + 0.4f * callIntensity;
        }

        // 在結尾位置（14-15）加強
        result.weights[14] = std::max(result.weights[14], 0.7f);
        result.weights[15] = std::max(result.weights[15], 0.6f);

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence();

        return result;
    }

    // ========================================
    // INTERLOCK: 互鎖策略
    // 嚴格交替，形成緊密織體
    // ========================================
    ResponsePattern generateInterlockResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);
        result.suggestedOffset = 0;

        // 找出輸入的主要打擊位置
        std::vector<int> inputHits;
        float threshold = 0.4f;

        for (int i = 0; i < 16; i++) {
            if (positionWeights[i] > threshold) {
                inputHits.push_back(i);
            }
        }

        // 初始化為零
        std::fill(result.weights.begin(), result.weights.end(), 0.0f);

        // 在每個輸入打擊之間的中點產生回應
        for (size_t i = 0; i < inputHits.size(); i++) {
            int current = inputHits[i];
            int next = inputHits[(i + 1) % inputHits.size()];

            // 處理環繞
            if (next <= current) next += 16;

            // 計算中點（使用四捨五入處理奇數間隙）
            int gap = next - current;
            if (gap > 1) {
                // 使用 (gap + 1) / 2 確保奇數間隙時中點偏後（更接近真正中心）
                int midpoint = (current + (gap + 1) / 2) % 16;
                result.weights[midpoint] = 0.9f;
                result.velocities[midpoint] = 0.7f;

                // 如果間隙夠大，再加 1/4 和 3/4 位置點
                if (gap >= 4) {
                    // 使用浮點計算後取整，確保更均勻分布
                    int quarter1 = (current + static_cast<int>(gap * 0.25f + 0.5f)) % 16;
                    int quarter3 = (current + static_cast<int>(gap * 0.75f + 0.5f)) % 16;
                    result.weights[quarter1] = 0.6f;
                    result.weights[quarter3] = 0.6f;
                    result.velocities[quarter1] = 0.5f;
                    result.velocities[quarter3] = 0.5f;
                }
            }
        }

        // 確保不與輸入重疊
        for (int i = 0; i < 16; i++) {
            if (positionWeights[i] > threshold) {
                result.weights[i] = 0.0f;
            }
        }

        // 如果結果太稀疏，補充一些 off-beat
        float totalWeight = 0.0f;
        for (float w : result.weights) totalWeight += w;

        if (totalWeight < 2.0f) {
            for (int i = 1; i < 16; i += 2) {
                if (result.weights[i] < 0.3f && positionWeights[i] < 0.3f) {
                    result.weights[i] = 0.4f;
                    result.velocities[i] = 0.4f;
                }
            }
        }

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence() * 0.9f;  // 互鎖需要足夠的輸入

        return result;
    }

    // ========================================
    // SHADOW: 影子策略
    // 跟隨但稍微偏移，像影子一樣
    // ========================================
    ResponsePattern generateShadowResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);

        // 微小偏移（1 步）
        int shadowOffset = 1;
        result.suggestedOffset = shadowOffset;

        for (int i = 0; i < 16; i++) {
            int sourcePos = (i - shadowOffset + 16) % 16;

            // 跟隨但稍弱
            result.weights[i] = positionWeights[sourcePos] * 0.6f;
            result.velocities[i] = 0.3f + 0.3f * positionWeights[sourcePos];

            // 如果原位置也有輸入，降低權重避免重疊感
            if (positionWeights[i] > 0.5f) {
                result.weights[i] *= 0.5f;
            }
        }

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence();

        return result;
    }

    // ========================================
    // DENSITY_MATCH: 密度匹配策略
    // 維持相同密度但完全不同位置
    // ========================================
    ResponsePattern generateDensityMatchResponse() const {
        ResponsePattern result;
        result.weights.resize(16);
        result.velocities.resize(16);
        result.suggestedOffset = 0;

        // 計算輸入的打擊數量
        int inputHitCount = 0;
        float threshold = 0.4f;
        std::vector<bool> inputOccupied(16, false);

        for (int i = 0; i < 16; i++) {
            if (positionWeights[i] > threshold) {
                inputHitCount++;
                inputOccupied[i] = true;
            }
        }

        // 找出可用位置（輸入沒有使用的）
        std::vector<int> availablePositions;
        for (int i = 0; i < 16; i++) {
            if (!inputOccupied[i]) {
                availablePositions.push_back(i);
            }
        }

        // 初始化
        std::fill(result.weights.begin(), result.weights.end(), 0.1f);
        std::fill(result.velocities.begin(), result.velocities.end(), 0.3f);

        // 選擇與輸入數量相同的位置
        // 優先選擇韻律上重要的位置
        std::vector<std::pair<int, float>> positionScores;

        for (int pos : availablePositions) {
            float score = 0.0f;

            // 優先 downbeat
            if (pos == 0 || pos == 4 || pos == 8 || pos == 12) {
                score += 0.3f;
            }
            // 其次 upbeat
            else if (pos == 2 || pos == 6 || pos == 10 || pos == 14) {
                score += 0.2f;
            }
            // 離最近輸入打擊越遠越好
            float minDist = 16.0f;
            for (int i = 0; i < 16; i++) {
                if (inputOccupied[i]) {
                    int dist = std::min(std::abs(pos - i), 16 - std::abs(pos - i));
                    minDist = std::min(minDist, static_cast<float>(dist));
                }
            }
            score += minDist * 0.1f;

            positionScores.push_back({pos, score});
        }

        // 排序，選擇最高分的位置
        std::sort(positionScores.begin(), positionScores.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        int hitsToPlace = std::min(inputHitCount, static_cast<int>(positionScores.size()));
        for (int i = 0; i < hitsToPlace; i++) {
            int pos = positionScores[i].first;
            result.weights[pos] = 0.8f;
            result.velocities[pos] = 0.5f + 0.3f * detectedDensity;
        }

        normalizeWeights(result.weights);
        result.confidence = calculateConfidence();

        return result;
    }

    // ========================================
    // Helper: Normalize weights
    // ========================================
    void normalizeWeights(std::vector<float>& weights) const {
        float maxW = *std::max_element(weights.begin(), weights.end());
        if (maxW > 0.0f && maxW != 1.0f) {
            for (float& w : weights) {
                w = w / maxW;
            }
        }
    }

    // ========================================
    // Helper: Calculate confidence based on input quality
    // ========================================
    float calculateConfidence() const {
        // 信心度基於：
        // 1. 有足夠的 onset 歷史
        // 2. 密度在合理範圍
        // 3. 有清晰的模式

        float conf = 0.3f;

        // 歷史長度
        if (onsetHistory.size() >= 8) conf += 0.2f;
        if (onsetHistory.size() >= 16) conf += 0.1f;

        // 密度合理性
        if (detectedDensity > 0.1f && detectedDensity < 0.8f) {
            conf += 0.2f;
        }

        // 模式清晰度（位置權重的變異程度）
        float variance = 0.0f;
        float mean = 0.0f;
        for (float w : positionWeights) mean += w;
        mean /= 16.0f;
        for (float w : positionWeights) {
            variance += (w - mean) * (w - mean);
        }
        variance /= 16.0f;

        // 高變異 = 清晰模式
        if (variance > 0.05f) conf += 0.2f;

        return std::clamp(conf, 0.0f, 1.0f);
    }

public:
    // ========================================
    // Get Real-time Interlock Position
    // 即時互鎖：根據當前步驟決定是否應該打擊
    // ========================================
    bool shouldPlayInterlock(int step, float randomValue = 0.5f) const {
        int pos = step % 16;

        // 如果輸入在這個位置有打擊，我們不打
        if (positionWeights[pos] > 0.5f) {
            return false;
        }

        // 檢查前後位置
        int prev = (pos - 1 + 16) % 16;
        int next = (pos + 1) % 16;

        // 如果前一個位置有輸入打擊，這裡可能是好的回應位置
        if (positionWeights[prev] > 0.6f) {
            return randomValue < 0.7f;
        }

        // 如果後一個位置有輸入打擊，可能也是好位置
        if (positionWeights[next] > 0.6f) {
            return randomValue < 0.5f;
        }

        // 否則基於互補權重決定
        float complementWeight = 1.0f - positionWeights[pos];
        return randomValue < complementWeight * 0.4f;
    }

    // ========================================
    // Suggest Best Strategy based on Input
    // ========================================
    ResponseStrategy suggestStrategy() const {
        // 根據輸入特徵建議最適合的策略

        // 如果輸入非常稀疏，用 ECHO 或 ANSWER
        if (detectedDensity < 0.2f) {
            return ResponseStrategy::ANSWER;
        }

        // 如果輸入非常密集，用 INTERLOCK
        if (detectedDensity > 0.6f) {
            return ResponseStrategy::INTERLOCK;
        }

        // 計算輸入的規律性
        float regularity = calculateRegularity();

        // 規律的輸入適合 SHADOW
        if (regularity > 0.7f) {
            return ResponseStrategy::SHADOW;
        }

        // 中等密度用 COMPLEMENT
        return ResponseStrategy::COMPLEMENT;
    }

private:
    float calculateRegularity() const {
        // 檢查輸入是否規律（間隔均勻）
        if (onsetHistory.size() < 4) return 0.5f;

        std::vector<int> gaps;
        for (size_t i = 1; i < onsetHistory.size(); i++) {
            gaps.push_back(onsetHistory[i] - onsetHistory[i-1]);
        }

        // 計算間隔的變異係數
        float mean = 0.0f;
        for (int g : gaps) mean += g;
        mean /= gaps.size();

        float variance = 0.0f;
        for (int g : gaps) {
            variance += (g - mean) * (g - mean);
        }
        variance /= gaps.size();

        float cv = (mean > 0) ? std::sqrt(variance) / mean : 1.0f;

        // 低變異係數 = 高規律性
        return std::clamp(1.0f - cv, 0.0f, 1.0f);
    }
};

} // namespace WorldRhythm
