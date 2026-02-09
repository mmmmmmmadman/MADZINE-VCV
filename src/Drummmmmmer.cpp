/**
 * Drummmmmmer.cpp
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

// ============================================================================
// DrummerSynth - 8 voice drum synth (2 voices per role)
// ============================================================================

class DrummerSynth {
private:
    MinimalVoice voices[8];
    float sampleRate = 44100.0f;

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        for (int i = 0; i < 8; i++) voices[i].setSampleRate(sr);
    }

    void setVoiceParams(int voice, SynthMode mode, float freq, float decay) {
        if (voice < 0 || voice > 7) return;
        voices[voice].setMode(mode);
        voices[voice].setFreq(freq);
        voices[voice].setDecay(decay);
    }

    void triggerVoice(int voice, float velocity = 1.0f) {
        if (voice < 0 || voice > 7) return;
        voices[voice].trigger(velocity);
    }

    float processVoice(int voice) {
        if (voice < 0 || voice > 7) return 0.0f;
        return voices[voice].process();
    }
};

// ============================================================================
// 8-voice style presets (from UniRhythm ExtendedStylePreset)
// ============================================================================

struct DrummerStylePreset {
    struct VoicePreset {
        SynthMode mode;
        float freq;
        float decay;
        const char* name;
    };
    VoicePreset voices[8];
};

// Voice assignments: 0-1=Timeline, 2-3=Foundation, 4-5=Groove, 6-7=Lead
static const DrummerStylePreset DRUMMER_PRESETS[10] = {
    // 0: West African
    {{{SynthMode::SINE, 4500.0f, 60.0f, "Gankogui"},
      {SynthMode::SINE, 3500.0f, 40.0f, "Bell Lo"},
      {SynthMode::SINE, 80.0f, 200.0f, "Dununba"},
      {SynthMode::SINE, 120.0f, 150.0f, "Dunun"},
      {SynthMode::SINE, 250.0f, 80.0f, "Sangban"},
      {SynthMode::SINE, 300.0f, 60.0f, "Kenkeni"},
      {SynthMode::NOISE, 700.0f, 40.0f, "Djembe Slap"},
      {SynthMode::NOISE, 400.0f, 50.0f, "Djembe Tone"}}},

    // 1: Afro-Cuban
    {{{SynthMode::SINE, 4000.0f, 20.0f, "Clave"},
      {SynthMode::SINE, 2000.0f, 30.0f, "Cowbell"},
      {SynthMode::SINE, 100.0f, 150.0f, "Tumba"},
      {SynthMode::SINE, 150.0f, 120.0f, "Conga Lo"},
      {SynthMode::SINE, 350.0f, 70.0f, "Conga Mid"},
      {SynthMode::SINE, 550.0f, 50.0f, "Quinto"},
      {SynthMode::NOISE, 3000.0f, 40.0f, "Timbales"},
      {SynthMode::NOISE, 5000.0f, 25.0f, "Quinto Slap"}}},

    // 2: Brazilian
    {{{SynthMode::SINE, 4500.0f, 35.0f, "Agogo Hi"},
      {SynthMode::SINE, 3000.0f, 35.0f, "Agogo Lo"},
      {SynthMode::SINE, 55.0f, 250.0f, "Surdo"},
      {SynthMode::SINE, 80.0f, 180.0f, "Surdo 2"},
      {SynthMode::SINE, 400.0f, 40.0f, "Tamborim"},
      {SynthMode::NOISE, 500.0f, 50.0f, "Caixa"},
      {SynthMode::NOISE, 6000.0f, 30.0f, "Ganza"},
      {SynthMode::NOISE, 8000.0f, 20.0f, "Chocalho"}}},

    // 3: Balkan
    {{{SynthMode::NOISE, 4000.0f, 25.0f, "Rim"},
      {SynthMode::NOISE, 3500.0f, 15.0f, "Click"},
      {SynthMode::SINE, 90.0f, 180.0f, "Tapan Bass"},
      {SynthMode::SINE, 130.0f, 120.0f, "Tapan Mid"},
      {SynthMode::SINE, 300.0f, 50.0f, "Tarabuka Doum"},
      {SynthMode::SINE, 450.0f, 35.0f, "Tarabuka Tek"},
      {SynthMode::NOISE, 3000.0f, 25.0f, "Tek Hi"},
      {SynthMode::NOISE, 5000.0f, 20.0f, "Ka"}}},

    // 4: Indian
    {{{SynthMode::NOISE, 8000.0f, 150.0f, "Manjira"},
      {SynthMode::NOISE, 6000.0f, 100.0f, "Ghungroo"},
      {SynthMode::SINE, 65.0f, 300.0f, "Baya Ge"},
      {SynthMode::SINE, 90.0f, 200.0f, "Baya Ka"},
      {SynthMode::SINE, 350.0f, 100.0f, "Daya Na"},
      {SynthMode::SINE, 500.0f, 80.0f, "Daya Tin"},
      {SynthMode::NOISE, 1500.0f, 60.0f, "Daya Ti"},
      {SynthMode::NOISE, 2500.0f, 40.0f, "Daya Re"}}},

    // 5: Gamelan
    {{{SynthMode::SINE, 700.0f, 400.0f, "Kenong"},
      {SynthMode::SINE, 600.0f, 350.0f, "Kethuk"},
      {SynthMode::SINE, 90.0f, 800.0f, "Gong"},
      {SynthMode::SINE, 150.0f, 500.0f, "Kempul"},
      {SynthMode::SINE, 800.0f, 200.0f, "Bonang Po"},
      {SynthMode::SINE, 1000.0f, 180.0f, "Bonang Sa"},
      {SynthMode::SINE, 1200.0f, 250.0f, "Gender"},
      {SynthMode::SINE, 1400.0f, 220.0f, "Saron"}}},

    // 6: Jazz
    {{{SynthMode::NOISE, 4500.0f, 120.0f, "Ride"},
      {SynthMode::NOISE, 2500.0f, 80.0f, "Ride Bell"},
      {SynthMode::SINE, 50.0f, 200.0f, "Kick"},
      {SynthMode::SINE, 80.0f, 150.0f, "Kick Ghost"},
      {SynthMode::NOISE, 500.0f, 100.0f, "Snare"},
      {SynthMode::NOISE, 400.0f, 60.0f, "Snare Ghost"},
      {SynthMode::NOISE, 8000.0f, 35.0f, "HiHat Cl"},
      {SynthMode::NOISE, 6000.0f, 150.0f, "HiHat Op"}}},

    // 7: Electronic
    {{{SynthMode::NOISE, 9000.0f, 30.0f, "HiHat"},
      {SynthMode::NOISE, 12000.0f, 20.0f, "HiHat Ac"},
      {SynthMode::SINE, 45.0f, 280.0f, "808 Kick"},
      {SynthMode::SINE, 60.0f, 200.0f, "Kick 2"},
      {SynthMode::NOISE, 1500.0f, 70.0f, "Clap"},
      {SynthMode::NOISE, 2500.0f, 50.0f, "Snare"},
      {SynthMode::NOISE, 6000.0f, 150.0f, "Open HH"},
      {SynthMode::SINE, 800.0f, 100.0f, "Perc"}}},

    // 8: Breakbeat
    {{{SynthMode::NOISE, 8000.0f, 25.0f, "HiHat"},
      {SynthMode::NOISE, 10000.0f, 15.0f, "HiHat Ac"},
      {SynthMode::SINE, 55.0f, 180.0f, "Kick"},
      {SynthMode::SINE, 70.0f, 120.0f, "Kick Gho"},
      {SynthMode::NOISE, 2500.0f, 80.0f, "Snare"},
      {SynthMode::NOISE, 2000.0f, 50.0f, "Snare Gh"},
      {SynthMode::NOISE, 4000.0f, 40.0f, "Ghost"},
      {SynthMode::NOISE, 6000.0f, 100.0f, "Open HH"}}},

    // 9: Techno
    {{{SynthMode::NOISE, 10000.0f, 20.0f, "HiHat"},
      {SynthMode::NOISE, 12000.0f, 12.0f, "HiHat Ac"},
      {SynthMode::SINE, 42.0f, 250.0f, "909 Kick"},
      {SynthMode::SINE, 55.0f, 180.0f, "Kick Lay"},
      {SynthMode::NOISE, 1800.0f, 55.0f, "Clap"},
      {SynthMode::NOISE, 3000.0f, 35.0f, "Rim"},
      {SynthMode::NOISE, 5000.0f, 80.0f, "Open HH"},
      {SynthMode::SINE, 600.0f, 60.0f, "Tom"}}}
};

inline void applyDrummerPreset(DrummerSynth& synth, int styleIndex) {
    if (styleIndex < 0 || styleIndex > 9) return;
    const DrummerStylePreset& preset = DRUMMER_PRESETS[styleIndex];
    for (int i = 0; i < 8; i++) {
        synth.setVoiceParams(i, preset.voices[i].mode,
                             preset.voices[i].freq, preset.voices[i].decay);
    }
}

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

struct Drummmmmmer : Module {
    enum ParamId {
        STYLE_PARAM,
        SPREAD_PARAM,
        VOICE_PARAM,
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

    // Drum synthesizer engine (8 voices: 2 per role)
    DrummerSynth drumSynth;

    // Voice selection RNG
    std::mt19937 voiceRng{std::random_device{}()};
    std::uniform_real_distribution<float> voiceDist{0.0f, 1.0f};

    // Trigger detection (Schmitt triggers)
    dsp::SchmittTrigger trigSchmitt[4];

    // Current style index
    int currentStyle = 0;

    // Last triggered voice index per role (0=v1, 1=v2)
    int lastTriggeredVoice[4] = {0, 0, 0, 0};

    // CV modulation display values
    float styleCvMod = 0.0f;
    float freqCvMod[4] = {};
    float decayCvMod[4] = {};

    // Panel theme
    int panelTheme = -1;
    float panelContrast = 255.0f;

    Drummmmmmer() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Global parameters
        configParam<WDStyleParamQuantity>(STYLE_PARAM, 0.f, 9.f, 0.f, "Style");
        getParamQuantity(STYLE_PARAM)->snapEnabled = true;
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.5f, "Stereo Spread", "%", 0.f, 100.f);
        configParam(VOICE_PARAM, 0.f, 1.f, 0.f, "Voice Variation", "%", 0.f, 100.f);

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
        applyDrummerPreset(drumSynth, 0);

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
            applyDrummerPreset(drumSynth, currentStyle);
        }

        // Get base preset for parameter modulation (8 voices)
        const DrummerStylePreset& preset = DRUMMER_PRESETS[currentStyle];

        // Voice variation probability
        float voiceProb = params[VOICE_PARAM].getValue();

        // Process each role (4 roles × 2 voices each)
        float voiceOutputs[4];

        for (int v = 0; v < 4; v++) {
            int v1 = v * 2;
            int v2 = v * 2 + 1;

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

            // Apply modulation to both voices (each with its own base freq/decay)
            float modFreq1 = preset.voices[v1].freq * std::pow(2.f, freqParam);
            float modDecay1 = preset.voices[v1].decay * decayParam;
            float modFreq2 = preset.voices[v2].freq * std::pow(2.f, freqParam);
            float modDecay2 = preset.voices[v2].decay * decayParam;

            drumSynth.setVoiceParams(v1, preset.voices[v1].mode, modFreq1, modDecay1);
            drumSynth.setVoiceParams(v2, preset.voices[v2].mode, modFreq2, modDecay2);

            // Trigger detection — select v1 or v2 based on VOICE probability
            if (inputs[TRIG_INPUT_TL + v].isConnected()) {
                if (trigSchmitt[v].process(inputs[TRIG_INPUT_TL + v].getVoltage(), 0.1f, 2.f)) {
                    float velocity = 1.0f;
                    if (inputs[VEL_INPUT_TL + v].isConnected()) {
                        velocity = clamp(inputs[VEL_INPUT_TL + v].getVoltage() / 10.f, 0.f, 1.f);
                    }

                    bool useV2 = voiceDist(voiceRng) < voiceProb;
                    lastTriggeredVoice[v] = useV2 ? 1 : 0;
                    drumSynth.triggerVoice(useV2 ? v2 : v1, velocity);
                }
            }

            // Process both voices and sum (one decaying, one possibly fresh)
            voiceOutputs[v] = drumSynth.processVoice(v1) + drumSynth.processVoice(v2);
        }

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
struct DrummmmmmerTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    int align;

    DrummmmmmerTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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
struct Drummmmmmer;

// Dynamic role title that changes color based on global style (left-aligned, compact for 8HP)
struct WDDynamicRoleTitle : TransparentWidget {
    Drummmmmmer* module = nullptr;
    int roleIndex = -1;  // 0=TL, 1=FD, 2=GR, 3=LD. -1=static text mode
    std::string text;    // fallback static text (used when roleIndex < 0)
    float fontSize;
    bool bold;

    WDDynamicRoleTitle(Vec pos, Vec size, std::string text, float fontSize = 6.5f, bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override;  // Implemented after Drummmmmmer definition
};

// White Bottom Panel (Y >= 330)
struct DrummmmmmerWhitePanel : Widget {
    DrummmmmmerWhitePanel(Vec pos, Vec size) {
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
    Drummmmmmer* module = nullptr;

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

struct DrummmmmmerWidget : ModuleWidget {
    PanelThemeHelper panelHelper;
    TechnoSnapKnob30* styleKnob = nullptr;
    MediumGrayKnob* freqKnobs[4] = {};
    MediumGrayKnob* decayKnobs[4] = {};

    DrummmmmmerWidget(Drummmmmmer* module) {
        setModule(module);
        panelHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // ========== TITLE AREA (Y=0-26) ==========
        addChild(new DrummmmmmerTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Drummmmmmer", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new DrummmmmmerTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // ========== WHITE BOTTOM PANEL (Y=330-380) ==========
        addChild(new DrummmmmmerWhitePanel(Vec(0, 330), Vec(box.size.x, 50)));

        // ========== PHASE 1: ALL PORTS AND KNOBS (z-order bottom) ==========

        // --- GLOBAL CONTROLS: STYLE knob (left) + StyleDisplay (center) + CV port (right), same row ---
        // STYLE knob (TechnoSnapKnob30, 30px) X=18, Y=56
        styleKnob = createParamCentered<TechnoSnapKnob30>(Vec(18.f, 56.f), module, Drummmmmmer::STYLE_PARAM);
        addParam(styleKnob);
        // STYLE CV port (24px) X=106, Y=56
        addInput(createInputCentered<PJ301MPort>(Vec(106.f, 56.f), module, Drummmmmmer::STYLE_CV_INPUT));

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
            addInput(createInputCentered<PJ301MPort>(Vec(trigX, sY), module, Drummmmmmer::TRIG_INPUT_TL + vi));
            freqKnobs[vi] = createParamCentered<MediumGrayKnob>(Vec(freqX, sY), module, Drummmmmmer::FREQ_PARAM_TL + vi);
            addParam(freqKnobs[vi]);
            decayKnobs[vi] = createParamCentered<MediumGrayKnob>(Vec(decayX, sY), module, Drummmmmmer::DECAY_PARAM_TL + vi);
            addParam(decayKnobs[vi]);
            addOutput(createOutputCentered<PJ301MPort>(Vec(outX, sY), module, Drummmmmmer::AUDIO_OUTPUT_TL + vi));

            // Row 2 (sY+26): VEL port | FREQ CV port | DECAY CV port
            addInput(createInputCentered<PJ301MPort>(Vec(trigX, sY + 26.f), module, Drummmmmmer::VEL_INPUT_TL + vi));
            addInput(createInputCentered<PJ301MPort>(Vec(freqX, sY + 26.f), module, Drummmmmmer::FREQ_CV_INPUT_TL + vi));
            addInput(createInputCentered<PJ301MPort>(Vec(decayX, sY + 26.f), module, Drummmmmmer::DECAY_CV_INPUT_TL + vi));
        }

        // --- WHITE OUTPUT AREA (Y=330-380) ---
        // SPREAD knob (MediumGrayKnob, 26px) X=trigX(15), Y=355
        addParam(createParamCentered<MediumGrayKnob>(Vec(15.f, 355.f), module, Drummmmmmer::SPREAD_PARAM));
        // VOICE knob (MediumGrayKnob, 26px) X=freqX(43), Y=355
        addParam(createParamCentered<MediumGrayKnob>(Vec(43.f, 355.f), module, Drummmmmmer::VOICE_PARAM));
        // MIX L output X=decayX(73), Y=355
        addOutput(createOutputCentered<PJ301MPort>(Vec(73.f, 355.f), module, Drummmmmmer::MIX_L_OUTPUT));
        // MIX R output X=103, Y=355
        addOutput(createOutputCentered<PJ301MPort>(Vec(103.f, 355.f), module, Drummmmmmer::MIX_R_OUTPUT));

        // ========== PHASE 2: ALL LABELS AND DISPLAYS (z-order top) ==========

        // --- GLOBAL LABELS ---
        // STYLE label (8.f bold white) above knob at X=18 (offset 28px for 30px knob: Y=56-28=28)
        addChild(new DrummmmmmerTextLabel(Vec(3, 28), Vec(30, 15), "STYLE", 8.f, nvgRGB(255, 255, 255), true));
        // CV label (8.f bold white) above port at X=106 (offset 24px for port: Y=56-24=32)
        addChild(new DrummmmmmerTextLabel(Vec(94, 32), Vec(24, 15), "CV", 8.f, nvgRGB(255, 255, 255), true));

        // --- StyleDisplay between knob and CV, same row (11.f colorful dynamic) ---
        StyleDisplay* styleDisp = new StyleDisplay(Vec(35, 48), Vec(56, 16));
        styleDisp->module = module;
        addChild(styleDisp);

        // --- COLUMN HEADERS (Y=72, aligned with voice columns) ---
        addChild(new DrummmmmmerTextLabel(Vec(33, 72), Vec(20, 15), "FREQ", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new DrummmmmmerTextLabel(Vec(60, 72), Vec(26, 15), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new DrummmmmmerTextLabel(Vec(93, 72), Vec(20, 15), "OUT", 8.f, nvgRGB(255, 255, 255), true));

        // --- VOICE NAMES (dynamic, shows current triggered voice name) ---
        {
            float nameX = 88.5f;
            float nameW = 29.f;
            for (int v = 0; v < 4; v++) {
                float row2Y = startY[v] + 26.f;
                int vi = voiceMap[v];  // map UI row to internal role
                auto* label = new WDDynamicRoleTitle(Vec(nameX, row2Y - 14.f), Vec(nameW, 28.f), "", 9.f, true);
                label->module = module;
                label->roleIndex = vi;
                addChild(label);
            }
        }

        // --- WHITE AREA LABELS (Y>=330, pink 7.f) ---
        // SPREAD label centered on X=15
        addChild(new DrummmmmmerTextLabel(Vec(0, 331), Vec(30, 15), "SPREAD", 7.f, nvgRGB(255, 133, 133), true));
        // VOICE label centered on X=43
        addChild(new DrummmmmmerTextLabel(Vec(28, 331), Vec(30, 15), "VOICE", 7.f, nvgRGB(255, 133, 133), true));
        // L label
        addChild(new DrummmmmmerTextLabel(Vec(61, 331), Vec(24, 15), "L", 7.f, nvgRGB(255, 133, 133), true));
        // R label
        addChild(new DrummmmmmerTextLabel(Vec(91, 331), Vec(24, 15), "R", 7.f, nvgRGB(255, 133, 133), true));
    }

    void step() override {
        Drummmmmmer* m = dynamic_cast<Drummmmmmer*>(module);
        if (m) {
            panelHelper.step(m);

            if (styleKnob) {
                bool connected = m->inputs[Drummmmmmer::STYLE_CV_INPUT].isConnected();
                styleKnob->setModulationEnabled(connected);
                if (connected) styleKnob->setModulation(m->styleCvMod);
            }
            for (int v = 0; v < 4; v++) {
                if (freqKnobs[v]) {
                    bool connected = m->inputs[Drummmmmmer::FREQ_CV_INPUT_TL + v].isConnected();
                    freqKnobs[v]->setModulationEnabled(connected);
                    if (connected) freqKnobs[v]->setModulation(m->freqCvMod[v]);
                }
                if (decayKnobs[v]) {
                    bool connected = m->inputs[Drummmmmmer::DECAY_CV_INPUT_TL + v].isConnected();
                    decayKnobs[v]->setModulationEnabled(connected);
                    if (connected) decayKnobs[v]->setModulation(m->decayCvMod[v]);
                }
            }
        }
        ModuleWidget::step();
    }

    void appendContextMenu(Menu* menu) override {
        Drummmmmmer* module = dynamic_cast<Drummmmmmer*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

// ============================================================================
// WDDynamicRoleTitle::draw implementation (after Drummmmmmer definition)
// ============================================================================

void WDDynamicRoleTitle::draw(const DrawArgs &args) {
    NVGcolor color = nvgRGB(255, 255, 255);

    // Determine display text
    std::string displayText = text;
    if (module) {
        color = STYLE_COLORS[module->currentStyle];
        if (roleIndex >= 0 && roleIndex < 4) {
            int voiceIdx = roleIndex * 2 + module->lastTriggeredVoice[roleIndex];
            displayText = DRUMMER_PRESETS[module->currentStyle].voices[voiceIdx].name;
        }
    }

    float cx = box.size.x / 2.f;

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // Check if name contains a space (split into two lines if so)
    std::string line1, line2;
    size_t spacePos = displayText.find(' ');
    bool twoLines = (spacePos != std::string::npos);

    if (twoLines) {
        line1 = displayText.substr(0, spacePos);
        line2 = displayText.substr(spacePos + 1);
    } else {
        line1 = displayText;
    }

    auto drawText = [&](const std::string& txt, float y) {
        // Colored glow
        nvgFontBlur(args.vg, 3.0f);
        nvgFillColor(args.vg, color);
        nvgText(args.vg, cx, y, txt.c_str(), NULL);
        nvgText(args.vg, cx, y, txt.c_str(), NULL);
        // White text
        nvgFontBlur(args.vg, 0.f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgText(args.vg, cx, y, txt.c_str(), NULL);
    };

    if (twoLines) {
        float lineH = fontSize + 1.f;
        float cy = box.size.y / 2.f;
        drawText(line1, cy - lineH * 0.5f);
        drawText(line2, cy + lineH * 0.5f);
    } else {
        drawText(line1, box.size.y / 2.f);
    }
}

Model* modelDrummmmmmer = createModel<Drummmmmmer, DrummmmmmerWidget>("Drummmmmmer");
