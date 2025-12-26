#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <cmath>
#include "dsp/resampler.hpp"
#include <sst/filters/HalfRateFilter.h>

// ===== GUI Components =====

struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    EnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
                      NVGcolor color = nvgRGB(255, 255, 255), bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);

        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

struct NumberWithBorder : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor textColor;
    NVGcolor borderColor;
    NVGcolor backgroundColor;
    bool hasBackground;

    NumberWithBorder(Vec pos, Vec size, std::string text, float fontSize = 64.f,
                     NVGcolor textColor = nvgRGB(255, 255, 255), NVGcolor borderColor = nvgRGB(0, 0, 0),
                     NVGcolor backgroundColor = nvgRGB(0, 0, 0), bool hasBackground = false) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->textColor = textColor;
        this->borderColor = borderColor;
        this->backgroundColor = backgroundColor;
        this->hasBackground = hasBackground;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        if (hasBackground) {
            // Draw background with border
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3.0f);
            nvgFillColor(args.vg, backgroundColor);
            nvgFill(args.vg);

            // Draw border
            nvgStrokeColor(args.vg, borderColor);
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);

            // Draw simple text on background
            nvgFillColor(args.vg, textColor);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        } else {
            // Original style: text with outline
            // Draw black border (outline) for text
            nvgFillColor(args.vg, borderColor);
            float borderOffset = 1.5f;
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;
                    nvgText(args.vg, box.size.x / 2.f + x * borderOffset, box.size.y / 2.f + y * borderOffset, text.c_str(), NULL);
                }
            }

            // Draw text on top
            nvgFillColor(args.vg, textColor);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        }
    }
};

struct WhiteBackgroundBox : Widget {
    WhiteBackgroundBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);

        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(200, 200, 200, 255));
        nvgStroke(args.vg);
    }
};

// ===== Custom Knobs =====

// Custom Parameter Quantity for wave shape display
struct WaveShapeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        float value = getValue();
        if (value <= 0.1f) {
            return "Sine";
        } else if (value <= 0.2f) {
            return "Sine→Triangle";
        } else if (value <= 0.3f) {
            return "Triangle";
        } else if (value <= 0.4f) {
            return "Triangle→Saw";
        } else if (value <= 0.5f) {
            return "Saw";
        } else if (value <= 0.6f) {
            return "Saw→Pulse";
        } else {
            // Pulse width from 98% to 1%
            float pw = 0.98f - (value - 0.6f) * 2.425f; // Maps 0.6-1.0 to 0.98-0.01
            return string::f("Pulse (PW: %.0f%%)", pw * 100.f);
        }
    }
};

// Custom Parameter Quantities for frequency display
struct ModFreqParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        float value = getValue();
        const float kModFreqKnobMin = 0.001f;
        const float kModFreqKnobMax = 6000.0f;

        // Exponential mapping: knob 0 = min freq, knob 1 = max freq
        float freq = kModFreqKnobMin * std::pow(kModFreqKnobMax / kModFreqKnobMin, value);
        return freq;
    }

    void setDisplayValue(float displayValue) override {
        // Convert frequency back to knob position
        const float kModFreqKnobMin = 0.001f;
        const float kModFreqKnobMax = 6000.0f;

        // Clamp to valid range
        displayValue = clamp(displayValue, kModFreqKnobMin, kModFreqKnobMax);

        // Reverse the exponential mapping
        float value = std::log(displayValue / kModFreqKnobMin) / std::log(kModFreqKnobMax / kModFreqKnobMin);
        setValue(value);
    }

    std::string getDisplayValueString() override {
        float freq = getDisplayValue();
        if (freq < 1.f) {
            return string::f("%.3f Hz", freq);
        } else if (freq < 10.f) {
            return string::f("%.2f Hz", freq);
        } else if (freq < 100.f) {
            return string::f("%.1f Hz", freq);
        } else if (freq < 1000.f) {
            return string::f("%.0f Hz", freq);
        } else {
            return string::f("%.2f kHz", freq / 1000.f);
        }
    }

    std::string getUnit() override {
        return "";
    }
};

struct DecayParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        float value = getValue();
        float decayTime;

        if (value >= 1.f) {
            // Maximum = drone mode
            return "Drone";
        } else if (value <= 0.5f) {
            // First 50%: 0-0.3 seconds
            decayTime = value * 0.6f;
        } else {
            // Last 50%: 0.3-3 seconds
            decayTime = 0.3f + (value - 0.5f) * 5.4f;
        }

        if (decayTime < 1.f) {
            return string::f("%.2f s", decayTime);
        } else {
            return string::f("%.1f s", decayTime);
        }
    }
};

struct FinalFreqParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        float value = getValue();
        const float kFinalFreqKnobMin = 20.0f;
        const float kFinalFreqKnobMax = 8000.0f;

        // Exponential mapping: knob 0 = min freq, knob 1 = max freq
        float freq = kFinalFreqKnobMin * std::pow(kFinalFreqKnobMax / kFinalFreqKnobMin, value);
        return freq;
    }

    void setDisplayValue(float displayValue) override {
        // Convert frequency back to knob position
        const float kFinalFreqKnobMin = 20.0f;
        const float kFinalFreqKnobMax = 8000.0f;

        // Clamp to valid range
        displayValue = clamp(displayValue, kFinalFreqKnobMin, kFinalFreqKnobMax);

        // Reverse the exponential mapping
        float value = std::log(displayValue / kFinalFreqKnobMin) / std::log(kFinalFreqKnobMax / kFinalFreqKnobMin);
        setValue(value);
    }

    std::string getDisplayValueString() override {
        float freq = getDisplayValue();
        if (freq < 100.f) {
            return string::f("%.1f Hz", freq);
        } else if (freq < 1000.f) {
            return string::f("%.0f Hz", freq);
        } else {
            return string::f("%.2f kHz", freq / 1000.f);
        }
    }

    std::string getUnit() override {
        return "";
    }
};

struct LpfCutoffParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        float value = getValue();
        const float kLpfCutoffMin = 10.0f;
        const float kLpfCutoffMax = 20000.0f;

        // Exponential mapping: knob 0 = min freq, knob 1 = max freq
        float freq = kLpfCutoffMin * std::pow(kLpfCutoffMax / kLpfCutoffMin, value);
        return freq;
    }

    void setDisplayValue(float displayValue) override {
        // Convert frequency back to knob position
        const float kLpfCutoffMin = 10.0f;
        const float kLpfCutoffMax = 20000.0f;

        // Clamp to valid range
        displayValue = clamp(displayValue, kLpfCutoffMin, kLpfCutoffMax);

        // Reverse the exponential mapping
        float value = std::log(displayValue / kLpfCutoffMin) / std::log(kLpfCutoffMax / kLpfCutoffMin);
        setValue(value);
    }

    std::string getDisplayValueString() override {
        float freq = getDisplayValue();
        if (freq < 100.f) {
            return string::f("%.1f Hz", freq);
        } else if (freq < 1000.f) {
            return string::f("%.0f Hz", freq);
        } else {
            return string::f("%.2f kHz", freq / 1000.f);
        }
    }

    std::string getUnit() override {
        return "";
    }
};

// LargeWhiteKnob now using from widgets/Knobs.hpp

// SmallWhiteKnob now using from widgets/Knobs.hpp

// SmallGrayKnob now using from widgets/Knobs.hpp

// HiddenTimeKnob now using from widgets/Knobs.hpp

// Forward declaration
struct VisualDisplay;

// ===== Module Definition =====
struct NIGOQ : Module {
    enum ParamIds {
        MOD_FREQ,
        FINAL_FREQ,
        LPF_CUTOFF,
        ORDER,
        HARMONICS,
        MOD_WAVE,
        FM_AMT_ATTEN,
        FOLD_AMT_ATTEN,
        AM_AMT_ATTEN,
        MOD_FM_ATTEN,
        FINAL_FM_ATTEN,
        DECAY,
        BASS,
        FM_AMT,
        FOLD_AMT,
        AM_AMT,
        SYNC_MODE,
        SCOPE_TIME,
        TRIG_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        TRIG_IN,
        MOD_WAVE_CV,
        MOD_EXT_IN,
        FINAL_EXT_IN,
        LPF_CUTOFF_CV,
        ORDER_CV,
        FM_AMT_CV,
        HARMONICS_CV,
        FOLD_AMT_CV,
        AM_AMT_CV,
        MOD_FM_IN,
        MOD_1VOCT,
        FINAL_FM_IN,
        FINAL_1VOCT,
        NUM_INPUTS
    };

    enum OutputIds {
        MOD_SIGNAL_OUT,
        FINAL_SINE_OUT,
        FINAL_FINAL_OUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        TRIG_LIGHT,
        NUM_LIGHTS
    };

    // Scope display (exactly like Observer)
    struct ScopePoint {
        float min = INFINITY;
        float max = -INFINITY;
    };

    static constexpr int SCOPE_BUFFER_SIZE = 256; // Same as Observer
    ScopePoint finalBuffer[SCOPE_BUFFER_SIZE];
    ScopePoint modBuffer[SCOPE_BUFFER_SIZE];
    ScopePoint currentFinal;
    ScopePoint currentMod;
    int bufferIndex = 0;
    int frameIndex = 0;

