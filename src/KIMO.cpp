#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

struct TechnoEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    TechnoEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f, 
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

// TechnoStandardBlackKnob 現在從 widgets/Knobs.hpp 引入

// TechnoSnapKnob 現在從 widgets/Knobs.hpp 引入

struct KimoAccentParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        return string::f("%d step", value);
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

static std::vector<bool> generateTechnoEuclideanRhythm(int length, int fill, int shift) {
    std::vector<bool> pattern(length, false);
    if (fill == 0 || length == 0) return pattern;
    if (fill > length) fill = length;

    shift = shift % length;
    if (shift < 0) shift += length;

    for (int i = 0; i < fill; ++i) {
        int index = (int)std::floor((float)i * length / fill);
        pattern[index] = true;
    }
    
    std::rotate(pattern.begin(), pattern.begin() + shift, pattern.end());
    return pattern;
}

struct UnifiedEnvelope {
    dsp::SchmittTrigger trigTrigger;
    dsp::PulseGenerator trigPulse;
    float phase = 0.f;
    bool gateState = false;
    static constexpr float ATTACK_TIME = 0.001f;
    
    void reset() {
        trigTrigger.reset();
        trigPulse.reset();
        phase = 0.f;
        gateState = false;
    }
    
    float smoothDecayEnvelope(float t, float totalTime, float shapeParam) {
        if (t >= totalTime) return 0.f;
        
        float normalizedT = t / totalTime;
        
        float frontK = -0.9f + shapeParam * 0.5f;
        float backK = -1.0f + 1.6f * std::pow(shapeParam, 0.3f);
        
        float transition = normalizedT * normalizedT * (3.f - 2.f * normalizedT);
        float k = frontK + (backK - frontK) * transition;
        
        float absT = std::abs(normalizedT);
        float denominator = k - 2.f * k * absT + 1.f;
        if (std::abs(denominator) < 1e-10f) {
            return 1.f - normalizedT;
        }
        
        float curveResult = (normalizedT - k * normalizedT) / denominator;
        return 1.f - curveResult;
    }
    
    float process(float sampleTime, float triggerVoltage, float decayTime, float shapeParam) {
        bool triggered = trigTrigger.process(triggerVoltage, 0.1f, 2.f);
        
        if (triggered) {
            phase = 0.f;
            gateState = true;
            trigPulse.trigger(0.03f);
        }
        
        float envOutput = 0.f;
        
        if (gateState) {
            if (phase < ATTACK_TIME) {
                envOutput = phase / ATTACK_TIME;
            } else {
                float decayPhase = phase - ATTACK_TIME;
                
                if (decayPhase >= decayTime) {
                    gateState = false;
                    envOutput = 0.f;
                } else {
                    envOutput = smoothDecayEnvelope(decayPhase, decayTime, shapeParam);
                }
            }
            
            phase += sampleTime;
        }
        
        return clamp(envOutput, 0.f, 1.f);
    }
    
    float getTrigger(float sampleTime) {
        return trigPulse.process(sampleTime) ? 10.0f : 0.0f;
    }
};

struct LinearEnvelope {
    dsp::SchmittTrigger trigTrigger;
    dsp::PulseGenerator trigPulse;
    float phase = 0.f;
    bool gateState = false;
    static constexpr float ATTACK_TIME = 0.001f;
    
    void reset() {
        trigTrigger.reset();
        trigPulse.reset();
        phase = 0.f;
        gateState = false;
    }
    
    float process(float sampleTime, float triggerVoltage, float decayTime) {
        bool triggered = trigTrigger.process(triggerVoltage, 0.1f, 2.f);
        
        if (triggered) {
            phase = 0.f;
            gateState = true;
            trigPulse.trigger(0.03f);
        }
        
        float envOutput = 0.f;
        
        if (gateState) {
            if (phase < ATTACK_TIME) {
                envOutput = phase / ATTACK_TIME;
            } else {
                float decayPhase = phase - ATTACK_TIME;
                
                if (decayPhase >= decayTime) {
                    gateState = false;
                    envOutput = 0.f;
                } else {
                    envOutput = 1.0f - (decayPhase / decayTime);
                }
            }
            
            phase += sampleTime;
        }
        
        return clamp(envOutput, 0.f, 1.f);
    }
    
