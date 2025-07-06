#include "plugin.hpp"

struct DensityParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        float value = getValue();
        int steps, primaryKnobs;
        
        if (value < 0.2f) {
            steps = 8 + (int)(value * 20);
            primaryKnobs = 2;
        } else if (value < 0.4f) {
            steps = 12 + (int)((value - 0.2f) * 40);
            primaryKnobs = 3;
        } else if (value < 0.6f) {
            steps = 20 + (int)((value - 0.4f) * 40);
            primaryKnobs = 4;
        } else {
            steps = 28 + (int)((value - 0.6f) * 50);
            primaryKnobs = 5;
        }
        steps = clamp(steps, 8, 48);
        
        return string::f("%d knobs, %d steps", primaryKnobs, steps);
    }
};

struct PPaTTTerning : Module {
    enum ParamId {
        K1_PARAM, K2_PARAM, K3_PARAM, K4_PARAM, K5_PARAM,
        STYLE_PARAM, DENSITY_PARAM, CHAOS_PARAM,
        CVD_ATTEN_PARAM, DELAY_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CLOCK_INPUT, RESET_INPUT, CVD_CV_INPUT, INPUTS_LEN
    };
    enum OutputId {
        CV_OUTPUT, TRIG_OUTPUT, CV2_OUTPUT, TRIG2_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId { 
        STYLE_LIGHT_RED,
        STYLE_LIGHT_GREEN,
        STYLE_LIGHT_BLUE,
        DELAY_LIGHT_RED,
        DELAY_LIGHT_GREEN,
        DELAY_LIGHT_BLUE,
        LIGHTS_LEN 
    };

    dsp::SchmittTrigger clockTrigger, resetTrigger, styleTrigger, delayTrigger;
    dsp::PulseGenerator gateOutPulse, gate2OutPulse;
    
    int currentStep = 0, sequenceLength = 16, stepToKnobMapping[64];
    float previousVoltage = -999.0f;
    int styleMode = 1;
    
    static const int MAX_DELAY = 8;
    float cvHistory[MAX_DELAY];
    int historyIndex = 0, track2Delay = 1;
    
    static const int CVD_BUFFER_SIZE = 192000;
    float cvdBuffer[CVD_BUFFER_SIZE];
    int cvdWriteIndex = 0;
    float sampleRate = 44100.0f;
    
    PPaTTTerning() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(K1_PARAM, -10.0f, 10.0f, 0.0f, "K1", "V");
        configParam(K2_PARAM, -10.0f, 10.0f, 2.0f, "K2", "V");
        configParam(K3_PARAM, -10.0f, 10.0f, 4.0f, "K3", "V");
        configParam(K4_PARAM, -10.0f, 10.0f, 6.0f, "K4", "V");
        configParam(K5_PARAM, -10.0f, 10.0f, 8.0f, "K5", "V");
        
        configParam(STYLE_PARAM, 0.0f, 2.0f, 1.0f, "Style");
        params[STYLE_PARAM].setValue((float)styleMode);
        
        configParam(DENSITY_PARAM, 0.0f, 1.0f, 0.5f, "Density");
        delete paramQuantities[DENSITY_PARAM];
        paramQuantities[DENSITY_PARAM] = new DensityParamQuantity;
        paramQuantities[DENSITY_PARAM]->module = this;
        paramQuantities[DENSITY_PARAM]->paramId = DENSITY_PARAM;
        paramQuantities[DENSITY_PARAM]->minValue = 0.0f;
        paramQuantities[DENSITY_PARAM]->maxValue = 1.0f;
        paramQuantities[DENSITY_PARAM]->defaultValue = 0.5f;
        paramQuantities[DENSITY_PARAM]->name = "Density";
        
        configParam(CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Chaos", "%", 0.f, 100.f);
        configParam(CVD_ATTEN_PARAM, 0.0f, 1.0f, 0.0f, "CVD Time/Attenuation");
        configParam(DELAY_PARAM, 0.0f, 5.0f, 1.0f, "Delay");
        params[DELAY_PARAM].setValue((float)track2Delay);
        
        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(CVD_CV_INPUT, "CVD Time CV");
        configOutput(CV_OUTPUT, "CV");
        configOutput(TRIG_OUTPUT, "Trigger");
        
        configLight(STYLE_LIGHT_RED, "");
        configLight(STYLE_LIGHT_GREEN, "");
        configLight(STYLE_LIGHT_BLUE, "");
        configLight(DELAY_LIGHT_RED, "");
        configLight(DELAY_LIGHT_GREEN, "");
        configLight(DELAY_LIGHT_BLUE, "");
        
        updateOutputDescriptions();
        
        for (int i = 0; i < MAX_DELAY; i++) cvHistory[i] = 0.0f;
        for (int i = 0; i < CVD_BUFFER_SIZE; i++) cvdBuffer[i] = 0.0f;
        generateMapping();
    }
    
