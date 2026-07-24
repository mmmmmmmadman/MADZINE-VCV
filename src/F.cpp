// F — MAD REPEATER 的單段 expander。
//
// 致敬 Dale Emmerson 的 DHE Modules「Booster Stage / Stage」系列：
// 一段就是一段，level + curve + duration，接起來才是完整的包絡。
// 這個模組的存在理由完全來自那個想法。
// 模組本身是重寫的，但曲線公式 stageTaper()（widgets/StageCurve.hpp）
// 直接取自 DHE 的 sigmoid.hpp curve()：
//   Copyright 2018-2021 Dale H. Emery，MIT License。
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

// F：MAD REPEATER 的單段 expander。所有包絡運算都在左側的 MAD REPEATER 進行，
// 本模組只提供該段的參數與顯示狀態。
struct F : Module, StageExpanderBase {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;

    enum ParamIds {
        LEVEL_PARAM = STAGE_LEVEL_PARAM,
        CURVE_PARAM = STAGE_CURVE_PARAM,
        DURATION_PARAM = STAGE_DURATION_PARAM,
        SYNC_PARAM = STAGE_SYNC_PARAM,
        NUM_PARAMS = STAGE_NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    // 兩個輸出的電壓都由 host 每個 sample 直接寫入，F 自己不做包絡運算
    enum OutputIds {
        TRIG_OUTPUT = STAGE_TRIG_OUTPUT,
        OUT_OUTPUT = STAGE_ENV_OUTPUT,
        NUM_OUTPUTS = STAGE_NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS = STAGE_NUM_LIGHTS
    };

    int scanCounter = 0;
    static constexpr int SCAN_INTERVAL = 64;

    F() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LEVEL_PARAM, 0.f, STAGE_MAX_LEVEL, 5.f, "Level", " V");
        configParam(CURVE_PARAM, -1.f, 1.f, 0.f, "Curve");
        configParam<StageDurationQuantity>(DURATION_PARAM, 0.f, 1.f, 0.5f, "Duration", " s",
                    STAGE_MAX_DURATION / STAGE_MIN_DURATION, STAGE_MIN_DURATION);
        dynamic_cast<StageDurationQuantity*>(paramQuantities[DURATION_PARAM])->syncParamId = SYNC_PARAM;
        configSwitch(SYNC_PARAM, 0.f, 1.f, 0.f, "Sync this stage to clock", {"Off", "On"});

        configOutput(TRIG_OUTPUT, "Stage start trigger");
        configOutput(OUT_OUTPUT, "Stage envelope");
    }

    // 往左掃描，中間必須全部是 F，直到遇到 MAD REPEATER 才算接上鏈
    bool scanChained() {
        Module* m = leftExpander.module;
        while (m) {
            if (m->model == modelMADREPEATER) return true;
            if (m->model == modelF) {
                m = m->leftExpander.module;
                continue;
            }
            return false;
        }
        return false;
    }

    void process(const ProcessArgs& args) override {
        if (++scanCounter >= SCAN_INTERVAL) {
            scanCounter = 0;
            bool chained = scanChained();
            stageDisp.chained = chained;
            // 與 host 斷開時自行清除顯示，避免停在最後一次收到的狀態
            if (!chained) {
                stageDisp.active = false;
                stageDisp.phase = 0.f;
                stageDisp.startVoltage = 0.f;
                stageDisp.endVoltage = params[LEVEL_PARAM].getValue();
                stageDisp.curvature = params[CURVE_PARAM].getValue();
            }
        }

        // 沒接 host 就沒人寫輸出，這裡自行維持「未運行 = 終點電壓」的定義
        if (!stageDisp.chained) {
            outputs[TRIG_OUTPUT].setVoltage(0.f);
            outputs[OUT_OUTPUT].setVoltage(params[LEVEL_PARAM].getValue());
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
        if (themeJ)
            panelTheme = json_integer_value(themeJ);
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ)
            panelContrast = json_real_value(contrastJ);
    }
};

struct FWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    FWidget(F* module) {
        setModule(module);
        panelThemeHelper.init(this, "4HP", module ? &module->panelContrast : nullptr);
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        float W = box.size.x;
        float centerX = W / 2.f;

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(W, 20), "F", 10.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(W, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        StageCurveDisplay* curveDisplay = new StageCurveDisplay(Vec(4, 34), Vec(52.96f, 96));
        curveDisplay->data = module ? &module->stageDisp : nullptr;
        addChild(curveDisplay);

        addChild(new EnhancedTextLabel(Vec(0, 144), Vec(W, 10), "LEVEL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(centerX, 172), module, F::LEVEL_PARAM));

        addChild(new EnhancedTextLabel(Vec(0, 191), Vec(W, 10), "CURVE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(centerX, 219), module, F::CURVE_PARAM));

        addChild(new EnhancedTextLabel(Vec(0, 237), Vec(W, 10), "DURATION", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(centerX, 265), module, F::DURATION_PARAM));

        addChild(new EnhancedTextLabel(Vec(0, 284), Vec(W, 10), "SYNC", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<CKSS>(Vec(centerX, 310), module, F::SYNC_PARAM));

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(W, 50)));
        addChild(new EnhancedTextLabel(Vec(0, 336), Vec(30, 14), "TRIG", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new EnhancedTextLabel(Vec(30, 336), Vec(30, 14), "OUT", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 362), module, F::TRIG_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 362), module, F::OUT_OUTPUT));
    }

    void step() override {
        if (auto* m = dynamic_cast<F*>(module)) {
            panelThemeHelper.step(m);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        auto* m = getModule<F>();
        if (!m) return;
        addPanelThemeMenu(menu, m);
    }
};

Model* modelF = createModel<F, FWidget>("F");
