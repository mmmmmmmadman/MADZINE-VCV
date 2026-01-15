#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

// 深藍色標題背景（U8 官方色 #004F7C）
struct AlexTitleBox : Widget {
    AlexTitleBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(0, 79, 124)); // #004F7C
        nvgFill(args.vg);

        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGBA(100, 100, 100, 255));
        nvgStroke(args.vg);
    }
};

// 白色底部區域（與 U8 相同）
struct AlexWhiteBox : Widget {
    AlexWhiteBox(Vec pos, Vec size) {
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

// 文字標籤（與 U8 的 TechnoEnhancedTextLabel 相同）
struct AlexTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;

    AlexTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
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

static const int ALEX_TRACKS = 4;

// Exclusive Solo Button for ALEX：長按 = exclusive（取消其他所有 solo）
template <typename TLight>
struct AlexExclusiveSoloButton : VCVLightLatch<TLight> {
    float pressTime = 0.f;
    bool pressing = false;
    bool exclusiveTriggered = false; // 防止重複觸發
    int trackIndex = 0; // 此按鈕對應的軌道
    static constexpr float LONG_PRESS_TIME = 0.4f; // 400ms

    void onDragStart(const event::DragStart& e) override {
        pressTime = 0.f;
        pressing = true;
        exclusiveTriggered = false;
        VCVLightLatch<TLight>::onDragStart(e);
    }

    void onDragEnd(const event::DragEnd& e) override {
        pressing = false;
        VCVLightLatch<TLight>::onDragEnd(e);
    }

    void step() override {
        VCVLightLatch<TLight>::step();
        if (pressing) {
            pressTime += APP->window->getLastFrameDuration();
            // 達到閾值時立即觸發 exclusive solo
            if (pressTime >= LONG_PRESS_TIME && !exclusiveTriggered) {
                exclusiveTriggered = true;
                Module* module = this->module;
                if (module) {
                    // 取消同一模組中其他軌道的 solo
                    for (int t = 0; t < ALEX_TRACKS; t++) {
                        if (t != trackIndex) {
                            module->params[12 + t].setValue(0.f); // SOLO_PARAM + t
                        }
                    }
                    // 取消整個 chain 中其他模組的所有 solo
                    Module* mod = module->leftExpander.module;
                    while (mod) {
                        if (mod->model == modelU8) {
                            mod->params[3].setValue(0.f);
                        } else if (mod->model == modelALEXANDERPLATZ) {
                            for (int t = 0; t < ALEX_TRACKS; t++) {
                                mod->params[12 + t].setValue(0.f);
                            }
                        } else if (mod->model == modelSHINJUKU) {
                            for (int t = 0; t < 8; t++) {
                                mod->params[24 + t].setValue(0.f);
                            }
                        } else {
                            break;
                        }
                        mod = mod->leftExpander.module;
                    }
                    mod = module->rightExpander.module;
                    while (mod) {
                        if (mod->model == modelU8) {
                            mod->params[3].setValue(0.f);
                        } else if (mod->model == modelALEXANDERPLATZ) {
                            for (int t = 0; t < ALEX_TRACKS; t++) {
                                mod->params[12 + t].setValue(0.f);
                            }
                        } else if (mod->model == modelSHINJUKU) {
                            for (int t = 0; t < 8; t++) {
                                mod->params[24 + t].setValue(0.f);
                            }
                        } else {
                            break;
                        }
                        mod = mod->rightExpander.module;
                    }
                    // 確保自己是 solo 狀態
                    this->getParamQuantity()->setValue(1.f);
                }
            }
        }
    }
};

struct ALEXANDERPLATZ : Module {
    int panelTheme = -1;
    float panelContrast = panelContrastDefault;

    enum ParamId {
        ENUMS(LEVEL_PARAM, ALEX_TRACKS),
        ENUMS(DUCK_PARAM, ALEX_TRACKS),
        ENUMS(MUTE_PARAM, ALEX_TRACKS),
        ENUMS(SOLO_PARAM, ALEX_TRACKS),
        PARAMS_LEN
    };
    enum InputId {
        ENUMS(LEFT_INPUT, ALEX_TRACKS),
        ENUMS(RIGHT_INPUT, ALEX_TRACKS),
        ENUMS(LEVEL_CV_INPUT, ALEX_TRACKS),
        ENUMS(DUCK_INPUT, ALEX_TRACKS),
        ENUMS(MUTE_TRIG_INPUT, ALEX_TRACKS),
        ENUMS(SOLO_TRIG_INPUT, ALEX_TRACKS),
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
        ENUMS(MUTE_LIGHT, ALEX_TRACKS),
        ENUMS(SOLO_LIGHT, ALEX_TRACKS),
        LIGHTS_LEN
    };

    static constexpr int MAX_POLY = 16;

    bool muteState[ALEX_TRACKS] = {false};
    bool soloState[ALEX_TRACKS] = {false};
    dsp::SchmittTrigger muteTrigger[ALEX_TRACKS];
    dsp::SchmittTrigger soloTrigger[ALEX_TRACKS];
    float levelCvModulation[ALEX_TRACKS] = {0.0f};
    float vuLevelL[ALEX_TRACKS] = {-60.0f};
    float vuLevelR[ALEX_TRACKS] = {-60.0f};

    ALEXANDERPLATZ() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        for (int t = 0; t < ALEX_TRACKS; t++) {
            configParam(LEVEL_PARAM + t, 0.0f, 2.0f, 1.0f, string::f("Track %d Level", t + 1));
            configParam(DUCK_PARAM + t, 0.0f, 1.0f, 0.0f, string::f("Track %d Duck", t + 1));
            configSwitch(MUTE_PARAM + t, 0.0f, 1.0f, 0.0f, string::f("Track %d Mute", t + 1), {"Unmuted", "Muted"});
            configSwitch(SOLO_PARAM + t, 0.0f, 1.0f, 0.0f, string::f("Track %d Solo", t + 1), {"Off", "Solo"});
            getParamQuantity(SOLO_PARAM + t)->description = "Hold for exclusive";

            configInput(LEFT_INPUT + t, string::f("Track %d Left", t + 1));
            configInput(RIGHT_INPUT + t, string::f("Track %d Right", t + 1));
            configInput(LEVEL_CV_INPUT + t, string::f("Track %d Level CV", t + 1));
            configInput(DUCK_INPUT + t, string::f("Track %d Duck", t + 1));
            configInput(MUTE_TRIG_INPUT + t, string::f("Track %d Mute Trigger", t + 1));
            configInput(SOLO_TRIG_INPUT + t, string::f("Track %d Solo Trigger", t + 1));
        }

        configInput(CHAIN_LEFT_INPUT, "Chain Left");
        configInput(CHAIN_RIGHT_INPUT, "Chain Right");
        configOutput(LEFT_OUTPUT, "Mix Left");
        configOutput(RIGHT_OUTPUT, "Mix Right");
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_real_value(contrastJ);
    }

    void process(const ProcessArgs& args) override {
        int maxChannels = 1;
        for (int t = 0; t < ALEX_TRACKS; t++) {
            maxChannels = std::max(maxChannels, inputs[LEFT_INPUT + t].getChannels());
            maxChannels = std::max(maxChannels, inputs[RIGHT_INPUT + t].getChannels());
        }
        maxChannels = std::max(maxChannels, inputs[CHAIN_LEFT_INPUT].getChannels());
        maxChannels = std::max(maxChannels, inputs[CHAIN_RIGHT_INPUT].getChannels());

        outputs[LEFT_OUTPUT].setChannels(maxChannels);
        outputs[RIGHT_OUTPUT].setChannels(maxChannels);

        // 跨模組 Solo 邏輯：檢查整個 chain 是否有任何軌道被 solo
        bool chainHasSolo = false;
        for (int t = 0; t < ALEX_TRACKS && !chainHasSolo; t++) {
            if (params[SOLO_PARAM + t].getValue() > 0.5f) chainHasSolo = true;
        }

        // 往左追蹤（使用硬編碼索引：U8 SOLO=3, ALEX SOLO=12+t, SHINJUKU SOLO=24+t）
        Module* mod = leftExpander.module;
        while (mod && !chainHasSolo) {
            if (mod->model == modelU8) {
                if (mod->params[3].getValue() > 0.5f) chainHasSolo = true; // U8::SOLO_PARAM
            } else if (mod->model == modelALEXANDERPLATZ) {
                for (int t = 0; t < ALEX_TRACKS && !chainHasSolo; t++) {
                    if (mod->params[12 + t].getValue() > 0.5f) chainHasSolo = true; // SOLO_PARAM + t
                }
            } else if (mod->model == modelSHINJUKU) {
                for (int t = 0; t < 8 && !chainHasSolo; t++) {
                    if (mod->params[24 + t].getValue() > 0.5f) chainHasSolo = true; // SHINJUKU::SOLO_PARAM + t
                }
            } else {
                break;
            }
            mod = mod->leftExpander.module;
        }

        // 往右追蹤
        mod = rightExpander.module;
        while (mod && !chainHasSolo) {
            if (mod->model == modelU8) {
                if (mod->params[3].getValue() > 0.5f) chainHasSolo = true;
            } else if (mod->model == modelALEXANDERPLATZ) {
                for (int t = 0; t < ALEX_TRACKS && !chainHasSolo; t++) {
                    if (mod->params[12 + t].getValue() > 0.5f) chainHasSolo = true;
                }
            } else if (mod->model == modelSHINJUKU) {
                for (int t = 0; t < 8 && !chainHasSolo; t++) {
                    if (mod->params[24 + t].getValue() > 0.5f) chainHasSolo = true;
                }
            } else {
                break;
            }
            mod = mod->rightExpander.module;
        }

        for (int c = 0; c < maxChannels; c++) {
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (int t = 0; t < ALEX_TRACKS; t++) {
                // Mute trigger 處理
                if (c == 0 && inputs[MUTE_TRIG_INPUT + t].isConnected()) {
                    if (muteTrigger[t].process(inputs[MUTE_TRIG_INPUT + t].getVoltage())) {
                        muteState[t] = !muteState[t];
                        params[MUTE_PARAM + t].setValue(muteState[t] ? 1.0f : 0.0f);
                    }
                }

                // Solo trigger 處理
                if (c == 0 && inputs[SOLO_TRIG_INPUT + t].isConnected()) {
                    if (soloTrigger[t].process(inputs[SOLO_TRIG_INPUT + t].getVoltage())) {
                        soloState[t] = !soloState[t];
                        params[SOLO_PARAM + t].setValue(soloState[t] ? 1.0f : 0.0f);
                    }
                }

                bool muted = params[MUTE_PARAM + t].getValue() > 0.5f;
                bool soloed = params[SOLO_PARAM + t].getValue() > 0.5f;
                bool soloMuted = chainHasSolo && !soloed;

                if (c == 0) {
                    // mute 燈在被 solo 靜音時也要亮起
                    lights[MUTE_LIGHT + t].setBrightness((muted || soloMuted) ? 1.0f : 0.0f);
                    lights[SOLO_LIGHT + t].setBrightness(soloed ? 1.0f : 0.0f);
                }

                // 如果 chain 有 solo 但此軌道沒有 solo，則跳過
                if (soloMuted) continue;
                if (muted) continue;

                float leftIn = inputs[LEFT_INPUT + t].getPolyVoltage(c);
                float rightIn = inputs[RIGHT_INPUT + t].isConnected()
                    ? inputs[RIGHT_INPUT + t].getPolyVoltage(c)
                    : leftIn;

                float level = params[LEVEL_PARAM + t].getValue();
                if (inputs[LEVEL_CV_INPUT + t].isConnected()) {
                    float cv = clamp(inputs[LEVEL_CV_INPUT + t].getPolyVoltage(c) / 10.0f, -1.0f, 1.0f);
                    level = clamp(level + cv, 0.0f, 2.0f);
                    if (c == 0) levelCvModulation[t] = cv;
                } else {
                    if (c == 0) levelCvModulation[t] = 0.0f;
                }

                float duck = 1.0f;
                if (inputs[DUCK_INPUT + t].isConnected()) {
                    float duckCV = clamp(inputs[DUCK_INPUT + t].getPolyVoltage(c) / 10.0f, 0.0f, 1.0f);
                    float duckAmount = params[DUCK_PARAM + t].getValue();
                    duck = clamp(1.0f - (duckCV * duckAmount * 3.0f), 0.0f, 1.0f);
                }

                mixL += leftIn * level * duck;
                mixR += rightIn * level * duck;

                if (c == 0) {
                    float peakL = std::abs(leftIn);
                    float peakR = std::abs(rightIn);
                    float dbL = (peakL > 0.0001f) ? 20.0f * log10f(peakL / 5.0f) : -60.0f;
                    float dbR = (peakR > 0.0001f) ? 20.0f * log10f(peakR / 5.0f) : -60.0f;

                    float attackCoeff = 1.0f - expf(-1.0f / (0.005f * args.sampleRate));
                    float releaseCoeff = 1.0f - expf(-1.0f / (0.3f * args.sampleRate));

                    vuLevelL[t] += (dbL > vuLevelL[t]) ? (dbL - vuLevelL[t]) * attackCoeff : (dbL - vuLevelL[t]) * releaseCoeff;
                    vuLevelR[t] += (dbR > vuLevelR[t]) ? (dbR - vuLevelR[t]) * attackCoeff : (dbR - vuLevelR[t]) * releaseCoeff;
                }
            }

            mixL += inputs[CHAIN_LEFT_INPUT].getPolyVoltage(c);
            mixR += inputs[CHAIN_RIGHT_INPUT].getPolyVoltage(c);

            // 防爆音：限制輸出在 ±10V
            outputs[LEFT_OUTPUT].setVoltage(clamp(mixL, -10.f, 10.f), c);
            outputs[RIGHT_OUTPUT].setVoltage(clamp(mixR, -10.f, 10.f), c);
        }
    }
};

// VU Meter（與 U8 相同樣式）
struct AlexVUMeter : TransparentWidget {
    ALEXANDERPLATZ* module = nullptr;
    int track = 0;
    bool isLeft = true;

