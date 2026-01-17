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
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
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

// Exclusive Solo Button：長按 = exclusive（取消其他所有 solo）
template <typename TLight>
struct ExclusiveSoloButton : VCVLightLatch<TLight> {
    float pressTime = 0.f;
    bool pressing = false;
    bool exclusiveTriggered = false; // 防止重複觸發
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
                    // 取消整個 chain 中其他所有 solo
                    // 往左追蹤
                    Module* mod = module->leftExpander.module;
                    while (mod) {
                        if (mod->model == modelU8) {
                            mod->params[3].setValue(0.f); // U8::SOLO_PARAM
                        } else if (mod->model == modelALEXANDERPLATZ) {
                            for (int t = 0; t < 4; t++) {
                                mod->params[12 + t].setValue(0.f); // ALEX::SOLO_PARAM + t
                            }
                        } else if (mod->model == modelSHINJUKU) {
                            for (int t = 0; t < 8; t++) {
                                mod->params[24 + t].setValue(0.f); // SHINJUKU::SOLO_PARAM + t
                            }
                        } else {
                            break;
                        }
                        mod = mod->leftExpander.module;
                    }
                    // 往右追蹤
                    mod = module->rightExpander.module;
                    while (mod) {
                        if (mod->model == modelU8) {
                            mod->params[3].setValue(0.f);
                        } else if (mod->model == modelALEXANDERPLATZ) {
                            for (int t = 0; t < 4; t++) {
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

struct U8 : Module {
    int panelTheme = -1;
    float panelContrast = panelContrastDefault; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        LEVEL_PARAM,
        DUCK_LEVEL_PARAM,
        MUTE_PARAM,
        SOLO_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        LEFT_INPUT,
        RIGHT_INPUT,
        DUCK_INPUT,
        LEVEL_CV_INPUT,
        MUTE_TRIG_INPUT,
        SOLO_TRIG_INPUT,
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
        SOLO_LIGHT,
        LIGHTS_LEN
    };

    static constexpr int DELAY_BUFFER_SIZE = 2048;
    static constexpr int MAX_POLY = 16;
    float delayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    int delayWriteIndex[MAX_POLY];

    bool muteState = false;
    dsp::SchmittTrigger muteTrigger;
    bool soloState = false;
    dsp::SchmittTrigger soloTrigger;

    // CV 調變顯示用
    float levelCvModulation = 0.0f;

    // VU Meter 電平值（dB，範圍 -60 到 +6）
    float vuLevelL = -60.0f;
    float vuLevelR = -60.0f;

    // Expander 輸出資料（供右側 U8 模組讀取）
    float expanderOutputL[MAX_POLY] = {0};
    float expanderOutputR[MAX_POLY] = {0};
    int expanderOutputLChannels = 0;
    int expanderOutputRChannels = 0;

    U8() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(LEVEL_PARAM, 0.0f, 2.0f, 1.0f, "Level");
        configParam(DUCK_LEVEL_PARAM, 0.0f, 1.0f, 0.0f, "Duck Level");
        configSwitch(MUTE_PARAM, 0.0f, 1.0f, 0.0f, "Mute", {"Unmuted", "Muted"});
        configSwitch(SOLO_PARAM, 0.0f, 1.0f, 0.0f, "Solo", {"Off", "Solo"});
        getParamQuantity(SOLO_PARAM)->description = "Hold for exclusive";

        configInput(LEFT_INPUT, "Left Audio");
        configInput(RIGHT_INPUT, "Right Audio");
        configInput(DUCK_INPUT, "Duck Signal");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(MUTE_TRIG_INPUT, "Mute Trigger");
        configInput(SOLO_TRIG_INPUT, "Solo Trigger");
        configInput(CHAIN_LEFT_INPUT, "Chain Left");
        configInput(CHAIN_RIGHT_INPUT, "Chain Right");

        configOutput(LEFT_OUTPUT, "Left Audio");
        configOutput(RIGHT_OUTPUT, "Right Audio");

        configLight(MUTE_LIGHT, "Mute Indicator");
        configLight(SOLO_LIGHT, "Solo Indicator");

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
        // Handle mute trigger (monophonic)
        if (inputs[MUTE_TRIG_INPUT].isConnected()) {
            if (muteTrigger.process(inputs[MUTE_TRIG_INPUT].getVoltage())) {
                muteState = !muteState;
                params[MUTE_PARAM].setValue(muteState ? 1.0f : 0.0f);
            }
        }

        // Handle solo trigger (monophonic)
        if (inputs[SOLO_TRIG_INPUT].isConnected()) {
            if (soloTrigger.process(inputs[SOLO_TRIG_INPUT].getVoltage())) {
                soloState = !soloState;
                params[SOLO_PARAM].setValue(soloState ? 1.0f : 0.0f);
            }
        }

        bool muted = params[MUTE_PARAM].getValue() > 0.5f;
        bool soloed = params[SOLO_PARAM].getValue() > 0.5f;

        // 跨模組 Solo 邏輯：檢查整個 chain 是否有任何軌道被 solo
        bool chainHasSolo = soloed;

        // 往左追蹤（使用硬編碼索引：U8 SOLO=3, ALEX SOLO=12+t, SHINJUKU SOLO=24+t）
        Module* mod = leftExpander.module;
        while (mod && !chainHasSolo) {
            if (mod->model == modelU8) {
                if (mod->params[3].getValue() > 0.5f) chainHasSolo = true; // SOLO_PARAM
            } else if (mod->model == modelALEXANDERPLATZ) {
                for (int t = 0; t < 4 && !chainHasSolo; t++) {
                    if (mod->params[12 + t].getValue() > 0.5f) chainHasSolo = true; // ALEX::SOLO_PARAM + t
                }
            } else if (mod->model == modelSHINJUKU) {
                for (int t = 0; t < 8 && !chainHasSolo; t++) {
                    if (mod->params[24 + t].getValue() > 0.5f) chainHasSolo = true; // SHINJUKU::SOLO_PARAM + t
                }
            } else {
                break; // 遇到非 mixer 模組停止
            }
            mod = mod->leftExpander.module;
        }

        // 往右追蹤
        mod = rightExpander.module;
        while (mod && !chainHasSolo) {
            if (mod->model == modelU8) {
                if (mod->params[3].getValue() > 0.5f) chainHasSolo = true;
            } else if (mod->model == modelALEXANDERPLATZ) {
                for (int t = 0; t < 4 && !chainHasSolo; t++) {
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

        // 如果 chain 有 solo 但自己沒有 solo，則視為 muted
        bool soloMuted = chainHasSolo && !soloed;
        if (soloMuted) {
            muted = true;
        }

        // 燈號：mute 燈在被 solo 靜音時也要亮起
        lights[MUTE_LIGHT].setBrightness((params[MUTE_PARAM].getValue() > 0.5f || soloMuted) ? 1.0f : 0.0f);
        lights[SOLO_LIGHT].setBrightness(soloed ? 1.0f : 0.0f);

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

        // 計算 CV 調變量供 Widget 顯示
        if (inputs[LEVEL_CV_INPUT].isConnected()) {
            // ±5V = 滿範圍，所以除以 5 而非 10
            float cvNorm = clamp(inputs[LEVEL_CV_INPUT].getVoltage() / 10.0f, -1.0f, 1.0f);
            levelCvModulation = cvNorm;
        } else {
            levelCvModulation = 0.0f;
        }

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

            // Apply level control (±5V = 滿範圍)
            float level = levelParam;
            if (inputs[LEVEL_CV_INPUT].isConnected()) {
                int levelChan = (c < levelCvChannels) ? c : 0;
                float cvNorm = clamp(inputs[LEVEL_CV_INPUT].getPolyVoltage(levelChan) / 10.0f, -1.0f, 1.0f);
                // CV 直接加到旋鈕值上（以 1.0 為單位）
                level = levelParam + cvNorm;
                level = clamp(level, 0.0f, 2.0f);
            }

            leftInput *= level * sidechainCV;

            if (muted) {
                leftInput = 0.0f;
            }

            // 防爆音：限制輸出在 ±10V
            outputs[LEFT_OUTPUT].setVoltage(clamp(leftInput + chainLeftInput, -10.f, 10.f), c);
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

            // Apply level control (±5V = 滿範圍)
            float level = levelParam;
            if (inputs[LEVEL_CV_INPUT].isConnected()) {
                int levelChan = (c < levelCvChannels) ? c : 0;
                float cvNorm = clamp(inputs[LEVEL_CV_INPUT].getPolyVoltage(levelChan) / 10.0f, -1.0f, 1.0f);
                // CV 直接加到旋鈕值上（以 1.0 為單位）
                level = levelParam + cvNorm;
                level = clamp(level, 0.0f, 2.0f);
            }

            rightInput *= level * sidechainCV;

            if (muted) {
                rightInput = 0.0f;
            }

            // 防爆音：限制輸出在 ±10V
            outputs[RIGHT_OUTPUT].setVoltage(clamp(rightInput + chainRightInput, -10.f, 10.f), c);
        }

        // 儲存 output 給右側 U8 模組
        expanderOutputLChannels = outputLeftChannels;
        expanderOutputRChannels = outputRightChannels;
        for (int c = 0; c < outputLeftChannels; c++) {
            expanderOutputL[c] = outputs[LEFT_OUTPUT].getVoltage(c);
        }
        for (int c = 0; c < outputRightChannels; c++) {
            expanderOutputR[c] = outputs[RIGHT_OUTPUT].getVoltage(c);
        }

        // 計算 VU Meter 電平（從輸入取得，pre-level, pre-mute，不含 chain）
        float peakL = 0.0f, peakR = 0.0f;
        for (int c = 0; c < leftChannels; c++) {
            peakL = std::max(peakL, std::abs(inputs[LEFT_INPUT].getPolyVoltage(c)));
        }
        for (int c = 0; c < rightChannels; c++) {
            peakR = std::max(peakR, std::abs(inputs[RIGHT_INPUT].getPolyVoltage(c)));
        }
        // 如果只有 L 輸入，R 也顯示 L 的電平（因為會自動 delay 到 R）
        if (inputs[LEFT_INPUT].isConnected() && !inputs[RIGHT_INPUT].isConnected()) {
            peakR = peakL;
        }

        // 轉換為 dB（以 5V 為 0dB 參考）
        float dbL = (peakL > 0.0001f) ? 20.0f * log10f(peakL / 5.0f) : -60.0f;
        float dbR = (peakR > 0.0001f) ? 20.0f * log10f(peakR / 5.0f) : -60.0f;

        // 平滑 VU 電平（快上慢下）
        float attackCoeff = 1.0f - expf(-1.0f / (0.005f * args.sampleRate)); // 5ms attack
        float releaseCoeff = 1.0f - expf(-1.0f / (0.3f * args.sampleRate));   // 300ms release

        if (dbL > vuLevelL) {
            vuLevelL += (dbL - vuLevelL) * attackCoeff;
        } else {
            vuLevelL += (dbL - vuLevelL) * releaseCoeff;
        }

        if (dbR > vuLevelR) {
            vuLevelR += (dbR - vuLevelR) * attackCoeff;
        } else {
            vuLevelR += (dbR - vuLevelR) * releaseCoeff;
        }
    }

    void processBypass(const ProcessArgs& args) override {
        int chainLeftChannels = inputs[CHAIN_LEFT_INPUT].getChannels();
        int chainRightChannels = inputs[CHAIN_RIGHT_INPUT].getChannels();

        outputs[LEFT_OUTPUT].setChannels(chainLeftChannels);
        outputs[RIGHT_OUTPUT].setChannels(chainRightChannels);

        for (int c = 0; c < chainLeftChannels; c++) {
            outputs[LEFT_OUTPUT].setVoltage(clamp(inputs[CHAIN_LEFT_INPUT].getPolyVoltage(c), -10.f, 10.f), c);
        }

        for (int c = 0; c < chainRightChannels; c++) {
            outputs[RIGHT_OUTPUT].setVoltage(clamp(inputs[CHAIN_RIGHT_INPUT].getPolyVoltage(c), -10.f, 10.f), c);
        }
    }
};

// 橫向 VU Meter（連續條狀風格）
struct HorizontalVUMeter : TransparentWidget {
    U8* module = nullptr;
    bool isLeft = true; // true = L, false = R

    static constexpr float MIN_DB = -36.0f;
    static constexpr float MAX_DB = 6.0f;

    HorizontalVUMeter(Vec pos, Vec size, bool isLeft) {
        box.pos = pos;
        box.size = size;
        this->isLeft = isLeft;
    }

    void draw(const DrawArgs &args) override {
        float level = -60.0f;
        if (module) {
            level = isLeft ? module->vuLevelL : module->vuLevelR;
        }

        // 將 dB 值映射到 0-1 範圍
        float normalizedLevel = (level - MIN_DB) / (MAX_DB - MIN_DB);
        normalizedLevel = clamp(normalizedLevel, 0.0f, 1.0f);

        // 計算紅色閾值（0dB 以上）
        float redThreshold = (0.0f - MIN_DB) / (MAX_DB - MIN_DB);     // ~0.86

        float barWidth = box.size.x * normalizedLevel;
        float barHeight = box.size.y;

        // 繪製背景（暗淡色）
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, barHeight);
        nvgFillColor(args.vg, nvgRGBA(40, 40, 40, 255));
        nvgFill(args.vg);

        if (normalizedLevel > 0.0f) {
            // 使用漸層繪製連續條
            NVGpaint gradient = nvgLinearGradient(args.vg,
                0, 0, box.size.x, 0,
                nvgRGB(80, 180, 80),   // 綠色起點
                nvgRGB(255, 50, 50));  // 紅色終點

            // 繪製填充條
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, barWidth, barHeight);
            nvgFillPaint(args.vg, gradient);
            nvgFill(args.vg);

            // 如果超過紅色閾值，繪製紅色高亮
            if (normalizedLevel > redThreshold) {
                float redStart = box.size.x * redThreshold;
                nvgBeginPath(args.vg);
                nvgRect(args.vg, redStart, 0, barWidth - redStart, barHeight);
                nvgFillColor(args.vg, nvgRGB(255, 50, 50));
                nvgFill(args.vg);
            }
        }
    }
};

struct U8Widget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    TechnoStandardBlackKnob* levelKnob = nullptr;
    HorizontalVUMeter* vuMeterL = nullptr;
    HorizontalVUMeter* vuMeterR = nullptr;

    // 自動 cable 追蹤
    int64_t autoChainLeftCableId = -1;
    int64_t autoChainRightCableId = -1;
    Module* lastRightExpander = nullptr;

    U8Widget(U8* module) {
        setModule(module);
        panelThemeHelper.init(this, "4HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new BlueBackgroundBox(Vec(0, 1), Vec(box.size.x, 18)));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "U8", 14.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 16), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new TrainCarWidget(Vec(0, 31), Vec(box.size.x, 35)));

        addChild(new TechnoEnhancedTextLabel(Vec(0, 28), Vec(box.size.x, 16), "INPUT", 8.f, nvgRGB(255, 255, 255), true));

        addChild(new TechnoEnhancedTextLabel(Vec(-5, 89), Vec(box.size.x + 10, 10), "LEVEL", 10.5f, nvgRGB(255, 255, 255), true));
        levelKnob = createParamCentered<TechnoStandardBlackKnob>(Vec(box.size.x / 2, 123), module, U8::LEVEL_PARAM);
        addParam(levelKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2, 161), module, U8::LEVEL_CV_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(-5, 182), Vec(box.size.x + 10, 10), "DUCK", 10.5f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(box.size.x / 2, 216), module, U8::DUCK_LEVEL_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2, 254), module, U8::DUCK_INPUT));

