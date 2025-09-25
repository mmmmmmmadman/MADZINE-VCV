#include "plugin.hpp"
#include <cmath>
#include <vector>
#include <cstring>

#ifdef __APPLE__
// External window functions for macOS
extern "C" {
    void* createMultiverseWindow();
    void destroyMultiverseWindow(void* window);
    void openMultiverseWindow(void* window);
    void closeMultiverseWindow(void* window);
    bool isMultiverseWindowOpen(void* window);
    void updateMultiverseChannel(void* window, int channel, const float* buffer, int size);
    void updateMultiverseChannelParams(void* window, int channel,
                                      float phase, float ratio, float angle,
                                      float intensity, float frequency);
    void updateMultiverseGlobalParams(void* window, float mixMode, float crossMod);
}
#endif

struct MixModeParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int mode = (int)std::round(getValue());
        mode = clamp(mode, 0, 3);

        const char* modeNames[] = {
            "Add", "Screen", "Difference", "Color Dodge"
        };

        return modeNames[mode];
    }
};

struct Multiverse : Module {
    enum ParamIds {
        PHASE_PARAM_1,
        RATIO_PARAM_1,
        ANGLE_PARAM_1,
        INTENSITY_PARAM_1,
        PHASE_PARAM_2,
        RATIO_PARAM_2,
        ANGLE_PARAM_2,
        INTENSITY_PARAM_2,
        PHASE_PARAM_3,
        RATIO_PARAM_3,
        ANGLE_PARAM_3,
        INTENSITY_PARAM_3,
        PHASE_PARAM_4,
        RATIO_PARAM_4,
        ANGLE_PARAM_4,
        INTENSITY_PARAM_4,
        FREEZE_PARAM,
        MIX_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        AUDIO_INPUT_1,
        AUDIO_INPUT_2,
        AUDIO_INPUT_3,
        AUDIO_INPUT_4,
        PHASE_CV_1,
        RATIO_CV_1,
        ANGLE_CV_1,
        INTENSITY_CV_1,
        PHASE_CV_2,
        RATIO_CV_2,
        ANGLE_CV_2,
        INTENSITY_CV_2,
        PHASE_CV_3,
        RATIO_CV_3,
        ANGLE_CV_3,
        INTENSITY_CV_3,
        PHASE_CV_4,
        RATIO_CV_4,
        ANGLE_CV_4,
        INTENSITY_CV_4,
        TRIGGER_INPUT,
        MIX_CV,
        NUM_INPUTS
    };

    enum OutputIds {
        NUM_OUTPUTS
    };

    enum LightIds {
        FREEZE_LIGHT,
        NUM_LIGHTS
    };

    // Display dimensions
    static const int DISPLAY_WIDTH = 1024;
    static const int DISPLAY_HEIGHT = 512;

    // Per-channel buffers
    struct Channel {
        float displayBuffer[DISPLAY_WIDTH];
        int bufferIndex = 0;
        int frameIndex = 0;
        float dominantFrequency = 440.0f;
        float lastVoltage = 0.0f;
        int zeroCrossings = 0;
        int sampleCount = 0;
    };

    Channel channels[4];

    // Trigger system
    dsp::SchmittTrigger signalTrigger[4];
    dsp::SchmittTrigger externalTrigger;
    bool triggerEnabled = false;
    dsp::SchmittTrigger freezeTrigger;
    bool freezeBuffer[4] = {false, false, false, false};

    // External window handle
#ifdef __APPLE__
    void* externalWindow = nullptr;
#endif

