#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "RipleyDSP.hpp"

using namespace rack;

// 白色背景區域
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
    }
};

struct BlackBackgroundBox : Widget {
    BlackBackgroundBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(10, 30, 20));
        nvgFill(args.vg);

        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 255));
        nvgStroke(args.vg);
    }
};

// 三行標題標籤
struct RunnerTitleLabel : TransparentWidget {
    std::string line1;
    std::string line2;
    std::string line3;

    RunnerTitleLabel(Vec pos, Vec size, std::string l1, std::string l2, std::string l3) {
        box.pos = pos;
        box.size = size;
        line1 = l1;
        line2 = l2;
        line3 = l3;
    }

    void draw(const DrawArgs &args) override {
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        // Line 1: 模組名稱 (12pt 白色) - Y=11
        nvgFontSize(args.vg, 12.f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgText(args.vg, box.size.x / 2.f, 11.f, line1.c_str(), NULL);

        // Line 2: 品牌 (10pt 黃色) - Y=26
        nvgFontSize(args.vg, 10.f);
        nvgFillColor(args.vg, nvgRGB(255, 200, 0));
        nvgText(args.vg, box.size.x / 2.f, 26.f, line3.c_str(), NULL);

        // Line 3: 效果類型 (7pt 外星螢光綠) - Y=33（MADZINE下方）
        nvgFontSize(args.vg, 7.f);
        nvgFillColor(args.vg, nvgRGB(57, 255, 20));
        nvgText(args.vg, box.size.x / 2.f, 33.f, line2.c_str(), NULL);
    }
};

// 參數標籤
struct RunnerParamLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;

    RunnerParamLabel(Vec pos, Vec size, std::string text, float fontSize = 7.f,
                     NVGcolor color = nvgRGB(255, 255, 255)) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

struct Runner : Module {
    int panelTheme = -1;
    float panelContrast = panelContrastDefault;

    enum ParamIds {
        TIME_L_PARAM,
        TIME_R_PARAM,
        FEEDBACK_PARAM,
        MIX_PARAM,
        CHAOS_PARAM,
        RATE_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        LEFT_INPUT,
        RIGHT_INPUT,
        TIME_L_CV_INPUT,
        TIME_R_CV_INPUT,
        FEEDBACK_CV_INPUT,
        MIX_CV_INPUT,
        CHAOS_CV_INPUT,
        RATE_CV_INPUT,
        NUM_INPUTS
    };

    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        CHAOS_OUTPUT,
        SH_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int DELAY_BUFFER_SIZE = 96000;
    static constexpr int MAX_POLY = 16;

    float leftDelayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    float rightDelayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    int delayWriteIndex[MAX_POLY];

    ChaosGenerator chaosGen[MAX_POLY];

    // S&H 狀態
    float lastSHValue[MAX_POLY] = {};
    float shPhase[MAX_POLY] = {};

    // CV 調變顯示
    float timeLCvMod = 0.0f;
    float timeRCvMod = 0.0f;
    float feedbackCvMod = 0.0f;
    float mixCvMod = 0.0f;
    float chaosCvMod = 0.0f;
    float rateCvMod = 0.0f;

    Runner() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(TIME_L_PARAM, 0.001f, 2.0f, 0.25f, "Time L", " s");
        configParam(TIME_R_PARAM, 0.001f, 2.0f, 0.25f, "Time R", " s");
        configParam(FEEDBACK_PARAM, 0.0f, 0.95f, 0.3f, "Feedback", "%", 0.f, 100.f);
        configParam(MIX_PARAM, 0.0f, 1.0f, 0.5f, "Mix", "%", 0.f, 100.f);
        configParam(CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Chaos", "%", 0.f, 100.f);
        configParam(RATE_PARAM, 0.01f, 2.0f, 0.5f, "Rate", "x");

        configInput(LEFT_INPUT, "Left Audio");
        configInput(RIGHT_INPUT, "Right Audio");
        configInput(TIME_L_CV_INPUT, "Time L CV");
        configInput(TIME_R_CV_INPUT, "Time R CV");
        configInput(FEEDBACK_CV_INPUT, "Feedback CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configInput(CHAOS_CV_INPUT, "Chaos CV");
        configInput(RATE_CV_INPUT, "Rate CV");

        configOutput(LEFT_OUTPUT, "Left Audio");
        configOutput(RIGHT_OUTPUT, "Right Audio");
        configOutput(CHAOS_OUTPUT, "Chaos CV");
        configOutput(SH_OUTPUT, "Sample & Hold CV");

        // Initialize buffers
        for (int c = 0; c < MAX_POLY; c++) {
            for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
                leftDelayBuffer[c][i] = 0.0f;
                rightDelayBuffer[c][i] = 0.0f;
            }
            delayWriteIndex[c] = 0;
        }
    }

