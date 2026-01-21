#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "WorldRhythm/PatternGenerator.hpp"
#include "WorldRhythm/StyleProfiles.hpp"
#include "WorldRhythm/MinimalDrumSynth.hpp"
#include <vector>
#include <algorithm>

// ============================================================================
// Uni Rhythm Module - 16HP
// Compact 4-track version of Universal Rhythm
// 4 merged outputs (Primary priority) with 8-voice pattern generation
// ============================================================================

// Style colors
static const NVGcolor UR_STYLE_COLORS[10] = {
    nvgRGB(255, 120, 100),  // 0: West African
    nvgRGB(100, 200, 255),  // 1: Afro-Cuban
    nvgRGB(255, 200, 80),   // 2: Brazilian
    nvgRGB(200, 100, 150),  // 3: Balkan
    nvgRGB(255, 150, 200),  // 4: Indian
    nvgRGB(150, 220, 180),  // 5: Gamelan
    nvgRGB(180, 150, 255),  // 6: Jazz
    nvgRGB(100, 220, 220),  // 7: Electronic
    nvgRGB(255, 180, 100),  // 8: Breakbeat
    nvgRGB(220, 220, 220),  // 9: Techno
};

// ============================================================================
// Helper Widgets
// ============================================================================

struct URCompactTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    URCompactTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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

struct URCompactWhiteBox : Widget {
    URCompactWhiteBox(Vec pos, Vec size) {
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

// Forward declaration
struct UniRhythm;

// Dynamic role title with style color
struct URCompactRoleTitle : TransparentWidget {
    UniRhythm* module = nullptr;
    int roleIndex = 0;
    std::string text;
    float fontSize;

    URCompactRoleTitle(Vec pos, Vec size, std::string text, int roleIndex, float fontSize = 9.f) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->roleIndex = roleIndex;
        this->fontSize = fontSize;
    }

    void draw(const DrawArgs &args) override;
};

// ============================================================================
// Extended Drum Synth - 8 voices (same as UniversalRhythm)
// ============================================================================

namespace unirhythm {

using namespace worldrhythm;

class ExtendedDrumSynth {
private:
    MinimalVoice voices[8];
    float sampleRate = 44100.0f;

public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        for (int i = 0; i < 8; i++) {
            voices[i].setSampleRate(sr);
        }
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

// 8-voice style presets (same as UniversalRhythm)
struct ExtendedStylePreset {
    struct VoicePreset {
        SynthMode mode;
        float freq;
        float decay;
    };
    VoicePreset voices[8];
};

const ExtendedStylePreset EXTENDED_PRESETS[10] = {
    // 0: West African
    {{{SynthMode::SINE, 4500.0f, 60.0f}, {SynthMode::SINE, 3500.0f, 40.0f},
      {SynthMode::SINE, 80.0f, 200.0f}, {SynthMode::SINE, 120.0f, 150.0f},
      {SynthMode::SINE, 250.0f, 80.0f}, {SynthMode::SINE, 300.0f, 60.0f},
      {SynthMode::NOISE, 700.0f, 40.0f}, {SynthMode::NOISE, 400.0f, 50.0f}}},
    // 1: Afro-Cuban
    {{{SynthMode::SINE, 4000.0f, 20.0f}, {SynthMode::SINE, 2000.0f, 30.0f},
      {SynthMode::SINE, 100.0f, 150.0f}, {SynthMode::SINE, 150.0f, 120.0f},
      {SynthMode::SINE, 350.0f, 70.0f}, {SynthMode::SINE, 550.0f, 50.0f},
      {SynthMode::NOISE, 3000.0f, 40.0f}, {SynthMode::NOISE, 5000.0f, 25.0f}}},
    // 2: Brazilian
    {{{SynthMode::SINE, 4500.0f, 35.0f}, {SynthMode::SINE, 3000.0f, 35.0f},
      {SynthMode::SINE, 55.0f, 250.0f}, {SynthMode::SINE, 80.0f, 180.0f},
      {SynthMode::SINE, 400.0f, 40.0f}, {SynthMode::NOISE, 500.0f, 50.0f},
      {SynthMode::NOISE, 6000.0f, 30.0f}, {SynthMode::NOISE, 8000.0f, 20.0f}}},
    // 3: Balkan
    {{{SynthMode::NOISE, 4000.0f, 25.0f}, {SynthMode::NOISE, 3500.0f, 15.0f},
      {SynthMode::SINE, 90.0f, 180.0f}, {SynthMode::SINE, 130.0f, 120.0f},
      {SynthMode::SINE, 300.0f, 50.0f}, {SynthMode::SINE, 450.0f, 35.0f},
      {SynthMode::NOISE, 3000.0f, 25.0f}, {SynthMode::NOISE, 5000.0f, 20.0f}}},
    // 4: Indian
    {{{SynthMode::NOISE, 8000.0f, 150.0f}, {SynthMode::NOISE, 6000.0f, 100.0f},
      {SynthMode::SINE, 65.0f, 300.0f}, {SynthMode::SINE, 90.0f, 200.0f},
      {SynthMode::SINE, 350.0f, 100.0f}, {SynthMode::SINE, 500.0f, 80.0f},
      {SynthMode::NOISE, 1500.0f, 60.0f}, {SynthMode::NOISE, 2500.0f, 40.0f}}},
    // 5: Gamelan
    {{{SynthMode::SINE, 700.0f, 400.0f}, {SynthMode::SINE, 600.0f, 350.0f},
      {SynthMode::SINE, 90.0f, 800.0f}, {SynthMode::SINE, 150.0f, 500.0f},
      {SynthMode::SINE, 800.0f, 200.0f}, {SynthMode::SINE, 1000.0f, 180.0f},
      {SynthMode::SINE, 1200.0f, 250.0f}, {SynthMode::SINE, 1400.0f, 220.0f}}},
    // 6: Jazz
    {{{SynthMode::NOISE, 4500.0f, 120.0f}, {SynthMode::NOISE, 2500.0f, 80.0f},
      {SynthMode::SINE, 50.0f, 200.0f}, {SynthMode::SINE, 80.0f, 150.0f},
      {SynthMode::NOISE, 500.0f, 100.0f}, {SynthMode::NOISE, 400.0f, 60.0f},
      {SynthMode::NOISE, 8000.0f, 35.0f}, {SynthMode::NOISE, 6000.0f, 150.0f}}},
    // 7: Electronic
    {{{SynthMode::NOISE, 9000.0f, 30.0f}, {SynthMode::NOISE, 12000.0f, 20.0f},
      {SynthMode::SINE, 45.0f, 280.0f}, {SynthMode::SINE, 60.0f, 200.0f},
      {SynthMode::NOISE, 1500.0f, 70.0f}, {SynthMode::NOISE, 2500.0f, 50.0f},
      {SynthMode::NOISE, 6000.0f, 150.0f}, {SynthMode::SINE, 800.0f, 100.0f}}},
    // 8: Breakbeat
    {{{SynthMode::NOISE, 8000.0f, 25.0f}, {SynthMode::NOISE, 10000.0f, 15.0f},
      {SynthMode::SINE, 55.0f, 180.0f}, {SynthMode::SINE, 70.0f, 120.0f},
      {SynthMode::NOISE, 2500.0f, 80.0f}, {SynthMode::NOISE, 2000.0f, 50.0f},
      {SynthMode::NOISE, 4000.0f, 40.0f}, {SynthMode::NOISE, 6000.0f, 100.0f}}},
    // 9: Techno
    {{{SynthMode::NOISE, 10000.0f, 20.0f}, {SynthMode::NOISE, 12000.0f, 12.0f},
      {SynthMode::SINE, 42.0f, 250.0f}, {SynthMode::SINE, 55.0f, 180.0f},
      {SynthMode::NOISE, 1800.0f, 55.0f}, {SynthMode::NOISE, 3000.0f, 35.0f},
      {SynthMode::NOISE, 5000.0f, 80.0f}, {SynthMode::SINE, 600.0f, 60.0f}}}
};

inline void applyRolePreset(ExtendedDrumSynth& synth, int role, int styleIndex) {
    if (styleIndex < 0 || styleIndex > 9) return;
    if (role < 0 || role > 3) return;
    const ExtendedStylePreset& preset = EXTENDED_PRESETS[styleIndex];
    int voiceBase = role * 2;
    synth.setVoiceParams(voiceBase, preset.voices[voiceBase].mode,
                         preset.voices[voiceBase].freq, preset.voices[voiceBase].decay);
    synth.setVoiceParams(voiceBase + 1, preset.voices[voiceBase + 1].mode,
                         preset.voices[voiceBase + 1].freq, preset.voices[voiceBase + 1].decay);
}

} // namespace unirhythm

// ============================================================================
// Pattern storage for 8 voices
// ============================================================================

struct URMultiVoicePatterns {
    WorldRhythm::Pattern patterns[8];

