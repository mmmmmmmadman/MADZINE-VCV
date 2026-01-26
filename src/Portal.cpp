#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

// ============================================================================
// IsolatorParamQuantity (from UniRhythm)
// ============================================================================

struct IsolatorParamQuantity : ParamQuantity {
    float getDisplayValue() override {
        return getValue();  // Return raw value, we handle display in getString()
    }

    std::string getString() override {
        float p = getValue();
        float gain;
        if (p < 0) {
            // Cut side: quadratic
            float t = 1.0f + p;
            gain = t * t;
        } else {
            // Boost side: linear to +12dB (gain 4.0)
            gain = 1.0f + p * 3.0f;
        }

        std::string s = getLabel();
        s += ": ";
        if (gain < 0.001f) {
            s += "Kill";
        } else {
            float dB = 20.0f * std::log10(gain);
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f dB", dB);
            s += buf;
        }
        return s;
    }
};

// ============================================================================
// ThreeBandIsolator (from UniRhythm)
// ============================================================================

class ThreeBandIsolator {
private:
    float sampleRate = 44100.0f;

    // Biquad coefficients for 2nd order Butterworth (cascaded for LR4)
    struct Biquad {
        float a0, a1, a2, b1, b2;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        void reset() { x1 = x2 = y1 = y2 = 0; }

        float process(float in) {
            float out = a0 * in + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    // Low pass for low band (250Hz), 2 stages for LR4
    Biquad lpLow1[2], lpLow2[2];  // L/R
    // High pass for low band (250Hz), 2 stages for LR4
    Biquad hpLow1[2], hpLow2[2];  // L/R
    // Low pass for high band (4kHz), 2 stages for LR4
    Biquad lpHigh1[2], lpHigh2[2];  // L/R
    // High pass for high band (4kHz), 2 stages for LR4
    Biquad hpHigh1[2], hpHigh2[2];  // L/R

    void calcButterworth2LP(Biquad& bq, float fc) {
        float w0 = 2.0f * M_PI * fc / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / std::sqrt(2.0f);
        float norm = 1.0f / (1.0f + alpha);
        bq.a0 = (1.0f - cosw0) * 0.5f * norm;
        bq.a1 = (1.0f - cosw0) * norm;
        bq.a2 = bq.a0;
        bq.b1 = -2.0f * cosw0 * norm;
        bq.b2 = (1.0f - alpha) * norm;
    }

    void calcButterworth2HP(Biquad& bq, float fc) {
        float w0 = 2.0f * M_PI * fc / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / std::sqrt(2.0f);
        float norm = 1.0f / (1.0f + alpha);
        bq.a0 = (1.0f + cosw0) * 0.5f * norm;
        bq.a1 = -(1.0f + cosw0) * norm;
        bq.a2 = bq.a0;
        bq.b1 = -2.0f * cosw0 * norm;
        bq.b2 = (1.0f - alpha) * norm;
    }

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        // Crossover frequencies: 250Hz and 4kHz
        for (int ch = 0; ch < 2; ch++) {
            calcButterworth2LP(lpLow1[ch], 250.0f);
            calcButterworth2LP(lpLow2[ch], 250.0f);
            calcButterworth2HP(hpLow1[ch], 250.0f);
            calcButterworth2HP(hpLow2[ch], 250.0f);
            calcButterworth2LP(lpHigh1[ch], 4000.0f);
            calcButterworth2LP(lpHigh2[ch], 4000.0f);
            calcButterworth2HP(hpHigh1[ch], 4000.0f);
            calcButterworth2HP(hpHigh2[ch], 4000.0f);
        }
        reset();
    }

    void reset() {
        for (int ch = 0; ch < 2; ch++) {
            lpLow1[ch].reset(); lpLow2[ch].reset();
            hpLow1[ch].reset(); hpLow2[ch].reset();
            lpHigh1[ch].reset(); lpHigh2[ch].reset();
            hpHigh1[ch].reset(); hpHigh2[ch].reset();
        }
    }

