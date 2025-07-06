#include "plugin.hpp"
#include <vector>
#include <algorithm>

struct MADDYEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    MADDYEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f, 
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
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

struct MADDYStandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    MADDYStandardBlackKnob() {
        box.size = Vec(26, 26);
    }
    
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

struct MADDYSnapKnob : ParamWidget {
    float accumDelta = 0.0f;
    
    MADDYSnapKnob() {
        box.size = Vec(26, 26);
    }
    
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
        nvgFillColor(args.vg, nvgRGB(130, 130, 130));
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
        float threshold = 30.0f;
        
        if (accumDelta >= threshold) {
            float currentValue = pq->getValue();
            float newValue = clamp(currentValue + 1.0f, pq->getMinValue(), pq->getMaxValue());
            pq->setValue(newValue);
            accumDelta = 0.0f;
        }
        else if (accumDelta <= -threshold) {
            float currentValue = pq->getValue();
            float newValue = clamp(currentValue - 1.0f, pq->getMinValue(), pq->getMaxValue());
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

struct WhiteKnob : ParamWidget {
    bool isDragging = false;
    
    WhiteKnob() {
        box.size = Vec(30, 30);
    }
    
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
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 8;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 133, 133));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, nvgRGB(255, 133, 133));
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

struct SmallGrayKnob : ParamWidget {
    bool isDragging = false;
    
    SmallGrayKnob() {
        box.size = Vec(21, 21);
    }
    
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
        nvgCircle(args.vg, radius, radius, radius - 3);
        nvgFillColor(args.vg, nvgRGB(180, 180, 180));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 6;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 1.5f);
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

struct MediumGrayKnob : ParamWidget {
    bool isDragging = false;
    
    MediumGrayKnob() {
        box.size = Vec(26, 26);
    }
    
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
        nvgFillColor(args.vg, nvgRGB(130, 130, 130));
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
        
        float sensitivity = 0.008f;
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

struct SectionBox : Widget {
    SectionBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 150));
        nvgStroke(args.vg);
    }
};

struct VerticalLine : Widget {
    VerticalLine(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, box.size.x / 2.0f, 0);
        nvgLineTo(args.vg, box.size.x / 2.0f, box.size.y);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 150));
        nvgStroke(args.vg);
    }
};

struct HorizontalLine : Widget {
    HorizontalLine(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, box.size.y / 2.0f);
        nvgLineTo(args.vg, box.size.x, box.size.y / 2.0f);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 150));
        nvgStroke(args.vg);
    }
};

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
            steps = 28 + (int)((value - 0.6f) * 50.1f);
            primaryKnobs = 5;
        }
        steps = clamp(steps, 8, 48);
        
        return string::f("%d knobs, %d steps", primaryKnobs, steps);
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

std::vector<bool> generateMADDYEuclideanRhythm(int length, int fill, int shift) {
    std::vector<bool> pattern(length, false);
    if (fill == 0 || length == 0) return pattern;
    if (fill > length) fill = length;

    for (int i = 0; i < fill; ++i) {
        int index = (int)std::floor((float)i * length / fill);
        pattern[index] = true;
    }
    
    std::rotate(pattern.begin(), pattern.begin() + shift, pattern.end());
    return pattern;
}

struct MADDY : Module {
    enum ParamId {
        FREQ_PARAM,
        SWING_PARAM,
        LENGTH_PARAM,
        DECAY_PARAM,
        TRACK1_FILL_PARAM,
        TRACK1_DIVMULT_PARAM,
        TRACK2_FILL_PARAM,
        TRACK2_DIVMULT_PARAM,
        TRACK3_FILL_PARAM,
        TRACK3_DIVMULT_PARAM,
        K1_PARAM, K2_PARAM, K3_PARAM, K4_PARAM, K5_PARAM,
        MODE_PARAM, DENSITY_PARAM, CHAOS_PARAM, CLOCK_SOURCE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        RESET_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        CLK_OUTPUT,
        TRACK1_OUTPUT,
        TRACK2_OUTPUT,
        TRACK3_OUTPUT,
        CHAIN_12_OUTPUT,
        CHAIN_23_OUTPUT,
        CHAIN_123_OUTPUT,
        CV_OUTPUT,
        TRIG_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        MODE_LIGHT_RED,
        MODE_LIGHT_GREEN,
        MODE_LIGHT_BLUE,
        CLOCK_SOURCE_LIGHT_RED,
        CLOCK_SOURCE_LIGHT_GREEN,
        CLOCK_SOURCE_LIGHT_BLUE,
        LIGHTS_LEN
    };

    float phase = 0.0f;
    float swingPhase = 0.0f;
    float prevResetTrigger = 0.0f;
    dsp::PulseGenerator clockPulse;
    bool isSwingBeat = false;
    
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
        dsp::PulseGenerator patternTrigPulse;
        
