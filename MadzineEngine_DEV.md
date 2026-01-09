# MadzineEngine 開發指南

版本：0.1.0（規劃階段）
建立日期：2026-01-02

---

## 新對話啟動指令

在新的 Claude Code 對話中，複製貼上以下內容開始開發：

```
讀取 /Users/madzine/Documents/VCV-Dev/MADZINE/MadzineEngine_DEV.md
在 /Users/madzine/Documents/VCV-Dev/ 建立 madzine-filter-engine 專案
從階段 1 開始實作

核心創新重點：
1. MADZINE Window - 原創窗函數（非 Kaiser/Blackman）
2. Hybrid FIR+IIR - 可調相位混合架構
3. Character Presets - Clean/Warm/Punchy/Vintage 四種風格
4. 靜態庫封裝 - 商業保護
```

---

## 相關檔案路徑

| 檔案 | 路徑 |
|------|------|
| 本開發指南 | `/Users/madzine/Documents/VCV-Dev/MADZINE/MadzineEngine_DEV.md` |
| 計劃檔案 | `/Users/madzine/.claude/plans/resilient-floating-reddy.md` |
| 詳細研究報告 | `/Users/madzine/.claude/plans/resilient-floating-reddy-agent-a7de7a6.md` |
| 目標專案位置 | `/Users/madzine/Documents/VCV-Dev/madzine-filter-engine/` |
| VCV 整合測試 | `/Users/madzine/Documents/VCV-Dev/MADZINE/src/Pinpple.cpp` |

---

## 專案目標

建立**原創**的混合 FIR + IIR 濾波器引擎，提供：
1. 高品質 Oversampling 抗混疊
2. MADZINE 獨有的聲音特色
3. 靜態庫封裝的商業保護

---

## 核心創新技術

### 1. MADZINE Window Function（原創窗函數）

不使用標準 Kaiser/Blackman 窗，設計參數化的「MADZINE 窗」：

```cpp
// 原創窗函數 - 結合非對稱設計和 character 參數
float madzine_window(int n, int N, float character) {
    float t = (float)n / (N - 1);

    // 基底：改良 Hanning
    float base = 0.5f - 0.5f * cosf(2.f * M_PI * t);

    // 創新 1：可調 character（0=乾淨, 1=溫暖）
    float mod = powf(sinf(M_PI * t), 1.0f + character);

    // 創新 2：非對稱設計（類比風格）
    float asym = 1.0f + 0.1f * character * (t - 0.5f);

    return base * mod * asym;
}
```

### 2. Hybrid FIR+IIR 架構（混合相位設計）

```
      +--[Linear Phase FIR]--+
      |                      |
In -->+                      +--> Crossfade --> Out
      |                      |
      +--[Min Phase IIR]-----+

Character: Clean (100% FIR) <--> Warm (100% IIR)
```

### 3. 聲音個性預設

| 預設 | 技術實作 |
|------|----------|
| **Clean** | 100% 線性相位 FIR，透明度最高 |
| **Warm** | 混合 70% IIR，帶有類比溫暖感 |
| **Punchy** | 最小相位 IIR，保留瞬態 |
| **Vintage** | 加入微量通帶漣波 (Chebyshev 風格) |

---

## 專案結構

```
madzine-filter-engine/          # 獨立專案（編譯為靜態庫）
├── CMakeLists.txt              # 跨平台建置
├── include/madzine/
│   ├── filter_engine.hpp       # 公開 API（只有這個給外部）
│   └── types.hpp               # 公開型別
├── src/                        # 核心實作（編譯後不公開）
│   ├── fir_polyphase.cpp       # FIR 多相濾波器
│   ├── iir_biquad.cpp          # IIR 雙二階級聯
│   ├── hybrid_processor.cpp    # 混合架構
│   ├── madzine_window.cpp      # 原創窗函數
│   ├── coefficients.cpp        # 預計算係數
│   └── simd/
│       ├── sse_impl.cpp        # Intel x64
│       ├── neon_impl.cpp       # Apple Silicon
│       └── scalar_impl.cpp     # 備用
├── tools/
│   └── coeff_generator.py      # Python 係數產生器
└── tests/
    ├── test_fir.cpp
    ├── test_iir.cpp
    └── test_hybrid.cpp
```

建議路徑：`/Users/madzine/Documents/VCV-Dev/madzine-filter-engine/`

---

## 公開 API 設計

