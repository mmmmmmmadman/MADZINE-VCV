#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <cmath>

// Enhanced text label (same as other modules)
struct EnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    EnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
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
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

// White background box
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

// AD Envelope class (based on ADGenerator)
struct ADEnvelope {
    enum Phase {
        IDLE,
        ATTACK,
        DECAY
    };

    Phase phase = IDLE;
    float triggerOutput = 0.0f;
    float followerOutput = 0.0f;
    float attackTime = 0.01f;
    float decayTime = 1.0f;
    float phaseTime = 0.0f;
    float curve = -0.9f;  // Default shape
    float followerState = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    dsp::SchmittTrigger trigger;

    void reset() {
        phase = IDLE;
        triggerOutput = 0.0f;
        followerOutput = 0.0f;
        phaseTime = 0.0f;
        followerState = 0.0f;
    }

    float applyCurve(float x, float curvature) {
        x = clamp(x, 0.0f, 1.0f);

        if (curvature == 0.0f) {
            return x;
        }

        float k = curvature;
        float abs_x = std::abs(x);
        float denominator = k - 2.0f * k * abs_x + 1.0f;

        if (std::abs(denominator) < 1e-6f) {
            return x;
        }

        return (x - k * x) / denominator;
    }

    float processEnvelopeFollower(float triggerVoltage, float sampleTime, float attackTime, float releaseTime, float curve) {
        attackCoeff = 1.0f - std::exp(-sampleTime / std::max(0.0005f, attackTime * 0.1f));
        releaseCoeff = 1.0f - std::exp(-sampleTime / std::max(0.001f, releaseTime * 0.5f));

        attackCoeff = clamp(attackCoeff, 0.0f, 1.0f);
        releaseCoeff = clamp(releaseCoeff, 0.0f, 1.0f);

        float rectified = std::abs(triggerVoltage) / 10.0f;
        rectified = clamp(rectified, 0.0f, 1.0f);

        float targetCoeff;
        if (rectified > followerState) {
            float progress = attackCoeff;
            targetCoeff = applyCurve(progress, curve);
        } else {
            float progress = releaseCoeff;
            targetCoeff = applyCurve(progress, curve);
        }

        targetCoeff = clamp(targetCoeff, 0.0f, 1.0f);

        followerState += (rectified - followerState) * targetCoeff;
        followerState = clamp(followerState, 0.0f, 1.0f);

        return followerState;
    }

    float processTriggerEnvelope(float triggerVoltage, float sampleTime, float attack, float decay, float curve) {
        // Trigger on rising edge (SchmittTrigger uses 0.1V low / 2.0V high thresholds)
        if (trigger.process(triggerVoltage)) {
            phase = ATTACK;
            phaseTime = 0.0f;
        }

        switch (phase) {
            case IDLE:
                triggerOutput = 0.0f;
                break;

            case ATTACK:
                phaseTime += sampleTime;
                if (phaseTime >= attack) {
                    phase = DECAY;
                    phaseTime = 0.0f;
                    triggerOutput = 1.0f;
                } else {
                    float t = phaseTime / attack;
                    triggerOutput = applyCurve(t, curve);
                }
                break;

            case DECAY:
                phaseTime += sampleTime;
                if (phaseTime >= decay) {
                    triggerOutput = 0.0f;
                    phase = IDLE;
                    phaseTime = 0.0f;
                } else {
                    float t = phaseTime / decay;
                    triggerOutput = 1.0f - applyCurve(t, curve);
                }
                break;
        }

        return clamp(triggerOutput, 0.0f, 1.0f);
    }

    float process(float sampleTime, float triggerVoltage, float attack, float decay) {
        float attackTime = std::pow(10.0f, (attack - 0.5f) * 6.0f);
        float decayTime = std::pow(10.0f, (decay - 0.5f) * 6.0f);

        attackTime = std::max(0.001f, attackTime);
        decayTime = std::max(0.001f, decayTime);

        // Only use trigger envelope - gate voltage amplitude should NOT affect envelope output
        float triggerEnv = processTriggerEnvelope(triggerVoltage, sampleTime, attackTime, decayTime, curve);

        return triggerEnv;
    }
};