    // Process stereo with gain parameters (-1 to +1)
    // -1 = cut, 0 = unity, +1 = +6dB boost
    void process(float& left, float& right, float lowParam, float midParam, float highParam) {
        // Convert bipolar param to gain
        // -1.0 -> 0.0 (cut), 0.0 -> 1.0 (unity), +1.0 -> 2.0 (+6dB)
        auto paramToGain = [](float p) {
            if (p < 0) {
                // Cut side: quadratic for musical feel
                float t = 1.0f + p;  // -1->0, 0->1
                return t * t;
            } else {
                // Boost side: linear to +12dB (gain 4.0)
                return 1.0f + p * 3.0f;  // 0->1, 1->4
            }
        };

        float gainLow = paramToGain(lowParam);
        float gainMid = paramToGain(midParam);
        float gainHigh = paramToGain(highParam);

        // Process each channel
        float inputs[2] = {left, right};
        float outputs[2];

        for (int ch = 0; ch < 2; ch++) {
            float in = inputs[ch];

            // Low band: cascaded LP at 250Hz
            float low = lpLow2[ch].process(lpLow1[ch].process(in));

            // High band: cascaded HP at 4kHz
            float high = hpHigh2[ch].process(hpHigh1[ch].process(in));

            // Mid band: HP at 250Hz then LP at 4kHz (cascaded)
            float midTemp = hpLow2[ch].process(hpLow1[ch].process(in));
            float mid = lpHigh2[ch].process(lpHigh1[ch].process(midTemp));

            // Sum with gains
            outputs[ch] = low * gainLow + mid * gainMid + high * gainHigh;
        }

        left = outputs[0];
        right = outputs[1];
    }
};

// ============================================================================
// TubeDrive (from UniRhythm)
// ============================================================================

class TubeDrive {
private:
    float sampleRate = 44100.0f;
    // DC blocker state (1st order HP ~10Hz)
    float dcBlockerL = 0, dcBlockerR = 0;
    float dcCoeff = 0.999f;

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        // DC blocker coefficient for ~10Hz cutoff
        float fc = 10.0f;
        dcCoeff = 1.0f - (2.0f * M_PI * fc / sr);
        if (dcCoeff < 0.9f) dcCoeff = 0.9f;
        if (dcCoeff > 0.9999f) dcCoeff = 0.9999f;
    }

    void reset() {
        dcBlockerL = dcBlockerR = 0;
    }

    // Process stereo with drive amount (0 to 1)
    void process(float& left, float& right, float driveAmount) {
        if (driveAmount < 0.01f) return;  // Bypass if no drive

        // Asymmetric waveshaping for even harmonics (tube-like)
        auto tubeShape = [](float x, float drive) {
            // Scale input
            float scaled = x * (1.0f + drive * 2.0f);
            // Asymmetric saturation
            if (scaled >= 0) {
                // Soft saturation on positive
                return std::tanh(scaled * 0.8f);
            } else {
                // Harder saturation on negative (creates even harmonics)
                return std::tanh(scaled * 1.0f);
            }
        };

        // Apply drive with makeup gain
        float makeupGain = 1.0f / (1.0f + driveAmount * 0.5f);
        left = tubeShape(left, driveAmount) * makeupGain;
        right = tubeShape(right, driveAmount) * makeupGain;

        // DC blocker (1st order HP)
        float prevL = dcBlockerL;
        float prevR = dcBlockerR;
        dcBlockerL = left - prevL + dcCoeff * dcBlockerL;
        dcBlockerR = right - prevR + dcCoeff * dcBlockerR;
        left = dcBlockerL;
        right = dcBlockerR;
    }
};

// ============================================================================
// Portal - UniRhythm Crossfader (8HP)
// Poly crossfader for two UniRhythm modules with DJ-style cue monitoring
// F4 Layout: Left controls, Right CV outputs
// ============================================================================

// Helper text label widget
struct PortalTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    PortalTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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

        if (bold) {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
            nvgStrokeColor(args.vg, color);
            nvgStrokeWidth(args.vg, 0.3f);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        } else {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
        }
    }
};

