#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

struct MADDYPlusEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    MADDYPlusEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
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

// MADDYPlusStandardBlackKnob now using implementation from widgets/Knobs.hpp

struct OldMADDYPlusStandardBlackKnob_UNUSED : ParamWidget {
    bool isDragging = false;

    OldMADDYPlusStandardBlackKnob_UNUSED() {
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

// MADDYPlusSnapKnob now using implementation from widgets/Knobs.hpp

struct OldMADDYPlusSnapKnob_UNUSED : ParamWidget {
    float accumDelta = 0.0f;

    OldMADDYPlusSnapKnob_UNUSED() {
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

// WhiteKnob now using implementation from widgets/Knobs.hpp

struct OldWhiteKnob_UNUSED : ParamWidget {
    bool isDragging = false;

    OldWhiteKnob_UNUSED() {
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

// SmallGrayKnob now using implementation from widgets/Knobs.hpp

struct OldSmallGrayKnob_UNUSED : ParamWidget {
    bool isDragging = false;

    OldSmallGrayKnob_UNUSED() {
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

// MediumGrayKnob now using implementation from widgets/Knobs.hpp

struct OldMediumGrayKnob_UNUSED : ParamWidget {
    bool isDragging = false;

    OldMediumGrayKnob_UNUSED() {
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

    std::string getLabel() override {
        return "Density";
    }
};

struct Ch2DensityParamQuantity : ParamQuantity {
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

    std::string getLabel() override {
        return "Ch2 Density";
    }
};

struct Ch3DensityParamQuantity : ParamQuantity {
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

    std::string getLabel() override {
        return "Ch3 Density";
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

std::vector<bool> generateMADDYPlusEuclideanRhythm(int length, int fill, int shift) {
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

struct MADDYPlus : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

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
        MANUAL_RESET_PARAM,


        CH2_CLOCK_SOURCE_PARAM,
        CH2_MODE_PARAM,
        CH2_DENSITY_PARAM,
        CH2_CVD_ATTEN_PARAM,
        CH2_STEP_DELAY_PARAM,


        CH3_CLOCK_SOURCE_PARAM,
        CH3_MODE_PARAM,
        CH3_DENSITY_PARAM,
        CH3_CVD_ATTEN_PARAM,
        CH3_STEP_DELAY_PARAM,

        CLOCK_CV_ATTEN_PARAM,

        PARAMS_LEN
    };
    enum InputId {
        CLOCK_CV_INPUT,
        CH2_CV_INPUT,
        CH3_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        CLK_OUTPUT,
        RESET_OUTPUT,
        TRACK1_OUTPUT,
        TRACK2_OUTPUT,
        TRACK3_OUTPUT,
        CHAIN_12_OUTPUT,
        CHAIN_23_OUTPUT,
        CHAIN_123_OUTPUT,
        CV_OUTPUT,
        TRIG_OUTPUT,
        CH2_CV_OUTPUT,
        CH2_TRIG_OUTPUT,
        CH3_CV_OUTPUT,
        CH3_TRIG_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        MODE_LIGHT_RED,
        MODE_LIGHT_GREEN,
        MODE_LIGHT_BLUE,
        MANUAL_RESET_LIGHT,
        LIGHTS_LEN
    };

    float phase = 0.0f;
    float swingPhase = 0.0f;
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
    dsp::PulseGenerator clockPulse;


        void reset() {
            currentTrackIndex = 0;
            globalClockCount = 0;
            for (int i = 0; i < 3; ++i) {
                trackStartClock[i] = 0;
            }
            chainTrigPulse.reset();
        clockPulse.reset();
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
            // Add bounds check to prevent array out of bounds access
            if (activeTrackIdx < 0 || activeTrackIdx >= 3) {
                return 0.0f;
            }
            trackStartClock[activeTrackIdx] = globalClockCount;
            chainTrigger = true;
            chainTrigPulse.trigger(0.001f);
        }

        chainTrigger = chainTrigger || chainTrigPulse.process(sampleTime) > 0.0f;
    if (tracks[activeTrackIdx].trigPulse.process(sampleTime) > 0.0f) {
        clockPulse.trigger(0.001f);
        }

        return tracks[activeTrackIdx].envelopeOutput * 10.0f;
    }
    };
    ChainedSequence chain12, chain23, chain123;

    float globalClockSeconds = 0.5f;
    bool internalClockTriggered = false;
    bool patternClockTriggered = false;
    float sampleRate = 44100.0f;

    dsp::SchmittTrigger modeTrigger;
    dsp::PulseGenerator gateOutPulse;

    int currentStep = 0, sequenceLength = 16, stepToKnobMapping[64];
    float previousVoltage = -999.0f;
    int modeValue = 1;
    int clockSourceValue = 0;


    int ch2ModeValue = 1;
    int ch2ClockSourceValue = 0;
    int ch2StepDelayValue = 1;
    int ch3ModeValue = 1;
    int ch3ClockSourceValue = 0;
    int ch3StepDelayValue = 1;


    int ch2CurrentStep = 0, ch2SequenceLength = 16, ch2StepToKnobMapping[64];
    int ch3CurrentStep = 0, ch3SequenceLength = 16, ch3StepToKnobMapping[64];
    float ch2PreviousVoltage = -999.0f;
    float ch3PreviousVoltage = -999.0f;
    dsp::PulseGenerator ch2GateOutPulse, ch3GateOutPulse;


    static const int CH2_MAX_DELAY = 8;
    static const int CH3_MAX_DELAY = 8;
    float ch2CvHistory[8];
    float ch3CvHistory[8];
    int ch2HistoryIndex = 0, ch3HistoryIndex = 0;


    static const int CH2_CVD_BUFFER_SIZE = 192000;
    static const int CH3_CVD_BUFFER_SIZE = 192000;
    float* ch2CvdBuffer = nullptr;
    float* ch3CvdBuffer = nullptr;
    int ch2CvdWriteIndex = 0, ch3CvdWriteIndex = 0;
    float ch2PreviousCVDOutput = -999.0f;
    float ch3PreviousCVDOutput = -999.0f;

    // Custom pattern support
    bool useCustomPattern[3] = {false, false, false};
    std::vector<int> customPattern[3];
    int customPatternIndex[3] = {0, 0, 0};

    MADDYPlus() {
        // Allocate buffers on heap to avoid stack overflow
        ch2CvdBuffer = new float[CH2_CVD_BUFFER_SIZE];
        ch3CvdBuffer = new float[CH3_CVD_BUFFER_SIZE];
        
        // Initialize buffers to zero
        for (int i = 0; i < CH2_CVD_BUFFER_SIZE; i++) ch2CvdBuffer[i] = 0.0f;
        for (int i = 0; i < CH3_CVD_BUFFER_SIZE; i++) ch3CvdBuffer[i] = 0.0f;
        
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(FREQ_PARAM, -3.0f, 7.0f, 2.8073549270629883f, "Frequency", " Hz", 2.0f, 1.0f);
        configParam(SWING_PARAM, 0.0f, 1.0f, 0.0f, "Swing", "°", 0.0f, -90.0f, 180.0f);
        configParam(LENGTH_PARAM, 1.0f, 32.0f, 32.0f, "Length");
        getParamQuantity(LENGTH_PARAM)->snapEnabled = true;
        configParam(DECAY_PARAM, 0.0f, 1.0f, 0.30000001192092896f, "Decay");

        configParam(K1_PARAM, -10.0f, 10.0f, 0.0f, "K1", "V");
        configParam(K2_PARAM, -10.0f, 10.0f, 2.0f, "K2", "V");
        configParam(K3_PARAM, -10.0f, 10.0f, 4.0f, "K3", "V");
        configParam(K4_PARAM, -10.0f, 10.0f, 6.0f, "K4", "V");
        configParam(K5_PARAM, -10.0f, 10.0f, 8.0f, "K5", "V");

        configParam(MODE_PARAM, 0.0f, 5.0f, 1.0f, "Mode");
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
        configParam(CLOCK_SOURCE_PARAM, 0.0f, 6.0f, 1.0f, "Clock Source");
        getParamQuantity(CLOCK_SOURCE_PARAM)->snapEnabled = true;

        // Track 1
        configParam(TRACK1_FILL_PARAM, 0.0f, 100.0f, 100.0f, "T1 Fill", "%");
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

        // Track 2
        configParam(TRACK2_FILL_PARAM, 0.0f, 100.0f, 100.0f, "T2 Fill", "%");
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

        // Track 3
        configParam(TRACK3_FILL_PARAM, 0.0f, 100.0f, 100.0f, "T3 Fill", "%");
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

        for (int i = 0; i < 3; ++i) {
            configOutput(TRACK1_OUTPUT + i, string::f("T%d Trigger", i+1));
        }

        configOutput(RESET_OUTPUT, "Reset");
        configOutput(CLK_OUTPUT, "Clock");
        configOutput(CHAIN_12_OUTPUT, "Chain 1+2");
        configOutput(CHAIN_23_OUTPUT, "Chain 2+3");
        configOutput(CHAIN_123_OUTPUT, "Chain 1+2+3");
        configOutput(CV_OUTPUT, "CV");
        configOutput(TRIG_OUTPUT, "Trigger");

        configLight(MODE_LIGHT_RED, "Mode Red");
        configLight(MODE_LIGHT_GREEN, "Mode Green");
        configLight(MODE_LIGHT_BLUE, "Mode Blue");
        configParam(MANUAL_RESET_PARAM, 0.0f, 1.0f, 0.0f, "Manual Reset");
        configLight(MANUAL_RESET_LIGHT, "Manual Reset Light");


        configParam(CH2_CLOCK_SOURCE_PARAM, 0.0f, 6.0f, 2.0f, "Ch2 Clock Source");
        getParamQuantity(CH2_CLOCK_SOURCE_PARAM)->snapEnabled = true;
        configParam(CH2_MODE_PARAM, 0.0f, 5.0f, 1.0f, "Ch2 Mode");
        getParamQuantity(CH2_MODE_PARAM)->snapEnabled = true;
        configParam(CH2_DENSITY_PARAM, 0.0f, 1.0f, 0.5f, "Ch2 Density");
        delete paramQuantities[CH2_DENSITY_PARAM];
        Ch2DensityParamQuantity* ch2DensityQuantity = new Ch2DensityParamQuantity;
        ch2DensityQuantity->module = this;
        ch2DensityQuantity->paramId = CH2_DENSITY_PARAM;
        ch2DensityQuantity->minValue = 0.0f;
        ch2DensityQuantity->maxValue = 1.0f;
        ch2DensityQuantity->defaultValue = 0.5f;
        ch2DensityQuantity->name = "Ch2 Density";
        paramQuantities[CH2_DENSITY_PARAM] = ch2DensityQuantity;
        configParam(CH2_CVD_ATTEN_PARAM, 0.0f, 1.0f, 0.0f, "Ch2 CVD Time/Attenuation");
        configParam(CH2_STEP_DELAY_PARAM, 0.0f, 5.0f, 0.0f, "Ch2 Step Delay");
        getParamQuantity(CH2_STEP_DELAY_PARAM)->snapEnabled = true;


        configParam(CH3_CLOCK_SOURCE_PARAM, 0.0f, 6.0f, 3.0f, "Ch3 Clock Source");
        getParamQuantity(CH3_CLOCK_SOURCE_PARAM)->snapEnabled = true;
        configParam(CH3_MODE_PARAM, 0.0f, 5.0f, 1.0f, "Ch3 Mode");
        getParamQuantity(CH3_MODE_PARAM)->snapEnabled = true;
        configParam(CH3_DENSITY_PARAM, 0.0f, 1.0f, 0.5f, "Ch3 Density");
        delete paramQuantities[CH3_DENSITY_PARAM];
        Ch3DensityParamQuantity* ch3DensityQuantity = new Ch3DensityParamQuantity;
        ch3DensityQuantity->module = this;
        ch3DensityQuantity->paramId = CH3_DENSITY_PARAM;
        ch3DensityQuantity->minValue = 0.0f;
        ch3DensityQuantity->maxValue = 1.0f;
        ch3DensityQuantity->defaultValue = 0.5f;
        ch3DensityQuantity->name = "Ch3 Density";
        paramQuantities[CH3_DENSITY_PARAM] = ch3DensityQuantity;
        configParam(CH3_CVD_ATTEN_PARAM, 0.0f, 1.0f, 0.0f, "Ch3 CVD Time/Attenuation");
        configParam(CH3_STEP_DELAY_PARAM, 0.0f, 5.0f, 0.0f, "Ch3 Step Delay");
        getParamQuantity(CH3_STEP_DELAY_PARAM)->snapEnabled = true;

        configParam(CLOCK_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Clock CV Attenuverter");

        configInput(CLOCK_CV_INPUT, "Clock CV");
        configInput(CH2_CV_INPUT, "Ch2 CV");
        configInput(CH3_CV_INPUT, "Ch3 CV");
        configOutput(CH2_CV_OUTPUT, "Ch2 CV");
        configOutput(CH2_TRIG_OUTPUT, "Ch2 Trigger");
        configOutput(CH3_CV_OUTPUT, "Ch3 CV");
        configOutput(CH3_TRIG_OUTPUT, "Ch3 Trigger");

        chain12.trackIndices = {0, 1};
        chain23.trackIndices = {1, 2};
        chain123.trackIndices = {0, 1, 0, 2};

        sampleRate = APP->engine->getSampleRate();
        generateMapping();
    }
    
    ~MADDYPlus() {
        // Clean up dynamically allocated buffers
        if (ch2CvdBuffer) {
            delete[] ch2CvdBuffer;
            ch2CvdBuffer = nullptr;
        }
        if (ch3CvdBuffer) {
            delete[] ch3CvdBuffer;
            ch3CvdBuffer = nullptr;
        }
    }

    void generateMapping() {
        // Use custom pattern if enabled
        if (useCustomPattern[0] && !customPattern[0].empty()) {
            float density = params[DENSITY_PARAM].getValue();

            // Density controls how many knobs are available
            int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;

            sequenceLength = customPattern[0].size();
            sequenceLength = clamp(sequenceLength, 1, 64);

            for (int i = 0; i < sequenceLength; i++) {
                // Clamp knob index to available knobs
                int knobIndex = customPattern[0][i] % primaryKnobs;
                stepToKnobMapping[i] = knobIndex;
            }
            return;
        }

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
            case 3:
                for (int i = 0; i < sequenceLength; i++) {
                    stepToKnobMapping[i] = (primaryKnobs - 1) - (i % primaryKnobs);
                }
                break;
            case 4: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < sequenceLength; i++) {
                    int reverseIndex = 31 - (i % 32);
                    stepToKnobMapping[i] = minimalistPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
            case 5: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < sequenceLength; i++) {
                    int reverseIndex = 4 - (i % 5);
                    stepToKnobMapping[i] = jumpPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
        }

        if (chaos > 0.3f) {
            int chaosSteps = (int)(chaos * sequenceLength * 0.3f);
            for (int i = 0; i < chaosSteps; i++) {
                int randomStep = random::u32() % sequenceLength;
                stepToKnobMapping[randomStep] = random::u32() % primaryKnobs;
            }
        }
    }

    void generateCh2Mapping() {
        // Use custom pattern if enabled
        if (useCustomPattern[1] && !customPattern[1].empty()) {
            float density = params[CH2_DENSITY_PARAM].getValue();

            // Density controls how many knobs are available
            int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;

            ch2SequenceLength = customPattern[1].size();
            ch2SequenceLength = clamp(ch2SequenceLength, 1, 64);

            for (int i = 0; i < ch2SequenceLength; i++) {
                // Clamp knob index to available knobs
                int knobIndex = customPattern[1][i] % primaryKnobs;
                ch2StepToKnobMapping[i] = knobIndex;
            }
            return;
        }

        float density = params[CH2_DENSITY_PARAM].getValue();
        float chaos = params[CHAOS_PARAM].getValue();

        if (density < 0.2f) {
            ch2SequenceLength = 8 + (int)(density * 20);
        } else if (density < 0.4f) {
            ch2SequenceLength = 12 + (int)((density - 0.2f) * 40);
        } else if (density < 0.6f) {
            ch2SequenceLength = 20 + (int)((density - 0.4f) * 40);
        } else {
            ch2SequenceLength = 28 + (int)((density - 0.6f) * 50.1f);
        }
        ch2SequenceLength = clamp(ch2SequenceLength, 8, 48);

        if (chaos > 0.0f) {
            float chaosRange = chaos * ch2SequenceLength * 0.5f;
            float randomOffset = (random::uniform() - 0.5f) * 2.0f * chaosRange;
            ch2SequenceLength += (int)randomOffset;
            ch2SequenceLength = clamp(ch2SequenceLength, 4, 64);
        }

        int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;

        for (int i = 0; i < 64; i++) ch2StepToKnobMapping[i] = 0;

        switch (ch2ModeValue) {
            case 0:
                for (int i = 0; i < ch2SequenceLength; i++) {
                    ch2StepToKnobMapping[i] = i % primaryKnobs;
                }
                break;
            case 1: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < ch2SequenceLength; i++) {
                    ch2StepToKnobMapping[i] = minimalistPattern[i % 32] % primaryKnobs;
                }
                break;
            }
            case 2: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < ch2SequenceLength; i++) {
                    ch2StepToKnobMapping[i] = jumpPattern[i % 5] % primaryKnobs;
                }
                break;
            }
            case 3:
                for (int i = 0; i < ch2SequenceLength; i++) {
                    ch2StepToKnobMapping[i] = (primaryKnobs - 1) - (i % primaryKnobs);
                }
                break;
            case 4: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < ch2SequenceLength; i++) {
                    int reverseIndex = 31 - (i % 32);
                    ch2StepToKnobMapping[i] = minimalistPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
            case 5: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < ch2SequenceLength; i++) {
                    int reverseIndex = 4 - (i % 5);
                    ch2StepToKnobMapping[i] = jumpPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
        }

        if (chaos > 0.3f) {
            int chaosSteps = (int)(chaos * ch2SequenceLength * 0.3f);
            for (int i = 0; i < chaosSteps; i++) {
                int randomStep = random::u32() % ch2SequenceLength;
                ch2StepToKnobMapping[randomStep] = random::u32() % 5;
            }
        }
    }

    void generateCh3Mapping() {
        // Use custom pattern if enabled
        if (useCustomPattern[2] && !customPattern[2].empty()) {
            float density = params[CH3_DENSITY_PARAM].getValue();

            // Density controls how many knobs are available
            int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;

            ch3SequenceLength = customPattern[2].size();
            ch3SequenceLength = clamp(ch3SequenceLength, 1, 64);

            for (int i = 0; i < ch3SequenceLength; i++) {
                // Clamp knob index to available knobs
                int knobIndex = customPattern[2][i] % primaryKnobs;
                ch3StepToKnobMapping[i] = knobIndex;
            }
            return;
        }

        float density = params[CH3_DENSITY_PARAM].getValue();
        float chaos = params[CHAOS_PARAM].getValue();

        if (density < 0.2f) {
            ch3SequenceLength = 8 + (int)(density * 20);
        } else if (density < 0.4f) {
            ch3SequenceLength = 12 + (int)((density - 0.2f) * 40);
        } else if (density < 0.6f) {
            ch3SequenceLength = 20 + (int)((density - 0.4f) * 40);
        } else {
            ch3SequenceLength = 28 + (int)((density - 0.6f) * 50.1f);
        }
        ch3SequenceLength = clamp(ch3SequenceLength, 8, 48);

        if (chaos > 0.0f) {
            float chaosRange = chaos * ch3SequenceLength * 0.5f;
            float randomOffset = (random::uniform() - 0.5f) * 2.0f * chaosRange;
            ch3SequenceLength += (int)randomOffset;
            ch3SequenceLength = clamp(ch3SequenceLength, 4, 64);
        }

        int primaryKnobs = (density < 0.2f) ? 2 : (density < 0.4f) ? 3 : (density < 0.6f) ? 4 : 5;

        for (int i = 0; i < 64; i++) ch3StepToKnobMapping[i] = 0;

        switch (ch3ModeValue) {
            case 0:
                for (int i = 0; i < ch3SequenceLength; i++) {
                    ch3StepToKnobMapping[i] = i % primaryKnobs;
                }
                break;
            case 1: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < ch3SequenceLength; i++) {
                    ch3StepToKnobMapping[i] = minimalistPattern[i % 32] % primaryKnobs;
                }
                break;
            }
            case 2: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < ch3SequenceLength; i++) {
                    ch3StepToKnobMapping[i] = jumpPattern[i % 5] % primaryKnobs;
                }
                break;
            }
            case 3:
                for (int i = 0; i < ch3SequenceLength; i++) {
                    ch3StepToKnobMapping[i] = (primaryKnobs - 1) - (i % primaryKnobs);
                }
                break;
            case 4: {
                int minimalistPattern[32] = {0,1,2,0,1,2,3,4,3,4,0,1,2,0,1,2,3,4,3,4,1,3,2,4,0,2,1,3,0,4,2,1};
                for (int i = 0; i < ch3SequenceLength; i++) {
                    int reverseIndex = 31 - (i % 32);
                    ch3StepToKnobMapping[i] = minimalistPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
            case 5: {
                int jumpPattern[5] = {0, 2, 4, 1, 3};
                for (int i = 0; i < ch3SequenceLength; i++) {
                    int reverseIndex = 4 - (i % 5);
                    ch3StepToKnobMapping[i] = jumpPattern[reverseIndex] % primaryKnobs;
                }
                break;
            }
        }

        if (chaos > 0.3f) {
            int chaosSteps = (int)(chaos * ch3SequenceLength * 0.3f);
            for (int i = 0; i < chaosSteps; i++) {
                int randomStep = random::u32() % ch3SequenceLength;
                ch3StepToKnobMapping[randomStep] = random::u32() % 5;
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


        ch2CurrentStep = 0;
        ch3CurrentStep = 0;
        ch2PreviousVoltage = -999.0f;
        ch3PreviousVoltage = -999.0f;
        ch2PreviousCVDOutput = -999.0f;
        ch3PreviousCVDOutput = -999.0f;


        for (int i = 0; i < CH2_MAX_DELAY; i++) ch2CvHistory[i] = 0.0f;
        for (int i = 0; i < CH3_MAX_DELAY; i++) ch3CvHistory[i] = 0.0f;
        
        // Safely clear buffers if they exist
        if (ch2CvdBuffer) {
            for (int i = 0; i < CH2_CVD_BUFFER_SIZE; i++) ch2CvdBuffer[i] = 0.0f;
        }
        if (ch3CvdBuffer) {
            for (int i = 0; i < CH3_CVD_BUFFER_SIZE; i++) ch3CvdBuffer[i] = 0.0f;
        }

        ch2HistoryIndex = 0;
        ch3HistoryIndex = 0;
        ch2CvdWriteIndex = 0;
        ch3CvdWriteIndex = 0;
        generateCh2Mapping();
        generateCh3Mapping();
    }

json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "modeValue", json_integer(modeValue));
        json_object_set_new(rootJ, "clockSourceValue", json_integer(clockSourceValue));


        json_t* attackTimesJ = json_array();
        for (int i = 0; i < 3; ++i) {
            json_array_append_new(attackTimesJ, json_real(tracks[i].attackTime));
        }
        json_object_set_new(rootJ, "attackTimes", attackTimesJ);


    json_t* shiftsJ = json_array();
    for (int i = 0; i < 3; ++i) {
     json_array_append_new(shiftsJ, json_integer(tracks[i].shift));
    }
    json_object_set_new(rootJ, "shifts", shiftsJ);

        // Save custom patterns
        json_t* customPatternsJ = json_array();
        for (int ch = 0; ch < 3; ch++) {
            json_t* channelJ = json_object();
            json_object_set_new(channelJ, "useCustomPattern", json_boolean(useCustomPattern[ch]));

            json_t* patternJ = json_array();
            for (int step : customPattern[ch]) {
                json_array_append_new(patternJ, json_integer(step));
            }
            json_object_set_new(channelJ, "pattern", patternJ);
            json_array_append_new(customPatternsJ, channelJ);
        }
        json_object_set_new(rootJ, "customPatterns", customPatternsJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }

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

        // Load custom patterns
        json_t* customPatternsJ = json_object_get(rootJ, "customPatterns");
        if (customPatternsJ) {
            for (int ch = 0; ch < 3; ch++) {
                json_t* channelJ = json_array_get(customPatternsJ, ch);
                if (channelJ) {
                    json_t* useCustomJ = json_object_get(channelJ, "useCustomPattern");
                    if (useCustomJ) {
                        useCustomPattern[ch] = json_boolean_value(useCustomJ);
                    }

                    json_t* patternJ = json_object_get(channelJ, "pattern");
                    if (patternJ) {
                        customPattern[ch].clear();
                        size_t arraySize = json_array_size(patternJ);
                        for (size_t i = 0; i < arraySize; i++) {
                            json_t* stepJ = json_array_get(patternJ, i);
                            if (stepJ) {
                                customPattern[ch].push_back(json_integer_value(stepJ));
                            }
                        }
                    }
                }
            }
        }
    }

    void process(const ProcessArgs& args) override {
        float freqParam = params[FREQ_PARAM].getValue();

        // Apply Clock CV modulation
        float clockCVMod = 0.0f;
        if (inputs[CLOCK_CV_INPUT].isConnected()) {
            float cvVoltage = inputs[CLOCK_CV_INPUT].getVoltage();
            float attenuation = params[CLOCK_CV_ATTEN_PARAM].getValue();
            clockCVMod = cvVoltage * attenuation;
        }

        float freq = std::pow(2.0f, freqParam + clockCVMod) * 1.0f;

        float swingParam = params[SWING_PARAM].getValue();
        float swing = clamp(swingParam, 0.0f, 1.0f);


        static float resetPulseTimer = 0.0f;

        if (params[MANUAL_RESET_PARAM].getValue() > 0.5f) {
            onReset();
            params[MANUAL_RESET_PARAM].setValue(0.0f);
            resetPulseTimer = 0.1f;
            outputs[RESET_OUTPUT].setVoltage(10.0f);
        } else {
            outputs[RESET_OUTPUT].setVoltage(0.0f);
        }


        if (resetPulseTimer > 0.0f) {
            lights[MANUAL_RESET_LIGHT].setBrightness(1.0f);
            resetPulseTimer -= args.sampleTime;
        } else {
            lights[MANUAL_RESET_LIGHT].setBrightness(0.0f);
        }

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

            track.pattern = generateMADDYPlusEuclideanRhythm(track.length, track.fill, track.shift);

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

        modeValue = (int)std::round(params[MODE_PARAM].getValue());

        clockSourceValue = (int)std::round(params[CLOCK_SOURCE_PARAM].getValue());

        patternClockTriggered = false;
    switch (clockSourceValue) {
        case 0:
            patternClockTriggered = internalClockTriggered;
            break;
        case 1:
            patternClockTriggered = tracks[0].justTriggered;
            break;
        case 2:
            patternClockTriggered = tracks[1].justTriggered;
            break;
        case 3:
            patternClockTriggered = tracks[2].justTriggered;
            break;
        case 4:
            patternClockTriggered = chain12.clockPulse.process(args.sampleTime) > 0.0f;
            break;
        case 5:
            patternClockTriggered = chain23.clockPulse.process(args.sampleTime) > 0.0f;
            break;
        case 6:
            patternClockTriggered = chain123.clockPulse.process(args.sampleTime) > 0.0f;
            break;
    }

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


        ch2ModeValue = (int)std::round(params[CH2_MODE_PARAM].getValue());
        ch2ClockSourceValue = (int)std::round(params[CH2_CLOCK_SOURCE_PARAM].getValue());
        ch2StepDelayValue = (int)std::round(params[CH2_STEP_DELAY_PARAM].getValue());

        bool ch2PatternClockTriggered = false;
        switch (ch2ClockSourceValue) {
            case 0:
                ch2PatternClockTriggered = internalClockTriggered;
                break;
            case 1:
                ch2PatternClockTriggered = tracks[0].justTriggered;
                break;
            case 2:
                ch2PatternClockTriggered = tracks[1].justTriggered;
                break;
            case 3:
                ch2PatternClockTriggered = tracks[2].justTriggered;
                break;
            case 4:
                ch2PatternClockTriggered = chain12.clockPulse.process(args.sampleTime) > 0.0f;
                break;
            case 5:
                ch2PatternClockTriggered = chain23.clockPulse.process(args.sampleTime) > 0.0f;
                break;
            case 6:
                ch2PatternClockTriggered = chain123.clockPulse.process(args.sampleTime) > 0.0f;
                break;
        }

        if (ch2PatternClockTriggered) {
            int ch2ActiveKnob = ch2StepToKnobMapping[ch2CurrentStep];
            // Ensure knob index is within bounds (0-4)
            ch2ActiveKnob = clamp(ch2ActiveKnob, 0, 4);
            float ch2Voltage = params[K1_PARAM + ch2ActiveKnob].getValue();
            ch2CvHistory[ch2HistoryIndex] = ch2Voltage;

            ch2CurrentStep = (ch2CurrentStep + 1) % ch2SequenceLength;
            generateCh2Mapping();

            ch2HistoryIndex = (ch2HistoryIndex + 1) % CH2_MAX_DELAY;
        }


        int ch2ShiftRegisterIndex = (ch2HistoryIndex - ch2StepDelayValue + CH2_MAX_DELAY) % CH2_MAX_DELAY;
        int ch2MappedKnob = ch2StepToKnobMapping[ch2CurrentStep];
        ch2MappedKnob = clamp(ch2MappedKnob, 0, 4);
        float ch2ShiftRegisterCV = (ch2StepDelayValue == 0) ? params[K1_PARAM + ch2MappedKnob].getValue() : ch2CvHistory[ch2ShiftRegisterIndex];

        float ch2DelayTimeMs = 0.0f;
        float ch2KnobValue = params[CH2_CVD_ATTEN_PARAM].getValue();

        if (!inputs[CH2_CV_INPUT].isConnected()) {
            ch2DelayTimeMs = ch2KnobValue * 1000.0f;
        } else {
            float ch2CvdCV = clamp(inputs[CH2_CV_INPUT].getVoltage(), 0.0f, 10.0f);
            ch2DelayTimeMs = (ch2CvdCV / 10.0f) * ch2KnobValue * 1000.0f;
        }

        if (ch2DelayTimeMs <= 0.001f) {
            outputs[CH2_CV_OUTPUT].setVoltage(ch2ShiftRegisterCV);
        } else if (ch2CvdBuffer) {  // Add null check
            ch2CvdBuffer[ch2CvdWriteIndex] = ch2ShiftRegisterCV;
            ch2CvdWriteIndex = (ch2CvdWriteIndex + 1) % CH2_CVD_BUFFER_SIZE;

            int ch2DelaySamples = (int)(ch2DelayTimeMs * sampleRate / 1000.0f);
            ch2DelaySamples = clamp(ch2DelaySamples, 0, CH2_CVD_BUFFER_SIZE - 1);

            int ch2ReadIndex = (ch2CvdWriteIndex - ch2DelaySamples + CH2_CVD_BUFFER_SIZE) % CH2_CVD_BUFFER_SIZE;
            float ch2DelayedCV = ch2CvdBuffer[ch2ReadIndex];

            outputs[CH2_CV_OUTPUT].setVoltage(ch2DelayedCV);
        } else {
            outputs[CH2_CV_OUTPUT].setVoltage(ch2ShiftRegisterCV);
        }

        float ch2CurrentDelayedCV = outputs[CH2_CV_OUTPUT].getVoltage();
        if (ch2CurrentDelayedCV != ch2PreviousCVDOutput) {
            ch2GateOutPulse.trigger(0.01f);
            ch2PreviousCVDOutput = ch2CurrentDelayedCV;
        }

        outputs[CH2_TRIG_OUTPUT].setVoltage(ch2GateOutPulse.process(args.sampleTime) ? 10.0f : 0.0f);


        ch3ModeValue = (int)std::round(params[CH3_MODE_PARAM].getValue());
        ch3ClockSourceValue = (int)std::round(params[CH3_CLOCK_SOURCE_PARAM].getValue());
        ch3StepDelayValue = (int)std::round(params[CH3_STEP_DELAY_PARAM].getValue());

        bool ch3PatternClockTriggered = false;
        switch (ch3ClockSourceValue) {
            case 0:
                ch3PatternClockTriggered = internalClockTriggered;
                break;
            case 1:
                ch3PatternClockTriggered = tracks[0].justTriggered;
                break;
            case 2:
                ch3PatternClockTriggered = tracks[1].justTriggered;
                break;
            case 3:
                ch3PatternClockTriggered = tracks[2].justTriggered;
                break;
            case 4:
                ch3PatternClockTriggered = chain12.clockPulse.process(args.sampleTime) > 0.0f;
                break;
            case 5:
                ch3PatternClockTriggered = chain23.clockPulse.process(args.sampleTime) > 0.0f;
                break;
            case 6:
                ch3PatternClockTriggered = chain123.clockPulse.process(args.sampleTime) > 0.0f;
                break;
        }

        if (ch3PatternClockTriggered) {
            int ch3ActiveKnob = ch3StepToKnobMapping[ch3CurrentStep];
            // Ensure knob index is within bounds (0-4)
            ch3ActiveKnob = clamp(ch3ActiveKnob, 0, 4);
            float ch3Voltage = params[K1_PARAM + ch3ActiveKnob].getValue();
            ch3CvHistory[ch3HistoryIndex] = ch3Voltage;

            ch3CurrentStep = (ch3CurrentStep + 1) % ch3SequenceLength;
            generateCh3Mapping();

            ch3HistoryIndex = (ch3HistoryIndex + 1) % CH3_MAX_DELAY;
        }


        int ch3ShiftRegisterIndex = (ch3HistoryIndex - ch3StepDelayValue + CH3_MAX_DELAY) % CH3_MAX_DELAY;
        int ch3MappedKnob = ch3StepToKnobMapping[ch3CurrentStep];
        ch3MappedKnob = clamp(ch3MappedKnob, 0, 4);
        float ch3ShiftRegisterCV = (ch3StepDelayValue == 0) ? params[K1_PARAM + ch3MappedKnob].getValue() : ch3CvHistory[ch3ShiftRegisterIndex];

        float ch3DelayTimeMs = 0.0f;
        float ch3KnobValue = params[CH3_CVD_ATTEN_PARAM].getValue();

        if (!inputs[CH3_CV_INPUT].isConnected()) {
            ch3DelayTimeMs = ch3KnobValue * 1000.0f;
        } else {
            float ch3CvdCV = clamp(inputs[CH3_CV_INPUT].getVoltage(), 0.0f, 10.0f);
            ch3DelayTimeMs = (ch3CvdCV / 10.0f) * ch3KnobValue * 1000.0f;
        }

        if (ch3DelayTimeMs <= 0.001f) {
            outputs[CH3_CV_OUTPUT].setVoltage(ch3ShiftRegisterCV);
        } else if (ch3CvdBuffer) {  // Add null check
            ch3CvdBuffer[ch3CvdWriteIndex] = ch3ShiftRegisterCV;
            ch3CvdWriteIndex = (ch3CvdWriteIndex + 1) % CH3_CVD_BUFFER_SIZE;

            int ch3DelaySamples = (int)(ch3DelayTimeMs * sampleRate / 1000.0f);
            ch3DelaySamples = clamp(ch3DelaySamples, 0, CH3_CVD_BUFFER_SIZE - 1);

            int ch3ReadIndex = (ch3CvdWriteIndex - ch3DelaySamples + CH3_CVD_BUFFER_SIZE) % CH3_CVD_BUFFER_SIZE;
            float ch3DelayedCV = ch3CvdBuffer[ch3ReadIndex];

            outputs[CH3_CV_OUTPUT].setVoltage(ch3DelayedCV);
        } else {
            outputs[CH3_CV_OUTPUT].setVoltage(ch3ShiftRegisterCV);
        }

        float ch3CurrentDelayedCV = outputs[CH3_CV_OUTPUT].getVoltage();
        if (ch3CurrentDelayedCV != ch3PreviousCVDOutput) {
            ch3GateOutPulse.trigger(0.01f);
            ch3PreviousCVDOutput = ch3CurrentDelayedCV;
        }

        outputs[CH3_TRIG_OUTPUT].setVoltage(ch3GateOutPulse.process(args.sampleTime) ? 10.0f : 0.0f);
        
        // Reset justTriggered flags after all channels have processed
        for (int i = 0; i < 3; i++) {
            tracks[i].justTriggered = false;
        }
    }
};

struct MADDYPlusClickableLight : ParamWidget {
    MADDYPlus* module;

    MADDYPlusClickableLight() {
        box.size = Vec(10, 10);
    }

    void draw(const DrawArgs& args) override {
        if (!module) return;

        float brightness = module->lights[MADDYPlus::MANUAL_RESET_LIGHT].getBrightness();

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, box.size.x / 2 - 1);


        if (brightness > 0.5f) {

            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        } else {

            nvgFillColor(args.vg, nvgRGB(255, 133, 133));
        }
        nvgFill(args.vg);


        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                pq->setValue(1.0f);
            }
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }
};