    URMultiVoicePatterns() {
        for (int i = 0; i < 8; i++) {
            patterns[i] = WorldRhythm::Pattern(16);
        }
    }

    void clear() {
        for (int i = 0; i < 8; i++) {
            patterns[i].clear();
        }
    }
};

// ============================================================================
// Uni Rhythm Module
// ============================================================================

struct UniRhythm : Module {
    int panelTheme = -1;
    float panelContrast = panelContrastDefault;

    enum ParamId {
        // Per-role parameters (4 roles x 5 params: Style, Density, Length, Freq, Decay)
        TIMELINE_STYLE_PARAM,
        TIMELINE_DENSITY_PARAM,
        TIMELINE_LENGTH_PARAM,
        TIMELINE_FREQ_PARAM,
        TIMELINE_DECAY_PARAM,
        FOUNDATION_STYLE_PARAM,
        FOUNDATION_DENSITY_PARAM,
        FOUNDATION_LENGTH_PARAM,
        FOUNDATION_FREQ_PARAM,
        FOUNDATION_DECAY_PARAM,
        GROOVE_STYLE_PARAM,
        GROOVE_DENSITY_PARAM,
        GROOVE_LENGTH_PARAM,
        GROOVE_FREQ_PARAM,
        GROOVE_DECAY_PARAM,
        LEAD_STYLE_PARAM,
        LEAD_DENSITY_PARAM,
        LEAD_LENGTH_PARAM,
        LEAD_FREQ_PARAM,
        LEAD_DECAY_PARAM,
        // Global parameter (integrates Humanize, Swing, Rest, Fill, Articulation, Spread)
        VARIATION_PARAM,
        // Buttons
        REGENERATE_PARAM,
        RESET_BUTTON_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        REGENERATE_INPUT,
        // Per-role CV inputs (4 roles x 5 CVs)
        TIMELINE_STYLE_CV_INPUT,
        TIMELINE_DENSITY_CV_INPUT,
        TIMELINE_LENGTH_CV_INPUT,
        TIMELINE_FREQ_CV_INPUT,
        TIMELINE_DECAY_CV_INPUT,
        FOUNDATION_STYLE_CV_INPUT,
        FOUNDATION_DENSITY_CV_INPUT,
        FOUNDATION_LENGTH_CV_INPUT,
        FOUNDATION_FREQ_CV_INPUT,
        FOUNDATION_DECAY_CV_INPUT,
        GROOVE_STYLE_CV_INPUT,
        GROOVE_DENSITY_CV_INPUT,
        GROOVE_LENGTH_CV_INPUT,
        GROOVE_FREQ_CV_INPUT,
        GROOVE_DECAY_CV_INPUT,
        LEAD_STYLE_CV_INPUT,
        LEAD_DENSITY_CV_INPUT,
        LEAD_LENGTH_CV_INPUT,
        LEAD_FREQ_CV_INPUT,
        LEAD_DECAY_CV_INPUT,
        // Global CV input
        VARIATION_CV_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        // Per-role outputs (4 roles x 4 outputs: Audio, Gate, Pitch CV, Velocity Envelope)
        TIMELINE_AUDIO_OUTPUT,
        TIMELINE_GATE_OUTPUT,
        TIMELINE_PITCH_OUTPUT,
        TIMELINE_VELENV_OUTPUT,
        FOUNDATION_AUDIO_OUTPUT,
        FOUNDATION_GATE_OUTPUT,
        FOUNDATION_PITCH_OUTPUT,
        FOUNDATION_VELENV_OUTPUT,
        GROOVE_AUDIO_OUTPUT,
        GROOVE_GATE_OUTPUT,
        GROOVE_PITCH_OUTPUT,
        GROOVE_VELENV_OUTPUT,
        LEAD_AUDIO_OUTPUT,
        LEAD_GATE_OUTPUT,
        LEAD_PITCH_OUTPUT,
        LEAD_VELENV_OUTPUT,
        // Mix outputs
        MIX_L_OUTPUT,
        MIX_R_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        TIMELINE_LIGHT,
        FOUNDATION_LIGHT,
        GROOVE_LIGHT,
        LEAD_LIGHT,
        CLOCK_LIGHT,
        LIGHTS_LEN
    };