    VisualDisplay* visualDisplay = nullptr;

    // Panel theme
    int panelTheme = -1; // -1 = Auto (follow VCV)

    // Oscillators
    float modPhase = 0.f;
    float finalPhase = 0.f;
    float prevFinalPhase = 0.f;  // For sync detection (FINAL syncs MOD)

    // AD Envelope implementation from AD Generator
    enum EnvelopePhase {
        ENV_IDLE,
        ENV_ATTACK,
        ENV_DECAY
    };

    struct ADEnvelope {
        EnvelopePhase phase = ENV_IDLE;
        float phaseTime = 0.0f;
        float output = 0.0f;
        dsp::SchmittTrigger trigger;

        void reset() {
            phase = ENV_IDLE;
            phaseTime = 0.0f;
            output = 0.0f;
            trigger.reset();
        }

        // Apply curve function from AD Generator
        float applyCurve(float x, float curvature) {
            x = clamp(x, 0.0f, 1.0f);

            if (curvature == 0.0f) {
                return x;
            }

            float k = curvature;
            float abs_x = std::abs(x);
            float denominator = k - 2.0f * k * abs_x + 1.0f;

            if (std::abs(denominator) < 1e-6f) {
                return x;
            }

            return (x - k * x) / denominator;
        }

        float process(float sampleTime, float triggerVoltage, float attackTime, float decayTime, float curveParam = 0.5f) {

            // Trigger detection with retrigger capability
            if (trigger.process(triggerVoltage)) {
                phase = ENV_ATTACK;
                phaseTime = 0.0f;
            }

            switch (phase) {
                case ENV_IDLE:
                    output = 0.0f;
                    break;

                case ENV_ATTACK:
                    phaseTime += sampleTime;
                    if (phaseTime >= attackTime) {
                        phase = ENV_DECAY;
                        phaseTime = 0.0f;
                        output = 1.0f;
                    } else {
                        float t = phaseTime / attackTime;
                        output = applyCurve(t, curveParam);  // Apply curve to attack
                    }
                    break;

                case ENV_DECAY:
                    phaseTime += sampleTime;
                    if (decayTime <= 0.0f) {
                        // Instant decay
                        output = 0.0f;
                        phase = ENV_IDLE;
                        phaseTime = 0.0f;
                    } else if (phaseTime >= decayTime) {
                        output = 0.0f;
                        phase = ENV_IDLE;
                        phaseTime = 0.0f;
                    } else {
                        float t = phaseTime / decayTime;
                        output = 1.0f - applyCurve(t, curveParam);  // Apply curve to decay
                    }
                    break;
            }

            return clamp(output, 0.0f, 1.0f);
        }
    };

    ADEnvelope modEnvelope;
    ADEnvelope finalEnvelope;

    // Attack time setting (in seconds)
    float attackTime = 0.01f;  // Default 10ms for punchy bass

    // Oversampling using Surge XT HalfRateFilter (block-based processing)
    static constexpr int BLOCK_SIZE = 8;  // Process 8 samples at a time
    int oversampleRate = 2;  // 1=no OS, 2=2x, 4=4x, 8=8x

    // HalfRateFilters for up/downsampling (cascaded for 4x/8x)
    sst::filters::HalfRate::HalfRateFilter upFilter1{6, true};    // First upsample stage (1x→2x)
    sst::filters::HalfRate::HalfRateFilter upFilter2{6, true};    // Second upsample stage (2x→4x)
    sst::filters::HalfRate::HalfRateFilter upFilter3{6, true};    // Third upsample stage (4x→8x)
    sst::filters::HalfRate::HalfRateFilter downFilter1{6, true};  // First downsample stage (8x→4x or 4x→2x or 2x→1x)
    sst::filters::HalfRate::HalfRateFilter downFilter2{6, true};  // Second downsample stage (4x→2x or 2x→1x)
    sst::filters::HalfRate::HalfRateFilter downFilter3{6, true};  // Third downsample stage (2x→1x)

    // Oversample buffers (sized for 8x maximum)
    static constexpr int MAX_BLOCK_SIZE_OS = BLOCK_SIZE * 8;  // 64 samples max
    float modOutputBuffer[MAX_BLOCK_SIZE_OS];      // MOD output buffer (oversampled)
    float finalOutputBuffer[MAX_BLOCK_SIZE_OS];    // FINAL output buffer (oversampled)
    float finalSineBuffer[MAX_BLOCK_SIZE_OS];      // Clean sine buffer (oversampled)
    float modOutputDownsampled[BLOCK_SIZE];        // Downsampled MOD output
    float finalOutputDownsampled[BLOCK_SIZE];      // Downsampled FINAL output
    float finalSineDownsampled[BLOCK_SIZE];        // Downsampled sine output
    int processPosition = BLOCK_SIZE + 1;          // Like Surge: trigger first process

    // Scope trigger detection (like Observer)
    dsp::SchmittTrigger scopeTriggers[16];

    // DC blocking for Order function
    float orderDCBlock = 0.0f;
    float orderDCBlock2 = 0.0f;

    // Smooth randomization variables
    float randomizeGlideTime = 1.0f;  // Default 1 second glide time
    float randomAmount = 1.0f;  // Default full range randomization (1.0 = 100%)
    bool smoothRandomizeActive = false;
    float smoothRandomizeTimer = 0.0f;
    static constexpr int PARAMS_LEN = NUM_PARAMS;
    float paramSourceValues[NUM_PARAMS] = {};
    float paramTargetValues[NUM_PARAMS] = {};

    // Random exclusion settings
    bool excludeFinalFreqFromRandom = true;  // Default: exclude final freq from randomization
    bool excludeDecayFromRandom = false;     // Default: include decay in randomization

    // Simple one-pole lowpass filter (more musical than Butterworth)
    struct SimpleLP {
        float z1 = 0.0f;
        float cutoff = 1.0f;
        float sampleRate = 44100.0f;

        void setSampleRate(float sr) {
            sampleRate = sr;
        }

        void setCutoff(float cutoffFreq) {
            // One-pole lowpass coefficient
            float fc = cutoffFreq / sampleRate;
            fc = clamp(fc, 0.0001f, 0.4999f);

            // Warped cutoff for more accurate frequency response
            float wc = std::tan(M_PI * fc);
            cutoff = wc / (1.0f + wc);
        }

        float process(float input) {
            // One-pole lowpass: y = x * cutoff + y * (1 - cutoff)
            z1 = input * cutoff + z1 * (1.0f - cutoff);
            return z1;
        }

        void reset() {
            z1 = 0.0f;
        }
    };

    // Cascade two one-pole filters for 12dB/oct slope
    struct TwoPoleLP {
        SimpleLP lp1, lp2;
        float resonance = 0.0f;

        void setSampleRate(float sr) {
            lp1.setSampleRate(sr);
            lp2.setSampleRate(sr);
        }

        void setCutoff(float cutoffFreq) {
            lp1.setCutoff(cutoffFreq);
            lp2.setCutoff(cutoffFreq);
        }

        float process(float input) {
            // Add resonance feedback
            float feedback = lp2.z1 * resonance * 0.4f;
            float stage1 = lp1.process(input - feedback);
            float output = lp2.process(stage1);
            return output;
        }

        void reset() {
            lp1.reset();
            lp2.reset();
        }
    };

    TwoPoleLP lpFilter;

    // Parameter smoothing to prevent zipper noise
    struct SmoothedParam {
        float value = 0.f;
        float target = 0.f;

        void setTarget(float newTarget) {
            target = newTarget;
        }

        float process() {
            // Exponential smoothing with ~5ms time constant at 44.1kHz
            const float alpha = 0.995f;
            value = value * alpha + target * (1.f - alpha);
            return value;
        }

        void reset(float initValue) {
            value = initValue;
            target = initValue;
        }
    };

    // Smoothed parameter values
    SmoothedParam smoothedModFreq;
    SmoothedParam smoothedFinalFreq;
    SmoothedParam smoothedLpfCutoff;
    SmoothedParam smoothedOrder;
    SmoothedParam smoothedHarmonics;
    SmoothedParam smoothedWaveMorph;
    SmoothedParam smoothedFmAmt;
    SmoothedParam smoothedFoldAmt;
    SmoothedParam smoothedSymAmt;
    SmoothedParam smoothedBass;

    // Wavefolding function with smooth, rounded folds
    float wavefold(float input, float amount) {
        // amount: 0 = no folding, 1 = max folding
        if (amount <= 0.0f) return input;

        // Progressive gain for more folds (up to 12x)
        float gain = 1.0f + amount * 11.0f;
        float amplified = input * gain;

        // Use cosine instead of sine for rounder peaks
        float folded = std::cos(amplified * M_PI * 0.25f);

        // Add harmonics with smoother blending
        if (amount > 0.35f) {
            float fold2 = std::cos(amplified * M_PI * 0.5f);
            float blend = (amount - 0.35f) / 0.65f;
            blend = blend * blend; // Square for smoother transition
            folded = folded * (1.0f - blend * 0.3f) + fold2 * blend * 0.3f;
        }

        if (amount > 0.6f) {
            float fold3 = std::cos(amplified * M_PI * 0.75f);
            float blend = (amount - 0.6f) / 0.4f;
            blend = blend * blend; // Square for smoother transition
            folded = folded * (1.0f - blend * 0.2f) + fold3 * blend * 0.2f;
        }

        if (amount > 0.8f) {
            float fold4 = std::cos(amplified * M_PI);
            float blend = (amount - 0.8f) / 0.2f;
            blend = blend * blend; // Square for smoother transition
            folded = folded * (1.0f - blend * 0.1f) + fold4 * blend * 0.1f;
        }

        // Double tanh for extra smoothness
        float output = std::tanh(folded);
        output = std::tanh(output * 1.5f);

        // Smooth crossfade
        float wetness = amount * amount; // Square for smoother response
        return input * (1.0f - wetness * 0.8f) + output * (wetness * 0.8f + 0.2f);
    }