```cpp
// include/madzine/filter_engine.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <functional>

namespace madzine {

enum class Character {
    Clean,      // 100% 線性相位 FIR
    Warm,       // 混合 70% IIR
    Punchy,     // 最小相位 IIR
    Vintage     // 帶通帶漣波
};

enum class OversampleRate {
    X2 = 2,
    X4 = 4,
    X8 = 8
};

class OversampleEngine {
public:
    explicit OversampleEngine(OversampleRate rate = OversampleRate::X2,
                              Character ch = Character::Clean);
    ~OversampleEngine();

    // 移動語義支援
    OversampleEngine(OversampleEngine&&) noexcept;
    OversampleEngine& operator=(OversampleEngine&&) noexcept;

    // 禁止複製
    OversampleEngine(const OversampleEngine&) = delete;
    OversampleEngine& operator=(const OversampleEngine&) = delete;

    // === 處理 API ===

    // 上取樣：1 個輸入 -> rate 個輸出
    void upsample(const float* input, float* output, size_t input_frames);

    // 下取樣：rate 個輸入 -> 1 個輸出
    void downsample(const float* input, float* output, size_t output_frames);

    // Callback 風格處理（方便整合）
    // dsp_callback 會在過取樣率下被呼叫
    template<typename F>
    void process(const float* input, float* output, size_t frames, F&& dsp_callback) {
        // 範例實作：
        // 1. upsample(input, upsampled_buffer, frames)
        // 2. dsp_callback(upsampled_buffer, processed_buffer, frames * rate)
        // 3. downsample(processed_buffer, output, frames)
    }

    // === 設定 ===
    void set_character(Character ch);
    void set_rate(OversampleRate rate);
    void reset();  // 清除濾波器狀態

    // === 資訊 ===
    size_t get_latency() const;        // 取樣點延遲
    OversampleRate get_rate() const;
    Character get_character() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace madzine
```

---

## 數學基礎

### FIR 係數計算（sinc 濾波器）

```python
from math import sin, cos, pi, pow

def sinc_lowpass(n, N, fc):
    """產生低通 FIR 係數（不依賴外部庫）"""
    x = n - (N - 1) / 2
    if abs(x) < 1e-10:
        return 2 * fc
    return sin(2 * pi * fc * x) / (pi * x)

def madzine_window(n, N, character=0.5):
    """MADZINE 原創窗函數"""
    t = n / (N - 1)
    base = 0.5 - 0.5 * cos(2 * pi * t)
    mod = pow(sin(pi * t + 1e-10), 1.0 + character)
    asym = 1.0 + 0.1 * character * (t - 0.5)
    return base * mod * asym

def generate_fir_coeffs(taps, cutoff, character=0.5):
    """產生完整 FIR 係數"""
    coeffs = []
    for n in range(taps):
        h = sinc_lowpass(n, taps, cutoff)
        w = madzine_window(n, taps, character)
        coeffs.append(h * w)

    # 正規化
    total = sum(coeffs)
    coeffs = [c / total for c in coeffs]

    return coeffs
```

### IIR Biquad 係數計算（Audio EQ Cookbook）

```python
from math import sin, cos, pi

def biquad_lowpass(fc, fs, Q=0.7071):
    """產生雙二階低通濾波器係數"""
    w0 = 2 * pi * fc / fs
    cos_w0 = cos(w0)
    sin_w0 = sin(w0)
    alpha = sin_w0 / (2 * Q)

    b0 = (1 - cos_w0) / 2
    b1 = 1 - cos_w0
    b2 = (1 - cos_w0) / 2
    a0 = 1 + alpha
    a1 = -2 * cos_w0
    a2 = 1 - alpha

    # 正規化
    return {
        'b0': b0 / a0,
        'b1': b1 / a0,
        'b2': b2 / a0,
        'a1': a1 / a0,
        'a2': a2 / a0
    }
```

### Butterworth 多階級聯

```python
from cmath import exp, pi as cpi

def butterworth_poles(order):
    """計算 Butterworth 濾波器極點"""
    poles = []
    for k in range(order):
        theta = cpi * (2 * k + order + 1) / (2 * order)
        poles.append(exp(1j * theta))
    return poles

# 4 階 Butterworth = 2 個 biquad 級聯
# 極點對：(p0, p0*), (p1, p1*)
```

---

## 實作步驟

### 階段 1：基礎建設
- [ ] 建立 CMake 專案結構
- [ ] 實作基本 FIR polyphase 上取樣器
- [ ] 實作基本 IIR biquad 級聯濾波器
- [ ] 建立單元測試框架