struct Ch1ModeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        MADDYPlus* module = dynamic_cast<MADDYPlus*>(this->module);
        if (!module) return "Minimalism";

        switch (module->modeValue) {
            case 0: return "Sequential";
            case 1: return "Minimalism";
            case 2: return "Jump";
            case 3: return "Rev Sequential";
            case 4: return "Rev Minimalism";
            case 5: return "Rev Jump";
            default: return "Minimalism";
        }
    }

    std::string getLabel() override {
        return "Ch1 Mode";
    }
};

struct Ch2ModeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        MADDYPlus* module = dynamic_cast<MADDYPlus*>(this->module);
        if (!module) return "Minimalism";

        switch (module->ch2ModeValue) {
            case 0: return "Sequential";
            case 1: return "Minimalism";
            case 2: return "Jump";
            case 3: return "Rev Sequential";
            case 4: return "Rev Minimalism";
            case 5: return "Rev Jump";
            default: return "Minimalism";
        }
    }

    std::string getLabel() override {
        return "Ch2 Mode";
    }
};

struct Ch3ModeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        MADDYPlus* module = dynamic_cast<MADDYPlus*>(this->module);
        if (!module) return "Minimalism";

        switch (module->ch3ModeValue) {
            case 0: return "Sequential";
            case 1: return "Minimalism";
            case 2: return "Jump";
            case 3: return "Rev Sequential";
            case 4: return "Rev Minimalism";
            case 5: return "Rev Jump";
            default: return "Minimalism";
        }
    }

    std::string getLabel() override {
        return "Ch3 Mode";
    }
};

