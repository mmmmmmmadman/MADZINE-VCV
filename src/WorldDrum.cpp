/**
 * WorldDrum.cpp
 * 4-Voice World Drum Synthesizer
 *
 * 從 Universal Rhythm 提取的獨立鼓合成器模組
 * - 4 聲部：Timeline, Foundation, Groove, Lead
 * - 10 種世界音樂曲風預設
 * - 完整 CV 控制
 */

#include "plugin.hpp"
#include "WorldRhythm/MinimalDrumSynth.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

using namespace worldrhythm;

// Style names (global, for ParamQuantity and Display)
static const char* WD_STYLE_NAMES[10] = {
    "West African", "Afro-Cuban", "Brazilian", "Balkan", "Indian",
    "Gamelan", "Jazz", "Electronic", "Breakbeat", "Techno"
};

struct WDStyleParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int index = static_cast<int>(getValue());
        if (index >= 0 && index < 10) {
            return WD_STYLE_NAMES[index];
        }
        return ParamQuantity::getDisplayValueString();
    }
};

struct WorldDrum : Module {
    enum ParamId {
        STYLE_PARAM,
        SPREAD_PARAM,
        // Per-voice parameters (4 voices)
        FREQ_PARAM_TL,
        FREQ_PARAM_FD,
        FREQ_PARAM_GR,
        FREQ_PARAM_LD,
        DECAY_PARAM_TL,
        DECAY_PARAM_FD,
        DECAY_PARAM_GR,
        DECAY_PARAM_LD,
        PARAMS_LEN
    };

    enum InputId {
        STYLE_CV_INPUT,
        // Per-voice inputs (4 voices × 4 types)
        TRIG_INPUT_TL,
        TRIG_INPUT_FD,
        TRIG_INPUT_GR,
        TRIG_INPUT_LD,
        VEL_INPUT_TL,
        VEL_INPUT_FD,
        VEL_INPUT_GR,
        VEL_INPUT_LD,
        FREQ_CV_INPUT_TL,
        FREQ_CV_INPUT_FD,
        FREQ_CV_INPUT_GR,
        FREQ_CV_INPUT_LD,
        DECAY_CV_INPUT_TL,
        DECAY_CV_INPUT_FD,
        DECAY_CV_INPUT_GR,
        DECAY_CV_INPUT_LD,
        INPUTS_LEN
    };

    enum OutputId {
        // Per-voice outputs
        AUDIO_OUTPUT_TL,
        AUDIO_OUTPUT_FD,
        AUDIO_OUTPUT_GR,
        AUDIO_OUTPUT_LD,
        // Stereo mix
        MIX_L_OUTPUT,
        MIX_R_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHTS_LEN
    };

    // Drum synthesizer engine
    MinimalDrumSynth drumSynth;

    // Trigger detection (Schmitt triggers)
    dsp::SchmittTrigger trigSchmitt[4];

    // Current style index
    int currentStyle = 0;

    // CV modulation display values
    float styleCvMod = 0.0f;
    float freqCvMod[4] = {};
    float decayCvMod[4] = {};

    // Panel theme
    int panelTheme = -1;
    float panelContrast = 255.0f;