    Multiverse() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Initialize channels
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < DISPLAY_WIDTH; j++) {
                channels[i].displayBuffer[j] = 0.0f;
            }
        }

        for (int i = 0; i < 4; i++) {
            configParam(PHASE_PARAM_1 + i * 4, 0.f, 360.f, 0.f, "Phase " + std::to_string(i + 1), "°");
            configParam(RATIO_PARAM_1 + i * 4, 0.f, 1.f, 0.5f, "Ratio " + std::to_string(i + 1));
            configParam(ANGLE_PARAM_1 + i * 4, 0.f, 1.f, 0.5f, "Angle " + std::to_string(i + 1));
            configParam(INTENSITY_PARAM_1 + i * 4, 0.f, 1.f, 0.5f, "Intensity " + std::to_string(i + 1));

            configInput(AUDIO_INPUT_1 + i, "Audio " + std::to_string(i + 1));
            configInput(PHASE_CV_1 + i * 4, "Phase CV " + std::to_string(i + 1));
            configInput(RATIO_CV_1 + i * 4, "Ratio CV " + std::to_string(i + 1));
            configInput(ANGLE_CV_1 + i * 4, "Angle CV " + std::to_string(i + 1));
            configInput(INTENSITY_CV_1 + i * 4, "Intensity CV " + std::to_string(i + 1));
        }

        configButton(FREEZE_PARAM, "Trigger");
        configParam<MixModeParamQuantity>(MIX_PARAM, 0.f, 3.f, 0.f, "Mix Mode", "");

        configInput(TRIGGER_INPUT, "External Trigger");
        configInput(MIX_CV, "Mix CV");

        configLight(FREEZE_LIGHT, "Trigger");

#ifdef __APPLE__
        // Create external window
        externalWindow = createMultiverseWindow();