        // Mute/Solo（與 Input L/R 對齊）
        addChild(new TechnoEnhancedTextLabel(Vec(-5, 270), Vec(box.size.x + 10, 10), "MUTE SOLO", 10.5f, nvgRGB(255, 255, 255), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(Vec(15, 292), module, U8::MUTE_PARAM, U8::MUTE_LIGHT));
        addParam(createLightParamCentered<ExclusiveSoloButton<MediumSimpleLight<GreenLight>>>(Vec(box.size.x - 15, 292), module, U8::SOLO_PARAM, U8::SOLO_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 316), module, U8::MUTE_TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x - 15, 316), module, U8::SOLO_TRIG_INPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(15, 59), module, U8::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(box.size.x - 15, 59), module, U8::RIGHT_INPUT));

        // 雙通道 VU Meter（Input 下方、LEVEL 標籤上方）
        vuMeterL = new HorizontalVUMeter(Vec(4, 71), Vec(box.size.x - 8, 5), true);
        vuMeterL->module = module;
        addChild(vuMeterL);

        vuMeterR = new HorizontalVUMeter(Vec(4, 79), Vec(box.size.x - 8, 5), false);
        vuMeterR->module = module;
        addChild(vuMeterR);

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

            // 更新 Level 旋鈕的 CV 調變顯示
            if (levelKnob) {
                bool cvConnected = module->inputs[U8::LEVEL_CV_INPUT].isConnected();
                levelKnob->setModulationEnabled(cvConnected);
                if (cvConnected) {
                    levelKnob->setModulation(module->levelCvModulation);
                }
            }

            // 自動 cable 創建（模組移開後保持連接，用戶可手動刪除）
            // 驗證自動 cable 是否仍然有效（可能被用戶刪除）
            if (autoChainLeftCableId >= 0 && !APP->engine->getCable(autoChainLeftCableId)) {
                autoChainLeftCableId = -1;
            }
            if (autoChainRightCableId >= 0 && !APP->engine->getCable(autoChainRightCableId)) {
                autoChainRightCableId = -1;
            }

            // 只在相鄰且尚未建立自動 cable 時創建
            Module* rightModule = module->rightExpander.module;
            if (rightModule && autoChainLeftCableId < 0 && autoChainRightCableId < 0) {
                int targetChainL = -1, targetChainR = -1;

                if (rightModule->model == modelU8) {
                    targetChainL = U8::CHAIN_LEFT_INPUT;
                    targetChainR = U8::CHAIN_RIGHT_INPUT;
                } else if (rightModule->model == modelYAMANOTE) {
                    targetChainL = 16; // YAMANOTE::CHAIN_L_INPUT
                    targetChainR = 17; // YAMANOTE::CHAIN_R_INPUT
                } else if (rightModule->model == modelALEXANDERPLATZ) {
                    targetChainL = 4 * 6;     // ALEX_TRACKS * 6 (6 input types per track)
                    targetChainR = 4 * 6 + 1;
                } else if (rightModule->model == modelSHINJUKU) {
                    targetChainL = 8 * 6;     // SHINJUKU_TRACKS * 6 (6 input types per track)
                    targetChainR = 8 * 6 + 1;
                }

                if (targetChainL >= 0) {
                    bool leftInputConnected = rightModule->inputs[targetChainL].isConnected();
                    bool rightInputConnected = rightModule->inputs[targetChainR].isConnected();

                    if (!leftInputConnected) {
                        Cable* cableL = new Cable;
                        cableL->outputModule = module;
                        cableL->outputId = U8::LEFT_OUTPUT;
                        cableL->inputModule = rightModule;
                        cableL->inputId = targetChainL;
                        APP->engine->addCable(cableL);
                        autoChainLeftCableId = cableL->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableL);
                        cw->color = color::fromHexString("#FFCC00"); // U8 黃色列車
                        APP->scene->rack->addCable(cw);
                    }

                    if (!rightInputConnected) {
                        Cable* cableR = new Cable;
                        cableR->outputModule = module;
                        cableR->outputId = U8::RIGHT_OUTPUT;
                        cableR->inputModule = rightModule;
                        cableR->inputId = targetChainR;
                        APP->engine->addCable(cableR);
                        autoChainRightCableId = cableR->id;

                        app::CableWidget* cw = new app::CableWidget;
                        cw->setCable(cableR);
                        cw->color = color::fromHexString("#FFCC00"); // U8 黃色列車
                        APP->scene->rack->addCable(cw);
                    }
                }
            }
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