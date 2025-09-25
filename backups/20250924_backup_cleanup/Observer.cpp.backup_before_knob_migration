#include "plugin.hpp"

struct Observer : Module {
    enum ParamIds {
        TIME_PARAM,
        TRIG_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRACK1_INPUT,
        TRACK2_INPUT,
        TRACK3_INPUT,
        TRACK4_INPUT,
        TRACK5_INPUT,
        TRACK6_INPUT,
        TRACK7_INPUT,
        TRACK8_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        TRIG_LIGHT,
        NUM_LIGHTS
    };

    struct ScopePoint {
        float min = INFINITY;
        float max = -INFINITY;
    };

    static constexpr int SCOPE_BUFFER_SIZE = 256; // Same as VCV Scope
    
    ScopePoint scopeBuffer[8][SCOPE_BUFFER_SIZE];
    ScopePoint currentPoint[8];
    int bufferIndex = 0;
    int frameIndex = 0;
    
    dsp::SchmittTrigger triggers[16];

    Observer() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        // Time parameter (same as VCV Scope and QQ)
        const float maxTime = -std::log2(5e1f);
        const float minTime = -std::log2(5e-3f);
        const float defaultTime = -std::log2(5e-1f);
        configParam(TIME_PARAM, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);
        
        // Trigger parameter (100% copy from VCV Scope)
        configSwitch(TRIG_PARAM, 0.f, 1.f, 1.f, "Trigger", {"Enabled", "Disabled"});
        
        configLight(TRIG_LIGHT, "Trigger Light");
        
        configInput(TRACK1_INPUT, "Track 1");
        configInput(TRACK2_INPUT, "Track 2");
        configInput(TRACK3_INPUT, "Track 3");
        configInput(TRACK4_INPUT, "Track 4");
        configInput(TRACK5_INPUT, "Track 5");
        configInput(TRACK6_INPUT, "Track 6");
        configInput(TRACK7_INPUT, "Track 7");
        configInput(TRACK8_INPUT, "Track 8");
    }

    void process(const ProcessArgs& args) override {
        bool trig = !params[TRIG_PARAM].getValue();
        lights[TRIG_LIGHT].setBrightness(trig);

        // Detect trigger if no longer recording (100% copy from VCV Scope)
        if (bufferIndex >= SCOPE_BUFFER_SIZE) {
            bool triggered = false;

            // Trigger immediately if trigger detection is disabled
            if (!trig) {
                triggered = true;
            }
            else {
                // Reset if triggered - use first connected input as trigger source
                for (int i = 0; i < 8; i++) {
                    if (inputs[TRACK1_INPUT + i].isConnected()) {
                        int trigChannels = inputs[TRACK1_INPUT + i].getChannels();
                        for (int c = 0; c < trigChannels; c++) {
                            float trigVoltage = inputs[TRACK1_INPUT + i].getVoltage(c);
                            if (triggers[c].process(rescale(trigVoltage, 0.f, 0.001f, 0.f, 1.f))) {
                                triggered = true;
                            }
                        }
                        break; // Only use first connected input
                    }
                }
            }

            if (triggered) {
                for (int c = 0; c < 16; c++) {
                    triggers[c].reset();
                }
                bufferIndex = 0;
                frameIndex = 0;
            }
        }

        // Add point to buffer if recording (100% copy from VCV Scope logic)
        if (bufferIndex < SCOPE_BUFFER_SIZE) {
            // Compute time
            float deltaTime = dsp::exp2_taylor5(-params[TIME_PARAM].getValue()) / SCOPE_BUFFER_SIZE;
            int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

            // Get input
            for (int i = 0; i < 8; i++) {
                float x = inputs[TRACK1_INPUT + i].getVoltage();
                currentPoint[i].min = std::min(currentPoint[i].min, x);
                currentPoint[i].max = std::max(currentPoint[i].max, x);
            }

            if (++frameIndex >= frameCount) {
                frameIndex = 0;
                // Push current point
                for (int i = 0; i < 8; i++) {
                    scopeBuffer[i][bufferIndex] = currentPoint[i];
                }
                // Reset current point
                for (int i = 0; i < 8; i++) {
                    currentPoint[i] = ScopePoint();
                }
                bufferIndex++;
            }
        }
    }
};

struct EnhancedTextLabel : Widget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    EnhancedTextLabel(Vec pos, Vec size, const std::string& text, float fontSize = 12.f, 
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
        nvgFillColor(args.vg, color);
        
        if (bold) {
            float offset = 0.3f;
            nvgText(args.vg, box.size.x / 2.f - offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f + offset, box.size.y / 2.f, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f - offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f + offset, text.c_str(), NULL);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        } else {
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        }
    }
};

struct HiddenTimeKnob : ParamWidget {
    HiddenTimeKnob() {
        box.size = Vec(120, 300); // Same size as scope display
    }
    
    void draw(const DrawArgs& args) override {
        // Draw nothing - completely invisible
    }
    