    void onReset() override {
        for (int c = 0; c < MAX_POLY; c++) {
            chaosGen[c].reset();
            for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
                leftDelayBuffer[c][i] = 0.0f;
                rightDelayBuffer[c][i] = 0.0f;
            }
            delayWriteIndex[c] = 0;
        }
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
        if (args.sampleRate <= 0) return;

        int leftChannels = inputs[LEFT_INPUT].getChannels();
        int rightChannels = inputs[RIGHT_INPUT].getChannels();
        int channels = std::max({1, leftChannels, rightChannels});

        outputs[LEFT_OUTPUT].setChannels(channels);
        outputs[RIGHT_OUTPUT].setChannels(channels);
        outputs[CHAOS_OUTPUT].setChannels(channels);
        outputs[SH_OUTPUT].setChannels(channels);

        for (int c = 0; c < channels; c++) {
            // Chaos 參數 + CV
            float chaosAmount = params[CHAOS_PARAM].getValue();
            if (inputs[CHAOS_CV_INPUT].isConnected()) {
                float cv = inputs[CHAOS_CV_INPUT].getPolyVoltage(c < inputs[CHAOS_CV_INPUT].getChannels() ? c : 0);
                chaosAmount += cv * 0.1f;
                if (c == 0) chaosCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                chaosCvMod = 0.0f;
            }
            chaosAmount = clamp(chaosAmount, 0.0f, 1.0f);

            // Rate 參數 + CV
            float chaosRate = params[RATE_PARAM].getValue();
            if (inputs[RATE_CV_INPUT].isConnected()) {
                float cv = inputs[RATE_CV_INPUT].getPolyVoltage(c < inputs[RATE_CV_INPUT].getChannels() ? c : 0);
                chaosRate += cv * 0.2f;
                if (c == 0) rateCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                rateCvMod = 0.0f;
            }
            chaosRate = clamp(chaosRate, 0.01f, 2.0f);

            bool chaosEnabled = chaosAmount > 0.0f;

            // Chaos 處理
            float chaosRaw = 0.0f;
            float chaosSH = 0.0f;
            if (chaosEnabled) {
                chaosRaw = chaosGen[c].process(chaosRate) * chaosAmount;

                // S&H 邏輯：根據 rate 取樣
                float shRate = chaosRate * 10.0f;
                shPhase[c] += shRate / args.sampleRate;
                if (shPhase[c] >= 1.0f) {
                    lastSHValue[c] = chaosRaw;
                    shPhase[c] = 0.0f;
                }
                chaosSH = lastSHValue[c];
            }
            outputs[CHAOS_OUTPUT].setVoltage(chaosRaw * 5.0f, c);
            outputs[SH_OUTPUT].setVoltage(chaosSH * 5.0f, c);

            // 取得輸入
            float leftInput = (c < leftChannels) ? inputs[LEFT_INPUT].getPolyVoltage(c) : 0.0f;
            float rightInput = 0.0f;
            if (inputs[RIGHT_INPUT].isConnected()) {
                rightInput = (c < rightChannels) ? inputs[RIGHT_INPUT].getPolyVoltage(c) : 0.0f;
            } else {
                rightInput = leftInput;
            }

            if (!std::isfinite(leftInput)) leftInput = 0.0f;
            if (!std::isfinite(rightInput)) rightInput = 0.0f;

            // Time L 參數 + CV
            float timeL = params[TIME_L_PARAM].getValue();
            if (inputs[TIME_L_CV_INPUT].isConnected()) {
                float cv = inputs[TIME_L_CV_INPUT].getPolyVoltage(c < inputs[TIME_L_CV_INPUT].getChannels() ? c : 0);
                timeL += cv * 0.2f;
                if (c == 0) timeLCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                timeLCvMod = 0.0f;
            }
            if (chaosEnabled) {
                timeL += chaosRaw * 0.1f;
            }
            timeL = clamp(timeL, 0.001f, 2.0f);

            // Time R 參數 + CV
            float timeR = params[TIME_R_PARAM].getValue();
            if (inputs[TIME_R_CV_INPUT].isConnected()) {
                float cv = inputs[TIME_R_CV_INPUT].getPolyVoltage(c < inputs[TIME_R_CV_INPUT].getChannels() ? c : 0);
                timeR += cv * 0.2f;
                if (c == 0) timeRCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                timeRCvMod = 0.0f;
            }
            if (chaosEnabled) {
                timeR += chaosRaw * 0.1f;
            }
            timeR = clamp(timeR, 0.001f, 2.0f);

            // Feedback 參數 + CV
            float feedback = params[FEEDBACK_PARAM].getValue();
            if (inputs[FEEDBACK_CV_INPUT].isConnected()) {
                float cv = inputs[FEEDBACK_CV_INPUT].getPolyVoltage(c < inputs[FEEDBACK_CV_INPUT].getChannels() ? c : 0);
                feedback += cv * 0.1f;
                if (c == 0) feedbackCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                feedbackCvMod = 0.0f;
            }
            if (chaosEnabled) {
                feedback += chaosRaw * 0.1f;
            }
            feedback = clamp(feedback, 0.0f, 0.95f);

            // 計算延遲樣本數
            int delaySamplesL = (int)(timeL * args.sampleRate);
            delaySamplesL = clamp(delaySamplesL, 1, DELAY_BUFFER_SIZE - 1);
            int delaySamplesR = (int)(timeR * args.sampleRate);
            delaySamplesR = clamp(delaySamplesR, 1, DELAY_BUFFER_SIZE - 1);

            // 讀取延遲訊號
            int readIndexL = (delayWriteIndex[c] - delaySamplesL + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
            int readIndexR = (delayWriteIndex[c] - delaySamplesR + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;

            float leftDelayed = leftDelayBuffer[c][readIndexL];
            float rightDelayed = rightDelayBuffer[c][readIndexR];

            // 寫入延遲緩衝
            leftDelayBuffer[c][delayWriteIndex[c]] = leftInput + leftDelayed * feedback;
            rightDelayBuffer[c][delayWriteIndex[c]] = rightInput + rightDelayed * feedback;
            delayWriteIndex[c] = (delayWriteIndex[c] + 1) % DELAY_BUFFER_SIZE;

            // Wet/Dry Mix + CV
            float mix = params[MIX_PARAM].getValue();
            if (inputs[MIX_CV_INPUT].isConnected()) {
                float cv = inputs[MIX_CV_INPUT].getPolyVoltage(c < inputs[MIX_CV_INPUT].getChannels() ? c : 0);
                mix += cv * 0.1f;
                if (c == 0) mixCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else if (c == 0) {
                mixCvMod = 0.0f;
            }
            mix = clamp(mix, 0.0f, 1.0f);

            float leftOut = leftInput * (1.0f - mix) + leftDelayed * mix;
            float rightOut = rightInput * (1.0f - mix) + rightDelayed * mix;

            if (!std::isfinite(leftOut)) leftOut = 0.0f;
            if (!std::isfinite(rightOut)) rightOut = 0.0f;

            outputs[LEFT_OUTPUT].setVoltage(leftOut, c);
            outputs[RIGHT_OUTPUT].setVoltage(rightOut, c);
        }
    }

    void processBypass(const ProcessArgs& args) override {
        int leftChannels = inputs[LEFT_INPUT].getChannels();
        int rightChannels = inputs[RIGHT_INPUT].getChannels();
        int channels = std::max({1, leftChannels, rightChannels});

        outputs[LEFT_OUTPUT].setChannels(channels);
        outputs[RIGHT_OUTPUT].setChannels(channels);

        for (int c = 0; c < channels; c++) {
            float leftInput = (c < leftChannels) ? inputs[LEFT_INPUT].getPolyVoltage(c) : 0.0f;
            float rightInput = inputs[RIGHT_INPUT].isConnected()
                ? ((c < rightChannels) ? inputs[RIGHT_INPUT].getPolyVoltage(c) : 0.0f)
                : leftInput;

            outputs[LEFT_OUTPUT].setVoltage(leftInput, c);
            outputs[RIGHT_OUTPUT].setVoltage(rightInput, c);
        }
    }
};

struct RunnerWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    StandardBlackKnob26* timeKnob = nullptr;
    StandardBlackKnob26* feedbackKnob = nullptr;

    // 自動配線追蹤
    int64_t autoSendLeftCableId = -1;
    int64_t autoSendRightCableId = -1;
    int64_t autoReturnLeftCableId = -1;
    int64_t autoReturnRightCableId = -1;
    Module* lastLeftExpander = nullptr;
    bool usingChannelB = false;

    RunnerWidget(Runner* module) {
        setModule(module);
        panelThemeHelper.init(this, "4HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // 黑色標題背景
        addChild(new BlackBackgroundBox(Vec(0, 1), Vec(box.size.x, 18)));

        // 三行標題（壓縮高度，與標準模組對齊）
        addChild(new RunnerTitleLabel(Vec(0, 0), Vec(box.size.x, 30),
            "Runner", "the Delay effect", "MADZINE"));

        float centerX = box.size.x / 2.0f;  // 30
        float leftX = 15.0f;
        float rightX = 45.0f;

        // Row 1: TIME L, TIME R (Y=72, 標籤 Y=48, 框高 15px)
        addChild(new RunnerParamLabel(Vec(0, 48), Vec(30, 15), "TIME L"));
        timeKnob = createParamCentered<StandardBlackKnob26>(Vec(leftX, 72), module, Runner::TIME_L_PARAM);
        addParam(timeKnob);

        addChild(new RunnerParamLabel(Vec(30, 48), Vec(30, 15), "TIME R"));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(rightX, 72), module, Runner::TIME_R_PARAM));

        // Row 2: FEEDBACK, MIX (Y=117, 標籤 Y=93)
        addChild(new RunnerParamLabel(Vec(0, 93), Vec(30, 15), "FEEDBACK", 5.f));
        feedbackKnob = createParamCentered<StandardBlackKnob26>(Vec(leftX, 117), module, Runner::FEEDBACK_PARAM);
        addParam(feedbackKnob);

        addChild(new RunnerParamLabel(Vec(30, 93), Vec(30, 15), "MIX"));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(rightX, 117), module, Runner::MIX_PARAM));