    WorldDrum() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Global parameters
        configParam<WDStyleParamQuantity>(STYLE_PARAM, 0.f, 9.f, 0.f, "Style");
        getParamQuantity(STYLE_PARAM)->snapEnabled = true;
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.5f, "Stereo Spread", "%", 0.f, 100.f);

        // Per-voice parameters
        const char* voiceNames[4] = {"Timeline", "Foundation", "Groove", "Lead"};

        for (int i = 0; i < 4; i++) {
            // FREQ: -1 to +1 (±1 octave adjustment)
            configParam(FREQ_PARAM_TL + i, -1.f, 1.f, 0.f,
                        std::string(voiceNames[i]) + " Freq", " oct");
            // DECAY: 0.2 to 2.0 (multiplier)
            configParam(DECAY_PARAM_TL + i, 0.2f, 2.f, 1.f,
                        std::string(voiceNames[i]) + " Decay", "x");
        }

        // Inputs
        configInput(STYLE_CV_INPUT, "Style CV");

        for (int i = 0; i < 4; i++) {
            configInput(TRIG_INPUT_TL + i, std::string(voiceNames[i]) + " Trigger");
            configInput(VEL_INPUT_TL + i, std::string(voiceNames[i]) + " Velocity CV");
            configInput(FREQ_CV_INPUT_TL + i, std::string(voiceNames[i]) + " Freq CV");
            configInput(DECAY_CV_INPUT_TL + i, std::string(voiceNames[i]) + " Decay CV");
        }

        // Outputs
        for (int i = 0; i < 4; i++) {
            configOutput(AUDIO_OUTPUT_TL + i, std::string(voiceNames[i]) + " Audio");
        }
        configOutput(MIX_L_OUTPUT, "Mix L");
        configOutput(MIX_R_OUTPUT, "Mix R");

        // Initialize with default style
        applyStylePreset(drumSynth, 0);

        // Load global settings
        panelTheme = madzineDefaultTheme;
        panelContrast = madzineDefaultContrast;
    }

    void onSampleRateChange() override {
        drumSynth.setSampleRate(APP->engine->getSampleRate());
    }

    void process(const ProcessArgs& args) override {
        // Update sample rate
        drumSynth.setSampleRate(args.sampleRate);

        // Read style parameter with CV
        float styleValue = params[STYLE_PARAM].getValue();
        if (inputs[STYLE_CV_INPUT].isConnected()) {
            float cv = inputs[STYLE_CV_INPUT].getVoltage();
            styleValue += cv;
            styleCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            styleCvMod = 0.0f;
        }
        int newStyle = clamp((int)std::round(styleValue), 0, 9);

        // Apply style preset if changed
        if (newStyle != currentStyle) {
            currentStyle = newStyle;
            applyStylePreset(drumSynth, currentStyle);
        }

        // Get base preset for parameter modulation
        const StyleSynthPreset& preset = STYLE_SYNTH_PRESETS[currentStyle];
        const StyleSynthPreset::VoicePreset* voicePresets[4] = {
            &preset.timeline, &preset.foundation, &preset.groove, &preset.lead
        };

        // Process each voice
        float voiceOutputs[4];

        for (int v = 0; v < 4; v++) {
            // Read parameters with CV modulation
            float freqParam = params[FREQ_PARAM_TL + v].getValue();
            float decayParam = params[DECAY_PARAM_TL + v].getValue();

            // FREQ CV: ±5V = ±1 octave
            if (inputs[FREQ_CV_INPUT_TL + v].isConnected()) {
                float cv = inputs[FREQ_CV_INPUT_TL + v].getVoltage();
                freqParam += cv * 0.2f;
                freqCvMod[v] = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else {
                freqCvMod[v] = 0.0f;
            }
            freqParam = clamp(freqParam, -1.f, 1.f);

            // DECAY CV: ±5V = ±0.9 multiplier
            if (inputs[DECAY_CV_INPUT_TL + v].isConnected()) {
                float cv = inputs[DECAY_CV_INPUT_TL + v].getVoltage();
                decayParam += cv * 0.18f;
                decayCvMod[v] = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else {
                decayCvMod[v] = 0.0f;
            }
            decayParam = clamp(decayParam, 0.2f, 2.f);

            // Calculate modulated frequency and decay
            float baseFreq = voicePresets[v]->freq;
            float baseDecay = voicePresets[v]->decay;
            float modFreq = baseFreq * std::pow(2.f, freqParam);
            float modDecay = baseDecay * decayParam;

            // Apply to synth (mode stays the same from preset)
            drumSynth.setVoiceParams(v, voicePresets[v]->mode, modFreq, modDecay);

            // Trigger detection
            if (inputs[TRIG_INPUT_TL + v].isConnected()) {
                if (trigSchmitt[v].process(inputs[TRIG_INPUT_TL + v].getVoltage(), 0.1f, 2.f)) {
                    // Get velocity (default 1.0, or from CV)
                    float velocity = 1.0f;
                    if (inputs[VEL_INPUT_TL + v].isConnected()) {
                        velocity = clamp(inputs[VEL_INPUT_TL + v].getVoltage() / 10.f, 0.f, 1.f);
                    }

                    drumSynth.triggerVoice(v, velocity);
                }
            }
        }

        // Process audio
        drumSynth.processSeparate(voiceOutputs);

        // Output per-voice audio
        for (int v = 0; v < 4; v++) {
            outputs[AUDIO_OUTPUT_TL + v].setVoltage(voiceOutputs[v] * 5.f);
        }

        // Stereo mix with spread
        float spread = params[SPREAD_PARAM].getValue();
        float mixL = 0.f, mixR = 0.f;

        // Panning positions: TL=-0.5, FD=0, GR=+0.3, LD=+0.7
        const float panPositions[4] = {-0.5f, 0.f, 0.3f, 0.7f};

        for (int v = 0; v < 4; v++) {
            float pan = panPositions[v] * spread;
            float gainL = std::cos((pan + 1.f) * 0.25f * M_PI);
            float gainR = std::sin((pan + 1.f) * 0.25f * M_PI);
            mixL += voiceOutputs[v] * gainL;
            mixR += voiceOutputs[v] * gainR;
        }

        // Soft limiting
        outputs[MIX_L_OUTPUT].setVoltage(std::tanh(mixL) * 5.f);
        outputs[MIX_R_OUTPUT].setVoltage(std::tanh(mixR) * 5.f);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_number_value(contrastJ);
    }
};

// STYLE_COLORS (MUJI-inspired palette)
const NVGcolor STYLE_COLORS[10] = {
    nvgRGB(255, 120, 100),  // 0: West African - Warm coral
    nvgRGB(100, 200, 255),  // 1: Afro-Cuban - Sky blue
    nvgRGB(255, 200, 80),   // 2: Brazilian - Golden yellow
    nvgRGB(200, 100, 150),  // 3: Balkan - Deep rose
    nvgRGB(255, 150, 200),  // 4: Indian - Pink
    nvgRGB(150, 220, 180),  // 5: Gamelan - Mint green
    nvgRGB(180, 150, 255),  // 6: Jazz - Lavender
    nvgRGB(100, 220, 220),  // 7: Electronic - Cyan
    nvgRGB(255, 180, 100),  // 8: Breakbeat - Orange
    nvgRGB(220, 220, 220),  // 9: Techno - Silver gray
};

// Enhanced Text Label (same as ALEXANDERPLATZ)
struct WorldDrumTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    int align;

    WorldDrumTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
                       NVGcolor color = nvgRGB(255, 255, 255), bool bold = true,
                       int align = NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
        this->bold = bold;
        this->align = align;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, align);

        float tx = (align & NVG_ALIGN_LEFT) ? 0.f : box.size.x / 2.f;

        if (bold) {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, tx, box.size.y / 2.f, text.c_str(), NULL);
            nvgStrokeColor(args.vg, color);
            nvgStrokeWidth(args.vg, 0.3f);
            nvgText(args.vg, tx, box.size.y / 2.f, text.c_str(), NULL);
        } else {
            nvgFillColor(args.vg, color);
            nvgText(args.vg, tx, box.size.y / 2.f, text.c_str(), NULL);
        }
    }
};

