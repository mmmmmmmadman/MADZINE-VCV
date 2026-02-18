/**
 * MinimalDrumSynth.hpp
 * WorldRhythm v0.17 - 極簡打擊樂合成引擎
 *
 * 設計理念：
 * - 只有 2 個參數：Freq 和 Decay
 * - Sine 模式：Freq 控制振盪頻率
 * - Noise 模式：Freq 控制 BPF 中心頻率
 * - Attack 固定超快 (<1ms) 確保瞬態
 * - Velocity 直接映射到 VCA 增益
 */

#pragma once
#include <cmath>
#include <random>

namespace worldrhythm {

// 合成器模式
enum class SynthMode {
    SINE,   // 音調類：Kick, Tom, Conga, Clave, Bell
    NOISE   // 噪音類：Hi-Hat, Snare, Clap, Shaker
};

/**
 * 極簡單聲道合成器
 */
class MinimalVoice {
private:
    // 振盪器狀態
    float phase = 0.0f;
    float sampleRate = 44100.0f;

    // 噪音生成器
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};

    // BPF 狀態 (2-pole)
    float bpfZ1 = 0.0f;
    float bpfZ2 = 0.0f;

    // v0.19: BPF 係數緩存（避免每個樣本重複計算三角函數）
    float cachedFreq = -1.0f;
    float cachedSampleRate = -1.0f;
    float bpf_b0 = 0.0f, bpf_b2 = 0.0f;
    float bpf_a1 = 0.0f, bpf_a2 = 0.0f;

    // VCA 包絡狀態
    float envValue = 0.0f;
    bool triggered = false;

    // 參數
    SynthMode mode = SynthMode::SINE;
    float freq = 100.0f;      // Hz
    float decay = 200.0f;     // ms（基準值）
    float actualDecay = 200.0f; // ms（實際值，受 velocity 影響）
    float velocity = 1.0f;    // 0-1

    // Pitch sweep 參數（theKICK 引擎移植）
    float sweep = 0.0f;       // pitch sweep 量 Hz（0 = 無 sweep）
    float bend = 1.0f;        // pitch sweep 衰減速度
    float pitchEnvTime = 0.0f; // pitch envelope 計時器

    // 常數
    static constexpr float ATTACK_TIME = 0.5f;  // ms，超快攻擊
    static constexpr float BPF_Q = 2.0f;        // BPF 品質因數

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
    }

    void setMode(SynthMode m) {
        mode = m;
    }

    void setFreq(float f) {
        freq = std::max(20.0f, std::min(f, 20000.0f));
    }

    void setDecay(float d) {
        decay = std::max(1.0f, std::min(d, 5000.0f));
    }

    void setVelocity(float v) {
        velocity = std::max(0.0f, std::min(v, 1.0f));
    }

    void setSweep(float s) {
        sweep = std::max(0.0f, s);
    }

    void setBend(float b) {
        bend = std::max(0.1f, std::min(b, 10.0f));
    }

    /**
     * 觸發音符
     * velocity 影響：
     * 1. 音量（VCA 峰值）
     * 2. 聲音長短（decay 縮放）- 輕音更短促，重音更飽滿
     */
    void trigger(float vel = 1.0f) {
        velocity = vel;
        envValue = velocity;  // 直接跳到峰值（超快攻擊）
        triggered = true;
        // 從 0.25 相位開始 = sin(π/2) = 1.0，產生瞬間 click
        phase = 0.25f;
        // 重置濾波器狀態避免爆音
        bpfZ1 = 0.0f;
        bpfZ2 = 0.0f;
        // 重置 pitch envelope
        pitchEnvTime = 0.0f;
        // 計算實際 decay（velocity 影響長度，1.5 倍影響力）
        // vel=1.0 -> 100% decay, vel=0.5 -> 46% decay, vel=0.2 -> 17% decay
        // 使用平方根讓變化更自然，係數調整為 1.5 倍影響
        float velScale = 0.1f + 0.9f * std::pow(velocity, 1.5f);
        actualDecay = decay * velScale;
    }

    float getActualDecay() const { return actualDecay; }

    /**
     * 處理一個樣本
     */
    float process() {
        if (envValue < 0.0001f) {
            triggered = false;
            return 0.0f;
        }

        float output = 0.0f;

        if (mode == SynthMode::SINE) {
            // === Sine 模式 ===
            float actualFreq = freq;
            if (sweep > 0.0f) {
                // Pitch sweep（theKICK 引擎）
                float pitchTau = 0.015f / bend;
                float pitchEnv = sweep * std::exp(-pitchEnvTime / pitchTau);
                actualFreq = freq + pitchEnv;
                pitchEnvTime += 1.0f / sampleRate;
            }
            output = std::sin(2.0f * M_PI * phase);
            phase += actualFreq / sampleRate;
            if (phase >= 1.0f) phase -= 1.0f;
            // tanh saturate for kick voices with sweep
            if (sweep > 0.0f) {
                constexpr float gain = 1.6f;
                constexpr float normFactor = 0.9217f; // 1/tanh(1.6)
                output = std::tanh(gain * output) * normFactor;
            }
        } else {
            // === Noise + BPF 模式 ===
            float noise = noiseDist(rng);
            output = processBPF(noise);
        }

        // VCA 包絡（指數衰減，使用 actualDecay）
        float decaySamples = (actualDecay / 1000.0f) * sampleRate;
        float decayCoef = std::exp(-1.0f / decaySamples);
        envValue *= decayCoef;

        return output * envValue;
    }

