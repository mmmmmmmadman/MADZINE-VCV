#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

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
        
        if (bold) {
            float offset = 0.3f;
            nvgText(args.vg, box.size.x / 2.f - offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f + offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f - offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f + offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        } else {
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

// StandardBlackKnob 現在從 widgets/Knobs.hpp 引入
struct UFOWidget : Widget {
    UFOWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        float centerX = box.size.x / 2.0f;
        float centerY = box.size.y / 2.0f;
        
        nvgSave(args.vg);
        nvgTranslate(args.vg, centerX, centerY);
        nvgRotate(args.vg, 15.0f * M_PI / 180.0f);
        nvgTranslate(args.vg, -centerX, -centerY);
        
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        
        nvgBeginPath(args.vg);
        nvgEllipse(args.vg, centerX, centerY, 12.0f, 4.0f);
        nvgStrokeWidth(args.vg, 0.8f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgEllipse(args.vg, centerX, centerY - 2.0f, 6.0f, 3.0f);
        nvgStrokeWidth(args.vg, 0.6f);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 1.0f);
        for (int i = 0; i < 5; ++i) {
            float angle = i * 2.0f * M_PI / 5.0f;
            float lightX = centerX + 8.0f * cosf(angle);
            float lightY = centerY + 2.0f * sinf(angle);
            
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, lightX, lightY, 1.0f);
            nvgStroke(args.vg);
        }
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX - 8.0f, centerY + 4.0f);
        nvgLineTo(args.vg, centerX - 12.0f, centerY + 12.0f);
        nvgLineTo(args.vg, centerX + 12.0f, centerY + 12.0f);
        nvgLineTo(args.vg, centerX + 8.0f, centerY + 4.0f);
        nvgClosePath(args.vg);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);
        
        nvgRestore(args.vg);
    }
};

struct FluteWidget : Widget {
    FluteWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        float centerX = box.size.x / 2.0f;
        float centerY = box.size.y / 2.0f;
        
        nvgSave(args.vg);
        nvgTranslate(args.vg, centerX, centerY);
        nvgRotate(args.vg, -15.0f * M_PI / 180.0f);
        nvgTranslate(args.vg, -centerX, -centerY);
        
        nvgStrokeWidth(args.vg, 0.8f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 15, centerY - 1.5f, 30, 3);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 18, centerY - 1, 3, 2);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 0.5f);
        
        float holePositions[] = {-10, -6, -2, 2, 6, 10};
        for (int i = 0; i < 6; ++i) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, centerX + holePositions[i], centerY, 0.8f);
            nvgStroke(args.vg);
        }
        
        nvgStrokeWidth(args.vg, 0.4f);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 7, centerY - 3, 2, 1.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX + 1, centerY - 3, 2, 1.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX + 9, centerY - 3, 2, 1.5f);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 0.3f);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX - 6, centerY - 2.25f);
        nvgLineTo(args.vg, centerX - 6, centerY - 1.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX + 2, centerY - 2.25f);
        nvgLineTo(args.vg, centerX + 2, centerY - 1.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX + 10, centerY - 2.25f);
        nvgLineTo(args.vg, centerX + 10, centerY - 1.5f);
        nvgStroke(args.vg);
        
        nvgRestore(args.vg);
    }
};

struct HouseWidget : Widget {
    HouseWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        float centerX = box.size.x / 2.0f;
        float centerY = box.size.y / 2.0f;
        
        nvgSave(args.vg);
        nvgTranslate(args.vg, centerX, centerY);
        nvgRotate(args.vg, -10.0f * M_PI / 180.0f);
        nvgTranslate(args.vg, -centerX, -centerY);
        
        nvgStrokeWidth(args.vg, 0.8f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 8, centerY, 16, 10);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX - 10, centerY);
        nvgLineTo(args.vg, centerX, centerY - 8);
        nvgLineTo(args.vg, centerX + 10, centerY);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 0.6f);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 2, centerY + 4, 4, 6);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, centerX + 1, centerY + 7, 0.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX - 6, centerY + 2, 2.5f, 2.5f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX + 3.5f, centerY + 2, 2.5f, 2.5f);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 0.4f);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX - 4.75f, centerY + 2);
        nvgLineTo(args.vg, centerX - 4.75f, centerY + 4.5f);
        nvgStroke(args.vg);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX - 6, centerY + 3.25f);
        nvgLineTo(args.vg, centerX - 3.5f, centerY + 3.25f);
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX + 4.75f, centerY + 2);
        nvgLineTo(args.vg, centerX + 4.75f, centerY + 4.5f);
        nvgStroke(args.vg);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, centerX + 3.5f, centerY + 3.25f);
        nvgLineTo(args.vg, centerX + 6, centerY + 3.25f);
        nvgStroke(args.vg);
        
        nvgStrokeWidth(args.vg, 0.6f);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, centerX + 6, centerY - 6, 2, 4);
        nvgStroke(args.vg);
        
        nvgRestore(args.vg);
    }
};

