#pragma once
#include <rack.hpp>
#include "KnobStyles.hpp"

using namespace rack;

namespace madzine {
namespace widgets {

/**
 * 基礎自定義旋鈕類別
 * 提供 undo/redo 支援和基本繪製功能
 */
class BaseCustomKnob : public app::Knob {
protected:
    // 樣式屬性
    NVGcolor baseColor = KnobColors::BLACK_BASE;
    NVGcolor centerColor = KnobColors::BLACK_CENTER;
    NVGcolor borderColor = KnobColors::GRAY_BORDER;
    NVGcolor indicatorColor = KnobColors::WHITE_INDICATOR;
    float indicatorMargin = KnobSizes::INDICATOR_MARGIN;

    // 雙擊歸零功能開關
    bool enableDoubleClickReset = true;

public:
    BaseCustomKnob() : app::Knob() {
        // 預設使用標準尺寸
        box.size = Vec(KnobSizes::STANDARD, KnobSizes::STANDARD);
        // 預設靈敏度
        speed = KnobSensitivity::SLOW;
        // Ensure snap is false for regular knobs
        snap = false;
    }

    void initParamQuantity() override {
        app::Knob::initParamQuantity();
        // Additional initialization if needed
    }

    /**
     * 取得顯示角度 (根據參數值)
     */
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;

        float normalizedValue = pq->getScaledValue();
        float angle = rescale(normalizedValue, 0.0f, 1.0f,
                            KnobAngles::MIN_ANGLE, KnobAngles::MAX_ANGLE);
        return angle;
    }

    /**
     * 繪製旋鈕本體
     */
    virtual void drawKnob(const DrawArgs& args, float radius) {
        // 外圈
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, baseColor);
        nvgFill(args.vg);

        // 邊框
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, borderColor);
        nvgStroke(args.vg);

        // 內圈
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, centerColor);
        nvgFill(args.vg);
    }

    /**
     * 繪製指示器
     */
    virtual void drawIndicator(const DrawArgs& args, float radius, float angle) {
        float indicatorLength = radius - indicatorMargin;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);

        // 指示線
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, indicatorColor);
        nvgStroke(args.vg);

        // 指示點
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, indicatorColor);
        nvgFill(args.vg);
    }

    /**
     * 主要繪製函數
     */
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();

        drawKnob(args, radius);
        drawIndicator(args, radius, angle);
    }

    /**
     * 雙擊歸零功能 - 智能歸零到合適的值
     * 這樣已經有自定義 onDoubleClick 的模組不會被影響
     */
    void onDoubleClick(const event::DoubleClick& e) override {
        if (enableDoubleClickReset) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                float minValue = pq->getMinValue();
                float maxValue = pq->getMaxValue();
                float targetValue;

                // 如果範圍包含 0，歸零
                if (minValue <= 0.0f && maxValue >= 0.0f) {
                    targetValue = 0.0f;
                }
                // 如果範圍不包含 0，歸到中點
                else {
                    targetValue = (minValue + maxValue) / 2.0f;
                }

                pq->setValue(targetValue);
                e.consume(this);
                return;
            }
        }
        // 如果沒有處理，讓父類處理
        app::Knob::onDoubleClick(e);
    }

};

/**
 * Snap 旋鈕基礎類別
 * 提供整數跳躍功能
 */
class BaseSnapKnob : public BaseCustomKnob {
public:
    BaseSnapKnob() : BaseCustomKnob() {
        // Critical: Set snap to true for VCV Rack's undo system
        snap = true;
    }

    // Use parent class's snap functionality, no need to override onDragMove
};

/**
 * 隱藏旋鈕基礎類別
 * 無視覺化但可拖動
 */
class BaseHiddenKnob : public BaseCustomKnob {
public:
    BaseHiddenKnob() : BaseCustomKnob() {
        // 隱藏旋鈕通常很小
        box.size = Vec(1, 1);
    }

    void draw(const DrawArgs& args) override {
        // 不繪製任何東西
    }
};

} // namespace widgets
} // namespace madzine