// White background box for output area
struct PortalWhiteBox : Widget {
    PortalWhiteBox(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

// Custom Cue A/B toggle button - displays green "A" or "B" text with button background
struct PortalCueButton : ParamWidget {
    void draw(const DrawArgs& args) override {
        float value = 0.0f;
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            value = pq->getValue();
        }

        bool isB = value > 0.5f;
        const char* text = isB ? "B" : "A";

        float cx = box.size.x / 2.f;
        float cy = box.size.y / 2.f;
        float radius = std::min(cx, cy) - 1.f;

        // Draw button background (dark circle)
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);

        // Draw button border
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        // Draw orange text (MADZINE brand color)
        nvgFontSize(args.vg, 12.f);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, nvgRGB(255, 200, 0));  // Orange (brand color)
        nvgText(args.vg, cx, cy, text, NULL);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            ParamQuantity* pq = getParamQuantity();
            if (pq) {
                float newValue = pq->getValue() > 0.5f ? 0.0f : 1.0f;
                pq->setValue(newValue);
            }
            e.consume(this);
        }
    }
};

// Custom horizontal slider for XFADE - minimalist line design
struct PortalXfadeSlider : ParamWidget {
    void draw(const DrawArgs& args) override {
        float value = 0.5f;
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            value = pq->getValue();
        }

        float width = box.size.x;
        float height = box.size.y;
        float centerY = height / 2.0f;

        // Draw horizontal track line (white)
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, centerY);
        nvgLineTo(args.vg, width, centerY);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        // Draw vertical indicator line (orange)
        float indicatorX = value * width;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, indicatorX, 0);
        nvgLineTo(args.vg, indicatorX, height);
        nvgStrokeColor(args.vg, nvgRGB(255, 200, 0));
        nvgStrokeWidth(args.vg, 4.0f);
        nvgStroke(args.vg);
    }

    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            float delta = e.mouseDelta.x / box.size.x * 0.5f;  // Half sensitivity
            float newValue = pq->getValue() + delta;
            pq->setValue(clamp(newValue, 0.0f, 1.0f));
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
        }
    }
};

// Crossfader curve types
enum CurveType {
    CURVE_LINEAR,
    CURVE_EQUAL_POWER,
    CURVE_CUT
};

