#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

struct QQ : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamIds {
        TRACK1_DECAY_TIME_PARAM,
        TRACK1_SHAPE_PARAM,
        TRACK2_DECAY_TIME_PARAM,
        TRACK2_SHAPE_PARAM,
        TRACK3_DECAY_TIME_PARAM,
        TRACK3_SHAPE_PARAM,
        SCOPE_TIME_PARAM,
        TRACK1_DECAY_CV_ATTEN_PARAM,
        TRACK2_DECAY_CV_ATTEN_PARAM,
        TRACK3_DECAY_CV_ATTEN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRACK1_TRIG_INPUT,
        TRACK2_TRIG_INPUT,
        TRACK3_TRIG_INPUT,
        TRACK1_DECAY_CV_INPUT,
        TRACK2_DECAY_CV_INPUT,
        TRACK3_DECAY_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRACK1_ENV_OUTPUT,
        TRACK2_ENV_OUTPUT,
        TRACK3_ENV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        TRACK1_TRIG_LIGHT,
        TRACK2_TRIG_LIGHT,
        TRACK3_TRIG_LIGHT,
        NUM_LIGHTS
    };

    struct TrackState {
        dsp::SchmittTrigger trigTrigger;
        dsp::PulseGenerator trigPulse;
        float phase = 0.f;
        bool gateState = false;
        float attackTime = 0.001f;  // Default 1ms attack time
        float lastEnvOutput = 0.f;  // For retrigger mode
    };

    struct ScopePoint {
        float value = 0.f;
    };

    TrackState tracks[3];

    bool retriggerEnabled = false;  // Retrigger option
    static constexpr int SCOPE_BUFFER_SIZE = 128;
    
    ScopePoint scopeBuffer[3][SCOPE_BUFFER_SIZE];
    int scopeBufferIndex = 0;
    int scopeFrameIndex = 0;

    QQ() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        configParam(TRACK1_DECAY_TIME_PARAM, 0.01f, 2.f, 1.f, "Track 1 Decay Time", "s");
        configParam(TRACK1_SHAPE_PARAM, 0.f, 0.99f, 0.5f, "Track 1 Shape");
        configParam(TRACK2_DECAY_TIME_PARAM, 0.01f, 2.f, 1.f, "Track 2 Decay Time", "s");
        configParam(TRACK2_SHAPE_PARAM, 0.f, 0.99f, 0.5f, "Track 2 Shape");
        configParam(TRACK3_DECAY_TIME_PARAM, 0.01f, 2.f, 1.f, "Track 3 Decay Time", "s");
        configParam(TRACK3_SHAPE_PARAM, 0.f, 0.99f, 0.5f, "Track 3 Shape");
        
        // Scope time parameter (same as VCV Scope)
        const float maxTime = -std::log2(5e1f);
        const float minTime = -std::log2(5e-3f);
        const float defaultTime = -std::log2(5e-1f);
        configParam(SCOPE_TIME_PARAM, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);
        
        // Track 1 Decay CV Attenuator (unipolar 0-1)
        configParam(TRACK1_DECAY_CV_ATTEN_PARAM, 0.f, 1.f, 0.5f, "Track 1 Decay CV Amount", "%", 0.f, 100.f);
        configParam(TRACK2_DECAY_CV_ATTEN_PARAM, 0.f, 1.f, 0.5f, "Track 2 Decay CV Amount", "%", 0.f, 100.f);
        configParam(TRACK3_DECAY_CV_ATTEN_PARAM, 0.f, 1.f, 0.5f, "Track 3 Decay CV Amount", "%", 0.f, 100.f);
        
        configInput(TRACK1_TRIG_INPUT, "Track 1 Trigger");
        configInput(TRACK2_TRIG_INPUT, "Track 2 Trigger");
        configInput(TRACK3_TRIG_INPUT, "Track 3 Trigger");
        configInput(TRACK1_DECAY_CV_INPUT, "Track 1 Decay CV");
        configInput(TRACK2_DECAY_CV_INPUT, "Track 2 Decay CV");
        configInput(TRACK3_DECAY_CV_INPUT, "Track 3 Decay CV");
        
        configOutput(TRACK1_ENV_OUTPUT, "Track 1 Envelope");
        configOutput(TRACK2_ENV_OUTPUT, "Track 2 Envelope");
        configOutput(TRACK3_ENV_OUTPUT, "Track 3 Envelope");
        
        configLight(TRACK1_TRIG_LIGHT, "Track 1 Trigger");
        configLight(TRACK2_TRIG_LIGHT, "Track 2 Trigger");
        configLight(TRACK3_TRIG_LIGHT, "Track 3 Trigger");
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

        // Save attack times for each track
        json_t* attackTimesJ = json_array();
        for (int i = 0; i < 3; i++) {
            json_array_append_new(attackTimesJ, json_real(tracks[i].attackTime));
        }
        json_object_set_new(rootJ, "attackTimes", attackTimesJ);

        // Save retrigger option
        json_object_set_new(rootJ, "retriggerEnabled", json_boolean(retriggerEnabled));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }

        // Load attack times for each track
        json_t* attackTimesJ = json_object_get(rootJ, "attackTimes");
        if (attackTimesJ) {
            for (int i = 0; i < 3; i++) {
                json_t* attackTimeJ = json_array_get(attackTimesJ, i);
                if (attackTimeJ) {
                    tracks[i].attackTime = json_real_value(attackTimeJ);
                }
            }
        }

        // Load retrigger option
        json_t* retriggerJ = json_object_get(rootJ, "retriggerEnabled");
        if (retriggerJ) {
            retriggerEnabled = json_boolean_value(retriggerJ);
        }
    }

    float smoothDecayEnvelope(float t, float totalTime, float shapeParam) {
        if (t >= totalTime) return 0.f;
        
        float normalizedT = t / totalTime;
        
        float frontK = -0.9f + shapeParam * 0.5f;
        float backK = -1.0f + 1.6f * std::pow(shapeParam, 0.3f);
        
        float transition = normalizedT * normalizedT * (3.f - 2.f * normalizedT);
        float k = frontK + (backK - frontK) * transition;
        
        float absT = std::abs(normalizedT);
        float denominator = k - 2.f * k * absT + 1.f;
        if (std::abs(denominator) < 1e-10f) {
            return 1.f - normalizedT;
        }
        
        float curveResult = (normalizedT - k * normalizedT) / denominator;
        return 1.f - curveResult;
    }

    void process(const ProcessArgs& args) override {
        for (int i = 0; i < 3; i++) {
            bool triggered = tracks[i].trigTrigger.process(inputs[TRACK1_TRIG_INPUT + i].getVoltage(), 0.1f, 2.f);

            if (triggered) {
                // Always restart phase on trigger
                tracks[i].phase = 0.f;
                tracks[i].gateState = true;
                tracks[i].trigPulse.trigger(0.03f);
            }

            lights[TRACK1_TRIG_LIGHT + i].setBrightness(tracks[i].trigPulse.process(args.sampleTime) ? 1.f : 0.f);

            float decayTime, shapeParam;
            if (i == 0) {
                decayTime = params[TRACK1_DECAY_TIME_PARAM].getValue();
                // Apply CV modulation to Track 1 decay time with attenuator
                if (inputs[TRACK1_DECAY_CV_INPUT].isConnected()) {
                    float cv = inputs[TRACK1_DECAY_CV_INPUT].getVoltage();
                    float attenuation = params[TRACK1_DECAY_CV_ATTEN_PARAM].getValue();
                    decayTime += cv / 10.f * 2.f * attenuation; // CV range with attenuator
                    decayTime = clamp(decayTime, 0.01f, 2.f);
                }
                shapeParam = params[TRACK1_SHAPE_PARAM].getValue();
            } else if (i == 1) {
                decayTime = params[TRACK2_DECAY_TIME_PARAM].getValue();
                // Apply CV modulation to Track 2 decay time with attenuator
                if (inputs[TRACK2_DECAY_CV_INPUT].isConnected()) {
                    float cv = inputs[TRACK2_DECAY_CV_INPUT].getVoltage();
                    float attenuation = params[TRACK2_DECAY_CV_ATTEN_PARAM].getValue();
                    decayTime += cv / 10.f * 2.f * attenuation; // CV range with attenuator
                    decayTime = clamp(decayTime, 0.01f, 2.f);
                }
                shapeParam = params[TRACK2_SHAPE_PARAM].getValue();
            } else {
                decayTime = params[TRACK3_DECAY_TIME_PARAM].getValue();
                // Apply CV modulation to Track 3 decay time with attenuator
                if (inputs[TRACK3_DECAY_CV_INPUT].isConnected()) {
                    float cv = inputs[TRACK3_DECAY_CV_INPUT].getVoltage();
                    float attenuation = params[TRACK3_DECAY_CV_ATTEN_PARAM].getValue();
                    decayTime += cv / 10.f * 2.f * attenuation; // CV range with attenuator
                    decayTime = clamp(decayTime, 0.01f, 2.f);
                }
                shapeParam = params[TRACK3_SHAPE_PARAM].getValue();
            }

            float envOutput = 0.f;

            if (tracks[i].gateState) {
                float attackTime = tracks[i].attackTime;  // Use per-track attack time

                if (tracks[i].phase < attackTime) {
                    // Attack phase
                    if (retriggerEnabled && tracks[i].lastEnvOutput > 0.f) {
                        // Retrigger mode: blend from last envelope output to 1.0
                        float attackProgress = tracks[i].phase / attackTime;
                        envOutput = tracks[i].lastEnvOutput + (1.f - tracks[i].lastEnvOutput) * attackProgress;
                    } else {
                        // Normal mode: attack from 0 to 1.0
                        envOutput = tracks[i].phase / attackTime;
                    }
                } else {
                    // Decay phase
                    float decayPhase = tracks[i].phase - attackTime;

                    if (decayPhase >= decayTime) {
                        tracks[i].gateState = false;
                        envOutput = 0.f;
                    } else {
                        envOutput = smoothDecayEnvelope(decayPhase, decayTime, shapeParam);
                    }
                }

                tracks[i].phase += args.sampleTime;
            }

            // Store last envelope output for retrigger
            tracks[i].lastEnvOutput = envOutput;

            outputs[TRACK1_ENV_OUTPUT + i].setVoltage(envOutput * 10.f);
        }
        
        // Update scope buffer
        float deltaTime = dsp::exp2_taylor5(-params[SCOPE_TIME_PARAM].getValue()) / SCOPE_BUFFER_SIZE;
        int frameCount = (int)std::ceil(deltaTime * args.sampleRate);
        if (++scopeFrameIndex >= frameCount) {
            scopeFrameIndex = 0;
            for (int i = 0; i < 3; i++) {
                scopeBuffer[i][scopeBufferIndex].value = outputs[TRACK1_ENV_OUTPUT + i].getVoltage();
            }
            scopeBufferIndex = (scopeBufferIndex + 1) % SCOPE_BUFFER_SIZE;
        }
    }
};

