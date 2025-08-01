#include "plugin.hpp"

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

struct GreenTrainCarWidget : Widget {
    GreenTrainCarWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        // Train car body (green)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 2, 7, box.size.x - 4, box.size.y - 10);
        nvgFillColor(args.vg, nvgRGB(128, 195, 66));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(150, 150, 150));
        nvgStroke(args.vg);
        
        // Four small windows (pink)
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

struct GreenBackgroundBox : Widget {
    GreenBackgroundBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(128, 195, 66));
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

struct YAMANOTE : Module {
    enum ParamId {
        CH1_SEND_A_PARAM,
        CH1_SEND_B_PARAM,
        CH2_SEND_A_PARAM,
        CH2_SEND_B_PARAM,
        CH3_SEND_A_PARAM,
        CH3_SEND_B_PARAM,
        CH4_SEND_A_PARAM,
        CH4_SEND_B_PARAM,
        CH5_SEND_A_PARAM,
        CH5_SEND_B_PARAM,
        CH6_SEND_A_PARAM,
        CH6_SEND_B_PARAM,
        CH7_SEND_A_PARAM,
        CH7_SEND_B_PARAM,
        CH8_SEND_A_PARAM,
        CH8_SEND_B_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CH1_L_INPUT,
        CH1_R_INPUT,
        CH2_L_INPUT,
        CH2_R_INPUT,
        CH3_L_INPUT,
        CH3_R_INPUT,
        CH4_L_INPUT,
        CH4_R_INPUT,
        CH5_L_INPUT,
        CH5_R_INPUT,
        CH6_L_INPUT,
        CH6_R_INPUT,
        CH7_L_INPUT,
        CH7_R_INPUT,
        CH8_L_INPUT,
        CH8_R_INPUT,
        CHAIN_L_INPUT,
        CHAIN_R_INPUT,
        RETURN_A_L_INPUT,
        RETURN_A_R_INPUT,
        RETURN_B_L_INPUT,
        RETURN_B_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        SEND_A_L_OUTPUT,
        SEND_A_R_OUTPUT,
        SEND_B_L_OUTPUT,
        SEND_B_R_OUTPUT,
        MIX_L_OUTPUT,
        MIX_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    YAMANOTE() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        for (int i = 0; i < 8; ++i) {
            configParam(CH1_SEND_A_PARAM + i * 2, 0.0f, 1.0f, 0.0f, string::f("CH%d Send A", i + 1));
            configParam(CH1_SEND_B_PARAM + i * 2, 0.0f, 1.0f, 0.0f, string::f("CH%d Send B", i + 1));
            
            configInput(CH1_L_INPUT + i * 2, string::f("CH%d Left", i + 1));
            configInput(CH1_R_INPUT + i * 2, string::f("CH%d Right", i + 1));
        }
        
        configInput(CHAIN_L_INPUT, "Chain Left");
        configInput(CHAIN_R_INPUT, "Chain Right");
        configInput(RETURN_A_L_INPUT, "Return A Left");
        configInput(RETURN_A_R_INPUT, "Return A Right");
        configInput(RETURN_B_L_INPUT, "Return B Left");
        configInput(RETURN_B_R_INPUT, "Return B Right");
        
        configOutput(SEND_A_L_OUTPUT, "Send A Left");
        configOutput(SEND_A_R_OUTPUT, "Send A Right");
        configOutput(SEND_B_L_OUTPUT, "Send B Left");
        configOutput(SEND_B_R_OUTPUT, "Send B Right");
        configOutput(MIX_L_OUTPUT, "Mix Left");
        configOutput(MIX_R_OUTPUT, "Mix Right");
    }

