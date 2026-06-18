#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "FFTCompDSP.hpp"
#include <cmath>

// ============================================================================
// EnhancedTextLabel - 共用文字標籤
// ============================================================================
struct FFTCompTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    FFTCompTextLabel(Vec pos, Vec size, std::string text, float fontSize = 10.f,
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

// White bottom panel for Y>=330
struct FFTCompWhiteBottomPanel : TransparentWidget {
    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 330, box.size.x, box.size.y - 330);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

// ============================================================================
// Module
// ============================================================================
struct FFTComp : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;

    enum ParamId {
        PRE_EQ_LO_PARAM,
        PRE_EQ_LMID_PARAM,
        PRE_EQ_HMID_PARAM,
        PRE_EQ_HI_PARAM,
        AMT_LO_PARAM,
        AMT_LMID_PARAM,
        AMT_HMID_PARAM,
        AMT_HI_PARAM,
        ATTACK_PARAM,
        RELEASE_PARAM,
        MIX_PARAM,
        GAIN_PARAM,
        POST_EQ_LO_PARAM,
        POST_EQ_LMID_PARAM,
        POST_EQ_HMID_PARAM,
        POST_EQ_HI_PARAM,
        SC_LPF_PARAM,
        SC_HPF_PARAM,
        OUT_SEL_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        IN_L_INPUT,
        IN_R_INPUT,
        SC_IN_L_INPUT,
        SC_IN_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_L_OUTPUT,
        OUT_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId { LIGHTS_LEN };

    FFTCompDSP dsp;
    float bandFreqs[4] = {180.f, 350.f, 1100.f, 3000.f};

    FFTComp() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Pre EQ gains
        configParam(PRE_EQ_LO_PARAM,   -24.f, 24.f, 0.f, "Pre EQ LO Gain",   " dB");
        configParam(PRE_EQ_LMID_PARAM, -24.f, 24.f, 0.f, "Pre EQ LMID Gain", " dB");
        configParam(PRE_EQ_HMID_PARAM, -24.f, 24.f, 0.f, "Pre EQ HMID Gain", " dB");
        configParam(PRE_EQ_HI_PARAM,   -24.f, 24.f, 0.f, "Pre EQ HI Gain",   " dB");

        // Suppression amounts
        configParam(AMT_LO_PARAM,   0.f, 1.f, 0.f, "Suppression Amount LO");
        configParam(AMT_LMID_PARAM, 0.f, 1.f, 0.f, "Suppression Amount LMID");
        configParam(AMT_HMID_PARAM, 0.f, 1.f, 0.f, "Suppression Amount HMID");
        configParam(AMT_HI_PARAM,   0.f, 1.f, 0.f, "Suppression Amount HI");

        // Envelope / mix / gain
        configParam(ATTACK_PARAM,  0.f, 1.f, 0.2f, "Attack");
        configParam(RELEASE_PARAM, 0.f, 1.f, 0.4f, "Release");
        configParam(MIX_PARAM,     0.f, 1.f, 1.0f, "Mix");
        configParam(GAIN_PARAM, -12.f, 12.f, 0.f, "Makeup Gain", " dB");

        // Post EQ gains
        configParam(POST_EQ_LO_PARAM,   -24.f, 24.f, 0.f, "Post EQ LO Gain",   " dB");
        configParam(POST_EQ_LMID_PARAM, -24.f, 24.f, 0.f, "Post EQ LMID Gain", " dB");
        configParam(POST_EQ_HMID_PARAM, -24.f, 24.f, 0.f, "Post EQ HMID Gain", " dB");
        configParam(POST_EQ_HI_PARAM,   -24.f, 24.f, 0.f, "Post EQ HI Gain",   " dB");

        // Sidechain filters (log-scale Hz)
        configParam(SC_LPF_PARAM, std::log2(200.f), std::log2(20000.f), std::log2(20000.f), "SC LPF", " Hz", 2.f);
        configParam(SC_HPF_PARAM, std::log2(20.f),  std::log2(2000.f),  std::log2(20.f),    "SC HPF", " Hz", 2.f);
        // Output selector: 0=Monitor (sidechain monitor), 1=Output (processed)
        configSwitch(OUT_SEL_PARAM, 0.f, 1.f, 1.f, "Output", {"Monitor", "Output"});

        configInput(IN_L_INPUT,    "Input L");
        configInput(IN_R_INPUT,    "Input R");
        configInput(SC_IN_L_INPUT, "Sidechain Input L");
        configInput(SC_IN_R_INPUT, "Sidechain Input R");

        configOutput(OUT_L_OUTPUT,     "Output L");
        configOutput(OUT_R_OUTPUT,     "Output R");

        for (int b = 0; b < 4; ++b) {
            dsp.setBandFrequency(b, bandFreqs[b]);
        }
    }

    void process(const ProcessArgs& args) override {
        dsp.setSampleRate(args.sampleRate);

        // Pre EQ
        dsp.setPreEqGain(0, params[PRE_EQ_LO_PARAM].getValue());
        dsp.setPreEqGain(1, params[PRE_EQ_LMID_PARAM].getValue());
        dsp.setPreEqGain(2, params[PRE_EQ_HMID_PARAM].getValue());
        dsp.setPreEqGain(3, params[PRE_EQ_HI_PARAM].getValue());

        // Amounts
        dsp.setAmount(0, params[AMT_LO_PARAM].getValue());
        dsp.setAmount(1, params[AMT_LMID_PARAM].getValue());
        dsp.setAmount(2, params[AMT_HMID_PARAM].getValue());
        dsp.setAmount(3, params[AMT_HI_PARAM].getValue());

        // Envelope / mix / gain
        dsp.setAttack(params[ATTACK_PARAM].getValue());
        dsp.setRelease(params[RELEASE_PARAM].getValue());
        dsp.setMix(params[MIX_PARAM].getValue());
        dsp.setMakeupGain(params[GAIN_PARAM].getValue());

        // Post EQ
        dsp.setPostEqGain(0, params[POST_EQ_LO_PARAM].getValue());
        dsp.setPostEqGain(1, params[POST_EQ_LMID_PARAM].getValue());
        dsp.setPostEqGain(2, params[POST_EQ_HMID_PARAM].getValue());
        dsp.setPostEqGain(3, params[POST_EQ_HI_PARAM].getValue());

        dsp.setScLpfFreq(std::pow(2.f, params[SC_LPF_PARAM].getValue()));
        dsp.setScHpfFreq(std::pow(2.f, params[SC_HPF_PARAM].getValue()));

        float inL = inputs[IN_L_INPUT].getVoltage();
        float inR = inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltage() : inL;
        float scL = inputs[SC_IN_L_INPUT].isConnected() ? inputs[SC_IN_L_INPUT].getVoltage() : inL;
        float scR = inputs[SC_IN_R_INPUT].isConnected() ? inputs[SC_IN_R_INPUT].getVoltage() : scL;

        float outL = 0.f, outR = 0.f, scMonL = 0.f, scMonR = 0.f;
        dsp.process(inL, inR, scL, scR, outL, outR, scMonL, scMonR);

        // OUT_SEL: 1 = processed output, 0 = sidechain monitor (filtered by SC LPF/HPF)
        bool selectOutput = params[OUT_SEL_PARAM].getValue() > 0.5f;
        if (selectOutput) {
            outputs[OUT_L_OUTPUT].setVoltage(outL);
            outputs[OUT_R_OUTPUT].setVoltage(outR);
        } else {
            outputs[OUT_L_OUTPUT].setVoltage(scMonL);
            outputs[OUT_R_OUTPUT].setVoltage(scMonR);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_t* freqsJ = json_array();
        for (int b = 0; b < 4; ++b) {
            json_array_append_new(freqsJ, json_real(bandFreqs[b]));
        }
        json_object_set_new(rootJ, "bandFreqs", freqsJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_real_value(contrastJ);

        json_t* freqsJ = json_object_get(rootJ, "bandFreqs");
        if (freqsJ && json_is_array(freqsJ)) {
            for (int b = 0; b < 4 && b < (int)json_array_size(freqsJ); ++b) {
                json_t* fJ = json_array_get(freqsJ, b);
                if (fJ) {
                    bandFreqs[b] = json_number_value(fJ);
                    dsp.setBandFrequency(b, bandFreqs[b]);
                }
            }
        }
    }
};

// ============================================================================
// SpectrumGRDisplay
// ============================================================================
struct SpectrumGRDisplay : TransparentWidget {
    FFTComp* module = nullptr;

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 25));
        nvgFill(args.vg);

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 60));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        if (!module) {
            // Placeholder text when no module bound
            nvgFontSize(args.vg, 9.f);
            nvgFontFaceId(args.vg, APP->window->uiFont->handle);
            nvgFillColor(args.vg, nvgRGBA(180, 180, 180, 180));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, "SPECTRUM / GR", NULL);
            return;
        }

        const float* spectrum = module->dsp.getSpectrumMagnitudes();
        const float* gr = module->dsp.getGainReductionPerBin();
        int numBins = module->dsp.getNumBins();
        if (numBins <= 0 || !spectrum || !gr) return;

        // Log frequency axis: map bin index to log-x
        // dB Y axis: top = 0 dB, bottom = -60 dB
        const float dBMin = -60.f;
        const float dBMax = 0.f;

        auto xForBin = [&](int bin) {
            // Skip DC (bin 0). Use log mapping from bin 1..numBins-1
            float minLog = std::log10(1.f);
            float maxLog = std::log10((float)(numBins - 1));
            float l = std::log10((float)std::max(1, bin));
            float t = (l - minLog) / (maxLog - minLog);
            return t * box.size.x;
        };

        auto yForDb = [&](float dB) {
            float t = (dB - dBMin) / (dBMax - dBMin);
            t = clamp(t, 0.f, 1.f);
            return box.size.y - t * box.size.y;
        };

        // ----- GR overlay (smooth orange stroke, drawn FIRST) -----
        // gr[bin]: linear gain 0..1, 1 = no GR. Curve hangs from top:
        // 0 dB reduction -> y=0 (top), full reduction -> y=box.size.y (bottom).
        const float grMaxDb = 24.f;
        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(255, 200, 0, 230));
        nvgStrokeWidth(args.vg, 0.8f);
        {
            bool started = false;
            for (int bin = 1; bin < numBins; ++bin) {
                float g = clamp(gr[bin], 1e-4f, 1.f);
                float redDb = -20.f * std::log10(g);
                float t = clamp(redDb / grMaxDb, 0.f, 1.f);
                float x = xForBin(bin);
                float y = t * box.size.y;
                if (!started) { nvgMoveTo(args.vg, x, y); started = true; }
                else { nvgLineTo(args.vg, x, y); }
            }
            if (started) nvgStroke(args.vg);
        }

        // ----- Spectrum line (white smooth stroke, drawn ON TOP) -----
        // Break path when magnitude drops below noise floor.
        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 230));
        nvgStrokeWidth(args.vg, 0.8f);
        bool drawing = false;
        for (int bin = 1; bin < numBins; ++bin) {
            float mag = spectrum[bin];
            float dB = (mag > 1e-6f) ? 20.f * std::log10(mag) : (dBMin - 10.f);
            float x = xForBin(bin);
            if (dB <= dBMin) {
                if (drawing) { nvgStroke(args.vg); nvgBeginPath(args.vg); nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 230)); nvgStrokeWidth(args.vg, 1.5f); drawing = false; }
                continue;
            }
            float y = yForDb(dB);
            if (!drawing) { nvgMoveTo(args.vg, x, y); drawing = true; }
            else { nvgLineTo(args.vg, x, y); }
        }
        if (drawing) nvgStroke(args.vg);
    }
};

