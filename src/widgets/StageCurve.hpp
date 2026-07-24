#pragma once
#include <rack.hpp>

// MAD REPEATER (host) 與 F (expander) 共用的段落定義與曲線顯示元件。
// 兩個模組的 param / light 索引共用同一組 enum，避免專案既有 expander
// 那種硬編碼 params[3] / params[12+t] 造成的靜默錯位。

namespace madzine {
namespace stage {

using namespace rack;

enum StageParamIds {
    STAGE_LEVEL_PARAM,
    STAGE_CURVE_PARAM,
    STAGE_DURATION_PARAM,
    STAGE_SYNC_PARAM,
    STAGE_NUM_PARAMS
};

// expander 的輸出由 host 直接寫入（host 不需要知道 F 的型別，只靠這組共用索引）
enum StageOutputIds {
    STAGE_TRIG_OUTPUT,
    STAGE_ENV_OUTPUT,
    STAGE_NUM_OUTPUTS
};

enum StageLightIds {
    STAGE_NUM_LIGHTS
};

static constexpr float STAGE_MIN_DURATION = 0.001f;
static constexpr float STAGE_MAX_DURATION = 10.f;
static constexpr float STAGE_MAX_LEVEL = 10.f;

// DURATION 旋鈕 0..1 → 1ms..10s 指數映射
inline float stageDuration(float knob) {
    return STAGE_MIN_DURATION * std::pow(STAGE_MAX_DURATION / STAGE_MIN_DURATION, clamp(knob, 0.f, 1.f));
}

// SYNC 模式下 DURATION 旋鈕改讀比例：中央 = 1 拍，左邊分頻(div)，右邊倍頻(mult)
static const float STAGE_RATIOS[] = {
    1.f / 16.f, 1.f / 12.f, 1.f / 8.f, 1.f / 6.f, 1.f / 4.f, 1.f / 3.f, 1.f / 2.f, 2.f / 3.f,
    1.f,
    3.f / 2.f, 2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f
};
static constexpr int STAGE_NUM_RATIOS = 17;

static const char* STAGE_RATIO_LABELS[] = {
    "1/16", "1/12", "1/8", "1/6", "1/4", "1/3", "1/2", "2/3",
    "1",
    "3/2", "2", "3", "4", "6", "8", "12", "16"
};

inline int stageRatioIndex(float knob) {
    int i = (int) std::round(clamp(knob, 0.f, 1.f) * (STAGE_NUM_RATIOS - 1));
    return clamp(i, 0, STAGE_NUM_RATIOS - 1);
}

inline float stageRatio(float knob) {
    return STAGE_RATIOS[stageRatioIndex(knob)];
}

// DURATION 的 tooltip：SYNC 關顯示秒數，SYNC 開顯示 div/mult 分數
struct StageDurationQuantity : ParamQuantity {
    int syncParamId = -1;

    bool isSynced() {
        if (!module || syncParamId < 0) return false;
        return module->params[syncParamId].getValue() > 0.5f;
    }

    std::string getDisplayValueString() override {
        if (!isSynced()) return ParamQuantity::getDisplayValueString();
        int i = stageRatioIndex(getValue());
        const char* r = STAGE_RATIO_LABELS[i];
        if (i < 8) return std::string("/") + (r + 2) + " beat";   // 1/16 → "/16 beat"
        if (i == 8) return "1 beat";
        return std::string("x") + r + " beat";
    }

    std::string getUnit() override {
        return isSynced() ? "" : ParamQuantity::getUnit();
    }
};

// J taper：curvature 0 = 線性，正值凹（慢起快收），負值凸。
//
// 公式取自 DHE Modules 的 sigmoid.hpp curve()：
//   Copyright 2018-2021 Dale H. Emery
//   https://github.com/dhemery/DHE-Modules — MIT License
// MIT 授權全文見本專案的 LICENSE-dist.txt。
// 此處輸入已 clamp 到 0..1，故省去原式的 abs()。
inline float stageTaper(float x, float curvature) {
    float k = clamp(curvature, -0.9999f, 0.9999f);
    x = clamp(x, 0.f, 1.f);
    return (x - k * x) / (k - 2.f * k * x + 1.f);
}

struct StageDisplayData {
    float startVoltage = 0.f;
    float endVoltage = 5.f;
    float curvature = 0.f;
    float phase = 0.f;
    bool active = false;
    bool chained = false;
};

// expander 以多重繼承提供 host 存取顯示狀態的介面，
// host 用 dynamic_cast 取得，不需要引用 F 的完整型別。
struct StageExpanderBase {
    StageDisplayData stageDisp;
    virtual ~StageExpanderBase() {}
};

struct StageCurveDisplay : TransparentWidget {
    StageDisplayData* data = nullptr;

    StageCurveDisplay(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    float yForVoltage(float v) const {
        float t = clamp(v / STAGE_MAX_LEVEL, 0.f, 1.f);
        return 3.f + (1.f - t) * (box.size.y - 6.f);
    }

    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 20));
        for (int i = 1; i < 4; i++) {
            float y = box.size.y * i / 4.f;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, y);
            nvgLineTo(args.vg, box.size.x, y);
            nvgStroke(args.vg);
        }

        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStroke(args.vg);

        TransparentWidget::draw(args);
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) {
            TransparentWidget::drawLayer(args, layer);
            return;
        }

        // module 為 nullptr（module browser 預覽）時畫預設示意曲線
        StageDisplayData preview;
        const StageDisplayData* d = data ? data : &preview;

        const float pad = 3.f;
        const float w = box.size.x - 2.f * pad;
        const int segments = 48;

        nvgSave(args.vg);
        nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);

        nvgBeginPath(args.vg);
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            float v = d->startVoltage + (d->endVoltage - d->startVoltage) * stageTaper(t, d->curvature);
            float x = pad + t * w;
            float y = yForVoltage(v);
            if (i == 0)
                nvgMoveTo(args.vg, x, y);
            else
                nvgLineTo(args.vg, x, y);
        }
        nvgStrokeColor(args.vg, d->active ? nvgRGB(255, 200, 0) : nvgRGBA(255, 200, 0, 90));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStroke(args.vg);

        if (d->active) {
            float t = clamp(d->phase, 0.f, 1.f);
            float x = pad + t * w;
            float v = d->startVoltage + (d->endVoltage - d->startVoltage) * stageTaper(t, d->curvature);
            float y = yForVoltage(v);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x, 0);
            nvgLineTo(args.vg, x, box.size.y);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 90));
            nvgStrokeWidth(args.vg, 1.f);
            nvgStroke(args.vg);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, x, y, 2.5f);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            nvgFill(args.vg);
        }

        nvgResetScissor(args.vg);
        nvgRestore(args.vg);

        TransparentWidget::drawLayer(args, layer);
    }
};

} // namespace stage
} // namespace madzine
