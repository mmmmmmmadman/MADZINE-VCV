#include "plugin.hpp"
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

struct TechnoStandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    TechnoStandardBlackKnob() {
        box.size = Vec(30, 30);
    }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float normalizedValue = pq->getScaledValue();
        float angle = rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
        return angle;
    }
    
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, nvgRGB(50, 50, 50));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 8;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
    
    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = true;
            e.consume(this);
        }
        else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = false;
        }
        ParamWidget::onButton(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!isDragging || !pq) return;
        
        float sensitivity = 0.002f;
        float deltaY = -e.mouseDelta.y;
        
        float range = pq->getMaxValue() - pq->getMinValue();
        float currentValue = pq->getValue();
        float newValue = currentValue + deltaY * sensitivity * range;
        newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
        
        pq->setValue(newValue);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        pq->reset();
        e.consume(this);
    }
};

struct TechnoSnapKnob : ParamWidget {
    float accumDelta = 0.0f;
    
    TechnoSnapKnob() {
        box.size = Vec(30, 30);
    }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float normalizedValue = pq->getScaledValue();
        float angle = rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
        return angle;
    }
    
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, nvgRGB(50, 50, 50));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 8;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
    
    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            accumDelta = 0.0f;
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        accumDelta += (e.mouseDelta.x - e.mouseDelta.y);
        
        float threshold = 15.0f;
        
        if (accumDelta >= threshold) {
            float currentValue = pq->getValue();
            float newValue = currentValue + 1.0f;
            newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
            pq->setValue(newValue);
            accumDelta = 0.0f;
        }
        else if (accumDelta <= -threshold) {
            float currentValue = pq->getValue();
            float newValue = currentValue - 1.0f;
            newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
            pq->setValue(newValue);
            accumDelta = 0.0f;
        }
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        pq->reset();
        e.consume(this);
    }
};

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
        
        configParam(FILL_PARAM, 0.0f, 100.0f, 25.0f, "Fill", "%");
        configParam(ACCENT_PARAM, 1.0f, 7.0f, 1.0f, "Accent");
        getParamQuantity(ACCENT_PARAM)->snapEnabled = true;
        delete paramQuantities[ACCENT_PARAM];
        paramQuantities[ACCENT_PARAM] = new KimoAccentParamQuantity;
        paramQuantities[ACCENT_PARAM]->module = this;
        paramQuantities[ACCENT_PARAM]->paramId = ACCENT_PARAM;
        paramQuantities[ACCENT_PARAM]->minValue = 1.0f;
        paramQuantities[ACCENT_PARAM]->maxValue = 7.0f;
        paramQuantities[ACCENT_PARAM]->defaultValue = 1.0f;
        paramQuantities[ACCENT_PARAM]->name = "Accent";
        paramQuantities[ACCENT_PARAM]->snapEnabled = true;
        
        configParam(ACCENT_DELAY_PARAM, 0.01f, 2.0f, 0.3f, "Accent Delay", " s");
        configParam(TUNE_PARAM, std::log2(24.0f), std::log2(500.0f), std::log2(60.0f), "Tune", " Hz", 2.f);
        configParam(FM_PARAM, 0.0f, 1.0f, 0.5f, "FM Amount");
        configParam(PUNCH_PARAM, 0.0f, 1.0f, 0.5f, "Punch Amount");
        configParam(DECAY_PARAM, std::log(0.01f), std::log(2.0f), std::log(0.3f), "Decay", " s", 2.718281828f);
        configParam(SHAPE_PARAM, 0.0f, 0.99f, 0.5f, "Shape");
        
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
        float fillPercentage = clamp(fillParam, 0.0f, 100.0f);
        track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

        track.shift = 0;

        track.pattern = generateTechnoEuclideanRhythm(track.length, track.fill, track.shift);

        if (globalClockTriggered && !track.pattern.empty() && globalClockActive) {
            track.stepTrack();
        }
        
        float decayParam = std::exp(params[DECAY_PARAM].getValue());
        if (inputs[DECAY_CV_INPUT].isConnected()) {
            decayParam += inputs[DECAY_CV_INPUT].getVoltage() / 10.0f;
            decayParam = clamp(decayParam, 0.01f, 2.0f);
        }
        float shapeParam = params[SHAPE_PARAM].getValue();
        
        float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
        float envelopeOutput = track.envelope.process(args.sampleTime, triggerOutput, decayParam, shapeParam);
        
        float fmAmount = params[FM_PARAM].getValue();
        if (inputs[FM_CV_INPUT].isConnected()) {
            fmAmount += inputs[FM_CV_INPUT].getVoltage() / 10.0f;
            fmAmount = clamp(fmAmount, 0.0f, 1.0f);
        }
        
        float freqParam = std::pow(2.0f, params[TUNE_PARAM].getValue());
        if (inputs[TUNE_CV_INPUT].isConnected()) {
            float freqCV = params[TUNE_PARAM].getValue() + inputs[TUNE_CV_INPUT].getVoltage();
            freqParam = std::pow(2.0f, freqCV);
            freqParam = clamp(freqParam, std::pow(2.0f, std::log2(24.0f)), std::pow(2.0f, std::log2(500.0f)));
        }
        
        float punchAmount = params[PUNCH_PARAM].getValue();
        if (inputs[PUNCH_CV_INPUT].isConnected()) {
            punchAmount += inputs[PUNCH_CV_INPUT].getVoltage() / 10.0f;
            punchAmount = clamp(punchAmount, 0.0f, 1.0f);
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
    KIMOWidget(KIMO* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "KIMO", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // 在 KIMOWidget 建構函數中，找到所有座標設定並修改：

        // CLK 和 FILL (保持不變)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 38), Vec(20, 15), "CLK", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 63), module, KIMO::CLK_INPUT));
        addChild(new TechnoEnhancedTextLabel(Vec(35, 38), Vec(20, 15), "FILL", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, 63), module, KIMO::FILL_PARAM));

        // ACCENT (78 -> 80)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 80), Vec(20, 15), "ACCENT", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob>(Vec(15, 105), module, KIMO::ACCENT_PARAM));

        // ACCENT DELAY (78 -> 80, 88 -> 90)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 80), Vec(20, 15), "DELAY", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, 105), module, KIMO::ACCENT_DELAY_PARAM));

        // TUNE (118 -> 122)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 122), Vec(20, 15), "TUNE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(15, 147), module, KIMO::TUNE_PARAM));

        // FM (118 -> 122)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 122), Vec(20, 15), "FM", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, 147), module, KIMO::FM_PARAM));

        // PUNCH (158 -> 164)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 164), Vec(20, 15), "PUNCH", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(15, 189), module, KIMO::PUNCH_PARAM));

        // DECAY (158 -> 164)
        addChild(new TechnoEnhancedTextLabel(Vec(35, 164), Vec(20, 15), "DECAY", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, 189), module, KIMO::DECAY_PARAM));

        // SHAPE (198 -> 206)
        addChild(new TechnoEnhancedTextLabel(Vec(5, 206), Vec(20, 15), "SHAPE", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(15, 231), module, KIMO::SHAPE_PARAM));

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
};

Model* modelKIMO = createModel<KIMO, KIMOWidget>("KIMO");