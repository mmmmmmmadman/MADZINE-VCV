#include "plugin.hpp"
#include <cmath>
#include <vector>
#include <deque>

#ifdef __APPLE__
// External window functions for macOS
extern "C" {
    // OpenGL window functions
    void* createINGOQWindow(int width, int height);
    void destroyINGOQWindow(void* window);
    void openINGOQWindow(void* window);
    void closeINGOQWindow(void* window);
    void updateINGOQWindow(void* window, const float* buffer, int size,
                          float brightness, float angle, float phase, float frequency);
    bool isINGOQWindowOpen(void* window);
}
#endif

struct INGOQ : Module {
    enum ParamId {
        TIME_PARAM,  // Time zoom control (like oscilloscope)
        BRIGHTNESS_PARAM,
        ANGLE_PARAM,  // Rotation angle
        PHASE_PARAM,  // Phase shift
        FREEZE_PARAM,  // Trigger on/off
        PARAMS_LEN
    };
    enum InputId {
        SIGNAL_INPUT,
        TRIGGER_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTS_LEN
    };
    enum LightId {
        FREEZE_LIGHT,
        LIGHTS_LEN
    };
    
    // Display dimensions - increased resolution
    static const int DISPLAY_WIDTH = 1024;
    static const int DISPLAY_HEIGHT = 512;
    
    // Display buffer - what we actually show
    float displayBuffer[DISPLAY_WIDTH];
    int bufferIndex = 0;
    int frameIndex = 0;
    
    // Trigger mode (like oscilloscope)
    bool triggerEnabled = false;
    dsp::SchmittTrigger triggerButtonToggle;
    dsp::SchmittTrigger signalTrigger;
    dsp::SchmittTrigger externalTrigger;
    
    // Frequency detection for color
    float dominantFrequency = 440.0f;
    float lastVoltage = 0.0f;
    int zeroCrossings = 0;
    int sampleCount = 0;
    
#ifdef __APPLE__
    // External window
    void* externalWindow = nullptr;
#endif
    
    INGOQ() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(TIME_PARAM, -2.0f, 2.0f, 0.0f, "Time Scale", " ms/screen", 10.0f, 10.0f);
        configParam(BRIGHTNESS_PARAM, 0.1f, 2.0f, 1.0f, "Brightness");
        configParam(ANGLE_PARAM, -180.0f, 180.0f, 0.0f, "Rotation Angle", "°");
        configParam(PHASE_PARAM, -180.0f, 180.0f, 0.0f, "Phase", "°");
        configButton(FREEZE_PARAM, "Trigger");
        
        configInput(SIGNAL_INPUT, "Signal");
        configInput(TRIGGER_INPUT, "External Trigger");
        
        configLight(FREEZE_LIGHT, "Trigger");
        
        // Initialize buffer
        for (int i = 0; i < DISPLAY_WIDTH; i++) {
            displayBuffer[i] = 0.0f;
        }
        
#ifdef __APPLE__
        // Initialize window pointer (window will be created when needed)
        externalWindow = nullptr;
#endif
    }
    
    ~INGOQ() {
#ifdef __APPLE__
        if (externalWindow) {
            destroyINGOQWindow(externalWindow);
        }
#endif
    }
    
    void process(const ProcessArgs& args) override {
        if (!inputs[SIGNAL_INPUT].isConnected()) return;
        
        float voltage = inputs[SIGNAL_INPUT].getVoltage();
        
        // Frequency detection for color mapping - optimized with smaller sample window
        if ((lastVoltage < 0 && voltage >= 0) || (lastVoltage >= 0 && voltage < 0)) {
            zeroCrossings++;
        }
        lastVoltage = voltage;
        sampleCount++;
        
        if (sampleCount >= 512) {  // Reduced from 1024 for faster response
            float newFreq = (zeroCrossings / 2.0f) * (args.sampleRate / 512.0f);
            if (newFreq > 20.0f && newFreq < 20000.0f) {
                dominantFrequency = newFreq;
            }
            zeroCrossings = 0;
            sampleCount = 0;
        }
        
        // Handle trigger button (latch mode toggle)
        if (triggerButtonToggle.process(params[FREEZE_PARAM].getValue())) {
            triggerEnabled = !triggerEnabled;
        }
        
        // Update trigger light
        lights[FREEZE_LIGHT].setBrightness(triggerEnabled ? 1.0f : 0.0f);
        
#ifdef __APPLE__
        // Update external window if open
        float brightness = params[BRIGHTNESS_PARAM].getValue();
        float angle = params[ANGLE_PARAM].getValue();
        float phase = params[PHASE_PARAM].getValue() / 360.0f;
        
        if (externalWindow && isINGOQWindowOpen(externalWindow)) {
            updateINGOQWindow(externalWindow, displayBuffer, DISPLAY_WIDTH,
                            brightness, angle, phase, dominantFrequency);
        }
#endif
        
        // Check if we've filled the buffer
        if (bufferIndex >= DISPLAY_WIDTH) {
            // Need to wait for trigger if enabled
            if (triggerEnabled) {
                bool triggered = false;
                
                // Check external trigger first
                if (inputs[TRIGGER_INPUT].isConnected()) {
                    if (externalTrigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
                        triggered = true;
                    }
                } else {
                    // Use signal input for trigger (threshold at 0V)
                    if (signalTrigger.process(rescale(voltage, 0.0f, 0.01f, 0.0f, 1.0f))) {
                        triggered = true;
                    }
                }
                
                if (triggered) {
                    // Start new sweep
                    bufferIndex = 0;
                    frameIndex = 0;
                    signalTrigger.reset();
                    externalTrigger.reset();
                }
            } else {
                // No trigger mode - continuous sweep
                bufferIndex = 0;
                frameIndex = 0;
            }
        }
        
        // Calculate how many samples per pixel based on time scale
        float timeScale = params[TIME_PARAM].getValue();
        float msPerScreen = std::pow(10.0f, timeScale) * 10.0f;
        float samplesPerScreen = args.sampleRate * msPerScreen / 1000.0f;
        float samplesPerPixel = samplesPerScreen / DISPLAY_WIDTH;
        
        // Accumulate samples and update display only if not waiting for trigger
        if (bufferIndex < DISPLAY_WIDTH) {
            frameIndex++;
            if (frameIndex >= (int)samplesPerPixel) {
                // Use peak detection for better visual accuracy at high resolution
                displayBuffer[bufferIndex] = voltage;
                bufferIndex++;
                frameIndex = 0;
            }
        }
    }
    
    void onReset() override {
        bufferIndex = 0;
        frameIndex = 0;
        dominantFrequency = 440.0f;
        triggerEnabled = false;
        for (int i = 0; i < DISPLAY_WIDTH; i++) {
            displayBuffer[i] = 0.0f;
        }
    }
    
    float getHueFromFrequency() {
        // Map frequency to hue (20Hz - 20kHz to 0-360 degrees)
        float logFreq = std::log10(std::max(20.0f, dominantFrequency));
        float logMin = std::log10(20.0f);
        float logMax = std::log10(20000.0f);
        float hue = ((logFreq - logMin) / (logMax - logMin)) * 360.0f;
        return std::fmod(hue, 360.0f);
    }
    
};