    // Symmetry function - unipolar, creates positive asymmetry only
    float asymmetricRectifier(float input, float amount) {
        // amount: 0 = no rectification (original signal)
        //         0.5 = half rectification (negative values reduced by 50%)
        //         1.0 = full half-wave rectification (negative values become 0)

        float output = input;

        if (input < 0.0f) {
            // For negative values, apply progressive attenuation
            // At amount=0: no change
            // At amount=0.5: multiply by 0.5
            // At amount=1.0: multiply by 0 (full rectification)
            output = input * (1.0f - amount);
        }
        // Positive values pass through unchanged

        // DC blocking for asymmetric waveforms
        // More aggressive DC blocking as rectification increases
        float dcBlockCutoff = 0.995f - amount * 0.01f; // Faster response for more rectification
        orderDCBlock = orderDCBlock * dcBlockCutoff + output * (1.0f - dcBlockCutoff);
        output = output - orderDCBlock;

        // Normalize output level to compensate for energy loss
        // As we remove negative parts, boost the signal to maintain perceived loudness
        float compensation = 1.0f + amount * 0.5f; // Up to 1.5x boost at full rectification
        output *= compensation;

        // Soft clipping to prevent excessive peaks
        output = std::tanh(output * 0.8f) * 1.25f;

        return output;
    }

    // PolyBLEP function for anti-aliasing
    float polyBLEP(float t, float dt) {
        // t = phase position, dt = phase increment (frequency)
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    // Helper function to generate morphing waveforms with PolyBLEP anti-aliasing
    float generateMorphingWave(float phase, float morphParam, float phaseInc = 0.01f) {
        // morphParam:
        // 0.0-0.2 = sine to triangle
        // 0.2-0.4 = triangle to saw
        // 0.4-0.6 = saw to pulse (98%)
        // 0.6-1.0 = pulse with variable width (98% to 1%)
        // Returns bipolar signal (-1 to 1)

        float output = 0.f;

        if (morphParam <= 0.2f) {
            // Morph between sine and triangle
            float blend = morphParam * 5.f; // 0 to 1
            float sine = std::sin(2.f * M_PI * phase);
            float triangle = 2.f * std::abs(2.f * (phase - std::floor(phase + 0.5f))) - 1.f;
            output = sine * (1.f - blend) + triangle * blend;
        }
        else if (morphParam <= 0.4f) {
            // Morph between triangle and saw (with PolyBLEP)
            float blend = (morphParam - 0.2f) * 5.f; // 0 to 1
            float triangle = 2.f * std::abs(2.f * (phase - std::floor(phase + 0.5f))) - 1.f;

            // Band-limited saw with PolyBLEP (falling/ramp-down)
            float saw = 1.f - 2.f * phase;  // Reversed: starts at +1, falls to -1
            saw += polyBLEP(phase, phaseInc);  // Add instead of subtract for falling saw

            output = triangle * (1.f - blend) + saw * blend;
        }
        else if (morphParam <= 0.6f) {
            // Morph between saw and pulse (with PolyBLEP)
            float blend = (morphParam - 0.4f) * 5.f; // 0 to 1

            // Band-limited saw (falling/ramp-down)
            float saw = 1.f - 2.f * phase;  // Reversed: starts at +1, falls to -1
            saw += polyBLEP(phase, phaseInc);  // Add instead of subtract for falling saw

            // Band-limited pulse (98% duty)
            float pulseWidth = 0.98f;
            float pulse = phase < pulseWidth ? 1.f : -1.f;
            pulse += polyBLEP(phase, phaseInc);
            pulse -= polyBLEP(std::fmod(phase + (1.f - pulseWidth), 1.f), phaseInc);

            output = saw * (1.f - blend) + pulse * blend;
        }
        else {
            // Variable pulse width with PolyBLEP anti-aliasing
            float pwParam = (morphParam - 0.6f) / 0.4f; // 0 to 1
            float pulseWidth = 0.98f - pwParam * 0.97f; // 98% down to 1%

            // Band-limited pulse
            float pulse = phase < pulseWidth ? 1.f : -1.f;
            pulse += polyBLEP(phase, phaseInc);
            pulse -= polyBLEP(std::fmod(phase + (1.f - pulseWidth), 1.f), phaseInc);

            output = pulse;
        }

        return output;
    }

    NIGOQ() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure parameters with Bass preset as default
        // Bass preset: Rich low-end with controlled harmonics
        configParam<ModFreqParamQuantity>(MOD_FREQ, 0.f, 1.f, 0.25f, "Modulation Frequency");  // ~8Hz for subtle movement
        configParam<FinalFreqParamQuantity>(FINAL_FREQ, 0.f, 1.f, 0.3f, "Final Frequency");  // ~110Hz (A2 bass range)
        configParam<LpfCutoffParamQuantity>(LPF_CUTOFF, 0.f, 1.f, 0.7504f, "LPF Cutoff");  // 3kHz default
        configParam(ORDER, 0.f, 1.f, 0.15f, "Rectify Amount", "%", 0.f, 100.f);  // Asymmetric rectification
        configParam(HARMONICS, 0.f, 1.f, 0.25f, "Wavefolding", "%", 0.f, 100.f);  // Moderate fold for harmonics
        configParam<WaveShapeParamQuantity>(MOD_WAVE, 0.f, 1.f, 0.15f, "Modulation Wave Shape");  // Between sine and triangle
        configParam(FM_AMT_ATTEN, 0.f, 1.f, 0.7f, "FM CV Attenuator", "%", 0.f, 100.f);  // Moderate CV response
        configParam(FOLD_AMT_ATTEN, 0.f, 1.f, 0.7f, "TM CV Attenuator", "%", 0.f, 100.f);  // Moderate CV response
        configParam(AM_AMT_ATTEN, 0.f, 1.f, 0.7f, "RECT CV Attenuator", "%", 0.f, 100.f);  // Moderate CV response
        configParam(MOD_FM_ATTEN, 0.f, 1.f, 0.f, "Mod FM Attenuator", "%", 0.f, 100.f);
        configParam(FINAL_FM_ATTEN, 0.f, 1.f, 0.f, "Final FM Attenuator", "%", 0.f, 100.f);
        configParam<DecayParamQuantity>(DECAY, 0.f, 1.f, 0.73f, "Decay Time");  // ~1.3s for sustained bass
        configParam(BASS, 0.f, 1.f, 0.3f, "Bass/Sine Mix", "%", 0.f, 100.f);  // 30% sine for fundamental boost
        configParam(FM_AMT, 0.f, 1.f, 0.05f, "Linear FM Index", "", 0.f, 4.f);  // Subtle FM for texture
        configParam(FOLD_AMT, 0.f, 1.f, 0.5f, "TM Amount", "%", 0.f, 100.f);  // 50% timbre modulation for richer harmonics
        configParam(AM_AMT, 0.f, 1.f, 0.2f, "RECT Mod Amount", "%", 0.f, 100.f);  // Rectify modulation for wave shaping
        configSwitch(SYNC_MODE, 0.f, 2.f, 0.f, "Sync Mode", {"Off", "Soft", "Hard"});  // Sync off for bass
        // Time parameter (exactly like Observer)
        const float maxTime = -std::log2(5e1f);
        const float minTime = -std::log2(5e-3f);
        const float defaultTime = -std::log2(5e-1f);
        configParam(SCOPE_TIME, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);

        // Trigger parameter (like Observer)
        configSwitch(TRIG_PARAM, 0.f, 1.f, 1.f, "Trigger", {"Enabled", "Disabled"});
        configLight(TRIG_LIGHT, "Trigger Light");

        // Configure inputs
        configInput(TRIG_IN, "Trigger");
        configInput(MOD_WAVE_CV, "Modulation Wave CV");
        configInput(MOD_EXT_IN, "External Modulation Input");
        configInput(FINAL_EXT_IN, "External Final Input");
        configInput(LPF_CUTOFF_CV, "LPF Cutoff CV");
        configInput(ORDER_CV, "Rectify CV");
        configInput(FM_AMT_CV, "FM Amount CV");
        configInput(HARMONICS_CV, "Harmonics CV");
        configInput(FOLD_AMT_CV, "Fold Amount CV");
        configInput(AM_AMT_CV, "RECT Mod Amount CV");
        configInput(MOD_FM_IN, "Modulation FM");
        configInput(MOD_1VOCT, "Modulation 1V/Oct");
        configInput(FINAL_FM_IN, "Final FM");
        configInput(FINAL_1VOCT, "Final 1V/Oct");

        // Configure outputs
        configOutput(MOD_SIGNAL_OUT, "Modulation Signal");
        configOutput(FINAL_SINE_OUT, "Final Sine");
        configOutput(FINAL_FINAL_OUT, "Final Output");

        // Initialize oversampling filters
        setupOversamplingFilters();

        // Initialize smoothed parameters to default values
        smoothedModFreq.reset(params[MOD_FREQ].getValue());
        smoothedFinalFreq.reset(params[FINAL_FREQ].getValue());
        smoothedLpfCutoff.reset(params[LPF_CUTOFF].getValue());
        smoothedOrder.reset(params[ORDER].getValue());
        smoothedHarmonics.reset(params[HARMONICS].getValue());
        smoothedWaveMorph.reset(params[MOD_WAVE].getValue());
        smoothedFmAmt.reset(params[FM_AMT].getValue());
        smoothedFoldAmt.reset(params[FOLD_AMT].getValue());
        smoothedSymAmt.reset(params[AM_AMT].getValue());
        smoothedBass.reset(params[BASS].getValue());
    }