    void onSampleRateChange() override {
        sampleRate = APP->engine->getSampleRate();
    }
    
    void updateOutputDescriptions() {
        configOutput(CV2_OUTPUT, string::f("CV2 (Delay %d + CVD)", track2Delay));
        configOutput(TRIG2_OUTPUT, string::f("Trigger 2 (Delay %d + CVD)", track2Delay));
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "track2Delay", json_integer(track2Delay));
        json_object_set_new(rootJ, "styleMode", json_integer(styleMode));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* t2 = json_object_get(rootJ, "track2Delay");
        if (t2) {
            track2Delay = clamp((int)json_integer_value(t2), 0, 5);
            params[DELAY_PARAM].setValue((float)track2Delay);
        }
        
        json_t* styleJ = json_object_get(rootJ, "styleMode");
        if (styleJ) {
            styleMode = clamp((int)json_integer_value(styleJ), 0, 2);
            params[STYLE_PARAM].setValue((float)styleMode);
        }
        
        updateOutputDescriptions();
    }

    void generateMapping() {
        int style = styleMode;
        float density = params[DENSITY_PARAM].getValue();
        float chaos = params[CHAOS_PARAM].getValue();
        
        if (density < 0.2f) {
            sequenceLength = 8 + (int)(density * 20);
        } else if (density < 0.4f) {
            sequenceLength = 12 + (int)((density - 0.2f) * 40);
        } else if (density < 0.6f) {
            sequenceLength = 20 + (int)((density - 0.4f) * 40);
        } else {
            sequenceLength = 28 + (int)((density - 0.2f) * 60);
        }
        sequenceLength = clamp(sequenceLength, 8, 48);
        
        if (chaos > 0.0f) {
            float chaosRange = chaos * sequenceLength * 0.5f;
            float randomOffset = (random::uniform() - 0.5f) * 2.0f * chaosRange;
            sequenceLength += (int)randomOffset;
            sequenceLength = clamp(sequenceLength, 4, 64);
        }
        
        int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;
        
        for (int i = 0; i < 64; i++) stepToKnobMapping[i] = 0;
        
        switch (style) {
            case 0:
                for (int i = 0; i < sequenceLength; i++) {
                    stepToKnobMapping[i] = i % primaryKnobs;
                }
                break;
            case 1: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < sequenceLength; i++) {
                    stepToKnobMapping[i] = minimalistPattern[i % 32] % primaryKnobs;
                }
                break;
            }
            case 2: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < sequenceLength; i++) {
                    stepToKnobMapping[i] = jumpPattern[i % 5] % primaryKnobs;
                }
                break;
            }
        }
        
        if (primaryKnobs < 5) {
            int insertInterval = sequenceLength / (5 - primaryKnobs + 1);
            for (int unusedKnob = primaryKnobs; unusedKnob < 5; unusedKnob++) {
                int insertPos = insertInterval * (unusedKnob - primaryKnobs + 1);
                if (insertPos < sequenceLength) stepToKnobMapping[insertPos] = unusedKnob;
            }
        }
        
        if (density > 0.8f) {
            int changeInterval = clamp(sequenceLength / 8, 3, 8);
            for (int i = changeInterval; i < sequenceLength; i += changeInterval) {
                stepToKnobMapping[i] = (stepToKnobMapping[i] + 2) % 5;
            }
        }
        
        if (chaos > 0.3f) {
            int chaosSteps = (int)(chaos * sequenceLength * 0.3f);
            for (int i = 0; i < chaosSteps; i++) {
                int randomStep = random::u32() % sequenceLength;
                stepToKnobMapping[randomStep] = random::u32() % 5;
            }
        }
    }

    void process(const ProcessArgs& args) override {
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            currentStep = 0;
            generateMapping();
            previousVoltage = -999.0f;
            for (int i = 0; i < MAX_DELAY; i++) cvHistory[i] = 0.0f;
            for (int i = 0; i < CVD_BUFFER_SIZE; i++) cvdBuffer[i] = 0.0f;
            historyIndex = 0;
            cvdWriteIndex = 0;
        }
        
        if (styleTrigger.process(params[STYLE_PARAM].getValue())) {
            styleMode = (styleMode + 1) % 3;
            params[STYLE_PARAM].setValue((float)styleMode);
            generateMapping();
        }
        
        if (delayTrigger.process(params[DELAY_PARAM].getValue())) {
            track2Delay = (track2Delay + 1) % 6;
            params[DELAY_PARAM].setValue((float)track2Delay);
            updateOutputDescriptions();
        }
        
        lights[STYLE_LIGHT_RED].setBrightness(styleMode == 0 ? 1.0f : 0.0f);
        lights[STYLE_LIGHT_GREEN].setBrightness(styleMode == 1 ? 1.0f : 0.0f);
        lights[STYLE_LIGHT_BLUE].setBrightness(styleMode == 2 ? 1.0f : 0.0f);
        
        float delayBrightness = (track2Delay == 0) ? 0.0f : (float)track2Delay / 5.0f;
        lights[DELAY_LIGHT_RED].setBrightness(delayBrightness);
        lights[DELAY_LIGHT_GREEN].setBrightness(0.0f);
        lights[DELAY_LIGHT_BLUE].setBrightness(delayBrightness);
        
        bool clockTriggered = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());
        if (clockTriggered) {
            int activeKnob = stepToKnobMapping[currentStep];
            float voltage = params[K1_PARAM + activeKnob].getValue();
            cvHistory[historyIndex] = voltage;
            
            currentStep = (currentStep + 1) % sequenceLength;
            generateMapping();
            
            int newActiveKnob = stepToKnobMapping[currentStep];
            float newVoltage = params[K1_PARAM + newActiveKnob].getValue();
            
            if (newVoltage != previousVoltage) gateOutPulse.trigger(0.01f);
            previousVoltage = newVoltage;
            
            int delay1Index = (historyIndex - track2Delay + 1 + MAX_DELAY) % MAX_DELAY;
            int delay2Index = (historyIndex - track2Delay + MAX_DELAY) % MAX_DELAY;
            if (track2Delay > 0 && cvHistory[delay1Index] != cvHistory[delay2Index]) gate2OutPulse.trigger(0.01f);
            
            historyIndex = (historyIndex + 1) % MAX_DELAY;
        }
        
        int activeKnob = stepToKnobMapping[currentStep];
        outputs[CV_OUTPUT].setVoltage(params[K1_PARAM + activeKnob].getValue());
        outputs[TRIG_OUTPUT].setVoltage(gateOutPulse.process(args.sampleTime) ? 10.0f : 0.0f);
        
        int shiftRegisterIndex = (historyIndex - track2Delay + MAX_DELAY) % MAX_DELAY;
        float shiftRegisterCV = (track2Delay == 0) ? outputs[CV_OUTPUT].getVoltage() : cvHistory[shiftRegisterIndex];
        
        float delayTimeMs = 0.0f;
        float knobValue = params[CVD_ATTEN_PARAM].getValue();
        
        if (!inputs[CVD_CV_INPUT].isConnected()) {
            delayTimeMs = knobValue * 1000.0f;
        } else {
            float cvdCV = clamp(inputs[CVD_CV_INPUT].getVoltage(), 0.0f, 10.0f);
            delayTimeMs = (cvdCV / 10.0f) * knobValue * 1000.0f;
        }
        
        if (delayTimeMs <= 0.001f) {
            outputs[CV2_OUTPUT].setVoltage(shiftRegisterCV);
        } else {
            cvdBuffer[cvdWriteIndex] = shiftRegisterCV;
            cvdWriteIndex = (cvdWriteIndex + 1) % CVD_BUFFER_SIZE;
            
            int delaySamples = (int)(delayTimeMs * sampleRate / 1000.0f);
            delaySamples = clamp(delaySamples, 0, CVD_BUFFER_SIZE - 1);
            
            int readIndex = (cvdWriteIndex - delaySamples + CVD_BUFFER_SIZE) % CVD_BUFFER_SIZE;
            float delayedCV = cvdBuffer[readIndex];
            
            outputs[CV2_OUTPUT].setVoltage(delayedCV);
        }
        
        outputs[TRIG2_OUTPUT].setVoltage(gate2OutPulse.process(args.sampleTime) ? 10.0f : 0.0f);
    }
};