struct Portal : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;

    // Isolator and Drive processors (from UniRhythm)
    ThreeBandIsolator isolator;
    TubeDrive tubeDrive;

    enum ParamId {
        XFADER_PARAM,
        CUE_A_PARAM,
        ISO_LOW_PARAM,
        ISO_MID_PARAM,
        ISO_HIGH_PARAM,
        DRIVE_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        DECK_A_INPUT,
        DECK_B_INPUT,
        XFADER_CV_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        // CV outputs for 4 roles × 3 types
        GATE_TL_OUTPUT,
        GATE_FD_OUTPUT,
        GATE_GR_OUTPUT,
        GATE_LD_OUTPUT,
        PITCH_TL_OUTPUT,
        PITCH_FD_OUTPUT,
        PITCH_GR_OUTPUT,
        PITCH_LD_OUTPUT,
        VELENV_TL_OUTPUT,
        VELENV_FD_OUTPUT,
        VELENV_GR_OUTPUT,
        VELENV_LD_OUTPUT,
        // Master Mix stereo
        MASTER_L_OUTPUT,
        MASTER_R_OUTPUT,
        // Cue stereo
        CUE_L_OUTPUT,
        CUE_R_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        CUE_A_LIGHT,
        LIGHTS_LEN
    };

    CurveType curveType = CURVE_EQUAL_POWER;

    Portal() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(XFADER_PARAM, 0.0f, 1.0f, 0.5f, "Crossfader", "%", 0.0f, 100.0f);
        configSwitch(CUE_A_PARAM, 0.0f, 1.0f, 0.0f, "Cue A/B", {"A", "B"});

        // Isolator parameters (bipolar: -1=cut, 0=unity, +1=boost)
        configParam<IsolatorParamQuantity>(ISO_LOW_PARAM, -1.0f, 1.0f, 0.0f, "Isolator Low", " dB");
        configParam<IsolatorParamQuantity>(ISO_MID_PARAM, -1.0f, 1.0f, 0.0f, "Isolator Mid", " dB");
        configParam<IsolatorParamQuantity>(ISO_HIGH_PARAM, -1.0f, 1.0f, 0.0f, "Isolator High", " dB");
        configParam(DRIVE_PARAM, 0.0f, 1.0f, 0.0f, "Master Drive", "%", 0.0f, 100.0f);

        configInput(DECK_A_INPUT, "Deck A (16ch poly)");
        configInput(DECK_B_INPUT, "Deck B (16ch poly)");
        configInput(XFADER_CV_INPUT, "Crossfader CV");

        // CV outputs
        const char* roles[] = {"Timeline", "Foundation", "Groove", "Lead"};
        for (int r = 0; r < 4; r++) {
            configOutput(GATE_TL_OUTPUT + r, std::string(roles[r]) + " Gate");
            configOutput(PITCH_TL_OUTPUT + r, std::string(roles[r]) + " Pitch");
            configOutput(VELENV_TL_OUTPUT + r, std::string(roles[r]) + " VelEnv");
        }

        configOutput(MASTER_L_OUTPUT, "Master L");
        configOutput(MASTER_R_OUTPUT, "Master R");
        configOutput(CUE_L_OUTPUT, "Cue L");
        configOutput(CUE_R_OUTPUT, "Cue R");
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        isolator.setSampleRate(sr);
        tubeDrive.setSampleRate(sr);
    }

    void calculateGains(float pos, float& gainA, float& gainB) {
        switch (curveType) {
            case CURVE_LINEAR:
                gainA = 1.0f - pos;
                gainB = pos;
                break;
            case CURVE_EQUAL_POWER:
                gainA = std::cos(pos * M_PI * 0.5f);
                gainB = std::sin(pos * M_PI * 0.5f);
                break;
            case CURVE_CUT:
                if (pos < 0.4f) {
                    gainA = 1.0f;
                    gainB = pos / 0.4f;
                } else if (pos > 0.6f) {
                    gainA = (1.0f - pos) / 0.4f;
                    gainB = 1.0f;
                } else {
                    float t = (pos - 0.4f) / 0.2f;
                    gainA = 1.0f - t;
                    gainB = t;
                }
                break;
        }
    }

    void process(const ProcessArgs& args) override {
        // Get crossfader position with CV
        float xfaderPos = params[XFADER_PARAM].getValue();
        if (inputs[XFADER_CV_INPUT].isConnected()) {
            xfaderPos += inputs[XFADER_CV_INPUT].getVoltage() * 0.1f;
            xfaderPos = clamp(xfaderPos, 0.0f, 1.0f);
        }

        float gainA, gainB;
        calculateGains(xfaderPos, gainA, gainB);

        // Poly channel layout from UniRhythm:
        // [TL:0-3] [FD:4-7] [GR:8-11] [LD:12-15]
        // Each role: Audio(0), Gate(1), Pitch(2), VelEnv(3)

        // Stereo panning for Master Mix
        const float rolePan[4] = {0.2f, 0.0f, -0.3f, -0.4f};  // TL, FD, GR, LD

        float masterL = 0.0f;
        float masterR = 0.0f;

        for (int role = 0; role < 4; role++) {
            int baseChannel = role * 4;

            // Audio crossfade for master mix
            float audioA = inputs[DECK_A_INPUT].getVoltage(baseChannel + 0);
            float audioB = inputs[DECK_B_INPUT].getVoltage(baseChannel + 0);
            float audio = audioA * gainA + audioB * gainB;

            // Apply panning to master mix
            float pan = rolePan[role];
            masterL += audio * (1.0f - pan) * 0.5f;
            masterR += audio * (1.0f + pan) * 0.5f;

            // CV outputs (crossfaded)
            float gateA = inputs[DECK_A_INPUT].getVoltage(baseChannel + 1);
            float gateB = inputs[DECK_B_INPUT].getVoltage(baseChannel + 1);
            outputs[GATE_TL_OUTPUT + role].setVoltage(gateA * gainA + gateB * gainB);

            float pitchA = inputs[DECK_A_INPUT].getVoltage(baseChannel + 2);
            float pitchB = inputs[DECK_B_INPUT].getVoltage(baseChannel + 2);
            outputs[PITCH_TL_OUTPUT + role].setVoltage(pitchA * gainA + pitchB * gainB);

            float velenvA = inputs[DECK_A_INPUT].getVoltage(baseChannel + 3);
            float velenvB = inputs[DECK_B_INPUT].getVoltage(baseChannel + 3);
            outputs[VELENV_TL_OUTPUT + role].setVoltage(velenvA * gainA + velenvB * gainB);
        }

        // Apply Master Isolator (three-band EQ)
        float isoLow = params[ISO_LOW_PARAM].getValue();
        float isoMid = params[ISO_MID_PARAM].getValue();
        float isoHigh = params[ISO_HIGH_PARAM].getValue();
        isolator.process(masterL, masterR, isoLow, isoMid, isoHigh);

        // Apply Master Drive (tube saturation)
        float driveAmount = params[DRIVE_PARAM].getValue();
        tubeDrive.process(masterL, masterR, driveAmount);

        // Master output with soft clip
        outputs[MASTER_L_OUTPUT].setVoltage(std::tanh(masterL) * 5.0f);
        outputs[MASTER_R_OUTPUT].setVoltage(std::tanh(masterR) * 5.0f);

        // Cue processing - single toggle button: 0=A, 1=B
        bool cueB = params[CUE_A_PARAM].getValue() > 0.5f;
        lights[CUE_A_LIGHT].setBrightness(cueB ? 1.0f : 0.0f);

        float cueL = 0.0f;
        float cueR = 0.0f;

        for (int role = 0; role < 4; role++) {
            int audioCh = role * 4;
            float audioA = inputs[DECK_A_INPUT].getVoltage(audioCh);
            float audioB = inputs[DECK_B_INPUT].getVoltage(audioCh);

            float cueAudio = cueB ? audioB : audioA;

            float pan = rolePan[role];
            cueL += cueAudio * (1.0f - pan) * 0.5f;
            cueR += cueAudio * (1.0f + pan) * 0.5f;
        }

        outputs[CUE_L_OUTPUT].setVoltage(std::tanh(cueL) * 5.0f);
        outputs[CUE_R_OUTPUT].setVoltage(std::tanh(cueR) * 5.0f);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_object_set_new(rootJ, "curveType", json_integer(static_cast<int>(curveType)));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_number_value(contrastJ);

        json_t* curveJ = json_object_get(rootJ, "curveType");
        if (curveJ) curveType = static_cast<CurveType>(json_integer_value(curveJ));
    }
};