    void setupOversamplingFilters() {
        // Reset all HalfRateFilters
        upFilter1.reset();
        upFilter2.reset();
        upFilter3.reset();
        downFilter1.reset();
        downFilter2.reset();
        downFilter3.reset();

        // Initialize oversample buffers
        processPosition = BLOCK_SIZE + 1;  // Trigger first process
        for (int i = 0; i < MAX_BLOCK_SIZE_OS; i++) {
            modOutputBuffer[i] = 0.0f;
            finalOutputBuffer[i] = 0.0f;
            finalSineBuffer[i] = 0.0f;
        }
        for (int i = 0; i < BLOCK_SIZE; i++) {
            modOutputDownsampled[i] = 0.0f;
            finalOutputDownsampled[i] = 0.0f;
            finalSineDownsampled[i] = 0.0f;
        }

        // Setup lowpass filter with current sample rate
        lpFilter.setSampleRate(APP->engine->getSampleRate());
        lpFilter.setCutoff(8000.0f);  // Default cutoff at 8kHz
        lpFilter.reset();
    }

    void onSampleRateChange() override {
        // Update oversampling filters for new sample rate
        setupOversamplingFilters();

        // Update lowpass filter sample rate
        lpFilter.setSampleRate(APP->engine->getSampleRate());
    }

    void onRandomize(const RandomizeEvent& e) override {
        // Store current parameter values as source
        for (int i = 0; i < PARAMS_LEN; i++) {
            paramSourceValues[i] = params[i].getValue();
        }

        // Generate random target values (respecting exclusion settings)
        for (int i = 0; i < PARAMS_LEN; i++) {
            // Check if this parameter should be excluded from randomization
            bool shouldExclude = false;
            if (i == FINAL_FREQ && excludeFinalFreqFromRandom) {
                shouldExclude = true;  // Skip FINAL_FREQ if setting is enabled
            }
            if (i == DECAY && excludeDecayFromRandom) {
                shouldExclude = true;  // Skip DECAY if setting is enabled
            }

            if (!shouldExclude) {
                ParamQuantity* pq = paramQuantities[i];
                if (pq && pq->isBounded()) {
                    float currentValue = paramSourceValues[i];
                    float minValue = pq->getMinValue();
                    float maxValue = pq->getMaxValue();
                    float fullRange = maxValue - minValue;
                    float randomRange = fullRange * randomAmount;

                    // Center the random range around current value, but clamp to parameter bounds
                    float rangeMin = clamp(currentValue - randomRange * 0.5f, minValue, maxValue);
                    float rangeMax = clamp(currentValue + randomRange * 0.5f, minValue, maxValue);

                    // If the clamped range is smaller than desired, expand it within bounds
                    if (rangeMax - rangeMin < randomRange) {
                        float deficit = randomRange - (rangeMax - rangeMin);
                        if (rangeMin > minValue) {
                            rangeMin = clamp(rangeMin - deficit * 0.5f, minValue, rangeMin);
                        }
                        if (rangeMax < maxValue) {
                            rangeMax = clamp(rangeMax + deficit * 0.5f, rangeMax, maxValue);
                        }
                    }

                    paramTargetValues[i] = random::uniform() * (rangeMax - rangeMin) + rangeMin;
                } else {
                    paramTargetValues[i] = paramSourceValues[i];
                }
            } else {
                paramTargetValues[i] = paramSourceValues[i];  // Keep parameter unchanged
            }
        }

        // Start smooth randomization
        smoothRandomizeActive = true;
        smoothRandomizeTimer = 0.0f;

        // Don't call Module::onRandomize(e) as we handle everything ourselves
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "oversampleRate", json_integer(oversampleRate));
        json_object_set_new(rootJ, "attackTime", json_real(attackTime));
        // Save randomization settings
        json_object_set_new(rootJ, "randomizeGlideTime", json_real(randomizeGlideTime));
        json_object_set_new(rootJ, "randomAmount", json_real(randomAmount));
        // Save random exclusion settings
        json_object_set_new(rootJ, "excludeFinalFreqFromRandom", json_boolean(excludeFinalFreqFromRandom));
        json_object_set_new(rootJ, "excludeDecayFromRandom", json_boolean(excludeDecayFromRandom));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* oversampleRateJ = json_object_get(rootJ, "oversampleRate");
        if (oversampleRateJ) {
            oversampleRate = json_integer_value(oversampleRateJ);
            // Only allow 1x or 2x (4x/8x removed due to issues)
            if (oversampleRate != 1 && oversampleRate != 2) {
                oversampleRate = 2;  // Default to 2x if invalid
            }
        }

        // Update oversampling filters with loaded settings
        setupOversamplingFilters();

        json_t* attackTimeJ = json_object_get(rootJ, "attackTime");
        if (attackTimeJ) {
            attackTime = json_real_value(attackTimeJ);
            attackTime = clamp(attackTime, 0.0001f, 0.1f);  // Clamp to valid range
        }

        // Load randomization settings
        json_t* randomizeGlideTimeJ = json_object_get(rootJ, "randomizeGlideTime");
        if (randomizeGlideTimeJ)
            randomizeGlideTime = json_real_value(randomizeGlideTimeJ);

        json_t* randomAmountJ = json_object_get(rootJ, "randomAmount");
        if (randomAmountJ)
            randomAmount = json_real_value(randomAmountJ);

        // Load random exclusion settings
        json_t* excludeFinalFreqFromRandomJ = json_object_get(rootJ, "excludeFinalFreqFromRandom");
        if (excludeFinalFreqFromRandomJ)
            excludeFinalFreqFromRandom = json_boolean_value(excludeFinalFreqFromRandomJ);