#endif
    }

    ~Multiverse() {
#ifdef __APPLE__
        if (externalWindow) {
            destroyMultiverseWindow(externalWindow);
            externalWindow = nullptr;
        }
#endif
    }

    void process(const ProcessArgs& args) override {
        // Update trigger state
        if (freezeTrigger.process(params[FREEZE_PARAM].getValue())) {
            triggerEnabled = !triggerEnabled;
        }
        lights[FREEZE_LIGHT].setBrightness(triggerEnabled ? 1.0f : 0.0f);

        // Process each channel
        for (int ch = 0; ch < 4; ch++) {
            if (!inputs[AUDIO_INPUT_1 + ch].isConnected()) continue;

            float voltage = inputs[AUDIO_INPUT_1 + ch].getVoltage();
            Channel& channel = channels[ch];

            // Check trigger if enabled
            if (triggerEnabled && !freezeBuffer[ch]) {
                bool triggered = false;

                // Check external trigger first
                if (inputs[TRIGGER_INPUT].isConnected()) {
                    if (externalTrigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
                        triggered = true;
                    }
                } else {
                    // Use signal input for trigger (threshold at 0V)
                    if (signalTrigger[ch].process(rescale(voltage, 0.0f, 0.01f, 0.0f, 1.0f))) {
                        triggered = true;
                    }
                }

                if (triggered) {
                    freezeBuffer[ch] = false;
                    channel.bufferIndex = 0;
                }
            }

            // Frequency detection for color mapping
            if ((channel.lastVoltage < 0 && voltage >= 0) || (channel.lastVoltage >= 0 && voltage < 0)) {
                channel.zeroCrossings++;
            }
            channel.lastVoltage = voltage;
            channel.sampleCount++;

            if (channel.sampleCount >= 512) {
                float newFreq = (channel.zeroCrossings / 2.0f) * (args.sampleRate / 512.0f);
                if (newFreq > 20.0f && newFreq < 20000.0f) {
                    channel.dominantFrequency = newFreq;
                }
                channel.zeroCrossings = 0;
                channel.sampleCount = 0;
            }

            // Get parameters with CV
            float ratio = params[RATIO_PARAM_1 + ch * 4].getValue();
            ratio = (ratio - 0.5f) * 4.0f; // Map 0-1 to -2 to 2
            if (inputs[RATIO_CV_1 + ch * 4].isConnected()) {
                ratio += inputs[RATIO_CV_1 + ch * 4].getVoltage() * 0.4f;
                ratio = clamp(ratio, -2.f, 2.f);
            }

            // Calculate samples per pixel based on time ratio
            float msPerScreen = std::pow(10.0f, ratio) * 10.0f;
            float samplesPerScreen = args.sampleRate * msPerScreen / 1000.0f;
            float samplesPerPixel = samplesPerScreen / DISPLAY_WIDTH;

            // Update buffer
            channel.frameIndex++;
            if (channel.frameIndex >= (int)samplesPerPixel) {
                if (channel.bufferIndex >= DISPLAY_WIDTH) {
                    channel.bufferIndex = 0;
                }
                channel.displayBuffer[channel.bufferIndex] = voltage;
                channel.bufferIndex++;
                channel.frameIndex = 0;
            }
        }

#ifdef __APPLE__
        // Update external window periodically
        static int updateCounter = 0;
        updateCounter++;
        if (externalWindow && updateCounter % 512 == 0) {
            // Get global mix mode
            float mixMode = params[MIX_PARAM].getValue();
            if (inputs[MIX_CV].isConnected()) {
                mixMode += inputs[MIX_CV].getVoltage() * 0.4f;
                mixMode = clamp(mixMode, 0.f, 3.f);
            }

            // Update each channel in external window
            for (int ch = 0; ch < 4; ch++) {
                // Get parameters with CV
                float phase = params[PHASE_PARAM_1 + ch * 4].getValue();
                if (inputs[PHASE_CV_1 + ch * 4].isConnected()) {
                    phase = std::fmod(phase + inputs[PHASE_CV_1 + ch * 4].getVoltage() * 36.f, 360.f);
                }

                float angle = params[ANGLE_PARAM_1 + ch * 4].getValue();
                angle = (angle - 0.5f) * 360.0f;
                if (inputs[ANGLE_CV_1 + ch * 4].isConnected()) {
                    angle += inputs[ANGLE_CV_1 + ch * 4].getVoltage() * 18.0f;
                    angle = clamp(angle, -180.f, 180.f);
                }

                float intensity = params[INTENSITY_PARAM_1 + ch * 4].getValue();
                intensity = intensity * 2.0f;
                if (inputs[INTENSITY_CV_1 + ch * 4].isConnected()) {
                    intensity += inputs[INTENSITY_CV_1 + ch * 4].getVoltage() * 0.2f;
                    intensity = clamp(intensity, 0.0f, 2.0f);
                }

                // Send data to external window
                updateMultiverseChannel(externalWindow, ch, channels[ch].displayBuffer, DISPLAY_WIDTH);
                updateMultiverseChannelParams(externalWindow, ch,
                                             phase / 360.0f, 0.0f, angle / 360.0f,
                                             intensity, channels[ch].dominantFrequency);
            }

            updateMultiverseGlobalParams(externalWindow, mixMode, 0.0f);
        }
#endif
    }

    // Octave-based frequency to hue mapping
    float getHueFromFrequency(float freq) {
        // Each octave cycles through full spectrum
        freq = clamp(freq, 20.0f, 20000.0f);
        const float baseFreq = 55.0f; // A1
        float octavePosition = std::fmod(std::log2(freq / baseFreq), 1.0f);
        if (octavePosition < 0) {
            octavePosition += 1.0f;
        }
        return octavePosition * 360.0f;
    }

    NVGcolor blendColors(NVGcolor c1, NVGcolor c2, float mixMode, float factor) {
        int mode = (int)std::round(mixMode);
        mode = clamp(mode, 0, 3);

        float r1 = c1.r, g1 = c1.g, b1 = c1.b, a1 = c1.a;
        float r2 = c2.r, g2 = c2.g, b2 = c2.b, a2 = c2.a;
        float r, g, b, a;

        switch (mode) {
            case 0: // Add
                r = std::min(1.0f, r1 + r2);
                g = std::min(1.0f, g1 + g2);
                b = std::min(1.0f, b1 + b2);
                a = std::min(1.0f, a1 + a2);
                break;
            case 1: // Screen
                r = 1 - (1 - r1) * (1 - r2);
                g = 1 - (1 - g1) * (1 - g2);
                b = 1 - (1 - b1) * (1 - b2);
                a = 1 - (1 - a1) * (1 - a2);
                break;
            case 2: // Difference
                r = std::abs(r1 - r2);
                g = std::abs(g1 - g2);
                b = std::abs(b1 - b2);
                a = std::max(a1, a2);
                break;
            case 3: // Color Dodge
                r = (r2 < 0.999f) ? std::min(1.0f, r1 / std::max(0.001f, 1.0f - r2)) : 1.0f;
                g = (g2 < 0.999f) ? std::min(1.0f, g1 / std::max(0.001f, 1.0f - g2)) : 1.0f;
                b = (b2 < 0.999f) ? std::min(1.0f, b1 / std::max(0.001f, 1.0f - b2)) : 1.0f;
                a = std::max(a1, a2);
                break;
            default:
                r = r1; g = g1; b = b1; a = a1;
                break;
        }

        return nvgRGBAf(r, g, b, a);
    }
};

