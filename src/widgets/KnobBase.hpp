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

    // CV 調變顯示屬性
    float cvModulation = 0.0f;                              // -1.0 ~ +1.0 正規化調變量
    bool modulationEnabled = false;                          // 是否啟用調變顯示
    NVGcolor modPositiveColor = KnobColors::MOD_POSITIVE;   // 正向調變顏色
    NVGcolor modNegativeColor = KnobColors::MOD_NEGATIVE;   // 負向調變顏色
    float modIndicatorWidth = 1.5f;                          // 副指示器線寬

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

    // ========================================
    // CV 調變顯示 API
    // ========================================

    /**
     * 設定 CV 調變量（模組每幀呼叫）
     * @param normalizedMod 正規化調變量 -1.0 ~ +1.0
     */
    void setModulation(float normalizedMod) {
        cvModulation = clamp(normalizedMod, -1.0f, 1.0f);
    }

    /**
     * 啟用/停用調變顯示
     */
    void setModulationEnabled(bool enabled) {
        modulationEnabled = enabled;
    }

    /**
     * 檢查調變顯示是否啟用
     */
    bool isModulationEnabled() const {
        return modulationEnabled;
    }

    /**
     * 取得調變後的實際角度
     */
    float getModulatedAngle() {
        float baseAngle = getDisplayAngle();
        float modRange = KnobAngles::MAX_ANGLE - KnobAngles::MIN_ANGLE;
        float modAngle = baseAngle + cvModulation * modRange;
        return clamp(modAngle, KnobAngles::MIN_ANGLE, KnobAngles::MAX_ANGLE);
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
     * 繪製 CV 調變副指示器
     * 顯示調變後的實際值位置
     */
    virtual void drawModulationIndicator(const DrawArgs& args, float radius, float modAngle) {
        if (!modulationEnabled || cvModulation == 0.0f) return;

        float indicatorLength = radius - indicatorMargin - 1.0f;  // 比主指示器稍短
        float lineX = radius + indicatorLength * std::sin(modAngle);
        float lineY = radius - indicatorLength * std::cos(modAngle);

        // 選擇顏色：正向調變=青色，負向調變=橙色
        NVGcolor modColor = (cvModulation > 0.0f) ? modPositiveColor : modNegativeColor;

        // 副指示線
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, modIndicatorWidth);
        nvgStrokeColor(args.vg, modColor);
        nvgStroke(args.vg);

        // 副指示點
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 1.5f);
        nvgFillColor(args.vg, modColor);
        nvgFill(args.vg);
    }

    /**
     * 繪製 mapping indicator（供 Stoermelder TRANSIT、CV-MAP 等外部映射模組使用）
     * VCV Rack 使用 ParamHandle 來追蹤被映射的參數
     */
    virtual void drawMappingIndicator(const DrawArgs& args) {
        if (!module || paramId < 0) return;

        // 從 Engine 取得此參數的 ParamHandle
        engine::ParamHandle* paramHandle = APP->engine->getParamHandle(module->id, paramId);
        if (!paramHandle) return;

        // 檢查 ParamHandle 是否有設定顏色（alpha > 0 表示有映射）
        if (paramHandle->color.a <= 0.f) return;

        // 繪製 mapping indicator（小圓點，位於右下角）
        float indicatorRadius = std::min(box.size.x, box.size.y) * 0.08f;
        indicatorRadius = clamp(indicatorRadius, 2.f, 3.5f);

        float x = box.size.x - indicatorRadius - 2.f;
        float y = box.size.y - indicatorRadius - 2.f;

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, x, y, indicatorRadius);
        nvgFillColor(args.vg, paramHandle->color);
        nvgFill(args.vg);
    }

    /**
     * 主要繪製函數
     */
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float baseAngle = getDisplayAngle();

        // 1. 繪製旋鈕本體
        drawKnob(args, radius);

        // 2. 繪製 CV 調變副指示器（在下層）
        if (modulationEnabled && cvModulation != 0.0f) {
            float modAngle = getModulatedAngle();
            drawModulationIndicator(args, radius, modAngle);
        }

        // 3. 繪製主指示器（在上層）
        drawIndicator(args, radius, baseAngle);

        // 4. 繪製 mapping indicator（供 Stoermelder TRANSIT 等外部映射模組使用）
        drawMappingIndicator(args);
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        // 呼叫父類的 drawLayer 以支援任何 layer-based 渲染
        app::Knob::drawLayer(args, layer);
    }

    /**
     * 雙擊歸零功能 - 使用參數的 defaultValue
     */
    void onDoubleClick(const event::DoubleClick& e) override {
        if (enableDoubleClickReset) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                // 直接使用 setValue + getDefaultValue 確保正確歸位
                pq->setValue(pq->getDefaultValue());
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