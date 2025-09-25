#pragma once
#include <rack.hpp>

namespace madzine {
namespace widgets {

// 顏色定義
namespace KnobColors {
    const NVGcolor BLACK_BASE = nvgRGB(30, 30, 30);
    const NVGcolor BLACK_CENTER = nvgRGB(50, 50, 50);
    const NVGcolor GRAY_BORDER = nvgRGB(100, 100, 100);
    const NVGcolor WHITE_INDICATOR = nvgRGB(255, 255, 255);
    const NVGcolor WHITE_BASE = nvgRGB(255, 255, 255);
    const NVGcolor GRAY_BASE = nvgRGB(128, 128, 128);
    const NVGcolor PINK_BASE = nvgRGB(255, 192, 203);
    const NVGcolor TECHNO_GREEN = nvgRGB(0, 255, 100);
}

// 尺寸定義
namespace KnobSizes {
    // 標準尺寸
    const float STANDARD = 38.0f;      // StandardBlackKnob
    const float TECHNO_STANDARD = 45.0f; // TechnoStandardBlackKnob (U8: 45x45, KIMO: 30x30 需要特別處理)
    const float LARGE = 46.0f;         // LargeBlackKnob
    const float LARGE_WHITE = 37.0f;   // LargeWhiteKnob
    const float SMALL = 28.0f;         // SmallBlackKnob, SmallWhiteKnob
    const float SMALL_GRAY = 21.0f;    // SmallGrayKnob
    const float MICRO = 20.0f;         // MicrotuneKnob
    const float SNAP_STANDARD = 26.0f; // SnapKnob
    const float SMALL_TECHNO = 15.0f;  // SmallTechnoStandardBlackKnob (TWNC2)

    // 指示器長度 (相對於半徑)
    const float INDICATOR_MARGIN = 8.0f;
}

// 角度範圍
namespace KnobAngles {
    const float MIN_ANGLE = -0.75f * M_PI;
    const float MAX_ANGLE = 0.75f * M_PI;
}

// 靈敏度映射 (ParamWidget sensitivity -> app::Knob speed)
// 全部設為快速反應 (1.0f)
namespace KnobSensitivity {
    const float VERY_SLOW = 1.0f;      // 快速
    const float SLOW = 1.0f;           // 快速 (預設)
    const float NORMAL = 1.0f;         // 快速
    const float FAST = 1.0f;           // 快速
    const float VERY_FAST = 1.0f;      // 快速
}

// Y軸偏移定義 (根據 COMPONENT_Y_OFFSET_REFERENCE.txt)
namespace LabelOffsets {
    const float LARGE_BLACK_KNOB = -25.0f;    // 46×46px
    const float LARGE_WHITE_KNOB = -29.0f;    // 37×37px
    const float STANDARD_BLACK_KNOB = -23.0f; // 38×38px
    const float SMALL_WHITE_KNOB = -26.0f;    // 28×28px
    const float SMALL_BLACK_KNOB = -15.0f;    // 28×28px
    const float SMALL_GRAY_KNOB = -15.0f;     // 21×21px
    const float MICROTUNE_KNOB = -15.0f;      // 20×20px
}

// Snap 旋鈕參數
namespace SnapKnobParams {
    const float DEFAULT_THRESHOLD = 10.0f;
    const float FINE_THRESHOLD = 5.0f;
    const float COARSE_THRESHOLD = 20.0f;
}

} // namespace widgets
} // namespace madzine