struct MultiverseDisplay : Widget {
    Multiverse* module = nullptr;

    // Framebuffer for smooth rendering
    int imageHandle = -1;
    std::vector<unsigned char> pixelData;

    MultiverseDisplay() {
        box.size = Vec(400, 380);
        size_t pixelCount = (size_t)Multiverse::DISPLAY_WIDTH * (size_t)Multiverse::DISPLAY_HEIGHT * 4;
        pixelData.resize(pixelCount);
    }

    ~MultiverseDisplay() {
        if (imageHandle >= 0 && APP && APP->window && APP->window->vg) {
            nvgDeleteImage(APP->window->vg, imageHandle);
        }
    }

    void draw(const DrawArgs &args) override {
        // Draw background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);

        if (!module) {
            // Draw border
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgStrokeColor(args.vg, nvgRGBA(60, 60, 60, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
            return;
        }

        // Clear pixel data
        std::fill(pixelData.begin(), pixelData.end(), 0);

        // Get global parameters
        float mixMode = module->params[Multiverse::MIX_PARAM].getValue();
        if (module->inputs[Multiverse::MIX_CV].isConnected()) {
            mixMode += module->inputs[Multiverse::MIX_CV].getVoltage() * 0.4f;
            mixMode = clamp(mixMode, 0.f, 3.f);
        }

        // Process each layer
        for (int layer = 0; layer < 4; layer++) {
            if (!module->inputs[Multiverse::AUDIO_INPUT_1 + layer].isConnected()) continue;

            // Get parameters
            float phase = module->params[Multiverse::PHASE_PARAM_1 + layer * 4].getValue();
            if (module->inputs[Multiverse::PHASE_CV_1 + layer * 4].isConnected()) {
                phase = std::fmod(phase + module->inputs[Multiverse::PHASE_CV_1 + layer * 4].getVoltage() * 36.f, 360.f);
            }

            float angle = module->params[Multiverse::ANGLE_PARAM_1 + layer * 4].getValue();
            angle = (angle - 0.5f) * 360.0f;
            if (module->inputs[Multiverse::ANGLE_CV_1 + layer * 4].isConnected()) {
                angle += module->inputs[Multiverse::ANGLE_CV_1 + layer * 4].getVoltage() * 18.0f;
                angle = clamp(angle, -180.f, 180.f);
            }

            float intensity = module->params[Multiverse::INTENSITY_PARAM_1 + layer * 4].getValue();
            intensity = intensity * 2.0f;
            if (module->inputs[Multiverse::INTENSITY_CV_1 + layer * 4].isConnected()) {
                intensity += module->inputs[Multiverse::INTENSITY_CV_1 + layer * 4].getVoltage() * 0.2f;
                intensity = clamp(intensity, 0.0f, 2.0f);
            }

            // Get frequency for color
            float freq = module->channels[layer].dominantFrequency;
            float hue = module->getHueFromFrequency(freq);

            // Convert HSV to RGB
            float c = 1.0f;
            float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2) - 1));
            float r, g, b;
            if (hue < 60) {
                r = c; g = x; b = 0;
            } else if (hue < 120) {
                r = x; g = c; b = 0;
            } else if (hue < 180) {
                r = 0; g = c; b = x;
            } else if (hue < 240) {
                r = 0; g = x; b = c;
            } else if (hue < 300) {
                r = x; g = 0; b = c;
            } else {
                r = c; g = 0; b = x;
            }

            // Create layer buffer
            std::vector<float> layerBuffer(Multiverse::DISPLAY_WIDTH * Multiverse::DISPLAY_HEIGHT, 0.0f);

            // Fill layer buffer with phase shift
            float phaseOffset = (phase / 360.0f) * Multiverse::DISPLAY_WIDTH;
            for (int x = 0; x < Multiverse::DISPLAY_WIDTH; x++) {
                int srcX = (int)(x + phaseOffset) % Multiverse::DISPLAY_WIDTH;
                if (srcX < 0) srcX += Multiverse::DISPLAY_WIDTH;

                float voltage = module->channels[layer].displayBuffer[srcX];
                float normalizedVoltage = clamp((voltage + 10.0f) * 0.05f * intensity, 0.0f, 1.0f);

                // Fill column
                for (int y = 0; y < Multiverse::DISPLAY_HEIGHT; y++) {
                    size_t idx = (size_t)y * Multiverse::DISPLAY_WIDTH + (size_t)x;
                    layerBuffer[idx] = normalizedVoltage;
                }
            }

            // Apply rotation if needed
            if (std::abs(angle) > 0.01f) {
                float angleRad = angle * M_PI / 180.0f;
                float cosA = std::cos(angleRad);
                float sinA = std::sin(angleRad);

                float w = Multiverse::DISPLAY_WIDTH;
                float h = Multiverse::DISPLAY_HEIGHT;
                float absCosA = std::abs(cosA);
                float absSinA = std::abs(sinA);
                float scaleX = (w * absCosA + h * absSinA) / w;
                float scaleY = (w * absSinA + h * absCosA) / h;
                float scale = std::max(scaleX, scaleY);

                int centerX = Multiverse::DISPLAY_WIDTH / 2;
                int centerY = Multiverse::DISPLAY_HEIGHT / 2;

                for (int y = 0; y < Multiverse::DISPLAY_HEIGHT; y++) {
                    for (int x = 0; x < Multiverse::DISPLAY_WIDTH; x++) {
                        float dx = (x - centerX) / scale;
                        float dy = (y - centerY) / scale;
                        int srcX = (int)(centerX + dx * cosA + dy * sinA);
                        int srcY = (int)(centerY - dx * sinA + dy * cosA);

                        if (srcX >= 0 && srcX < Multiverse::DISPLAY_WIDTH &&
                            srcY >= 0 && srcY < Multiverse::DISPLAY_HEIGHT) {
                            size_t srcIdx = (size_t)srcY * Multiverse::DISPLAY_WIDTH + (size_t)srcX;
                            if (layerBuffer[srcIdx] > 0.0f) {
                                size_t dstIdx = ((size_t)y * Multiverse::DISPLAY_WIDTH + (size_t)x) * 4;

                                NVGcolor existing = nvgRGBA(pixelData[dstIdx], pixelData[dstIdx + 1],
                                                           pixelData[dstIdx + 2], pixelData[dstIdx + 3] / 255.0f);
                                NVGcolor newColor = nvgRGBAf(r * layerBuffer[srcIdx], g * layerBuffer[srcIdx],
                                                            b * layerBuffer[srcIdx], layerBuffer[srcIdx]);
                                NVGcolor blended = module->blendColors(existing, newColor, mixMode, 1.0f);

                                pixelData[dstIdx + 0] = (unsigned char)(blended.r * 255);
                                pixelData[dstIdx + 1] = (unsigned char)(blended.g * 255);
                                pixelData[dstIdx + 2] = (unsigned char)(blended.b * 255);
                                pixelData[dstIdx + 3] = 255;
                            }
                        }
                    }
                }
            } else {
                // No rotation - direct copy
                for (size_t i = 0; i < layerBuffer.size(); i++) {
                    if (layerBuffer[i] > 0.0f) {
                        size_t idx = i * 4;

                        NVGcolor existing = nvgRGBA(pixelData[idx], pixelData[idx + 1],
                                                   pixelData[idx + 2], pixelData[idx + 3] / 255.0f);
                        NVGcolor newColor = nvgRGBAf(r * layerBuffer[i], g * layerBuffer[i],
                                                    b * layerBuffer[i], layerBuffer[i]);
                        NVGcolor blended = module->blendColors(existing, newColor, mixMode, 1.0f);

                        pixelData[idx + 0] = (unsigned char)(blended.r * 255);
                        pixelData[idx + 1] = (unsigned char)(blended.g * 255);
                        pixelData[idx + 2] = (unsigned char)(blended.b * 255);
                        pixelData[idx + 3] = 255;
                    }
                }
            }
        }