struct EnvVCA6 : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamId {
        // Channel 1
        CH1_ATTACK_PARAM,
        CH1_RELEASE_PARAM,
        CH1_OUT_VOL_PARAM,
        CH1_GATE_TRIG_PARAM,
        CH1_SUM_LATCH_PARAM,
        // Channel 2
        CH2_ATTACK_PARAM,
        CH2_RELEASE_PARAM,
        CH2_OUT_VOL_PARAM,
        CH2_GATE_TRIG_PARAM,
        CH2_SUM_LATCH_PARAM,
        // Channel 3
        CH3_ATTACK_PARAM,
        CH3_RELEASE_PARAM,
        CH3_OUT_VOL_PARAM,
        CH3_GATE_TRIG_PARAM,
        CH3_SUM_LATCH_PARAM,
        // Channel 4
        CH4_ATTACK_PARAM,
        CH4_RELEASE_PARAM,
        CH4_OUT_VOL_PARAM,
        CH4_GATE_TRIG_PARAM,
        CH4_SUM_LATCH_PARAM,
        // Channel 5
        CH5_ATTACK_PARAM,
        CH5_RELEASE_PARAM,
        CH5_OUT_VOL_PARAM,
        CH5_GATE_TRIG_PARAM,
        CH5_SUM_LATCH_PARAM,
        // Channel 6
        CH6_ATTACK_PARAM,
        CH6_RELEASE_PARAM,
        CH6_OUT_VOL_PARAM,
        CH6_GATE_TRIG_PARAM,
        CH6_SUM_LATCH_PARAM,
        GATE_MODE_PARAM, // 0 = full cycle gate, 1 = end of cycle trigger
        PARAMS_LEN
    };
    enum InputId {
        // Channel 1
        CH1_IN_L_INPUT,
        CH1_IN_R_INPUT,
        CH1_GATE_INPUT,
        CH1_VOL_CTRL_INPUT,
        // Channel 2
        CH2_IN_L_INPUT,
        CH2_IN_R_INPUT,
        CH2_GATE_INPUT,
        CH2_VOL_CTRL_INPUT,
        // Channel 3
        CH3_IN_L_INPUT,
        CH3_IN_R_INPUT,
        CH3_GATE_INPUT,
        CH3_VOL_CTRL_INPUT,
        // Channel 4
        CH4_IN_L_INPUT,
        CH4_IN_R_INPUT,
        CH4_GATE_INPUT,
        CH4_VOL_CTRL_INPUT,
        // Channel 5
        CH5_IN_L_INPUT,
        CH5_IN_R_INPUT,
        CH5_GATE_INPUT,
        CH5_VOL_CTRL_INPUT,
        // Channel 6
        CH6_IN_L_INPUT,
        CH6_IN_R_INPUT,
        CH6_GATE_INPUT,
        CH6_VOL_CTRL_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        // Channel 1
        CH1_GATE_OUTPUT,
        CH1_ENV_OUTPUT,
        CH1_OUT_L_OUTPUT,
        CH1_OUT_R_OUTPUT,
        // Channel 2
        CH2_GATE_OUTPUT,
        CH2_ENV_OUTPUT,
        CH2_OUT_L_OUTPUT,
        CH2_OUT_R_OUTPUT,
        // Channel 3
        CH3_GATE_OUTPUT,
        CH3_ENV_OUTPUT,
        CH3_OUT_L_OUTPUT,
        CH3_OUT_R_OUTPUT,
        // Channel 4
        CH4_GATE_OUTPUT,
        CH4_ENV_OUTPUT,
        CH4_OUT_L_OUTPUT,
        CH4_OUT_R_OUTPUT,
        // Channel 5
        CH5_GATE_OUTPUT,
        CH5_ENV_OUTPUT,
        CH5_OUT_L_OUTPUT,
        CH5_OUT_R_OUTPUT,
        // Channel 6
        CH6_GATE_OUTPUT,
        CH6_ENV_OUTPUT,
        CH6_OUT_L_OUTPUT,
        CH6_OUT_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        CH1_VCA_LIGHT,
        CH2_VCA_LIGHT,
        CH3_VCA_LIGHT,
        CH4_VCA_LIGHT,
        CH5_VCA_LIGHT,
        CH6_VCA_LIGHT,
        LIGHTS_LEN
    };

    ADEnvelope envelopes[6];
    dsp::SchmittTrigger sumLatchTriggers[6]; // Only for sum latch buttons
    bool gateOutputStates[6] = {false}; // Track gate output states
    bool lastEnvelopeActive[6] = {false}; // Track envelope state for end-of-cycle trigger
    dsp::PulseGenerator endOfCyclePulses[6]; // Generate end-of-cycle triggers
    dsp::PulseGenerator startOfCyclePulses[6]; // Generate start-of-cycle triggers
    bool lastGateHigh[6] = {false}; // Track gate input state for start trigger

    EnvVCA6() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters for all 6 channels
        for (int i = 0; i < 6; i++) {
            configParam(CH1_ATTACK_PARAM + i * 5, 0.f, 1.f, 0.1f, string::f("Ch %d Attack", i + 1));
            configParam(CH1_RELEASE_PARAM + i * 5, 0.f, 1.f, 0.5f, string::f("Ch %d Release", i + 1));
            configParam(CH1_OUT_VOL_PARAM + i * 5, 0.f, 1.f, 0.8f, string::f("Ch %d Out Volume", i + 1));
            configParam(CH1_GATE_TRIG_PARAM + i * 5, 0.f, 1.f, 0.f, string::f("Ch %d Manual Gate (Momentary)", i + 1));
            if (i < 5) { // Only CH1-5 have sum buttons
                configParam(CH1_SUM_LATCH_PARAM + i * 5, 0.f, 1.f, 0.f, string::f("Ch %d Sum to Ch6", i + 1));
            } else { // CH6 sum button is disabled/hidden
                configParam(CH1_SUM_LATCH_PARAM + i * 5, 0.f, 1.f, 0.f, "Disabled");
            }

            // Configure inputs
            configInput(CH1_IN_L_INPUT + i * 4, string::f("Ch %d In L", i + 1));
            configInput(CH1_IN_R_INPUT + i * 4, string::f("Ch %d In R", i + 1));
            configInput(CH1_GATE_INPUT + i * 4, string::f("Ch %d Gate", i + 1));
            configInput(CH1_VOL_CTRL_INPUT + i * 4, string::f("Ch %d Vol Ctrl", i + 1));

            // Configure outputs
            configOutput(CH1_GATE_OUTPUT + i * 4, string::f("Ch %d Gate", i + 1));

            if (i == 5) { // CH6 special tooltips
                configOutput(CH1_ENV_OUTPUT + i * 4, "Ch 6 Envelope / Sum Envelope");
                configOutput(CH1_OUT_L_OUTPUT + i * 4, "Ch 6 Out L / Sum L");
                configOutput(CH1_OUT_R_OUTPUT + i * 4, "Ch 6 Out R / Sum R");
            } else {
                configOutput(CH1_ENV_OUTPUT + i * 4, string::f("Ch %d Envelope", i + 1));
                configOutput(CH1_OUT_L_OUTPUT + i * 4, string::f("Ch %d Out L", i + 1));
                configOutput(CH1_OUT_R_OUTPUT + i * 4, string::f("Ch %d Out R", i + 1));
            }

            // Configure lights
            configLight(CH1_VCA_LIGHT + i, string::f("Ch %d VCA Active", i + 1));
        }

        // Hidden parameter for gate output mode (controlled by context menu)
        // 0 = full cycle gate, 1 = end of cycle trigger, 2 = start+end triggers
        configParam(GATE_MODE_PARAM, 0.f, 2.f, 0.f, "Gate Output Mode");
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
        for (int i = 0; i < 6; i++) {
            // Get parameters for this channel
            float attackParam = params[CH1_ATTACK_PARAM + i * 5].getValue();
            float releaseParam = params[CH1_RELEASE_PARAM + i * 5].getValue();
            float outVolParam = params[CH1_OUT_VOL_PARAM + i * 5].getValue();
            bool manualGatePressed = params[CH1_GATE_TRIG_PARAM + i * 5].getValue() > 0.5f;
            bool sumLatchPressed = params[CH1_SUM_LATCH_PARAM + i * 5].getValue() > 0.5f;

            // Get inputs
            float inL = inputs[CH1_IN_L_INPUT + i * 4].getVoltage();
            float inR = inputs[CH1_IN_R_INPUT + i * 4].getVoltage();
            float gateIn = inputs[CH1_GATE_INPUT + i * 4].getVoltage();
            float volCtrl = inputs[CH1_VOL_CTRL_INPUT + i * 4].getVoltage();

            // Mono-to-stereo: if only L input connected, copy to R
            if (!inputs[CH1_IN_R_INPUT + i * 4].isConnected() && inputs[CH1_IN_L_INPUT + i * 4].isConnected()) {
                inR = inL;
            }

            // Manual gate logic: momentary (only while button pressed)
            bool manualGateActive = params[CH1_GATE_TRIG_PARAM + i * 5].getValue() > 0.5f;

            // Combine gate sources (input + momentary manual gate)
            float combinedGate = std::max(gateIn, manualGateActive ? 10.f : 0.f);

            // Process envelope
            float envelopeOutput = envelopes[i].process(args.sampleTime, combinedGate, attackParam, releaseParam);

            // Apply VCA (envelope controls volume)
            float vcaGain = envelopeOutput;

            // Apply volume control CV (0-10V range) - default to 1.0 if not connected
            float volCtrlGain = 1.f;
            if (inputs[CH1_VOL_CTRL_INPUT + i * 4].isConnected()) {
                volCtrlGain = clamp(volCtrl / 10.f, 0.f, 1.f);
            }
            vcaGain *= volCtrlGain;

            // Apply output volume knob
            vcaGain *= outVolParam;

            // Process audio
            float outL = inL * vcaGain;
            float outR = inR * vcaGain;

            // Gate output logic (three modes)
            int gateMode = (int)params[GATE_MODE_PARAM].getValue(); // 0 = full cycle, 1 = end trigger, 2 = start+end
            float gateOutputVoltage = 0.f;

            if (gateMode == 0) {
                // Mode 0: Full cycle gate (gate high during entire envelope)
                if (combinedGate > 1.f) {
                    gateOutputStates[i] = true;
                }
                if (envelopes[i].phase == ADEnvelope::IDLE && envelopeOutput <= 0.001f) {
                    gateOutputStates[i] = false;
                }
                gateOutputVoltage = gateOutputStates[i] ? 10.f : 0.f;
            } else if (gateMode == 1) {
                // Mode 1: End of cycle trigger only
                bool envelopeActive = (envelopeOutput > 0.001f);
                if (lastEnvelopeActive[i] && !envelopeActive) {
                    // Envelope just finished - trigger pulse
                    endOfCyclePulses[i].trigger(1e-3f); // 1ms pulse
                }
                lastEnvelopeActive[i] = envelopeActive;
                gateOutputVoltage = endOfCyclePulses[i].process(args.sampleTime) ? 10.f : 0.f;
            } else {
                // Mode 2: Start + End of cycle triggers
                bool gateHigh = (combinedGate > 1.f);
                bool envelopeActive = (envelopeOutput > 0.001f);

                // Detect rising edge of gate (start of cycle)
                if (gateHigh && !lastGateHigh[i]) {
                    startOfCyclePulses[i].trigger(1e-3f); // 1ms pulse at start
                }
                lastGateHigh[i] = gateHigh;

                // Detect end of envelope (end of cycle)
                if (lastEnvelopeActive[i] && !envelopeActive) {
                    endOfCyclePulses[i].trigger(1e-3f); // 1ms pulse at end
                }
                lastEnvelopeActive[i] = envelopeActive;

                // Output either trigger (start OR end)
                bool startTrigger = startOfCyclePulses[i].process(args.sampleTime);
                bool endTrigger = endOfCyclePulses[i].process(args.sampleTime);
                gateOutputVoltage = (startTrigger || endTrigger) ? 10.f : 0.f;
            }

            // Set outputs
            outputs[CH1_GATE_OUTPUT + i * 4].setVoltage(gateOutputVoltage);
            outputs[CH1_ENV_OUTPUT + i * 4].setVoltage(envelopeOutput * 10.f);
            outputs[CH1_OUT_L_OUTPUT + i * 4].setVoltage(outL);
            outputs[CH1_OUT_R_OUTPUT + i * 4].setVoltage(outR);

            // VCA light shows current VCA level
            lights[CH1_VCA_LIGHT + i].setBrightness(vcaGain);
        }

        // Sum outputs to CH6 (if sum latch is enabled) - ADD to CH6, don't replace
        float sumL = 0.f;
        float sumR = 0.f;
        float sumEnv = 0.f;
        int sumCount = 0;

        for (int i = 0; i < 5; i++) { // Only sum first 5 channels (CH1-CH5)
            bool sumEnabled = params[CH1_SUM_LATCH_PARAM + i * 5].getValue() > 0.5f;
            if (sumEnabled) {
                sumL += outputs[CH1_OUT_L_OUTPUT + i * 4].getVoltage() * 0.3f; // Scale for mixing
                sumR += outputs[CH1_OUT_R_OUTPUT + i * 4].getVoltage() * 0.3f;

                // Sum envelopes with RMS-like scaling to prevent overload
                float envValue = outputs[CH1_ENV_OUTPUT + i * 4].getVoltage() / 10.f; // Convert to 0-1
                sumEnv += envValue * envValue; // Square for RMS
                sumCount++;
            }
        }

        // ADD sum to CH6 outputs (not replace) if any channels are summed
        if (sumCount > 0) {
            // Add summed audio to CH6's own output
            float ch6L = outputs[CH1_OUT_L_OUTPUT + 5 * 4].getVoltage();
            float ch6R = outputs[CH1_OUT_R_OUTPUT + 5 * 4].getVoltage();
            outputs[CH1_OUT_L_OUTPUT + 5 * 4].setVoltage(ch6L + sumL);
            outputs[CH1_OUT_R_OUTPUT + 5 * 4].setVoltage(ch6R + sumR);

            // Add RMS envelope sum to CH6's envelope
            float ch6Env = outputs[CH1_ENV_OUTPUT + 5 * 4].getVoltage();
            float rmsEnv = std::sqrt(sumEnv / sumCount) * 10.f; // Back to 0-10V range
            outputs[CH1_ENV_OUTPUT + 5 * 4].setVoltage(std::max(ch6Env, rmsEnv)); // Use max to preserve CH6 envelope
        }
    }
};

