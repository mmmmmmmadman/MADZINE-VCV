#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

struct Obserfour : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

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

    static constexpr int SCOPE_BUFFER_SIZE = 256;
    
    ScopePoint scopeBuffer[8][SCOPE_BUFFER_SIZE];
    ScopePoint currentPoint[8];
    int bufferIndex = 0;
    int frameIndex = 0;
    
    dsp::SchmittTrigger triggers[16];

    Obserfour() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        const float maxTime = -std::log2(5e1f);
        const float minTime = -std::log2(5e-3f);
        const float defaultTime = -std::log2(5e-1f);
        configParam(TIME_PARAM, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);
        
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

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
    }

    void process(const ProcessArgs& args) override {
        bool trig = !params[TRIG_PARAM].getValue();
        lights[TRIG_LIGHT].setBrightness(trig);

        if (bufferIndex >= SCOPE_BUFFER_SIZE) {
            bool triggered = false;

            if (!trig) {
                triggered = true;
            }
            else {
                for (int i = 0; i < 8; i++) {
                    if (inputs[TRACK1_INPUT + i].isConnected()) {
                        int trigChannels = inputs[TRACK1_INPUT + i].getChannels();
                        for (int c = 0; c < trigChannels; c++) {
                            float trigVoltage = inputs[TRACK1_INPUT + i].getVoltage(c);
                            if (triggers[c].process(rescale(trigVoltage, 0.f, 0.001f, 0.f, 1.f))) {
                                triggered = true;
                            }
                        }
                        break;
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

        if (bufferIndex < SCOPE_BUFFER_SIZE) {
            float deltaTime = dsp::exp2_taylor5(-params[TIME_PARAM].getValue()) / SCOPE_BUFFER_SIZE;
            int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

            for (int i = 0; i < 8; i++) {
                float x = inputs[TRACK1_INPUT + i].getVoltage();
                currentPoint[i].min = std::min(currentPoint[i].min, x);
                currentPoint[i].max = std::max(currentPoint[i].max, x);
            }

            if (++frameIndex >= frameCount) {
                frameIndex = 0;
                for (int i = 0; i < 8; i++) {
                    scopeBuffer[i][bufferIndex] = currentPoint[i];
                }
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

// HiddenTimeKnob now using from widgets/Knobs.hpp

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

struct ObserfourScopeDisplay : LedDisplay {
    Obserfour* module;
    ModuleWidget* moduleWidget;
    
    ObserfourScopeDisplay() {
        box.size = Vec(120, 300);
    }
    
    void drawWave(const DrawArgs& args, int inputIndex, int displayTrack, NVGcolor color) {
        if (!module) return;
        
        nvgSave(args.vg);
        
        float trackHeight = box.size.y / 4.0f;
        float trackY = displayTrack * trackHeight;
        
        Rect b = Rect(Vec(0, trackY), Vec(box.size.x, trackHeight));
        nvgScissor(args.vg, RECT_ARGS(b));
        nvgBeginPath(args.vg);
        
        for (int i = 0; i < Obserfour::SCOPE_BUFFER_SIZE; i++) {
            const Obserfour::ScopePoint& point = module->scopeBuffer[inputIndex][i];
            float max = point.max;
            if (!std::isfinite(max))
                max = 0.f;

            Vec p;
            p.x = (float)i / (Obserfour::SCOPE_BUFFER_SIZE - 1);
            p.y = (max) * -0.05f + 0.5f;
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
        
        float trackHeight = box.size.y / 4.0f;
        
        for (int i = 0; i < 4; i++) {
            float trackY = i * trackHeight;
            
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY);
            nvgLineTo(args.vg, box.size.x, trackY);
            nvgStroke(args.vg);
            
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY + trackHeight / 2);
            nvgLineTo(args.vg, box.size.x, trackY + trackHeight / 2);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 15));
            nvgStroke(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 30));
        }
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, box.size.y);
        nvgLineTo(args.vg, box.size.x, box.size.y);
        nvgStroke(args.vg);
        
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
        
        for (int i = 0; i < 4; i++) {
            // Draw first input of each track pair
            PortWidget* inputPort1 = moduleWidget->getInput(Obserfour::TRACK1_INPUT + i);
            CableWidget* cable1 = APP->scene->rack->getTopCable(inputPort1);
            NVGcolor trackColor1 = cable1 ? cable1->color : nvgRGB(255, 255, 255);
            drawWave(args, i, i, trackColor1);
            
            // Draw second input of each track pair (inputs 5-8) on same display track
            PortWidget* inputPort2 = moduleWidget->getInput(Obserfour::TRACK5_INPUT + i);
            CableWidget* cable2 = APP->scene->rack->getTopCable(inputPort2);
            NVGcolor trackColor2 = cable2 ? cable2->color : nvgRGB(255, 255, 255);
            drawWave(args, i + 4, i, trackColor2);
        }
    }
};

struct ClickableLight : ParamWidget {
    Obserfour* module;
    
    ClickableLight() {
        box.size = Vec(8, 8);
    }
    
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        float brightness = module->lights[Obserfour::TRIG_LIGHT].getBrightness();
        
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

struct ObserfourWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    ObserfourWidget(Obserfour* module) {
        setModule(module);
        panelThemeHelper.init(this, "EuclideanRhythm");
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Obserfour", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        ClickableLight* trigLight = createParam<ClickableLight>(Vec(100, 13), module, Obserfour::TRIG_PARAM);
        trigLight->module = module;
        addParam(trigLight);
        
        ObserfourScopeDisplay* scopeDisplay = new ObserfourScopeDisplay();
        scopeDisplay->box.pos = Vec(0, 30);
        scopeDisplay->box.size = Vec(120, 300);
        scopeDisplay->module = module;
        scopeDisplay->moduleWidget = this;
        addChild(scopeDisplay);
        
        addParam(createParam<madzine::widgets::HiddenTimeKnobObserver>(Vec(0, 30), module, Obserfour::TIME_PARAM));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, Obserfour::TRACK1_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 343), module, Obserfour::TRACK2_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(75, 343), module, Obserfour::TRACK3_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(105, 343), module, Obserfour::TRACK4_INPUT));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, Obserfour::TRACK5_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45, 368), module, Obserfour::TRACK6_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(75, 368), module, Obserfour::TRACK7_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(105, 368), module, Obserfour::TRACK8_INPUT));
    }

    void step() override {
        Obserfour* module = dynamic_cast<Obserfour*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Obserfour* module = dynamic_cast<Obserfour*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelObserfour = createModel<Obserfour, ObserfourWidget>("Obserfour");