    // Engines
    WorldRhythm::PatternGenerator patternGen;
    unirhythm::ExtendedDrumSynth drumSynth;

    // Pattern storage
    URMultiVoicePatterns patterns;
    int currentSteps[4] = {0, 0, 0, 0};
    int roleLengths[4] = {16, 16, 16, 16};  // Track each role's length
    int currentBar = 0;
    int globalStep = 0;

    // Triggers and pulses
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger resetButtonTrigger;
    dsp::SchmittTrigger regenerateTrigger;
    dsp::SchmittTrigger regenerateButtonTrigger;

    dsp::PulseGenerator gatePulses[4];  // 4 merged outputs
    dsp::PulseGenerator clockPulse;

    // Velocity/Pitch tracking per merged output
    float currentVelocities[4] = {0.f, 0.f, 0.f, 0.f};
    float currentPitches[4] = {0.f, 0.f, 0.f, 0.f};
    bool lastTriggerWasPrimary[4] = {true, true, true, true};

    // Cached synth parameters
    float cachedFreqs[8] = {0};
    float cachedDecays[8] = {0};
    float currentFreqs[8] = {0};

    // Change detection
    int lastStyles[4] = {-1, -1, -1, -1};
    float lastDensities[4] = {-1.f, -1.f, -1.f, -1.f};
    float lastVariation = -1.0f;

    // PPQN setting
    int ppqn = 4;
    int ppqnCounter = 0;

    // Velocity Envelope (same as UniversalRhythm)
    struct VelocityEnvelope {
        enum Phase { IDLE, ATTACK, DECAY };
        Phase phase = IDLE;
        float output = 0.0f;
        float phaseTime = 0.0f;
        float peakVoltage = 0.0f;
        float attackTime = 0.0003f;
        float currentDecayTime = 1.0f;
        float curve = -0.95f;

        float applyCurve(float x, float curvature) {
            x = clamp(x, 0.0f, 1.0f);
            if (curvature == 0.0f) return x;
            float k = curvature;
            float abs_x = std::abs(x);
            float denominator = k - 2.0f * k * abs_x + 1.0f;
            if (std::abs(denominator) < 1e-6f) return x;
            return (x - k * x) / denominator;
        }

        void trigger(float decayParam, float sampleRate, float velocity) {
            peakVoltage = velocity * 8.0f;
            phase = ATTACK;
            phaseTime = 0.0f;
            float sqrtDecay = std::pow(decayParam, 0.33f);
            float mappedDecay = rescale(sqrtDecay, 0.0f, 1.0f, 0.0f, 0.8f);
            currentDecayTime = std::pow(10.0f, (mappedDecay - 0.8f) * 5.0f);
            currentDecayTime = std::max(0.01f, currentDecayTime);
        }

        float process(float sampleTime) {
            switch (phase) {
                case IDLE:
                    output = 0.0f;
                    break;
                case ATTACK:
                    phaseTime += sampleTime;
                    if (phaseTime >= attackTime) {
                        phase = DECAY;
                        phaseTime = 0.0f;
                        output = 1.0f;
                    } else {
                        float t = phaseTime / attackTime;
                        output = applyCurve(t, curve);
                    }
                    break;
                case DECAY:
                    phaseTime += sampleTime;
                    if (phaseTime >= currentDecayTime) {
                        output = 0.0f;
                        phase = IDLE;
                        phaseTime = 0.0f;
                    } else {
                        float t = phaseTime / currentDecayTime;
                        output = 1.0f - applyCurve(t, curve);
                    }
                    break;
            }
            output = clamp(output, 0.0f, 1.0f);
            return output * peakVoltage;
        }
    };

    VelocityEnvelope velocityEnv[4];  // 4 merged outputs

