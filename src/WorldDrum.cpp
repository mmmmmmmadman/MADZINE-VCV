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

    // Panel theme
    int panelTheme = -1;
    float panelContrast = 255.0f;

    // Style names for display
    static constexpr const char* STYLE_NAMES[10] = {
        "West African",
        "Afro-Cuban",
        "Brazilian",
        "Balkan",
        "Indian",
        "Gamelan",
        "Jazz",
        "Electronic",
        "Breakbeat",
        "Techno"
    };

    WorldDrum() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Global parameters
        configParam(STYLE_PARAM, 0.f, 9.f, 0.f, "Style");
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
            // 0-10V maps to 0-9
            styleValue += inputs[STYLE_CV_INPUT].getVoltage();
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
                freqParam += inputs[FREQ_CV_INPUT_TL + v].getVoltage() * 0.2f;
            }
            freqParam = clamp(freqParam, -1.f, 1.f);

            // DECAY CV: ±5V = ±0.9 multiplier
            if (inputs[DECAY_CV_INPUT_TL + v].isConnected()) {
                decayParam += inputs[DECAY_CV_INPUT_TL + v].getVoltage() * 0.18f;
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

    WorldDrumTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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
        if (module) {
            styleName = WorldDrum::STYLE_NAMES[module->currentStyle];
        }

        nvgFontSize(args.vg, 9.f);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, nvgRGB(255, 200, 0));
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, styleName.c_str(), NULL);
    }
};

struct WorldDrumWidget : ModuleWidget {
    PanelThemeHelper panelHelper;