        enum Phase {
            IDLE,
            ATTACK,
            DECAY
        };
        
        Phase envelopePhase = IDLE;
        float envelopeOutput = 0.0f;
        float envelopePhaseTime = 0.0f;
        float attackTime = 0.006f;
        float decayTime = 1.0f;
        float curve = 0.0f;
        float lastDecayParam = -1.0f;
        float currentDecayTime = 1.0f;
        float lastUsedDecayParam = 0.3f;
        bool justTriggered = false;

        void reset() {
            dividedProgressSeconds = 0.0f;
            dividerCount = 0;
            shouldStep = false;
            prevMultipliedGate = false;
            currentStep = 0;
            shift = 0;
            pattern.clear();
            gateState = false;
            envelopePhase = IDLE;
            envelopeOutput = 0.0f;
            envelopePhaseTime = 0.0f;
            lastDecayParam = -1.0f;
            currentDecayTime = 1.0f;
            lastUsedDecayParam = 0.3f;  
            justTriggered = false;
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
               currentStep = (currentStep + 1) % length;
               gateState = !pattern.empty() && pattern[currentStep];
               if (gateState) {
                  trigPulse.trigger(0.001f);
                  envelopePhase = ATTACK;
                  envelopePhaseTime = 0.0f;
                  justTriggered = true;
                }
        }
        
