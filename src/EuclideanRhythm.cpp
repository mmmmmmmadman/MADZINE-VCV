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

struct StandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    StandardBlackKnob() {
        box.size = Vec(26, 26);
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

struct SnapKnob : ParamWidget {
    float accumDelta = 0.0f;
    
    SnapKnob() {
        box.size = Vec(26, 26);
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
        
        float threshold = 10.0f;
        
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
        TRACK2_DIVMULT_PARAM,
        TRACK2_LENGTH_PARAM,
        TRACK2_FILL_PARAM,
        TRACK2_SHIFT_PARAM,
        TRACK2_LENGTH_CV_ATTEN_PARAM,
        TRACK2_FILL_CV_ATTEN_PARAM,
        TRACK2_SHIFT_CV_ATTEN_PARAM,
        TRACK3_DIVMULT_PARAM,
        TRACK3_LENGTH_PARAM,
        TRACK3_FILL_PARAM,
        TRACK3_SHIFT_PARAM,
        TRACK3_LENGTH_CV_ATTEN_PARAM,
        TRACK3_FILL_CV_ATTEN_PARAM,
        TRACK3_SHIFT_CV_ATTEN_PARAM,
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
        TRACK1_TRIG_OUTPUT,
        TRACK2_TRIG_OUTPUT,
        TRACK3_TRIG_OUTPUT,
        MASTER_TRIG_OUTPUT,
        CHAIN_12_OUTPUT,
        CHAIN_23_OUTPUT,
        CHAIN_123_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        TRACK1_LIGHT,
        TRACK2_LIGHT,
        TRACK3_LIGHT,
        CHAIN_12_T1_LIGHT,
        CHAIN_12_T2_LIGHT,
        CHAIN_23_T2_LIGHT,
        CHAIN_23_T3_LIGHT,
        CHAIN_123_T1_LIGHT,
        CHAIN_123_T2_LIGHT,
        CHAIN_123_T3_LIGHT, 
        OR_RED_LIGHT,
        OR_GREEN_LIGHT,
        OR_BLUE_LIGHT,
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger manualResetTrigger;
    
    float globalClockSeconds = 0.5f;
    float secondsSinceLastClock = -1.0f;
    
    dsp::PulseGenerator orRedPulse;
    dsp::PulseGenerator orGreenPulse;
    dsp::PulseGenerator orBluePulse;

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
        bool cycleCompleted = false;
        dsp::PulseGenerator trigPulse;

        void reset() {
            dividedProgressSeconds = 0.0f;
            dividerCount = 0;
            shouldStep = false;
            prevMultipliedGate = false;
            currentStep = 0;
            pattern.clear();
            gateState = false;
            cycleCompleted = false;
        }
        
        void updateDivMult(int divMultParam) {
            divMultValue = divMultParam;
            if (divMultValue > 0) {
                division = 1;
                multiplication = divMultValue + 1;
            } else if (divMultValue < 0) {
                division = -divMultValue + 1;
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
            cycleCompleted = false;
            currentStep = (currentStep + 1) % length;
            if (currentStep == 0) {
                cycleCompleted = true;
            }
            gateState = !pattern.empty() && pattern[currentStep];
            if (gateState) {
                trigPulse.trigger(0.01f);
            }
        }
    };
    TrackState tracks[3];

    struct ChainedSequence {
        int currentTrackIndex = 0;
        std::vector<int> trackIndices;
        int globalClockCount = 0;
        int trackStartClock[3] = {0, 0, 0};
        
        ChainedSequence() {}
        
        void reset() {
            currentTrackIndex = 0;
            globalClockCount = 0;
            for (int i = 0; i < 3; ++i) {
                trackStartClock[i] = 0;
            }
        }
        
        int calculateTrackCycleClock(const TrackState& track) {
            return track.length * track.division / track.multiplication;
        }
        
        float processStep(TrackState tracks[], float sampleTime, bool globalClockTriggered) {
            if (trackIndices.empty()) {
                return 0.0f;
            }
            
            if (globalClockTriggered) {
                globalClockCount++;
            }
            
            if (currentTrackIndex >= (int)trackIndices.size()) {
                currentTrackIndex = 0;
            }
            
            int activeTrackIdx = trackIndices[currentTrackIndex];
            if (activeTrackIdx < 0 || activeTrackIdx >= 3) {
                return 0.0f;
            }
            
            TrackState& activeTrack = tracks[activeTrackIdx];
            int trackCycleClock = calculateTrackCycleClock(activeTrack);
            int elapsedClock = globalClockCount - trackStartClock[activeTrackIdx];
            
            if (elapsedClock >= trackCycleClock) {
                currentTrackIndex++;
                if (currentTrackIndex >= (int)trackIndices.size()) {
                    currentTrackIndex = 0;
                }
                activeTrackIdx = trackIndices[currentTrackIndex];
                trackStartClock[activeTrackIdx] = globalClockCount;
            }
            
            return tracks[activeTrackIdx].trigPulse.process(sampleTime) ? 10.0f : 0.0f;
        }
    };
    ChainedSequence chain12, chain23, chain123;

    EuclideanRhythm() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(GLOBAL_CLOCK_INPUT, "Global Clock");
        configInput(GLOBAL_RESET_INPUT, "Global Reset");
        configParam(MANUAL_RESET_PARAM, 0.0f, 1.0f, 0.0f, "Manual Reset");

        chain12.trackIndices = {0, 1};
        chain23.trackIndices = {1, 2};
        chain123.trackIndices = {0, 1, 0, 2};

        for (int i = 0; i < 3; ++i) {
            int paramBase = TRACK1_DIVMULT_PARAM + i * 7;
            int inputBase = TRACK1_LENGTH_CV_INPUT + i * 3;
            
            configParam(paramBase, -3.0f, 3.0f, 0.0f, string::f("T%d Div/Mult", i+1));
            getParamQuantity(paramBase)->snapEnabled = true;
            delete paramQuantities[paramBase];
            paramQuantities[paramBase] = new DivMultParamQuantity;
            paramQuantities[paramBase]->module = this;
            paramQuantities[paramBase]->paramId = paramBase;
            paramQuantities[paramBase]->minValue = -3.0f;
            paramQuantities[paramBase]->maxValue = 3.0f;
            paramQuantities[paramBase]->defaultValue = 0.0f;
            paramQuantities[paramBase]->name = string::f("T%d Div/Mult", i+1);
            paramQuantities[paramBase]->snapEnabled = true;
            
            configParam(paramBase + 1, 1.0f, 32.0f, 16.0f, string::f("T%d Length", i+1));
            getParamQuantity(paramBase + 1)->snapEnabled = true;
            configParam(paramBase + 2, 0.0f, 100.0f, 25.0f, string::f("T%d Fill", i+1), "%");
            configParam(paramBase + 3, 0.0f, 31.0f, 0.0f, string::f("T%d Shift", i+1));
            getParamQuantity(paramBase + 3)->snapEnabled = true;
            configParam(paramBase + 4, -1.0f, 1.0f, 0.0f, string::f("T%d Length CV", i+1));
            configParam(paramBase + 5, -1.0f, 1.0f, 0.0f, string::f("T%d Fill CV", i+1));
            configParam(paramBase + 6, -1.0f, 1.0f, 0.0f, string::f("T%d Shift CV", i+1));
            
            configInput(inputBase, string::f("T%d Length CV", i+1));
            configInput(inputBase + 1, string::f("T%d Fill CV", i+1));
            configInput(inputBase + 2, string::f("T%d Shift CV", i+1));
            configOutput(TRACK1_TRIG_OUTPUT + i, string::f("T%d Trigger", i+1));
            configLight(TRACK1_LIGHT + i, string::f("T%d Light", i+1));
        }
        
        configOutput(MASTER_TRIG_OUTPUT, "Master Trigger Sum");
        configOutput(CHAIN_12_OUTPUT, "Chain 1+2");
        configOutput(CHAIN_23_OUTPUT, "Chain 2+3");
        configOutput(CHAIN_123_OUTPUT, "Chain 1+2+3");
        
        configLight(OR_RED_LIGHT, "OR Red Light");
        configLight(OR_GREEN_LIGHT, "OR Green Light");
        configLight(OR_BLUE_LIGHT, "OR Blue Light");
    }