    static constexpr float MIN_DB = -36.0f;
    static constexpr float MAX_DB = 6.0f;

    void draw(const DrawArgs &args) override {
        float level = -60.0f;
        if (module) {
            level = isLeft ? module->vuLevelL[track] : module->vuLevelR[track];
        }

        float normalizedLevel = clamp((level - MIN_DB) / (MAX_DB - MIN_DB), 0.0f, 1.0f);
        float redThreshold = (0.0f - MIN_DB) / (MAX_DB - MIN_DB);

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGBA(40, 40, 40, 255));
        nvgFill(args.vg);

        if (normalizedLevel > 0.0f) {
            float barWidth = box.size.x * normalizedLevel;

            NVGpaint gradient = nvgLinearGradient(args.vg, 0, 0, box.size.x, 0,
                nvgRGB(80, 180, 80), nvgRGB(255, 50, 50));

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, barWidth, box.size.y);
            nvgFillPaint(args.vg, gradient);
            nvgFill(args.vg);

            if (normalizedLevel > redThreshold) {
                float redStart = box.size.x * redThreshold;
                nvgBeginPath(args.vg);
                nvgRect(args.vg, redStart, 0, barWidth - redStart, box.size.y);
                nvgFillColor(args.vg, nvgRGB(255, 50, 50));
                nvgFill(args.vg);
            }
        }
    }
};