    float getTrigger(float sampleTime) {
        return trigPulse.process(sampleTime) ? 10.0f : 0.0f;
    }
};

struct BasicSineVCO {
    float phase = 0.0f;
    float sampleRate = 44100.0f;
    
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    float process(float freq_hz, float fm_cv, float saturation = 1.0f) {
        float modulated_freq = freq_hz * std::pow(2.0f, fm_cv);
        modulated_freq = clamp(modulated_freq, 1.0f, sampleRate * 0.45f);
        
        float delta_phase = modulated_freq / sampleRate;
        
        phase += delta_phase;
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }
        
        float sine_wave = std::sin(2.0f * M_PI * phase);
        
        if (saturation > 1.0f) {
            sine_wave = std::tanh(sine_wave * saturation) / std::tanh(saturation);
        }
        
        return sine_wave * 5.0f;
    }
};

struct KIMO : Module {
    int panelTheme = -1; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        FILL_PARAM,
        ACCENT_PARAM,
        ACCENT_DELAY_PARAM,
        TUNE_PARAM,
        FM_PARAM,
        PUNCH_PARAM,
        DECAY_PARAM,
        SHAPE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CLK_INPUT,
        TUNE_CV_INPUT,
        FM_CV_INPUT,
        PUNCH_CV_INPUT,
        DECAY_CV_INPUT,
        FILL_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        VCA_ENV_OUTPUT,
        FM_ENV_OUTPUT,
        ACCENT_ENV_OUTPUT,
        AUDIO_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;

    // CV 調變顯示用
    float fillCvMod = 0.0f;
    float tuneCvMod = 0.0f;
    float fmCvMod = 0.0f;
    float punchCvMod = 0.0f;
    float decayCvMod = 0.0f;

    float globalClockSeconds = 0.5f;
    float secondsSinceLastClock = -1.0f;
    static constexpr int GLOBAL_LENGTH = 16;
    
    BasicSineVCO kickVCO;
    
    struct QuarterNoteClock {
        int currentStep = 0;
        int shiftAmount = 1;
        dsp::PulseGenerator trigPulse;
        
        void reset() {
            currentStep = 0;
        }
        
        bool processStep(bool globalClockTriggered, int shift) {
            shiftAmount = shift;
            if (globalClockTriggered) {
                currentStep = (currentStep + 1) % 4;
                
                int targetStep = shiftAmount % 4;
                if (currentStep == targetStep) {
                    trigPulse.trigger(0.01f);
                    return true;
                }
            }
            return false;
        }
        
        float getTrigger(float sampleTime) {
            return trigPulse.process(sampleTime) ? 10.0f : 0.0f;
        }
    };

    struct TrackState {
        int currentStep = 0;
        int length = GLOBAL_LENGTH;
        int fill = 4;
        int shift = 0;
        std::vector<bool> pattern;
        bool gateState = false;
        dsp::PulseGenerator trigPulse;
        
        UnifiedEnvelope envelope;
        LinearEnvelope vcaEnvelope;

        void reset() {
            currentStep = 0;
            pattern.clear();
            gateState = false;
            envelope.reset();
            vcaEnvelope.reset();
        }
        
        void stepTrack() {
            currentStep = (currentStep + 1) % length;
            gateState = !pattern.empty() && pattern[currentStep];
            if (gateState) {
                trigPulse.trigger(0.01f);
            }
        }
    };
    
    TrackState track;
    QuarterNoteClock quarterClock;
    UnifiedEnvelope accentVCA;

    KIMO() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(CLK_INPUT, "Clock");
        configInput(TUNE_CV_INPUT, "Tune CV");
        configInput(FM_CV_INPUT, "FM CV");
        configInput(PUNCH_CV_INPUT, "Punch CV");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(FILL_CV_INPUT, "Fill CV");
        