    WorldDrumWidget(WorldDrum* module) {
        setModule(module);
        panelHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // ========== TITLE AREA (Y=0-30) ==========
        addChild(new WorldDrumTextLabel(Vec(0, 1), Vec(box.size.x, 20), "WORLDDRUM", 10.f, nvgRGB(255, 200, 0), true));
        addChild(new WorldDrumTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // ========== GLOBAL CONTROLS (Y=30-118，壓縮佈局) ==========
        // STYLE 區塊（左側）
        addChild(new WorldDrumTextLabel(Vec(10, 30), Vec(30, 15), "STYLE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(30.f, 54.f), module, WorldDrum::STYLE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(30.f, 82.f), module, WorldDrum::STYLE_CV_INPUT));

        // Style 名稱動態顯示（中央）
        StyleDisplay* styleDisp = new StyleDisplay(Vec(0, 93), Vec(box.size.x, 12));
        styleDisp->module = module;
        addChild(styleDisp);

        // SPREAD 旋鈕（右側，對齊 STYLE 旋鈕 Y）
        addChild(new WorldDrumTextLabel(Vec(76, 30), Vec(30, 15), "SPREAD", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<MediumGrayKnob>(Vec(91.f, 54.f), module, WorldDrum::SPREAD_PARAM));

        // ========== 4 VOICES AREA (Y=118-318，每聲部 50px) ==========
        // 佈局策略：
        // - 只在第一個聲部顯示 FREQ/DECAY 標籤
        // - 聲部標籤使用動態彩色（左側）
        // - 每個聲部：TRIG | FREQ旋鈕 | DECAY旋鈕 | VEL
        //                  | FREQ CV  | DECAY CV  |

        const float voiceBaseY[4] = {118, 168, 218, 268};  // 每聲部起始 Y
        const char* voiceLabels[4] = {"Timeline", "Foundation", "Groove", "Lead"};

        // 只在第一個聲部上方顯示控制標籤（避免重複）
        addChild(new WorldDrumTextLabel(Vec(31, 106), Vec(30, 15), "FREQ", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new WorldDrumTextLabel(Vec(61, 106), Vec(30, 15), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new WorldDrumTextLabel(Vec(91, 106), Vec(30, 15), "VEL", 8.f, nvgRGB(255, 255, 255), true));

        for (int v = 0; v < 4; v++) {
            float baseY = voiceBaseY[v];
            float row1Y = baseY + 12;  // 主控制行（TRIG/旋鈕/VEL）
            float row2Y = baseY + 37;  // CV 輸入行

            // 動態多彩色聲部標籤（左側）
            WDDynamicRoleTitle* roleTitle = new WDDynamicRoleTitle(
                Vec(3, baseY), Vec(55, 15), voiceLabels[v], 7.f, true
            );
            roleTitle->module = module;
            addChild(roleTitle);

            // Row 1: TRIG | FREQ 旋鈕 | DECAY 旋鈕 | VEL
            float trigX = 15;
            float freqX = 46;
            float decayX = 76;
            float velX = 106;

            addInput(createInputCentered<PJ301MPort>(Vec(trigX, row1Y), module, WorldDrum::TRIG_INPUT_TL + v));
            addParam(createParamCentered<SmallGrayKnob>(Vec(freqX, row1Y), module, WorldDrum::FREQ_PARAM_TL + v));
            addParam(createParamCentered<SmallGrayKnob>(Vec(decayX, row1Y), module, WorldDrum::DECAY_PARAM_TL + v));
            addInput(createInputCentered<PJ301MPort>(Vec(velX, row1Y), module, WorldDrum::VEL_INPUT_TL + v));

            // Row 2: FREQ CV | DECAY CV（對齊旋鈕下方）
            addInput(createInputCentered<PJ301MPort>(Vec(freqX, row2Y), module, WorldDrum::FREQ_CV_INPUT_TL + v));
            addInput(createInputCentered<PJ301MPort>(Vec(decayX, row2Y), module, WorldDrum::DECAY_CV_INPUT_TL + v));
        }

        // ========== WHITE BOTTOM PANEL (Y=330-380) ==========
        addChild(new WorldDrumWhitePanel(Vec(0, 330), Vec(box.size.x, 50)));

        // 輸出標籤（上方，確保不被端口遮住）
        addChild(new WorldDrumTextLabel(Vec(0, 319), Vec(box.size.x, 15), "AUDIO OUT", 7.f, nvgRGB(255, 255, 255), true));

        // Row 1 (Y=343): 4 個聲部獨立輸出
        const float audioX[4] = {30.f, 55.f, 80.f, 105.f};
        for (int i = 0; i < 4; i++) {
            addOutput(createOutputCentered<PJ301MPort>(Vec(audioX[i], 343.f), module, WorldDrum::AUDIO_OUTPUT_TL + i));
        }

        // Row 2 (Y=368): MIX L/R（粉紅色標籤）
        addChild(new WorldDrumTextLabel(Vec(28, 357), Vec(15, 10), "L", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new WorldDrumTextLabel(Vec(53, 357), Vec(15, 10), "R", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(36.f, 368.f), module, WorldDrum::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(61.f, 368.f), module, WorldDrum::MIX_R_OUTPUT));
    }

    void step() override {
        WorldDrum* m = dynamic_cast<WorldDrum*>(module);
        if (m) {
            panelHelper.step(m);
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
    NVGcolor color = nvgRGB(255, 255, 255);  // Default white

    if (module) {
        float styleValue = module->params[WorldDrum::STYLE_PARAM].getValue();
        if (module->inputs[WorldDrum::STYLE_CV_INPUT].isConnected()) {
            styleValue += module->inputs[WorldDrum::STYLE_CV_INPUT].getVoltage();
        }
        int styleIndex = clamp((int)std::round(styleValue), 0, 9);
        color = STYLE_COLORS[styleIndex];
    }

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    // Draw subtle white outline for better visibility (reduced from 0.6f to 0.4f)
    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
    for (float dx = -0.4f; dx <= 0.4f; dx += 0.4f) {
        for (float dy = -0.4f; dy <= 0.4f; dy += 0.4f) {
            if (dx != 0 || dy != 0) {
                nvgText(args.vg, 1.f + dx, box.size.y / 2.f + dy, text.c_str(), NULL);
            }
        }
    }

    // Draw main text with color
    if (bold) {
        nvgFillColor(args.vg, color);
        nvgText(args.vg, 1.f, box.size.y / 2.f, text.c_str(), NULL);
        nvgStrokeColor(args.vg, color);
        nvgStrokeWidth(args.vg, 0.25f);
        nvgText(args.vg, 1.f, box.size.y / 2.f, text.c_str(), NULL);
    } else {
        nvgFillColor(args.vg, color);
        nvgText(args.vg, 1.f, box.size.y / 2.f, text.c_str(), NULL);
    }
}

Model* modelWorldDrum = createModel<WorldDrum, WorldDrumWidget>("WorldDrum");
