// MAD REPEATER — 可用 F expander 串成多段的包絡產生器。
//
// 致敬 Dale Emmerson 的 DHE Modules「Booster Stage / Stage」系列。
// 把包絡拆成一段一段、每段自己有 level / curve / duration、
// 需要幾段就接幾段 —— 這個做法是他先想出來的，不是我。
//
// expander 串鏈的形式則是 VCV 社群長年的慣例（host 在左，expander 往右長）。
//
// 模組架構、時鐘同步、鏈掃描這些是重寫的。
// 但曲線公式 stageTaper()（widgets/StageCurve.hpp）直接取自 DHE 的
// sigmoid.hpp curve()：Copyright 2018-2021 Dale H. Emery，MIT License。
// 出處與授權見 LICENSE-dist.txt。

#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "widgets/StageCurve.hpp"

using namespace madzine::stage;

struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    EnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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

        if (bold) {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
            nvgStrokeColor(args.vg, color);
            nvgStrokeWidth(args.vg, 0.3f);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        } else {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        }
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

struct MADREPEATER : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;

    enum ParamIds {
        // 第 0 段的終點沿用共用索引，起點是主模組獨有的參數
        END_PARAM = STAGE_LEVEL_PARAM,
        CURVE_PARAM = STAGE_CURVE_PARAM,
        DURATION_PARAM = STAGE_DURATION_PARAM,
        SYNC_PARAM = STAGE_SYNC_PARAM,
        START_PARAM = STAGE_NUM_PARAMS,
        LOOP_PARAM,
        TRIG_BUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIGGER_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENVELOPE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // 本段（第 0 段）的顯示狀態
    StageDisplayData stageDisp;

    dsp::SchmittTrigger trigTrigger;
    dsp::BooleanTrigger buttonTrigger;

    // SYNC：TRIGGER 兼作時鐘，量測 pulse 間隔換算段長
    int ppqn = 1;
    float clockPeriod = 0.5f;
    float timeSinceClock = 0.f;
    bool clockValid = false;

    // 右側 F expander 鏈（降頻重掃）
    std::vector<Module*> chainModules;
    std::vector<StageExpanderBase*> chainStages;
    int scanCounter = 0;
    static constexpr int SCAN_INTERVAL = 64;
    static constexpr int MAX_STAGES = 64;
    // 每一段開始瞬間的 trigger（索引 1..N 對應第 N 個 expander），固定長度避免 audio thread 配置
    dsp::PulseGenerator stageTrigs[MAX_STAGES + 1];

    bool running = false;
    int stageIndex = 0;      // 0 = 本模組，1..N = 第 N 個 expander
    float phase = 0.f;
    float stageStart = 0.f;  // 目前這一段的起始電位
    float outVoltage = 0.f;

    MADREPEATER() {
        // 先配好容量，避免每個 sample 重掃時在 audio thread 上配置記憶體
        chainModules.reserve(64);
        chainStages.reserve(64);
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(START_PARAM, 0.f, STAGE_MAX_LEVEL, 0.f, "Start level", " V");
        configParam(END_PARAM, 0.f, STAGE_MAX_LEVEL, 5.f, "End level", " V");
        configParam(CURVE_PARAM, -1.f, 1.f, 0.f, "Curve");
        // 0..1 → 1ms..10s 指數顯示：0.001 * 10000^v；SYNC 開啟時改顯示 div/mult
        configParam<StageDurationQuantity>(DURATION_PARAM, 0.f, 1.f, 0.5f, "Duration", " s",
                    STAGE_MAX_DURATION / STAGE_MIN_DURATION, STAGE_MIN_DURATION);
        dynamic_cast<StageDurationQuantity*>(paramQuantities[DURATION_PARAM])->syncParamId = SYNC_PARAM;
        configSwitch(SYNC_PARAM, 0.f, 1.f, 0.f, "Sync this stage to clock", {"Off", "On"});
        configSwitch(LOOP_PARAM, 0.f, 1.f, 0.f, "Loop", {"Off", "On"});
        configButton(TRIG_BUTTON_PARAM, "Manual trigger");

        configInput(TRIGGER_INPUT, "Trigger");
        configOutput(ENVELOPE_OUTPUT, "Envelope");
    }

    // 必須每個 sample 都現走一次，不能快取跨 sample：
    // expander 被刪除後快取的 Module* 立刻懸空，下一次重掃前解參考會存取已釋放記憶體。
    // Rack 在 engine 鎖內移除模組時會把相鄰模組的 expander 指標設為 nullptr，所以現走是安全的。
    void rescanChain() {
        chainModules.clear();
        chainStages.clear();
        Module* m = rightExpander.module;
        while (m && m->model == modelF && (int)chainModules.size() < MAX_STAGES) {
            StageExpanderBase* s = dynamic_cast<StageExpanderBase*>(m);
            if (!s) break;
            chainModules.push_back(m);
            chainStages.push_back(s);
            m = m->rightExpander.module;
        }
        // 鏈長度改變時，若目前段索引已落在鏈外就收回
        if (stageIndex > (int)chainStages.size()) {
            stageIndex = 0;
            phase = 0.f;
            running = false;
        }
    }

    float stageLevel(int i) {
        if (i == 0) return params[END_PARAM].getValue();
        return chainModules[i - 1]->params[STAGE_LEVEL_PARAM].getValue();
    }
    float stageCurve(int i) {
        if (i == 0) return params[CURVE_PARAM].getValue();
        return chainModules[i - 1]->params[STAGE_CURVE_PARAM].getValue();
    }
    // 每一段各自決定要不要跟時鐘
    bool stageSynced(int i) {
        if (i == 0) return params[SYNC_PARAM].getValue() > 0.5f;
        return chainModules[i - 1]->params[STAGE_SYNC_PARAM].getValue() > 0.5f;
    }
    bool anyStageSynced() {
        if (params[SYNC_PARAM].getValue() > 0.5f) return true;
        for (Module* m : chainModules)
            if (m->params[STAGE_SYNC_PARAM].getValue() > 0.5f) return true;
        return false;
    }

    float stageTime(int i) {
        float knob = (i == 0) ? params[DURATION_PARAM].getValue()
                              : chainModules[i - 1]->params[STAGE_DURATION_PARAM].getValue();
        // 該段 SYNC 開啟且已量到時鐘週期時，段長 = 一拍 × 比例；量不到就退回自由模式秒數
        if (stageSynced(i) && clockValid) {
            float beat = clockPeriod * (float) ppqn;
            return clamp(beat * stageRatio(knob), STAGE_MIN_DURATION, 600.f);
        }
        return stageDuration(knob);
    }

    void updateDisplays() {
        int total = (int)chainStages.size() + 1;
        // 第一段的起點由 START 旋鈕決定，是固定值，不受 outVoltage 變動影響
        float prev = params[START_PARAM].getValue();
        for (int i = 0; i < total; i++) {
            StageDisplayData& d = (i == 0) ? stageDisp : chainStages[i - 1]->stageDisp;
            bool isActive = running && stageIndex == i;
            d.startVoltage = isActive ? stageStart : prev;
            d.endVoltage = stageLevel(i);
            d.curvature = stageCurve(i);
            d.active = isActive;
            d.phase = isActive ? phase : 0.f;
            d.chained = true;
            prev = d.endVoltage;
        }
    }

    void process(const ProcessArgs& args) override {
        rescanChain();
        if (++scanCounter >= SCAN_INTERVAL)
            scanCounter = 0;

        int lastStage = (int)chainStages.size();

        // 鏈中任一段開了 SYNC，就套用「pulse 不打斷運行中的鏈」規則
        bool sync = anyStageSynced();
        bool clockEdge = trigTrigger.process(inputs[TRIGGER_INPUT].getVoltage(), 0.1f, 1.f);

        // 時鐘週期量測：只有兩個 pulse 之間才有間隔可用
        timeSinceClock += args.sampleTime;
        if (clockEdge) {
            if (timeSinceClock > 1e-4f && timeSinceClock < 10.f) {
                clockPeriod = timeSinceClock;
                clockValid = true;
            }
            timeSinceClock = 0.f;
        }
        // 時鐘停掉超過 4 個週期就視為失效，不再用舊週期一直跑
        if (clockValid && timeSinceClock > clockPeriod * 4.f)
            clockValid = false;

        bool start = buttonTrigger.process(params[TRIG_BUTTON_PARAM].getValue() > 0.5f);
        if (clockEdge) {
            // SYNC 開啟時 TRIGGER 只當時鐘，但鏈停著時仍由時鐘起頭，否則沒有東西能啟動它
            if (!sync || !running) start = true;
        }

        if (start) {
            running = true;
            stageIndex = 0;
            phase = 0.f;
            stageStart = params[START_PARAM].getValue();
            stageTrigs[0].trigger(1e-3f);
        }

        if (running) {
            float level = stageLevel(stageIndex);
            float curve = stageCurve(stageIndex);
            float duration = stageTime(stageIndex);

            phase += args.sampleTime / duration;

            if (phase >= 1.f) {
                outVoltage = level;
                phase = 0.f;
                if (stageIndex >= lastStage) {
                    if (params[LOOP_PARAM].getValue() > 0.5f) {
                        // 繞回第 0 段，起點一律回到 START；START ≠ 尾段終點時會有跳變
                        stageIndex = 0;
                        stageStart = params[START_PARAM].getValue();
                        stageTrigs[0].trigger(1e-3f);
                    } else {
                        running = false;
                        stageIndex = 0;
                    }
                } else {
                    stageIndex++;
                    stageStart = outVoltage;
                    stageTrigs[stageIndex].trigger(1e-3f);
                }
            } else {
                outVoltage = stageStart + (level - stageStart) * stageTaper(phase, curve);
            }
        }

        outputs[ENVELOPE_OUTPUT].setVoltage(outVoltage);

        // 每個 expander 的兩個輸出由 host 寫入：
        // OUT — 正在跑這一段時跟著包絡，其餘時間維持該段的終點電壓
        // TRIG — 這一段開始的瞬間送 1ms pulse
        for (int i = 0; i < (int)chainModules.size(); i++) {
            int s = i + 1;
            bool isActive = running && stageIndex == s;
            Module* m = chainModules[i];
            m->outputs[STAGE_ENV_OUTPUT].setVoltage(isActive ? outVoltage : stageLevel(s));
            m->outputs[STAGE_TRIG_OUTPUT].setVoltage(stageTrigs[s].process(args.sampleTime) ? 10.f : 0.f);
        }

        if (scanCounter == 0)
            updateDisplays();
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_object_set_new(rootJ, "ppqn", json_integer(ppqn));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ)
            panelTheme = json_integer_value(themeJ);
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ)
            panelContrast = json_real_value(contrastJ);
        json_t* ppqnJ = json_object_get(rootJ, "ppqn");
        if (ppqnJ)
            ppqn = json_integer_value(ppqnJ);
    }
};