struct ALEXANDERPLATZWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    TechnoStandardBlackKnob* levelKnobs[ALEX_TRACKS] = {nullptr};

    // Chain 自動接線（模組移開後保持連接）
    int64_t autoChainLeftCableId = -1;
    int64_t autoChainRightCableId = -1;

    ALEXANDERPLATZWidget(ALEXANDERPLATZ* module) {
        setModule(module);
        panelThemeHelper.init(this, "16HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(16 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // 標題區（深藍色 #004F7C）
        addChild(new AlexTitleBox(Vec(0, 1), Vec(box.size.x, 18)));
        addChild(new AlexTextLabel(Vec(0, 1), Vec(box.size.x, 20), "ALEXANDERPLATZ", 14.f, nvgRGB(255, 255, 255)));
        addChild(new AlexTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0)));

        // 每軌寬度（4HP = 60.96px）
        float trackWidth = 4 * RACK_GRID_WIDTH;

        for (int t = 0; t < ALEX_TRACKS; t++) {
            float trackX = t * trackWidth;
            float centerX = trackX + trackWidth / 2;

            // INPUT 標籤（按規範：標籤框 Y = 元件 Y - 24 = 59 - 24 = 35）
            addChild(new AlexTextLabel(Vec(trackX, 35), Vec(trackWidth, 15), "INPUT", 8.f, nvgRGB(255, 255, 255)));

            // L/R 輸入
            addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 59), module, ALEXANDERPLATZ::LEFT_INPUT + t));
            addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 59), module, ALEXANDERPLATZ::RIGHT_INPUT + t));

            // VU Meters
            AlexVUMeter* vuL = new AlexVUMeter();
            vuL->box.pos = Vec(trackX + 4, 71);
            vuL->box.size = Vec(trackWidth - 8, 5);
            vuL->module = module;
            vuL->track = t;
            vuL->isLeft = true;
            addChild(vuL);

            AlexVUMeter* vuR = new AlexVUMeter();
            vuR->box.pos = Vec(trackX + 4, 79);
            vuR->box.size = Vec(trackWidth - 8, 5);
            vuR->module = module;
            vuR->track = t;
            vuR->isLeft = false;
            addChild(vuR);

            // LEVEL
            addChild(new AlexTextLabel(Vec(trackX - 5, 89), Vec(trackWidth + 10, 10), "LEVEL", 10.5f, nvgRGB(255, 255, 255)));
            levelKnobs[t] = createParamCentered<TechnoStandardBlackKnob>(Vec(centerX, 123), module, ALEXANDERPLATZ::LEVEL_PARAM + t);
            addParam(levelKnobs[t]);
            addInput(createInputCentered<PJ301MPort>(Vec(centerX, 161), module, ALEXANDERPLATZ::LEVEL_CV_INPUT + t));

            // DUCK
            addChild(new AlexTextLabel(Vec(trackX - 5, 182), Vec(trackWidth + 10, 10), "DUCK", 10.5f, nvgRGB(255, 255, 255)));
            addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(centerX, 216), module, ALEXANDERPLATZ::DUCK_PARAM + t));
            addInput(createInputCentered<PJ301MPort>(Vec(centerX, 254), module, ALEXANDERPLATZ::DUCK_INPUT + t));

            // MUTE / SOLO
            addChild(new AlexTextLabel(Vec(trackX - 5, 270), Vec(trackWidth + 10, 10), "MUTE SOLO", 10.5f, nvgRGB(255, 255, 255)));
            addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(Vec(centerX - 15, 292), module, ALEXANDERPLATZ::MUTE_PARAM + t, ALEXANDERPLATZ::MUTE_LIGHT + t));
            {
                auto* soloBtn = createLightParamCentered<AlexExclusiveSoloButton<MediumSimpleLight<GreenLight>>>(Vec(centerX + 15, 292), module, ALEXANDERPLATZ::SOLO_PARAM + t, ALEXANDERPLATZ::SOLO_LIGHT + t);
                soloBtn->trackIndex = t;
                addParam(soloBtn);
            }
            addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 316), module, ALEXANDERPLATZ::MUTE_TRIG_INPUT + t));
            addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 316), module, ALEXANDERPLATZ::SOLO_TRIG_INPUT + t));
        }

        // 底部白色區域
        addChild(new AlexWhiteBox(Vec(0, 330), Vec(box.size.x, 60)));

        // Chain 輸入（左側）
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, ALEXANDERPLATZ::CHAIN_LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, ALEXANDERPLATZ::CHAIN_RIGHT_INPUT));

        // Mix 輸出（右側）
        addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, 343), module, ALEXANDERPLATZ::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x - 15, 368), module, ALEXANDERPLATZ::RIGHT_OUTPUT));
    }

    void step() override {
        ALEXANDERPLATZ* module = dynamic_cast<ALEXANDERPLATZ*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // Level 旋鈕 CV 調變顯示
            for (int t = 0; t < ALEX_TRACKS; t++) {
                if (levelKnobs[t]) {
                    bool cvConnected = module->inputs[ALEXANDERPLATZ::LEVEL_CV_INPUT + t].isConnected();
                    levelKnobs[t]->setModulationEnabled(cvConnected);
                    if (cvConnected) {
                        levelKnobs[t]->setModulation(module->levelCvModulation[t]);
                    }
                }
            }

            // Chain 自動接線（模組移開後保持連接，用戶可手動刪除）
            // 先驗證現有的自動 cable（被手動刪除時清除 ID）
            if (autoChainLeftCableId >= 0 && !APP->engine->getCable(autoChainLeftCableId)) {
                autoChainLeftCableId = -1;
            }
            if (autoChainRightCableId >= 0 && !APP->engine->getCable(autoChainRightCableId)) {
                autoChainRightCableId = -1;
            }

            // 只有在相鄰且目前沒有自動 cable 時才建立新的
            Module* rightModule = module->rightExpander.module;
            if (rightModule && autoChainLeftCableId < 0 && autoChainRightCableId < 0) {
                bool rightIsU8 = rightModule->model == modelU8;
                bool rightIsYamanote = rightModule->model == modelYAMANOTE;
                bool rightIsAlex = rightModule->model == modelALEXANDERPLATZ;
                bool rightIsShinjuku = rightModule->model == modelSHINJUKU;

                int targetChainL = -1, targetChainR = -1;
                if (rightIsU8) {
                    targetChainL = 6; // U8::CHAIN_LEFT_INPUT
                    targetChainR = 7; // U8::CHAIN_RIGHT_INPUT
                } else if (rightIsYamanote) {
                    targetChainL = 16; // YAMANOTE::CHAIN_L_INPUT
                    targetChainR = 17; // YAMANOTE::CHAIN_R_INPUT
                } else if (rightIsAlex) {
                    targetChainL = ALEX_TRACKS * 6; // CHAIN_LEFT_INPUT
                    targetChainR = ALEX_TRACKS * 6 + 1;
                } else if (rightIsShinjuku) {
                    targetChainL = 8 * 6; // SHINJUKU CHAIN_LEFT_INPUT
                    targetChainR = 8 * 6 + 1;
                }

                if (targetChainL >= 0) {
                    bool leftConnected = rightModule->inputs[targetChainL].isConnected();
                    bool rightConnected = rightModule->inputs[targetChainR].isConnected();

                    if (!leftConnected) {
                        Cable* cableL = new Cable;
                        cableL->outputModule = module;
                        cableL->outputId = ALEXANDERPLATZ::LEFT_OUTPUT;
                        cableL->inputModule = rightModule;
                        cableL->inputId = targetChainL;
                        APP->engine->addCable(cableL);
                        autoChainLeftCableId = cableL->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableL);
                        cw->color = color::fromHexString("#004F7C");
                        APP->scene->rack->addCable(cw);
                    }

                    if (!rightConnected) {
                        Cable* cableR = new Cable;
                        cableR->outputModule = module;
                        cableR->outputId = ALEXANDERPLATZ::RIGHT_OUTPUT;
                        cableR->inputModule = rightModule;
                        cableR->inputId = targetChainR;
                        APP->engine->addCable(cableR);
                        autoChainRightCableId = cableR->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableR);
                        cw->color = color::fromHexString("#004F7C");
                        APP->scene->rack->addCable(cw);
                    }
                }
            }
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        ALEXANDERPLATZ* module = dynamic_cast<ALEXANDERPLATZ*>(this->module);
        if (!module) return;
        addPanelThemeMenu(menu, module);
    }
};

Model* modelALEXANDERPLATZ = createModel<ALEXANDERPLATZ, ALEXANDERPLATZWidget>("ALEXANDERPLATZ");