struct ADGenerator : Module {
    int panelTheme = -1; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        ATK_ALL_PARAM,
        DEC_ALL_PARAM,
        AUTO_ROUTE_PARAM,
        TRACK1_ATTACK_PARAM,
        TRACK1_DECAY_PARAM,
        TRACK1_CURVE_PARAM,
        TRACK1_BPF_ENABLE_PARAM,
        TRACK1_BPF_FREQ_PARAM,
        TRACK1_BPF_GAIN_PARAM,
        TRACK2_ATTACK_PARAM,
        TRACK2_DECAY_PARAM,
        TRACK2_CURVE_PARAM,
        TRACK2_BPF_ENABLE_PARAM,
        TRACK2_BPF_FREQ_PARAM,
        TRACK2_BPF_GAIN_PARAM,
        TRACK3_ATTACK_PARAM,
        TRACK3_DECAY_PARAM,
        TRACK3_CURVE_PARAM,
        TRACK3_BPF_ENABLE_PARAM,
        TRACK3_BPF_FREQ_PARAM,
        TRACK3_BPF_GAIN_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        TRACK1_TRIG_INPUT,
        TRACK2_TRIG_INPUT,
        TRACK3_TRIG_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        TRACK1_OUTPUT,
        TRACK2_OUTPUT,
        TRACK3_OUTPUT,
        SUM_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        AUTO_ROUTE_LIGHT,
        TRACK1_BPF_LIGHT,
        TRACK2_BPF_LIGHT,
        TRACK3_BPF_LIGHT,
        LIGHTS_LEN
    };

    bool autoRouteEnabled = false;
    bool bpfEnabled[3] = {false, false, false};
    float bpfCutoffs[3] = {200.0f, 1000.0f, 5000.0f};
    float bpfGains[3] = {3.0f, 3.0f, 3.0f};
    
    struct BandPassFilter {
        float lowpass1 = 0.0f, highpass1 = 0.0f, bandpass1 = 0.0f;
        float lowpass2 = 0.0f, highpass2 = 0.0f, bandpass2 = 0.0f;
        float lowpass3 = 0.0f, highpass3 = 0.0f, bandpass3 = 0.0f;
        float lowpass4 = 0.0f, highpass4 = 0.0f, bandpass4 = 0.0f;
        
        void reset() {
            lowpass1 = highpass1 = bandpass1 = 0.0f;
            lowpass2 = highpass2 = bandpass2 = 0.0f;
            lowpass3 = highpass3 = bandpass3 = 0.0f;
            lowpass4 = highpass4 = bandpass4 = 0.0f;
        }
        
        float process(float input, float cutoff, float sampleRate) {
            float f = 2.0f * std::sin(M_PI * cutoff / sampleRate);
            f = clamp(f, 0.0f, 1.0f);
            
            lowpass1 += f * (input - lowpass1);
            highpass1 = input - lowpass1;
            bandpass1 += f * (highpass1 - bandpass1);
            
            lowpass2 += f * (bandpass1 - lowpass2);
            highpass2 = bandpass1 - lowpass2;
            bandpass2 += f * (highpass2 - bandpass2);
            
            lowpass3 += f * (bandpass2 - lowpass3);
            highpass3 = bandpass2 - lowpass3;
            bandpass3 += f * (highpass3 - bandpass3);
            
            lowpass4 += f * (bandpass3 - lowpass4);
            highpass4 = bandpass3 - lowpass4;
            bandpass4 += f * (highpass4 - bandpass4);
            
            return bandpass4;
        }
    };
    
    BandPassFilter bpfFilters[3];

    struct ADEnvelope {
        enum Phase {
            IDLE,
            ATTACK,
            DECAY
        };
        
        Phase phase = IDLE;
        float triggerOutput = 0.0f;
        float followerOutput = 0.0f;
        float attackTime = 0.01f;
        float decayTime = 1.0f;
        float phaseTime = 0.0f;
        float curve = 0.0f;
        float followerState = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        dsp::SchmittTrigger trigger;
        
        Phase oldPhase = IDLE;
        float oldOutput = 0.0f;
        float oldAttackTime = 0.01f;
        float oldDecayTime = 1.0f;
        float oldPhaseTime = 0.0f;
        float oldCurve = 0.0f;
        dsp::SchmittTrigger oldTrigger;
        
        void reset() {
            phase = IDLE;
            triggerOutput = 0.0f;
            followerOutput = 0.0f;
            followerState = 0.0f;
            phaseTime = 0.0f;
            oldPhase = IDLE;
            oldOutput = 0.0f;
            oldPhaseTime = 0.0f;
        }
        
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
        
        float processEnvelopeFollower(float triggerVoltage, float sampleTime, float attackTime, float releaseTime, float curve) {
            attackCoeff = 1.0f - std::exp(-sampleTime / std::max(0.0005f, attackTime * 0.1f));
            releaseCoeff = 1.0f - std::exp(-sampleTime / std::max(0.001f, releaseTime * 0.5f));
            
            attackCoeff = clamp(attackCoeff, 0.0f, 1.0f);
            releaseCoeff = clamp(releaseCoeff, 0.0f, 1.0f);
            
            float rectified = std::abs(triggerVoltage) / 10.0f;
            rectified = clamp(rectified, 0.0f, 1.0f);
            
            float targetCoeff;
            if (rectified > followerState) {
                float progress = attackCoeff;
                targetCoeff = applyCurve(progress, curve);
            } else {
                float progress = releaseCoeff;
                targetCoeff = applyCurve(progress, curve);
            }
            
            targetCoeff = clamp(targetCoeff, 0.0f, 1.0f);
            
            followerState += (rectified - followerState) * targetCoeff;
            followerState = clamp(followerState, 0.0f, 1.0f);
            
            return followerState;
        }
        
        float processTriggerEnvelope(float triggerVoltage, float sampleTime, float attack, float decay, float curve) {
            bool isHighVoltage = (std::abs(triggerVoltage) > 9.5f);
            
            if (phase == IDLE && isHighVoltage && trigger.process(triggerVoltage)) {
                phase = ATTACK;
                phaseTime = 0.0f;
            }
            
            switch (phase) {
                case IDLE:
                    triggerOutput = 0.0f;
                    break;
                    
                case ATTACK:
                    phaseTime += sampleTime;
                    if (phaseTime >= attack) {
                        phase = DECAY;
                        phaseTime = 0.0f;
                        triggerOutput = 1.0f;
                    } else {
                        float t = phaseTime / attack;
                        triggerOutput = applyCurve(t, curve);
                    }
                    break;
                    
                case DECAY:
                    phaseTime += sampleTime;
                    if (phaseTime >= decay) {
                        triggerOutput = 0.0f;
                        phase = IDLE;
                        phaseTime = 0.0f;
                    } else {
                        float t = phaseTime / decay;
                        triggerOutput = 1.0f - applyCurve(t, curve);
                    }
                    break;
            }
            
            return clamp(triggerOutput, 0.0f, 1.0f);
        }
        
        float processOldVersion(float sampleTime, float triggerVoltage, float attack, float decay, float curveParam, float atkAll, float decAll) {
            float atkOffset = atkAll * 0.5f;
            float decOffset = decAll * 0.5f;
            
            oldAttackTime = std::pow(10.0f, (attack - 0.5f) * 6.0f) + atkOffset;
            oldDecayTime = std::pow(10.0f, (decay - 0.5f) * 6.0f) + decOffset;
            
            oldAttackTime = std::max(0.001f, oldAttackTime);
            oldDecayTime = std::max(0.001f, oldDecayTime);
            
            oldCurve = curveParam;
            
            if (oldPhase == IDLE && oldTrigger.process(triggerVoltage)) {
                oldPhase = ATTACK;
                oldPhaseTime = 0.0f;
            }
            
            switch (oldPhase) {
                case IDLE:
                    oldOutput = 0.0f;
                    break;
                    
                case ATTACK:
                    oldPhaseTime += sampleTime;
                    if (oldPhaseTime >= oldAttackTime) {
                        oldPhase = DECAY;
                        oldPhaseTime = 0.0f;
                        oldOutput = 1.0f;
                    } else {
                        float t = oldPhaseTime / oldAttackTime;
                        oldOutput = applyCurve(t, oldCurve);
                    }
                    break;
                    
                case DECAY:
                    oldPhaseTime += sampleTime;
                    if (oldPhaseTime >= oldDecayTime) {
                        oldOutput = 0.0f;
                        oldPhase = IDLE;
                        oldPhaseTime = 0.0f;
                    } else {
                        float t = oldPhaseTime / oldDecayTime;
                        oldOutput = 1.0f - applyCurve(t, oldCurve);
                    }
                    break;
            }
            
            oldOutput = clamp(oldOutput, 0.0f, 1.0f);
            return oldOutput * 10.0f;
        }
        
        float process(float sampleTime, float triggerVoltage, float attack, float decay, float curveParam, float atkAll, float decAll, bool useBPF) {
            if (!useBPF) {
                return processOldVersion(sampleTime, triggerVoltage, attack, decay, curveParam, atkAll, decAll);
            } else {
                float atkOffset = atkAll * 0.5f;
                float decOffset = decAll * 0.5f;
                
                attackTime = std::pow(10.0f, (attack - 0.5f) * 6.0f) + atkOffset;
                decayTime = std::pow(10.0f, (decay - 0.5f) * 6.0f) + decOffset;
                
                attackTime = std::max(0.001f, attackTime);
                decayTime = std::max(0.001f, decayTime);
                
                curve = curveParam;
                
                float triggerEnv = processTriggerEnvelope(triggerVoltage, sampleTime, attackTime, decayTime, curve);
                float followerEnv = processEnvelopeFollower(triggerVoltage, sampleTime, attackTime, decayTime, curve);
                
                float output = std::max(triggerEnv, followerEnv);
                
                return output * 10.0f;
            }
        }
    };
    
    ADEnvelope envelopes[3];

    ADGenerator() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(ATK_ALL_PARAM, -1.0f, 1.0f, 0.0f, "Attack All");
        configParam(DEC_ALL_PARAM, -1.0f, 1.0f, 0.0f, "Decay All");
        configParam(AUTO_ROUTE_PARAM, 0.0f, 1.0f, 1.0f, "Auto Route");

        // Track 1 預設值來自 .vcvm
        configParam(TRACK1_ATTACK_PARAM, 0.0f, 1.0f, 0.0020000000949949026f, "Track 1 Attack", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK1_DECAY_PARAM, 0.0f, 1.0f, 0.30000001192092896f, "Track 1 Decay", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK1_CURVE_PARAM, -0.99f, 0.99f, -0.74844002723693848f, "Track 1 Curve");
        configParam(TRACK1_BPF_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "Track 1 BPF Enable");
        configParam(TRACK1_BPF_FREQ_PARAM, 20.0f, 8000.0f, 200.0f, "Track 1 BPF Frequency", " Hz");
        configParam(TRACK1_BPF_GAIN_PARAM, 0.1f, 100.0f, 3.0f, "Track 1 BPF Gain", "x");

        // Track 2 預設值來自 .vcvm
        configParam(TRACK2_ATTACK_PARAM, 0.0f, 1.0f, 0.0f, "Track 2 Attack", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK2_DECAY_PARAM, 0.0f, 1.0f, 0.30000001192092896f, "Track 2 Decay", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK2_CURVE_PARAM, -0.99f, 0.99f, -0.8316001296043396f, "Track 2 Curve");
        configParam(TRACK2_BPF_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "Track 2 BPF Enable");
        configParam(TRACK2_BPF_FREQ_PARAM, 20.0f, 8000.0f, 1000.0f, "Track 2 BPF Frequency", " Hz");
        configParam(TRACK2_BPF_GAIN_PARAM, 0.1f, 100.0f, 3.0f, "Track 2 BPF Gain", "x");

        // Track 3 預設值來自 .vcvm
        configParam(TRACK3_ATTACK_PARAM, 0.0f, 1.0f, 0.0f, "Track 3 Attack", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK3_DECAY_PARAM, 0.0f, 1.0f, 0.30000001192092896f, "Track 3 Decay", " s", 0.0f, 1.0f, std::pow(10.0f, -2.0f));
        configParam(TRACK3_CURVE_PARAM, -0.99f, 0.99f, -0.73062008619308472f, "Track 3 Curve");
        configParam(TRACK3_BPF_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "Track 3 BPF Enable");
        configParam(TRACK3_BPF_FREQ_PARAM, 20.0f, 8000.0f, 5000.0f, "Track 3 BPF Frequency", " Hz");
        configParam(TRACK3_BPF_GAIN_PARAM, 0.1f, 100.0f, 3.0f, "Track 3 BPF Gain", "x");

        for (int i = 0; i < 3; ++i) {
            configInput(TRACK1_TRIG_INPUT + i, string::f("Track %d Trigger", i + 1));
            configOutput(TRACK1_OUTPUT + i, string::f("Track %d Envelope", i + 1));
        }
        
        configOutput(SUM_OUTPUT, "Sum");
        configLight(AUTO_ROUTE_LIGHT, "Auto Route Light");
        for (int i = 0; i < 3; ++i) {
            configLight(TRACK1_BPF_LIGHT + i, string::f("Track %d BPF Light", i + 1));
        }
    }

    void onReset() override {
        for (int i = 0; i < 3; ++i) {
            envelopes[i].reset();
            bpfFilters[i].reset();
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "autoRouteEnabled", json_boolean(autoRouteEnabled));
        
        json_t* bpfEnabledJ = json_array();
        for (int i = 0; i < 3; ++i) {
            json_array_append_new(bpfEnabledJ, json_boolean(bpfEnabled[i]));
        }
        json_object_set_new(rootJ, "bpfEnabled", bpfEnabledJ);
        
        json_t* bpfCutoffsJ = json_array();
        for (int i = 0; i < 3; ++i) {
            json_array_append_new(bpfCutoffsJ, json_real(bpfCutoffs[i]));
        }
        json_object_set_new(rootJ, "bpfCutoffs", bpfCutoffsJ);
        
        json_t* bpfGainsJ = json_array();
        for (int i = 0; i < 3; ++i) {
            json_array_append_new(bpfGainsJ, json_real(bpfGains[i]));
        }
        json_object_set_new(rootJ, "bpfGains", bpfGainsJ);
        
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }

        json_t* autoRouteJ = json_object_get(rootJ, "autoRouteEnabled");
        if (autoRouteJ) {
            autoRouteEnabled = json_boolean_value(autoRouteJ);
        }
        
        json_t* bpfEnabledJ = json_object_get(rootJ, "bpfEnabled");
        if (bpfEnabledJ) {
            for (int i = 0; i < 3; ++i) {
                json_t* enabledJ = json_array_get(bpfEnabledJ, i);
                if (enabledJ) {
                    bpfEnabled[i] = json_boolean_value(enabledJ);
                }
            }
        }
        
        json_t* bpfCutoffsJ = json_object_get(rootJ, "bpfCutoffs");
        if (bpfCutoffsJ) {
            for (int i = 0; i < 3; ++i) {
                json_t* cutoffJ = json_array_get(bpfCutoffsJ, i);
                if (cutoffJ) {
                    bpfCutoffs[i] = json_real_value(cutoffJ);
                }
            }
        }
        
        json_t* bpfGainsJ = json_object_get(rootJ, "bpfGains");
        if (bpfGainsJ) {
            for (int i = 0; i < 3; ++i) {
                json_t* gainJ = json_array_get(bpfGainsJ, i);
                if (gainJ) {
                    bpfGains[i] = json_real_value(gainJ);
                }
            }
        }
    }

    void process(const ProcessArgs& args) override {
        float sumOutput = 0.0f;
        float atkAll = params[ATK_ALL_PARAM].getValue();
        float decAll = params[DEC_ALL_PARAM].getValue();
        
        autoRouteEnabled = params[AUTO_ROUTE_PARAM].getValue() > 0.5f;
        
        for (int i = 0; i < 3; ++i) {
            bpfEnabled[i] = params[TRACK1_BPF_ENABLE_PARAM + i * 6].getValue() > 0.5f;
            bpfCutoffs[i] = params[TRACK1_BPF_FREQ_PARAM + i * 6].getValue();
            bpfGains[i] = params[TRACK1_BPF_GAIN_PARAM + i * 6].getValue();
        }
        
        float inputSignals[3];
        
        if (autoRouteEnabled) {
            float input1Signal = inputs[TRACK1_TRIG_INPUT].getVoltage();
            inputSignals[0] = input1Signal;
            inputSignals[1] = input1Signal;
            inputSignals[2] = input1Signal;
        } else {
            inputSignals[0] = inputs[TRACK1_TRIG_INPUT].getVoltage();
            inputSignals[1] = inputs[TRACK2_TRIG_INPUT].getVoltage();
            inputSignals[2] = inputs[TRACK3_TRIG_INPUT].getVoltage();
        }
        
        for (int i = 0; i < 3; ++i) {
            float processedSignal = inputSignals[i];
            if (bpfEnabled[i]) {
                processedSignal = bpfFilters[i].process(inputSignals[i], bpfCutoffs[i], args.sampleRate);
            }
            
            float attackParam = params[TRACK1_ATTACK_PARAM + i * 6].getValue();
            float decayParam = params[TRACK1_DECAY_PARAM + i * 6].getValue();
            float curveParam = params[TRACK1_CURVE_PARAM + i * 6].getValue();
            
            float envelopeOutput = envelopes[i].process(args.sampleTime, processedSignal, attackParam, decayParam, curveParam, atkAll, decAll, bpfEnabled[i]);
            
            if (bpfEnabled[i]) {
                envelopeOutput *= bpfGains[i];
            }
            
            outputs[TRACK1_OUTPUT + i].setVoltage(envelopeOutput);
            
            sumOutput += envelopeOutput * 0.33f;
        }
        
        sumOutput = clamp(sumOutput, 0.0f, 10.0f);
        outputs[SUM_OUTPUT].setVoltage(sumOutput);
        
        lights[AUTO_ROUTE_LIGHT].setBrightness(autoRouteEnabled ? 1.0f : 0.0f);
        for (int i = 0; i < 3; ++i) {
            lights[TRACK1_BPF_LIGHT + i].setBrightness(bpfEnabled[i] ? 1.0f : 0.0f);
        }
    }
};