// ============================================================================
// Widget
// ============================================================================
struct FFTCompWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    FFTCompWidget(FFTComp* module) {
        setModule(module);
        panelThemeHelper.init(this, "14HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(14 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // White bottom panel (Y>=330)
        FFTCompWhiteBottomPanel* whitePanel = new FFTCompWhiteBottomPanel();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // ===== Title =====
        addChild(new FFTCompTextLabel(Vec(0, 1), Vec(box.size.x, 20),
                                      "F F T  C O M P", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new FFTCompTextLabel(Vec(0, 13), Vec(box.size.x, 20),
                                      "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // ===== Knob X positions (4 columns) =====
        const float colX[4] = {35.f, 82.f, 129.f, 176.f};
        const char* bandLabels[4] = {"LO", "LMID", "HMID", "HI"};

        // ===== Section title color (white for readability on red panel) =====
        const NVGcolor sectionColor = nvgRGB(255, 255, 255);

        // ----- Pre EQ section : title Y=30, band labels Y=42, knobs Y=66 -----
        addChild(new FFTCompTextLabel(Vec(0, 36), Vec(box.size.x, 10),
                                      "PRE EQ", 9.f, sectionColor, true));
        for (int i = 0; i < 4; ++i) {
            addChild(new FFTCompTextLabel(Vec(colX[i] - 25, 48), Vec(50, 10),
                                          bandLabels[i], 8.f, nvgRGB(255, 255, 255), true));
        }
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[0], 72), module, FFTComp::PRE_EQ_LO_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[1], 72), module, FFTComp::PRE_EQ_LMID_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[2], 72), module, FFTComp::PRE_EQ_HMID_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[3], 72), module, FFTComp::PRE_EQ_HI_PARAM));

        // ----- SpectrumGRDisplay: pos(5,84) size(box.size.x-10,58) -----
        {
            SpectrumGRDisplay* disp = new SpectrumGRDisplay();
            disp->box.pos = Vec(5, 90);
            disp->box.size = Vec(box.size.x - 10, 64);
            disp->module = module;
            addChild(disp);
        }

        // ----- COMPRESSOR section : single header, two knob rows -----
        addChild(new FFTCompTextLabel(Vec(0, 162), Vec(box.size.x, 10),
                                      "COMPRESSOR", 9.f, sectionColor, true));
        // Row 1: AMOUNT band labels Y=148, knobs Y=174 (MediumGrayKnob 26px, offset 26)
        for (int i = 0; i < 4; ++i) {
            addChild(new FFTCompTextLabel(Vec(colX[i] - 25, 172), Vec(50, 10),
                                          bandLabels[i], 8.f, nvgRGB(255, 255, 255), true));
        }
        addParam(createParamCentered<MediumGrayKnob>(Vec(colX[0], 196), module, FFTComp::AMT_LO_PARAM));
        addParam(createParamCentered<MediumGrayKnob>(Vec(colX[1], 196), module, FFTComp::AMT_LMID_PARAM));
        addParam(createParamCentered<MediumGrayKnob>(Vec(colX[2], 196), module, FFTComp::AMT_HMID_PARAM));
        addParam(createParamCentered<MediumGrayKnob>(Vec(colX[3], 196), module, FFTComp::AMT_HI_PARAM));

        // Row 2: GLOBAL env labels Y=206, knobs Y=238 (LargeWhiteKnob 37px, offset 32)
        const char* envLabels[4] = {"ATTACK", "RELEASE", "MIX", "GAIN"};
        for (int i = 0; i < 4; ++i) {
            addChild(new FFTCompTextLabel(Vec(colX[i] - 25, 217), Vec(50, 10),
                                          envLabels[i], 8.f, nvgRGB(255, 255, 255), true));
        }
        addParam(createParamCentered<LargeWhiteKnob>(Vec(colX[0], 249), module, FFTComp::ATTACK_PARAM));
        addParam(createParamCentered<LargeWhiteKnob>(Vec(colX[1], 249), module, FFTComp::RELEASE_PARAM));
        addParam(createParamCentered<LargeWhiteKnob>(Vec(colX[2], 249), module, FFTComp::MIX_PARAM));
        addParam(createParamCentered<LargeWhiteKnob>(Vec(colX[3], 249), module, FFTComp::GAIN_PARAM));

        // ----- Post EQ section : title Y=270, band labels Y=282, knobs Y=308 -----
        addChild(new FFTCompTextLabel(Vec(0, 276), Vec(box.size.x, 10),
                                      "POST EQ", 9.f, sectionColor, true));
        for (int i = 0; i < 4; ++i) {
            addChild(new FFTCompTextLabel(Vec(colX[i] - 25, 288), Vec(50, 10),
                                          bandLabels[i], 8.f, nvgRGB(255, 255, 255), true));
        }
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[0], 312), module, FFTComp::POST_EQ_LO_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[1], 312), module, FFTComp::POST_EQ_LMID_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[2], 312), module, FFTComp::POST_EQ_HMID_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(colX[3], 312), module, FFTComp::POST_EQ_HI_PARAM));

        // ===== I/O row (white area, Y>=330) =====
        NVGcolor pinkColor = nvgRGB(255, 133, 133);

        // Pair labels (Y=355.5 center between row1=343 and row2=368)
        // IN label: 放最左, 中心 X=7, 視覺範圍 ~4-10, 與 IN port 圓 (X=26, 圓 14-38) 留 gap 4px
        addChild(new FFTCompTextLabel(Vec(0.f,   351.f), Vec(14.f, 9.f), "IN",    7.f, pinkColor, true));
        // SC IN: 垂直堆疊, 中心 X=58 (距 SC IN port 22 < 距 IN port 32, 視覺歸屬 SC IN pair)
        addChild(new FFTCompTextLabel(Vec(51.f, 343.5f), Vec(14.f, 9.f), "SC", 7.f, pinkColor, true));
        addChild(new FFTCompTextLabel(Vec(51.f, 358.5f), Vec(14.f, 9.f), "IN", 7.f, pinkColor, true));

        // SC LPF / HPF knob row labels (LP / HP 中心 X=101, 視覺 98-104, 完全避開 SC IN port X 範圍 68-92)
        addChild(new FFTCompTextLabel(Vec(94.f, 338.5f), Vec(14.f, 9.f), "LP", 7.f, pinkColor, true));
        addChild(new FFTCompTextLabel(Vec(94.f, 363.5f), Vec(14.f, 9.f), "HP", 7.f, pinkColor, true));

        // Switch labels (above and below)
        addChild(new FFTCompTextLabel(Vec(151.f, 332.f), Vec(14.f, 9.f), "OUT", 7.f, pinkColor, true));
        addChild(new FFTCompTextLabel(Vec(151.f, 371.f), Vec(14.f, 9.f), "MON", 7.f, pinkColor, true));

        // IN L/R (X=26, 圓 14-38)
        addInput(createInputCentered<PJ301MPort>(Vec(26.f, 343.f), module, FFTComp::IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(26.f, 368.f), module, FFTComp::IN_R_INPUT));
        // SC IN L/R (X=80, 圓 68-92)
        addInput(createInputCentered<PJ301MPort>(Vec(80.f, 343.f), module, FFTComp::SC_IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(80.f, 368.f), module, FFTComp::SC_IN_R_INPUT));
        // SC LPF / HPF knobs (X=120, 圓 107-133)
        addParam(createParamCentered<StandardBlackKnob26>(Vec(120.f, 343.f), module, FFTComp::SC_LPF_PARAM));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(120.f, 368.f), module, FFTComp::SC_HPF_PARAM));
        // OUT_SEL switch
        addParam(createParamCentered<CKSS>(Vec(158.f, 355.5f), module, FFTComp::OUT_SEL_PARAM));
        // OUT L/R
        addOutput(createOutputCentered<PJ301MPort>(Vec(190.f, 343.f), module, FFTComp::OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(190.f, 368.f), module, FFTComp::OUT_R_OUTPUT));
    }

    void step() override {
        FFTComp* m = dynamic_cast<FFTComp*>(this->module);
        if (m) panelThemeHelper.step(m);
        ModuleWidget::step();
    }

    // Right-click context menu - band frequency editing
    struct BandFreqQuantity : Quantity {
        FFTComp* module;
        int band;
        float minHz, maxHz;
        BandFreqQuantity(FFTComp* m, int b, float lo, float hi)
            : module(m), band(b), minHz(lo), maxHz(hi) {}

        void setValue(float value) override {
            value = clamp(value, getMinValue(), getMaxValue());
            float hz = std::pow(2.f, value);
            if (module) {
                module->bandFreqs[band] = hz;
                module->dsp.setBandFrequency(band, hz);
            }
        }
        float getValue() override {
            if (!module) return std::log2(FFTCompDSP::DEFAULT_FREQS[band]);
            return std::log2(std::max(1.f, module->bandFreqs[band]));
        }
        float getMinValue() override { return std::log2(minHz); }
        float getMaxValue() override { return std::log2(maxHz); }
        float getDefaultValue() override { return std::log2(FFTCompDSP::DEFAULT_FREQS[band]); }
        std::string getLabel() override {
            static const char* names[4] = {"LO", "LMID", "HMID", "HI"};
            return std::string("Band ") + names[band] + " Frequency";
        }
        std::string getUnit() override { return " Hz"; }
        std::string getDisplayValueString() override {
            float hz = module ? module->bandFreqs[band] : FFTCompDSP::DEFAULT_FREQS[band];
            return string::f("%.0f", hz);
        }
    };

    struct BandFreqSlider : ui::Slider {
        BandFreqSlider(FFTComp* module, int band, float lo, float hi) {
            box.size.x = 200.0f;
            quantity = new BandFreqQuantity(module, band, lo, hi);
        }
        ~BandFreqSlider() { delete quantity; }
    };

    void appendContextMenu(ui::Menu* menu) override {
        FFTComp* m = dynamic_cast<FFTComp*>(this->module);
        if (!m) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Band Frequencies"));

        // Reasonable per-band frequency ranges
        const float ranges[4][2] = {
            { 20.f,    500.f},  // LO
            { 80.f,   1500.f},  // LMID
            {300.f,   6000.f},  // HMID
            {800.f,  18000.f}   // HI
        };
        const char* names[4] = {"LO", "LMID", "HMID", "HI"};
        for (int b = 0; b < 4; ++b) {
            menu->addChild(createMenuLabel(std::string("  ") + names[b]));
            BandFreqSlider* slider = new BandFreqSlider(m, b, ranges[b][0], ranges[b][1]);
            menu->addChild(slider);
        }

        menu->addChild(new MenuSeparator);
        addPanelThemeMenu(menu, m);
    }
};

Model* modelFFTComp = createModel<FFTComp, FFTCompWidget>("FFTComp");