    UniRhythm() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Per-role parameters
        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            std::string roleName;
            switch (role) {
                case 0: roleName = "Timeline"; break;
                case 1: roleName = "Foundation"; break;
                case 2: roleName = "Groove"; break;
                case 3: roleName = "Lead"; break;
            }
            configParam(TIMELINE_STYLE_PARAM + baseParam, 0.f, 9.f, 9.f, roleName + " Style");
            getParamQuantity(TIMELINE_STYLE_PARAM + baseParam)->snapEnabled = true;
            configParam(TIMELINE_DENSITY_PARAM + baseParam, 0.f, 1.f, 0.5f, roleName + " Density", "%", 0.f, 100.f);
            configParam(TIMELINE_LENGTH_PARAM + baseParam, 4.f, 32.f, 16.f, roleName + " Length", " steps");
            getParamQuantity(TIMELINE_LENGTH_PARAM + baseParam)->snapEnabled = true;
            configParam(TIMELINE_FREQ_PARAM + baseParam, -1.f, 1.f, 0.f, roleName + " Freq", "%", 0.f, 100.f);
            configParam(TIMELINE_DECAY_PARAM + baseParam, -1.f, 1.f, 0.f, roleName + " Decay", "%", 0.f, 100.f);
        }

        // Global parameters
        configParam(VARIATION_PARAM, 0.f, 1.f, 0.5f, "Variation", "%", 0.f, 100.f);

        // Buttons
        configButton(REGENERATE_PARAM, "Regenerate");
        configButton(RESET_BUTTON_PARAM, "Reset");

        // Inputs
        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(REGENERATE_INPUT, "Regenerate");

        // Per-role CV inputs
        for (int role = 0; role < 4; role++) {
            int baseCv = role * 5;
            std::string roleName;
            switch (role) {
                case 0: roleName = "Timeline"; break;
                case 1: roleName = "Foundation"; break;
                case 2: roleName = "Groove"; break;
                case 3: roleName = "Lead"; break;
            }
            configInput(TIMELINE_STYLE_CV_INPUT + baseCv, roleName + " Style CV");
            configInput(TIMELINE_DENSITY_CV_INPUT + baseCv, roleName + " Density CV");
            configInput(TIMELINE_LENGTH_CV_INPUT + baseCv, roleName + " Length CV");
            configInput(TIMELINE_FREQ_CV_INPUT + baseCv, roleName + " Freq CV");
            configInput(TIMELINE_DECAY_CV_INPUT + baseCv, roleName + " Decay CV");
        }

        // Global CV input
        configInput(VARIATION_CV_INPUT, "Variation CV");

        // Outputs (4 roles)
        const char* roleNames[4] = {"Timeline", "Foundation", "Groove", "Lead"};
        for (int role = 0; role < 4; role++) {
            configOutput(TIMELINE_AUDIO_OUTPUT + role * 4, std::string(roleNames[role]) + " Audio");
            configOutput(TIMELINE_GATE_OUTPUT + role * 4, std::string(roleNames[role]) + " Gate");
            configOutput(TIMELINE_PITCH_OUTPUT + role * 4, std::string(roleNames[role]) + " Pitch CV");
            configOutput(TIMELINE_VELENV_OUTPUT + role * 4, std::string(roleNames[role]) + " Velocity Env");
        }
        configOutput(MIX_L_OUTPUT, "Mix L");
        configOutput(MIX_R_OUTPUT, "Mix R");
    }

    void onSampleRateChange() override {
        drumSynth.setSampleRate(APP->engine->getSampleRate());
    }

    void onReset() override {
        for (int i = 0; i < 4; i++) {
            currentSteps[i] = 0;
            lastStyles[i] = -1;
            lastDensities[i] = -1.f;
        }
        globalStep = 0;
        currentBar = 0;
        ppqnCounter = 0;
        lastVariation = -1.0f;
        regeneratePatterns();
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_object_set_new(rootJ, "ppqn", json_integer(ppqn));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_real_value(contrastJ);
        json_t* ppqnJ = json_object_get(rootJ, "ppqn");
        if (ppqnJ) ppqn = json_integer_value(ppqnJ);

        regeneratePatterns();
    }

    void regeneratePatterns() {
        float variation = params[VARIATION_PARAM].getValue();

        // Derive parameters from Variation
        float ghostProb = variation * 0.4f;

        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            int styleIdx = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue());
            float density = params[TIMELINE_DENSITY_PARAM + baseParam].getValue();
            int length = static_cast<int>(params[TIMELINE_LENGTH_PARAM + baseParam].getValue());
            length = clamp(length, 4, 32);
            roleLengths[role] = length;

            styleIdx = clamp(styleIdx, 0, 9);

            // Generate patterns for both voices of this role
            int voiceBase = role * 2;
            const WorldRhythm::StyleProfile& style = *WorldRhythm::STYLES[styleIdx];
            WorldRhythm::Role roleType = static_cast<WorldRhythm::Role>(role);

            // Primary voice
            patterns.patterns[voiceBase] = patternGen.generate(
                roleType, style, length, density, variation
            );

            // Secondary voice (interlock)
            patterns.patterns[voiceBase + 1] = patternGen.generateWithInterlock(
                roleType, style, length, density * 0.6f, variation,
                patterns.patterns[voiceBase]
            );

            // Add ghost notes based on variation
            if (ghostProb > 0.1f) {
                addGhostNotes(patterns.patterns[voiceBase], ghostProb);
                addGhostNotes(patterns.patterns[voiceBase + 1], ghostProb * 0.7f);
            }

            // Apply style preset to synth
            unirhythm::applyRolePreset(drumSynth, role, styleIdx);

            // Cache base frequencies and decays
            const auto& preset = unirhythm::EXTENDED_PRESETS[styleIdx];
            cachedFreqs[voiceBase] = preset.voices[voiceBase].freq;
            cachedFreqs[voiceBase + 1] = preset.voices[voiceBase + 1].freq;
            cachedDecays[voiceBase] = preset.voices[voiceBase].decay;
            cachedDecays[voiceBase + 1] = preset.voices[voiceBase + 1].decay;