    void onEnter(const event::Enter& e) override {
        glfwSetCursor(APP->window->win, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
        ParamWidget::onEnter(e);
    }
    
    void onLeave(const event::Leave& e) override {
        glfwSetCursor(APP->window->win, NULL);
        ParamWidget::onLeave(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        float sensitivity = 0.01f; // Vertical drag sensitivity
        float deltaValue = -e.mouseDelta.y * sensitivity;
        pq->setValue(pq->getValue() + deltaValue);
        e.consume(this);
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

struct ObserverScopeDisplay : LedDisplay {
    Observer* module;
    ModuleWidget* moduleWidget;
    
    ObserverScopeDisplay() {
        box.size = Vec(120, 300); // 8HP width, adjusted height for 8 tracks
    }
    
    void drawWave(const DrawArgs& args, int track, NVGcolor color) {
        if (!module) return;
        
        nvgSave(args.vg);
        
        // Calculate track area (12.5% height each for 8 tracks)
        float trackHeight = box.size.y / 8.0f;
        float trackY = track * trackHeight;
        
        Rect b = Rect(Vec(0, trackY), Vec(box.size.x, trackHeight));
        nvgScissor(args.vg, RECT_ARGS(b));
        nvgBeginPath(args.vg);
        
        for (int i = 0; i < Observer::SCOPE_BUFFER_SIZE; i++) {
            const Observer::ScopePoint& point = module->scopeBuffer[track][i];
            float max = point.max;
            if (!std::isfinite(max))
                max = 0.f;

            Vec p;
            p.x = (float)i / (Observer::SCOPE_BUFFER_SIZE - 1);
            p.y = (max) * -0.05f + 0.5f; // Scale for ±10V range
            p = b.interpolate(p);
            
            if (i == 0)
                nvgMoveTo(args.vg, p.x, p.y);
            else
                nvgLineTo(args.vg, p.x, p.y);
        }
        
        nvgStrokeColor(args.vg, color);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStroke(args.vg);
        nvgResetScissor(args.vg);
        nvgRestore(args.vg);
    }
    
    void drawBackground(const DrawArgs& args) {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 30));
        nvgStrokeWidth(args.vg, 0.5f);
        
        // Draw track separation lines
        float trackHeight = box.size.y / 8.0f;
        
        for (int i = 0; i < 8; i++) {
            float trackY = i * trackHeight;
            
            // Top line of each track area
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY);
            nvgLineTo(args.vg, box.size.x, trackY);
            nvgStroke(args.vg);
            
            // Center line of each track (for reference)
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY + trackHeight / 2);
            nvgLineTo(args.vg, box.size.x, trackY + trackHeight / 2);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 15));
            nvgStroke(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 30));
        }
        
        // Bottom line of last track
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, box.size.y);
        nvgLineTo(args.vg, box.size.x, box.size.y);
        nvgStroke(args.vg);
        
        // Overall border
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStroke(args.vg);
    }
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;
        
        drawBackground(args);
        
        if (!module || !moduleWidget) return;
        
        // Get input colors from cable connections, with white as default
        for (int i = 0; i < 8; i++) {
            PortWidget* inputPort = moduleWidget->getInput(Observer::TRACK1_INPUT + i);
            CableWidget* cable = APP->scene->rack->getTopCable(inputPort);
            NVGcolor trackColor = cable ? cable->color : nvgRGB(255, 255, 255); // White when no cable
            
            drawWave(args, i, trackColor);
        }
    }
};

struct ClickableLight : ParamWidget {
    Observer* module;
    
    ClickableLight() {
        box.size = Vec(8, 8);
    }
    
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        float brightness = module->lights[Observer::TRIG_LIGHT].getBrightness();
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, box.size.x / 2 - 1);
        
        if (brightness > 0.5f) {
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        } else {
            nvgFillColor(args.vg, nvgRGB(255, 133, 133));
        }
        nvgFill(args.vg);
        
        nvgStrokeColor(args.vg, nvgRGB(200, 200, 200));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);
    }
    
    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                float currentValue = pq->getValue();
                pq->setValue(1.f - currentValue);
            }
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }
};

struct ObserverWidget : ModuleWidget {
    ObserverWidget(Observer* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Observer", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        // Clickable light for Trig - using correct ParamWidget method
        ClickableLight* trigLight = createParam<ClickableLight>(Vec(100, 13), module, Observer::TRIG_PARAM);
        trigLight->module = module;
        addParam(trigLight);
        
        // Scope display - adjusted height for new input layout
        ObserverScopeDisplay* scopeDisplay = new ObserverScopeDisplay();
        scopeDisplay->box.pos = Vec(0, 30);
        scopeDisplay->box.size = Vec(120, 300); // Adjusted height
        scopeDisplay->module = module;
        scopeDisplay->moduleWidget = this;
        addChild(scopeDisplay);
        
        // Hidden time control knob (overlapping entire scope display)
        addParam(createParam<HiddenTimeKnob>(Vec(0, 30), module, Observer::TIME_PARAM));
        
        // White background box for inputs (at bottom) - consistent with other modules
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        // 8 inputs arranged in 4x2 grid - more evenly distributed X positions
        // Top row (tracks 1-4) - Y position matches MADDY top row
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, Observer::TRACK1_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 343), module, Observer::TRACK2_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(75, 343), module, Observer::TRACK3_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(105, 343), module, Observer::TRACK4_INPUT));
        
        // Bottom row (tracks 5-8) - Y position matches MADDY bottom row
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, Observer::TRACK5_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 368), module, Observer::TRACK6_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(75, 368), module, Observer::TRACK7_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(105, 368), module, Observer::TRACK8_INPUT));
    }
};

Model* modelObserver = createModel<Observer, ObserverWidget>("Observer");