// StandardBlackKnob 現在從 widgets/Knobs.hpp 引入
// 使用 30x30 版本

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

struct CVConnectionLine : Widget {
    int trackNumber;
    
    CVConnectionLine(Vec pos, Vec size, int track) {
        box.pos = pos;
        box.size = size;
        trackNumber = track;
    }
    
    void draw(const DrawArgs &args) override {
        float centerX = 30; // 4 HP = 60px, so center is 30px
        
        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        
        if (trackNumber == 1) {
            // Track 1: Decay knob at (15, 85), CV input at (centerX + 15, 63)
            nvgMoveTo(args.vg, 15, 85);
            nvgLineTo(args.vg, centerX + 15, 63);
        } else if (trackNumber == 2) {
            // Track 2: Decay knob at (15, 165), CV input at (centerX + 15, 143)
            nvgMoveTo(args.vg, 15, 165);
            nvgLineTo(args.vg, centerX + 15, 143);
        } else if (trackNumber == 3) {
            // Track 3: Decay knob at (15, 245), CV input at (centerX + 15, 223)
            nvgMoveTo(args.vg, 15, 245);
            nvgLineTo(args.vg, centerX + 15, 223);
        }
        
        nvgStroke(args.vg);
    }
};

// HiddenTimeKnobQQ 現在從 widgets/Knobs.hpp 引入