struct INGOQDisplay : Widget {
    INGOQ* module = nullptr;
    
    // Framebuffer for smooth rendering
    int imageHandle = -1;
    std::vector<unsigned char> pixelData;
    
    INGOQDisplay() {
        box.size = Vec(300, 200);
        size_t pixelCount = (size_t)INGOQ::DISPLAY_WIDTH * (size_t)INGOQ::DISPLAY_HEIGHT * 4;
        pixelData.resize(pixelCount);
    }
    
    ~INGOQDisplay() {
        if (imageHandle >= 0 && APP && APP->window && APP->window->vg) {
            nvgDeleteImage(APP->window->vg, imageHandle);
        }
    }
    
    void draw(const DrawArgs& args) override {
        // Draw background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        if (!module) {
            // Draw border
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgStrokeColor(args.vg, nvgRGBA(128, 128, 128, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
            return;
        }
        
        // Use the display buffer directly
        float* buffer = module->displayBuffer;
        
        // Get current hue from frequency
        float hue = module->getHueFromFrequency();
        float brightness = module->params[INGOQ::BRIGHTNESS_PARAM].getValue();
        
        // Convert HSV to RGB
        float c = 1.0f;
        float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2) - 1));
        float m = 0;
        
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
        
        r = (r + m) * 255;
        g = (g + m) * 255;
        b = (b + m) * 255;
        
        // Get phase shift
        float phaseShift = module->params[INGOQ::PHASE_PARAM].getValue();
        float phaseOffset = (phaseShift / 360.0f) * INGOQ::DISPLAY_WIDTH;
        
        // Optimized pixel update with reduced operations
        for (int x = 0; x < INGOQ::DISPLAY_WIDTH; x++) {
            // Apply phase shift by reading from shifted position
            int shiftedX = (x + (int)phaseOffset + INGOQ::DISPLAY_WIDTH) % INGOQ::DISPLAY_WIDTH;
            float voltage = buffer[shiftedX];
            
            // Map voltage (-10V to +10V) to brightness (0 to 1)
            float normalizedVoltage = (voltage + 10.0f) * 0.05f;  // Optimized: multiply by 0.05 instead of divide by 20
            normalizedVoltage = clamp(normalizedVoltage, 0.0f, 1.0f) * brightness;
            
            // Pre-calculate color values
            unsigned char finalR = (unsigned char)(r * normalizedVoltage);
            unsigned char finalG = (unsigned char)(g * normalizedVoltage);
            unsigned char finalB = (unsigned char)(b * normalizedVoltage);
            
            // Fill entire column with pre-calculated color
            size_t baseIdx = (size_t)x * 4;
            for (int y = 0; y < INGOQ::DISPLAY_HEIGHT; y++) {
                size_t idx = (size_t)y * (size_t)INGOQ::DISPLAY_WIDTH * 4 + baseIdx;
                if (idx + 3 < pixelData.size()) {
                    pixelData[idx + 0] = finalR;
                    pixelData[idx + 1] = finalG;
                    pixelData[idx + 2] = finalB;
                    pixelData[idx + 3] = 255;
                }
            }
        }
        
        // Create or update NanoVG image
        if (pixelData.size() > 0 && args.vg) {
            if (imageHandle >= 0) {
                nvgUpdateImage(args.vg, imageHandle, pixelData.data());
            } else {
                imageHandle = nvgCreateImageRGBA(args.vg, INGOQ::DISPLAY_WIDTH, INGOQ::DISPLAY_HEIGHT, 
                                                0, pixelData.data());
            }
            
            // Draw the image scaled to widget size with rotation
            if (imageHandle >= 0) {
                // Get rotation angle
                float angle = module->params[INGOQ::ANGLE_PARAM].getValue();
                float angleRad = angle * M_PI / 180.0f;
                
                // Calculate scale factor to fill frame when rotated
                // We need to scale up the image so that when rotated, it still fills the entire frame
                float scale = 1.0f;
                if (std::abs(angle) > 0.01f) {
                    // Calculate how much we need to scale to cover the frame after rotation
                    float w = box.size.x;
                    float h = box.size.y;
                    float cosA = std::abs(std::cos(angleRad));
                    float sinA = std::abs(std::sin(angleRad));
                    
                    // The scale needed to ensure the rotated rectangle covers the original
                    float scaleX = (w * cosA + h * sinA) / w;
                    float scaleY = (w * sinA + h * cosA) / h;
                    scale = std::max(scaleX, scaleY);
                }
                
                // Save current transform
                nvgSave(args.vg);
                
                // Apply clipping to prevent rotation from going outside bounds
                nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);
                
                // Translate to center, rotate and scale, then translate back
                nvgTranslate(args.vg, box.size.x / 2.0f, box.size.y / 2.0f);
                nvgRotate(args.vg, angleRad);
                nvgScale(args.vg, scale, scale);
                nvgTranslate(args.vg, -box.size.x / 2.0f, -box.size.y / 2.0f);
                
                // Draw the image with proper scaling for high resolution
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
                // Scale pattern to fit display area
                float scaleX = box.size.x / INGOQ::DISPLAY_WIDTH;
                float scaleY = box.size.y / INGOQ::DISPLAY_HEIGHT;
                NVGpaint paint = nvgImagePattern(args.vg, 0, 0, 
                                                box.size.x / scaleX, box.size.y / scaleY, 
                                                0, imageHandle, 1.0f);
                nvgFillPaint(args.vg, paint);
                nvgFill(args.vg);
                
                // Reset scissor
                nvgResetScissor(args.vg);
                
                // Restore transform
                nvgRestore(args.vg);
            }
        }
        
        // Draw border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGBA(128, 128, 128, 255));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
    }
};