        // Row 3: CHAOS, RATE (Y=162, 標籤 Y=138)
        addChild(new RunnerParamLabel(Vec(0, 138), Vec(30, 15), "CHAOS"));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(leftX, 162), module, Runner::CHAOS_PARAM));

        addChild(new RunnerParamLabel(Vec(30, 138), Vec(30, 15), "RATE"));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(rightX, 162), module, Runner::RATE_PARAM));

        // CV Row 1 (Y=197, 標籤 Y=173)
        addChild(new RunnerParamLabel(Vec(0, 173), Vec(30, 15), "TIME L"));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX, 197), module, Runner::TIME_L_CV_INPUT));

        addChild(new RunnerParamLabel(Vec(30, 173), Vec(30, 15), "TIME R"));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX, 197), module, Runner::TIME_R_CV_INPUT));

        // CV Row 2 (Y=232, 標籤 Y=208)
        addChild(new RunnerParamLabel(Vec(0, 208), Vec(30, 15), "FEEDBACK", 5.f));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX, 232), module, Runner::FEEDBACK_CV_INPUT));

        addChild(new RunnerParamLabel(Vec(30, 208), Vec(30, 15), "MIX"));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX, 232), module, Runner::MIX_CV_INPUT));

        // CV Row 3 (Y=267, 標籤 Y=243)
        addChild(new RunnerParamLabel(Vec(0, 243), Vec(30, 15), "CHAOS"));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX, 267), module, Runner::CHAOS_CV_INPUT));

        addChild(new RunnerParamLabel(Vec(30, 243), Vec(30, 15), "RATE"));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX, 267), module, Runner::RATE_CV_INPUT));

        // Chaos / S&H Outputs (Y=302, 標籤 Y=278)
        addChild(new RunnerParamLabel(Vec(0, 278), Vec(30, 15), "CHAOS"));
        addOutput(createOutputCentered<PJ301MPort>(Vec(leftX, 302), module, Runner::CHAOS_OUTPUT));

        addChild(new RunnerParamLabel(Vec(30, 278), Vec(30, 15), "S&H"));
        addOutput(createOutputCentered<PJ301MPort>(Vec(rightX, 302), module, Runner::SH_OUTPUT));

        // 白色背景區域 (Y >= 330)
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, box.size.y - 330)));

        // Audio I/O - 左邊 Input, 右邊 Output
        // L row (Y: 343)
        addInput(createInputCentered<PJ301MPort>(Vec(leftX, 343), module, Runner::LEFT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(rightX, 343), module, Runner::LEFT_OUTPUT));
        // R row (Y: 368)
        addInput(createInputCentered<PJ301MPort>(Vec(leftX, 368), module, Runner::RIGHT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(rightX, 368), module, Runner::RIGHT_OUTPUT));
    }

    void step() override {
        Runner* module = dynamic_cast<Runner*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            if (timeKnob) {
                bool connected = module->inputs[Runner::TIME_L_CV_INPUT].isConnected();
                timeKnob->setModulationEnabled(connected);
                if (connected) timeKnob->setModulation(module->timeLCvMod);
            }
            if (feedbackKnob) {
                bool connected = module->inputs[Runner::FEEDBACK_CV_INPUT].isConnected();
                feedbackKnob->setModulationEnabled(connected);
                if (connected) feedbackKnob->setModulation(module->feedbackCvMod);
            }

            // 自動配線到 YAMANOTE Send/Return
            Module* leftModule = module->leftExpander.module;
            bool leftIsYamanote = leftModule && leftModule->model == modelYAMANOTE;

            // 檢查是否是鏈式配線：左邊是效果器，再左邊是 YAMANOTE
            bool leftIsRipleyEffect = leftModule && (
                leftModule->model == modelRunner ||
                leftModule->model == modelFacehugger ||
                leftModule->model == modelOvomorph);
            Module* yamanoteModule = nullptr;
            bool useChannelB = false;

            if (leftIsYamanote) {
                yamanoteModule = leftModule;
                useChannelB = false;  // 直接連 YAMANOTE，使用 A
            } else if (leftIsRipleyEffect) {
                // 檢查效果器的左邊是否是 YAMANOTE
                Module* leftLeftModule = leftModule->leftExpander.module;
                if (leftLeftModule && leftLeftModule->model == modelYAMANOTE) {
                    yamanoteModule = leftLeftModule;
                    useChannelB = true;  // 鏈式配線，使用 B
                }
            }

            if (leftModule != lastLeftExpander) {
                // Expander 改變了，清理舊的自動 cable
                if (autoSendLeftCableId >= 0) {
                    app::CableWidget* cw = APP->scene->rack->getCable(autoSendLeftCableId);
                    if (cw) {
                        APP->scene->rack->removeCable(cw);
                        delete cw;
                    }
                    autoSendLeftCableId = -1;
                }
                if (autoSendRightCableId >= 0) {
                    app::CableWidget* cw = APP->scene->rack->getCable(autoSendRightCableId);
                    if (cw) {
                        APP->scene->rack->removeCable(cw);
                        delete cw;
                    }
                    autoSendRightCableId = -1;
                }
                if (autoReturnLeftCableId >= 0) {
                    app::CableWidget* cw = APP->scene->rack->getCable(autoReturnLeftCableId);
                    if (cw) {
                        APP->scene->rack->removeCable(cw);
                        delete cw;
                    }
                    autoReturnLeftCableId = -1;
                }
                if (autoReturnRightCableId >= 0) {
                    app::CableWidget* cw = APP->scene->rack->getCable(autoReturnRightCableId);
                    if (cw) {
                        APP->scene->rack->removeCable(cw);
                        delete cw;
                    }
                    autoReturnRightCableId = -1;
                }

                lastLeftExpander = leftModule;
                usingChannelB = useChannelB;

                // 如果找到 YAMANOTE（直接或鏈式），創建自動 cable
                if (yamanoteModule) {
                    // YAMANOTE 的 Send/Return port IDs
                    const int YAMANOTE_SEND_A_L = 0;
                    const int YAMANOTE_SEND_A_R = 1;
                    const int YAMANOTE_SEND_B_L = 2;
                    const int YAMANOTE_SEND_B_R = 3;
                    const int YAMANOTE_RETURN_A_L = 18;
                    const int YAMANOTE_RETURN_A_R = 19;
                    const int YAMANOTE_RETURN_B_L = 20;
                    const int YAMANOTE_RETURN_B_R = 21;

                    int sendL, sendR, returnL, returnR;
                    if (useChannelB) {
                        // 鏈式配線，使用 Channel B
                        sendL = YAMANOTE_SEND_B_L;
                        sendR = YAMANOTE_SEND_B_R;
                        returnL = YAMANOTE_RETURN_B_L;
                        returnR = YAMANOTE_RETURN_B_R;
                    } else {
                        // 直接連 YAMANOTE，使用 Channel A
                        sendL = YAMANOTE_SEND_A_L;
                        sendR = YAMANOTE_SEND_A_R;
                        returnL = YAMANOTE_RETURN_A_L;
                        returnR = YAMANOTE_RETURN_A_R;
                    }

                    NVGcolor yamanoteColor = color::fromHexString("#80C342");

                    // YAMANOTE Send L → Runner IN L
                    if (!module->inputs[Runner::LEFT_INPUT].isConnected()) {
                        Cable* cable = new Cable;
                        cable->outputModule = yamanoteModule;
                        cable->outputId = sendL;
                        cable->inputModule = module;
                        cable->inputId = Runner::LEFT_INPUT;
                        APP->engine->addCable(cable);
                        autoSendLeftCableId = cable->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cable);
                        cw->color = yamanoteColor;
                        APP->scene->rack->addCable(cw);
                    }

                    // YAMANOTE Send R → Runner IN R
                    if (!module->inputs[Runner::RIGHT_INPUT].isConnected()) {
                        Cable* cable = new Cable;
                        cable->outputModule = yamanoteModule;
                        cable->outputId = sendR;
                        cable->inputModule = module;
                        cable->inputId = Runner::RIGHT_INPUT;
                        APP->engine->addCable(cable);
                        autoSendRightCableId = cable->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cable);
                        cw->color = yamanoteColor;
                        APP->scene->rack->addCable(cw);
                    }

                    // Runner OUT L → YAMANOTE Return L
                    if (!yamanoteModule->inputs[returnL].isConnected()) {
                        Cable* cable = new Cable;
                        cable->outputModule = module;
                        cable->outputId = Runner::LEFT_OUTPUT;
                        cable->inputModule = yamanoteModule;
                        cable->inputId = returnL;
                        APP->engine->addCable(cable);
                        autoReturnLeftCableId = cable->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cable);
                        cw->color = yamanoteColor;
                        APP->scene->rack->addCable(cw);
                    }

                    // Runner OUT R → YAMANOTE Return R
                    if (!yamanoteModule->inputs[returnR].isConnected()) {
                        Cable* cable = new Cable;
                        cable->outputModule = module;
                        cable->outputId = Runner::RIGHT_OUTPUT;
                        cable->inputModule = yamanoteModule;
                        cable->inputId = returnR;
                        APP->engine->addCable(cable);
                        autoReturnRightCableId = cable->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cable);
                        cw->color = yamanoteColor;
                        APP->scene->rack->addCable(cw);
                    }
                }
            }

            // 驗證自動 cable 是否仍然有效（可能被用戶刪除）
            if (autoSendLeftCableId >= 0 && !APP->engine->getCable(autoSendLeftCableId)) {
                autoSendLeftCableId = -1;
            }
            if (autoSendRightCableId >= 0 && !APP->engine->getCable(autoSendRightCableId)) {
                autoSendRightCableId = -1;
            }
            if (autoReturnLeftCableId >= 0 && !APP->engine->getCable(autoReturnLeftCableId)) {
                autoReturnLeftCableId = -1;
            }
            if (autoReturnRightCableId >= 0 && !APP->engine->getCable(autoReturnRightCableId)) {
                autoReturnRightCableId = -1;
            }
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Runner* module = dynamic_cast<Runner*>(this->module);
        if (!module) return;
        addPanelThemeMenu(menu, module);
    }
};

Model* modelRunner = createModel<Runner, RunnerWidget>("Runner");