// ============================================================================
// Widget (8HP) - F4 Layout: 左側控制、右側 CV 輸出
// ============================================================================

struct PortalWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    PortalWidget(Portal* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // 8HP = 121.92px
        NVGcolor white = nvgRGB(255, 255, 255);
        NVGcolor orange = nvgRGB(255, 200, 0);
        NVGcolor pink = nvgRGB(255, 133, 133);
        NVGcolor black = nvgRGB(0, 0, 0);

        // ===== 標題區域（Y=0-30）=====
        addChild(new PortalTextLabel(Vec(0, 1), Vec(box.size.x, 20), "PORTAL", 14.f, orange, true));
        addChild(new PortalTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, orange, false));

        // ===== CV Out 區域（2列×6行，對齊 CV in X 座標）=====
        // X 座標（對齊 CV in: 23, 61, 99）
        float rowLabelX = 23.0f;  // 行標籤中心 X（對齊 CV in 第一列）
        float cvCol1X = 61.0f;   // Timeline / Foundation 端口（對齊 CV in 第二列）
        float cvCol2X = 99.0f;   // Groove / Lead 端口（對齊 CV in 第三列）

        // ----- 上半組（Timeline + Groove）-----
        // 組標籤：Timeline (中心對齊 X=23), Groove (中心對齊 X=61)
        addChild(new PortalTextLabel(Vec(cvCol1X - 20, 39), Vec(40, 15), "Timeline", 8.f, white, true));
        addChild(new PortalTextLabel(Vec(cvCol2X - 15, 39), Vec(30, 15), "Groove", 8.f, white, true));

        // Gate 行：端口 Y=65
        addChild(new PortalTextLabel(Vec(rowLabelX - 12.5, 57.5), Vec(25, 15), "Gate", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 65), module, Portal::GATE_TL_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 65), module, Portal::GATE_GR_OUTPUT));

        // Pitch 行：端口 Y=91
        addChild(new PortalTextLabel(Vec(rowLabelX - 12.5, 83.5), Vec(25, 15), "Pitch", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 91), module, Portal::PITCH_TL_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 91), module, Portal::PITCH_GR_OUTPUT));

        // Env 行：端口 Y=117
        addChild(new PortalTextLabel(Vec(rowLabelX - 20, 109.5), Vec(40, 15), "Vel / Env", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 117), module, Portal::VELENV_TL_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 117), module, Portal::VELENV_GR_OUTPUT));

        // ----- 下半組（Foundation + Lead）-----
        // 組標籤：Foundation (中心對齊 X=23), Lead (中心對齊 X=61)
        addChild(new PortalTextLabel(Vec(cvCol1X - 23, 134), Vec(46, 15), "Foundation", 8.f, white, true));
        addChild(new PortalTextLabel(Vec(cvCol2X - 15, 134), Vec(30, 15), "Lead", 8.f, white, true));

        // Gate 行：端口 Y=160
        addChild(new PortalTextLabel(Vec(rowLabelX - 12.5, 152.5), Vec(25, 15), "Gate", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 160), module, Portal::GATE_FD_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 160), module, Portal::GATE_LD_OUTPUT));

        // Pitch 行：端口 Y=186
        addChild(new PortalTextLabel(Vec(rowLabelX - 12.5, 178.5), Vec(25, 15), "Pitch", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 186), module, Portal::PITCH_FD_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 186), module, Portal::PITCH_LD_OUTPUT));

        // Env 行：端口 Y=212
        addChild(new PortalTextLabel(Vec(rowLabelX - 20, 204.5), Vec(40, 15), "Vel / Env", 8.f, white, true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol1X, 212), module, Portal::VELENV_FD_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(cvCol2X, 212), module, Portal::VELENV_LD_OUTPUT));

        // ===== Input 區域（Y=228-258，橫向排列：Poly in A, Cross CV, Poly in B）=====
        // 提示文字（橘色）
        addChild(new PortalTextLabel(Vec(0, 228), Vec(box.size.x, 10), "From UniRhythm Poly Out", 6.f, orange, false));

        // 端口 Y=258, 標籤偏移 24px → 標籤 Y=234
        float inputY = 258.0f;
        float inputLabelY = 234.0f;

        // Poly in A (X=23, Y=258)
        addChild(new PortalTextLabel(Vec(0, inputLabelY), Vec(46, 15), "Poly in A", 8.f, white, true));
        addInput(createInputCentered<PJ301MPort>(Vec(23, inputY), module, Portal::DECK_A_INPUT));

        // Cross CV (X=61, Y=258) - 標籤改為 Cross CV
        addChild(new PortalTextLabel(Vec(42, inputLabelY), Vec(38, 15), "Cross CV", 8.f, white, true));
        addInput(createInputCentered<PJ301MPort>(Vec(61, inputY), module, Portal::XFADER_CV_INPUT));

        // Poly in B (X=99, Y=258)
        addChild(new PortalTextLabel(Vec(76, inputLabelY), Vec(46, 15), "Poly in B", 8.f, white, true));
        addInput(createInputCentered<PJ301MPort>(Vec(99, inputY), module, Portal::DECK_B_INPUT));

        // ===== 旋鈕區域（Y=268-294，4 個旋鈕橫排）=====
        // 參考 EuclideanRhythm.cpp 的 X 座標：13, 44, 75, 105
        // 26px 旋鈕，標籤偏移 24px
        float knobY = 294.0f;
        float knobLabelY = 270.0f;
        float knobX[4] = {13.0f, 44.0f, 75.0f, 106.0f};

        // LOW
        addChild(new PortalTextLabel(Vec(knobX[0] - 12, knobLabelY), Vec(24, 15), "LOW", 8.f, white, true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(knobX[0], knobY), module, Portal::ISO_LOW_PARAM));

        // MID
        addChild(new PortalTextLabel(Vec(knobX[1] - 12, knobLabelY), Vec(24, 15), "MID", 8.f, white, true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(knobX[1], knobY), module, Portal::ISO_MID_PARAM));

        // HIGH
        addChild(new PortalTextLabel(Vec(knobX[2] - 12, knobLabelY), Vec(24, 15), "HIGH", 8.f, white, true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(knobX[2], knobY), module, Portal::ISO_HIGH_PARAM));

        // DRIVE
        addChild(new PortalTextLabel(Vec(knobX[3] - 12, knobLabelY), Vec(24, 15), "DRIVE", 8.f, white, true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(knobX[3], knobY), module, Portal::DRIVE_PARAM));

        // ===== XFADE 區域（Y=298-323）=====

        // 水平滑桿 (Y=310, 寬度 95px, 高度 20px, 置中)
        float sliderWidth = 95.0f;
        float sliderHeight = 20.0f;
        float sliderX = (box.size.x - sliderWidth) / 2.0f;
        float sliderY = 306.0f;

        PortalXfadeSlider* slider = new PortalXfadeSlider();
        slider->box.pos = Vec(sliderX, sliderY);
        slider->box.size = Vec(sliderWidth, sliderHeight);
        slider->module = module;
        slider->paramId = Portal::XFADER_PARAM;
        addParam(slider);

        // A 和 B 標籤在滑桿兩側 (Y=308.5, 與滑桿中心對齊)
        addChild(new PortalTextLabel(Vec(sliderX - 12, 308.5), Vec(10, 15), "A", 8.f, white, true));
        addChild(new PortalTextLabel(Vec(sliderX + sliderWidth + 2, 308.5), Vec(10, 15), "B", 8.f, white, true));

        // ===== 白色區域（Y=330-380）- 保持原佈局 =====
        addChild(new PortalWhiteBox(Vec(0, 330), Vec(box.size.x, 50)));

        float whiteCol1X = 15.0f;
        float whiteCol2X = 42.0f;
        float whiteCol3X = 72.0f;
        float whiteCol4X = 102.0f;
        float row1Y = 343.0f;
        float row2Y = 368.0f;

        // 左排（X=15）：Cue 標籤、Out 標籤、按鈕
        addChild(new PortalTextLabel(Vec(whiteCol1X - 10, 333), Vec(20, 15), "Cue", 7.f, pink, true));
        addChild(new PortalTextLabel(Vec(whiteCol1X - 10, 343), Vec(20, 15), "Out", 7.f, pink, true));
        PortalCueButton* cueButton = new PortalCueButton();
        cueButton->box.pos = Vec(whiteCol1X - 10, 355);
        cueButton->box.size = Vec(20, 20);
        cueButton->module = module;
        cueButton->paramId = Portal::CUE_A_PARAM;
        addParam(cueButton);

        // 中左排（X=42）：Cue Out L/R
        addOutput(createOutputCentered<PJ301MPort>(Vec(whiteCol2X, row1Y), module, Portal::CUE_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(whiteCol2X, row2Y), module, Portal::CUE_R_OUTPUT));

        // 中右排（X=72）：Master 標籤、Output 標籤
        addChild(new PortalTextLabel(Vec(whiteCol3X - 17.5, 341.5), Vec(35, 15), "Master", 7.f, pink, true));
        addChild(new PortalTextLabel(Vec(whiteCol3X - 17.5, 353.5), Vec(35, 15), "Output", 7.f, pink, true));

        // 右排（X=102）：Master L/R
        addOutput(createOutputCentered<PJ301MPort>(Vec(whiteCol4X, row1Y), module, Portal::MASTER_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(whiteCol4X, row2Y), module, Portal::MASTER_R_OUTPUT));
    }

    void step() override {
        Portal* module = dynamic_cast<Portal*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Portal* module = dynamic_cast<Portal*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("Crossfader Curve", "",
            [=](Menu* menu) {
                menu->addChild(createCheckMenuItem("Linear", "",
                    [=]() { return module->curveType == CURVE_LINEAR; },
                    [=]() { module->curveType = CURVE_LINEAR; }
                ));
                menu->addChild(createCheckMenuItem("Equal Power (Recommended)", "",
                    [=]() { return module->curveType == CURVE_EQUAL_POWER; },
                    [=]() { module->curveType = CURVE_EQUAL_POWER; }
                ));
                menu->addChild(createCheckMenuItem("Cut", "",
                    [=]() { return module->curveType == CURVE_CUT; },
                    [=]() { module->curveType = CURVE_CUT; }
                ));
            }
        ));

        addPanelThemeMenu(menu, module);
    }
};

Model* modelPortal = createModel<Portal, PortalWidget>("Portal");
