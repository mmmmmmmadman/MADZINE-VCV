#include "plugin.hpp"

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
        box.size = Vec(45, 45);
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

struct TrainCarWidget : Widget {
    TrainCarWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 2, 7, box.size.x - 4, box.size.y - 10);
        nvgFillColor(args.vg, nvgRGB(255, 204, 0));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(150, 150, 150));
        nvgStroke(args.vg);
        
        float originalWindowWidth = (box.size.x - 16) / 2;
        float smallWindowWidth = originalWindowWidth / 5 * 2.5;
        float windowHeight = (box.size.y - 12) / 5 * 2.5;
        float spacing = (box.size.x - 4 * smallWindowWidth - 4) / 5;
        
        for (int i = 0; i < 4; i++) {
            float windowX = 2 + spacing + i * (smallWindowWidth + spacing);
            float windowY = 11;
            
            nvgBeginPath(args.vg);
            nvgRect(args.vg, windowX, windowY, smallWindowWidth, windowHeight);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            nvgFill(args.vg);
        }
    }
};

struct BlueBackgroundBox : Widget {
    BlueBackgroundBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(34, 79, 134));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 255));
        nvgStroke(args.vg);
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

struct U8 : Module {
    enum ParamId {
        LEVEL_PARAM,
        DUCK_LEVEL_PARAM,
        MUTE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        LEFT_INPUT,
        RIGHT_INPUT,
        DUCK_INPUT,
        LEVEL_CV_INPUT,
        MUTE_TRIG_INPUT,
        CHAIN_LEFT_INPUT,
        CHAIN_RIGHT_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        MUTE_LIGHT,
        LIGHTS_LEN
    };

    static constexpr int DELAY_BUFFER_SIZE = 2048;
    float delayBuffer[DELAY_BUFFER_SIZE];
    int delayWriteIndex = 0;

    bool muteState = false;
    dsp::SchmittTrigger muteTrigger;

    U8() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(LEVEL_PARAM, 0.0f, 2.0f, 1.0f, "Level");
        configParam(DUCK_LEVEL_PARAM, 0.0f, 1.0f, 0.0f, "Duck Level");
        configSwitch(MUTE_PARAM, 0.0f, 1.0f, 0.0f, "Mute", {"Unmuted", "Muted"});
        
        configInput(LEFT_INPUT, "Left Audio");
        configInput(RIGHT_INPUT, "Right Audio");
        configInput(DUCK_INPUT, "Duck Signal");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(MUTE_TRIG_INPUT, "Mute Trigger");
        configInput(CHAIN_LEFT_INPUT, "Chain Left");
        configInput(CHAIN_RIGHT_INPUT, "Chain Right");
        
        configOutput(LEFT_OUTPUT, "Left Audio");
        configOutput(RIGHT_OUTPUT, "Right Audio");
        
        configLight(MUTE_LIGHT, "Mute Indicator");
        
        for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
            delayBuffer[i] = 0.0f;
        }
    }

    void process(const ProcessArgs& args) override {
        if (inputs[MUTE_TRIG_INPUT].isConnected()) {
            if (muteTrigger.process(inputs[MUTE_TRIG_INPUT].getVoltage())) {
                muteState = !muteState;
                params[MUTE_PARAM].setValue(muteState ? 1.0f : 0.0f);
            }
        }
        
        float leftInput = inputs[LEFT_INPUT].getVoltage();
        float rightInput;
        
        bool leftConnected = inputs[LEFT_INPUT].isConnected();
        bool rightConnected = inputs[RIGHT_INPUT].isConnected();
        
        if (leftConnected && !rightConnected) {
            int delaySamples = (int)(0.02f * args.sampleRate);
            delaySamples = clamp(delaySamples, 1, DELAY_BUFFER_SIZE - 1);
            
            int readIndex = (delayWriteIndex - delaySamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
            rightInput = delayBuffer[readIndex];
            
            delayBuffer[delayWriteIndex] = leftInput;
            delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SIZE;
        } else if (rightConnected) {
            rightInput = inputs[RIGHT_INPUT].getVoltage();
        } else {
            rightInput = leftInput;
        }
        
        float duckCV = clamp(inputs[DUCK_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        float duckAmount = params[DUCK_LEVEL_PARAM].getValue();
        float sidechainCV = clamp(1.0f - (duckCV * duckAmount * 3.0f), 0.0f, 1.0f);
        
        float levelParam = params[LEVEL_PARAM].getValue();
        if (inputs[LEVEL_CV_INPUT].isConnected()) {
            levelParam += inputs[LEVEL_CV_INPUT].getVoltage() / 5.0f;
            levelParam = clamp(levelParam, 0.0f, 2.0f);
        }
        
        leftInput *= levelParam * sidechainCV;
        rightInput *= levelParam * sidechainCV;
        
        bool muted = params[MUTE_PARAM].getValue() > 0.5f;
        lights[MUTE_LIGHT].setBrightness(muted ? 1.0f : 0.0f);
        
        if (muted) {
            leftInput = 0.0f;
            rightInput = 0.0f;
        }
        
        float chainLeftInput = inputs[CHAIN_LEFT_INPUT].getVoltage();
        float chainRightInput = inputs[CHAIN_RIGHT_INPUT].getVoltage();
        
        outputs[LEFT_OUTPUT].setVoltage(leftInput + chainLeftInput);
        outputs[RIGHT_OUTPUT].setVoltage(rightInput + chainRightInput);
    }
    
    void processBypass(const ProcessArgs& args) override {
        float chainLeftInput = inputs[CHAIN_LEFT_INPUT].getVoltage();
        float chainRightInput = inputs[CHAIN_RIGHT_INPUT].getVoltage();
        
        outputs[LEFT_OUTPUT].setVoltage(chainLeftInput);
        outputs[RIGHT_OUTPUT].setVoltage(chainRightInput);
    }
};

struct U8Widget : ModuleWidget {
    U8Widget(U8* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/SwingLFO.svg")));
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new BlueBackgroundBox(Vec(0, 1), Vec(box.size.x, 18)));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "U8", 14.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new TrainCarWidget(Vec(0, 31), Vec(box.size.x, 35)));

        addChild(new TechnoEnhancedTextLabel(Vec(0, 28), Vec(box.size.x, 16), "INPUT", 8.f, nvgRGB(255, 255, 255), true));

        addChild(new TechnoEnhancedTextLabel(Vec(-5, 89), Vec(box.size.x + 10, 10), "LEVEL", 10.5f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(box.size.x / 2, 123), module, U8::LEVEL_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2, 161), module, U8::LEVEL_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-5, 182), Vec(box.size.x + 10, 10), "DUCK", 10.5f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(box.size.x / 2, 216), module, U8::DUCK_LEVEL_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2, 254), module, U8::DUCK_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-5, 270), Vec(box.size.x + 10, 10), "MUTE", 10.5f, nvgRGB(255, 255, 255), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(Vec(box.size.x / 2, 292), module, U8::MUTE_PARAM, U8::MUTE_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2, 316), module, U8::MUTE_TRIG_INPUT));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 59), module, U8::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x - 15, 59), module, U8::RIGHT_INPUT));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 60)));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, U8::CHAIN_LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, U8::CHAIN_RIGHT_INPUT));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, 343), module, U8::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, 368), module, U8::RIGHT_OUTPUT));
    }
};

Model* modelU8 = createModel<U8, U8Widget>("U8");