        json_t* excludeDecayFromRandomJ = json_object_get(rootJ, "excludeDecayFromRandom");
        if (excludeDecayFromRandomJ)
            excludeDecayFromRandom = json_boolean_value(excludeDecayFromRandomJ);
    }



    // Helper struct to hold all per-sample processing state
    struct ProcessState {
        float modFreq;
        float waveMorph;
        float finalFreq;
        float fmModAmount;
        float foldAmount;
        float tmAmount;
        float rectifyAmount;
        float rectModAmount;
        float lpfCutoff;
        float bassAmount;
        float triggerVoltage;
        float decayTime;
        bool isLongDecay;
    };

    // Process a single sample at potentially oversampled rate
    // Returns: {modOutput, finalOutput, finalSineOutput}
    std::tuple<float, float, float> processSingleSample(const ProcessState& state, float oversampledSampleTime) {
        float modOutput = 0.f;
        float modSignal = 0.f;

        // Check if external modulation input is connected
        if (inputs[MOD_EXT_IN].isConnected()) {
            // Use external input (assuming ±5V audio range)
            modSignal = inputs[MOD_EXT_IN].getVoltage() / 5.f; // Normalize to ±1
            modSignal = clamp(modSignal, -1.f, 1.f);
            modOutput = (modSignal + 1.f) * 5.f; // Convert -1..1 to 0..10V
        } else {
            // Use internal oscillator
            float deltaPhase = state.modFreq * oversampledSampleTime;
            modPhase += deltaPhase;
            if (modPhase >= 1.f) {
                modPhase -= 1.f;
            }

            // Generate morphing waveform with PolyBLEP (bipolar -1 to 1)
            modSignal = generateMorphingWave(modPhase, state.waveMorph, deltaPhase);
            modOutput = (modSignal + 1.f) * 5.f; // Convert -1..1 to 0..10V
        }

        // Calculate VCA gains
        float modVcaGain, finalVcaGain;
        if (state.isLongDecay) {
            modVcaGain = 1.f;
            finalVcaGain = 1.f;
        } else {
            const float fixedCurve = -0.95f;
            modVcaGain = modEnvelope.process(oversampledSampleTime, state.triggerVoltage, attackTime, state.decayTime, fixedCurve);
            finalVcaGain = finalEnvelope.process(oversampledSampleTime, state.triggerVoltage, attackTime, state.decayTime, fixedCurve);
        }

        // Calculate modulation signal (post-VCA) for FM/TM/AM
        float modOutputWithVca = modOutput * modVcaGain;
        float modSignalForModulation;
        if (inputs[MOD_EXT_IN].isConnected()) {
            modSignalForModulation = modSignal * modVcaGain;
        } else {
            modSignalForModulation = (modOutputWithVca - 5.f) / 5.f;
        }

        // Track previous FINAL phase for sync detection (FINAL syncs MOD)
        prevFinalPhase = finalPhase;

        // Calculate base phase increment
        float basePhaseInc = state.finalFreq * oversampledSampleTime;

        // Calculate FM phase increment
        float fmPhaseInc = 0.0f;
        if (state.fmModAmount > 0.0f) {
            float fmIndex = state.fmModAmount * state.fmModAmount * 4.f;
            fmPhaseInc = state.finalFreq * modSignalForModulation * fmIndex * oversampledSampleTime;
        }

        // Total phase increment can be negative for true TZ-FM
        float finalDeltaPhase = basePhaseInc + fmPhaseInc;

        // Update final phase with TZ-FM phase increment
        finalPhase += finalDeltaPhase;

        // Get sync mode
        int syncMode = (int)params[SYNC_MODE].getValue();

        // Detect sync trigger BEFORE wrapping
        bool syncTrigger = false;
        if (finalPhase >= 1.0f && prevFinalPhase < 1.0f) {
            syncTrigger = true;
        }
        if (finalPhase < 0.0f && prevFinalPhase >= 0.0f) {
            syncTrigger = true;
        }

        // Apply sync to MOD oscillator based on mode
        if (syncTrigger && syncMode > 0) {
            if (syncMode == 2) {
                modPhase = 0.f;  // Hard sync
            } else if (syncMode == 1) {
                if (modPhase > 0.5f) {  // Soft sync
                    modPhase = 0.f;
                }
            }
        }

        // Wrap phase to [0, 1]
        finalPhase = finalPhase - std::floor(finalPhase);

        float finalSignal;

        // Check if external final input is connected
        if (inputs[FINAL_EXT_IN].isConnected()) {
            finalSignal = inputs[FINAL_EXT_IN].getVoltage() / 5.f;
            finalSignal = clamp(finalSignal, -1.f, 1.f);
        } else {
            // Generate Buchla-style "sine" with harmonics
            float fundamental = std::sin(2.f * M_PI * finalPhase);
            float harmonic2 = 0.08f * std::sin(4.f * M_PI * finalPhase);
            float harmonic3 = 0.05f * std::sin(6.f * M_PI * finalPhase);
            finalSignal = (fundamental + harmonic2 + harmonic3) * 0.92f;
        }

        // Store clean sine for later
        float cleanSine = finalSignal;

        // Apply wavefolding with TM modulation
        float foldAmountWithMod = state.foldAmount;
        if (state.tmAmount > 0.0f) {
            // Modulate fold amount with the post-VCA mod signal
            // modSignalForModulation ranges from -1 to 1, convert to 0-1 for unipolar modulation
            float timbreModulation = (modSignalForModulation * 0.5f + 0.5f) * state.tmAmount;
            foldAmountWithMod += timbreModulation;
            foldAmountWithMod = clamp(foldAmountWithMod, 0.f, 1.f);
        }

        if (foldAmountWithMod > 0.0f) {
            finalSignal = wavefold(finalSignal, foldAmountWithMod);
        }

        // Apply rectification with RECT modulation
        float rectifyAmountWithMod = state.rectifyAmount;
        if (state.rectModAmount > 0.0f) {
            // Modulate rectification with the post-VCA mod signal
            float rectModulation = (modSignalForModulation * 0.5f + 0.5f) * state.rectModAmount;
            rectifyAmountWithMod += rectModulation;
            rectifyAmountWithMod = clamp(rectifyAmountWithMod, 0.f, 1.f);
        }

        finalSignal = asymmetricRectifier(finalSignal, rectifyAmountWithMod);

        // Apply lowpass filter
        lpFilter.setCutoff(state.lpfCutoff);
        finalSignal = lpFilter.process(finalSignal);

        // Apply VCA and scale to ±5V
        float finalOutput = finalSignal * 5.f * finalVcaGain;
        float finalSineOutput = cleanSine * 5.f * finalVcaGain;

        // Apply BASS knob
        if (state.bassAmount > 0.0f) {
            float cleanSineScaled = finalSineOutput * state.bassAmount * 2.0f;
            finalOutput = finalOutput + cleanSineScaled;

            // Soft clipping
            if (std::abs(finalOutput) > 5.0f) {
                float sign = finalOutput > 0 ? 1.0f : -1.0f;
                float excess = std::abs(finalOutput) - 5.0f;
                finalOutput = sign * (5.0f + std::tanh(excess * 0.3f) * 2.0f);
            }
        }

        return std::make_tuple(modOutputWithVca, finalOutput, finalSineOutput);
    }

    void process(const ProcessArgs& args) override {
        // Handle smooth randomization
        if (smoothRandomizeActive) {
            smoothRandomizeTimer += args.sampleTime;
            float progress = smoothRandomizeTimer / randomizeGlideTime;

            if (progress >= 1.0f) {
                // Randomization complete
                progress = 1.0f;
                smoothRandomizeActive = false;
            }

            // Smooth interpolation using sine curve for more natural feel
            float smoothProgress = (1.0f - cosf(progress * M_PI)) * 0.5f;

            // Apply interpolated values to parameters
            for (int i = 0; i < PARAMS_LEN; i++) {
                // Check if this parameter should be excluded from smooth randomization
                bool shouldExclude = false;
                if (i == FINAL_FREQ && excludeFinalFreqFromRandom) {
                    shouldExclude = true;
                }
                if (i == DECAY && excludeDecayFromRandom) {
                    shouldExclude = true;
                }

                if (!shouldExclude) {
                    float currentValue = paramSourceValues[i] + (paramTargetValues[i] - paramSourceValues[i]) * smoothProgress;
                    params[i].setValue(currentValue);
                }
            }
        }

        // Update smoothed parameter targets
        smoothedModFreq.setTarget(params[MOD_FREQ].getValue());
        smoothedFinalFreq.setTarget(params[FINAL_FREQ].getValue());
        smoothedLpfCutoff.setTarget(params[LPF_CUTOFF].getValue());
        smoothedOrder.setTarget(params[ORDER].getValue());
        smoothedHarmonics.setTarget(params[HARMONICS].getValue());
        smoothedWaveMorph.setTarget(params[MOD_WAVE].getValue());
        smoothedFmAmt.setTarget(params[FM_AMT].getValue());
        smoothedFoldAmt.setTarget(params[FOLD_AMT].getValue());
        smoothedSymAmt.setTarget(params[AM_AMT].getValue());
        smoothedBass.setTarget(params[BASS].getValue());

        // Process smoothed parameters
        float modFreqKnob = smoothedModFreq.process();
        const float kModFreqKnobMin = 0.001f;
        const float kModFreqKnobMax = 6000.0f;
        float modFreq = kModFreqKnobMin * std::pow(kModFreqKnobMax / kModFreqKnobMin, modFreqKnob);

        // Apply 1V/Oct CV if connected
        if (inputs[MOD_1VOCT].isConnected()) {
            float voct = inputs[MOD_1VOCT].getVoltage();
            modFreq *= std::pow(2.f, voct);
        }

        // Apply FM if connected
        if (inputs[MOD_FM_IN].isConnected()) {
            float fmAmount = params[MOD_FM_ATTEN].getValue();
            float fmSignal = inputs[MOD_FM_IN].getVoltage() / 5.f; // Normalize to ±1
            modFreq *= (1.f + fmSignal * fmAmount);
        }

        // Clamp frequency to valid range
        modFreq = clamp(modFreq, 0.001f, args.sampleRate * oversampleRate / 2.f);

        // Get wave morph parameter
        float waveMorph = smoothedWaveMorph.process();

        // Apply wave CV if connected
        if (inputs[MOD_WAVE_CV].isConnected()) {
            float waveCV = inputs[MOD_WAVE_CV].getVoltage() / 10.f; // Normalize 0-10V to 0-1
            waveMorph = clamp(waveMorph + waveCV, 0.f, 1.f);
        }

        //  ===== BLOCK-BASED OVERSAMPLE PROCESSING (Surge XT style) =====

        // Prepare processing state from current parameters
        // Get decay parameter
        float decayParam = params[DECAY].getValue();
        float decayTime;
        if (decayParam <= 0.5f) {
            decayTime = decayParam * 0.6f;
        } else {
            decayTime = 0.3f + (decayParam - 0.5f) * 5.4f;
        }

        float triggerVoltage = inputs[TRIG_IN].isConnected() ? inputs[TRIG_IN].getVoltage() : 0.0f;
        bool isLongDecay = (decayTime >= 3.f);

        if (isLongDecay) {
            modEnvelope.reset();
            finalEnvelope.reset();
        }

        // Get FINAL frequency
        float finalFreqKnob = smoothedFinalFreq.process();
        const float kFinalFreqKnobMin = 20.0f;
        const float kFinalFreqKnobMax = 8000.0f;
        float finalFreq = kFinalFreqKnobMin * std::pow(kFinalFreqKnobMax / kFinalFreqKnobMin, finalFreqKnob);

        // Apply 1V/Oct CV if connected
        if (inputs[FINAL_1VOCT].isConnected()) {
            float voct = inputs[FINAL_1VOCT].getVoltage();
            finalFreq *= std::pow(2.f, voct);
        }

        // Apply external Linear FM if connected
        if (inputs[FINAL_FM_IN].isConnected()) {
            float fmAmount = params[FINAL_FM_ATTEN].getValue();
            float fmSignal = inputs[FINAL_FM_IN].getVoltage() / 5.f;
            finalFreq *= (1.f + fmSignal * fmAmount * 10.f);
        }

        // Internal FM amount
        float fmModAmount = smoothedFmAmt.process();
        if (inputs[FM_AMT_CV].isConnected()) {
            float fmAttenuation = params[FM_AMT_ATTEN].getValue();
            float fmCV = inputs[FM_AMT_CV].getVoltage() / 10.f;
            fmModAmount += fmCV * fmAttenuation;
            fmModAmount = clamp(fmModAmount, 0.f, 1.f);
        }

        // Get fold amount
        float foldAmount = smoothedHarmonics.process();
        if (inputs[HARMONICS_CV].isConnected()) {
            float foldCV = inputs[HARMONICS_CV].getVoltage() / 10.f;
            foldAmount += foldCV;
            foldAmount = clamp(foldAmount, 0.f, 1.f);
        }

        // TM amount
        float tmAmount = smoothedFoldAmt.process();
        if (inputs[FOLD_AMT_CV].isConnected()) {
            float tmAttenuation = params[FOLD_AMT_ATTEN].getValue();
            float tmCV = inputs[FOLD_AMT_CV].getVoltage() / 10.f;
            tmAmount += tmCV * tmAttenuation;
            tmAmount = clamp(tmAmount, 0.f, 1.f);
        }

        // Rectify amount
        float rectifyAmount = smoothedOrder.process();
        if (inputs[ORDER_CV].isConnected()) {
            float rectifyCV = inputs[ORDER_CV].getVoltage() / 10.f;
            rectifyAmount += rectifyCV;
            rectifyAmount = clamp(rectifyAmount, 0.f, 1.f);
        }

        // RECT modulation amount
        float rectModAmount = smoothedSymAmt.process();
        if (inputs[AM_AMT_CV].isConnected()) {
            float rectModAttenuation = params[AM_AMT_ATTEN].getValue();
            float rectModCV = inputs[AM_AMT_CV].getVoltage() / 10.f;
            rectModAmount += rectModCV * rectModAttenuation;
            rectModAmount = clamp(rectModAmount, 0.f, 1.f);
        }

        // LPF cutoff
        float lpfCutoffParam = smoothedLpfCutoff.process();
        const float kLpfCutoffMin = 10.0f;
        const float kLpfCutoffMax = 20000.0f;
        float lpfCutoff = kLpfCutoffMin * std::pow(kLpfCutoffMax / kLpfCutoffMin, lpfCutoffParam);

        if (inputs[LPF_CUTOFF_CV].isConnected()) {
            float lpfCV = inputs[LPF_CUTOFF_CV].getVoltage() / 10.f;
            float cvAmount = lpfCV * 2.f - 1.f;
            lpfCutoff *= std::pow(2.f, cvAmount * 2.f);
        }

        lpfCutoff = clamp(lpfCutoff, 20.f, args.sampleRate * oversampleRate / 2.f * 0.49f);

        // Bass amount
        float bassAmount = smoothedBass.process();

        // Build processing state
        ProcessState state;
        state.modFreq = modFreq;
        state.waveMorph = waveMorph;
        state.finalFreq = finalFreq;
        state.fmModAmount = fmModAmount;
        state.foldAmount = foldAmount;
        state.tmAmount = tmAmount;
        state.rectifyAmount = rectifyAmount;
        state.rectModAmount = rectModAmount;
        state.lpfCutoff = lpfCutoff;
        state.bassAmount = bassAmount;
        state.triggerVoltage = triggerVoltage;
        state.decayTime = decayTime;
        state.isLongDecay = isLongDecay;

        // ===== BLOCK PROCESSING (Like Surge XT) =====
        // Output variables for this sample
        float modOutputFinal, finalOutputFinal, finalSineOutputFinal;

        if (oversampleRate == 1) {
            // No oversampling - direct processing
            auto [modOut, finalOut, sineOut] = processSingleSample(state, args.sampleTime);
            modOutputFinal = modOut;
            finalOutputFinal = finalOut;
            finalSineOutputFinal = sineOut;
        } else {
            // Block-based 2x oversampling (only 2x supported)
            if (processPosition >= BLOCK_SIZE) {
                processPosition = 0;

                // Calculate oversample time (2x only)
                float oversampledSampleTime = args.sampleTime / 2.0f;
                int blockSizeOS = BLOCK_SIZE * 2;  // 16 samples

                // Process entire block at 2x rate
                for (int i = 0; i < blockSizeOS; i++) {
                    auto [modOut, finalOut, sineOut] = processSingleSample(state, oversampledSampleTime);
                    modOutputBuffer[i] = modOut;
                    finalOutputBuffer[i] = finalOut;
                    finalSineBuffer[i] = sineOut;
                }

                // Downsample: 2x → 1x (16 → 8 samples)
                // Note: process_block_D2 requires (floatL, floatR, nsamples)
                // For mono signals, use same buffer for both L and R
                downFilter1.process_block_D2(modOutputBuffer, modOutputBuffer, BLOCK_SIZE * 2);
                downFilter2.process_block_D2(finalOutputBuffer, finalOutputBuffer, BLOCK_SIZE * 2);
                downFilter3.process_block_D2(finalSineBuffer, finalSineBuffer, BLOCK_SIZE * 2);

                // Copy downsampled results
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    modOutputDownsampled[i] = modOutputBuffer[i];
                    finalOutputDownsampled[i] = finalOutputBuffer[i];
                    finalSineDownsampled[i] = finalSineBuffer[i];
                }
            }

            // Output current sample from downsampled buffers
            modOutputFinal = modOutputDownsampled[processPosition];
            finalOutputFinal = finalOutputDownsampled[processPosition];
            finalSineOutputFinal = finalSineDownsampled[processPosition];

            processPosition++;
        }

        // Set outputs
        outputs[MOD_SIGNAL_OUT].setVoltage(modOutputFinal);
        outputs[FINAL_SINE_OUT].setVoltage(finalSineOutputFinal);
        outputs[FINAL_FINAL_OUT].setVoltage(finalOutputFinal);

        // Trigger light control
        bool trig = !params[TRIG_PARAM].getValue();
        lights[TRIG_LIGHT].setBrightness(trig);

        // Scope recording
        if (bufferIndex >= SCOPE_BUFFER_SIZE) {
            bool triggered = false;

            if (!trig) {
                triggered = true;
            } else {
                if (scopeTriggers[0].process(rescale(finalSineOutputFinal, 0.f, 0.001f, 0.f, 1.f))) {
                    triggered = true;
                }
            }

            if (triggered) {
                for (int c = 0; c < 16; c++) {
                    scopeTriggers[c].reset();
                }
                bufferIndex = 0;
                frameIndex = 0;
            }
        }

        if (bufferIndex < SCOPE_BUFFER_SIZE) {
            float deltaTime = dsp::exp2_taylor5(-params[SCOPE_TIME].getValue()) / SCOPE_BUFFER_SIZE;
            int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

            float modSample = modOutputFinal / 5.0f - 1.0f;
            float finalSample = finalOutputFinal / 5.0f;
            currentFinal.min = std::min(currentFinal.min, finalSample);
            currentFinal.max = std::max(currentFinal.max, finalSample);
            currentMod.min = std::min(currentMod.min, modSample);
            currentMod.max = std::max(currentMod.max, modSample);

            if (++frameIndex >= frameCount) {
                frameIndex = 0;
                finalBuffer[bufferIndex] = currentFinal;
                modBuffer[bufferIndex] = currentMod;
                currentFinal = ScopePoint();
                currentMod = ScopePoint();
                bufferIndex++;
            }
        }
    }
};