        // Create or update NanoVG image
        if (imageHandle >= 0) {
            nvgUpdateImage(args.vg, imageHandle, pixelData.data());
        } else {
            imageHandle = nvgCreateImageRGBA(args.vg, Multiverse::DISPLAY_WIDTH, Multiverse::DISPLAY_HEIGHT,
                                            0, pixelData.data());
        }

        // Draw the image scaled to widget size
        if (imageHandle >= 0) {
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            float scaleX = box.size.x / Multiverse::DISPLAY_WIDTH;
            float scaleY = box.size.y / Multiverse::DISPLAY_HEIGHT;
            NVGpaint paint = nvgImagePattern(args.vg, 0, 0,
                                            box.size.x / scaleX, box.size.y / scaleY,
                                            0, imageHandle, 1.0f);
            nvgFillPaint(args.vg, paint);
            nvgFill(args.vg);
        }

        // Draw border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGBA(60, 60, 60, 255));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
    }
};

// White knob
struct SmallWhiteKnob : ParamWidget {
    bool isDragging = false;

    SmallWhiteKnob() {
        box.size = Vec(26, 26);
    }

    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        float normalizedValue = pq->getScaledValue();
        return rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
    }

    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);

        float indicatorLength = radius - 6;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 133, 133));
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 1.5f);
        nvgFillColor(args.vg, nvgRGB(255, 133, 133));
        nvgFill(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = true;
            e.consume(this);
        }
        else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = false;
        }
        ParamWidget::onButton(e);
    }

    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!isDragging || !pq) return;

        float sensitivity = 0.004f;
        float deltaY = -e.mouseDelta.y;
        float range = pq->getMaxValue() - pq->getMinValue();
        float currentValue = pq->getValue();
        float newValue = clamp(currentValue + deltaY * sensitivity * range, pq->getMinValue(), pq->getMaxValue());
        pq->setValue(newValue);
    }

    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        pq->reset();
        e.consume(this);
    }
};