       float processEnvelope(float sampleTime, float decayParam) {
    if (envelopePhase == ATTACK && envelopePhaseTime == 0.0f) {
        float sqrtDecay = std::pow(decayParam, 0.33f);
        float mappedDecay = rescale(sqrtDecay, 0.0f, 1.0f, 0.0f, 0.8f);
        curve = rescale(decayParam, 0.0f, 1.0f, -0.8f, -0.45f);
        currentDecayTime = std::pow(10.0f, (mappedDecay - 0.8f) * 5.0f);
        currentDecayTime = std::max(0.01f, currentDecayTime);
        lastUsedDecayParam = decayParam;
    }
    
    switch (envelopePhase) {
        case IDLE:
            envelopeOutput = 0.0f;
            break;
            
        case ATTACK:
            envelopePhaseTime += sampleTime;
            if (envelopePhaseTime >= attackTime) {
                envelopePhase = DECAY;
                envelopePhaseTime = 0.0f;
                envelopeOutput = 1.0f;
            } else {
                float t = envelopePhaseTime / attackTime;
                envelopeOutput = applyCurve(t, curve);
            }
            break;
            
        case DECAY:
            envelopePhaseTime += sampleTime;
            if (envelopePhaseTime >= currentDecayTime) {
                envelopeOutput = 0.0f;
                envelopePhase = IDLE;
                envelopePhaseTime = 0.0f;
            } else {
                float t = envelopePhaseTime / currentDecayTime;
                envelopeOutput = 1.0f - applyCurve(t, curve);
            }
            break;
    }
    
    envelopeOutput = clamp(envelopeOutput, 0.0f, 1.0f);
    return envelopeOutput * 10.0f;
}
    };
    TrackState tracks[3];

    struct ChainedSequence {
        int currentTrackIndex = 0;
        std::vector<int> trackIndices;
        int globalClockCount = 0;
        int trackStartClock[3] = {0, 0, 0};
        dsp::PulseGenerator chainTrigPulse;
        
        void reset() {
            currentTrackIndex = 0;
            globalClockCount = 0;
            for (int i = 0; i < 3; ++i) {
                trackStartClock[i] = 0;
            }
            chainTrigPulse.reset();
        }
        
        int calculateTrackCycleClock(const TrackState& track) {
            return track.length * track.division / track.multiplication;
        }
        
        float processStep(TrackState tracks[], float sampleTime, bool globalClockTriggered, float decayParam, bool& chainTrigger) {
            chainTrigger = false;
            if (trackIndices.empty()) return 0.0f;
            
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
                chainTrigger = true;
                chainTrigPulse.trigger(0.001f);
            }
            
            
            return tracks[activeTrackIdx].envelopeOutput * 10.0f;
        }
    };
    ChainedSequence chain12, chain23, chain123;

    float globalClockSeconds = 0.5f;
    bool internalClockTriggered = false;
    bool patternClockTriggered = false;
    
    dsp::SchmittTrigger modeTrigger;
    dsp::SchmittTrigger clockSourceTrigger;
    dsp::PulseGenerator gateOutPulse;
    
    int currentStep = 0, sequenceLength = 16, stepToKnobMapping[64];
    float previousVoltage = -999.0f;
    int modeValue = 1;
    int clockSourceValue = 0;

    MADDY() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(FREQ_PARAM, -3.0f, 7.0f, 1.0f, "Frequency", " Hz", 2.0f, 1.0f);
        configParam(SWING_PARAM, 0.0f, 1.0f, 0.0f, "Swing", "°", 0.0f, -90.0f, 180.0f);
        configParam(LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "Length");
        getParamQuantity(LENGTH_PARAM)->snapEnabled = true;
        configParam(DECAY_PARAM, 0.0f, 1.0f, 0.3f, "Decay");
        
        configParam(K1_PARAM, -10.0f, 10.0f, 0.0f, "K1", "V");
        configParam(K2_PARAM, -10.0f, 10.0f, 2.0f, "K2", "V");
        configParam(K3_PARAM, -10.0f, 10.0f, 4.0f, "K3", "V");
        configParam(K4_PARAM, -10.0f, 10.0f, 6.0f, "K4", "V");
        configParam(K5_PARAM, -10.0f, 10.0f, 8.0f, "K5", "V");
        
        configParam(MODE_PARAM, 0.0f, 2.0f, 1.0f, "Mode");
        getParamQuantity(MODE_PARAM)->snapEnabled = true;
        configParam(DENSITY_PARAM, 0.0f, 1.0f, 0.5f, "Density");
        delete paramQuantities[DENSITY_PARAM];
        DensityParamQuantity* densityQuantity = new DensityParamQuantity;
        densityQuantity->module = this;
        densityQuantity->paramId = DENSITY_PARAM;
        densityQuantity->minValue = 0.0f;
        densityQuantity->maxValue = 1.0f;
        densityQuantity->defaultValue = 0.5f;
        densityQuantity->name = "Density";
        paramQuantities[DENSITY_PARAM] = densityQuantity;
        configParam(CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Chaos", "%", 0.f, 100.f);
        configParam(CLOCK_SOURCE_PARAM, 0.0f, 6.0f, 0.0f, "Clock Source");
        getParamQuantity(CLOCK_SOURCE_PARAM)->snapEnabled = true;
        
        for (int i = 0; i < 3; ++i) {
            configParam(TRACK1_FILL_PARAM + i * 2, 0.0f, 100.0f, 25.0f, string::f("T%d Fill", i+1), "%");
            configParam(TRACK1_DIVMULT_PARAM + i * 2, -3.0f, 3.0f, 0.0f, string::f("T%d Div/Mult", i+1));
            getParamQuantity(TRACK1_DIVMULT_PARAM + i * 2)->snapEnabled = true;
            delete paramQuantities[TRACK1_DIVMULT_PARAM + i * 2];
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2] = new DivMultParamQuantity;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->module = this;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->paramId = TRACK1_DIVMULT_PARAM + i * 2;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->minValue = -3.0f;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->maxValue = 3.0f;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->defaultValue = 0.0f;
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->name = string::f("T%d Div/Mult", i+1);
            paramQuantities[TRACK1_DIVMULT_PARAM + i * 2]->snapEnabled = true;
            
            configOutput(TRACK1_OUTPUT + i, string::f("T%d Trigger", i+1));
        }
        
        configInput(RESET_INPUT, "Reset");
        configOutput(CLK_OUTPUT, "Clock");
        configOutput(CHAIN_12_OUTPUT, "Chain 1+2");
        configOutput(CHAIN_23_OUTPUT, "Chain 2+3");
        configOutput(CHAIN_123_OUTPUT, "Chain 1+2+3");
        configOutput(CV_OUTPUT, "CV");
        configOutput(TRIG_OUTPUT, "Trigger");
        
        configLight(MODE_LIGHT_RED, "Mode Red");
        configLight(MODE_LIGHT_GREEN, "Mode Green");
        configLight(MODE_LIGHT_BLUE, "Mode Blue");
        configLight(CLOCK_SOURCE_LIGHT_RED, "Clock Source Red");
        configLight(CLOCK_SOURCE_LIGHT_GREEN, "Clock Source Green");
        configLight(CLOCK_SOURCE_LIGHT_BLUE, "Clock Source Blue");

        chain12.trackIndices = {0, 1};
        chain23.trackIndices = {1, 2};
        chain123.trackIndices = {0, 1, 0, 2};
        
        generateMapping();
    }

    void generateMapping() {
        float density = params[DENSITY_PARAM].getValue();
        float chaos = params[CHAOS_PARAM].getValue();
        
        if (density < 0.2f) {
            sequenceLength = 8 + (int)(density * 20);
        } else if (density < 0.4f) {
            sequenceLength = 12 + (int)((density - 0.2f) * 40);
        } else if (density < 0.6f) {
            sequenceLength = 20 + (int)((density - 0.4f) * 40);
        } else {
            sequenceLength = 28 + (int)((density - 0.6f) * 50.1f);
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
        
        switch (modeValue) {
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
        
        if (chaos > 0.3f) {
            int chaosSteps = (int)(chaos * sequenceLength * 0.3f);
            for (int i = 0; i < chaosSteps; i++) {
                int randomStep = random::u32() % sequenceLength;
                stepToKnobMapping[randomStep] = random::u32() % 5;
            }
        }
    }

    void onReset() override {
        phase = 0.0f;
        swingPhase = 0.0f;
        isSwingBeat = false;
        globalClockSeconds = 0.5f;
        for (int i = 0; i < 3; ++i) {
            tracks[i].reset();
        }
        chain12.reset();
        chain23.reset();
        chain123.reset();
        
        currentStep = 0;
        generateMapping();
        previousVoltage = -999.0f;
    }

json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "modeValue", json_integer(modeValue));
        json_object_set_new(rootJ, "clockSourceValue", json_integer(clockSourceValue));
        
        // 儲存所有軌道的攻擊時間
        json_t* attackTimesJ = json_array();
        for (int i = 0; i < 3; ++i) {
            json_array_append_new(attackTimesJ, json_real(tracks[i].attackTime));
        }
        json_object_set_new(rootJ, "attackTimes", attackTimesJ);

	// 儲存所有軌道的 shift 設定
	json_t* shiftsJ = json_array();
	for (int i = 0; i < 3; ++i) {
   	 json_array_append_new(shiftsJ, json_integer(tracks[i].shift));
	}
	json_object_set_new(rootJ, "shifts", shiftsJ);
        
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
   	 json_t* modeJ = json_object_get(rootJ, "modeValue");
	    if (modeJ) {
	        modeValue = json_integer_value(modeJ);
	        params[MODE_PARAM].setValue((float)modeValue);
    	}
    
    	json_t* clockSourceJ = json_object_get(rootJ, "clockSourceValue");
    	if (clockSourceJ) {
        	clockSourceValue = json_integer_value(clockSourceJ);
        	params[CLOCK_SOURCE_PARAM].setValue((float)clockSourceValue);
    	}
    
    	json_t* attackTimesJ = json_object_get(rootJ, "attackTimes");
    	if (attackTimesJ) {
        	for (int i = 0; i < 3; ++i) {
            	json_t* attackTimeJ = json_array_get(attackTimesJ, i);
            	if (attackTimeJ) {
                	tracks[i].attackTime = json_real_value(attackTimeJ);
            		}
        	}
    	}
    
    	json_t* shiftsJ = json_object_get(rootJ, "shifts");
    	if (shiftsJ) {
        	for (int i = 0; i < 3; ++i) {
            	json_t* shiftJ = json_array_get(shiftsJ, i);
            	if (shiftJ) {
                	tracks[i].shift = json_integer_value(shiftJ);
            		}
        	}
            }
	}

    void process(const ProcessArgs& args) override {
        float freqParam = params[FREQ_PARAM].getValue();
        float freq = std::pow(2.0f, freqParam) * 1.0f;
        
        float swingParam = params[SWING_PARAM].getValue();
        float swing = clamp(swingParam, 0.0f, 1.0f);
        
        float resetTrigger = inputs[RESET_INPUT].getVoltage();
        if (resetTrigger >= 2.0f && prevResetTrigger < 2.0f) {
            onReset();
        }
        prevResetTrigger = resetTrigger;
        
        float deltaPhase = freq * args.sampleTime;
        phase += deltaPhase;
        internalClockTriggered = false;
        
        float phaseThreshold = 1.0f;
        if (isSwingBeat && swing > 0.0f) {
            float swingOffset = swing * 0.25f;
            phaseThreshold = 1.0f + swingOffset;
        }
        
        if (phase >= phaseThreshold) {
            phase -= phaseThreshold;
            clockPulse.trigger(0.001f);
            internalClockTriggered = true;
            globalClockSeconds = phaseThreshold / freq;
            isSwingBeat = !isSwingBeat;
        }
        
        float clockOutput = clockPulse.process(args.sampleTime) ? 10.0f : 0.0f;
        outputs[CLK_OUTPUT].setVoltage(clockOutput);

        int globalLength = (int)std::round(params[LENGTH_PARAM].getValue());
        globalLength = clamp(globalLength, 1, 32);
        
        float decayParam = params[DECAY_PARAM].getValue();

        for (int i = 0; i < 3; ++i) {
            TrackState& track = tracks[i];
            
            int divMultParam = (int)std::round(params[TRACK1_DIVMULT_PARAM + i * 2].getValue());
            track.updateDivMult(divMultParam);

            track.length = globalLength;

            float fillParam = params[TRACK1_FILL_PARAM + i * 2].getValue();
            float fillPercentage = clamp(fillParam, 0.0f, 100.0f);
            track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

            track.pattern = generateMADDYEuclideanRhythm(track.length, track.fill, track.shift);

            bool trackClockTrigger = track.processClockDivMult(internalClockTriggered, globalClockSeconds, args.sampleTime);

            if (trackClockTrigger && !track.pattern.empty()) {
                track.stepTrack();
            }
            
            float envelopeOutput = track.processEnvelope(args.sampleTime, decayParam);
            outputs[TRACK1_OUTPUT + i].setVoltage(envelopeOutput);
        }
        
        bool chain12Trigger, chain23Trigger, chain123Trigger;
        float chain12Output = chain12.processStep(tracks, args.sampleTime, internalClockTriggered, decayParam, chain12Trigger);
        outputs[CHAIN_12_OUTPUT].setVoltage(chain12Output);
        
        float chain23Output = chain23.processStep(tracks, args.sampleTime, internalClockTriggered, decayParam, chain23Trigger);
        outputs[CHAIN_23_OUTPUT].setVoltage(chain23Output);
        
        float chain123Output = chain123.processStep(tracks, args.sampleTime, internalClockTriggered, decayParam, chain123Trigger);
        outputs[CHAIN_123_OUTPUT].setVoltage(chain123Output);
        
        if (modeTrigger.process(params[MODE_PARAM].getValue())) {
            modeValue = (modeValue + 1) % 3;
            params[MODE_PARAM].setValue((float)modeValue);
            generateMapping();
        }
        
        if (clockSourceTrigger.process(params[CLOCK_SOURCE_PARAM].getValue())) {
            clockSourceValue = (clockSourceValue + 1) % 7;
            params[CLOCK_SOURCE_PARAM].setValue((float)clockSourceValue);
        }
        
        lights[CLOCK_SOURCE_LIGHT_RED].setBrightness(0.0f);
        lights[CLOCK_SOURCE_LIGHT_GREEN].setBrightness(0.0f);
        lights[CLOCK_SOURCE_LIGHT_BLUE].setBrightness(0.0f);
        
        switch (clockSourceValue) {
            case 0:
                lights[CLOCK_SOURCE_LIGHT_RED].setBrightness(1.0f);
                break;
            case 1:
                lights[CLOCK_SOURCE_LIGHT_GREEN].setBrightness(1.0f);
                break;
            case 2:
                lights[CLOCK_SOURCE_LIGHT_BLUE].setBrightness(1.0f);
                break;
            case 3:
                lights[CLOCK_SOURCE_LIGHT_RED].setBrightness(1.0f);
                lights[CLOCK_SOURCE_LIGHT_GREEN].setBrightness(1.0f);
                break;
            case 4:
                lights[CLOCK_SOURCE_LIGHT_RED].setBrightness(1.0f);
                lights[CLOCK_SOURCE_LIGHT_BLUE].setBrightness(1.0f);
                break;
            case 5:
                lights[CLOCK_SOURCE_LIGHT_GREEN].setBrightness(1.0f);
                lights[CLOCK_SOURCE_LIGHT_BLUE].setBrightness(1.0f);
                break;
            case 6:
                lights[CLOCK_SOURCE_LIGHT_RED].setBrightness(1.0f);
                lights[CLOCK_SOURCE_LIGHT_GREEN].setBrightness(1.0f);
                lights[CLOCK_SOURCE_LIGHT_BLUE].setBrightness(1.0f);
                break;
        }
        
        patternClockTriggered = false;
        switch (clockSourceValue) {
            case 0:
                patternClockTriggered = internalClockTriggered;
                break;
            case 1:
                patternClockTriggered = tracks[0].justTriggered;
                tracks[0].justTriggered = false;
                break;
            case 2:
                patternClockTriggered = tracks[1].justTriggered;
                tracks[1].justTriggered = false;
                break;
            case 3:
                patternClockTriggered = tracks[2].justTriggered;
                tracks[2].justTriggered = false;
                break;
            case 4:
                patternClockTriggered = chain12Trigger;
                break;
            case 5:
                patternClockTriggered = chain23Trigger;
                break;
            case 6:
                patternClockTriggered = chain123Trigger;
                break;
        }
        
        lights[MODE_LIGHT_RED].setBrightness(modeValue == 0 ? 1.0f : 0.0f);
        lights[MODE_LIGHT_GREEN].setBrightness(modeValue == 1 ? 1.0f : 0.0f);
        lights[MODE_LIGHT_BLUE].setBrightness(modeValue == 2 ? 1.0f : 0.0f);
        
        if (patternClockTriggered) {
            currentStep = (currentStep + 1) % sequenceLength;
            generateMapping();
            
            int newActiveKnob = stepToKnobMapping[currentStep];
            float newVoltage = params[K1_PARAM + newActiveKnob].getValue();
            
            if (newVoltage != previousVoltage) gateOutPulse.trigger(0.01f);
            previousVoltage = newVoltage;
        }
        
        int activeKnob = stepToKnobMapping[currentStep];
        outputs[CV_OUTPUT].setVoltage(params[K1_PARAM + activeKnob].getValue());
        outputs[TRIG_OUTPUT].setVoltage(gateOutPulse.process(args.sampleTime) ? 10.0f : 0.0f);
    }
};