private:
    /**
     * v0.19: 更新 BPF 係數（僅在參數變化時呼叫）
     */
    void updateBPFCoefficients() {
        if (freq == cachedFreq && sampleRate == cachedSampleRate) {
            return;  // 無變化，使用緩存
        }

        // 計算新係數
        float omega = 2.0f * M_PI * freq / sampleRate;
        float sinOmega = std::sin(omega);
        float cosOmega = std::cos(omega);
        float alpha = sinOmega / (2.0f * BPF_Q);

        float a0 = 1.0f + alpha;

        // 正規化後的係數
        bpf_b0 = alpha / a0;
        bpf_b2 = -alpha / a0;
        bpf_a1 = (-2.0f * cosOmega) / a0;
        bpf_a2 = (1.0f - alpha) / a0;

        // 更新緩存標記
        cachedFreq = freq;
        cachedSampleRate = sampleRate;
    }

    /**
     * 2-pole 帶通濾波器（v0.19 優化版）
     */
    float processBPF(float input) {
        // 確保係數是最新的（僅在參數變化時重新計算）
        updateBPFCoefficients();

        // Direct Form II（b1 = 0，已省略）
        float w = input - bpf_a1 * bpfZ1 - bpf_a2 * bpfZ2;
        float output = bpf_b0 * w + bpf_b2 * bpfZ2;

        bpfZ2 = bpfZ1;
        bpfZ1 = w;

        return output;
    }
};

/**
 * 4 聲道打擊樂合成器（對應 4 個 Role）
 */
class MinimalDrumSynth {
private:
    MinimalVoice voices[4];  // Timeline, Foundation, Groove, Lead
    float sampleRate = 44100.0f;

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        for (int i = 0; i < 4; i++) {
            voices[i].setSampleRate(sr);
        }
    }

    /**
     * 設定某個 Role 的音色
     */
    void setVoiceParams(int role, SynthMode mode, float freq, float decay, float sweep = 0.f, float bend = 1.f) {
        if (role < 0 || role > 3) return;
        voices[role].setMode(mode);
        voices[role].setFreq(freq);
        voices[role].setDecay(decay);
        voices[role].setSweep(sweep);
        voices[role].setBend(bend);
    }

    /**
     * 觸發某個 Role
     */
    void triggerVoice(int role, float velocity = 1.0f) {
        if (role < 0 || role > 3) return;
        voices[role].trigger(velocity);
    }

    /**
     * 處理一個樣本（混合 4 個聲道）
     */
    float process() {
        float mix = 0.0f;
        for (int i = 0; i < 4; i++) {
            mix += voices[i].process();
        }
        // 簡單的軟限幅
        return std::tanh(mix * 0.5f);
    }

    /**
     * 處理並輸出 4 個獨立聲道
     */
    void processSeparate(float* outputs) {
        for (int i = 0; i < 4; i++) {
            outputs[i] = voices[i].process();
        }
    }
};

/**
 * 風格預設參數
 */
struct StyleSynthPreset {
    struct VoicePreset {
        SynthMode mode;
        float freq;
        float decay;
        float sweep = 0.f;
        float bend = 1.f;
    };

    VoicePreset timeline;
    VoicePreset foundation;
    VoicePreset groove;
    VoicePreset lead;
};

/**
 * 10 種風格的預設音色
 */