// ===== Dual-track Scope Display Widget =====
struct VisualDisplay : Widget {
    NIGOQ* module;

    VisualDisplay(NIGOQ* module) : module(module) {
        box.size = Vec(66, 38.5);
    }

    void draw(const DrawArgs& args) override {
        if (!module) return;

        // Draw background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        // Draw border
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 255));
        nvgStroke(args.vg);

        // Draw center line
        float centerY = box.size.y / 2;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, centerY);
        nvgLineTo(args.vg, box.size.x, centerY);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 30));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);

        // Draw PRIN trace (top half, pink)
        float trackHeight = box.size.y / 2.0f;
        float trackY = 0;

        nvgSave(args.vg);
        Rect b = Rect(Vec(0, trackY), Vec(box.size.x, trackHeight));
        nvgScissor(args.vg, RECT_ARGS(b));
        nvgBeginPath(args.vg);

        for (int i = 0; i < NIGOQ::SCOPE_BUFFER_SIZE; i++) {
            const NIGOQ::ScopePoint& point = module->finalBuffer[i];
            float value = point.max;
            if (!std::isfinite(value))
                value = 0.f;

            Vec p;
            p.x = (float)i / (NIGOQ::SCOPE_BUFFER_SIZE - 1) * b.size.x;
            // Scale signal properly (already normalized to +/-1 in process)
            p.y = b.pos.y + b.size.y * 0.5f * (1.f - value);

            if (i == 0)
                nvgMoveTo(args.vg, p.x, p.y);
            else
                nvgLineTo(args.vg, p.x, p.y);
        }

        nvgStrokeColor(args.vg, nvgRGB(255, 133, 133)); // Pink
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
        nvgResetScissor(args.vg);
        nvgRestore(args.vg);

        // Draw MOD trace (bottom half, cyan)
        trackY = trackHeight;

        nvgSave(args.vg);
        b = Rect(Vec(0, trackY), Vec(box.size.x, trackHeight));
        nvgScissor(args.vg, RECT_ARGS(b));
        nvgBeginPath(args.vg);

        for (int i = 0; i < NIGOQ::SCOPE_BUFFER_SIZE; i++) {
            const NIGOQ::ScopePoint& point = module->modBuffer[i];
            float value = point.max;
            if (!std::isfinite(value))
                value = 0.f;

            Vec p;
            p.x = (float)i / (NIGOQ::SCOPE_BUFFER_SIZE - 1) * b.size.x;
            // Scale signal properly (already normalized to +/-1 in process)
            p.y = b.pos.y + b.size.y * 0.5f * (1.f - value);

            if (i == 0)
                nvgMoveTo(args.vg, p.x, p.y);
            else
                nvgLineTo(args.vg, p.x, p.y);
        }

        nvgStrokeColor(args.vg, nvgRGB(133, 200, 255)); // Cyan
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
        nvgResetScissor(args.vg);
        nvgRestore(args.vg);
    }
};