struct DelayParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        PPaTTTerning* module = dynamic_cast<PPaTTTerning*>(this->module);
        if (!module) return "1 step";
        
        int delay = module->track2Delay;
        if (delay == 0) return "No delay";
        if (delay == 1) return "1 step";
        return string::f("%d steps", delay);
    }
    
    std::string getLabel() override {
        return "Delay";
    }
};

struct StyleParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        PPaTTTerning* module = dynamic_cast<PPaTTTerning*>(this->module);
        if (!module) return "Sequential";
        
        switch (module->styleMode) {
            case 0: return "Sequential";
            case 1: return "Minimalism";
            case 2: return "Jump";
            default: return "Minimalism";
        }
    }
    
    std::string getLabel() override {
        return "Mode";
    }
};

struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    EnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f, 
                      NVGcolor color = nvgRGB(255, 255, 255), bool bold = true) {
        box.pos = pos; box.size = size; this->text = text; this->fontSize = fontSize; this->color = color; this->bold = bold;
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
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

struct StandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    StandardBlackKnob() { box.size = Vec(30, 30); }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        float normalizedValue = pq->getScaledValue();
        return rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
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
        } else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
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
        float newValue = clamp(currentValue + deltaY * sensitivity * range, pq->getMinValue(), pq->getMaxValue());
        pq->setValue(newValue);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        pq->reset();
        e.consume(this);
    }
};