struct INGOQWidget : ModuleWidget {
    INGOQWidget(INGOQ* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/INGOQ.svg")));
        
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        // Display
        INGOQDisplay* display = new INGOQDisplay();
        display->module = module;
        display->box.pos = Vec(30, 50);
        addChild(display);
        
        // Inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 100)), module, INGOQ::SIGNAL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 115)), module, INGOQ::TRIGGER_INPUT));
        
        // Controls - first row
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35, 100)), module, INGOQ::TIME_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50, 100)), module, INGOQ::BRIGHTNESS_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(65, 100)), module, INGOQ::ANGLE_PARAM));
        
        // Controls - second row
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35, 115)), module, INGOQ::PHASE_PARAM));
        
        // Trigger button with light
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(
            mm2px(Vec(55, 115)), module, INGOQ::FREEZE_PARAM, INGOQ::FREEZE_LIGHT));
    }
    
#ifdef __APPLE__
    void appendContextMenu(Menu* menu) override {
        INGOQ* module = dynamic_cast<INGOQ*>(this->module);
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        
        struct ExternalWindowItem : MenuItem {
            INGOQ* module;
            void onAction(const event::Action& e) override {
                // Toggle OpenGL window
                if (module->externalWindow && isINGOQWindowOpen(module->externalWindow)) {
                    // Close OpenGL window
                    closeINGOQWindow(module->externalWindow);
                } else {
                    // Create and open OpenGL window
                    if (!module->externalWindow) {
                        module->externalWindow = createINGOQWindow(INGOQ::DISPLAY_WIDTH, INGOQ::DISPLAY_HEIGHT);
                    } else {
                        openINGOQWindow(module->externalWindow);
                    }
                }
            }
        };
        
        ExternalWindowItem* item = new ExternalWindowItem;
        item->text = "External Display Window";
        item->module = module;
        if (module->externalWindow && isINGOQWindowOpen(module->externalWindow)) {
            item->rightText = "✓";
        }
        menu->addChild(item);
    }
#endif
};

Model* modelINGOQ = createModel<INGOQ, INGOQWidget>("INGOQ");