// ===== Clickable Light Button =====
struct ClickableLight : ParamWidget {
    NIGOQ* module;

    ClickableLight() {
        box.size = Vec(8, 8);
    }

    void draw(const DrawArgs& args) override {
        if (!module) return;

        float brightness = module->lights[NIGOQ::TRIG_LIGHT].getBrightness();

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, box.size.x / 2 - 1);

        if (brightness > 0.5f) {
            // Pink when trigger is disabled (free running)
            nvgFillColor(args.vg, nvgRGB(255, 133, 133));
        } else {
            // Dark gray when trigger is enabled (waiting for trigger)
            nvgFillColor(args.vg, nvgRGB(80, 80, 80));
        }
        nvgFill(args.vg);

        nvgStrokeColor(args.vg, nvgRGB(200, 200, 200));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                float newValue = pq->getValue() > 0.5f ? 0.f : 1.f;
                pq->setValue(newValue);
            }
            e.consume(this);
        }
    }
};

// ===== Module Widget =====
struct NIGOQWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    NIGOQWidget(NIGOQ* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");


        // Title labels
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(180, 20), "N I G O Q", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(180, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // White background box
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        // "259m" Numbers
        addChild(new NumberWithBorder(Vec(20, 245), Vec(30, 35), "2", 72.f, nvgRGB(255, 255, 255), nvgRGB(0, 0, 0)));
        addChild(new NumberWithBorder(Vec(45, 245), Vec(30, 35), "5", 72.f, nvgRGB(255, 255, 255), nvgRGB(0, 0, 0)));
        addChild(new NumberWithBorder(Vec(70, 245), Vec(30, 35), "9", 72.f, nvgRGB(255, 255, 255), nvgRGB(0, 0, 0)));
        addChild(new NumberWithBorder(Vec(100, 250), Vec(21, 25), "m", 50.4f, nvgRGB(255, 182, 193), nvgRGB(0, 0, 0)));

        // Inputs
        addInput(createInputCentered<PJ301MPort>(Vec(165, 55), module, NIGOQ::TRIG_IN));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 95), module, NIGOQ::MOD_WAVE_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(55, 92), module, NIGOQ::MOD_EXT_IN));
        addInput(createInputCentered<PJ301MPort>(Vec(125, 92), module, NIGOQ::FINAL_EXT_IN));
        addInput(createInputCentered<PJ301MPort>(Vec(165, 130), module, NIGOQ::LPF_CUTOFF_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(165, 175), module, NIGOQ::ORDER_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 130), module, NIGOQ::FM_AMT_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(165, 220), module, NIGOQ::HARMONICS_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 220), module, NIGOQ::FOLD_AMT_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 175), module, NIGOQ::AM_AMT_CV));
        addInput(createInputCentered<PJ301MPort>(Vec(50, 310), module, NIGOQ::MOD_FM_IN));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 310), module, NIGOQ::MOD_1VOCT));
        addInput(createInputCentered<PJ301MPort>(Vec(135, 310), module, NIGOQ::FINAL_FM_IN));
        addInput(createInputCentered<PJ301MPort>(Vec(165, 310), module, NIGOQ::FINAL_1VOCT));

        // Large white knobs with custom param quantities
        addParam(createParamCentered<madzine::widgets::LargeWhiteKnob>(Vec(55, 55), module, NIGOQ::MOD_FREQ));
        addParam(createParamCentered<madzine::widgets::LargeWhiteKnob>(Vec(125, 55), module, NIGOQ::FINAL_FREQ));

        // Custom ParamQuantities are now configured in NIGOQ constructor via configParam<>

        // Standard black knobs (same as PPaTTTerning)
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(125, 130), module, NIGOQ::LPF_CUTOFF));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(125, 175), module, NIGOQ::ORDER));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(125, 220), module, NIGOQ::HARMONICS));

        // Multiverse-style knobs for MOD WAVE and CV attenuators (26×26px, 深灰+白色內圈+粉紅指示器)
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(20, 55), module, NIGOQ::MOD_WAVE));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(55, 130), module, NIGOQ::FM_AMT_ATTEN));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(55, 220), module, NIGOQ::FOLD_AMT_ATTEN));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(55, 175), module, NIGOQ::AM_AMT_ATTEN));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(77, 310), module, NIGOQ::MOD_FM_ATTEN));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(108, 310), module, NIGOQ::FINAL_FM_ATTEN));

        // Small gray knob for DECAY only
        addParam(createParamCentered<madzine::widgets::SmallGrayKnob>(Vec(165, 90), module, NIGOQ::DECAY));

        // MediumGrayKnob for modulation amounts and BASS - like MADDY FILL knobs (26×26px)
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(90, 130), module, NIGOQ::FM_AMT));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(90, 220), module, NIGOQ::FOLD_AMT));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(90, 175), module, NIGOQ::AM_AMT));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(165, 265), module, NIGOQ::BASS));

        // Switch
        addParam(createParamCentered<CKSSThree>(Vec(90, 85), module, NIGOQ::SYNC_MODE));

        // Input labels
        addChild(new EnhancedTextLabel(Vec(145, 34), Vec(40, 10), "TRIG", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 74), Vec(40, 10), "WAVE", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(145, 109), Vec(40, 10), "LPF", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(145, 154), Vec(40, 10), "RECT", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 109), Vec(40, 10), "FM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(145, 199), Vec(40, 10), "FOLD", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 199), Vec(40, 10), "TM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 154), Vec(40, 10), "RECT", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(30, 289), Vec(40, 10), "M.FM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 289), Vec(40, 10), "M.V/O", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(115, 289), Vec(40, 10), "F.FM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(145, 289), Vec(40, 10), "F.V/O", 7.f, nvgRGB(255, 255, 255), true));

        // Parameter labels
        addChild(new EnhancedTextLabel(Vec(23, 26), Vec(64, 15), "MOD FREQ", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(93, 26), Vec(64, 15), "FINAL FREQ", 7.f, nvgRGB(255, 255, 255), true));

        // External input labels
        addChild(new EnhancedTextLabel(Vec(40, 71), Vec(30, 10), "EXT IN", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(110, 71), Vec(30, 10), "EXT IN", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(93, 103), Vec(64, 12), "LPF", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(102, 148), Vec(46, 12), "RECTIFY", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(90, 193), Vec(70, 12), "FOLD", 7.f, nvgRGB(255, 255, 255), true));

        // Small knob labels
        addChild(new EnhancedTextLabel(Vec(4, 34), Vec(30, 12), "WAVE", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(30, 109), Vec(50, 10), "CV ATT", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(30, 199), Vec(50, 10), "CV ATT", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(30, 154), Vec(50, 10), "CV ATT", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(62, 289), Vec(30, 10), "M.FM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(93, 289), Vec(30, 10), "F.FM", 7.f, nvgRGB(255, 255, 255), true));

        // Gray knob labels
        addChild(new EnhancedTextLabel(Vec(150, 70), Vec(30, 10), "DEC", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(150, 242), Vec(30, 10), "BASS", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(78, 107), Vec(25, 12), "FM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(75, 197), Vec(30, 12), "TM", 7.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(78, 152), Vec(25, 12), "RECT", 7.f, nvgRGB(255, 255, 255), true));

        // Switch label
        addChild(new EnhancedTextLabel(Vec(75, 60), Vec(30, 12), "SYNC", 7.f, nvgRGB(255, 255, 255), true));

        // Add scope display
        VisualDisplay* scopeDisplay = new VisualDisplay(module);
        scopeDisplay->box.pos = Vec(40, 335);
        addChild(scopeDisplay);
        if (module) {
            module->visualDisplay = scopeDisplay;
        }

        // Hidden time knob (overlaps scope display)
        addParam(createParam<madzine::widgets::HiddenTimeKnobNIGOQ>(Vec(40, 335), module, NIGOQ::SCOPE_TIME));

        // Trigger light button (to the right of scope)
        ClickableLight* trigLight = createParam<ClickableLight>(Vec(110, 330), module, NIGOQ::TRIG_PARAM);
        trigLight->module = module;
        addParam(trigLight);

        // Outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(20, 360), module, NIGOQ::MOD_SIGNAL_OUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(135, 360), module, NIGOQ::FINAL_SINE_OUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(165, 360), module, NIGOQ::FINAL_FINAL_OUT));

        // Output labels (pink color)
        addChild(new EnhancedTextLabel(Vec(0, 339), Vec(40, 10), "MOD", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(120, 339), Vec(30, 10), "SINE", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(145, 339), Vec(40, 10), "FINAL", 7.f, nvgRGB(255, 133, 133), true));
    }

    void step() override {
        NIGOQ* module = dynamic_cast<NIGOQ*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(Menu* menu) override {
        NIGOQ* module = dynamic_cast<NIGOQ*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);

        menu->addChild(new MenuSeparator);

        menu->addChild(createMenuLabel("Oversampling"));

        // Simple checkbox for 2x oversample on/off
        struct OversampleMenuItem : MenuItem {
            NIGOQ* module;
            void onAction(const event::Action& e) override {
                module->oversampleRate = (module->oversampleRate == 2) ? 1 : 2;
                module->setupOversamplingFilters();
            }
        };

        OversampleMenuItem* oversampleItem = createMenuItem<OversampleMenuItem>("2x Oversample", CHECKMARK(module->oversampleRate == 2));
        oversampleItem->module = module;
        menu->addChild(oversampleItem);

        menu->addChild(new MenuSeparator);

        // Surge XT HalfRateFilter has fixed quality (M=6, steep=true)
        // No quality menu needed - it's always highest quality

        menu->addChild(new MenuSeparator);

        menu->addChild(createMenuLabel("Attack Time"));

        struct AttackTimeSlider : ui::Slider {
            struct AttackTimeQuantity : Quantity {
                NIGOQ* module;
                AttackTimeQuantity(NIGOQ* module) : module(module) {}

                void setValue(float value) override {
                    if (module) {
                        value = clamp(value, 0.0f, 1.0f);
                        // Map 0-1 to 0.1ms - 100ms with exponential scaling
                        float minAttack = 0.0001f;  // 0.1 ms
                        float maxAttack = 0.1f;     // 100 ms
                        // Exponential mapping for better control at low values
                        module->attackTime = minAttack * std::pow(maxAttack / minAttack, value);
                    }
                }

                float getValue() override {
                    if (module) {
                        float minAttack = 0.0001f;
                        float maxAttack = 0.1f;
                        // Reverse exponential mapping
                        return std::log(module->attackTime / minAttack) / std::log(maxAttack / minAttack);
                    }
                    return 0.21f;  // Default position for 1ms
                }

                float getMinValue() override { return 0.0f; }
                float getMaxValue() override { return 1.0f; }
                float getDefaultValue() override { return 0.21f; }  // Default for 1ms
                std::string getLabel() override { return "Attack Time"; }
                std::string getUnit() override { return " ms"; }
                std::string getDisplayValueString() override {
                    if (module) {
                        float ms = module->attackTime * 1000.0f;
                        if (ms < 1.0f) {
                            return string::f("%.2f", ms);
                        } else if (ms < 10.0f) {
                            return string::f("%.1f", ms);
                        } else {
                            return string::f("%.0f", ms);
                        }
                    }
                    return "1.00";
                }
            };

            AttackTimeSlider(NIGOQ* module) {
                box.size.x = 200.0f;
                quantity = new AttackTimeQuantity(module);
            }

            ~AttackTimeSlider() {
                delete quantity;
            }
        };

        AttackTimeSlider* slider = new AttackTimeSlider(module);

        struct AttackTimeDisplay : ui::MenuLabel {
            NIGOQ* module;

            AttackTimeDisplay(NIGOQ* module) : module(module) {
                text = "1.00 ms";
            }

            void step() override {
                if (module) {
                    float ms = module->attackTime * 1000.0f;
                    if (ms < 1.0f) {
                        text = string::f("%.2f ms", ms);
                    } else if (ms < 10.0f) {
                        text = string::f("%.1f ms", ms);
                    } else {
                        text = string::f("%.0f ms", ms);
                    }
                }
                ui::MenuLabel::step();
            }
        };

        AttackTimeDisplay* display = new AttackTimeDisplay(module);

        menu->addChild(slider);
        menu->addChild(display);

        menu->addChild(new MenuSeparator);

        // Randomize Glide Time Slider
        menu->addChild(createMenuLabel("Randomize Glide Time"));

        struct GlideTimeSlider : ui::Slider {
            struct GlideTimeQuantity : Quantity {
                NIGOQ* module;
                GlideTimeQuantity(NIGOQ* module) : module(module) {}

                void setValue(float value) override {
                    if (module) {
                        value = clamp(value, 0.0f, 1.0f);
                        // Map 0-1 to 0.1s - 30s with exponential scaling for better control
                        float minGlide = 0.1f;   // 0.1 seconds
                        float maxGlide = 30.0f;   // 30 seconds
                        // Exponential mapping for more precision at low values
                        module->randomizeGlideTime = minGlide * std::pow(maxGlide / minGlide, value);
                    }
                }

                float getValue() override {
                    if (module) {
                        float minGlide = 0.1f;
                        float maxGlide = 30.0f;
                        // Reverse exponential mapping
                        return std::log(module->randomizeGlideTime / minGlide) / std::log(maxGlide / minGlide);
                    }
                    return 0.37f;  // Default position for 1s
                }

                float getMinValue() override { return 0.0f; }
                float getMaxValue() override { return 1.0f; }
                float getDefaultValue() override { return 0.37f; }  // Default for 1s
                std::string getLabel() override { return "Glide Time"; }
                std::string getUnit() override { return " s"; }
                std::string getDisplayValueString() override {
                    if (module) {
                        if (module->randomizeGlideTime < 1.0f) {
                            return string::f("%.2f", module->randomizeGlideTime);
                        } else if (module->randomizeGlideTime < 10.0f) {
                            return string::f("%.1f", module->randomizeGlideTime);
                        } else {
                            return string::f("%.0f", module->randomizeGlideTime);
                        }
                    }
                    return "1.00";
                }
            };

            GlideTimeSlider(NIGOQ* module) {
                box.size.x = 200.0f;
                quantity = new GlideTimeQuantity(module);
            }

            ~GlideTimeSlider() {
                delete quantity;
            }
        };

        GlideTimeSlider* glideSlider = new GlideTimeSlider(module);
        menu->addChild(glideSlider);

        menu->addChild(new MenuSeparator);

        // Random Amount Slider
        menu->addChild(createMenuLabel("Random Amount"));

        struct RandomAmountSlider : ui::Slider {
            struct RandomAmountQuantity : Quantity {
                NIGOQ* module;
                RandomAmountQuantity(NIGOQ* module) : module(module) {}

                void setValue(float value) override {
                    if (module) {
                        module->randomAmount = clamp(value, 0.0f, 1.0f);
                    }
                }

                float getValue() override {
                    if (module) {
                        return module->randomAmount;
                    }
                    return 1.0f;
                }

                float getMinValue() override { return 0.0f; }
                float getMaxValue() override { return 1.0f; }
                float getDefaultValue() override { return 1.0f; }
                std::string getLabel() override { return "Amount"; }
                std::string getUnit() override { return "%"; }
                std::string getDisplayValueString() override {
                    if (module) {
                        return string::f("%.0f", module->randomAmount * 100.0f);
                    }
                    return "100";
                }
            };

            RandomAmountSlider(NIGOQ* module) {
                box.size.x = 200.0f;
                quantity = new RandomAmountQuantity(module);
            }

            ~RandomAmountSlider() {
                delete quantity;
            }
        };

        RandomAmountSlider* amountSlider = new RandomAmountSlider(module);
        menu->addChild(amountSlider);

        menu->addChild(new MenuSeparator);

        // Random exclusion settings
        menu->addChild(createMenuLabel("Randomization Exclusions"));

        struct FinalFreqExcludeItem : MenuItem {
            NIGOQ* module;
            void onAction(const event::Action& e) override {
                module->excludeFinalFreqFromRandom = !module->excludeFinalFreqFromRandom;
            }
        };

        FinalFreqExcludeItem* finalFreqItem = createMenuItem<FinalFreqExcludeItem>(
            "Final Frequency affected by Random",
            CHECKMARK(!module->excludeFinalFreqFromRandom)
        );
        finalFreqItem->module = module;
        menu->addChild(finalFreqItem);

        struct DecayExcludeItem : MenuItem {
            NIGOQ* module;
            void onAction(const event::Action& e) override {
                module->excludeDecayFromRandom = !module->excludeDecayFromRandom;
            }
        };

        DecayExcludeItem* decayItem = createMenuItem<DecayExcludeItem>(
            "Decay affected by Random",
            CHECKMARK(!module->excludeDecayFromRandom)
        );
        decayItem->module = module;
        menu->addChild(decayItem);
    }
};

Model* modelNIGOQ = createModel<NIGOQ, NIGOQWidget>("NIGOQ");
