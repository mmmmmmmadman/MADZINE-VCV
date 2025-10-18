#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

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

// TechnoStandardBlackKnob 現在從 widgets/Knobs.hpp 引入

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
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

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
    static constexpr int MAX_POLY = 16;
    float delayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    int delayWriteIndex[MAX_POLY];

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

        // Initialize delay buffers
        for (int c = 0; c < MAX_POLY; c++) {
            for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
                delayBuffer[c][i] = 0.0f;
            }
            delayWriteIndex[c] = 0;
        }
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
        // Handle mute trigger (monophonic)
        if (inputs[MUTE_TRIG_INPUT].isConnected()) {
            if (muteTrigger.process(inputs[MUTE_TRIG_INPUT].getVoltage())) {
                muteState = !muteState;
                params[MUTE_PARAM].setValue(muteState ? 1.0f : 0.0f);
            }
        }

        bool muted = params[MUTE_PARAM].getValue() > 0.5f;
        lights[MUTE_LIGHT].setBrightness(muted ? 1.0f : 0.0f);

        // Get polyphonic channel counts
        int leftChannels = inputs[LEFT_INPUT].getChannels();
        int rightChannels = inputs[RIGHT_INPUT].getChannels();
        int chainLeftChannels = inputs[CHAIN_LEFT_INPUT].getChannels();
        int chainRightChannels = inputs[CHAIN_RIGHT_INPUT].getChannels();

        // Determine output channel count
        int outputLeftChannels = std::max({leftChannels, chainLeftChannels, 1});
        int outputRightChannels = std::max({rightChannels, chainRightChannels, 1});

        // If left is connected but right isn't, use delay for stereo effect
        bool useDelay = inputs[LEFT_INPUT].isConnected() && !inputs[RIGHT_INPUT].isConnected();
        if (useDelay) {
            outputRightChannels = outputLeftChannels;
        }

        outputs[LEFT_OUTPUT].setChannels(outputLeftChannels);
        outputs[RIGHT_OUTPUT].setChannels(outputRightChannels);

        // Get duck and level parameters (can be polyphonic)
        int duckChannels = inputs[DUCK_INPUT].getChannels();
        int levelCvChannels = inputs[LEVEL_CV_INPUT].getChannels();

        float levelParam = params[LEVEL_PARAM].getValue();
        float duckAmount = params[DUCK_LEVEL_PARAM].getValue();

        // Process left output channels
        for (int c = 0; c < outputLeftChannels; c++) {
            float leftInput = (c < leftChannels) ? inputs[LEFT_INPUT].getPolyVoltage(c) : 0.0f;
            float chainLeftInput = (c < chainLeftChannels) ? inputs[CHAIN_LEFT_INPUT].getPolyVoltage(c) : 0.0f;

            // Apply ducking (use matching channel or channel 0)
            float duckCV = 0.0f;
            if (inputs[DUCK_INPUT].isConnected()) {
                int duckChan = (c < duckChannels) ? c : 0;
                duckCV = clamp(inputs[DUCK_INPUT].getPolyVoltage(duckChan) / 10.0f, 0.0f, 1.0f);
            }
            float sidechainCV = clamp(1.0f - (duckCV * duckAmount * 3.0f), 0.0f, 1.0f);

            // Apply level control
            float level = levelParam;
            if (inputs[LEVEL_CV_INPUT].isConnected()) {
                int levelChan = (c < levelCvChannels) ? c : 0;
                float cvLevel = clamp(inputs[LEVEL_CV_INPUT].getPolyVoltage(levelChan) / 10.0f, 0.0f, 1.0f);
                level = levelParam * cvLevel;
            }

            leftInput *= level * sidechainCV;

            if (muted) {
                leftInput = 0.0f;
            }

            outputs[LEFT_OUTPUT].setVoltage(leftInput + chainLeftInput, c);
        }

        // Process right output channels
        for (int c = 0; c < outputRightChannels; c++) {
            float rightInput = 0.0f;

            if (useDelay && c < leftChannels) {
                // Use delay for right channel when only left is connected
                int delaySamples = (int)(0.02f * args.sampleRate);
                delaySamples = clamp(delaySamples, 1, DELAY_BUFFER_SIZE - 1);

                int readIndex = (delayWriteIndex[c] - delaySamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
                rightInput = delayBuffer[c][readIndex];

                // Store left input in delay buffer
                delayBuffer[c][delayWriteIndex[c]] = inputs[LEFT_INPUT].getPolyVoltage(c);
                delayWriteIndex[c] = (delayWriteIndex[c] + 1) % DELAY_BUFFER_SIZE;
            } else if (c < rightChannels) {
                rightInput = inputs[RIGHT_INPUT].getPolyVoltage(c);
            }

            float chainRightInput = (c < chainRightChannels) ? inputs[CHAIN_RIGHT_INPUT].getPolyVoltage(c) : 0.0f;

            // Apply ducking
            float duckCV = 0.0f;
            if (inputs[DUCK_INPUT].isConnected()) {
                int duckChan = (c < duckChannels) ? c : 0;
                duckCV = clamp(inputs[DUCK_INPUT].getPolyVoltage(duckChan) / 10.0f, 0.0f, 1.0f);
            }
            float sidechainCV = clamp(1.0f - (duckCV * duckAmount * 3.0f), 0.0f, 1.0f);

            // Apply level control
            float level = levelParam;
            if (inputs[LEVEL_CV_INPUT].isConnected()) {
                int levelChan = (c < levelCvChannels) ? c : 0;
                float cvLevel = clamp(inputs[LEVEL_CV_INPUT].getPolyVoltage(levelChan) / 10.0f, 0.0f, 1.0f);
                level = levelParam * cvLevel;
            }

            rightInput *= level * sidechainCV;

            if (muted) {
                rightInput = 0.0f;
            }

            outputs[RIGHT_OUTPUT].setVoltage(rightInput + chainRightInput, c);
        }
    }

    void processBypass(const ProcessArgs& args) override {
        int chainLeftChannels = inputs[CHAIN_LEFT_INPUT].getChannels();
        int chainRightChannels = inputs[CHAIN_RIGHT_INPUT].getChannels();

        outputs[LEFT_OUTPUT].setChannels(chainLeftChannels);
        outputs[RIGHT_OUTPUT].setChannels(chainRightChannels);

        for (int c = 0; c < chainLeftChannels; c++) {
            outputs[LEFT_OUTPUT].setVoltage(inputs[CHAIN_LEFT_INPUT].getPolyVoltage(c), c);
        }

        for (int c = 0; c < chainRightChannels; c++) {
            outputs[RIGHT_OUTPUT].setVoltage(inputs[CHAIN_RIGHT_INPUT].getPolyVoltage(c), c);
        }
    }
};

struct U8Widget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    U8Widget(U8* module) {
        setModule(module);
        panelThemeHelper.init(this, "EuclideanRhythm");

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

    void step() override {
        U8* module = dynamic_cast<U8*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        U8* module = dynamic_cast<U8*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelU8 = createModel<U8, U8Widget>("U8");