// Clickable light (for Gate/Trig and Sum buttons)
struct EnvVCA6ClickableLight : ParamWidget {
    EnvVCA6* module;

    EnvVCA6ClickableLight() {
        box.size = Vec(10, 10);
    }

    void draw(const DrawArgs& args) override {
        float brightness = 0.f;
        if (module) {
            brightness = module->params[paramId].getValue();
        }

        // Draw light
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, 4);
        nvgFillColor(args.vg, nvgRGB(brightness * 255, brightness * 133, brightness * 133));
        nvgFill(args.vg);

        // Draw border
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, 4);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStrokeWidth(args.vg, 1.0);
        nvgStroke(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
            if (e.action == GLFW_PRESS) {
                // For manual gate: momentary press
                if (paramId % 5 == 3) { // Manual gate params (CH1_GATE_TRIG_PARAM = 3, 8, 13, ...)
                    module->params[paramId].setValue(1.f);
                } else { // For sum latch: toggle
                    float currentValue = module->params[paramId].getValue();
                    module->params[paramId].setValue(currentValue > 0.5f ? 0.f : 1.f);
                }
            } else if (e.action == GLFW_RELEASE) {
                // For manual gate: release
                if (paramId % 5 == 3) {
                    module->params[paramId].setValue(0.f);
                }
            }
        }
        OpaqueWidget::onButton(e);
    }
};