// Pink knob
struct SmallPinkKnob : SmallWhiteKnob {
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, nvgRGB(255, 133, 133));
        nvgFill(args.vg);

        float indicatorLength = radius - 6;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
    }
};

struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;

    EnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
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
        nvgText(args.vg, box.size.x/2, box.size.y/2, text.c_str(), NULL);
    }
};

struct MultiverseWidget : ModuleWidget {
    Multiverse* multiverseModule = nullptr;

    MultiverseWidget(Multiverse* module) {
        multiverseModule = module;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/40HP.svg")));

        box.size = Vec(40 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(180, 20), "MULTIVERSE", 14.f, nvgRGB(255, 200, 0)));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(180, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0)));

        MultiverseDisplay* display = new MultiverseDisplay();
        display->module = module;
        display->box.pos = Vec(200, 0);
        addChild(display);

        float audioInputX = 25.0f;
        float knobStartX = 55.0f;
        float cvStartX = 125.0f;
        float inputSpacing = 71.0f;

        for (int i = 0; i < 4; i++) {
            float yPos = 88 + i * inputSpacing;

            addInput(createInputCentered<PJ301MPort>(Vec(audioInputX, yPos), module, Multiverse::AUDIO_INPUT_1 + i));
            addChild(new EnhancedTextLabel(Vec(audioInputX - 15, yPos - 23), Vec(30, 12), "IN " + std::to_string(i + 1), 8.f, nvgRGB(255, 255, 255)));

            addParam(createParamCentered<SmallWhiteKnob>(Vec(knobStartX, yPos - 23), module, Multiverse::PHASE_PARAM_1 + i * 4));
            addParam(createParamCentered<SmallWhiteKnob>(Vec(knobStartX + 30, yPos - 23), module, Multiverse::RATIO_PARAM_1 + i * 4));
            addParam(createParamCentered<SmallWhiteKnob>(Vec(knobStartX, yPos + 10), module, Multiverse::ANGLE_PARAM_1 + i * 4));
            addParam(createParamCentered<SmallWhiteKnob>(Vec(knobStartX + 30, yPos + 10), module, Multiverse::INTENSITY_PARAM_1 + i * 4));

            addInput(createInputCentered<PJ301MPort>(Vec(cvStartX, yPos - 23), module, Multiverse::PHASE_CV_1 + i * 4));
            addInput(createInputCentered<PJ301MPort>(Vec(cvStartX + 30, yPos - 23), module, Multiverse::RATIO_CV_1 + i * 4));
            addInput(createInputCentered<PJ301MPort>(Vec(cvStartX, yPos + 10), module, Multiverse::ANGLE_CV_1 + i * 4));
            addInput(createInputCentered<PJ301MPort>(Vec(cvStartX + 30, yPos + 10), module, Multiverse::INTENSITY_CV_1 + i * 4));

            addChild(new EnhancedTextLabel(Vec(knobStartX - 13, yPos - 46), Vec(26, 10), "PHS", 7.f, nvgRGB(255, 255, 255)));
            addChild(new EnhancedTextLabel(Vec(knobStartX + 17, yPos - 46), Vec(26, 10), "RAT", 7.f, nvgRGB(255, 255, 255)));
            addChild(new EnhancedTextLabel(Vec(knobStartX - 13, yPos - 13), Vec(26, 10), "ANG", 7.f, nvgRGB(255, 255, 255)));
            addChild(new EnhancedTextLabel(Vec(knobStartX + 17, yPos - 13), Vec(26, 10), "INT", 7.f, nvgRGB(255, 255, 255)));
        }

        float globalControlsY = 360.0f;

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(20, globalControlsY), module, Multiverse::FREEZE_PARAM, Multiverse::FREEZE_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, globalControlsY), module, Multiverse::TRIGGER_INPUT));
        addParam(createParamCentered<SmallPinkKnob>(Vec(85, globalControlsY), module, Multiverse::MIX_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(120, globalControlsY), module, Multiverse::MIX_CV));

        addChild(new EnhancedTextLabel(Vec(15, globalControlsY - 20), Vec(30, 10), "TRG", 7.f, nvgRGB(255, 255, 255)));
        addChild(new EnhancedTextLabel(Vec(70, globalControlsY - 20), Vec(30, 10), "MIX", 7.f, nvgRGB(255, 133, 133)));
    }

    void appendContextMenu(Menu* menu) override {
        ModuleWidget::appendContextMenu(menu);

#ifdef __APPLE__
        if (!multiverseModule) return;

        menu->addChild(new MenuSeparator());

        struct ExternalWindowItem : MenuItem {
            Multiverse* module;
            void onAction(const event::Action& e) override {
                if (module->externalWindow) {
                    if (isMultiverseWindowOpen(module->externalWindow)) {
                        closeMultiverseWindow(module->externalWindow);
                    } else {
                        openMultiverseWindow(module->externalWindow);
                    }
                }
            }
            void step() override {
                if (module->externalWindow) {
                    text = isMultiverseWindowOpen(module->externalWindow) ?
                           "Close External Window" : "Open External Window";
                } else {
                    text = "External Window (unavailable)";
                    disabled = true;
                }
                MenuItem::step();
            }
        };

        ExternalWindowItem* item = new ExternalWindowItem();
        item->module = multiverseModule;
        menu->addChild(item);
#endif
    }
};

Model* modelMultiverse = createModel<Multiverse, MultiverseWidget>("Multiverse");