    void onReset() override {
        secondsSinceLastClock = -1.0f;
        globalClockSeconds = 0.5f;
        for (int i = 0; i < 3; ++i) {
            tracks[i].reset();
        }
        chain12.reset();
        chain23.reset();
        chain123.reset();
    }

    void process(const ProcessArgs& args) override {
        bool globalClockActive = inputs[GLOBAL_CLOCK_INPUT].isConnected();
        bool globalClockTriggered = false;
        bool globalResetTriggered = false;
        bool manualResetTriggered = false;
        
        if (globalClockActive) {
            float clockVoltage = inputs[GLOBAL_CLOCK_INPUT].getVoltage();
            globalClockTriggered = clockTrigger.process(clockVoltage);
        }
        
        if (inputs[GLOBAL_RESET_INPUT].isConnected()) {
            globalResetTriggered = resetTrigger.process(inputs[GLOBAL_RESET_INPUT].getVoltage());
        }
        
        manualResetTriggered = manualResetTrigger.process(params[MANUAL_RESET_PARAM].getValue());
        
        if (globalResetTriggered || manualResetTriggered) {
            onReset();
            return;
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

        for (int i = 0; i < 3; ++i) {
            TrackState& track = tracks[i];
            
            int divMultParam = (int)std::round(params[TRACK1_DIVMULT_PARAM + i * 7].getValue());
            track.updateDivMult(divMultParam);

            float lengthParam = params[TRACK1_LENGTH_PARAM + i * 7].getValue();
            float lengthCV = 0.0f;
            if (inputs[TRACK1_LENGTH_CV_INPUT + i * 3].isConnected()) {
                float lengthCVAtten = params[TRACK1_LENGTH_CV_ATTEN_PARAM + i * 7].getValue();
                lengthCV = inputs[TRACK1_LENGTH_CV_INPUT + i * 3].getVoltage() * lengthCVAtten;
            }
            track.length = (int)std::round(clamp(lengthParam + lengthCV, 1.0f, 32.0f));

            float fillParam = params[TRACK1_FILL_PARAM + i * 7].getValue();
            float fillCV = 0.0f;
            if (inputs[TRACK1_FILL_CV_INPUT + i * 3].isConnected()) {
                float fillCVAtten = params[TRACK1_FILL_CV_ATTEN_PARAM + i * 7].getValue();
                fillCV = inputs[TRACK1_FILL_CV_INPUT + i * 3].getVoltage() * fillCVAtten * 10.0f;
            }
            float fillPercentage = clamp(fillParam + fillCV, 0.0f, 100.0f);
            track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

            float shiftParam = params[TRACK1_SHIFT_PARAM + i * 7].getValue();
            float shiftCV = 0.0f;
            if (inputs[TRACK1_SHIFT_CV_INPUT + i * 3].isConnected()) {
                float shiftCVAtten = params[TRACK1_SHIFT_CV_ATTEN_PARAM + i * 7].getValue();
                shiftCV = inputs[TRACK1_SHIFT_CV_INPUT + i * 3].getVoltage() * shiftCVAtten;
            }
            track.shift = (int)std::round(clamp(shiftParam + shiftCV, 0.0f, (float)track.length - 1.0f));

            track.pattern = generateEuclideanRhythm(track.length, track.fill, track.shift);

            bool trackClockTrigger = track.processClockDivMult(globalClockTriggered, globalClockSeconds, args.sampleTime);

            if (trackClockTrigger && !track.pattern.empty() && globalClockActive) {
                track.stepTrack();
            }
            
            float trigOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
            outputs[TRACK1_TRIG_OUTPUT + i].setVoltage(trigOutput);
            
            lights[TRACK1_LIGHT + i].setBrightness(track.gateState ? 1.0f : 0.0f);
        }
        
        float masterTrigSum = 0.0f;
        for (int i = 0; i < 3; ++i) {
            if (outputs[TRACK1_TRIG_OUTPUT + i].getVoltage() > 0.0f) {
                masterTrigSum = 10.0f;
                break;
            }
        }
        outputs[MASTER_TRIG_OUTPUT].setVoltage(masterTrigSum);
        
        bool track1Active = outputs[TRACK1_TRIG_OUTPUT].getVoltage() > 0.0f;
        bool track2Active = outputs[TRACK2_TRIG_OUTPUT].getVoltage() > 0.0f;
        bool track3Active = outputs[TRACK3_TRIG_OUTPUT].getVoltage() > 0.0f;
        
        if (track1Active) {
            orRedPulse.trigger(0.03f);
        }
        if (track2Active) {
            orGreenPulse.trigger(0.03f);
        }
        if (track3Active) {
            orBluePulse.trigger(0.03f);
        }
        
        lights[OR_RED_LIGHT].setBrightness(orRedPulse.process(args.sampleTime) ? 1.0f : 0.0f);
        lights[OR_GREEN_LIGHT].setBrightness(orGreenPulse.process(args.sampleTime) ? 1.0f : 0.0f);
        lights[OR_BLUE_LIGHT].setBrightness(orBluePulse.process(args.sampleTime) ? 1.0f : 0.0f);
        
        if (globalClockActive) {
            float chain12Output = chain12.processStep(tracks, args.sampleTime, globalClockTriggered);
            outputs[CHAIN_12_OUTPUT].setVoltage(chain12Output);
            
            float chain23Output = chain23.processStep(tracks, args.sampleTime, globalClockTriggered);
            outputs[CHAIN_23_OUTPUT].setVoltage(chain23Output);
            
            float chain123Output = chain123.processStep(tracks, args.sampleTime, globalClockTriggered);
            outputs[CHAIN_123_OUTPUT].setVoltage(chain123Output);
            
            lights[CHAIN_12_T1_LIGHT].setBrightness(chain12.currentTrackIndex == 0 ? 1.0f : 0.0f);
            lights[CHAIN_12_T2_LIGHT].setBrightness(chain12.currentTrackIndex == 1 ? 1.0f : 0.0f);
            
            lights[CHAIN_23_T2_LIGHT].setBrightness(chain23.currentTrackIndex == 0 ? 1.0f : 0.0f);
            lights[CHAIN_23_T3_LIGHT].setBrightness(chain23.currentTrackIndex == 1 ? 1.0f : 0.0f);
            
            int activeTrack123 = chain123.trackIndices[chain123.currentTrackIndex];
            lights[CHAIN_123_T1_LIGHT].setBrightness(activeTrack123 == 0 ? 1.0f : 0.0f);
            lights[CHAIN_123_T2_LIGHT].setBrightness(activeTrack123 == 1 ? 1.0f : 0.0f);
            lights[CHAIN_123_T3_LIGHT].setBrightness(activeTrack123 == 2 ? 1.0f : 0.0f);
        }
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
            addParam(createParamCentered<SnapKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_LENGTH_PARAM + i * 7));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_LENGTH_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_LENGTH_CV_ATTEN_PARAM + i * 7));
            x += 31;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "FILL", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<StandardBlackKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_FILL_PARAM + i * 7));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_FILL_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_FILL_CV_ATTEN_PARAM + i * 7));
            x += 31;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "SHFT", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<SnapKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_SHIFT_PARAM + i * 7));
            addInput(createInputCentered<PJ301MPort>(Vec(x + 12, y + 47), module, EuclideanRhythm::TRACK1_SHIFT_CV_INPUT + i * 3));
            addParam(createParamCentered<Trimpot>(Vec(x + 12, y + 69), module, EuclideanRhythm::TRACK1_SHIFT_CV_ATTEN_PARAM + i * 7));
            x += 30;

            addChild(new EnhancedTextLabel(Vec(x, y), Vec(25, 10), "D/M", 7.f, nvgRGB(200, 200, 200), true));
            addParam(createParamCentered<SnapKnob>(Vec(x + 12, y + 22), module, EuclideanRhythm::TRACK1_DIVMULT_PARAM + i * 7));
        }
        
        for (int i = 0; i < 3; ++i) {
            float y = trackY[i];
            float outputX = 106;
            float outputY = y + 69;
            
            addChild(new EnhancedTextLabel(Vec(outputX - 12, outputY - 21), Vec(25, 10), "OUT " + std::to_string(i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addOutput(createOutputCentered<PJ301MPort>(Vec(outputX, outputY), module, EuclideanRhythm::TRACK1_TRIG_OUTPUT + i));
        }
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 325)));
        
        float chainOutputY = 358;
        float chainPositions[3] = {13, 44, 75};
        
        addChild(new EnhancedTextLabel(Vec(chainPositions[0] - 12, chainOutputY - 21), Vec(25, 10), "1+2", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(chainPositions[0], chainOutputY), module, EuclideanRhythm::CHAIN_12_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(chainPositions[0] - 8, chainOutputY + 17), module, EuclideanRhythm::CHAIN_12_T1_LIGHT));
        addChild(createLightCentered<SmallLight<GreenLight>>(Vec(chainPositions[0] + 8, chainOutputY + 17), module, EuclideanRhythm::CHAIN_12_T2_LIGHT));
        
        addChild(new EnhancedTextLabel(Vec(chainPositions[1] - 12, chainOutputY - 21), Vec(25, 10), "2+3", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(chainPositions[1], chainOutputY), module, EuclideanRhythm::CHAIN_23_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(Vec(chainPositions[1] - 8, chainOutputY + 17), module, EuclideanRhythm::CHAIN_23_T2_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(chainPositions[1] + 8, chainOutputY + 17), module, EuclideanRhythm::CHAIN_23_T3_LIGHT));
        
        addChild(new EnhancedTextLabel(Vec(chainPositions[2] - 12, chainOutputY - 21), Vec(25, 10), "1213", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(chainPositions[2], chainOutputY), module, EuclideanRhythm::CHAIN_123_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(chainPositions[2] - 10, chainOutputY + 17), module, EuclideanRhythm::CHAIN_123_T1_LIGHT));
        addChild(createLightCentered<SmallLight<GreenLight>>(Vec(chainPositions[2], chainOutputY + 17), module, EuclideanRhythm::CHAIN_123_T2_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(chainPositions[2] + 10, chainOutputY + 17), module, EuclideanRhythm::CHAIN_123_T3_LIGHT));
        
        float outputY = 358;
        float outputSpacing = 31;
        float startX = 13;
        
        float mixX = startX + 3 * outputSpacing - 2;
        addChild(new EnhancedTextLabel(Vec(mixX - 12, 337), Vec(25, 10), "OR", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixX, outputY), module, EuclideanRhythm::MASTER_TRIG_OUTPUT));
        addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(mixX + 8, outputY + 17), module, EuclideanRhythm::OR_RED_LIGHT));
    }
};

Model* modelEuclideanRhythm = createModel<EuclideanRhythm, EuclideanRhythmWidget>("EuclideanRhythm");