### 階段 2：原創創新
- [ ] 實作 MADZINE Window 函數
- [ ] 實作 Hybrid 混合相位架構
- [ ] 設計 4 種聲音個性預設係數
- [ ] SIMD 最佳化 (SSE/NEON)

### 階段 3：VCV 整合
- [ ] 建立靜態庫建置流程
- [ ] 整合到 Pinpple oscillator 測試
- [ ] 跨平台驗證 (macOS Intel/ARM, Windows, Linux)
- [ ] CPU 效能測試

---

## SIMD 抽象層

```cpp
// include/madzine/simd_utils.hpp
#pragma once

#if defined(__SSE4_2__)
    #include <immintrin.h>
    #define MADZINE_USE_SSE
    using simd_float4 = __m128;
#elif defined(__ARM_NEON)
    #include <arm_neon.h>
    #define MADZINE_USE_NEON
    using simd_float4 = float32x4_t;
#else
    #define MADZINE_USE_SCALAR
    struct simd_float4 { float v[4]; };
#endif

namespace madzine {
namespace simd {

inline simd_float4 load(const float* ptr);
inline void store(float* ptr, simd_float4 v);
inline simd_float4 mul(simd_float4 a, simd_float4 b);
inline simd_float4 add(simd_float4 a, simd_float4 b);
inline simd_float4 set1(float v);
inline float horizontal_sum(simd_float4 v);

} // namespace simd
} // namespace madzine
```

---

## CMakeLists.txt 範本

```cmake
cmake_minimum_required(VERSION 3.16)
project(MadzineFilterEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 靜態庫
add_library(madzine_filter STATIC
    src/fir_polyphase.cpp
    src/iir_biquad.cpp
    src/hybrid_processor.cpp
    src/madzine_window.cpp
    src/coefficients.cpp
)

# SIMD 選擇
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|ARM64")
    target_sources(madzine_filter PRIVATE src/simd/neon_impl.cpp)
    target_compile_definitions(madzine_filter PRIVATE MADZINE_USE_NEON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_sources(madzine_filter PRIVATE src/simd/sse_impl.cpp)
    target_compile_definitions(madzine_filter PRIVATE MADZINE_USE_SSE)
    if(NOT MSVC)
        target_compile_options(madzine_filter PRIVATE -msse4.2)
    endif()
else()
    target_sources(madzine_filter PRIVATE src/simd/scalar_impl.cpp)
endif()

target_include_directories(madzine_filter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# macOS Universal Binary
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")
endif()

# 測試
enable_testing()
add_subdirectory(tests)
```

---

## VCV Rack 整合範例

```cpp
// 在 VCV 模組中使用 MadzineEngine
#include "madzine/filter_engine.hpp"

struct MyOscillator : Module {
    madzine::OversampleEngine oversampler{
        madzine::OversampleRate::X4,
        madzine::Character::Warm
    };

    void process(const ProcessArgs& args) override {
        float input = /* ... */;

        float output = 0.f;
        oversampler.process(&input, &output, 1, [this](const float* in, float* out, size_t n) {
            // 這裡的 DSP 會以 4x 取樣率執行
            for (size_t i = 0; i < n; i++) {
                out[i] = waveshaper(in[i]);  // 非線性處理
            }
        });

        outputs[OUT].setVoltage(output);
    }
};
```

---

## 原創性說明

本引擎的創新點：

| 項目 | 說明 |
|------|------|
| **MADZINE Window** | 原創窗函數，非標準 Kaiser/Blackman |
| **Hybrid Phase** | 可調線性/最小相位混合架構 |
| **Character Presets** | MADZINE 獨有聲音風格 |
| **Asymmetric Design** | 類比設備特有的非對稱響應 |

數學基礎使用公開的 DSP 理論（sinc 函數、biquad 公式、Butterworth 設計），但具體實作和聲音設計為原創。

---

## 技術參考

- [Audio EQ Cookbook](https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html) - Biquad 公式
- [VHDLwhiz Polyphase Filters](https://vhdlwhiz.com/part-5-polyphase-fir-filters/) - 多相分解
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) - 抗混疊指南
- [Cascaded Biquad Design](https://www.dsprelated.com/showarticle/1137.php) - IIR 級聯

---

## 聯絡

GitHub: https://github.com/mmmmmmmadman/MADZINE-VCV
Email: madzinetw@gmail.com