// Forward declaration for dynamic role title
struct WorldDrum;

// Dynamic role title that changes color based on global style (left-aligned, compact for 8HP)
struct WDDynamicRoleTitle : TransparentWidget {
    WorldDrum* module = nullptr;
    std::string text;
    float fontSize;
    bool bold;

    WDDynamicRoleTitle(Vec pos, Vec size, std::string text, float fontSize = 6.5f, bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override;  // Implemented after WorldDrum definition
};

// White Bottom Panel (Y >= 330)
struct WorldDrumWhitePanel : Widget {
    WorldDrumWhitePanel(Vec pos, Vec size) {
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

// Dynamic Style Display
struct StyleDisplay : TransparentWidget {
    WorldDrum* module = nullptr;

    StyleDisplay(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        std::string styleName = "West African";
        NVGcolor color = STYLE_COLORS[0];
        if (module) {
            styleName = WD_STYLE_NAMES[module->currentStyle];
            color = STYLE_COLORS[module->currentStyle];
        }

        nvgFontSize(args.vg, 11.f);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        float cx = box.size.x / 2.f;
        float cy = box.size.y / 2.f;

        // Colored glow (style color)
        nvgFontBlur(args.vg, 3.0f);
        nvgFillColor(args.vg, color);
        nvgText(args.vg, cx, cy, styleName.c_str(), NULL);
        nvgText(args.vg, cx, cy, styleName.c_str(), NULL);

        // White text (always readable)
        nvgFontBlur(args.vg, 0.f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgText(args.vg, cx, cy, styleName.c_str(), NULL);
    }
};

struct WorldDrumWidget : ModuleWidget {
    PanelThemeHelper panelHelper;
    TechnoSnapKnob30* styleKnob = nullptr;
    MediumGrayKnob* freqKnobs[4] = {};
    MediumGrayKnob* decayKnobs[4] = {};

    WorldDrumWidget(WorldDrum* module) {
        setModule(module);
        panelHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // ========== TITLE AREA (Y=0-26) ==========
        addChild(new WorldDrumTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Drummmmmmer", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new WorldDrumTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // ========== WHITE BOTTOM PANEL (Y=330-380) ==========
        addChild(new WorldDrumWhitePanel(Vec(0, 330), Vec(box.size.x, 50)));

        // ========== PHASE 1: ALL PORTS AND KNOBS (z-order bottom) ==========

        // --- GLOBAL CONTROLS: STYLE knob (left) + StyleDisplay (center) + CV port (right), same row ---
        // STYLE knob (TechnoSnapKnob30, 30px) X=18, Y=56
        styleKnob = createParamCentered<TechnoSnapKnob30>(Vec(18.f, 56.f), module, WorldDrum::STYLE_PARAM);
        addParam(styleKnob);
        // STYLE CV port (24px) X=106, Y=56
        addInput(createInputCentered<PJ301MPort>(Vec(106.f, 56.f), module, WorldDrum::STYLE_CV_INPUT));

        // --- 4 VOICES AREA (spacing=61px, shifted down 12px from original) ---
        // Row2[3] bottom = 281+26+12 = 319 < 330
        const float startY[4] = {98.f, 159.f, 220.f, 281.f};
        // Map UI row to internal voice index: Lead(3), Groove(2), Timeline(0), Foundation(1)
        const int voiceMap[4] = {3, 2, 0, 1};

        // Voice X coordinates
        float trigX = 15.f;
        float freqX = 43.f;
        float decayX = 73.f;
        float outX = 103.f;

        for (int v = 0; v < 4; v++) {
            float sY = startY[v];
            int vi = voiceMap[v];  // internal voice index

            // Row 1: TRIG port | FREQ knob (26px) | DECAY knob (26px) | Audio OUT port
            addInput(createInputCentered<PJ301MPort>(Vec(trigX, sY), module, WorldDrum::TRIG_INPUT_TL + vi));
            freqKnobs[vi] = createParamCentered<MediumGrayKnob>(Vec(freqX, sY), module, WorldDrum::FREQ_PARAM_TL + vi);
            addParam(freqKnobs[vi]);
            decayKnobs[vi] = createParamCentered<MediumGrayKnob>(Vec(decayX, sY), module, WorldDrum::DECAY_PARAM_TL + vi);
            addParam(decayKnobs[vi]);
            addOutput(createOutputCentered<PJ301MPort>(Vec(outX, sY), module, WorldDrum::AUDIO_OUTPUT_TL + vi));

            // Row 2 (sY+26): VEL port | FREQ CV port | DECAY CV port
            addInput(createInputCentered<PJ301MPort>(Vec(trigX, sY + 26.f), module, WorldDrum::VEL_INPUT_TL + vi));
            addInput(createInputCentered<PJ301MPort>(Vec(freqX, sY + 26.f), module, WorldDrum::FREQ_CV_INPUT_TL + vi));
            addInput(createInputCentered<PJ301MPort>(Vec(decayX, sY + 26.f), module, WorldDrum::DECAY_CV_INPUT_TL + vi));
        }

        // --- WHITE OUTPUT AREA (Y=330-380) ---
        // SPREAD knob (MediumGrayKnob, 26px) X=25, Y=355
        addParam(createParamCentered<MediumGrayKnob>(Vec(25.f, 355.f), module, WorldDrum::SPREAD_PARAM));
        // MIX L output X=70, Y=355
        addOutput(createOutputCentered<PJ301MPort>(Vec(70.f, 355.f), module, WorldDrum::MIX_L_OUTPUT));
        // MIX R output X=103, Y=355
        addOutput(createOutputCentered<PJ301MPort>(Vec(103.f, 355.f), module, WorldDrum::MIX_R_OUTPUT));

        // ========== PHASE 2: ALL LABELS AND DISPLAYS (z-order top) ==========

        // --- GLOBAL LABELS ---
        // STYLE label (8.f bold white) above knob at X=18 (offset 28px for 30px knob: Y=56-28=28)
        addChild(new WorldDrumTextLabel(Vec(3, 28), Vec(30, 15), "STYLE", 8.f, nvgRGB(255, 255, 255), true));
        // CV label (8.f bold white) above port at X=106 (offset 24px for port: Y=56-24=32)
        addChild(new WorldDrumTextLabel(Vec(94, 32), Vec(24, 15), "CV", 8.f, nvgRGB(255, 255, 255), true));

        // --- StyleDisplay between knob and CV, same row (11.f colorful dynamic) ---
        StyleDisplay* styleDisp = new StyleDisplay(Vec(35, 48), Vec(56, 16));
        styleDisp->module = module;
        addChild(styleDisp);

        // --- COLUMN HEADERS (Y=72, aligned with voice columns) ---
        addChild(new WorldDrumTextLabel(Vec(33, 72), Vec(20, 15), "FREQ", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new WorldDrumTextLabel(Vec(60, 72), Vec(26, 15), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new WorldDrumTextLabel(Vec(93, 72), Vec(20, 15), "OUT", 8.f, nvgRGB(255, 255, 255), true));

        // --- VOICE NAMES (white 7.f bold, in Row2 OUT column, centered) ---
        // Single-line: Lead, Groove. Two-line: Time/line, Founda/tion
        {
            float nameX = 88.5f;
            float nameW = 29.f;
            auto addRoleLabel = [&](Vec pos, Vec size, std::string text) {
                auto* label = new WDDynamicRoleTitle(pos, size, text, 10.f, true);
                label->module = module;
                addChild(label);
            };
            for (int v = 0; v < 4; v++) {
                float row2Y = startY[v] + 26.f;
                if (v == 0) {
                    addRoleLabel(Vec(nameX, row2Y - 7.f), Vec(nameW, 15), "Lead");
                } else if (v == 1) {
                    addRoleLabel(Vec(nameX, row2Y - 7.f), Vec(nameW, 15), "Groove");
                } else if (v == 2) {
                    addRoleLabel(Vec(nameX, row2Y - 12.f), Vec(nameW, 15), "Time");
                    addRoleLabel(Vec(nameX, row2Y - 3.f), Vec(nameW, 15), "line");
                } else {
                    addRoleLabel(Vec(nameX, row2Y - 12.f), Vec(nameW, 15), "Founda");
                    addRoleLabel(Vec(nameX, row2Y - 3.f), Vec(nameW, 15), "tion");
                }
            }
        }

        // --- WHITE AREA LABELS (Y>=330, pink 7.f) ---
        // SPREAD label
        addChild(new WorldDrumTextLabel(Vec(8, 331), Vec(34, 15), "SPREAD", 7.f, nvgRGB(255, 133, 133), true));
        // L label
        addChild(new WorldDrumTextLabel(Vec(58, 331), Vec(24, 15), "L", 7.f, nvgRGB(255, 133, 133), true));
        // R label
        addChild(new WorldDrumTextLabel(Vec(91, 331), Vec(24, 15), "R", 7.f, nvgRGB(255, 133, 133), true));
    }

    void step() override {
        WorldDrum* m = dynamic_cast<WorldDrum*>(module);
        if (m) {
            panelHelper.step(m);

            if (styleKnob) {
                bool connected = m->inputs[WorldDrum::STYLE_CV_INPUT].isConnected();
                styleKnob->setModulationEnabled(connected);
                if (connected) styleKnob->setModulation(m->styleCvMod);
            }
            for (int v = 0; v < 4; v++) {
                if (freqKnobs[v]) {
                    bool connected = m->inputs[WorldDrum::FREQ_CV_INPUT_TL + v].isConnected();
                    freqKnobs[v]->setModulationEnabled(connected);
                    if (connected) freqKnobs[v]->setModulation(m->freqCvMod[v]);
                }
                if (decayKnobs[v]) {
                    bool connected = m->inputs[WorldDrum::DECAY_CV_INPUT_TL + v].isConnected();
                    decayKnobs[v]->setModulationEnabled(connected);
                    if (connected) decayKnobs[v]->setModulation(m->decayCvMod[v]);
                }
            }
        }
        ModuleWidget::step();
    }

    void appendContextMenu(Menu* menu) override {
        WorldDrum* module = dynamic_cast<WorldDrum*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

// ============================================================================
// WDDynamicRoleTitle::draw implementation (after WorldDrum definition)
// ============================================================================

void WDDynamicRoleTitle::draw(const DrawArgs &args) {
    NVGcolor color = nvgRGB(255, 255, 255);

    if (module) {
        color = STYLE_COLORS[module->currentStyle];
    }

    float cx = box.size.x / 2.f;
    float cy = box.size.y / 2.f;

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // Colored glow (style color)
    nvgFontBlur(args.vg, 3.0f);
    nvgFillColor(args.vg, color);
    nvgText(args.vg, cx, cy, text.c_str(), NULL);
    nvgText(args.vg, cx, cy, text.c_str(), NULL);

    // White text (always readable)
    nvgFontBlur(args.vg, 0.f);
    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
    nvgText(args.vg, cx, cy, text.c_str(), NULL);
}

Model* modelWorldDrum = createModel<WorldDrum, WorldDrumWidget>("WorldDrum");
