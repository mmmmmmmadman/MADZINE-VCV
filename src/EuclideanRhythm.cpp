#include "plugin.hpp"
#include <vector>
#include <numeric>
#include <algorithm>

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

struct DivMultParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        if (value > 0) {
            return string::f("%dx", value + 1);
        } else if (value < 0) {
            return string::f("1/%dx", -value + 1);
        } else {
            return "1x";
        }
    }
};

std::vector<bool> generateEuclideanRhythm(int length, int fill, int shift) {
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

struct SlewLimiter {
    float riseRate = 0.f;
    float fallRate = 0.f;
    float out = 0.f;

    void setRiseTime(float time) {
        if (time > 0.f) {
            riseRate = 1.f / time;
        } else {
            riseRate = INFINITY;
        }
    }

    void setFallTime(float time) {
        if (time > 0.f) {
            fallRate = 1.f / time;
        } else {
            fallRate = INFINITY;
        }
    }

    float process(float deltaTime, float in) {
        if (in > out) {
            if (std::isinf(riseRate)) {
                out = in;
            } else {
                float diff = in - out;
                out += diff * riseRate * deltaTime;
                if (out > in) {
                    out = in;
                }
            }
        } else if (in < out) {
            if (std::isinf(fallRate)) {
                out = in;
            } else {
                float diff = out - in;
                out -= diff * fallRate * deltaTime;
                if (out < in) {
                    out = in;
                }
            }
        }
        return out;
    }

    void reset() {
        out = 0.f;
    }
};

struct EuclideanRhythm : Module {
    enum ParamId {
        MANUAL_RESET_PARAM,
        TRACK1_DIVMULT_PARAM,
        TRACK1_LENGTH_PARAM,
        TRACK1_FILL_PARAM,
        TRACK1_SHIFT_PARAM,
        TRACK1_LENGTH_CV_ATTEN_PARAM,
        TRACK1_FILL_CV_ATTEN_PARAM,
        TRACK1_SHIFT_CV_ATTEN_PARAM,
        TRACK1_RISE_PARAM,
        TRACK1_FALL_PARAM,
        TRACK2_DIVMULT_PARAM,
        TRACK2_LENGTH_PARAM,
        TRACK2_FILL_PARAM,
        TRACK2_SHIFT_PARAM,
        TRACK2_LENGTH_CV_ATTEN_PARAM,
        TRACK2_FILL_CV_ATTEN_PARAM,
        TRACK2_SHIFT_CV_ATTEN_PARAM,
        TRACK2_RISE_PARAM,
        TRACK2_FALL_PARAM,
        TRACK3_DIVMULT_PARAM,
        TRACK3_LENGTH_PARAM,
        TRACK3_FILL_PARAM,
        TRACK3_SHIFT_PARAM,
        TRACK3_LENGTH_CV_ATTEN_PARAM,
        TRACK3_FILL_CV_ATTEN_PARAM,
        TRACK3_SHIFT_CV_ATTEN_PARAM,
        TRACK3_RISE_PARAM,
        TRACK3_FALL_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        GLOBAL_CLOCK_INPUT,
        GLOBAL_RESET_INPUT,
        TRACK1_LENGTH_CV_INPUT,
        TRACK1_FILL_CV_INPUT,
        TRACK1_SHIFT_CV_INPUT,
        TRACK2_LENGTH_CV_INPUT,
        TRACK2_FILL_CV_INPUT,
        TRACK2_SHIFT_CV_INPUT,
        TRACK3_LENGTH_CV_INPUT,
        TRACK3_FILL_CV_INPUT,
        TRACK3_SHIFT_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        TRACK1_OUTPUT,
        TRACK2_OUTPUT,
        TRACK3_OUTPUT,
        MASTER_OUTPUT,
        TRACK1_TRIG_OUTPUT,
        TRACK2_TRIG_OUTPUT,
        TRACK3_TRIG_OUTPUT,
        MASTER_TRIG_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        TRACK1_LIGHT,
        TRACK2_LIGHT,
        TRACK3_LIGHT,
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger manualResetTrigger;
    float lastClockTime = 0.f;
    float clockInterval = 0.5f;

    struct TrackState {
        int divMultValue = 1;
        float internalClock = 0.f;
        float trackInterval = 0.5f;
        int currentStep = 0;
        int length = 16;
        int fill = 4;
        int shift = 0;
        std::vector<bool> pattern;
        SlewLimiter slewLimiter;
        bool gateState = false;
        bool lastGateState = false;
        dsp::PulseGenerator trigPulse;

        void reset() {
            internalClock = 0.f;
            currentStep = 0;
            pattern.clear();
            slewLimiter.reset();
            gateState = false;
            lastGateState = false;
        }
    };
    TrackState tracks[3];

    EuclideanRhythm() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(GLOBAL_CLOCK_INPUT, "Global Clock");
        configInput(GLOBAL_RESET_INPUT, "Global Reset");
        configParam(MANUAL_RESET_PARAM, 0.0f, 1.0f, 0.0f, "Manual Reset");

        configParam(TRACK1_DIVMULT_PARAM, -3.0f, 3.0f, 0.0f, "T1 Div/Mult");
        getParamQuantity(TRACK1_DIVMULT_PARAM)->snapEnabled = true;
        delete paramQuantities[TRACK1_DIVMULT_PARAM];
        paramQuantities[TRACK1_DIVMULT_PARAM] = new DivMultParamQuantity;
        paramQuantities[TRACK1_DIVMULT_PARAM]->module = this;
        paramQuantities[TRACK1_DIVMULT_PARAM]->paramId = TRACK1_DIVMULT_PARAM;
        paramQuantities[TRACK1_DIVMULT_PARAM]->minValue = -3.0f;
        paramQuantities[TRACK1_DIVMULT_PARAM]->maxValue = 3.0f;
        paramQuantities[TRACK1_DIVMULT_PARAM]->defaultValue = 0.0f;
        paramQuantities[TRACK1_DIVMULT_PARAM]->name = "T1 Div/Mult";
        paramQuantities[TRACK1_DIVMULT_PARAM]->snapEnabled = true;
        configParam(TRACK1_LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "T1 Length");
        getParamQuantity(TRACK1_LENGTH_PARAM)->snapEnabled = true;
        configParam(TRACK1_FILL_PARAM, 0.0f, 100.0f, 25.0f, "T1 Fill", "%");
        configParam(TRACK1_SHIFT_PARAM, 0.0f, 31.0f, 0.0f, "T1 Shift");
        getParamQuantity(TRACK1_SHIFT_PARAM)->snapEnabled = true;
        configParam(TRACK1_LENGTH_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T1 Length CV");
        configParam(TRACK1_FILL_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T1 Fill CV");
        configParam(TRACK1_SHIFT_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T1 Shift CV");
        configParam(TRACK1_RISE_PARAM, 0.0f, 0.1f, 0.0f, "T1 Rise", "s");
        configParam(TRACK1_FALL_PARAM, 0.0f, 0.1f, 0.0f, "T1 Fall", "s");
        configInput(TRACK1_LENGTH_CV_INPUT, "T1 Length CV");
        configInput(TRACK1_FILL_CV_INPUT, "T1 Fill CV");
        configInput(TRACK1_SHIFT_CV_INPUT, "T1 Shift CV");
        configOutput(TRACK1_OUTPUT, "T1 Output");
        configOutput(TRACK1_TRIG_OUTPUT, "T1 Trigger");
        configLight(TRACK1_LIGHT, "T1 Light");

        configParam(TRACK2_DIVMULT_PARAM, -3.0f, 3.0f, 0.0f, "T2 Div/Mult");
        getParamQuantity(TRACK2_DIVMULT_PARAM)->snapEnabled = true;
        delete paramQuantities[TRACK2_DIVMULT_PARAM];
        paramQuantities[TRACK2_DIVMULT_PARAM] = new DivMultParamQuantity;
        paramQuantities[TRACK2_DIVMULT_PARAM]->module = this;
        paramQuantities[TRACK2_DIVMULT_PARAM]->paramId = TRACK2_DIVMULT_PARAM;
        paramQuantities[TRACK2_DIVMULT_PARAM]->minValue = -3.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->maxValue = 3.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->defaultValue = 0.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->name = "T2 Div/Mult";
        paramQuantities[TRACK2_DIVMULT_PARAM]->snapEnabled = true;
        configParam(TRACK2_LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "T2 Length");
        getParamQuantity(TRACK2_LENGTH_PARAM)->snapEnabled = true;
        configParam(TRACK2_FILL_PARAM, 0.0f, 100.0f, 25.0f, "T2 Fill", "%");
        configParam(TRACK2_SHIFT_PARAM, 0.0f, 31.0f, 0.0f, "T2 Shift");
        getParamQuantity(TRACK2_SHIFT_PARAM)->snapEnabled = true;
        configParam(TRACK2_LENGTH_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T2 Length CV");
        configParam(TRACK2_FILL_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T2 Fill CV");
        configParam(TRACK2_SHIFT_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T2 Shift CV");
        configParam(TRACK2_RISE_PARAM, 0.0f, 0.1f, 0.0f, "T2 Rise", "s");
        configParam(TRACK2_FALL_PARAM, 0.0f, 0.1f, 0.0f, "T2 Fall", "s");
        configInput(TRACK2_LENGTH_CV_INPUT, "T2 Length CV");
        configInput(TRACK2_FILL_CV_INPUT, "T2 Fill CV");
        configInput(TRACK2_SHIFT_CV_INPUT, "T2 Shift CV");
        configOutput(TRACK2_OUTPUT, "T2 Output");
        configOutput(TRACK2_TRIG_OUTPUT, "T2 Trigger");
        configLight(TRACK2_LIGHT, "T2 Light");

        configParam(TRACK3_DIVMULT_PARAM, -3.0f, 3.0f, 0.0f, "T3 Div/Mult");
        getParamQuantity(TRACK3_DIVMULT_PARAM)->snapEnabled = true;
        delete paramQuantities[TRACK3_DIVMULT_PARAM];
        paramQuantities[TRACK3_DIVMULT_PARAM] = new DivMultParamQuantity;
        paramQuantities[TRACK3_DIVMULT_PARAM]->module = this;
        paramQuantities[TRACK3_DIVMULT_PARAM]->paramId = TRACK3_DIVMULT_PARAM;
        paramQuantities[TRACK3_DIVMULT_PARAM]->minValue = -3.0f;
        paramQuantities[TRACK3_DIVMULT_PARAM]->maxValue = 3.0f;
        paramQuantities[TRACK3_DIVMULT_PARAM]->defaultValue = 0.0f;
        paramQuantities[TRACK3_DIVMULT_PARAM]->name = "T3 Div/Mult";
        paramQuantities[TRACK3_DIVMULT_PARAM]->snapEnabled = true;
        configParam(TRACK3_LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "T3 Length");
        getParamQuantity(TRACK3_LENGTH_PARAM)->snapEnabled = true;
        configParam(TRACK3_FILL_PARAM, 0.0f, 100.0f, 25.0f, "T3 Fill", "%");
        configParam(TRACK3_SHIFT_PARAM, 0.0f, 31.0f, 0.0f, "T3 Shift");
        getParamQuantity(TRACK3_SHIFT_PARAM)->snapEnabled = true;
        configParam(TRACK3_LENGTH_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T3 Length CV");
        configParam(TRACK3_FILL_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T3 Fill CV");
        configParam(TRACK3_SHIFT_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "T3 Shift CV");
        configParam(TRACK3_RISE_PARAM, 0.0f, 0.1f, 0.0f, "T3 Rise", "s");
        configParam(TRACK3_FALL_PARAM, 0.0f, 0.1f, 0.0f, "T3 Fall", "s");
        configInput(TRACK3_LENGTH_CV_INPUT, "T3 Length CV");
        configInput(TRACK3_FILL_CV_INPUT, "T3 Fill CV");
        configInput(TRACK3_SHIFT_CV_INPUT, "T3 Shift CV");
        configOutput(TRACK3_OUTPUT, "T3 Output");
        configOutput(TRACK3_TRIG_OUTPUT, "T3 Trigger");
        configLight(TRACK3_LIGHT, "T3 Light");
        
        configOutput(MASTER_OUTPUT, "Master Mix Output");
        configOutput(MASTER_TRIG_OUTPUT, "Master Trigger Sum");
    }

    void onReset() override {
        for (int i = 0; i < 3; ++i) {
            tracks[i].reset();
        }
    }

    void process(const ProcessArgs& args) override {
        bool globalClockActive = inputs[GLOBAL_CLOCK_INPUT].isConnected();
        bool globalClockTriggered = false;
        bool globalResetTriggered = false;
        bool manualResetTriggered = false;
        float clockVoltage = 0.0f;
        
        if (globalClockActive) {
            clockVoltage = inputs[GLOBAL_CLOCK_INPUT].getVoltage();
            globalClockTriggered = clockTrigger.process(clockVoltage);
        }
        
        if (inputs[GLOBAL_RESET_INPUT].isConnected()) {
            globalResetTriggered = resetTrigger.process(inputs[GLOBAL_RESET_INPUT].getVoltage());
        }
        
        manualResetTriggered = manualResetTrigger.process(params[MANUAL_RESET_PARAM].getValue());
        
        if (globalClockTriggered) {
            float currentTime = args.sampleTime * args.frame;
            if (lastClockTime > 0.f) {
                clockInterval = currentTime - lastClockTime;
                clockInterval = clamp(clockInterval, 0.01f, 10.0f);
            }
            lastClockTime = currentTime;
        }

        for (int i = 0; i < 3; ++i) {
            TrackState& track = tracks[i];
            
            if (globalResetTriggered || manualResetTriggered) {
                track.reset();
            }
            
            track.divMultValue = (int)std::round(params[TRACK1_DIVMULT_PARAM + i * 9].getValue());
            
            if (track.divMultValue > 0) {
                track.trackInterval = clockInterval / (track.divMultValue + 1);
            } else if (track.divMultValue < 0) {
                track.trackInterval = clockInterval * (-track.divMultValue + 1);
            } else {
                track.trackInterval = clockInterval;
            }

            float lengthParam = params[TRACK1_LENGTH_PARAM + i * 9].getValue();
            float lengthCV = 0.0f;
            if (inputs[TRACK1_LENGTH_CV_INPUT + i * 3].isConnected()) {
                float lengthCVAtten = params[TRACK1_LENGTH_CV_ATTEN_PARAM + i * 9].getValue();
                lengthCV = inputs[TRACK1_LENGTH_CV_INPUT + i * 3].getVoltage() * lengthCVAtten;
            }
            track.length = (int)std::round(clamp(lengthParam + lengthCV, 1.0f, 32.0f));

            float fillParam = params[TRACK1_FILL_PARAM + i * 9].getValue();
            float fillCV = 0.0f;
            if (inputs[TRACK1_FILL_CV_INPUT + i * 3].isConnected()) {
                float fillCVAtten = params[TRACK1_FILL_CV_ATTEN_PARAM + i * 9].getValue();
                fillCV = inputs[TRACK1_FILL_CV_INPUT + i * 3].getVoltage() * fillCVAtten * 10.0f;
            }
            float fillPercentage = clamp(fillParam + fillCV, 0.0f, 100.0f);
            track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

            float shiftParam = params[TRACK1_SHIFT_PARAM + i * 9].getValue();
            float shiftCV = 0.0f;
            if (inputs[TRACK1_SHIFT_CV_INPUT + i * 3].isConnected()) {
                float shiftCVAtten = params[TRACK1_SHIFT_CV_ATTEN_PARAM + i * 9].getValue();
                shiftCV = inputs[TRACK1_SHIFT_CV_INPUT + i * 3].getVoltage() * shiftCVAtten;
            }
            track.shift = (int)std::round(clamp(shiftParam + shiftCV, 0.0f, (float)track.length - 1.0f));

            float riseTime = params[TRACK1_RISE_PARAM + i * 9].getValue();
            float fallTime = params[TRACK1_FALL_PARAM + i * 9].getValue();
            track.slewLimiter.setRiseTime(riseTime);
            track.slewLimiter.setFallTime(fallTime);

            track.internalClock += args.sampleTime;
            bool trackClockTrigger = false;
            if (track.internalClock >= track.trackInterval) {
                track.internalClock -= track.trackInterval;
                trackClockTrigger = true;
            }
            
            track.pattern = generateEuclideanRhythm(track.length, track.fill, track.shift);

            if (trackClockTrigger && !track.pattern.empty()) {
                track.currentStep = (track.currentStep + 1) % track.length;
                track.gateState = track.pattern[track.currentStep];
                if (track.gateState && globalClockActive) {
                    track.trigPulse.trigger(0.001f);
                }
            }
            
            float rawOutput = 0.0f;
            if (track.gateState && globalClockActive) {
                float pulseWidth = track.trackInterval * 0.5f;
                float timeInCycle = track.internalClock;
                if (timeInCycle < pulseWidth) {
                    rawOutput = 10.0f;
                }
            }
            
            float outputVoltage = track.slewLimiter.process(args.sampleTime, rawOutput);
            
            outputs[TRACK1_OUTPUT + i].setVoltage(outputVoltage);
            
            float trigOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
            outputs[TRACK1_TRIG_OUTPUT + i].setVoltage(trigOutput);
            
            lights[TRACK1_LIGHT + i].setBrightness(track.gateState ? 1.0f : 0.0f);
        }
        
        float masterMix = 0.0f;
        for (int i = 0; i < 3; ++i) {
            float trackVoltage = outputs[TRACK1_OUTPUT + i].getVoltage();
            masterMix += trackVoltage * 0.4f;
        }
        outputs[MASTER_OUTPUT].setVoltage(masterMix);
        
        float masterTrigSum = 0.0f;
        for (int i = 0; i < 3; ++i) {
            if (outputs[TRACK1_TRIG_OUTPUT + i].getVoltage() > 0.0f) {
                masterTrigSum = 10.0f;
                break;
            }
        }
        outputs[MASTER_TRIG_OUTPUT].setVoltage(masterTrigSum);
    }
};

struct EuclideanRhythmWidget : ModuleWidget {
    EuclideanRhythmWidget(EuclideanRhythm* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Euclidean Rhythm", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new EnhancedTextLabel(Vec(18, 34), Vec(30, 15), "CLK", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(33, 56), module, EuclideanRhythm::GLOBAL_CLOCK_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(62, 34), Vec(30, 15), "RST", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(77, 56), module, EuclideanRhythm::GLOBAL_RESET_INPUT));
        
        addParam(createParamCentered<VCVButton>(Vec(100, 56), module, EuclideanRhythm::MANUAL_RESET_PARAM));

        float trackY[3] = {77, 159, 241};
        
        for (int i = 0; i < 3; ++i) {
            float y = trackY[i];
            float x = 1;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "LEN", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_LENGTH_PARAM + i * 9));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_LENGTH_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_LENGTH_CV_ATTEN_PARAM + i * 9));
            x += 31;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "FILL", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_FILL_PARAM + i * 9));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_FILL_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_FILL_CV_ATTEN_PARAM + i * 9));
            x += 31;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "SHFT", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_SHIFT_PARAM + i * 9));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_SHIFT_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_SHIFT_CV_ATTEN_PARAM + i * 9));
            x += 30;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "D/M", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_DIVMULT_PARAM + i * 9));
            
            addChild(new EnhancedTextLabel(Vec(x, y + 35), Vec(25, 10), "SLEW", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 52), module, EuclideanRhythm::TRACK1_RISE_PARAM + i * 9));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 70), module, EuclideanRhythm::TRACK1_FALL_PARAM + i * 9));
        }
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 325)));
        
        float outputY = 343;
        float outputSpacing = 31;
        float startX = 13;
        
        for (int i = 0; i < 3; ++i) {
            float outputX = startX + i * outputSpacing;
            addChild(new EnhancedTextLabel(Vec(outputX - 12, 322), Vec(25, 10), "OUT " + std::to_string(i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addOutput(createOutputCentered<PJ301MPort>(Vec(outputX, outputY), module, EuclideanRhythm::TRACK1_OUTPUT + i));
        }
        
        float mixX = startX + 3 * outputSpacing - 2;
        addChild(new EnhancedTextLabel(Vec(mixX - 12, 322), Vec(25, 10), "SUM", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixX, outputY), module, EuclideanRhythm::MASTER_OUTPUT));
        
        float trigY = 368;
        
        for (int i = 0; i < 3; ++i) {
            float outputX = startX + i * outputSpacing;
            addOutput(createOutputCentered<PJ301MPort>(Vec(outputX, trigY), module, EuclideanRhythm::TRACK1_TRIG_OUTPUT + i));
        }
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixX, trigY), module, EuclideanRhythm::MASTER_TRIG_OUTPUT));
    }
};

Model* modelEuclideanRhythm = createModel<EuclideanRhythm, EuclideanRhythmWidget>("EuclideanRhythm");