    void process(const ProcessArgs& args) override {
        float sendAL = 0.0f, sendAR = 0.0f;
        float sendBL = 0.0f, sendBR = 0.0f;
        float mixL = 0.0f, mixR = 0.0f;
        
        for (int i = 0; i < 8; ++i) {
            float inputL = inputs[CH1_L_INPUT + i * 2].getVoltage();
            float inputR = inputs[CH1_R_INPUT + i * 2].getVoltage();
            
            if (inputs[CH1_L_INPUT + i * 2].isConnected() && !inputs[CH1_R_INPUT + i * 2].isConnected()) {
                inputR = inputL;
            }
            
            float sendALevel = params[CH1_SEND_A_PARAM + i * 2].getValue();
            float sendBLevel = params[CH1_SEND_B_PARAM + i * 2].getValue();
            
            sendAL += inputL * sendALevel;
            sendAR += inputR * sendALevel;
            sendBL += inputL * sendBLevel;
            sendBR += inputR * sendBLevel;
        }
        
        outputs[SEND_A_L_OUTPUT].setVoltage(sendAL);
        outputs[SEND_A_R_OUTPUT].setVoltage(sendAR);
        outputs[SEND_B_L_OUTPUT].setVoltage(sendBL);
        outputs[SEND_B_R_OUTPUT].setVoltage(sendBR);
        
        float returnAL = inputs[RETURN_A_L_INPUT].getVoltage();
        float returnAR = inputs[RETURN_A_R_INPUT].getVoltage();
        float returnBL = inputs[RETURN_B_L_INPUT].getVoltage();
        float returnBR = inputs[RETURN_B_R_INPUT].getVoltage();
        
        float chainL = inputs[CHAIN_L_INPUT].getVoltage();
        float chainR = inputs[CHAIN_R_INPUT].getVoltage();
        
        mixL += returnAL + returnBL + chainL;
        mixR += returnAR + returnBR + chainR;
        
        outputs[MIX_L_OUTPUT].setVoltage(mixL);
        outputs[MIX_R_OUTPUT].setVoltage(mixR);
    }
    
    void processBypass(const ProcessArgs& args) override {
        float chainL = inputs[CHAIN_L_INPUT].getVoltage();
        float chainR = inputs[CHAIN_R_INPUT].getVoltage();
        
        outputs[MIX_L_OUTPUT].setVoltage(chainL);
        outputs[MIX_R_OUTPUT].setVoltage(chainR);
    }
};

struct YAMANOTEWidget : ModuleWidget {
    YAMANOTEWidget(YAMANOTE* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new GreenBackgroundBox(Vec(0, 1), Vec(box.size.x, 18)));
        addChild(new GreenTrainCarWidget(Vec(0, 31), Vec(box.size.x, 35)));
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "YAMANOTE", 12.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        float startY = 52;
        float rowHeight = 33;
        
        for (int i = 0; i < 8; ++i) {
            float y = startY + i * rowHeight;
            
            addChild(new EnhancedTextLabel(Vec(5, y - 24), Vec(20, 15), "L", 7.f, nvgRGB(255, 255, 255), true));
            addInput(createInputCentered<PJ301MPort>(Vec(15, y), module, YAMANOTE::CH1_L_INPUT + i * 2));
            
            addChild(new EnhancedTextLabel(Vec(35, y - 24), Vec(20, 15), "R", 7.f, nvgRGB(255, 255, 255), true));
            addInput(createInputCentered<PJ301MPort>(Vec(45, y), module, YAMANOTE::CH1_R_INPUT + i * 2));
            
            addChild(new EnhancedTextLabel(Vec(65, y - 24), Vec(20, 15), "SendA", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob>(Vec(75, y), module, YAMANOTE::CH1_SEND_A_PARAM + i * 2));
            
            addChild(new EnhancedTextLabel(Vec(95, y - 24), Vec(20, 15), "SendB", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob>(Vec(105, y), module, YAMANOTE::CH1_SEND_B_PARAM + i * 2));
        }

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 330)));
        
        addChild(new EnhancedTextLabel(Vec(18, 292), Vec(30, 15), "SEND A", 6.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 315), module, YAMANOTE::SEND_A_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 315), module, YAMANOTE::SEND_A_R_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(77, 292), Vec(30, 15), "SEND B", 6.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(75, 315), module, YAMANOTE::SEND_B_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 315), module, YAMANOTE::SEND_B_R_OUTPUT));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 343), module, YAMANOTE::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 368), module, YAMANOTE::MIX_R_OUTPUT));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, YAMANOTE::CHAIN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, YAMANOTE::CHAIN_R_INPUT));
        
        addInput(createInputCentered<PJ301MPort>(Vec(45, 343), module, YAMANOTE::RETURN_A_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 368), module, YAMANOTE::RETURN_A_R_INPUT));
        
        addInput(createInputCentered<PJ301MPort>(Vec(75, 343), module, YAMANOTE::RETURN_B_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(75, 368), module, YAMANOTE::RETURN_B_R_INPUT));
    }
};

Model* modelYAMANOTE = createModel<YAMANOTE, YAMANOTEWidget>("YAMANOTE");