struct MADREPEATERWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    MADREPEATERWidget(MADREPEATER* module) {
        setModule(module);
        panelThemeHelper.init(this, "6HP", module ? &module->panelContrast : nullptr);
        box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        float W = box.size.x;
        float centerX = W / 2.f;

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(W, 20), "MAD REPEATER", 10.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(W, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        StageCurveDisplay* curveDisplay = new StageCurveDisplay(Vec(4, 34), Vec(W - 8.f, 96));
        curveDisplay->data = module ? &module->stageDisp : nullptr;
        addChild(curveDisplay);

        // LEVEL 群組：標題 + START / END 子標籤 + 兩顆並排旋鈕
        float startX = centerX - 17.f;
        float endX = centerX + 17.f;

        addChild(new EnhancedTextLabel(Vec(0, 133), Vec(W, 10), "LEVEL", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(startX - 15.f, 144), Vec(30, 10), "START", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(endX - 15.f, 144), Vec(30, 10), "END", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(startX, 172), module, MADREPEATER::START_PARAM));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(endX, 172), module, MADREPEATER::END_PARAM));

        addChild(new EnhancedTextLabel(Vec(0, 191), Vec(W, 10), "CURVE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(centerX, 219), module, MADREPEATER::CURVE_PARAM));

        addChild(new EnhancedTextLabel(Vec(0, 237), Vec(W, 10), "DURATION", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(centerX, 265), module, MADREPEATER::DURATION_PARAM));

        float loopX = centerX - 25.72f;
        float manualX = centerX + 25.72f;

        addChild(new EnhancedTextLabel(Vec(loopX - 12.5f, 284), Vec(25, 10), "LOOP", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(centerX - 12.5f, 284), Vec(25, 10), "SYNC", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(manualX - 12.5f, 284), Vec(25, 10), "MANUAL", 8.f, nvgRGB(255, 255, 255), true));

        addParam(createParamCentered<CKSS>(Vec(loopX, 310), module, MADREPEATER::LOOP_PARAM));
        addParam(createParamCentered<CKSS>(Vec(centerX, 310), module, MADREPEATER::SYNC_PARAM));
        addParam(createParamCentered<VCVButton>(Vec(manualX, 310), module, MADREPEATER::TRIG_BUTTON_PARAM));

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(W, 50)));
        addChild(new EnhancedTextLabel(Vec(0, 336), Vec(30, 14), "TRIGGER", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(W - 30, 336), Vec(30, 14), "OUT", 7.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 362), module, MADREPEATER::TRIGGER_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(W - 15, 362), module, MADREPEATER::ENVELOPE_OUTPUT));
    }

    void step() override {
        if (auto* m = dynamic_cast<MADREPEATER*>(module)) {
            panelThemeHelper.step(m);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        auto* m = getModule<MADREPEATER>();
        if (!m) return;
        addPanelThemeMenu(menu, m);

        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("Sync PPQN", string::f("%d", m->ppqn), [=](ui::Menu* sub) {
            static const int ppqnOptions[] = {1, 2, 4, 8, 12, 24, 48};
            for (int v : ppqnOptions) {
                sub->addChild(createCheckMenuItem(string::f("%d PPQN", v), "",
                    [=]() { return m->ppqn == v; },
                    [=]() { m->ppqn = v; }));
            }
        }));
    }
};

Model* modelMADREPEATER = createModel<MADREPEATER, MADREPEATERWidget>("MADREPEATER");