struct DynamicTextLabel : TransparentWidget {
    Module* module;
    int paramId;
    std::vector<std::string> textOptions;
    float fontSize;
    NVGcolor color;
    
    DynamicTextLabel(Vec pos, Vec size, Module* module, int paramId, 
                     std::vector<std::string> options, float fontSize = 8.f, 
                     NVGcolor color = nvgRGB(255, 255, 255)) {
        box.pos = pos;
        box.size = size;
        this->module = module;
        this->paramId = paramId;
        this->textOptions = options;
        this->fontSize = fontSize;
        this->color = color;
    }
    
    void draw(const DrawArgs &args) override {
        if (!module) return;
        
        MADDY* maddyModule = dynamic_cast<MADDY*>(module);
        if (!maddyModule) return;
        
        int currentValue = maddyModule->clockSourceValue;
        currentValue = clamp(currentValue, 0, (int)textOptions.size() - 1);
        
        std::string currentText = textOptions[currentValue];
        
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);
        
        float offset = 0.3f;
        nvgText(args.vg, box.size.x / 2.f - offset, box.size.y / 2.f, currentText.c_str(), NULL);
        nvgText(args.vg, box.size.x / 2.f + offset, box.size.y / 2.f, currentText.c_str(), NULL);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f - offset, currentText.c_str(), NULL);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f + offset, currentText.c_str(), NULL);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, currentText.c_str(), NULL);
    }
};