struct EnvVCA6Widget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    EnvVCA6Widget(EnvVCA6* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");
        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Add module name and brand labels
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "E n v  V C A  6", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        addChild(new EnhancedTextLabel(Vec(0, 27), Vec(box.size.x, 12), "Collaborated with offthesky", 10.f, nvgRGB(255, 255, 255), false));

        // White background for bottom section
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        // Layout parameters for 6 channels
        float channelHeight = 48.f;  // Height per channel (增加間距)
        float startY = 53.f;         // Starting Y position (往下移動)

        for (int i = 0; i < 6; i++) {
            float y = startY + i * channelHeight;

            // Input jacks (left side)
            addInput(createInputCentered<PJ301MPort>(Vec(15, y), module, EnvVCA6::CH1_IN_L_INPUT + i * 4));      // IN L
            addInput(createInputCentered<PJ301MPort>(Vec(15, y + 24), module, EnvVCA6::CH1_IN_R_INPUT + i * 4));  // IN R (Y+4 total)
            addInput(createInputCentered<PJ301MPort>(Vec(45, y), module, EnvVCA6::CH1_GATE_INPUT + i * 4));       // Gate In
            addInput(createInputCentered<PJ301MPort>(Vec(45, y + 24), module, EnvVCA6::CH1_VOL_CTRL_INPUT + i * 4)); // Vol Ctrl (Y+4 total)

            // Control knobs (center)
            addParam(createParamCentered<StandardBlackKnob26>(Vec(75, y), module, EnvVCA6::CH1_ATTACK_PARAM + i * 5));    // Attack
            addParam(createParamCentered<StandardBlackKnob26>(Vec(105, y), module, EnvVCA6::CH1_RELEASE_PARAM + i * 5));  // Release
            addParam(createParamCentered<StandardBlackKnob26>(Vec(135, y), module, EnvVCA6::CH1_OUT_VOL_PARAM + i * 5));  // Out Vol

            // Buttons and light (center-right)
            EnvVCA6ClickableLight* gateTrigButton = new EnvVCA6ClickableLight();
            gateTrigButton->box.pos = Vec(70, y + 15);  // X-5 total
            gateTrigButton->module = module;
            gateTrigButton->paramId = EnvVCA6::CH1_GATE_TRIG_PARAM + i * 5;
            addParam(gateTrigButton);

            EnvVCA6ClickableLight* sumLatchButton = new EnvVCA6ClickableLight();
            sumLatchButton->box.pos = Vec(100, y + 15);  // X-5 total
            sumLatchButton->module = module;
            sumLatchButton->paramId = EnvVCA6::CH1_SUM_LATCH_PARAM + i * 5;
            addParam(sumLatchButton);

            addChild(createLightCentered<MediumLight<GreenLight>>(Vec(135, y + 20), module, EnvVCA6::CH1_VCA_LIGHT + i)); // VCA Light (Y+5 total)

            // Add labels below first channel's buttons
            if (i == 0) {
                addChild(new EnhancedTextLabel(Vec(65, y + 25), Vec(20, 10), "Manual", 6.f, nvgRGB(255, 255, 255), true)); // Manual gate label
                addChild(new EnhancedTextLabel(Vec(95, y + 25), Vec(20, 10), "Sum", 6.f, nvgRGB(255, 255, 255), true));    // Sum label (Y-3, bold)
            }

            // Output jacks (right side)
            addOutput(createOutputCentered<PJ301MPort>(Vec(165, y), module, EnvVCA6::CH1_GATE_OUTPUT + i * 4));    // Gate Out
            addOutput(createOutputCentered<PJ301MPort>(Vec(165, y + 24), module, EnvVCA6::CH1_ENV_OUTPUT + i * 4)); // Env Out (Y+4 total)
        }

        // Six channel outputs in white section (Y=330+)
        // Align with main control knobs X positions (75, 105, 135, and add more for 6 channels)
        float outputXPositions[6] = {15, 45, 75, 105, 135, 165};

        for (int i = 0; i < 6; i++) {
            float x = outputXPositions[i];

            // Output L/R (no labels)
            addOutput(createOutputCentered<PJ301MPort>(Vec(x, 343), module, EnvVCA6::CH1_OUT_L_OUTPUT + i * 4));    // Out L
            addOutput(createOutputCentered<PJ301MPort>(Vec(x, 368), module, EnvVCA6::CH1_OUT_R_OUTPUT + i * 4));    // Out R
        }
    }

    void step() override {
        EnvVCA6* module = dynamic_cast<EnvVCA6*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        EnvVCA6* module = dynamic_cast<EnvVCA6*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator);

        // Gate output mode selection
        menu->addChild(createMenuLabel("Gate Output Mode"));

        struct GateModeItem : MenuItem {
            EnvVCA6* module;
            int mode; // 0 = full cycle, 1 = end trigger, 2 = start+end

            void onAction(const event::Action& e) override {
                module->params[EnvVCA6::GATE_MODE_PARAM].setValue((float)mode);
            }

            void step() override {
                int currentMode = (int)module->params[EnvVCA6::GATE_MODE_PARAM].getValue();
                rightText = (currentMode == mode) ? "✓" : "";
                MenuItem::step();
            }
        };

        GateModeItem* fullCycleItem = createMenuItem<GateModeItem>("Full Cycle Gate", "Gate high during entire envelope");
        fullCycleItem->module = module;
        fullCycleItem->mode = 0;
        menu->addChild(fullCycleItem);

        GateModeItem* endTriggerItem = createMenuItem<GateModeItem>("End of Cycle Trigger", "Trigger when envelope finishes");
        endTriggerItem->module = module;
        endTriggerItem->mode = 1;
        menu->addChild(endTriggerItem);

        GateModeItem* startEndItem = createMenuItem<GateModeItem>("Start + End Triggers", "Triggers at both start and end of cycle");
        startEndItem->module = module;
        startEndItem->mode = 2;
        menu->addChild(startEndItem);

        addPanelThemeMenu(menu, module);
    }
};

Model* modelEnvVCA6 = createModel<EnvVCA6, EnvVCA6Widget>("EnvVCA6");