inline const StyleSynthPreset STYLE_SYNTH_PRESETS[10] = {
    // 0: West African (decay × 0.6)
    {
        .timeline   = {SynthMode::SINE,  800.0f,  48.0f},   // Bell
        .foundation = {SynthMode::SINE,  80.0f,   180.0f},  // Djembe Bass
        .groove     = {SynthMode::SINE,  250.0f,  72.0f},   // Djembe Tone
        .lead       = {SynthMode::NOISE, 2000.0f, 36.0f}    // Djembe Slap
    },
    // 1: Afro-Cuban (decay × 0.6)
    {
        .timeline   = {SynthMode::SINE,  1200.0f, 18.0f},   // Clave
        .foundation = {SynthMode::SINE,  120.0f,  120.0f},  // Conga Low
        .groove     = {SynthMode::SINE,  280.0f,  60.0f},   // Conga High
        .lead       = {SynthMode::NOISE, 3000.0f, 48.0f}    // Timbales
    },
    // 2: Brazilian (decay × 0.6)
    {
        .timeline   = {SynthMode::SINE,  1000.0f, 30.0f},   // Agogo
        .foundation = {SynthMode::SINE,  60.0f,   210.0f},  // Surdo
        .groove     = {SynthMode::SINE,  400.0f,  36.0f},   // Tamborim
        .lead       = {SynthMode::NOISE, 4000.0f, 24.0f}    // Repinique
    },
    // 3: Balkan (decay × 0.6)
    {
        .timeline   = {SynthMode::NOISE, 5000.0f, 24.0f},   // Rim
        .foundation = {SynthMode::SINE,  100.0f,  150.0f},  // Tapan Bass
        .groove     = {SynthMode::SINE,  300.0f,  48.0f},   // Tarabuka Doum
        .lead       = {SynthMode::NOISE, 2500.0f, 30.0f}    // Tarabuka Tek
    },
    // 4: Indian (decay × 0.6)
    {
        .timeline   = {SynthMode::SINE,  2000.0f, 120.0f},  // Manjira
        .foundation = {SynthMode::SINE,  70.0f,   240.0f},  // Tabla Baya
        .groove     = {SynthMode::SINE,  350.0f,  90.0f},   // Tabla Daya
        .lead       = {SynthMode::SINE,  500.0f,  60.0f}    // Tabla Tin
    },
    // 5: Gamelan (decay × 0.6)
    {
        .timeline   = {SynthMode::SINE,  600.0f,  300.0f},  // Kenong
        .foundation = {SynthMode::SINE,  100.0f,  700.0f},  // Gong (縮短：1200→700)
        .groove     = {SynthMode::SINE,  800.0f,  180.0f},  // Bonang
        .lead       = {SynthMode::SINE,  1200.0f, 240.0f}   // Gender
    },
    // 6: Jazz (decay × 0.6)
    {
        .timeline   = {SynthMode::NOISE, 8000.0f, 100.0f},  // Ride (縮短：300→100)
        .foundation = {SynthMode::SINE,  55.0f,   180.0f},  // Kick
        .groove     = {SynthMode::NOISE, 2000.0f, 90.0f},   // Snare
        .lead       = {SynthMode::NOISE, 10000.0f, 30.0f}   // Hi-Hat
    },
    // 7: Electronic (decay × 0.6)
    {
        .timeline   = {SynthMode::NOISE, 9000.0f, 24.0f},                // Hi-Hat
        .foundation = {SynthMode::SINE,  50.0f,   240.0f, 120.f, 0.8f},  // 808 Kick + sweep
        .groove     = {SynthMode::NOISE, 1500.0f, 60.0f},                // Clap
        .lead       = {SynthMode::NOISE, 6000.0f, 120.0f}                // Open Hat
    },
    // 8: Breakbeat (decay × 0.6)
    {
        .timeline   = {SynthMode::NOISE, 8000.0f, 18.0f},                // Hi-Hat
        .foundation = {SynthMode::SINE,  60.0f,   150.0f, 140.f, 1.0f},  // Kick + sweep
        .groove     = {SynthMode::NOISE, 2500.0f, 72.0f},                // Snare
        .lead       = {SynthMode::NOISE, 4000.0f, 36.0f}                 // Ghost
    },
    // 9: Techno (decay × 0.6)
    {
        .timeline   = {SynthMode::NOISE, 10000.0f, 15.0f},               // Hi-Hat
        .foundation = {SynthMode::SINE,  45.0f,   210.0f, 160.f, 1.2f},  // 909 Kick + sweep
        .groove     = {SynthMode::NOISE, 1800.0f, 48.0f},                // Clap
        .lead       = {SynthMode::NOISE, 3500.0f, 30.0f}                 // Rim
    }
};

/**
 * 套用風格預設到合成器
 */
inline void applyStylePreset(MinimalDrumSynth& synth, int styleIndex) {
    if (styleIndex < 0 || styleIndex > 9) return;

    const StyleSynthPreset& preset = STYLE_SYNTH_PRESETS[styleIndex];

    synth.setVoiceParams(0, preset.timeline.mode,
                         preset.timeline.freq, preset.timeline.decay,
                         preset.timeline.sweep, preset.timeline.bend);
    synth.setVoiceParams(1, preset.foundation.mode,
                         preset.foundation.freq, preset.foundation.decay,
                         preset.foundation.sweep, preset.foundation.bend);
    synth.setVoiceParams(2, preset.groove.mode,
                         preset.groove.freq, preset.groove.decay,
                         preset.groove.sweep, preset.groove.bend);
    synth.setVoiceParams(3, preset.lead.mode,
                         preset.lead.freq, preset.lead.decay,
                         preset.lead.sweep, preset.lead.bend);
}

} // namespace worldrhythm