struct Ch1ClockSourceParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        switch (value) {
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
        return "Ch1 Clock Source";
    }
};

struct Ch2ClockSourceParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        switch (value) {
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
        return "Ch2 Clock Source";
    }
};

struct Ch3ClockSourceParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        switch (value) {
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
        return "Ch3 Clock Source";
    }
};


struct MADDYPlusWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    MADDYPlusWidget(MADDYPlus* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");

        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new MADDYPlusEnhancedTextLabel(Vec(27, 1), Vec(box.size.x, 20), "M A D D Y +", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new MADDYPlusEnhancedTextLabel(Vec(27, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // Clock CV Input and Attenuverter (left side of yellow title)
        addInput(createInputCentered<PJ301MPort>(Vec(16, 17), module, MADDYPlus::CLOCK_CV_INPUT));
        addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(Vec(42, 17), module, MADDYPlus::CLOCK_CV_ATTEN_PARAM));

        // Original LEN position
        addChild(new MADDYPlusEnhancedTextLabel(Vec(8, 28), Vec(25, 15), "LEN", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(20, 52), module, MADDYPlus::LENGTH_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(48, 28), Vec(25, 15), "RST", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(60, 52), module, MADDYPlus::RESET_OUTPUT));

        MADDYPlusClickableLight* resetButton = createParam<MADDYPlusClickableLight>(Vec(72, 50), module, MADDYPlus::MANUAL_RESET_PARAM);
resetButton->module = module;
        addParam(resetButton);

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 28), Vec(25, 15), "FREQ", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MicrotuneKnob>(Vec(98, 52), module, MADDYPlus::FREQ_PARAM));

        // Original DECAY position
        addChild(new MADDYPlusEnhancedTextLabel(Vec(8, 61), Vec(25, 15), "DECAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(20, 85), module, MADDYPlus::DECAY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(48, 61), Vec(25, 15), "SWING", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MicrotuneKnob>(Vec(60, 85), module, MADDYPlus::SWING_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 61), Vec(25, 15), "CLK", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 85), module, MADDYPlus::CLK_OUTPUT));

        addChild(new VerticalLine(Vec(39, 55), Vec(1, 242)));
        addChild(new VerticalLine(Vec(117, 55), Vec(1, 242)));
        addChild(new HorizontalLine(Vec(40, 96), Vec(40, 1)));
        addChild(new HorizontalLine(Vec(122, 172), Vec(60, 1)));


        float trackY[3] = {107, 183, 259};

        for (int i = 0; i < 3; ++i) {
            float y = trackY[i];

            addChild(new MADDYPlusEnhancedTextLabel(Vec(8, y - 10), Vec(25, 10), string::f("T%d", i+1), 7.f, nvgRGB(255, 200, 100), true));

            addChild(new MADDYPlusEnhancedTextLabel(Vec(8, y), Vec(25, 10), "FILL", 6.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(20, y + 20), module, MADDYPlus::TRACK1_FILL_PARAM + i * 2));

            addChild(new MADDYPlusEnhancedTextLabel(Vec(8, y + 33), Vec(25, 10), "D/M", 6.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(20, y + 53), module, MADDYPlus::TRACK1_DIVMULT_PARAM + i * 2));
        }

        float cvY[5] = {127, 172, 217, 262, 307};
        for (int i = 0; i < 5; ++i) {
            addChild(new MADDYPlusEnhancedTextLabel(Vec(40, cvY[i] - 30), Vec(40, 10), string::f("Step %d", i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addChild(new MADDYPlusEnhancedTextLabel(Vec(48, cvY[i] - 15), Vec(25, 10), std::to_string(i + 1), 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(60, cvY[i] - 5), module, MADDYPlus::K1_PARAM + i));
        }

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 97), Vec(25, 10), "MODE", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(98, 116), module, MADDYPlus::MODE_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 130), Vec(25, 10), "DENSITY", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(98, 154), module, MADDYPlus::DENSITY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 170), Vec(25, 10), "CHAOS", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(98, 194), module, MADDYPlus::CHAOS_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 210), Vec(25, 10), "CV OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 234), module, MADDYPlus::CV_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 250), Vec(25, 10), "TRIG OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(98, 274), module, MADDYPlus::TRIG_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(86, 288), Vec(25, 10), "CLK SRC", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(98, 308), module, MADDYPlus::CLOCK_SOURCE_PARAM));


        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(-2, 337), Vec(20, 15), "T1", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 343), module, MADDYPlus::TRACK1_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(-2, 362), Vec(20, 15), "12", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 368), module, MADDYPlus::CHAIN_12_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(38, 337), Vec(20, 15), "T2", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 343), module, MADDYPlus::TRACK2_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(38, 362), Vec(20, 15), "23", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 368), module, MADDYPlus::CHAIN_23_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(75, 337), Vec(20, 15), "T3", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 343), module, MADDYPlus::TRACK3_OUTPUT));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(75, 365), Vec(20, 6), "12", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new MADDYPlusEnhancedTextLabel(Vec(75, 371), Vec(20, 6), "13", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 368), module, MADDYPlus::CHAIN_123_OUTPUT));


        float ch2OffsetX = 8 * RACK_GRID_WIDTH;

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 27), Vec(25, 10), "CLK SRC", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 15, 50), module, MADDYPlus::CH2_CLOCK_SOURCE_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 72), Vec(25, 10), "MODE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 15, 95), module, MADDYPlus::CH2_MODE_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 45), Vec(25, 10), "DENSITY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(ch2OffsetX + 45, 70), module, MADDYPlus::CH2_DENSITY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 115), Vec(25, 10), "CVD", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(ch2OffsetX + 15, 140), module, MADDYPlus::CH2_CVD_ATTEN_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 92), Vec(25, 10), "DELAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 45, 115), module, MADDYPlus::CH2_STEP_DELAY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 137), Vec(25, 10), "DCV IN", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(ch2OffsetX + 45, 160), module, MADDYPlus::CH2_CV_INPUT));


        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 174), Vec(25, 10), "CLK SRC", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 15, 197), module, MADDYPlus::CH3_CLOCK_SOURCE_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 219), Vec(25, 10), "MODE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 15, 242), module, MADDYPlus::CH3_MODE_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 192), Vec(25, 10), "DENSITY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(ch2OffsetX + 45, 217), module, MADDYPlus::CH3_DENSITY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 3, 262), Vec(25, 10), "CVD", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(ch2OffsetX + 15, 287), module, MADDYPlus::CH3_CVD_ATTEN_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 244), Vec(25, 10), "DELAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MADDYPlusSnapKnob>(Vec(ch2OffsetX + 45, 267), module, MADDYPlus::CH3_STEP_DELAY_PARAM));

        addChild(new MADDYPlusEnhancedTextLabel(Vec(ch2OffsetX + 33, 289), Vec(25, 10), "DCV IN", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(ch2OffsetX + 45, 312), module, MADDYPlus::CH3_CV_INPUT));


        addOutput(createOutputCentered<PJ301MPort>(Vec(ch2OffsetX + 15, 343), module, MADDYPlus::CH2_CV_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(ch2OffsetX + 15, 368), module, MADDYPlus::CH2_TRIG_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(ch2OffsetX + 45, 343), module, MADDYPlus::CH3_CV_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(ch2OffsetX + 45, 368), module, MADDYPlus::CH3_TRIG_OUTPUT));

        if (module) {
            delete module->paramQuantities[MADDYPlus::MODE_PARAM];
            Ch1ModeParamQuantity* ch1ModeQuantity = new Ch1ModeParamQuantity;
            ch1ModeQuantity->module = module;
            ch1ModeQuantity->paramId = MADDYPlus::MODE_PARAM;
            ch1ModeQuantity->minValue = 0.0f;
            ch1ModeQuantity->maxValue = 5.0f;
            ch1ModeQuantity->defaultValue = 1.0f;
            ch1ModeQuantity->name = "Ch1 Mode";
            ch1ModeQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::MODE_PARAM] = ch1ModeQuantity;

            delete module->paramQuantities[MADDYPlus::CLOCK_SOURCE_PARAM];
            Ch1ClockSourceParamQuantity* ch1ClockSourceQuantity = new Ch1ClockSourceParamQuantity;
            ch1ClockSourceQuantity->module = module;
            ch1ClockSourceQuantity->paramId = MADDYPlus::CLOCK_SOURCE_PARAM;
            ch1ClockSourceQuantity->minValue = 0.0f;
            ch1ClockSourceQuantity->maxValue = 6.0f;
            ch1ClockSourceQuantity->defaultValue = 0.0f;
            ch1ClockSourceQuantity->name = "Ch1 Clock Source";
            ch1ClockSourceQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::CLOCK_SOURCE_PARAM] = ch1ClockSourceQuantity;

            delete module->paramQuantities[MADDYPlus::CH2_CLOCK_SOURCE_PARAM];
            Ch2ClockSourceParamQuantity* ch2ClockSourceQuantity = new Ch2ClockSourceParamQuantity;
            ch2ClockSourceQuantity->module = module;
            ch2ClockSourceQuantity->paramId = MADDYPlus::CH2_CLOCK_SOURCE_PARAM;
            ch2ClockSourceQuantity->minValue = 0.0f;
            ch2ClockSourceQuantity->maxValue = 6.0f;
            ch2ClockSourceQuantity->defaultValue = 0.0f;
            ch2ClockSourceQuantity->name = "Ch2 Clock Source";
            ch2ClockSourceQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::CH2_CLOCK_SOURCE_PARAM] = ch2ClockSourceQuantity;

            delete module->paramQuantities[MADDYPlus::CH3_CLOCK_SOURCE_PARAM];
            Ch3ClockSourceParamQuantity* ch3ClockSourceQuantity = new Ch3ClockSourceParamQuantity;
            ch3ClockSourceQuantity->module = module;
            ch3ClockSourceQuantity->paramId = MADDYPlus::CH3_CLOCK_SOURCE_PARAM;
            ch3ClockSourceQuantity->minValue = 0.0f;
            ch3ClockSourceQuantity->maxValue = 6.0f;
            ch3ClockSourceQuantity->defaultValue = 0.0f;
            ch3ClockSourceQuantity->name = "Ch3 Clock Source";
            ch3ClockSourceQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::CH3_CLOCK_SOURCE_PARAM] = ch3ClockSourceQuantity;


            delete module->paramQuantities[MADDYPlus::CH2_MODE_PARAM];
            Ch2ModeParamQuantity* ch2ModeQuantity = new Ch2ModeParamQuantity;
            ch2ModeQuantity->module = module;
            ch2ModeQuantity->paramId = MADDYPlus::CH2_MODE_PARAM;
            ch2ModeQuantity->minValue = 0.0f;
            ch2ModeQuantity->maxValue = 5.0f;
            ch2ModeQuantity->defaultValue = 1.0f;
            ch2ModeQuantity->name = "Ch2 Mode";
            ch2ModeQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::CH2_MODE_PARAM] = ch2ModeQuantity;

            delete module->paramQuantities[MADDYPlus::CH3_MODE_PARAM];
            Ch3ModeParamQuantity* ch3ModeQuantity = new Ch3ModeParamQuantity;
            ch3ModeQuantity->module = module;
            ch3ModeQuantity->paramId = MADDYPlus::CH3_MODE_PARAM;
            ch3ModeQuantity->minValue = 0.0f;
            ch3ModeQuantity->maxValue = 5.0f;
            ch3ModeQuantity->defaultValue = 1.0f;
            ch3ModeQuantity->name = "Ch3 Mode";
            ch3ModeQuantity->snapEnabled = true;
            module->paramQuantities[MADDYPlus::CH3_MODE_PARAM] = ch3ModeQuantity;
          }
    }

    struct AttackTimeItem : ui::MenuItem {
        MADDYPlus* module;
        float attackTime;

        void onAction(const event::Action& e) override {
            if (module) {
                for (int i = 0; i < 3; ++i) {
                    module->tracks[i].attackTime = attackTime;
                }
            }
        }
    };

    struct PatternTextField : ui::TextField {
        MADDYPlus* module;
        int channelId;

        PatternTextField(MADDYPlus* module, int channelId) : module(module), channelId(channelId) {
            box.size.x = 200;
            placeholder = "e.g. 12312345";

            if (module && !module->customPattern[channelId].empty()) {
                text = "";
                for (int step : module->customPattern[channelId]) {
                    text += std::to_string(step + 1);
                }
            }
        }

        void onSelectKey(const event::SelectKey& e) override {
            if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
                if (!module) return;

                std::vector<int> steps;
                for (char c : text) {
                    if (c >= '1' && c <= '5') {
                        steps.push_back(c - '1');
                    }
                }

                if (!steps.empty()) {
                    module->customPattern[channelId] = steps;
                    module->customPatternIndex[channelId] = 0;

                    // Immediately regenerate mappings
                    if (channelId == 0) module->generateMapping();
                    else if (channelId == 1) module->generateCh2Mapping();
                    else if (channelId == 2) module->generateCh3Mapping();
                }

                ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
                if (overlay) {
                    overlay->requestDelete();
                }
            }
            TextField::onSelectKey(e);
        }
    };

    void step() override {
        MADDYPlus* module = dynamic_cast<MADDYPlus*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        MADDYPlus* module = getModule<MADDYPlus>();
        if (!module) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Custom Pattern"));

        const char* channelNames[3] = {"Channel 1", "Channel 2", "Channel 3"};
        for (int ch = 0; ch < 3; ch++) {
            menu->addChild(createMenuLabel(channelNames[ch]));

            struct PatternEnableItem : ui::MenuItem {
                MADDYPlus* module;
                int channelId;

                PatternEnableItem(MADDYPlus* module, int channelId) : module(module), channelId(channelId) {
                    text = "Use Custom Pattern";
                    rightText = CHECKMARK(module && module->useCustomPattern[channelId]);
                }

                void onAction(const event::Action& e) override {
                    if (module) {
                        module->useCustomPattern[channelId] = !module->useCustomPattern[channelId];
                    }
                }
            };

            menu->addChild(new PatternEnableItem(module, ch));

            menu->addChild(createMenuLabel("Pattern (1-5):"));
            menu->addChild(new PatternTextField(module, ch));

            if (!module->customPattern[ch].empty()) {
                std::string currentPattern = "Current: ";
                for (int step : module->customPattern[ch]) {
                    currentPattern += std::to_string(step + 1);
                }
                menu->addChild(createMenuLabel(currentPattern));
            }
        }

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Attack Time"));

        float currentAttackTime = module->tracks[0].attackTime;
        std::string currentLabel = string::f("Current: %.3fms", currentAttackTime * 1000.0f);
        menu->addChild(createMenuLabel(currentLabel));

        struct AttackTimeSlider : ui::Slider {
            struct AttackTimeQuantity : Quantity {
                MADDYPlus* module;
                AttackTimeQuantity(MADDYPlus* module) : module(module) {}

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

            AttackTimeSlider(MADDYPlus* module) {
                box.size.x = 200.0f;
                quantity = new AttackTimeQuantity(module);
            }

            ~AttackTimeSlider() {
                delete quantity;
            }
        };

        AttackTimeSlider* slider = new AttackTimeSlider(module);

        struct AttackTimeDisplay : ui::MenuLabel {
            MADDYPlus* module;

            AttackTimeDisplay(MADDYPlus* module) : module(module) {
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


        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Shift Settings"));

        for (int trackId = 0; trackId < 3; trackId++) {
            std::string trackLabel = string::f("Track %d Shift", trackId + 1);
            menu->addChild(createMenuLabel(trackLabel));


            struct TrackShiftMenu : MenuItem {
                MADDYPlus* module;
                int trackIndex;

                TrackShiftMenu(MADDYPlus* module, int trackIndex) : module(module), trackIndex(trackIndex) {
                    text = string::f("Track %d Shift", trackIndex + 1);
                    rightText = string::f("%d step", module ? module->tracks[trackIndex].shift : 0);
                }

                Menu* createChildMenu() override {
                    Menu* menu = new Menu();

                    for (int shift = 0; shift <= 4; shift++) {
                        struct ShiftMenuItem : MenuItem {
                            MADDYPlus* module;
                            int trackIndex;
                            int shiftValue;

                            ShiftMenuItem(MADDYPlus* module, int trackIndex, int shiftValue)
                                : module(module), trackIndex(trackIndex), shiftValue(shiftValue) {
                                text = string::f("%d step", shiftValue);
                                if (module && module->tracks[trackIndex].shift == shiftValue) {
                                    rightText = CHECKMARK_STRING;
                                }
                            }

                            void onAction(const event::Action& e) override {
                                if (module && trackIndex >= 0 && trackIndex < 3) {
                                    module->tracks[trackIndex].shift = shiftValue;
                                }
                            }
                        };

                        menu->addChild(new ShiftMenuItem(module, trackIndex, shift));
                    }

                    return menu;
                }
            };

            menu->addChild(new TrackShiftMenu(module, trackId));
        }

        addPanelThemeMenu(menu, module);
    }
};

Model* modelMADDYPlus = createModel<MADDYPlus, MADDYPlusWidget>("MADDYPlus");