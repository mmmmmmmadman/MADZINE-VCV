#include "plugin.hpp"

struct SwingLFO : Module {
    enum ParamId {
        FREQ_PARAM,
        SWING_PARAM,
        SHAPE_PARAM,
        MIX_PARAM,
        FREQ_CV_ATTEN_PARAM,
        SWING_CV_ATTEN_PARAM,
        SHAPE_CV_ATTEN_PARAM,
        MIX_CV_ATTEN_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        FREQ_CV_INPUT,
        SWING_CV_INPUT,
        SHAPE_CV_INPUT,
        RESET_INPUT,
        MIX_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        SAW_OUTPUT,
        PULSE_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    enum WaveformType {
        SAW = 0,
        PULSE = 1
    };

    float phase = 0.0f;
    float secondPhase = 0.0f;
    float prevResetTrigger = 0.0f;

    SwingLFO() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(FREQ_PARAM, -3.0f, 7.0f, 1.0f, "Frequency", " Hz", 2.0f, 1.0f);
        configParam(SWING_PARAM, 0.0f, 1.0f, 0.0f, "Swing", "°", 0.0f, -90.0f, 180.0f);
        configParam(SHAPE_PARAM, 0.0f, 1.0f, 0.5f, "Shape", "%", 0.f, 100.f);
        configParam(MIX_PARAM, 0.0f, 1.0f, 0.5f, "Mix");
        