            lastStyles[role] = styleIdx;
            lastDensities[role] = density;
        }

        lastVariation = variation;
    }

    void addGhostNotes(WorldRhythm::Pattern& pattern, float probability) {
        for (int i = 0; i < pattern.length; i++) {
            if (!pattern.hasOnsetAt(i)) {
                int prev = (i - 1 + pattern.length) % pattern.length;
                int next = (i + 1) % pattern.length;
                bool nearHit = pattern.hasOnsetAt(prev) || pattern.hasOnsetAt(next);

                float prob = probability;
                if (nearHit) prob *= 2.0f;
                if (i % 2 == 1) prob *= 1.5f;

                if (random::uniform() < prob) {
                    pattern.setOnset(i, 0.25f + random::uniform() * 0.1f);
                }
            }
        }
    }

    void process(const ProcessArgs& args) override {
        // Check for parameter changes
        bool needRegen = false;

        // Get variation (with CV)
        float variation = params[VARIATION_PARAM].getValue();
        if (inputs[VARIATION_CV_INPUT].isConnected()) {
            variation += inputs[VARIATION_CV_INPUT].getVoltage() * 0.1f;
            variation = clamp(variation, 0.f, 1.f);
        }

        if (std::abs(variation - lastVariation) > 0.01f) {
            needRegen = true;
        }

        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            int baseCv = role * 5;

            // Get style (with CV)
            float styleValue = params[TIMELINE_STYLE_PARAM + baseParam].getValue();
            if (inputs[TIMELINE_STYLE_CV_INPUT + baseCv].isConnected()) {
                styleValue += inputs[TIMELINE_STYLE_CV_INPUT + baseCv].getVoltage();
            }
            int styleIdx = static_cast<int>(styleValue);
            styleIdx = clamp(styleIdx, 0, 9);

            // Get density (with CV)
            float density = params[TIMELINE_DENSITY_PARAM + baseParam].getValue();
            if (inputs[TIMELINE_DENSITY_CV_INPUT + baseCv].isConnected()) {
                density += inputs[TIMELINE_DENSITY_CV_INPUT + baseCv].getVoltage() * 0.1f;
                density = clamp(density, 0.f, 1.f);
            }

            if (styleIdx != lastStyles[role] || std::abs(density - lastDensities[role]) > 0.01f) {
                needRegen = true;
            }
        }

        // Handle regenerate trigger
        if (regenerateButtonTrigger.process(params[REGENERATE_PARAM].getValue()) ||
            regenerateTrigger.process(inputs[REGENERATE_INPUT].getVoltage())) {
            needRegen = true;
        }

        if (needRegen) {
            regeneratePatterns();
        }

        // Handle reset
        if (resetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue()) ||
            resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            for (int i = 0; i < 4; i++) {
                currentSteps[i] = 0;
            }
            globalStep = 0;
            currentBar = 0;
            ppqnCounter = 0;
        }

        // Process clock
        if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
            clockPulse.trigger(1e-3f);

            ppqnCounter++;
            if (ppqnCounter >= (4 / ppqn)) {
                ppqnCounter = 0;

                // Process each role with Primary priority merge
                for (int role = 0; role < 4; role++) {
                    int voiceBase = role * 2;
                    int step = currentSteps[role];
                    int length = roleLengths[role];

                    // Check Primary and Secondary patterns
                    bool primaryHit = patterns.patterns[voiceBase].hasOnsetAt(step);
                    bool secondaryHit = patterns.patterns[voiceBase + 1].hasOnsetAt(step);

                    // Primary priority merge
                    bool shouldTrigger = false;
                    float velocity = 0.0f;
                    int sourceVoice = voiceBase;

                    if (primaryHit) {
                        shouldTrigger = true;
                        velocity = patterns.patterns[voiceBase].getVelocity(step);
                        sourceVoice = voiceBase;
                        lastTriggerWasPrimary[role] = true;
                    } else if (secondaryHit) {
                        shouldTrigger = true;
                        velocity = patterns.patterns[voiceBase + 1].getVelocity(step);
                        sourceVoice = voiceBase + 1;
                        lastTriggerWasPrimary[role] = false;
                    }

                    if (shouldTrigger) {
                        // Get Freq/Decay modulation (with CV)
                        int baseParam = role * 5;
                        int baseCv = role * 5;

                        float freqMod = params[TIMELINE_FREQ_PARAM + baseParam].getValue();
                        if (inputs[TIMELINE_FREQ_CV_INPUT + baseCv].isConnected()) {
                            freqMod += inputs[TIMELINE_FREQ_CV_INPUT + baseCv].getVoltage() * 0.1f;
                            freqMod = clamp(freqMod, -1.f, 1.f);
                        }

                        float decayMod = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
                        if (inputs[TIMELINE_DECAY_CV_INPUT + baseCv].isConnected()) {
                            decayMod += inputs[TIMELINE_DECAY_CV_INPUT + baseCv].getVoltage() * 0.1f;
                            decayMod = clamp(decayMod, -1.f, 1.f);
                        }

                        // Apply modulation to synth
                        float baseFreq = cachedFreqs[sourceVoice];
                        float baseDecay = cachedDecays[sourceVoice];
                        float modFreq = baseFreq * std::pow(2.0f, freqMod * 2.0f);
                        float modDecay = baseDecay * std::pow(2.0f, decayMod);

                        drumSynth.setVoiceParams(sourceVoice,
                            unirhythm::EXTENDED_PRESETS[lastStyles[role]].voices[sourceVoice].mode,
                            modFreq, modDecay);

                        // Trigger synth and outputs
                        drumSynth.triggerVoice(sourceVoice, velocity);
                        gatePulses[role].trigger(1e-3f);

                        // Store velocity and pitch for outputs
                        currentVelocities[role] = velocity;
                        currentFreqs[sourceVoice] = modFreq;
                        currentPitches[role] = std::log2(modFreq / 261.63f);  // C4 = 0V

                        // Trigger velocity envelope
                        float decayParam = (decayMod + 1.0f) * 0.5f;  // Convert -1~1 to 0~1
                        velocityEnv[role].trigger(decayParam, args.sampleRate, velocity);
                    }

                    // Advance step
                    currentSteps[role]++;
                    if (currentSteps[role] >= length) {
                        currentSteps[role] = 0;
                    }
                }

                globalStep++;
                if (globalStep >= 16) {
                    globalStep = 0;
                    currentBar++;
                }
            }
        }

        // Process audio and outputs
        float mixL = 0.0f, mixR = 0.0f;

        for (int role = 0; role < 4; role++) {
            int voiceBase = role * 2;

            // Process both voices for audio mix
            float audio1 = drumSynth.processVoice(voiceBase);
            float audio2 = drumSynth.processVoice(voiceBase + 1);
            float roleAudio = audio1 + audio2;

            // Stereo panning per role
            float pan = 0.0f;
            switch (role) {
                case 0: pan = 0.3f; break;   // Timeline: slight right
                case 1: pan = 0.0f; break;   // Foundation: center
                case 2: pan = -0.2f; break;  // Groove: slight left
                case 3: pan = -0.4f; break;  // Lead: left
            }
            mixL += roleAudio * (0.5f - pan * 0.5f);
            mixR += roleAudio * (0.5f + pan * 0.5f);

            // Audio output
            outputs[TIMELINE_AUDIO_OUTPUT + role * 4].setVoltage(roleAudio * 5.0f);

            // Gate output
            float gateOut = gatePulses[role].process(args.sampleTime) ? 10.0f : 0.0f;
            outputs[TIMELINE_GATE_OUTPUT + role * 4].setVoltage(gateOut);

            // Pitch CV output
            outputs[TIMELINE_PITCH_OUTPUT + role * 4].setVoltage(currentPitches[role]);

            // Velocity Envelope output
            outputs[TIMELINE_VELENV_OUTPUT + role * 4].setVoltage(velocityEnv[role].process(args.sampleTime));

            // Light
            lights[TIMELINE_LIGHT + role].setBrightness(gateOut > 0.0f ? 1.0f : 0.0f);
        }

        // Mix outputs with soft clipping
        outputs[MIX_L_OUTPUT].setVoltage(std::tanh(mixL * 0.7f) * 5.0f);
        outputs[MIX_R_OUTPUT].setVoltage(std::tanh(mixR * 0.7f) * 5.0f);

        // Clock light
        lights[CLOCK_LIGHT].setBrightness(clockPulse.process(args.sampleTime) ? 1.0f : 0.0f);
    }
};

