#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

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

// StandardBlackKnob 現在從 widgets/Knobs.hpp 引入
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
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

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

    static constexpr int MAX_POLY = 16;

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
        // Determine maximum polyphonic channels across all inputs
        int maxChannels = 1;

        // Check channel inputs
        for (int i = 0; i < 8; ++i) {
            int leftChannels = inputs[CH1_L_INPUT + i * 2].getChannels();
            int rightChannels = inputs[CH1_R_INPUT + i * 2].getChannels();
            maxChannels = std::max({maxChannels, leftChannels, rightChannels});
        }

        // Check return and chain inputs
        maxChannels = std::max({maxChannels,
            inputs[CHAIN_L_INPUT].getChannels(),
            inputs[CHAIN_R_INPUT].getChannels(),
            inputs[RETURN_A_L_INPUT].getChannels(),
            inputs[RETURN_A_R_INPUT].getChannels(),
            inputs[RETURN_B_L_INPUT].getChannels(),
            inputs[RETURN_B_R_INPUT].getChannels()
        });

        // Set output channels
        outputs[SEND_A_L_OUTPUT].setChannels(maxChannels);
        outputs[SEND_A_R_OUTPUT].setChannels(maxChannels);
        outputs[SEND_B_L_OUTPUT].setChannels(maxChannels);
        outputs[SEND_B_R_OUTPUT].setChannels(maxChannels);
        outputs[MIX_L_OUTPUT].setChannels(maxChannels);
        outputs[MIX_R_OUTPUT].setChannels(maxChannels);

        // Process each polyphonic channel
        for (int c = 0; c < maxChannels; c++) {
            float sendAL = 0.0f, sendAR = 0.0f;
            float sendBL = 0.0f, sendBR = 0.0f;
            float mixL = 0.0f, mixR = 0.0f;

            // Process each input channel
            for (int i = 0; i < 8; ++i) {
                int leftChannels = inputs[CH1_L_INPUT + i * 2].getChannels();
                int rightChannels = inputs[CH1_R_INPUT + i * 2].getChannels();

                float inputL = 0.0f, inputR = 0.0f;

                // Get input voltages (use channel 0 if current channel doesn't exist)
                if (inputs[CH1_L_INPUT + i * 2].isConnected()) {
                    int useChan = (c < leftChannels) ? c : 0;
                    inputL = inputs[CH1_L_INPUT + i * 2].getPolyVoltage(useChan);
                }

                if (inputs[CH1_R_INPUT + i * 2].isConnected()) {
                    int useChan = (c < rightChannels) ? c : 0;
                    inputR = inputs[CH1_R_INPUT + i * 2].getPolyVoltage(useChan);
                } else if (inputs[CH1_L_INPUT + i * 2].isConnected()) {
                    // If only left is connected, use it for right as well
                    inputR = inputL;
                }

                float sendALevel = params[CH1_SEND_A_PARAM + i * 2].getValue();
                float sendBLevel = params[CH1_SEND_B_PARAM + i * 2].getValue();

                sendAL += inputL * sendALevel;
                sendAR += inputR * sendALevel;
                sendBL += inputL * sendBLevel;
                sendBR += inputR * sendBLevel;
            }

            // Set send outputs
            outputs[SEND_A_L_OUTPUT].setVoltage(sendAL, c);
            outputs[SEND_A_R_OUTPUT].setVoltage(sendAR, c);
            outputs[SEND_B_L_OUTPUT].setVoltage(sendBL, c);
            outputs[SEND_B_R_OUTPUT].setVoltage(sendBR, c);

            // Process returns and chain
            float returnAL = 0.0f, returnAR = 0.0f;
            float returnBL = 0.0f, returnBR = 0.0f;
            float chainL = 0.0f, chainR = 0.0f;

            if (inputs[RETURN_A_L_INPUT].isConnected()) {
                int chanCount = inputs[RETURN_A_L_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                returnAL = inputs[RETURN_A_L_INPUT].getPolyVoltage(useChan);
            }

            if (inputs[RETURN_A_R_INPUT].isConnected()) {
                int chanCount = inputs[RETURN_A_R_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                returnAR = inputs[RETURN_A_R_INPUT].getPolyVoltage(useChan);
            }

            if (inputs[RETURN_B_L_INPUT].isConnected()) {
                int chanCount = inputs[RETURN_B_L_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                returnBL = inputs[RETURN_B_L_INPUT].getPolyVoltage(useChan);
            }

            if (inputs[RETURN_B_R_INPUT].isConnected()) {
                int chanCount = inputs[RETURN_B_R_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                returnBR = inputs[RETURN_B_R_INPUT].getPolyVoltage(useChan);
            }

            if (inputs[CHAIN_L_INPUT].isConnected()) {
                int chanCount = inputs[CHAIN_L_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                chainL = inputs[CHAIN_L_INPUT].getPolyVoltage(useChan);
            }

            if (inputs[CHAIN_R_INPUT].isConnected()) {
                int chanCount = inputs[CHAIN_R_INPUT].getChannels();
                int useChan = (c < chanCount) ? c : 0;
                chainR = inputs[CHAIN_R_INPUT].getPolyVoltage(useChan);
            }

            // Mix outputs
            mixL = returnAL + returnBL + chainL;
            mixR = returnAR + returnBR + chainR;

            outputs[MIX_L_OUTPUT].setVoltage(mixL, c);
            outputs[MIX_R_OUTPUT].setVoltage(mixR, c);
        }
    }

    void processBypass(const ProcessArgs& args) override {
        int chainLeftChannels = inputs[CHAIN_L_INPUT].getChannels();
        int chainRightChannels = inputs[CHAIN_R_INPUT].getChannels();
        int maxChannels = std::max(chainLeftChannels, chainRightChannels);

        outputs[MIX_L_OUTPUT].setChannels(maxChannels);
        outputs[MIX_R_OUTPUT].setChannels(maxChannels);

        for (int c = 0; c < maxChannels; c++) {
            float chainL = (c < chainLeftChannels) ? inputs[CHAIN_L_INPUT].getPolyVoltage(c) : 0.0f;
            float chainR = (c < chainRightChannels) ? inputs[CHAIN_R_INPUT].getPolyVoltage(c) : 0.0f;

            outputs[MIX_L_OUTPUT].setVoltage(chainL, c);
            outputs[MIX_R_OUTPUT].setVoltage(chainR, c);
        }
    }
};

struct YAMANOTEWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    YAMANOTEWidget(YAMANOTE* module) {
        setModule(module);
        panelThemeHelper.init(this, "EuclideanRhythm");

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
            addParam(createParamCentered<StandardBlackKnob26>(Vec(75, y), module, YAMANOTE::CH1_SEND_A_PARAM + i * 2));

            addChild(new EnhancedTextLabel(Vec(95, y - 24), Vec(20, 15), "SendB", 7.f, nvgRGB(255, 255, 255), true));
            addParam(createParamCentered<StandardBlackKnob26>(Vec(105, y), module, YAMANOTE::CH1_SEND_B_PARAM + i * 2));
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

    void step() override {
        YAMANOTE* module = dynamic_cast<YAMANOTE*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        YAMANOTE* module = dynamic_cast<YAMANOTE*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelYAMANOTE = createModel<YAMANOTE, YAMANOTEWidget>("YAMANOTE");