        configParam(FREQ_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Freq CV Attenuverter");
        configParam(SWING_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Swing CV Attenuverter");
        configParam(SHAPE_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Shape CV Attenuverter");
        configParam(MIX_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Mix CV Attenuverter");
        
        configInput(FREQ_CV_INPUT, "Frequency CV");
        configInput(SWING_CV_INPUT, "Swing CV");
        configInput(SHAPE_CV_INPUT, "Shape CV");
        configInput(RESET_INPUT, "Reset");
        configInput(MIX_CV_INPUT, "Mix CV");
        
        configOutput(SAW_OUTPUT, "Saw Wave");
        configOutput(PULSE_OUTPUT, "Pulse Wave");
    }

    float getWaveform(float phase, int waveType, float shape) {
        switch (waveType) {
            case SAW: {
                if (shape < 0.5f) {
                    float sawWave = phase;
                    float triWave = (phase < 0.5f) ? (2.0f * phase) : (2.0f - 2.0f * phase);
                    float mix = shape * 2.0f;
                    return (sawWave * (1.0f - mix) + triWave * mix) * 10.0f;
                } else {
                    float triWave = (phase < 0.5f) ? (2.0f * phase) : (2.0f - 2.0f * phase);
                    float rampWave = 1.0f - phase;
                    float mix = (shape - 0.5f) * 2.0f;
                    return (triWave * (1.0f - mix) + rampWave * mix) * 10.0f;
                }
            }
            case PULSE: {
                float pulseWidth = 0.01f + shape * 0.29f;
                return (phase < pulseWidth) ? 10.0f : 0.0f;
            }
            default:
                return 0.0f;
        }
    }

    void process(const ProcessArgs& args) override {
        float freqParam = params[FREQ_PARAM].getValue();
        float freqCVAttenuation = params[FREQ_CV_ATTEN_PARAM].getValue();
        float freqCV = 0.0f;
        if (inputs[FREQ_CV_INPUT].isConnected()) {
            freqCV = inputs[FREQ_CV_INPUT].getVoltage() * freqCVAttenuation;
        }
        float freq = std::pow(2.0f, freqParam + freqCV) * 1.0f;
        
        float swingParam = params[SWING_PARAM].getValue();
        float swingCV = 0.0f;
        if (inputs[SWING_CV_INPUT].isConnected()) {
            float swingCVAttenuation = params[SWING_CV_ATTEN_PARAM].getValue();
            swingCV = inputs[SWING_CV_INPUT].getVoltage() / 10.0f * swingCVAttenuation;
        }
        float swing = swingParam + swingCV;
        swing = clamp(swing, 0.0f, 1.0f);
        
        float shapeParam = params[SHAPE_PARAM].getValue();
        float shapeCV = 0.0f;
        if (inputs[SHAPE_CV_INPUT].isConnected()) {
            float shapeCVAttenuation = params[SHAPE_CV_ATTEN_PARAM].getValue();
            shapeCV = inputs[SHAPE_CV_INPUT].getVoltage() / 10.0f * shapeCVAttenuation;
        }
        float shape = shapeParam + shapeCV;
        shape = clamp(shape, 0.0f, 1.0f);
        
        float mixParam = params[MIX_PARAM].getValue();
        float mixCV = 0.0f;
        if (inputs[MIX_CV_INPUT].isConnected()) {
            float mixCVAttenuation = params[MIX_CV_ATTEN_PARAM].getValue();
            mixCV = inputs[MIX_CV_INPUT].getVoltage() / 10.0f * mixCVAttenuation;
        }
        float mix = mixParam + mixCV;
        mix = clamp(mix, 0.0f, 1.0f);
        
        float phaseOffset = (180.0f - swing * 90.0f) * M_PI / 180.0f;
        
        float resetTrigger = 0.0f;
        if (inputs[RESET_INPUT].isConnected()) {
            resetTrigger = inputs[RESET_INPUT].getVoltage();
            if (resetTrigger >= 2.0f && prevResetTrigger < 2.0f) {
                phase = 0.0f;
                secondPhase = phaseOffset / (2.0f * M_PI);
                while (secondPhase >= 1.0f)
                    secondPhase -= 1.0f;
            }
            prevResetTrigger = resetTrigger;
        }
        
        float deltaPhase = freq * args.sampleTime;
        phase += deltaPhase;
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }
        
        secondPhase = phase + (phaseOffset / (2.0f * M_PI));
        while (secondPhase >= 1.0f)
            secondPhase -= 1.0f;
        
        if (outputs[SAW_OUTPUT].isConnected()) {
            float mainSaw = getWaveform(phase, SAW, shape);
            float secondSaw = getWaveform(secondPhase, SAW, shape);
            float mixedSaw = mainSaw * (1.0f - mix) + secondSaw * mix;
            outputs[SAW_OUTPUT].setVoltage(mixedSaw);
        }
        
        if (outputs[PULSE_OUTPUT].isConnected()) {
            float mainPulse = getWaveform(phase, PULSE, shape);
            float secondPulse = getWaveform(secondPhase, PULSE, shape);
            float mixedPulse = mainPulse * (1.0f - mix) + secondPulse * mix;
            outputs[PULSE_OUTPUT].setVoltage(mixedPulse);
        }
    }
};

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
            // Reduced to 60% of original boldness - using smaller offset and fewer passes
            float offset = 0.3f;  // 60% of original 0.5f offset
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

struct SwingLFOWidget : ModuleWidget {
    SwingLFOWidget(SwingLFO* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwingLFO.svg")));
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        float centerX = box.size.x / 2;
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "SwingLFO", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        addChild(new EnhancedTextLabel(Vec(0, 26), Vec(box.size.x, 20), "FREQ", 12.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX + 15, 59), module, SwingLFO::FREQ_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(5, 40), Vec(20, 20), "RST", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 65), module, SwingLFO::RESET_INPUT));
        
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 89), module, SwingLFO::FREQ_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 89), module, SwingLFO::FREQ_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(0, 105), Vec(box.size.x, 20), "SWING", 12.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX, 136), module, SwingLFO::SWING_PARAM));
        
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 166), module, SwingLFO::SWING_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 166), module, SwingLFO::SWING_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(0, 182), Vec(box.size.x, 20), "SHAPE", 12.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX, 214), module, SwingLFO::SHAPE_PARAM));
        
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 244), module, SwingLFO::SHAPE_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 244), module, SwingLFO::SHAPE_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(0, 257), Vec(box.size.x, 20), "MIX", 12.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX, 289), module, SwingLFO::MIX_PARAM));
        
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 317), module, SwingLFO::MIX_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 317), module, SwingLFO::MIX_CV_INPUT));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(60, 50)));
        
        addChild(new EnhancedTextLabel(Vec(5, 335), Vec(20, 20), "SAW", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, 343), module, SwingLFO::SAW_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(5, 360), Vec(20, 20), "PULSE", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, 368), module, SwingLFO::PULSE_OUTPUT));
    }
};

Model* modelSwingLFO = createModel<SwingLFO, SwingLFOWidget>("SwingLFO");