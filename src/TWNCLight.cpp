#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

struct TWNCLightEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    TWNCLightEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f, 
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





struct TWNCLightDivMultParamQuantity : ParamQuantity {
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

struct TWNCLightVCAShiftParamQuantity : ParamQuantity {
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

std::vector<bool> generateTWNCLightEuclideanRhythm(int length, int fill, int shift) {
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

struct TWNCLight : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamId {
        GLOBAL_LENGTH_PARAM,
        TRACK1_FILL_PARAM,
        VCA_SHIFT_PARAM,
        VCA_DECAY_PARAM,
        TRACK1_DECAY_PARAM,
        TRACK1_SHAPE_PARAM,
        TRACK2_FILL_PARAM,
        TRACK2_DIVMULT_PARAM,
        TRACK2_DECAY_PARAM,
        TRACK2_SHAPE_PARAM,
        TRACK2_SHIFT_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        GLOBAL_CLOCK_INPUT,
        DRUM_FREQ_CV_INPUT,
        DRUM_DECAY_CV_INPUT,
        HATS_FREQ_CV_INPUT,
        HATS_DECAY_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        MAIN_VCA_ENV_OUTPUT,
        TRACK1_FM_ENV_OUTPUT,
        TRACK2_VCA_ENV_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;
    
    float globalClockSeconds = 0.5f;
    float secondsSinceLastClock = -1.0f;
    int globalClockCount = 0;

    struct QuarterNoteClock {
        int currentStep = 0;
        int shiftAmount = 1;
        dsp::PulseGenerator trigPulse;
        
        void reset() {
            currentStep = 0;
        }
        
        bool processStep(bool globalClockTriggered, int globalLength, int shift) {
            shiftAmount = shift;
            if (globalClockTriggered) {
		    currentStep = (currentStep + 1) % 4;
    
		    int targetStep = (shiftAmount - 1) % 4;
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
        int divMultValue = 0;
        int division = 1;
        int multiplication = 1;
        float dividedClockSeconds = 0.5f;
        float multipliedClockSeconds = 0.5f;
        float dividedProgressSeconds = 0.0f;
        float gateSeconds = 0.0f;
        int dividerCount = 0;
        bool shouldStep = false;
        bool prevMultipliedGate = false;
        
        int currentStep = 0;
        int length = 16;
        int fill = 4;
        int shift = 0;
        std::vector<bool> pattern;
        bool gateState = false;
        dsp::PulseGenerator trigPulse;
        
        UnifiedEnvelope envelope;
        UnifiedEnvelope vcaEnvelope;

        void reset() {
            dividedProgressSeconds = 0.0f;
            dividerCount = 0;
            shouldStep = false;
            prevMultipliedGate = false;
            currentStep = 0;
            pattern.clear();
            gateState = false;
            envelope.reset();
            vcaEnvelope.reset();
        }
        
        void updateDivMult(int divMultParam) {
            divMultValue = divMultParam;
            if (divMultParam > 0) {
                division = 1;
                multiplication = divMultParam + 1;
            } else if (divMultParam < 0) {
                division = -divMultParam + 1;
                multiplication = 1;
            } else {
                division = 1;
                multiplication = 1;
            }
        }
        
        bool processClockDivMult(bool globalClock, float globalClockSeconds, float sampleTime) {
            dividedClockSeconds = globalClockSeconds * (float)division;
            multipliedClockSeconds = dividedClockSeconds / (float)multiplication;
            gateSeconds = std::max(0.001f, multipliedClockSeconds * 0.5f);
            
            if (globalClock) {
                if (dividerCount < 1) {
                    dividedProgressSeconds = 0.0f;
                } else {
                    dividedProgressSeconds += sampleTime;
                }
                ++dividerCount;
                if (dividerCount >= division) {
                    dividerCount = 0;
                }
            } else {
                dividedProgressSeconds += sampleTime;
            }
            
            shouldStep = false;
            if (dividedProgressSeconds < dividedClockSeconds) {
                float multipliedProgressSeconds = dividedProgressSeconds / multipliedClockSeconds;
                multipliedProgressSeconds -= (float)(int)multipliedProgressSeconds;
                multipliedProgressSeconds *= multipliedClockSeconds;
                
                bool currentMultipliedGate = multipliedProgressSeconds <= gateSeconds;
                
                if (currentMultipliedGate && !prevMultipliedGate) {
                    shouldStep = true;
                }
                prevMultipliedGate = currentMultipliedGate;
            }
            
            return shouldStep;
        }
        
        void stepTrack() {
            currentStep = (currentStep + 1) % length;
            gateState = !pattern.empty() && pattern[currentStep];
            if (gateState) {
                trigPulse.trigger(0.01f);
            }
        }
    };
    TrackState tracks[2];
    QuarterNoteClock quarterClock;
    UnifiedEnvelope mainVCA;

    TWNCLight() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(GLOBAL_CLOCK_INPUT, "Global Clock");
        configInput(DRUM_FREQ_CV_INPUT, "Drum Frequency CV");
        configInput(DRUM_DECAY_CV_INPUT, "Drum Decay CV");
        configInput(HATS_FREQ_CV_INPUT, "Hats Frequency CV");
        configInput(HATS_DECAY_CV_INPUT, "Hats Decay CV");
        
        configParam(GLOBAL_LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "Global Length");
        getParamQuantity(GLOBAL_LENGTH_PARAM)->snapEnabled = true;

        configParam(TRACK1_FILL_PARAM, 0.0f, 100.0f, 66.599990844726562f, "Track 1 Fill", "%");
        
        configParam(VCA_SHIFT_PARAM, 1.0f, 7.0f, 1.0f, "VCA Shift");
        getParamQuantity(VCA_SHIFT_PARAM)->snapEnabled = true;
        delete paramQuantities[VCA_SHIFT_PARAM];
        paramQuantities[VCA_SHIFT_PARAM] = new TWNCLightVCAShiftParamQuantity;
        paramQuantities[VCA_SHIFT_PARAM]->module = this;
        paramQuantities[VCA_SHIFT_PARAM]->paramId = VCA_SHIFT_PARAM;
        paramQuantities[VCA_SHIFT_PARAM]->minValue = 1.0f;
        paramQuantities[VCA_SHIFT_PARAM]->maxValue = 7.0f;
        paramQuantities[VCA_SHIFT_PARAM]->defaultValue = 1.0f;
        paramQuantities[VCA_SHIFT_PARAM]->name = "VCA Shift";
        paramQuantities[VCA_SHIFT_PARAM]->snapEnabled = true;
        
        configParam(VCA_DECAY_PARAM, 0.01f, 2.0f, 0.54929012060165405f, "VCA Decay", " s");

        configParam(TRACK1_DECAY_PARAM, 0.01f, 2.0f, 0.30000001192092896f, "Track 1 Decay", " s");
        configParam(TRACK1_SHAPE_PARAM, 0.0f, 0.99f, 0.5f, "Track 1 Shape");
        
        configParam(TRACK2_FILL_PARAM, 0.0f, 100.0f, 100.0f, "Track 2 Fill", "%");
        configParam(TRACK2_DIVMULT_PARAM, -3.0f, 3.0f, -3.0f, "Track 2 Div/Mult");
        getParamQuantity(TRACK2_DIVMULT_PARAM)->snapEnabled = true;
        delete paramQuantities[TRACK2_DIVMULT_PARAM];
        paramQuantities[TRACK2_DIVMULT_PARAM] = new TWNCLightDivMultParamQuantity;
        paramQuantities[TRACK2_DIVMULT_PARAM]->module = this;
        paramQuantities[TRACK2_DIVMULT_PARAM]->paramId = TRACK2_DIVMULT_PARAM;
        paramQuantities[TRACK2_DIVMULT_PARAM]->minValue = -3.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->maxValue = 3.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->defaultValue = -3.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->name = "Track 2 Div/Mult";
        paramQuantities[TRACK2_DIVMULT_PARAM]->snapEnabled = true;
        
        configParam(TRACK2_DECAY_PARAM, 0.01f, 2.0f, 0.093579992651939392f, "Track 2 Decay", " s");
        configParam(TRACK2_SHAPE_PARAM, 0.0f, 0.99f, 0.5f, "Track 2 Shape");

        configParam(TRACK2_SHIFT_PARAM, 1.0f, 4.0f, 3.0f, "Track 2 Shift");
        getParamQuantity(TRACK2_SHIFT_PARAM)->snapEnabled = true;
        
        configOutput(MAIN_VCA_ENV_OUTPUT, "Accent VCA Envelope");
        configOutput(TRACK1_FM_ENV_OUTPUT, "Track 1 FM Envelope");
        configOutput(TRACK2_VCA_ENV_OUTPUT, "Track 2 VCA Envelope");
    }

    void onReset() override {
        secondsSinceLastClock = -1.0f;
        globalClockSeconds = 0.5f;
        globalClockCount = 0;
        for (int i = 0; i < 2; ++i) {
            tracks[i].reset();
            tracks[i].currentStep = 0;
        }
        quarterClock.reset();
        mainVCA.reset();
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
        bool globalClockActive = inputs[GLOBAL_CLOCK_INPUT].isConnected();
        bool globalClockTriggered = false;
        
        if (globalClockActive) {
            float clockVoltage = inputs[GLOBAL_CLOCK_INPUT].getVoltage();
            globalClockTriggered = clockTrigger.process(clockVoltage, 0.1f, 2.0f);
            
            if (globalClockTriggered) {
                globalClockCount++;
                if (globalClockCount >= 32) {
                    globalClockCount = 0;
                    for (int i = 0; i < 2; ++i) {
                        tracks[i].currentStep = 0;
                    }
                    quarterClock.currentStep = 0;
                }
                
                if (secondsSinceLastClock > 0.0f) {
                    globalClockSeconds = secondsSinceLastClock;
                    globalClockSeconds = clamp(globalClockSeconds, 0.01f, 10.0f);
                }
                secondsSinceLastClock = 0.0f;
            }
        }
        
        if (secondsSinceLastClock >= 0.0f) {
            secondsSinceLastClock += args.sampleTime;
        }

        int globalLength = (int)std::round(params[GLOBAL_LENGTH_PARAM].getValue());
        globalLength = clamp(globalLength, 1, 32);
        
        int vcaShift = (int)std::round(params[VCA_SHIFT_PARAM].getValue());
        bool vcaTriggered = quarterClock.processStep(globalClockTriggered, globalLength, vcaShift);
        float vcaTrigger = quarterClock.getTrigger(args.sampleTime);

        static int hatsDelayCounter = 0;
        static bool hatsDelayActive = false;
        static bool hatsStarted = false;
        static int lastVcaShift = -1;

        int currentHatsShift = (int)std::round(params[TRACK2_SHIFT_PARAM].getValue());
        
        if (currentHatsShift != lastVcaShift) {
            hatsStarted = false;
            hatsDelayActive = false;
            hatsDelayCounter = 0;
            lastVcaShift = currentHatsShift;
        }

        if (vcaTriggered && !hatsStarted) {
            if (currentHatsShift == 1) {
                hatsStarted = true;
                hatsDelayActive = false;
            } else {
                hatsDelayCounter = currentHatsShift - 1;
                hatsDelayActive = true;
            }
        }

        if (hatsDelayActive && globalClockTriggered) {
            hatsDelayCounter--;
            if (hatsDelayCounter <= 0) {
                hatsStarted = true;
                hatsDelayActive = false;
            }
        }

        bool hatsBaseClock = false;
        if (hatsStarted && globalClockActive) {
            hatsBaseClock = vcaTriggered || globalClockTriggered;
        }

        for (int i = 0; i < 2; ++i) {
            TrackState& track = tracks[i];
            
            if (i == 0) {
                track.updateDivMult(0);
            } else {
                int divMultParam = (int)std::round(params[TRACK2_DIVMULT_PARAM].getValue());
                track.updateDivMult(divMultParam);
            }

            track.length = globalLength;

            float fillParam = (i == 0) ? params[TRACK1_FILL_PARAM].getValue() : params[TRACK2_FILL_PARAM].getValue();
            float fillPercentage = clamp(fillParam, 0.0f, 100.0f);
            track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

            if (i == 0) {
                track.shift = 0;
            } else {
                float shiftParam = params[TRACK2_SHIFT_PARAM].getValue();
                track.shift = (int)std::round(clamp(shiftParam, 1.0f, 4.0f));
            }

            track.pattern = generateTWNCLightEuclideanRhythm(track.length, track.fill, 0);
            
            bool trackClockTrigger;
            if (i == 1) {
                trackClockTrigger = track.processClockDivMult(hatsBaseClock, globalClockSeconds, args.sampleTime);
            } else {
                trackClockTrigger = track.processClockDivMult(globalClockTriggered, globalClockSeconds, args.sampleTime);
            }

            if (trackClockTrigger && !track.pattern.empty() && globalClockActive) {
                track.stepTrack();
            }
            
            if (i == 0) {
                float decayParam = params[TRACK1_DECAY_PARAM].getValue();
                if (inputs[DRUM_DECAY_CV_INPUT].isConnected()) {
                    decayParam += inputs[DRUM_DECAY_CV_INPUT].getVoltage() / 10.0f;
                    decayParam = clamp(decayParam, 0.01f, 2.0f);
                }
                float shapeParam = params[TRACK1_SHAPE_PARAM].getValue();
                
                float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
                float envelopeOutput = track.envelope.process(args.sampleTime, triggerOutput, decayParam * 0.5f, shapeParam);
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam, shapeParam);
                
                float vcaDecayParam = params[VCA_DECAY_PARAM].getValue();
                float mainVCAOutput = mainVCA.process(args.sampleTime, vcaTrigger, vcaDecayParam, 0.5f);
                
                outputs[MAIN_VCA_ENV_OUTPUT].setVoltage(mainVCAOutput * 10.0f);
                outputs[TRACK1_FM_ENV_OUTPUT].setVoltage(envelopeOutput * 10.0f);
            } else {
                float decayParam = params[TRACK2_DECAY_PARAM].getValue();
                if (inputs[HATS_DECAY_CV_INPUT].isConnected()) {
                    decayParam += inputs[HATS_DECAY_CV_INPUT].getVoltage() / 10.0f;
                    decayParam = clamp(decayParam, 0.01f, 2.0f);
                }
                float shapeParam = params[TRACK2_SHAPE_PARAM].getValue();
                
                float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam * 0.5f, shapeParam);
                
                outputs[TRACK2_VCA_ENV_OUTPUT].setVoltage(vcaEnvelopeOutput * 10.0f);
            }
        }
    }
};

struct TWNCLightWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    TWNCLightWidget(TWNCLight* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP");
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TWNCLightEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "TWNC LTE", 10.f, nvgRGB(255, 200, 0), true));
        addChild(new TWNCLightEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 8.f, nvgRGB(255, 200, 0), false));

        addChild(new TWNCLightEnhancedTextLabel(Vec(5, 30), Vec(20, 15), "CLK", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 51), module, TWNCLight::GLOBAL_CLOCK_INPUT));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, 30), Vec(20, 15), "LEN", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::SnapKnob26>(Vec(45, 53), module, TWNCLight::GLOBAL_LENGTH_PARAM));

        float drumY = 71;
        addChild(new TWNCLightEnhancedTextLabel(Vec(20, drumY), Vec(20, 10), "Drum", 6.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, drumY + 12), Vec(20, 10), "ACCNT", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::SnapKnob26>(Vec(15, drumY + 33), module, TWNCLight::VCA_SHIFT_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, drumY + 12), Vec(20, 10), "SHAPE", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(45, drumY + 33), module, TWNCLight::TRACK1_SHAPE_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, drumY + 48), Vec(20, 10), "FILL", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(15, drumY + 69), module, TWNCLight::TRACK1_FILL_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, drumY + 48), Vec(20, 10), "A.DEC", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(45, drumY + 69), module, TWNCLight::VCA_DECAY_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, drumY + 84), Vec(20, 10), "DECAY", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(15, drumY + 105), module, TWNCLight::TRACK1_DECAY_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, drumY + 84), Vec(20, 10), "D.D", 5.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(45, drumY + 105), module, TWNCLight::DRUM_DECAY_CV_INPUT));

        float hatsY = 195;
        addChild(new TWNCLightEnhancedTextLabel(Vec(20, hatsY), Vec(20, 10), "HATs", 6.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, hatsY + 12), Vec(20, 10), "FILL", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(15, hatsY + 33), module, TWNCLight::TRACK2_FILL_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, hatsY + 12), Vec(20, 10), "SHIFT", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::SnapKnob26>(Vec(45, hatsY + 33), module, TWNCLight::TRACK2_SHIFT_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, hatsY + 48), Vec(20, 10), "D/M", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::SnapKnob26>(Vec(15, hatsY + 69), module, TWNCLight::TRACK2_DIVMULT_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, hatsY + 48), Vec(20, 10), "DECAY", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(45, hatsY + 69), module, TWNCLight::TRACK2_DECAY_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, hatsY + 84), Vec(20, 10), "SHAPE", 5.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::StandardBlackKnob26>(Vec(15, hatsY + 105), module, TWNCLight::TRACK2_SHAPE_PARAM));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(35, hatsY + 84), Vec(20, 10), "H.D", 5.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(45, hatsY + 105), module, TWNCLight::HATS_DECAY_CV_INPUT));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(60, 50)));
        
        addChild(new TWNCLightEnhancedTextLabel(Vec(5, 335), Vec(20, 20), "ENVs", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 343), module, TWNCLight::MAIN_VCA_ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 368), module, TWNCLight::TRACK1_FM_ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 368), module, TWNCLight::TRACK2_VCA_ENV_OUTPUT));
    }

    void step() override {
        TWNCLight* module = dynamic_cast<TWNCLight*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        TWNCLight* module = dynamic_cast<TWNCLight*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelTWNCLight = createModel<TWNCLight, TWNCLightWidget>("TWNCLight");