struct WhiteBackgroundBox : Widget {
    WhiteBackgroundBox(Vec pos, Vec size) { box.pos = pos; box.size = size; }
    
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

struct PPaTTTerningWidget : ModuleWidget {
    PPaTTTerningWidget(PPaTTTerning* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwingLFO.svg")));
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        float centerX = box.size.x / 2;
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "PPaTTTerning", 9.5f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        addChild(new EnhancedTextLabel(Vec(5, 31), Vec(20, 20), "CLK", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 55), module, PPaTTTerning::CLOCK_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 31), Vec(20, 20), "RST", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 55), module, PPaTTTerning::RESET_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(8, 69), Vec(15, 15), "1", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX - 15, 97), module, PPaTTTerning::K1_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(38, 74), Vec(15, 15), "MODE", 7.f, nvgRGB(255, 255, 255), true));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(centerX + 15, 97), module, PPaTTTerning::STYLE_LIGHT_RED));
        addParam(createParamCentered<VCVButton>(Vec(centerX + 15, 97), module, PPaTTTerning::STYLE_PARAM));
        
        if (module) {
            delete module->paramQuantities[PPaTTTerning::STYLE_PARAM];
            module->paramQuantities[PPaTTTerning::STYLE_PARAM] = new StyleParamQuantity;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->module = module;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->paramId = PPaTTTerning::STYLE_PARAM;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->minValue = 0.0f;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->maxValue = 2.0f;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->defaultValue = 1.0f;
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->name = "Mode";
            module->paramQuantities[PPaTTTerning::STYLE_PARAM]->snapEnabled = true;
        }
        
        addChild(new EnhancedTextLabel(Vec(8, 114), Vec(15, 15), "2", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX - 15, 142), module, PPaTTTerning::K2_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(32, 114), Vec(26, 15), "DENSITY", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX + 15, 142), module, PPaTTTerning::DENSITY_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(8, 159), Vec(15, 15), "3", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX - 15, 187), module, PPaTTTerning::K3_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(35, 159), Vec(20, 15), "CHAOS", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX + 15, 187), module, PPaTTTerning::CHAOS_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(8, 204), Vec(15, 15), "4", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX - 15, 232), module, PPaTTTerning::K4_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(32, 204), Vec(26, 15), "CV OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, 232), module, PPaTTTerning::CV_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(8, 249), Vec(15, 15), "5", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(centerX - 15, 277), module, PPaTTTerning::K5_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(30, 249), Vec(30, 15), "TRIG", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, 277), module, PPaTTTerning::TRIG_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(5, 294), Vec(20, 15), "T2.DLY", 7.f, nvgRGB(255, 255, 255), true));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(15, 315), module, PPaTTTerning::DELAY_LIGHT_RED));
        addParam(createParamCentered<VCVButton>(Vec(15, 315), module, PPaTTTerning::DELAY_PARAM));
        
        if (module) {
            delete module->paramQuantities[PPaTTTerning::DELAY_PARAM];
            module->paramQuantities[PPaTTTerning::DELAY_PARAM] = new DelayParamQuantity;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->module = module;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->paramId = PPaTTTerning::DELAY_PARAM;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->minValue = 0.0f;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->maxValue = 5.0f;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->defaultValue = 1.0f;
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->name = "Delay";
            module->paramQuantities[PPaTTTerning::DELAY_PARAM]->snapEnabled = true;
        }
        
        addChild(new EnhancedTextLabel(Vec(30, 295), Vec(30, 15), "Taiwan", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(30, 305), Vec(30, 15), "is NOT", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(32, 315), Vec(30, 15), "China", 8.f, nvgRGB(255, 255, 255), true));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(60, 50)));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 345), module, PPaTTTerning::CV2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 345), module, PPaTTTerning::TRIG2_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(5, 360), Vec(20, 15), "CVD", 7.f, nvgRGB(255, 133, 133), true));
        addParam(createParamCentered<Trimpot>(Vec(15, 370), module, PPaTTTerning::CVD_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 370), module, PPaTTTerning::CVD_CV_INPUT));
    }
};

Model* modelPPaTTTerning = createModel<PPaTTTerning, PPaTTTerningWidget>("PPaTTTerning");