// HiddenAttenuatorKnob 現在從 widgets/Knobs.hpp 引入

struct QQScopeDisplay : LedDisplay {
    QQ* module;
    ModuleWidget* moduleWidget;
    
    QQScopeDisplay() {
        box.size = Vec(60, 51);
    }
    
    void drawWave(const DrawArgs& args, int track, NVGcolor color) {
        if (!module) return;
        
        nvgSave(args.vg);
        
        // Calculate track area (31% height each, with gaps)
        float trackHeight = box.size.y * 0.31f;
        float gap = (box.size.y - 3 * trackHeight) / 2; // Space between tracks
        float trackY = track * (trackHeight + gap);
        
        Rect b = Rect(Vec(0, trackY), Vec(box.size.x, trackHeight));
        nvgScissor(args.vg, RECT_ARGS(b));
        nvgBeginPath(args.vg);
        
        for (int i = 0; i < QQ::SCOPE_BUFFER_SIZE; i++) {
            float value = module->scopeBuffer[track][(i + module->scopeBufferIndex) % QQ::SCOPE_BUFFER_SIZE].value;
            value = clamp(value, 0.f, 10.f);
            
            Vec p;
            p.x = (float)i / (QQ::SCOPE_BUFFER_SIZE - 1);
            p.y = 1.f - (value / 10.f); // Invert for proper display
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
        float trackHeight = box.size.y * 0.31f;
        float gap = (box.size.y - 3 * trackHeight) / 2;
        
        for (int i = 0; i < 3; i++) {
            float trackY = i * (trackHeight + gap);
            
            // Top line of each track area
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY);
            nvgLineTo(args.vg, box.size.x, trackY);
            nvgStroke(args.vg);
            
            // Bottom line of each track area
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY + trackHeight);
            nvgLineTo(args.vg, box.size.x, trackY + trackHeight);
            nvgStroke(args.vg);
            
            // Center line of each track (for reference)
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, trackY + trackHeight / 2);
            nvgLineTo(args.vg, box.size.x, trackY + trackHeight / 2);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 15));
            nvgStroke(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 30));
        }
        
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
        
        // Get input colors from cable connections
        for (int i = 0; i < 3; i++) {
            PortWidget* inputPort = moduleWidget->getInput(QQ::TRACK1_TRIG_INPUT + i);
            CableWidget* cable = APP->scene->rack->getTopCable(inputPort);
            NVGcolor trackColor = cable ? cable->color : nvgRGB(255, 255, 255);
            
            drawWave(args, i, trackColor);
        }
    }
};

