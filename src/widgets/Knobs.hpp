#pragma once
#include "KnobBase.hpp"
#include "KnobStyles.hpp"

using namespace rack;

namespace madzine {
namespace widgets {

// ============================================================================
// Phase 1: 核心旋鈕 (Core Knobs)
// ============================================================================

/**
 * StandardBlackKnob - 標準黑色旋鈕 (30×30px 版本)
 * 使用模組: PPaTTTerning, QQ, 以及大多數使用 30px 的模組
 * 尺寸: 30×30px
 */
struct StandardBlackKnob : BaseCustomKnob {
    StandardBlackKnob() {
        box.size = Vec(30, 30);
        speed = KnobSensitivity::SLOW;
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * StandardBlackKnob26 - 標準黑色旋鈕 (26×26px 版本)
 * 使用模組: Pyramid, EuclideanRhythm
 * 尺寸: 26×26px
 */
struct StandardBlackKnob26 : BaseCustomKnob {
    StandardBlackKnob26() {
        box.size = Vec(26, 26);
        speed = KnobSensitivity::SLOW;
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * SnapKnob26 - 26×26px 版本的 Snap 旋鈕
 * 使用模組: TWNCLight
 * 尺寸: 26×26px
 */
struct SnapKnob26 : BaseSnapKnob {
    SnapKnob26() {
        box.size = Vec(26, 26);
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * TechnoStandardBlackKnob - Techno 風格的標準黑色旋鈕
 * 使用模組: U8 (45×45px)
 * 尺寸: 45×45px
 */
struct TechnoStandardBlackKnob : BaseCustomKnob {
    TechnoStandardBlackKnob() {
        box.size = Vec(KnobSizes::TECHNO_STANDARD, KnobSizes::TECHNO_STANDARD);
        speed = KnobSensitivity::SLOW;
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * TechnoStandardBlackKnob30 - KIMO 專用的 Techno 旋鈕
 * 使用模組: KIMO
 * 尺寸: 30×30px
 */
struct TechnoStandardBlackKnob30 : BaseCustomKnob {
    TechnoStandardBlackKnob30() {
        box.size = Vec(30, 30);
        speed = KnobSensitivity::SLOW;
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * SmallGrayKnob - 小型灰色旋鈕
 * 使用模組: MADDY, MADDYPlus, NIGOQ
 * 尺寸: 21×21px
 */
struct SmallGrayKnob : BaseCustomKnob {
    SmallGrayKnob() {
        box.size = Vec(KnobSizes::SMALL_GRAY, KnobSizes::SMALL_GRAY);
        speed = KnobSensitivity::SLOW;
        baseColor = nvgRGB(30, 30, 30);
        centerColor = nvgRGB(180, 180, 180);
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 255, 255);
        indicatorMargin = 6;
    }

    void drawIndicator(const DrawArgs& args, float radius, float angle) override {
        float indicatorLength = radius - indicatorMargin;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, indicatorColor);
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 1.5f);
        nvgFillColor(args.vg, indicatorColor);
        nvgFill(args.vg);
    }
};

/**
 * HiddenTimeKnob - 隱藏的時間調整旋鈕
 * 使用模組: Observer, Obserfour, QQ, NIGOQ
 * 尺寸: 1×1px (隱藏)
 */
struct HiddenTimeKnob : BaseHiddenKnob {
    HiddenTimeKnob() {
        speed = KnobSensitivity::VERY_SLOW;
    }
};

/**
 * HiddenTimeKnobQQ - QQ 專用隱藏時間旋鈕
 * 尺寸: 60×51px (與 scope display 相同)
 */
struct HiddenTimeKnobQQ : BaseHiddenKnob {
    HiddenTimeKnobQQ() {
        box.size = Vec(60, 51);
        speed = 0.01f;  // 特殊靈敏度
    }

    void onEnter(const event::Enter& e) override {
        glfwSetCursor(APP->window->win, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
        BaseHiddenKnob::onEnter(e);
    }

    void onLeave(const event::Leave& e) override {
        glfwSetCursor(APP->window->win, NULL);
        BaseHiddenKnob::onLeave(e);
    }
};

/**
 * HiddenTimeKnobObserver - Observer/Obserfour 專用隱藏時間旋鈕
 * 尺寸: 120×300px (與 scope display 相同)
 */
struct HiddenTimeKnobObserver : BaseHiddenKnob {
    HiddenTimeKnobObserver() {
        box.size = Vec(120, 300);
        speed = KnobSensitivity::VERY_SLOW;
    }

    void onEnter(const event::Enter& e) override {
        glfwSetCursor(APP->window->win, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
        BaseHiddenKnob::onEnter(e);
    }

    void onLeave(const event::Leave& e) override {
        glfwSetCursor(APP->window->win, NULL);
        BaseHiddenKnob::onLeave(e);
    }
};

/**
 * HiddenTimeKnobNIGOQ - NIGOQ 專用隱藏時間旋鈕
 * 尺寸: 66×38.5px (與 scope display 相同)
 */
struct HiddenTimeKnobNIGOQ : BaseHiddenKnob {
    HiddenTimeKnobNIGOQ() {
        box.size = Vec(66, 38.5);
        speed = KnobSensitivity::VERY_SLOW;
    }

    void onEnter(const event::Enter& e) override {
        glfwSetCursor(APP->window->win, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
        BaseHiddenKnob::onEnter(e);
    }

    void onLeave(const event::Leave& e) override {
        glfwSetCursor(APP->window->win, NULL);
        BaseHiddenKnob::onLeave(e);
    }
};

/**
 * HiddenAttenuatorKnob - 隱藏的衰減旋鈕
 * 尺寸: 24×24px (PJ301MPort 大小)
 */
struct HiddenAttenuatorKnob : BaseHiddenKnob {
    HiddenAttenuatorKnob() {
        box.size = Vec(24, 24);
        speed = 0.005f;  // 特殊靈敏度
    }

    void onEnter(const event::Enter& e) override {
        glfwSetCursor(APP->window->win, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
        BaseHiddenKnob::onEnter(e);
    }

    void onLeave(const event::Leave& e) override {
        glfwSetCursor(APP->window->win, NULL);
        BaseHiddenKnob::onLeave(e);
    }
};

// ============================================================================
// Phase 2: Snap 旋鈕 (Snap Knobs)
// ============================================================================

/**
 * SnapKnob - 標準 Snap 旋鈕
 * 使用模組: EuclideanRhythm
 * 尺寸: 26×26px
 */
struct SnapKnob : BaseSnapKnob {
    SnapKnob() {
        box.size = Vec(KnobSizes::SNAP_STANDARD, KnobSizes::SNAP_STANDARD);
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

/**
 * TechnoSnapKnob30 - KIMO 專用的 Techno Snap 旋鈕
 * 使用模組: KIMO
 * 尺寸: 30×30px
 */
struct TechnoSnapKnob30 : BaseSnapKnob {
    TechnoSnapKnob30() {
        box.size = Vec(30, 30);
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
    }
};

// ============================================================================
// Phase 3: 特殊旋鈕 (Special Knobs)
// ============================================================================

/**
 * SmallWhiteKnob - Multiverse 專用小型旋鈕
 * 使用模組: Multiverse
 * 尺寸: 26×26px
 * 特色: 深灰外圈 + 白色內圈 + 粉紅色指示器
 */
struct SmallWhiteKnob : BaseCustomKnob {
    SmallWhiteKnob() {
        box.size = Vec(26, 26);
        speed = KnobSensitivity::SLOW;
        baseColor = nvgRGB(30, 30, 30);        // 深灰外圈
        centerColor = nvgRGB(255, 255, 255);   // 白色內圈
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 133, 133); // 粉紅色指示器
        indicatorMargin = 6;
    }
};

/**
 * LargeWhiteKnob - NIGOQ 專用大型旋鈕
 * 使用模組: NIGOQ
 * 尺寸: 37×37px
 * 特色: 深灰外圈 + 白色內圈 + 粉紅色指示器
 */
struct LargeWhiteKnob : BaseCustomKnob {
    LargeWhiteKnob() {
        box.size = Vec(37, 37);
        speed = KnobSensitivity::SLOW;
        baseColor = nvgRGB(30, 30, 30);        // 深灰外圈
        centerColor = nvgRGB(255, 255, 255);   // 白色內圈
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 133, 133); // 粉紅色指示器
        indicatorMargin = 8;
    }
};

/**
 * MicrotuneKnob - 微調旋鈕
 * 使用模組: Quantizer
 * 尺寸: 20×20px
 */
struct MicrotuneKnob : BaseCustomKnob {
    MicrotuneKnob() {
        box.size = Vec(KnobSizes::MICRO, KnobSizes::MICRO);
        speed = KnobSensitivity::VERY_SLOW;  // 非常精細的控制
        baseColor = KnobColors::BLACK_BASE;
        centerColor = KnobColors::BLACK_CENTER;
        borderColor = KnobColors::GRAY_BORDER;
        indicatorColor = KnobColors::WHITE_INDICATOR;
        indicatorMargin = 5.0f;  // 較小的指示器
    }
};

// ============================================================================
// Phase 4: 模組專屬旋鈕 (Module-Specific Knobs)
// ============================================================================

/**
 * MADDYSnapKnob - MADDY 模組專用 Snap 旋鈕
 */
struct MADDYSnapKnob : BaseSnapKnob {
    MADDYSnapKnob() {
        box.size = Vec(26, 26);
        baseColor = nvgRGB(30, 30, 30);
        centerColor = nvgRGB(130, 130, 130);
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 255, 255);
        indicatorMargin = 8;
    }
};

/**
 * WhiteKnob - 標準白色旋鈕
 * 使用模組: MADDY, MADDYPlus
 */
struct WhiteKnob : BaseCustomKnob {
    WhiteKnob() {
        box.size = Vec(30, 30);
        speed = KnobSensitivity::SLOW;
        baseColor = nvgRGB(30, 30, 30);
        centerColor = nvgRGB(255, 255, 255);
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 133, 133);
        indicatorMargin = 8;
    }
};

/**
 * MediumGrayKnob - 中型灰色旋鈕
 * 使用模組: MADDY, MADDYPlus, Multiverse
 */
struct MediumGrayKnob : BaseCustomKnob {
    MediumGrayKnob() {
        box.size = Vec(26, 26);
        speed = KnobSensitivity::SLOW;
        baseColor = nvgRGB(30, 30, 30);
        centerColor = nvgRGB(130, 130, 130);
        borderColor = nvgRGB(100, 100, 100);
        indicatorColor = nvgRGB(255, 255, 255);
        indicatorMargin = 8;
    }
};

// 其他特殊旋鈕

struct MADDYPlusSnapKnob : MADDYSnapKnob {};

} // namespace widgets
} // namespace madzine

// 為了相容性，將命名空間內的類別導出到全域
using namespace madzine::widgets;