struct ModeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        MADDY* module = dynamic_cast<MADDY*>(this->module);
        if (!module) return "Minimalism";
        
        switch (module->modeValue) {
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

struct ClockSourceParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        MADDY* module = dynamic_cast<MADDY*>(this->module);
        if (!module) return "LFO";
        
        switch (module->clockSourceValue) {
            case 0: return "LFO";
            case 1: return "T1";
            case 2: return "T2";
            case 3: return "T3";
            case 4: return "12";
            case 5: return "23";
            case 6: return "1213";
            default: return "LFO";
        }
    }
    
    std::string getLabel() override {
        return "Clock Source";
    }
};

struct MADDYWidget : ModuleWidget {
    MADDYWidget(MADDY* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        addChild(new MADDYEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "M A D D Y", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new MADDYEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        addChild(new MADDYEnhancedTextLabel(Vec(48, 28), Vec(25, 15), "RST", 7.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, 52), module, MADDY::RESET_INPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 28), Vec(25, 15), "FREQ", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<SmallGrayKnob>(Vec(98, 52), module, MADDY::FREQ_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(48, 61), Vec(25, 15), "SWING", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<SmallGrayKnob>(Vec(60, 85), module, MADDY::SWING_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 61), Vec(25, 15), "CLK", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 85), module, MADDY::CLK_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(8, 28), Vec(25, 15), "LEN", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<MediumGrayKnob>(Vec(20, 52), module, MADDY::LENGTH_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(8, 61), Vec(25, 15), "DECAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<MediumGrayKnob>(Vec(20, 85), module, MADDY::DECAY_PARAM));
        
        addChild(new VerticalLine(Vec(39, 55), Vec(1, 242)));
        addChild(new HorizontalLine(Vec(40, 96), Vec(40, 1)));
        
        float trackY[3] = {107, 183, 259};
        
        for (int i = 0; i < 3; ++i) {
            float y = trackY[i];
            
            addChild(new MADDYEnhancedTextLabel(Vec(8, y - 10), Vec(25, 10), string::f("T%d", i+1), 7.f, nvgRGB(255, 200, 100), true));
            
            addChild(new MADDYEnhancedTextLabel(Vec(8, y), Vec(25, 10), "FILL", 6.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<MediumGrayKnob>(Vec(20, y + 20), module, MADDY::TRACK1_FILL_PARAM + i * 2));
            
            addChild(new MADDYEnhancedTextLabel(Vec(8, y + 33), Vec(25, 10), "D/M", 6.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<MADDYSnapKnob>(Vec(20, y + 53), module, MADDY::TRACK1_DIVMULT_PARAM + i * 2));
        }
        
        float cvY[5] = {127, 172, 217, 262, 307};
        for (int i = 0; i < 5; ++i) {
            addChild(new MADDYEnhancedTextLabel(Vec(40, cvY[i] - 30), Vec(40, 10), string::f("Step %d", i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addChild(new MADDYEnhancedTextLabel(Vec(48, cvY[i] - 15), Vec(25, 10), std::to_string(i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<WhiteKnob>(Vec(60, cvY[i] - 5), module, MADDY::K1_PARAM + i));
        }
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 97), Vec(25, 10), "MODE", 7.f, nvgRGB(255, 255, 255), true));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(98, 116), module, MADDY::MODE_LIGHT_RED));
        addParam(createParamCentered<VCVButton>(Vec(98, 116), module, MADDY::MODE_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 130), Vec(25, 10), "DENSITY", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<WhiteKnob>(Vec(98, 154), module, MADDY::DENSITY_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 170), Vec(25, 10), "CHAOS", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<WhiteKnob>(Vec(98, 194), module, MADDY::CHAOS_PARAM));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 210), Vec(25, 10), "CV OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 234), module, MADDY::CV_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 250), Vec(25, 10), "TRIG OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 274), module, MADDY::TRIG_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(86, 290), Vec(25, 10), "CLK SRC", 6.f, nvgRGB(255, 255, 255), true));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(98, 308), module, MADDY::CLOCK_SOURCE_LIGHT_RED));
        addParam(createParamCentered<VCVButton>(Vec(98, 308), module, MADDY::CLOCK_SOURCE_PARAM));
        
        std::vector<std::string> clockSourceTexts = {"LFO", "T1", "T2", "T3", "12", "23", "1213"};
        addChild(new DynamicTextLabel(Vec(86, 317), Vec(25, 10), module, MADDY::CLOCK_SOURCE_PARAM, clockSourceTexts, 7.f, nvgRGB(255, 255, 255)));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addChild(new MADDYEnhancedTextLabel(Vec(-2, 337), Vec(20, 15), "T1", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 343), module, MADDY::TRACK1_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(-2, 362), Vec(20, 15), "12", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 368), module, MADDY::CHAIN_12_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(38, 337), Vec(20, 15), "T2", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 343), module, MADDY::TRACK2_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(38, 362), Vec(20, 15), "23", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 368), module, MADDY::CHAIN_23_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(75, 337), Vec(20, 15), "T3", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 343), module, MADDY::TRACK3_OUTPUT));
        
        addChild(new MADDYEnhancedTextLabel(Vec(75, 365), Vec(20, 6), "12", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new MADDYEnhancedTextLabel(Vec(75, 371), Vec(20, 6), "13", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 368), module, MADDY::CHAIN_123_OUTPUT));
        
        if (module) {
            delete module->paramQuantities[MADDY::MODE_PARAM];
            ModeParamQuantity* modeQuantity = new ModeParamQuantity;
            modeQuantity->module = module;
            modeQuantity->paramId = MADDY::MODE_PARAM;
            modeQuantity->minValue = 0.0f;
            modeQuantity->maxValue = 2.0f;
            modeQuantity->defaultValue = 1.0f;
            modeQuantity->name = "Mode";
            modeQuantity->snapEnabled = true;
            module->paramQuantities[MADDY::MODE_PARAM] = modeQuantity;

            delete module->paramQuantities[MADDY::CLOCK_SOURCE_PARAM];
            ClockSourceParamQuantity* clockSourceQuantity = new ClockSourceParamQuantity;
            clockSourceQuantity->module = module;
            clockSourceQuantity->paramId = MADDY::CLOCK_SOURCE_PARAM;
            clockSourceQuantity->minValue = 0.0f;
            clockSourceQuantity->maxValue = 6.0f;
            clockSourceQuantity->defaultValue = 0.0f;
            clockSourceQuantity->name = "Clock Source";
            clockSourceQuantity->snapEnabled = true;
            module->paramQuantities[MADDY::CLOCK_SOURCE_PARAM] = clockSourceQuantity;
          }
    }

    struct AttackTimeItem : ui::MenuItem {
        MADDY* module;
        float attackTime;
        
        void onAction(const event::Action& e) override {
            if (module) {
                for (int i = 0; i < 3; ++i) {
                    module->tracks[i].attackTime = attackTime;
                }
            }
        }
    };

    void appendContextMenu(Menu* menu) override {
        MADDY* module = getModule<MADDY>();
        if (!module) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Attack Time"));
    
        float currentAttackTime = module->tracks[0].attackTime;
        std::string currentLabel = string::f("Current: %.3fms", currentAttackTime * 1000.0f);
        menu->addChild(createMenuLabel(currentLabel));

        struct AttackTimeChoice {
            std::string name;
            float value;
        };

        std::vector<AttackTimeChoice> choices = {
            {"0.5ms", 0.0005f},
            {"1ms", 0.001f},
            {"2ms", 0.002f},
            {"3ms", 0.003f},
            {"4ms", 0.004f},
            {"5ms", 0.005f},
            {"6ms (Default)", 0.006f},
            {"8ms", 0.008f},
            {"10ms", 0.010f},
            {"15ms", 0.015f},
            {"20ms", 0.020f}
        };

    struct AttackTimeSlider : ui::Slider {
        struct AttackTimeQuantity : Quantity {
            MADDY* module;
            AttackTimeQuantity(MADDY* module) : module(module) {}
        
            void setValue(float value) override {
                    if (module) {
                            value = clamp(value, 0.0f, 1.0f);
                            float attackTime = rescale(value, 0.0f, 1.0f, 0.0005f, 0.020f);
                            for (int i = 0; i < 3; ++i) {
                                module->tracks[i].attackTime = attackTime;
                            }
                    }
            }
        
            float getValue() override {
                if (module) {
                      return rescale(module->tracks[0].attackTime, 0.0005f, 0.020f, 0.0f, 1.0f);
                }
                return 0.3f;
            }
        
            float getMinValue() override { return 0.0f; }
            float getMaxValue() override { return 1.0f; }            
            float getDefaultValue() override { return 0.275f; }
            std::string getLabel() override { return "Attack Time"; }
            std::string getUnit() override { return " ms"; }
            std::string getDisplayValueString() override {
                if (module) {
                    return string::f("%.2f", module->tracks[0].attackTime * 1000.0f);
                }
                return "6.00";
            }
        };
    
        AttackTimeSlider(MADDY* module) {
            box.size.x = 200.0f;
            quantity = new AttackTimeQuantity(module);

        }
    
        ~AttackTimeSlider() {
            delete quantity;
        }
    };

        AttackTimeSlider* slider = new AttackTimeSlider(module);
        
        struct AttackTimeDisplay : ui::MenuLabel {
            MADDY* module;
    
            AttackTimeDisplay(MADDY* module) : module(module) {
                text = "6.00 ms";
            }
    
            void step() override {
                if (module) {
                    text = string::f("%.2f ms", module->tracks[0].attackTime * 1000.0f);
                }
                ui::MenuLabel::step();
            }
        };
        
        AttackTimeDisplay* display = new AttackTimeDisplay(module);
        
        menu->addChild(slider);
        menu->addChild(display);

	// Shift 控制
	menu->addChild(new MenuSeparator);	
	menu->addChild(createMenuLabel("Shift Settings"));

	for (int trackId = 0; trackId < 3; trackId++) {
	    std::string trackLabel = string::f("Track %d Shift", trackId + 1);
	    menu->addChild(createMenuLabel(trackLabel));
    
	    int currentShift = module->tracks[trackId].shift;
	    std::string currentLabel = string::f("Current: %d steps", currentShift);
	    menu->addChild(createMenuLabel(currentLabel));

	    struct ShiftSlider : ui::Slider {
	        struct ShiftQuantity : Quantity {
	            MADDY* module;
	            int trackIndex;
            
	            ShiftQuantity(MADDY* module, int trackIndex) : module(module), trackIndex(trackIndex) {}
        
	            void setValue(float value) override {
	                if (module) {
	                    value = clamp(value, 0.0f, 1.0f);
	                    int shift = (int)std::round(rescale(value, 0.0f, 1.0f, 0.0f, 15.0f)); 
	                    module->tracks[trackIndex].shift = shift;
	                }
	            }
        
	            float getValue() override {
	                if (module) {
	                    return rescale((float)module->tracks[trackIndex].shift, 0.0f, 15.0f, 0.0f, 1.0f);
	                }
	                return 0.0f;
	            }
        
	            float getMinValue() override { return 0.0f; }
	            float getMaxValue() override { return 1.0f; }            
	            float getDefaultValue() override { return 0.0f; }
	            std::string getLabel() override { return string::f("Track %d Shift", trackIndex + 1); }
	            std::string getUnit() override { return " steps"; }
            std::string getDisplayValueString() override {
	                if (module) {
	                    return string::f("%d", module->tracks[trackIndex].shift);
	                }
	                return "0";
	            }
	        };
    
	        ShiftSlider(MADDY* module, int trackIndex) {
	            box.size.x = 200.0f;
	            quantity = new ShiftQuantity(module, trackIndex);
	        }
    
 	       ~ShiftSlider() {
	            delete quantity;
	        }

		void onDragMove(const event::DragMove& e) override {
		    if (!quantity) return;
    
		    float sensitivity = 0.003f;
		    float delta = e.mouseDelta.x * sensitivity;
    
		    float currentValue = quantity->getValue();
		    float newValue = clamp(currentValue + delta, 0.0f, 1.0f);
		    quantity->setValue(newValue);
		}
  	    };

	    ShiftSlider* slider = new ShiftSlider(module, trackId);
    
	    struct ShiftDisplay : ui::MenuLabel {
	        MADDY* module;
	        int trackIndex;
    
	        ShiftDisplay(MADDY* module, int trackIndex) : module(module), trackIndex(trackIndex) {
	            text = "0 steps";
	        }
    
	        void step() override {
	            if (module) {
	                text = string::f("%d steps", module->tracks[trackIndex].shift);
	            }
	            ui::MenuLabel::step();
	        }
	    };
    
	    ShiftDisplay* display = new ShiftDisplay(module, trackId);
    
	    menu->addChild(slider);
	    menu->addChild(display);
	}
    }
};

Model* modelMADDY = createModel<MADDY, MADDYWidget>("MADDY");