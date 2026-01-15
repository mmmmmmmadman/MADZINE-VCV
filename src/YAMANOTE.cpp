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

        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
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
    int panelTheme = -1;
    float panelContrast = panelContrastDefault; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

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

    // Expander 輸出資料（供右側 YAMANOTE 模組讀取）
    float expanderOutputL[MAX_POLY] = {0};
    float expanderOutputR[MAX_POLY] = {0};
    int expanderOutputChannels = 0;

    // Send 累積資料（供右側 YAMANOTE 讀取）
    float expanderSendAL[MAX_POLY] = {0};
    float expanderSendAR[MAX_POLY] = {0};
    float expanderSendBL[MAX_POLY] = {0};
    float expanderSendBR[MAX_POLY] = {0};
    int expanderSendChannels = 0;

    // U8 追蹤資訊（供右側 YAMANOTE 讀取）
    int totalU8HandledByThis = 0;  // 此 YAMANOTE 處理的 U8 數量

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
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) {
            panelContrast = json_real_value(contrastJ);
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

            // 從左邊 YAMANOTE 讀取 Send 累積信號（透過 Expander）
            float leftSendAL = 0.0f, leftSendAR = 0.0f;
            float leftSendBL = 0.0f, leftSendBR = 0.0f;
            Module* leftMod = leftExpander.module;
            if (leftMod && leftMod->model == modelYAMANOTE) {
                YAMANOTE* leftYam = dynamic_cast<YAMANOTE*>(leftMod);
                if (leftYam && c < leftYam->expanderSendChannels) {
                    leftSendAL = leftYam->expanderSendAL[c];
                    leftSendAR = leftYam->expanderSendAR[c];
                    leftSendBL = leftYam->expanderSendBL[c];
                    leftSendBR = leftYam->expanderSendBR[c];
                }
            }

            // Send 輸出 = 自己的 CH Send + 左邊累積的 Send
            float totalSendAL = sendAL + leftSendAL;
            float totalSendAR = sendAR + leftSendAR;
            float totalSendBL = sendBL + leftSendBL;
            float totalSendBR = sendBR + leftSendBR;

            outputs[SEND_A_L_OUTPUT].setVoltage(totalSendAL, c);
            outputs[SEND_A_R_OUTPUT].setVoltage(totalSendAR, c);
            outputs[SEND_B_L_OUTPUT].setVoltage(totalSendBL, c);
            outputs[SEND_B_R_OUTPUT].setVoltage(totalSendBR, c);

            // 儲存 Send 累積值供右邊 YAMANOTE 讀取
            expanderSendAL[c] = totalSendAL;
            expanderSendAR[c] = totalSendAR;
            expanderSendBL[c] = totalSendBL;
            expanderSendBR[c] = totalSendBR;

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

        // 儲存 output 給右側 YAMANOTE 模組
        expanderOutputChannels = maxChannels;
        expanderSendChannels = maxChannels;
        for (int c = 0; c < maxChannels; c++) {
            expanderOutputL[c] = outputs[MIX_L_OUTPUT].getVoltage(c);
            expanderOutputR[c] = outputs[MIX_R_OUTPUT].getVoltage(c);
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

    // 自動 chain cable 追蹤
    int64_t autoChainLeftCableId = -1;
    int64_t autoChainRightCableId = -1;
    Module* lastRightExpander = nullptr;

    // 自動 CH input cable 追蹤（最多 8 個 U8，每個有 L/R）
    static constexpr int MAX_U8_COUNT = 8;
    int64_t autoInputCableIds[MAX_U8_COUNT][2];
    Module* lastLeftU8s[MAX_U8_COUNT] = {nullptr};
    int64_t lastU8InputSourceIds[MAX_U8_COUNT][2];

    // Send/Return port widgets（用於隱藏/顯示）
    app::PortWidget* sendALPort = nullptr;
    app::PortWidget* sendARPort = nullptr;
    app::PortWidget* sendBLPort = nullptr;
    app::PortWidget* sendBRPort = nullptr;
    app::PortWidget* returnALPort = nullptr;
    app::PortWidget* returnARPort = nullptr;
    app::PortWidget* returnBLPort = nullptr;
    app::PortWidget* returnBRPort = nullptr;
    widget::Widget* sendALabel = nullptr;
    widget::Widget* sendBLabel = nullptr;
    bool lastRightIsYamanote = false;

    YAMANOTEWidget(YAMANOTE* module) {
        // 初始化陣列為 -1
        for (int i = 0; i < MAX_U8_COUNT; i++) {
            autoInputCableIds[i][0] = -1;
            autoInputCableIds[i][1] = -1;
            lastU8InputSourceIds[i][0] = -1;
            lastU8InputSourceIds[i][1] = -1;
        }
        setModule(module);
        panelThemeHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

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

        // Send A（儲存參考以便隱藏/顯示）
        sendALabel = new EnhancedTextLabel(Vec(18, 292), Vec(30, 15), "SEND A", 6.f, nvgRGB(255, 255, 255), true);
        addChild(sendALabel);
        sendALPort = createOutputCentered<PJ301MPort>(Vec(15, 315), module, YAMANOTE::SEND_A_L_OUTPUT);
        addOutput(sendALPort);
        sendARPort = createOutputCentered<PJ301MPort>(Vec(45, 315), module, YAMANOTE::SEND_A_R_OUTPUT);
        addOutput(sendARPort);

        // Send B（儲存參考以便隱藏/顯示）
        sendBLabel = new EnhancedTextLabel(Vec(77, 292), Vec(30, 15), "SEND B", 6.f, nvgRGB(255, 255, 255), true);
        addChild(sendBLabel);
        sendBLPort = createOutputCentered<PJ301MPort>(Vec(75, 315), module, YAMANOTE::SEND_B_L_OUTPUT);
        addOutput(sendBLPort);
        sendBRPort = createOutputCentered<PJ301MPort>(Vec(105, 315), module, YAMANOTE::SEND_B_R_OUTPUT);
        addOutput(sendBRPort);

        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 343), module, YAMANOTE::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 368), module, YAMANOTE::MIX_R_OUTPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, YAMANOTE::CHAIN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, YAMANOTE::CHAIN_R_INPUT));

        // Return A（儲存參考以便隱藏/顯示）
        returnALPort = createInputCentered<PJ301MPort>(Vec(45, 343), module, YAMANOTE::RETURN_A_L_INPUT);
        addInput(returnALPort);
        returnARPort = createInputCentered<PJ301MPort>(Vec(45, 368), module, YAMANOTE::RETURN_A_R_INPUT);
        addInput(returnARPort);

        // Return B（儲存參考以便隱藏/顯示）
        returnBLPort = createInputCentered<PJ301MPort>(Vec(75, 343), module, YAMANOTE::RETURN_B_L_INPUT);
        addInput(returnBLPort);
        returnBRPort = createInputCentered<PJ301MPort>(Vec(75, 368), module, YAMANOTE::RETURN_B_R_INPUT);
        addInput(returnBRPort);
    }

    void step() override {
        YAMANOTE* module = dynamic_cast<YAMANOTE*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // 自動 cable 創建/刪除
            Module* rightModule = module->rightExpander.module;
            bool rightIsYamanote = rightModule && rightModule->model == modelYAMANOTE;
            bool rightIsU8 = rightModule && rightModule->model == modelU8;

            // Send/Return port 隱藏/顯示邏輯
            // 當右側有 YAMANOTE 時，隱藏自己的 Send/Return（因為信號會從右側累積輸出）
            if (rightIsYamanote != lastRightIsYamanote) {
                lastRightIsYamanote = rightIsYamanote;
                if (rightIsYamanote) {
                    // 隱藏 Send/Return ports 和標籤
                    if (sendALPort) sendALPort->hide();
                    if (sendARPort) sendARPort->hide();
                    if (sendBLPort) sendBLPort->hide();
                    if (sendBRPort) sendBRPort->hide();
                    if (returnALPort) returnALPort->hide();
                    if (returnARPort) returnARPort->hide();
                    if (returnBLPort) returnBLPort->hide();
                    if (returnBRPort) returnBRPort->hide();
                    if (sendALabel) sendALabel->hide();
                    if (sendBLabel) sendBLabel->hide();
                } else {
                    // 顯示 Send/Return ports 和標籤
                    if (sendALPort) sendALPort->show();
                    if (sendARPort) sendARPort->show();
                    if (sendBLPort) sendBLPort->show();
                    if (sendBRPort) sendBRPort->show();
                    if (returnALPort) returnALPort->show();
                    if (returnARPort) returnARPort->show();
                    if (returnBLPort) returnBLPort->show();
                    if (returnBRPort) returnBRPort->show();
                    if (sendALabel) sendALabel->show();
                    if (sendBLabel) sendBLabel->show();
                }
            }

            if (rightModule != lastRightExpander) {
                // Expander 改變了，但不刪除舊的 cable（保留 chain 連接）
                lastRightExpander = rightModule;

                // 如果右側是 YAMANOTE，創建新的自動 cable（只創建不刪除）
                if (rightIsYamanote) {
                    bool leftInputConnected = rightModule->inputs[YAMANOTE::CHAIN_L_INPUT].isConnected();
                    bool rightInputConnected = rightModule->inputs[YAMANOTE::CHAIN_R_INPUT].isConnected();

                    if (!leftInputConnected) {
                        Cable* cableL = new Cable;
                        cableL->outputModule = module;
                        cableL->outputId = YAMANOTE::MIX_L_OUTPUT;
                        cableL->inputModule = rightModule;
                        cableL->inputId = YAMANOTE::CHAIN_L_INPUT;
                        APP->engine->addCable(cableL);
                        autoChainLeftCableId = cableL->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableL);
                        cw->color = color::fromHexString("#80C342"); // YAMANOTE 綠色
                        APP->scene->rack->addCable(cw);
                    }

                    if (!rightInputConnected) {
                        Cable* cableR = new Cable;
                        cableR->outputModule = module;
                        cableR->outputId = YAMANOTE::MIX_R_OUTPUT;
                        cableR->inputModule = rightModule;
                        cableR->inputId = YAMANOTE::CHAIN_R_INPUT;
                        APP->engine->addCable(cableR);
                        autoChainRightCableId = cableR->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableR);
                        cw->color = color::fromHexString("#80C342"); // YAMANOTE 綠色
                        APP->scene->rack->addCable(cw);
                    }
                }
                // 如果右側是 U8，創建新的自動 cable
                else if (rightIsU8) {
                    // U8 的 chain input IDs
                    const int U8_CHAIN_LEFT = 5;  // CHAIN_LEFT_INPUT
                    const int U8_CHAIN_RIGHT = 6; // CHAIN_RIGHT_INPUT

                    bool leftInputConnected = rightModule->inputs[U8_CHAIN_LEFT].isConnected();
                    bool rightInputConnected = rightModule->inputs[U8_CHAIN_RIGHT].isConnected();

                    if (!leftInputConnected) {
                        Cable* cableL = new Cable;
                        cableL->outputModule = module;
                        cableL->outputId = YAMANOTE::MIX_L_OUTPUT;
                        cableL->inputModule = rightModule;
                        cableL->inputId = U8_CHAIN_LEFT;
                        APP->engine->addCable(cableL);
                        autoChainLeftCableId = cableL->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableL);
                        cw->color = color::fromHexString("#80C342"); // YAMANOTE 綠色
                        APP->scene->rack->addCable(cw);
                    }

                    if (!rightInputConnected) {
                        Cable* cableR = new Cable;
                        cableR->outputModule = module;
                        cableR->outputId = YAMANOTE::MIX_R_OUTPUT;
                        cableR->inputModule = rightModule;
                        cableR->inputId = U8_CHAIN_RIGHT;
                        APP->engine->addCable(cableR);
                        autoChainRightCableId = cableR->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableR);
                        cw->color = color::fromHexString("#80C342"); // YAMANOTE 綠色
                        APP->scene->rack->addCable(cw);
                    }
                }
            }

            // 驗證自動 cable 是否仍然有效（可能被用戶刪除）
            if (autoChainLeftCableId >= 0 && !APP->engine->getCable(autoChainLeftCableId)) {
                autoChainLeftCableId = -1;
            }
            if (autoChainRightCableId >= 0 && !APP->engine->getCable(autoChainRightCableId)) {
                autoChainRightCableId = -1;
            }

            // === 自動 CH Input Cable 功能 ===
            // 透過 Chain Cable 追蹤 U8，支援多個 YAMANOTE 串接
            // 邏輯：先追蹤到 chain 最前面，收集所有 U8，再根據 offset 分配

            // U8 的 port IDs
            const int U8_LEFT_INPUT = 0;
            const int U8_RIGHT_INPUT = 1;
            const int U8_CHAIN_LEFT = 5;

            // 收集整個 chain 的所有 U8（從右到左，會穿過其他 YAMANOTE）
            static constexpr int MAX_TOTAL_U8 = 64;  // 整個 chain 最多 64 個 U8
            Module* allU8s[MAX_TOTAL_U8] = {nullptr};
            int totalU8Count = 0;
            int yamanotesBefore = 0;  // 在自己之前遇到的 YAMANOTE 數量

            // 找到 CHAIN_L_INPUT 的 cable 來源，開始追蹤
            app::ModuleWidget* thisWidget = APP->scene->rack->getModule(module->id);
            if (thisWidget) {
                app::PortWidget* chainLPort = nullptr;
                for (app::PortWidget* pw : thisWidget->getInputs()) {
                    if (pw->portId == YAMANOTE::CHAIN_L_INPUT) {
                        chainLPort = pw;
                        break;
                    }
                }

                if (chainLPort) {
                    std::vector<app::CableWidget*> chainCables = APP->scene->rack->getCompleteCablesOnPort(chainLPort);
                    if (!chainCables.empty() && chainCables[0]->cable) {
                        Module* currentMod = chainCables[0]->cable->outputModule;

                        // 追蹤整個 chain，穿過其他 YAMANOTE
                        while (currentMod && totalU8Count < MAX_TOTAL_U8) {
                            if (currentMod->model == modelU8) {
                                // 是 U8，記錄並繼續追蹤
                                allU8s[totalU8Count++] = currentMod;

                                // 找到這個 U8 的 CHAIN_LEFT 輸入，繼續追蹤
                                app::ModuleWidget* u8Widget = APP->scene->rack->getModule(currentMod->id);
                                if (!u8Widget) break;

                                app::PortWidget* u8ChainPort = nullptr;
                                for (app::PortWidget* pw : u8Widget->getInputs()) {
                                    if (pw->portId == U8_CHAIN_LEFT) {
                                        u8ChainPort = pw;
                                        break;
                                    }
                                }

                                if (!u8ChainPort) break;

                                std::vector<app::CableWidget*> u8ChainCables = APP->scene->rack->getCompleteCablesOnPort(u8ChainPort);
                                if (u8ChainCables.empty() || !u8ChainCables[0]->cable) break;

                                currentMod = u8ChainCables[0]->cable->outputModule;
                            }
                            else if (currentMod->model == modelYAMANOTE) {
                                // 遇到另一個 YAMANOTE，記錄並穿過它繼續追蹤
                                yamanotesBefore++;

                                // 找到這個 YAMANOTE 的 CHAIN_L_INPUT，繼續追蹤
                                app::ModuleWidget* yamWidget = APP->scene->rack->getModule(currentMod->id);
                                if (!yamWidget) break;

                                app::PortWidget* yamChainPort = nullptr;
                                for (app::PortWidget* pw : yamWidget->getInputs()) {
                                    if (pw->portId == YAMANOTE::CHAIN_L_INPUT) {
                                        yamChainPort = pw;
                                        break;
                                    }
                                }

                                if (!yamChainPort) break;

                                std::vector<app::CableWidget*> yamChainCables = APP->scene->rack->getCompleteCablesOnPort(yamChainPort);
                                if (yamChainCables.empty() || !yamChainCables[0]->cable) break;

                                currentMod = yamChainCables[0]->cable->outputModule;
                            }
                            else {
                                break;  // 遇到非 U8 非 YAMANOTE，停止追蹤
                            }
                        }
                    }
                }
            }

            // 計算此 YAMANOTE 負責的 U8 範圍
            // allU8s[0] 是最近的（最右邊），allU8s[totalU8Count-1] 是最遠的（最左邊）
            // 每個 YAMANOTE 負責 8 個 U8
            int myOffset = yamanotesBefore * 8;  // 前面的 YAMANOTE 已處理的數量

            // 更新自己處理的 U8 總數（供右側 YAMANOTE 讀取）
            int myU8Count = std::min(8, std::max(0, totalU8Count - myOffset));
            module->totalU8HandledByThis = myOffset + myU8Count;

            // 對每個 CH 處理自動 input cable
            for (int ch = 0; ch < 8; ch++) {
                // 計算對應的 allU8s index（CH1 對應最左邊的 U8）
                int u8Index = totalU8Count - 1 - myOffset - ch;
                Module* u8Module = (u8Index >= 0 && u8Index < totalU8Count) ? allU8s[u8Index] : nullptr;

                // 如果這個 CH 沒有對應的 U8，清理舊的 cable
                if (!u8Module) {
                    if (lastLeftU8s[ch] != nullptr) {
                        for (int lr = 0; lr < 2; lr++) {
                            if (autoInputCableIds[ch][lr] >= 0) {
                                app::CableWidget* cw = APP->scene->rack->getCable(autoInputCableIds[ch][lr]);
                                if (cw) {
                                    APP->scene->rack->removeCable(cw);
                                    delete cw;
                                }
                                autoInputCableIds[ch][lr] = -1;
                            }
                        }
                        lastLeftU8s[ch] = nullptr;
                        lastU8InputSourceIds[ch][0] = -1;
                        lastU8InputSourceIds[ch][1] = -1;
                    }
                    continue;
                }

                // 獲取 U8 的 ModuleWidget
                app::ModuleWidget* u8Widget = APP->scene->rack->getModule(u8Module->id);
                if (!u8Widget) continue;

                // 處理 L 和 R 兩個通道
                for (int lr = 0; lr < 2; lr++) {
                    int u8InputId = (lr == 0) ? U8_LEFT_INPUT : U8_RIGHT_INPUT;
                    int yamanoteChId = (lr == 0) ? (YAMANOTE::CH1_L_INPUT + ch * 2) : (YAMANOTE::CH1_R_INPUT + ch * 2);

                    // 找到 U8 輸入 port 的 PortWidget
                    app::PortWidget* u8Port = nullptr;
                    for (app::PortWidget* pw : u8Widget->getInputs()) {
                        if (pw->portId == u8InputId) {
                            u8Port = pw;
                            break;
                        }
                    }
                    if (!u8Port) continue;

                    // 獲取連接到 U8 輸入的 cables
                    std::vector<app::CableWidget*> cables = APP->scene->rack->getCompleteCablesOnPort(u8Port);

                    int64_t currentSourceId = -1;
                    Module* sourceModule = nullptr;
                    int sourceOutputId = -1;
                    NVGcolor sourceColor = color::fromHexString("#FFCC00");

                    if (!cables.empty()) {
                        app::CableWidget* sourceCable = cables[0];
                        if (sourceCable->cable) {
                            sourceModule = sourceCable->cable->outputModule;
                            sourceOutputId = sourceCable->cable->outputId;
                            currentSourceId = sourceCable->cable->id;
                            sourceColor = sourceCable->color;
                        }
                    }

                    // 檢查源頭是否改變
                    if (currentSourceId != lastU8InputSourceIds[ch][lr]) {
                        // 刪除舊的自動 cable
                        if (autoInputCableIds[ch][lr] >= 0) {
                            app::CableWidget* oldCw = APP->scene->rack->getCable(autoInputCableIds[ch][lr]);
                            if (oldCw) {
                                APP->scene->rack->removeCable(oldCw);
                                delete oldCw;
                            }
                            autoInputCableIds[ch][lr] = -1;
                        }

                        lastU8InputSourceIds[ch][lr] = currentSourceId;

                        // 如果有新的源頭且 CH 未被佔用，創建新的自動 cable
                        if (sourceModule && !module->inputs[yamanoteChId].isConnected()) {
                            Cable* newCable = new Cable;
                            newCable->outputModule = sourceModule;
                            newCable->outputId = sourceOutputId;
                            newCable->inputModule = module;
                            newCable->inputId = yamanoteChId;
                            APP->engine->addCable(newCable);
                            autoInputCableIds[ch][lr] = newCable->id;

                            app::CableWidget* cw = new app::CableWidget;
                            cw->setCable(newCable);
                            cw->color = sourceColor;
                            APP->scene->rack->addCable(cw);
                        }
                    }

                    // 驗證自動 cable 是否仍然有效
                    if (autoInputCableIds[ch][lr] >= 0 && !APP->engine->getCable(autoInputCableIds[ch][lr])) {
                        autoInputCableIds[ch][lr] = -1;
                    }
                }

                lastLeftU8s[ch] = u8Module;
            }
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