// ============================================================================
// Pattern Display Widget - 8 rows (2 voices per role)
// ============================================================================

struct URCompactPatternDisplay : TransparentWidget {
    UniRhythm* module = nullptr;

    void draw(const DrawArgs& args) override {
        // Rounded background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.0f);
        nvgFillColor(args.vg, nvgRGB(25, 25, 25));
        nvgFill(args.vg);

        if (!module) return;

        float rowHeight = box.size.y / 8.0f;  // 8 rows for 8 voices

        // Get colors from each role's style
        NVGcolor colors[8];
        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            int styleIndex = static_cast<int>(module->params[UniRhythm::TIMELINE_STYLE_PARAM + baseParam].getValue());
            styleIndex = clamp(styleIndex, 0, 9);
            NVGcolor baseColor = UR_STYLE_COLORS[styleIndex];

            // Primary voice - full color
            colors[role * 2] = baseColor;
            // Secondary voice - dimmer
            colors[role * 2 + 1] = nvgRGBA(
                baseColor.r * 255 * 0.7f,
                baseColor.g * 255 * 0.7f,
                baseColor.b * 255 * 0.7f,
                200
            );
        }

        // Draw patterns (each role may have different length)
        // Display order (top to bottom): Lead, Groove, Timeline, Foundation
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        // Display row mapping: row 0-1 = Lead(3), row 2-3 = Groove(2), row 4-5 = Timeline(0), row 6-7 = Foundation(1)
        const int displayToRole[4] = {3, 2, 0, 1};  // Lead, Groove, Timeline, Foundation

        for (int displayRow = 0; displayRow < 4; displayRow++) {
            int role = displayToRole[displayRow];
            int length = module->roleLengths[role];
            if (length <= 0 || length > 64) continue;

            int step = module->currentSteps[role];
            step = clamp(step, 0, length - 1);
            float stepWidth = box.size.x / length;

            // Current step indicator
            int baseParam = role * 5;
            int styleIndex = static_cast<int>(module->params[UniRhythm::TIMELINE_STYLE_PARAM + baseParam].getValue());
            styleIndex = clamp(styleIndex, 0, 9);
            NVGcolor stepColor = UR_STYLE_COLORS[styleIndex];

            nvgBeginPath(args.vg);
            nvgRect(args.vg, step * stepWidth, displayRow * rowHeight * 2, stepWidth, rowHeight * 2);
            nvgFillColor(args.vg, nvgRGBA(stepColor.r * 255, stepColor.g * 255, stepColor.b * 255, 60));
            nvgFill(args.vg);

            // Draw two voices for this role
            for (int voiceIdx = 0; voiceIdx < 2; voiceIdx++) {
                int v = role * 2 + voiceIdx;
                float y = (displayRow * 2 + voiceIdx) * rowHeight + rowHeight / 2.0f;

                const auto& pattern = module->patterns.patterns[v];
                int patternLength = std::min(length, static_cast<int>(pattern.length));

                for (int i = 0; i < patternLength; i++) {
                    if (pattern.hasOnsetAt(i)) {
                        float vel = pattern.getVelocity(i);
                        float x = i * stepWidth + stepWidth / 2.0f;
                        float radius = 1.5f + vel * 1.5f;

                        nvgBeginPath(args.vg);
                        nvgCircle(args.vg, x, y, radius);
                        nvgFillColor(args.vg, colors[v]);
                        nvgFill(args.vg);
                    }
                }
            }
        }
    }
};