struct ADGeneratorWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    ADGeneratorWidget(ADGenerator* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP");
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "ADGenerator", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new UFOWidget(Vec(80, 285), Vec(40, 25)));
        addChild(new FluteWidget(Vec(78, 125), Vec(40, 25)));
        addChild(new HouseWidget(Vec(80, 205), Vec(40, 25)));

        addChild(new EnhancedTextLabel(Vec(15, 30), Vec(30, 15), "ATK ALL", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<Trimpot>(Vec(30, 50), module, ADGenerator::ATK_ALL_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(50, 30), Vec(30, 15), "DEC ALL", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<Trimpot>(Vec(65, 50), module, ADGenerator::DEC_ALL_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(83, 30), Vec(30, 15), "ROUTE", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<VCVLatch>(Vec(98, 50), module, ADGenerator::AUTO_ROUTE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(98, 65), module, ADGenerator::AUTO_ROUTE_LIGHT));

        float trackY[3] = {95, 185, 275};
        
        for (int i = 0; i < 3; ++i) {
            float y = trackY[i];
            float x = 10;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "IN", 7.f, nvgRGB(255, 255, 255), true));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_TRIG_INPUT + i));
            x += 27;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "ATK", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_ATTACK_PARAM + i * 6));
            x += 27;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "DEC", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_DECAY_PARAM + i * 6));
            x += 27;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "CURV", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_CURVE_PARAM + i * 6));
            x += 27;

            x = 10;
            y += 35;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "Follower", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<VCVLatch>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_BPF_ENABLE_PARAM + i * 6));
            addChild(createLightCentered<MediumLight<BlueLight>>(Vec(x + 7, y + 12), module, ADGenerator::TRACK1_BPF_LIGHT + i));
            x += 27;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "FREQ", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_BPF_FREQ_PARAM + i * 6));
            x += 27;

            addChild(new EnhancedTextLabel(Vec(x - 5, y - 25), Vec(25, 10), "GAIN", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 7, y - 3), module, ADGenerator::TRACK1_BPF_GAIN_PARAM + i * 6));
        }
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 325)));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(13, 358), module, ADGenerator::TRACK1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 358), module, ADGenerator::TRACK2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(75, 358), module, ADGenerator::TRACK3_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(104, 358), module, ADGenerator::SUM_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(7, 337), Vec(12, 10), "1", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(38, 337), Vec(12, 10), "2", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(69, 337), Vec(12, 10), "3", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(96, 337), Vec(16, 10), "MIYA", 7.f, nvgRGB(255, 133, 133), true));
    }

    void step() override {
        ADGenerator* module = dynamic_cast<ADGenerator*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        ADGenerator* module = dynamic_cast<ADGenerator*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelADGenerator = createModel<ADGenerator, ADGeneratorWidget>("ADGenerator");