struct QQWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    QQWidget(QQ* module) {
        setModule(module);
        panelThemeHelper.init(this, "SwingLFO");

        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Q_Q", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        float centerX = box.size.x / 2;
        
        // Track 1 (unchanged)
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 45), module, QQ::TRACK1_TRIG_INPUT));
        addChild(new EnhancedTextLabel(Vec(35, 35), Vec(20, 20), "T1", 8.f, nvgRGB(255, 255, 255), true));
        
        // Add connection line BEFORE adding knobs and inputs (so it appears underneath)
        addChild(new CVConnectionLine(Vec(0, 0), Vec(box.size.x, 120), 1));
        
        addChild(new EnhancedTextLabel(Vec(5, 55), Vec(20, 20), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(15, 85), module, QQ::TRACK1_DECAY_TIME_PARAM));
        
        // Track 1 Decay CV input (moved up 2px)
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 63), module, QQ::TRACK1_DECAY_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 70), Vec(20, 20), "SHAPE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(45, 100), module, QQ::TRACK1_SHAPE_PARAM));
        
        // Hidden attenuator knob (positioned below CV input, not overlapping)
        addParam(createParam<HiddenAttenuatorKnob>(Vec(centerX + 15 - 12, 65), module, QQ::TRACK1_DECAY_CV_ATTEN_PARAM));
        
        // Track 2 (moved up by 10px total)
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 125), module, QQ::TRACK2_TRIG_INPUT));
        addChild(new EnhancedTextLabel(Vec(35, 115), Vec(20, 20), "T2", 8.f, nvgRGB(255, 255, 255), true));
        
        // Track 2 connection line
        addChild(new CVConnectionLine(Vec(0, 0), Vec(box.size.x, 200), 2));
        
        addChild(new EnhancedTextLabel(Vec(5, 135), Vec(20, 20), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(15, 165), module, QQ::TRACK2_DECAY_TIME_PARAM));
        
        // Track 2 Decay CV input
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 143), module, QQ::TRACK2_DECAY_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 150), Vec(20, 20), "SHAPE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(45, 180), module, QQ::TRACK2_SHAPE_PARAM));
        
        // Track 2 Hidden attenuator knob
        addParam(createParam<HiddenAttenuatorKnob>(Vec(centerX + 15 - 12, 145), module, QQ::TRACK2_DECAY_CV_ATTEN_PARAM));
        
        // Track 3 (moved up by 20px total)
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 205), module, QQ::TRACK3_TRIG_INPUT));
        addChild(new EnhancedTextLabel(Vec(35, 195), Vec(20, 20), "T3", 8.f, nvgRGB(255, 255, 255), true));
        
        // Track 3 connection line
        addChild(new CVConnectionLine(Vec(0, 0), Vec(box.size.x, 280), 3));
        
        addChild(new EnhancedTextLabel(Vec(5, 215), Vec(20, 20), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(15, 245), module, QQ::TRACK3_DECAY_TIME_PARAM));
        
        // Track 3 Decay CV input
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 223), module, QQ::TRACK3_DECAY_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 230), Vec(20, 20), "SHAPE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(45, 260), module, QQ::TRACK3_SHAPE_PARAM));
        
        // Track 3 Hidden attenuator knob
        addParam(createParam<HiddenAttenuatorKnob>(Vec(centerX + 15 - 12, 225), module, QQ::TRACK3_DECAY_CV_ATTEN_PARAM));
        
        // Scope display
        QQScopeDisplay* scopeDisplay = new QQScopeDisplay();
        scopeDisplay->box.pos = Vec(0, 279);
        scopeDisplay->module = module;
        scopeDisplay->moduleWidget = this;
        addChild(scopeDisplay);
        
        // Hidden time control knob (overlapping scope display)
        addParam(createParam<HiddenTimeKnobQQ>(Vec(0, 279), module, QQ::SCOPE_TIME_PARAM));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(60, 50)));
        
        addChild(new EnhancedTextLabel(Vec(5, 335), Vec(20, 20), "QUTQ", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 343), module, QQ::TRACK1_ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 368), module, QQ::TRACK2_ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 368), module, QQ::TRACK3_ENV_OUTPUT));
    }

    void step() override {
        QQ* module = dynamic_cast<QQ*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        QQ* module = dynamic_cast<QQ*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Attack Time"));

        // Attack Time Slider for Track 1
        struct AttackTimeSlider1 : ui::Slider {
            struct AttackTimeQuantity : Quantity {
                QQ* module;
                int trackIndex;
                AttackTimeQuantity(QQ* module, int trackIndex) : module(module), trackIndex(trackIndex) {}

                void setValue(float value) override {
                    if (module) {
                        value = clamp(value, 0.0f, 1.0f);
                        float attackTime = rescale(value, 0.0f, 1.0f, 0.0005f, 0.020f);
                        module->tracks[trackIndex].attackTime = attackTime;
                    }
                }

                float getValue() override {
                    if (module) {
                        return rescale(module->tracks[trackIndex].attackTime, 0.0005f, 0.020f, 0.0f, 1.0f);
                    }
                    return 0.1f;
                }

                float getDefaultValue() override { return rescale(0.001f, 0.0005f, 0.020f, 0.0f, 1.0f); }
                float getMinValue() override { return 0.0f; }
                float getMaxValue() override { return 1.0f; }
                std::string getLabel() override { return "Track " + std::to_string(trackIndex + 1) + " Attack"; }
                std::string getUnit() override { return " ms"; }
                std::string getDisplayValueString() override {
                    if (module) {
                        return string::f("%.2f", module->tracks[trackIndex].attackTime * 1000.0f);
                    }
                    return "1.00";
                }
            };

            AttackTimeSlider1(QQ* module, int trackIndex) {
                box.size.x = 200.0f;
                quantity = new AttackTimeQuantity(module, trackIndex);
            }

            ~AttackTimeSlider1() {
                delete quantity;
            }
        };

        // Track 1
        menu->addChild(createMenuLabel("Track 1"));
        AttackTimeSlider1* slider1 = new AttackTimeSlider1(module, 0);
        menu->addChild(slider1);

        // Track 2
        menu->addChild(createMenuLabel("Track 2"));
        AttackTimeSlider1* slider2 = new AttackTimeSlider1(module, 1);
        menu->addChild(slider2);

        // Track 3
        menu->addChild(createMenuLabel("Track 3"));
        AttackTimeSlider1* slider3 = new AttackTimeSlider1(module, 2);
        menu->addChild(slider3);

        // Retrigger option
        menu->addChild(new MenuSeparator);
        struct RetriggerItem : ui::MenuItem {
            QQ* module;
            void onAction(const event::Action& e) override {
                if (module) {
                    module->retriggerEnabled = !module->retriggerEnabled;
                }
            }
        };
        RetriggerItem* retriggerItem = new RetriggerItem;
        retriggerItem->text = "Retrigger";
        retriggerItem->rightText = CHECKMARK(module->retriggerEnabled);
        retriggerItem->module = module;
        menu->addChild(retriggerItem);
    }
};

Model* modelQQ = createModel<QQ, QQWidget>("QQ");