// ============================================================================
// Role Title Implementation
// ============================================================================

void URCompactRoleTitle::draw(const DrawArgs &args) {
    NVGcolor color = nvgRGB(255, 255, 255);

    if (module) {
        int baseParam = roleIndex * 5;
        int styleIdx = static_cast<int>(module->params[UniRhythm::TIMELINE_STYLE_PARAM + baseParam].getValue());
        styleIdx = clamp(styleIdx, 0, 9);
        color = UR_STYLE_COLORS[styleIdx];
    }

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg, color);
    nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
}

// ============================================================================
// Module Widget - 16HP
// ============================================================================

struct UniRhythmWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    UniRhythmWidget(UniRhythm* module) {
        setModule(module);
        panelThemeHelper.init(this, "16HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(16 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Title (centered)
        NVGcolor titleColor = nvgRGB(255, 200, 0);
        addChild(new URCompactTextLabel(Vec(0, 2), Vec(box.size.x, 14), "U N I  R H Y T H M", 14.f, titleColor, true));
        addChild(new URCompactTextLabel(Vec(0, 13), Vec(box.size.x, 10), "MADZINE", 8.f, titleColor, false));

        // Pattern display (same height as UniversalRhythm: 50px)
        {
            URCompactPatternDisplay* display = new URCompactPatternDisplay();
            display->box.pos = Vec(8, 28);
            display->box.size = Vec(box.size.x - 16, 50);  // height=50 like UniversalRhythm
            display->module = module;
            addChild(display);
        }

        // Control row: below Pattern Display
        float ctrlLabelY = 85;
        float ctrlY = 109;  // 85 + 24 = 109 (label + 24px offset)

        addChild(new URCompactTextLabel(Vec(5, ctrlLabelY), Vec(30, 10), "CLOCK", 8.f));
        addInput(createInputCentered<PJ301MPort>(Vec(20, ctrlY), module, UniRhythm::CLOCK_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(32, ctrlY - 8), module, UniRhythm::CLOCK_LIGHT));

        addChild(new URCompactTextLabel(Vec(40, ctrlLabelY), Vec(30, 10), "RESET", 8.f));
        addParam(createParamCentered<VCVButton>(Vec(55, ctrlY), module, UniRhythm::RESET_BUTTON_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(75, ctrlY), module, UniRhythm::RESET_INPUT));

        addChild(new URCompactTextLabel(Vec(95, ctrlLabelY), Vec(30, 10), "REGEN", 8.f));
        addParam(createParamCentered<VCVButton>(Vec(110, ctrlY), module, UniRhythm::REGENERATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(130, ctrlY), module, UniRhythm::REGENERATE_INPUT));

        addChild(new URCompactTextLabel(Vec(155, ctrlLabelY), Vec(50, 10), "VARIATION", 8.f));
        addParam(createParamCentered<madzine::widgets::SmallGrayKnob>(Vec(180, ctrlY), module, UniRhythm::VARIATION_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(205, ctrlY), module, UniRhythm::VARIATION_CV_INPUT));

        // Role sections (4 columns)
        // Display order: Foundation, Timeline, Groove, Lead
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        const char* displayNames[4] = {"FOUND", "TIME", "GROOVE", "LEAD"};
        const int roleOrder[4] = {1, 0, 2, 3};  // Foundation, Timeline, Groove, Lead
        NVGcolor white = nvgRGB(255, 255, 255);
        float trackWidth = 4 * RACK_GRID_WIDTH;  // 60.96px per track

        float labelToKnob = 24;  // Standard label-to-knob distance
        float knobVSpacing = 42;  // Vertical spacing to fit 5 knobs
        float roleY = 135;  // Role section starting Y

        for (int uiPos = 0; uiPos < 4; uiPos++) {
            int role = roleOrder[uiPos];
            float trackX = uiPos * trackWidth;
            float centerX = trackX + trackWidth / 2;
            int baseParam = role * 5;
            int baseCv = role * 5;

            // Role title
            URCompactRoleTitle* title = new URCompactRoleTitle(
                Vec(trackX, roleY), Vec(trackWidth, 10), displayNames[uiPos], role, 8.f);
            title->module = module;
            addChild(title);

            // Style (label above knob)
            float knob1Y = roleY + 28;
            addChild(new URCompactTextLabel(Vec(trackX, knob1Y - labelToKnob), Vec(trackWidth, 10), "STYLE", 8.f, white, true));
            addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(
                Vec(centerX, knob1Y), module, UniRhythm::TIMELINE_STYLE_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(
                Vec(centerX, knob1Y + 30), module, UniRhythm::TIMELINE_STYLE_CV_INPUT + baseCv));

            // Density (label above knob)
            float knob2Y = knob1Y + knobVSpacing + 8;
            addChild(new URCompactTextLabel(Vec(trackX, knob2Y - labelToKnob), Vec(trackWidth, 10), "DENSITY", 8.f, white, true));
            addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(
                Vec(centerX, knob2Y), module, UniRhythm::TIMELINE_DENSITY_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(
                Vec(centerX, knob2Y + 30), module, UniRhythm::TIMELINE_DENSITY_CV_INPUT + baseCv));

            // Length (label above knob)
            float knob3Y = knob2Y + knobVSpacing + 8;
            addChild(new URCompactTextLabel(Vec(trackX, knob3Y - labelToKnob), Vec(trackWidth, 10), "LENGTH", 8.f, white, true));
            addParam(createParamCentered<madzine::widgets::SmallWhiteKnob>(
                Vec(centerX, knob3Y), module, UniRhythm::TIMELINE_LENGTH_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(
                Vec(centerX, knob3Y + 30), module, UniRhythm::TIMELINE_LENGTH_CV_INPUT + baseCv));

            // Freq (label above knob)
            float knob4Y = knob3Y + knobVSpacing + 8;
            addChild(new URCompactTextLabel(Vec(trackX, knob4Y - labelToKnob), Vec(trackWidth, 10), "FREQ", 8.f, white, true));
            addParam(createParamCentered<madzine::widgets::SmallGrayKnob>(
                Vec(centerX, knob4Y), module, UniRhythm::TIMELINE_FREQ_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(
                Vec(centerX, knob4Y + 30), module, UniRhythm::TIMELINE_FREQ_CV_INPUT + baseCv));

            // Decay (label above knob)
            float knob5Y = knob4Y + knobVSpacing + 8;
            addChild(new URCompactTextLabel(Vec(trackX, knob5Y - labelToKnob), Vec(trackWidth, 10), "DECAY", 8.f, white, true));
            addParam(createParamCentered<madzine::widgets::SmallGrayKnob>(
                Vec(centerX, knob5Y), module, UniRhythm::TIMELINE_DECAY_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(
                Vec(centerX, knob5Y + 30), module, UniRhythm::TIMELINE_DECAY_CV_INPUT + baseCv));
        }

        // Output section (white background, Y=330, height=50)
        addChild(new URCompactWhiteBox(Vec(0, 330), Vec(box.size.x, 50)));

        NVGcolor labelColor = nvgRGB(255, 133, 133);

        // Row positions (VCV UI SPECIFICATION.md: Row 1=343, Row 2=368)
        float row1Y = 343;
        float row2Y = 368;

        // Left side type labels
        addChild(new URCompactTextLabel(Vec(2, 337), Vec(18, 8), "Audio", 7.f, labelColor, true));
        addChild(new URCompactTextLabel(Vec(20, 337), Vec(18, 8), "Gate", 7.f, labelColor, true));
        addChild(new URCompactTextLabel(Vec(2, 362), Vec(18, 8), "Pitch", 7.f, labelColor, true));
        addChild(new URCompactTextLabel(Vec(20, 362), Vec(18, 8), "Velo", 7.f, labelColor, true));

        // Output display order: Foundation, Timeline, Groove, Lead (same as roleOrder)
        // 4 roles, trackWidth spacing
        for (int uiPos = 0; uiPos < 4; uiPos++) {
            int role = roleOrder[uiPos];
            float trackX = uiPos * trackWidth;
            float centerX = trackX + trackWidth / 2;

            // Row 1: Audio (left), Gate (right) - 15px apart
            addOutput(createOutputCentered<PJ301MPort>(Vec(centerX - 15, row1Y), module, UniRhythm::TIMELINE_AUDIO_OUTPUT + role * 4));
            addChild(createLightCentered<SmallLight<GreenLight>>(Vec(centerX, row1Y - 10), module, UniRhythm::TIMELINE_LIGHT + role));
            addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, row1Y), module, UniRhythm::TIMELINE_GATE_OUTPUT + role * 4));

            // Row 2: Pitch (left), VelEnv (right) - 15px apart
            addOutput(createOutputCentered<PJ301MPort>(Vec(centerX - 15, row2Y), module, UniRhythm::TIMELINE_PITCH_OUTPUT + role * 4));
            addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, row2Y), module, UniRhythm::TIMELINE_VELENV_OUTPUT + role * 4));
        }

        // Mix outputs (right side margin, vertically stacked)
        float mixX = box.size.x - 15;
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixX, row1Y), module, UniRhythm::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixX, row2Y), module, UniRhythm::MIX_R_OUTPUT));
        addChild(new URCompactTextLabel(Vec(mixX - 20, row1Y - 4), Vec(12, 8), "L", 7.f, labelColor, true));
        addChild(new URCompactTextLabel(Vec(mixX - 20, row2Y - 4), Vec(12, 8), "R", 7.f, labelColor, true));
    }

    void step() override {
        UniRhythm* module = dynamic_cast<UniRhythm*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        UniRhythm* module = dynamic_cast<UniRhythm*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator());

        // PPQN selection
        menu->addChild(createSubmenuItem("Clock PPQN", "",
            [=](Menu* menu) {
                menu->addChild(createCheckMenuItem("1 PPQN (Quarter note)", "",
                    [=]() { return module->ppqn == 1; },
                    [=]() { module->ppqn = 1; }
                ));
                menu->addChild(createCheckMenuItem("2 PPQN (8th note)", "",
                    [=]() { return module->ppqn == 2; },
                    [=]() { module->ppqn = 2; }
                ));
                menu->addChild(createCheckMenuItem("4 PPQN (16th note)", "",
                    [=]() { return module->ppqn == 4; },
                    [=]() { module->ppqn = 4; }
                ));
            }
        ));

        addPanelThemeMenu(menu, module);
    }
};

Model* modelUniRhythm = createModel<UniRhythm, UniRhythmWidget>("UniRhythm");