        configParam(FILL_PARAM, 0.0f, 100.0f, 71.20001220703125f, "Fill", "%");
        configParam(ACCENT_PARAM, 1.0f, 7.0f, 3.0f, "Accent");
        getParamQuantity(ACCENT_PARAM)->snapEnabled = true;
        delete paramQuantities[ACCENT_PARAM];
        paramQuantities[ACCENT_PARAM] = new KimoAccentParamQuantity;
        paramQuantities[ACCENT_PARAM]->module = this;
        paramQuantities[ACCENT_PARAM]->paramId = ACCENT_PARAM;
        paramQuantities[ACCENT_PARAM]->minValue = 1.0f;
        paramQuantities[ACCENT_PARAM]->maxValue = 7.0f;
        paramQuantities[ACCENT_PARAM]->defaultValue = 3.0f;
        paramQuantities[ACCENT_PARAM]->name = "Accent";
        paramQuantities[ACCENT_PARAM]->snapEnabled = true;
        
        configParam(ACCENT_DELAY_PARAM, 0.01f, 2.0f, 0.54331988096237183f, "Accent Delay", " s");
        configParam(TUNE_PARAM, std::log2(24.0f), std::log2(500.0f), 4.5849623680114746f, "Tune", " Hz", 2.f);
        configParam(FM_PARAM, 0.0f, 1.0f, 0.12400007992982864f, "FM Amount");
        configParam(PUNCH_PARAM, 0.0f, 1.0f, 0.67500001192092896f, "Punch Amount");
        configParam(DECAY_PARAM, std::log(0.01f), std::log(2.0f), -3.180246114730835f, "Decay", " s", 2.718281828f);
        configParam(SHAPE_PARAM, 0.0f, 0.99f, 0.11884991824626923f, "Shape");
        
        configOutput(VCA_ENV_OUTPUT, "VCA Envelope");
        configOutput(FM_ENV_OUTPUT, "FM Envelope");
        configOutput(ACCENT_ENV_OUTPUT, "Accent Envelope");
        configOutput(AUDIO_OUTPUT, "Audio");
        
        kickVCO.setSampleRate(44100.0f);
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        kickVCO.setSampleRate(sr);
    }

    void onReset() override {
        secondsSinceLastClock = -1.0f;
        globalClockSeconds = 0.5f;
        track.reset();
        quarterClock.reset();
        accentVCA.reset();
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
    }

    void process(const ProcessArgs& args) override {
        bool globalClockActive = inputs[CLK_INPUT].isConnected();
        bool globalClockTriggered = false;
        
        if (globalClockActive) {
            float clockVoltage = inputs[CLK_INPUT].getVoltage();
            globalClockTriggered = clockTrigger.process(clockVoltage);
        }
        
        if (globalClockTriggered) {
            if (secondsSinceLastClock > 0.0f) {
                globalClockSeconds = secondsSinceLastClock;
                globalClockSeconds = clamp(globalClockSeconds, 0.01f, 10.0f);
            }
            secondsSinceLastClock = 0.0f;
        }
        
        if (secondsSinceLastClock >= 0.0f) {
            secondsSinceLastClock += args.sampleTime;
        }

        int accentShift = (int)std::round(params[ACCENT_PARAM].getValue());
        bool accentTriggered = quarterClock.processStep(globalClockTriggered, accentShift);
        float accentTrigger = quarterClock.getTrigger(args.sampleTime);

        track.length = GLOBAL_LENGTH;

        float fillParam = params[FILL_PARAM].getValue();
        if (inputs[FILL_CV_INPUT].isConnected()) {
            float cv = inputs[FILL_CV_INPUT].getVoltage();
            fillParam += cv * 10.0f;
            fillCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            fillCvMod = 0.0f;
        }
        float fillPercentage = clamp(fillParam, 0.0f, 100.0f);
        track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

        track.shift = 0;

        track.pattern = generateTechnoEuclideanRhythm(track.length, track.fill, track.shift);

        if (globalClockTriggered && !track.pattern.empty() && globalClockActive) {
            track.stepTrack();
        }
        
        float decayParam = std::exp(params[DECAY_PARAM].getValue());
        if (inputs[DECAY_CV_INPUT].isConnected()) {
            float cv = inputs[DECAY_CV_INPUT].getVoltage();
            decayParam += cv / 10.0f;
            decayParam = clamp(decayParam, 0.01f, 2.0f);
            decayCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            decayCvMod = 0.0f;
        }
        float shapeParam = params[SHAPE_PARAM].getValue();
        
        float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
        float envelopeOutput = track.envelope.process(args.sampleTime, triggerOutput, decayParam, shapeParam);
        
        float fmAmount = params[FM_PARAM].getValue();
        if (inputs[FM_CV_INPUT].isConnected()) {
            float cv = inputs[FM_CV_INPUT].getVoltage();
            fmAmount += cv / 10.0f;
            fmAmount = clamp(fmAmount, 0.0f, 1.0f);
            fmCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            fmCvMod = 0.0f;
        }

        float freqParam = std::pow(2.0f, params[TUNE_PARAM].getValue());
        if (inputs[TUNE_CV_INPUT].isConnected()) {
            float cv = inputs[TUNE_CV_INPUT].getVoltage();
            float freqCV = params[TUNE_PARAM].getValue() + cv;
            freqParam = std::pow(2.0f, freqCV);
            freqParam = clamp(freqParam, std::pow(2.0f, std::log2(24.0f)), std::pow(2.0f, std::log2(500.0f)));
            tuneCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            tuneCvMod = 0.0f;
        }

        float punchAmount = params[PUNCH_PARAM].getValue();
        if (inputs[PUNCH_CV_INPUT].isConnected()) {
            float cv = inputs[PUNCH_CV_INPUT].getVoltage();
            punchAmount += cv / 10.0f;
            punchAmount = clamp(punchAmount, 0.0f, 1.0f);
            punchCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            punchCvMod = 0.0f;
        }
        
        float envelopeFM = envelopeOutput * fmAmount * 20.0f;
        float punchSaturation = 1.0f + (punchAmount * 2.0f);
        float audioOutput = kickVCO.process(freqParam, envelopeFM, punchSaturation);
        
        float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam);
        
        float accentDelayParam = params[ACCENT_DELAY_PARAM].getValue();
        float accentVCAOutput = accentVCA.process(args.sampleTime, accentTrigger, accentDelayParam, 0.5f);
        
        float finalAudioOutput = audioOutput * vcaEnvelopeOutput * accentVCAOutput * 1.8f;
        
        outputs[VCA_ENV_OUTPUT].setVoltage(vcaEnvelopeOutput * 10.0f);
        outputs[FM_ENV_OUTPUT].setVoltage(envelopeOutput * 10.0f);
        outputs[ACCENT_ENV_OUTPUT].setVoltage(accentVCAOutput * 10.0f);
        outputs[AUDIO_OUTPUT].setVoltage(finalAudioOutput);
    }
};

struct KIMOWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    TechnoStandardBlackKnob30* fillKnob = nullptr;
    TechnoStandardBlackKnob30* tuneKnob = nullptr;
    TechnoStandardBlackKnob30* fmKnob = nullptr;
    TechnoStandardBlackKnob30* punchKnob = nullptr;
    TechnoStandardBlackKnob30* decayKnob = nullptr;

    KIMOWidget(KIMO* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP");
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "KIMO", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // 在 KIMOWidget 建構函數中，找到所有座標設定並修改：

        // CLK 和 FILL (保持不變)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 38), Vec(20, 15), "CLK", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 63), module, KIMO::CLK_INPUT));
        addChild(new TechnoEnhancedTextLabel(Vec(35, 38), Vec(20, 15), "FILL", 6.f, nvgRGB(255, 255, 255), true));
        fillKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(45, 63), module, KIMO::FILL_PARAM);
        addParam(fillKnob);

        // ACCENT (78 -> 80)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 80), Vec(20, 15), "ACCENT", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(15, 105), module, KIMO::ACCENT_PARAM));

        // ACCENT DELAY (78 -> 80, 88 -> 90)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 80), Vec(20, 15), "DELAY", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(45, 105), module, KIMO::ACCENT_DELAY_PARAM));

        // TUNE (118 -> 122)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 122), Vec(20, 15), "TUNE", 6.f, nvgRGB(255, 255, 255), true));
        tuneKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(15, 147), module, KIMO::TUNE_PARAM);
        addParam(tuneKnob);

        // FM (118 -> 122)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 122), Vec(20, 15), "FM", 6.f, nvgRGB(255, 255, 255), true));
        fmKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(45, 147), module, KIMO::FM_PARAM);
        addParam(fmKnob);

        // PUNCH (158 -> 164)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 164), Vec(20, 15), "PUNCH", 5.f, nvgRGB(255, 255, 255), true));
        punchKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(15, 189), module, KIMO::PUNCH_PARAM);
        addParam(punchKnob);

        // DECAY (158 -> 164)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 164), Vec(20, 15), "DECAY", 5.f, nvgRGB(255, 255, 255), true));
        decayKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(45, 189), module, KIMO::DECAY_PARAM);
        addParam(decayKnob);

        // SHAPE (198 -> 206)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 206), Vec(20, 15), "SHAPE", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(15, 231), module, KIMO::SHAPE_PARAM));

        // FILL CV
        addChild(new TechnoEnhancedTextLabel(Vec(35, 206), Vec(20, 15), "FILL", 5.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 231), module, KIMO::FILL_CV_INPUT));

        // TUNE CV (178 -> 250)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 250), Vec(20, 15), "TUNE", 5.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 272), module, KIMO::TUNE_CV_INPUT));

        // FM CV (213 -> 250)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 250), Vec(20, 15), "FM", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 272), module, KIMO::FM_CV_INPUT));

        // PUNCH CV (213 -> 285)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 285), Vec(20, 15), "PUNCH", 5.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 308), module, KIMO::PUNCH_CV_INPUT));

        // DECAY CV (248 -> 285)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 285), Vec(20, 15), "DECAY", 5.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 308), module, KIMO::DECAY_CV_INPUT));

        // 輸出區塊 (保持原座標)
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        // VCA 輸出 (保持原座標)
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 343), module, KIMO::VCA_ENV_OUTPUT));

        // FM 輸出 (保持原座標)
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 343), module, KIMO::FM_ENV_OUTPUT));

        // ACCENT 輸出 (保持原座標)
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 368), module, KIMO::ACCENT_ENV_OUTPUT));

        // AUDIO 輸出 (保持原座標)
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 368), module, KIMO::AUDIO_OUTPUT));
    }

    void step() override {
        KIMO* module = dynamic_cast<KIMO*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // CV 調變顯示更新
            auto updateKnob = [&](TechnoStandardBlackKnob30* knob, int inputId, float cvMod) {
                if (knob) {
                    bool connected = module->inputs[inputId].isConnected();
                    knob->setModulationEnabled(connected);
                    if (connected) knob->setModulation(cvMod);
                }
            };

            updateKnob(fillKnob, KIMO::FILL_CV_INPUT, module->fillCvMod);
            updateKnob(tuneKnob, KIMO::TUNE_CV_INPUT, module->tuneCvMod);
            updateKnob(fmKnob, KIMO::FM_CV_INPUT, module->fmCvMod);
            updateKnob(punchKnob, KIMO::PUNCH_CV_INPUT, module->punchCvMod);
            updateKnob(decayKnob, KIMO::DECAY_CV_INPUT, module->decayCvMod);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        KIMO* module = dynamic_cast<KIMO*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelKIMO = createModel<KIMO, KIMOWidget>("KIMO");