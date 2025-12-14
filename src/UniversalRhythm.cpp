#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "WorldRhythm/PatternGenerator.hpp"
#include "WorldRhythm/HumanizeEngine.hpp"
#include "WorldRhythm/StyleProfiles.hpp"
#include "WorldRhythm/MinimalDrumSynth.hpp"
#include "WorldRhythm/RestEngine.hpp"
#include "WorldRhythm/FillGenerator.hpp"
#include "WorldRhythm/ArticulationEngine.hpp"
#include "WorldRhythm/ArticulationProfiles.hpp"
#include "WorldRhythm/KotekanEngine.hpp"
#include "WorldRhythm/LlamadaEngine.hpp"
#include "WorldRhythm/CrossRhythmEngine.hpp"
#include "WorldRhythm/AsymmetricGroupingEngine.hpp"
#include "WorldRhythm/AmenBreakEngine.hpp"
#include <vector>
#include <algorithm>

// ============================================================================
// Universal Rhythm Module - 40HP
// Cross-cultural rhythm generator with integrated synthesis
// 8 voice outputs + mix output
// Per-role Style/Density/Length controls
// Global REST parameter with RestEngine
// ============================================================================

// ============================================================================
// Style names and colors (MUJI-inspired pastel palette)
// ============================================================================

const char* STYLE_NAMES[10] = {
    "W.African", "Afro-Cuban", "Brazilian", "Balkan", "Indian",
    "Gamelan", "Jazz", "Electronic", "Breakbeat", "Techno"
};

// Groove template names
const char* GROOVE_TEMPLATE_NAMES[7] = {
    "Auto", "Straight", "Swing", "African", "Latin", "LaidBack", "Pushed"
};


// MUJI-inspired palette with better contrast between styles
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

// ============================================================================
// Custom ParamQuantity for Style with names
// ============================================================================

struct StyleParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int index = static_cast<int>(getValue());
        if (index >= 0 && index < 10) {
            return STYLE_NAMES[index];
        }
        return ParamQuantity::getDisplayValueString();
    }
};



// ============================================================================
// Helper Widgets (MADDY+ style)
// ============================================================================

struct URTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    URTextLabel(Vec pos, Vec size, std::string text, float fontSize = 10.f,
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

// Forward declaration
struct UniversalRhythm;

// Dynamic role title that changes color based on style
struct URDynamicRoleTitle : TransparentWidget {
    UniversalRhythm* module = nullptr;
    int roleIndex = 0;
    std::string text;
    float fontSize;
    bool bold;

    URDynamicRoleTitle(Vec pos, Vec size, std::string text, int roleIndex, float fontSize = 9.f, bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->roleIndex = roleIndex;
        this->fontSize = fontSize;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override;  // Implemented after UniversalRhythm definition
};

// Dynamic style name display (shows current style name below Decay)
struct URStyleNameDisplay : TransparentWidget {
    UniversalRhythm* module = nullptr;
    int roleIndex = 0;
    float fontSize;

    URStyleNameDisplay(Vec pos, Vec size, int roleIndex, float fontSize = 7.f) {
        box.pos = pos;
        box.size = size;
        this->roleIndex = roleIndex;
        this->fontSize = fontSize;
    }

    void draw(const DrawArgs &args) override;  // Implemented after UniversalRhythm definition
};

struct URWhiteBackgroundBox : Widget {
    URWhiteBackgroundBox(Vec pos, Vec size) {
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

struct URVerticalLine : Widget {
    URVerticalLine(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, box.size.x / 2.0f, 0);
        nvgLineTo(args.vg, box.size.x / 2.0f, box.size.y);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 150));
        nvgStroke(args.vg);
    }
};

struct URHorizontalLine : Widget {
    URHorizontalLine(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, box.size.y / 2.0f);
        nvgLineTo(args.vg, box.size.x, box.size.y / 2.0f);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 150));
        nvgStroke(args.vg);
    }
};

// ============================================================================
// Extended Drum Synth - 8 voices
// ============================================================================

namespace worldrhythm {

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

// 8-voice style presets
struct ExtendedStylePreset {
    struct VoicePreset {
        SynthMode mode;
        float freq;
        float decay;
        const char* name;
    };
    VoicePreset voices[8];
};

// Voice assignments per style (2 voices per role):
// 0-1: Timeline, 2-3: Foundation, 4-5: Groove, 6-7: Lead

const ExtendedStylePreset EXTENDED_PRESETS[10] = {
    // 0: West African
    // Timeline: Bell 3-6kHz, Foundation: 80-150Hz, Groove: 250-400Hz, Lead: high
    {{{SynthMode::SINE, 4500.0f, 60.0f, "Gankogui"},
      {SynthMode::SINE, 3500.0f, 40.0f, "Bell Lo"},
      {SynthMode::SINE, 80.0f, 200.0f, "Dununba"},
      {SynthMode::SINE, 120.0f, 150.0f, "Dunun"},
      {SynthMode::SINE, 250.0f, 80.0f, "Sangban"},
      {SynthMode::SINE, 300.0f, 60.0f, "Kenkeni"},
      {SynthMode::NOISE, 700.0f, 40.0f, "Djembe Slap"},
      {SynthMode::NOISE, 400.0f, 50.0f, "Djembe Tone"}}},

    // 1: Afro-Cuban
    // Timeline: Clave 3-6kHz, Foundation: 80-150Hz, Groove: 250-700Hz, Lead: high
    {{{SynthMode::SINE, 4000.0f, 20.0f, "Clave"},
      {SynthMode::SINE, 2000.0f, 30.0f, "Cowbell"},
      {SynthMode::SINE, 100.0f, 150.0f, "Tumba"},
      {SynthMode::SINE, 150.0f, 120.0f, "Conga Lo"},
      {SynthMode::SINE, 350.0f, 70.0f, "Conga Mid"},
      {SynthMode::SINE, 550.0f, 50.0f, "Quinto"},
      {SynthMode::NOISE, 3000.0f, 40.0f, "Timbales"},
      {SynthMode::NOISE, 5000.0f, 25.0f, "Quinto Slap"}}},

    // 2: Brazilian
    // Timeline: Agogô 3-6kHz, Foundation: Surdo 55-80Hz, Groove: 250-700Hz, Lead: high
    {{{SynthMode::SINE, 4500.0f, 35.0f, "Agogo Hi"},
      {SynthMode::SINE, 3000.0f, 35.0f, "Agogo Lo"},
      {SynthMode::SINE, 55.0f, 250.0f, "Surdo"},
      {SynthMode::SINE, 80.0f, 180.0f, "Surdo 2"},
      {SynthMode::SINE, 400.0f, 40.0f, "Tamborim"},
      {SynthMode::NOISE, 500.0f, 50.0f, "Caixa"},
      {SynthMode::NOISE, 6000.0f, 30.0f, "Ganza"},
      {SynthMode::NOISE, 8000.0f, 20.0f, "Chocalho"}}},

    // 3: Balkan
    // Timeline: Rim 3-6kHz, Foundation: Tapan 90-130Hz, Groove: 250-700Hz, Lead: high
    {{{SynthMode::NOISE, 4000.0f, 25.0f, "Rim"},
      {SynthMode::NOISE, 3500.0f, 15.0f, "Click"},
      {SynthMode::SINE, 90.0f, 180.0f, "Tapan Bass"},
      {SynthMode::SINE, 130.0f, 120.0f, "Tapan Mid"},
      {SynthMode::SINE, 300.0f, 50.0f, "Tarabuka Doum"},
      {SynthMode::SINE, 450.0f, 35.0f, "Tarabuka Tek"},
      {SynthMode::NOISE, 3000.0f, 25.0f, "Tek Hi"},
      {SynthMode::NOISE, 5000.0f, 20.0f, "Ka"}}},

    // 4: Indian
    // Timeline: Manjira 6-12kHz (Air layer), Foundation: Bayan 80-150Hz, Groove: Dayan 250-700Hz
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
    // Timeline: Ride 3-6kHz (swing pattern), Foundation: Kick 40-80Hz, Groove: Snare 400-700Hz
    {{{SynthMode::NOISE, 4500.0f, 120.0f, "Ride"},
      {SynthMode::NOISE, 2500.0f, 80.0f, "Ride Bell"},
      {SynthMode::SINE, 50.0f, 200.0f, "Kick"},
      {SynthMode::SINE, 80.0f, 150.0f, "Kick Ghost"},
      {SynthMode::NOISE, 500.0f, 100.0f, "Snare"},
      {SynthMode::NOISE, 400.0f, 60.0f, "Snare Ghost"},
      {SynthMode::NOISE, 8000.0f, 35.0f, "HiHat Cl"},
      {SynthMode::NOISE, 6000.0f, 150.0f, "HiHat Op"}}},

    // 7: Electronic
    // Timeline: Hi-hat 6-12kHz, Foundation: 808 Kick 40-80Hz, Groove: Clap 1-3kHz, Lead: variable
    {{{SynthMode::NOISE, 9000.0f, 30.0f, "HiHat"},
      {SynthMode::NOISE, 12000.0f, 20.0f, "HiHat Ac"},
      {SynthMode::SINE, 45.0f, 280.0f, "808 Kick"},
      {SynthMode::SINE, 60.0f, 200.0f, "Kick 2"},
      {SynthMode::NOISE, 1500.0f, 70.0f, "Clap"},
      {SynthMode::NOISE, 2500.0f, 50.0f, "Snare"},
      {SynthMode::NOISE, 6000.0f, 150.0f, "Open HH"},
      {SynthMode::SINE, 800.0f, 100.0f, "Perc"}}},

    // 8: Breakbeat
    // Timeline: Hi-hat 6-12kHz, Foundation: Kick 40-80Hz (2-step), Groove: Snare 1-3kHz, Lead: breaks
    {{{SynthMode::NOISE, 8000.0f, 25.0f, "HiHat"},
      {SynthMode::NOISE, 10000.0f, 15.0f, "HiHat Ac"},
      {SynthMode::SINE, 55.0f, 180.0f, "Kick"},
      {SynthMode::SINE, 70.0f, 120.0f, "Kick Gho"},
      {SynthMode::NOISE, 2500.0f, 80.0f, "Snare"},
      {SynthMode::NOISE, 2000.0f, 50.0f, "Snare Gh"},
      {SynthMode::NOISE, 4000.0f, 40.0f, "Ghost"},
      {SynthMode::NOISE, 6000.0f, 100.0f, "Open HH"}}},

    // 9: Techno
    // Timeline: Hi-hat 6-12kHz, Foundation: 909 Kick 40-60Hz, Groove: Clap 1-3kHz, Lead: minimal perc
    {{{SynthMode::NOISE, 10000.0f, 20.0f, "HiHat"},
      {SynthMode::NOISE, 12000.0f, 12.0f, "HiHat Ac"},
      {SynthMode::SINE, 42.0f, 250.0f, "909 Kick"},
      {SynthMode::SINE, 55.0f, 180.0f, "Kick Lay"},
      {SynthMode::NOISE, 1800.0f, 55.0f, "Clap"},
      {SynthMode::NOISE, 3000.0f, 35.0f, "Rim"},
      {SynthMode::NOISE, 5000.0f, 80.0f, "Open HH"},
      {SynthMode::SINE, 600.0f, 60.0f, "Tom"}}}
};

// Apply preset for specific role (2 voices)
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

} // namespace worldrhythm

// ============================================================================
// Pattern storage for 8 voices
// ============================================================================

struct MultiVoicePatterns {
    WorldRhythm::Pattern patterns[8];

    MultiVoicePatterns() {
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
// Universal Rhythm Module
// ============================================================================

struct UniversalRhythm : Module {
    int panelTheme = 0;

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
        // Global parameters
        VARIATION_PARAM,
        HUMANIZE_PARAM,
        SWING_PARAM,
        REST_PARAM,
        // Fill parameter (combined probability + intensity)
        FILL_PARAM,
        // Articulation parameter
        ARTICULATION_PARAM,
        // Ghost and Accent parameters
        GHOST_PARAM,
        ACCENT_PROB_PARAM,
        // Spread parameter (stereo width)
        SPREAD_PARAM,
        // Buttons
        REGENERATE_PARAM,
        RESET_BUTTON_PARAM,
        // Mix parameters (per-role): 0 = internal synth only, 1 = external only
        TIMELINE_MIX_PARAM,
        FOUNDATION_MIX_PARAM,
        GROOVE_MIX_PARAM,
        LEAD_MIX_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        REGENERATE_INPUT,
        REST_CV_INPUT,  // REST CV input
        FILL_INPUT,     // Fill trigger input
        // Per-role CV inputs (Style, Density, Freq, Decay per role)
        TIMELINE_STYLE_CV_INPUT,
        TIMELINE_DENSITY_CV_INPUT,
        TIMELINE_FREQ_CV_INPUT,
        TIMELINE_DECAY_CV_INPUT,
        FOUNDATION_STYLE_CV_INPUT,
        FOUNDATION_DENSITY_CV_INPUT,
        FOUNDATION_FREQ_CV_INPUT,
        FOUNDATION_DECAY_CV_INPUT,
        GROOVE_STYLE_CV_INPUT,
        GROOVE_DENSITY_CV_INPUT,
        GROOVE_FREQ_CV_INPUT,
        GROOVE_DECAY_CV_INPUT,
        LEAD_STYLE_CV_INPUT,
        LEAD_DENSITY_CV_INPUT,
        LEAD_FREQ_CV_INPUT,
        LEAD_DECAY_CV_INPUT,
        // Audio inputs (2 per role)
        TIMELINE_AUDIO_INPUT_1,
        TIMELINE_AUDIO_INPUT_2,
        FOUNDATION_AUDIO_INPUT_1,
        FOUNDATION_AUDIO_INPUT_2,
        GROOVE_AUDIO_INPUT_1,
        GROOVE_AUDIO_INPUT_2,
        LEAD_AUDIO_INPUT_1,
        LEAD_AUDIO_INPUT_2,
        INPUTS_LEN
    };

    enum OutputId {
        // Audio outputs
        MIX_L_OUTPUT,
        MIX_R_OUTPUT,
        VOICE1_AUDIO_OUTPUT,
        VOICE2_AUDIO_OUTPUT,
        VOICE3_AUDIO_OUTPUT,
        VOICE4_AUDIO_OUTPUT,
        VOICE5_AUDIO_OUTPUT,
        VOICE6_AUDIO_OUTPUT,
        VOICE7_AUDIO_OUTPUT,
        VOICE8_AUDIO_OUTPUT,
        // Gate outputs
        VOICE1_GATE_OUTPUT,
        VOICE2_GATE_OUTPUT,
        VOICE3_GATE_OUTPUT,
        VOICE4_GATE_OUTPUT,
        VOICE5_GATE_OUTPUT,
        VOICE6_GATE_OUTPUT,
        VOICE7_GATE_OUTPUT,
        VOICE8_GATE_OUTPUT,
        // CV outputs (velocity)
        VOICE1_CV_OUTPUT,
        VOICE2_CV_OUTPUT,
        VOICE3_CV_OUTPUT,
        VOICE4_CV_OUTPUT,
        VOICE5_CV_OUTPUT,
        VOICE6_CV_OUTPUT,
        VOICE7_CV_OUTPUT,
        VOICE8_CV_OUTPUT,
        // Accent outputs
        VOICE1_ACCENT_OUTPUT,
        VOICE2_ACCENT_OUTPUT,
        VOICE3_ACCENT_OUTPUT,
        VOICE4_ACCENT_OUTPUT,
        VOICE5_ACCENT_OUTPUT,
        VOICE6_ACCENT_OUTPUT,
        VOICE7_ACCENT_OUTPUT,
        VOICE8_ACCENT_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        VOICE1_LIGHT,
        VOICE2_LIGHT,
        VOICE3_LIGHT,
        VOICE4_LIGHT,
        VOICE5_LIGHT,
        VOICE6_LIGHT,
        VOICE7_LIGHT,
        VOICE8_LIGHT,
        CLOCK_LIGHT,
        LIGHTS_LEN
    };

    // Engines
    WorldRhythm::PatternGenerator patternGen;
    WorldRhythm::HumanizeEngine humanize;
    WorldRhythm::RestEngine restEngine;
    WorldRhythm::FillGenerator fillGen;
    WorldRhythm::ArticulationEngine articulationEngine;
    WorldRhythm::KotekanEngine kotekanEngine;
    WorldRhythm::LlamadaEngine llamadaEngine;
    WorldRhythm::CrossRhythmEngine crossRhythmEngine;
    WorldRhythm::AsymmetricGroupingEngine asymmetricEngine;
    WorldRhythm::AmenBreakEngine amenBreakEngine;
    worldrhythm::ExtendedDrumSynth drumSynth;

    // Pattern storage
    MultiVoicePatterns patterns;           // Working patterns (with rest applied)
    MultiVoicePatterns originalPatterns;   // Original patterns (before rest)
    int roleLengths[4] = {16, 16, 16, 16};  // Per-role lengths
    int currentSteps[4] = {0, 0, 0, 0};     // Per-role step counters
    int currentBar = 0;
    float appliedRest = 0.0f;              // Last applied rest amount

    // Cached synth parameters for TUNE/DECAY modification
    float cachedFreqs[8] = {0};
    float cachedDecays[8] = {0};
    float currentFreqs[8] = {0};  // Actual frequencies after FREQ knob/CV modulation (for Pitch CV output)

    // Triggers and pulses
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger resetButtonTrigger;
    dsp::SchmittTrigger regenerateTrigger;
    dsp::SchmittTrigger regenerateButtonTrigger;
    dsp::SchmittTrigger fillTrigger;

    // Fill state
    bool fillActive = false;
    int fillStepsRemaining = 0;
    MultiVoicePatterns fillPatterns;  // Temporary fill patterns
    WorldRhythm::FillType currentFillType = WorldRhythm::FillType::NONE;

    // Fill pre-determination (decided at bar start, triggered at fillStartStep)
    bool nextBarHasFill = false;
    int fillStartStep = 0;
    int fillLengthStepsPlanned = 0;

    dsp::PulseGenerator gatePulses[8];
    dsp::PulseGenerator accentPulses[8];
    dsp::PulseGenerator clockPulse;

    // Velocity tracking per voice (for CV output)
    float currentVelocities[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    bool currentAccents[8] = {false, false, false, false, false, false, false, false};

    // Global step counter for bar tracking (moved from static to member for proper reset)
    int globalStep = 0;

    // PPQN setting (1, 2, or 4 pulses per quarter note)
    // 4 PPQN = 16th note clock (default), 2 PPQN = 8th note clock, 1 PPQN = quarter note clock
    int ppqn = 4;
    int ppqnCounter = 0;  // Counter for clock division

    // Flam/Drag delayed trigger support
    struct DelayedTrigger {
        float samplesRemaining = 0;
        int voice = -1;
        float velocity = 0;
        bool isAccent = false;
        int role = 0;           // Role index for articulation profile
        bool isStrongBeat = false;  // For articulation selection
        bool isSubNote = false;     // True for articulation sub-notes (no further articulation needed)
    };
    std::vector<DelayedTrigger> delayedTriggers;

    // Change detection (per role)
    int lastStyles[4] = {-1, -1, -1, -1};
    float lastDensities[4] = {-1.f, -1.f, -1.f, -1.f};
    int lastLengths[4] = {-1, -1, -1, -1};
    float lastVariation = -1.0f;
    float lastRoleFreqs[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float lastRoleDecays[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float lastSwing = 0.5f;

    // External audio VCA envelopes (per voice)
    struct VCAEnvelope {
        float amplitude = 0.0f;
        float decayRate = 0.0f;

        void trigger(float decayTimeMs, float sampleRate, float velocity = 1.0f) {
            amplitude = 1.0f;
            // Velocity affects decay length (same formula as internal synth)
            // vel=1.0 -> 100% decay, vel=0.5 -> 46% decay, vel=0.2 -> 17% decay
            float velScale = 0.1f + 0.9f * std::pow(velocity, 1.5f);
            float actualDecayMs = decayTimeMs * velScale;
            // Convert decay time to decay rate per sample
            decayRate = 1.0f / (actualDecayMs * 0.001f * sampleRate);
        }

        float process() {
            if (amplitude > 0.0f) {
                float current = amplitude;
                amplitude -= decayRate;
                if (amplitude < 0.0f) amplitude = 0.0f;
                return current;
            }
            return 0.0f;
        }

        bool isActive() const {
            return amplitude > 0.001f;
        }
    };

    VCAEnvelope externalVCA[8];  // One VCA per voice for external audio gating
    float currentMix[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Current mix value per role (0=internal, 1=external)

    UniversalRhythm() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Per-role parameters
        const char* roleNames[4] = {"Timeline", "Foundation", "Groove", "Lead"};
        for (int r = 0; r < 4; r++) {
            int baseParam = r * 5;  // STYLE, DENSITY, LENGTH, FREQ, DECAY per role
            configParam<StyleParamQuantity>(TIMELINE_STYLE_PARAM + baseParam, 0.0f, 9.0f, 0.0f,
                        std::string(roleNames[r]) + " Style");
            getParamQuantity(TIMELINE_STYLE_PARAM + baseParam)->snapEnabled = true;

            configParam(TIMELINE_DENSITY_PARAM + baseParam, 0.0f, 0.9f,
                        r == 1 ? 0.2f : (r == 0 ? 0.4f : 0.5f),  // Foundation sparser
                        std::string(roleNames[r]) + " Density", "%", 0.0f, 100.0f);

            configParam(TIMELINE_LENGTH_PARAM + baseParam, 4.0f, 32.0f, 16.0f,
                        std::string(roleNames[r]) + " Length");
            getParamQuantity(TIMELINE_LENGTH_PARAM + baseParam)->snapEnabled = true;

            configParam(TIMELINE_FREQ_PARAM + baseParam, -1.0f, 1.0f, 0.0f,
                        std::string(roleNames[r]) + " Freq", " oct");

            configParam(TIMELINE_DECAY_PARAM + baseParam, 0.2f, 2.0f, 1.0f,
                        std::string(roleNames[r]) + " Decay", "x");
        }

        // MIX parameters (per-role): 0 = internal synth, 1 = external input
        for (int r = 0; r < 4; r++) {
            configParam(TIMELINE_MIX_PARAM + r, 0.0f, 1.0f, 0.0f,
                       std::string(roleNames[r]) + " Mix", "%", 0.0f, 100.0f);
        }

        // Global parameters
        configParam(VARIATION_PARAM, 0.0f, 1.0f, 0.3f, "Variation", "%", 0.0f, 100.0f);
        configParam(HUMANIZE_PARAM, 0.0f, 1.0f, 0.5f, "Humanize", "%", 0.0f, 100.0f);
        configParam(SWING_PARAM, 0.0f, 1.0f, 0.5f, "Swing", "%", 0.0f, 100.0f);
        configParam(REST_PARAM, 0.0f, 1.0f, 0.0f, "Rest", "%", 0.0f, 100.0f);

        // Fill parameters
        configParam(FILL_PARAM, 0.0f, 1.0f, 0.3f, "Fill", "%", 0.0f, 100.0f);  // Combined probability + intensity

        // Articulation and Groove parameters
        configParam(ARTICULATION_PARAM, 0.0f, 1.0f, 0.0f, "Articulation", "%", 0.0f, 100.0f);

        // Ghost and Accent parameters
        configParam(GHOST_PARAM, 0.0f, 1.0f, 0.0f, "Ghost Notes", "%", 0.0f, 100.0f);
        configParam(ACCENT_PROB_PARAM, 0.0f, 1.0f, 0.0f, "Accent", "%", 0.0f, 100.0f);
        configParam(SPREAD_PARAM, 0.0f, 1.0f, 0.5f, "Spread", "%", 0.0f, 100.0f);

        // Regenerate button
        configParam(REGENERATE_PARAM, 0.0f, 1.0f, 0.0f, "Regenerate");

        // Reset button
        configParam(RESET_BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "Reset");

        // Mix parameters (per-role: 0=internal synth, 1=external audio)
        for (int r = 0; r < 4; r++) {
            configParam(TIMELINE_MIX_PARAM + r, 0.0f, 1.0f, 0.0f, std::string(roleNames[r]) + " Mix (Int/Ext)", "%", 0.0f, 100.0f);
        }

        // Note: TUNE_PARAM and DECAY_PARAM removed - now per-role

        // Inputs
        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(REGENERATE_INPUT, "Regenerate");
        configInput(REST_CV_INPUT, "Rest CV");
        configInput(FILL_INPUT, "Fill Trigger");

        for (int r = 0; r < 4; r++) {
            configInput(TIMELINE_STYLE_CV_INPUT + r * 4, std::string(roleNames[r]) + " Style CV");
            configInput(TIMELINE_DENSITY_CV_INPUT + r * 4, std::string(roleNames[r]) + " Density CV");
            configInput(TIMELINE_FREQ_CV_INPUT + r * 4, std::string(roleNames[r]) + " Freq CV");
            configInput(TIMELINE_DECAY_CV_INPUT + r * 4, std::string(roleNames[r]) + " Decay CV");
        }

        // External audio inputs (2 per role)
        for (int r = 0; r < 4; r++) {
            configInput(TIMELINE_AUDIO_INPUT_1 + r * 2, std::string(roleNames[r]) + " Audio Input 1");
            configInput(TIMELINE_AUDIO_INPUT_2 + r * 2, std::string(roleNames[r]) + " Audio Input 2");
        }

        // Audio inputs (per-role stereo processing)
        for (int r = 0; r < 4; r++) {
            configInput(TIMELINE_AUDIO_INPUT_1 + r * 2, std::string(roleNames[r]) + " Audio Input 1");
            configInput(TIMELINE_AUDIO_INPUT_2 + r * 2, std::string(roleNames[r]) + " Audio Input 2");
        }

        // Outputs
        configOutput(MIX_L_OUTPUT, "Mix L");
        configOutput(MIX_R_OUTPUT, "Mix R");
        const char* voiceLabels[8] = {"Timeline 1", "Timeline 2", "Foundation 1", "Foundation 2",
                                       "Groove 1", "Groove 2", "Lead 1", "Lead 2"};
        for (int i = 0; i < 8; i++) {
            configOutput(VOICE1_AUDIO_OUTPUT + i, std::string(voiceLabels[i]) + " Audio");
            configOutput(VOICE1_GATE_OUTPUT + i, std::string(voiceLabels[i]) + " Gate");
            configOutput(VOICE1_CV_OUTPUT + i, std::string(voiceLabels[i]) + " Pitch CV (1V/Oct, C4=0V)");
            configOutput(VOICE1_ACCENT_OUTPUT + i, std::string(voiceLabels[i]) + " Velocity CV");
        }

        // Initialize
        regenerateAllPatterns();
    }

    void onSampleRateChange() override {
        drumSynth.setSampleRate(APP->engine->getSampleRate());
    }

    void onReset() override {
        for (int i = 0; i < 4; i++) {
            currentSteps[i] = 0;
        }
        currentBar = 0;
        globalStep = 0;  // Reset global step counter
        ppqnCounter = 0;  // Reset PPQN counter
        fillActive = false;
        fillStepsRemaining = 0;
        regenerateAllPatterns();
    }

    // Reset step counters (called by reset input/button)
    void resetSteps() {
        for (int i = 0; i < 4; i++) {
            currentSteps[i] = 0;
        }
        currentBar = 0;
        globalStep = 0;  // Reset global step counter for fill timing
        ppqnCounter = 0;
        fillActive = false;
        fillStepsRemaining = 0;
        nextBarHasFill = false;
        fillStartStep = 0;
        fillLengthStepsPlanned = 0;
    }

    // Apply per-role FREQ and DECAY to voices (with CV modulation)
    void applySynthModifiers() {
        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            float freqAmount = params[TIMELINE_FREQ_PARAM + baseParam].getValue();  // -1 to +1 octave
            float decayMult = params[TIMELINE_DECAY_PARAM + baseParam].getValue();  // 0.2 to 2.0

            // Apply CV modulation
            if (inputs[TIMELINE_FREQ_CV_INPUT + role * 4].isConnected()) {
                freqAmount += inputs[TIMELINE_FREQ_CV_INPUT + role * 4].getVoltage() * 0.2f;  // ±1V = ±0.2 octave
                freqAmount = clamp(freqAmount, -1.0f, 1.0f);
            }
            if (inputs[TIMELINE_DECAY_CV_INPUT + role * 4].isConnected()) {
                decayMult += inputs[TIMELINE_DECAY_CV_INPUT + role * 4].getVoltage() * 0.18f;  // ±1V = ±0.18
                decayMult = clamp(decayMult, 0.2f, 2.0f);
            }

            // Calculate frequency multiplier
            float freqMult = std::pow(2.0f, freqAmount);

            int voiceBase = role * 2;
            for (int v = 0; v < 2; v++) {
                int voiceIdx = voiceBase + v;
                if (cachedFreqs[voiceIdx] > 0) {
                    float newFreq = cachedFreqs[voiceIdx] * freqMult;
                    float newDecay = cachedDecays[voiceIdx] * decayMult;
                    currentFreqs[voiceIdx] = newFreq;  // Store for Pitch CV output
                    int styleIndex = lastStyles[role];
                    if (styleIndex >= 0 && styleIndex <= 9) {
                        const worldrhythm::ExtendedStylePreset& preset = worldrhythm::EXTENDED_PRESETS[styleIndex];
                        drumSynth.setVoiceParams(voiceIdx, preset.voices[voiceIdx].mode, newFreq, newDecay);
                    }
                }
            }

            lastRoleFreqs[role] = freqAmount;
            lastRoleDecays[role] = decayMult;
        }
    }

    void regenerateAllPatternsInterlocked() {
        // Get parameters
        float variation = params[VARIATION_PARAM].getValue();
        float restAmount = params[REST_PARAM].getValue();
        if (inputs[REST_CV_INPUT].isConnected()) {
            restAmount += inputs[REST_CV_INPUT].getVoltage() * 0.1f;
            restAmount = clamp(restAmount, 0.0f, 1.0f);
        }
        float humanizeAmount = params[HUMANIZE_PARAM].getValue();
        float swingAmount = params[SWING_PARAM].getValue();

        // Use Timeline's style for the interlock config (main style)
        int mainStyleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM].getValue());
        mainStyleIndex = clamp(mainStyleIndex, 0, WorldRhythm::NUM_STYLES - 1);
        const WorldRhythm::StyleProfile& mainStyle = *WorldRhythm::STYLES[mainStyleIndex];

        // Get style-specific interlock configuration
        WorldRhythm::PatternGenerator::InterlockConfig config =
            WorldRhythm::PatternGenerator::getStyleInterlockConfig(mainStyleIndex);

        // Get the length from Timeline (use as base for interlocked generation)
        int baseLength = static_cast<int>(params[TIMELINE_LENGTH_PARAM].getValue());
        float baseDensity = params[TIMELINE_DENSITY_PARAM].getValue();

        // Generate all 4 roles with proper interlock relationships
        WorldRhythm::PatternGenerator::RolePatterns interlocked =
            patternGen.generateInterlocked(mainStyle, baseLength, baseDensity, variation, config);

        // Store primary patterns (voice 0, 2, 4, 6)
        patterns.patterns[0] = interlocked.timeline;
        patterns.patterns[2] = interlocked.foundation;
        patterns.patterns[4] = interlocked.groove;
        patterns.patterns[6] = interlocked.lead;

        // Now generate per-role with individual settings and secondary voices
        for (int r = 0; r < 4; r++) {
            int baseParam = r * 5;
            float styleCV = 0.0f;
            if (inputs[TIMELINE_STYLE_CV_INPUT + r * 4].isConnected()) {
                styleCV = inputs[TIMELINE_STYLE_CV_INPUT + r * 4].getVoltage();
            }
            int styleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue() + styleCV);
            styleIndex = clamp(styleIndex, 0, WorldRhythm::NUM_STYLES - 1);

            float density = params[TIMELINE_DENSITY_PARAM + baseParam].getValue();
            int length = static_cast<int>(params[TIMELINE_LENGTH_PARAM + baseParam].getValue());
            roleLengths[r] = length;

            const WorldRhythm::StyleProfile& style = *WorldRhythm::STYLES[styleIndex];
            WorldRhythm::Role roleType = static_cast<WorldRhythm::Role>(r);

            // CRITICAL: If density is 0, force empty patterns for this role (complete silence)
            if (density < 0.01f) {
                patterns.patterns[r * 2] = WorldRhythm::Pattern(length);
                patterns.patterns[r * 2 + 1] = WorldRhythm::Pattern(length);
                // Skip all further processing for this role
                roleLengths[r] = length;
                lastStyles[r] = styleIndex;
                lastDensities[r] = density;
                lastLengths[r] = length;
                continue;
            }

            // If this role has different length/density than base, regenerate primary
            if (length != baseLength || std::abs(density - baseDensity) > 0.05f || styleIndex != mainStyleIndex) {
                patterns.patterns[r * 2] = patternGen.generate(roleType, style, length, density, variation);
            }

            // Generate secondary pattern (complementary to primary)
            // For Gamelan style (5), use KotekanEngine for proper interlocking
            if (styleIndex == 5 && (r == 2 || r == 3)) {  // Gamelan Groove or Lead
                WorldRhythm::KotekanType kotekanType = kotekanEngine.getRecommendedType(styleIndex);
                kotekanEngine.setType(kotekanType);
                kotekanEngine.setIntensity(1.0f);

                WorldRhythm::KotekanPair kotekan = kotekanEngine.generate(length, 0.8f, density);
                patterns.patterns[r * 2] = kotekan.polos;
                patterns.patterns[r * 2 + 1] = kotekan.sangsih;
            }
            // For Breakbeat style (8), use AmenBreakEngine for authentic break patterns
            else if (styleIndex == 8) {
                if (r == 1) {  // Foundation = Kick
                    patterns.patterns[r * 2] = amenBreakEngine.generateKick(length, density);
                    patterns.patterns[r * 2 + 1] = amenBreakEngine.generateKick(length, density * 0.7f);
                } else if (r == 2) {  // Groove = Snare
                    patterns.patterns[r * 2] = amenBreakEngine.generateSnare(length, density);
                    patterns.patterns[r * 2 + 1] = amenBreakEngine.generateSnare(length, density * 0.6f);
                } else if (r == 3) {  // Lead = Hihat + chops
                    float chopIntensity = variation;
                    patterns.patterns[r * 2] = amenBreakEngine.generateRandomChop(length, density, chopIntensity);
                    patterns.patterns[r * 2 + 1] = amenBreakEngine.generateHihat(length, density * 0.8f);
                } else {
                    // Timeline uses standard generation
                    patterns.patterns[r * 2 + 1] = patternGen.generateWithInterlock(
                        roleType, style, length, density * 0.5f, variation + 0.2f,
                        patterns.patterns[r * 2]);
                }
            }
            else {
                patterns.patterns[r * 2 + 1] = patternGen.generateWithInterlock(
                    roleType, style, length, density * 0.5f, variation + 0.2f,
                    patterns.patterns[r * 2]);
            }

            // Apply CrossRhythmEngine for African/Cuban/Brazilian styles (0, 1, 2)
            // Creates 3:2 polyrhythmic feel between roles
            if ((styleIndex == 0 || styleIndex == 1 || styleIndex == 2) && r == 2) {
                // Apply 3:2 cross-rhythm overlay to Groove role
                WorldRhythm::CrossRhythmType crType = crossRhythmEngine.getStyleCrossRhythm(styleIndex);
                float crIntensity = crossRhythmEngine.getStyleCrossRhythmIntensity(styleIndex);
                crossRhythmEngine.applyCrossRhythmOverlay(patterns.patterns[r * 2], crType, crIntensity, 0.6f);
                crossRhythmEngine.applyCrossRhythmOverlay(patterns.patterns[r * 2 + 1], crType, crIntensity * 0.7f, 0.4f);
            }

            // Apply AsymmetricGroupingEngine for Balkan (3) and Indian (4) styles
            // Balkan: 2+2+3 grouping, Indian: 2+3+2 grouping (Carnatic-style asymmetric)
            if (styleIndex == 3 || styleIndex == 4) {
                WorldRhythm::GroupingType groupType = WorldRhythm::AsymmetricGroupingEngine::getStyleDefaultGrouping(styleIndex);
                asymmetricEngine.setGroupingType(groupType);
                // Indian uses lighter intensity to maintain tala feel without overpowering
                float intensity = (styleIndex == 3) ? 0.8f : 0.6f;
                float secondaryIntensity = (styleIndex == 3) ? 0.6f : 0.45f;
                asymmetricEngine.applyToPattern(patterns.patterns[r * 2], intensity);
                asymmetricEngine.applyToPattern(patterns.patterns[r * 2 + 1], secondaryIntensity);
            }

            // Apply humanization with swing
            if (humanizeAmount > 0.01f) {
                humanize.setStyle(styleIndex);
                humanize.setSwing(swingAmount);  // Apply swing parameter
                humanize.setGrooveForStyle(styleIndex);  // Auto groove based on style
                humanize.humanizePattern(patterns.patterns[r * 2], roleType, currentBar, 4);
                humanize.humanizePattern(patterns.patterns[r * 2 + 1], roleType, currentBar, 4);
            }

            // Generate accents with adjustable probability
            // Generate base accents from style
            patternGen.generateAccents(patterns.patterns[r * 2], roleType, style);
            patternGen.generateAccents(patterns.patterns[r * 2 + 1], roleType, style);

            // Add extra accents based on ACCENT_PARAM (0=none, 1=all onsets become accents)
            float accentAmount = params[ACCENT_PROB_PARAM].getValue();
            if (accentAmount > 0.01f) {
                for (int i = 0; i < patterns.patterns[r * 2].length; i++) {
                    // Only add accents to existing onsets that aren't already accented
                    if (patterns.patterns[r * 2].hasOnsetAt(i) && !patterns.patterns[r * 2].accents[i]) {
                        // Prioritize strong beats (positions 0, 4, 8, 12 in 16-step)
                        bool isStrongBeat = (i % 4 == 0);
                        float prob = isStrongBeat ? accentAmount : accentAmount * 0.5f;
                        if ((float)rand() / RAND_MAX < prob) {
                            patterns.patterns[r * 2].accents[i] = true;
                        }
                    }
                    if (patterns.patterns[r * 2 + 1].hasOnsetAt(i) && !patterns.patterns[r * 2 + 1].accents[i]) {
                        bool isStrongBeat = (i % 4 == 0);
                        float prob = isStrongBeat ? accentAmount : accentAmount * 0.5f;
                        if ((float)rand() / RAND_MAX < prob) {
                            patterns.patterns[r * 2 + 1].accents[i] = true;
                        }
                    }
                }
            }

            // Add ghost notes based on GHOST_PARAM (all roles with role-specific multipliers)
            float ghostAmount = params[GHOST_PARAM].getValue();
            if (ghostAmount > 0.01f) {
                float roleMultiplier = (r == 2 || r == 3) ? 1.0f : 0.5f;  // More for Groove/Lead
                patternGen.addGhostNotes(patterns.patterns[r * 2], style, ghostAmount * roleMultiplier);
                patternGen.addGhostNotes(patterns.patterns[r * 2 + 1], style, ghostAmount * roleMultiplier * 0.8f);
            }

            // Save original patterns (before rest) for on-the-fly rest adjustment
            originalPatterns.patterns[r * 2] = patterns.patterns[r * 2];
            originalPatterns.patterns[r * 2 + 1] = patterns.patterns[r * 2 + 1];

            // Apply RestEngine (position-weighted rest)
            if (restAmount > 0.01f) {
                restEngine.setStyle(styleIndex);
                restEngine.applyRest(patterns.patterns[r * 2], roleType, restAmount);
                restEngine.applyRest(patterns.patterns[r * 2 + 1], roleType, restAmount);
            }

            // Apply and cache synth preset for this role
            const worldrhythm::ExtendedStylePreset& preset = worldrhythm::EXTENDED_PRESETS[styleIndex];
            int voiceBase = r * 2;
            cachedFreqs[voiceBase] = preset.voices[voiceBase].freq;
            cachedFreqs[voiceBase + 1] = preset.voices[voiceBase + 1].freq;
            cachedDecays[voiceBase] = preset.voices[voiceBase].decay;
            cachedDecays[voiceBase + 1] = preset.voices[voiceBase + 1].decay;
            worldrhythm::applyRolePreset(drumSynth, r, styleIndex);

            // Update tracking
            lastStyles[r] = styleIndex;
            lastDensities[r] = density;
            lastLengths[r] = length;
        }

        // Apply TUNE/DECAY modifiers
        applySynthModifiers();

        lastVariation = variation;
        lastSwing = swingAmount;
    }

    void regenerateRolePattern(int role) {
        int baseParam = role * 5;
        float styleCV = 0.0f;
        if (inputs[TIMELINE_STYLE_CV_INPUT + role * 4].isConnected()) {
            styleCV = inputs[TIMELINE_STYLE_CV_INPUT + role * 4].getVoltage();
        }
        int styleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue() + styleCV);
        styleIndex = clamp(styleIndex, 0, WorldRhythm::NUM_STYLES - 1);

        // Read density with CV modulation
        float densityCV = 0.0f;
        if (inputs[TIMELINE_DENSITY_CV_INPUT + role * 4].isConnected()) {
            densityCV = inputs[TIMELINE_DENSITY_CV_INPUT + role * 4].getVoltage() * 0.1f;
        }
        float density = clamp(params[TIMELINE_DENSITY_PARAM + baseParam].getValue() + densityCV, 0.0f, 0.9f);
        int length = static_cast<int>(params[TIMELINE_LENGTH_PARAM + baseParam].getValue());

        // CRITICAL: If density is 0, force empty patterns and skip all processing
        if (density < 0.01f) {
            patterns.patterns[role * 2] = WorldRhythm::Pattern(length);
            patterns.patterns[role * 2 + 1] = WorldRhythm::Pattern(length);
            roleLengths[role] = length;
            lastStyles[role] = styleIndex;
            lastDensities[role] = density;
            lastLengths[role] = length;
            // Cache synth preset
            const worldrhythm::ExtendedStylePreset& preset = worldrhythm::EXTENDED_PRESETS[styleIndex];
            int voiceBase = role * 2;
            cachedFreqs[voiceBase] = preset.voices[voiceBase].freq;
            cachedFreqs[voiceBase + 1] = preset.voices[voiceBase + 1].freq;
            cachedDecays[voiceBase] = preset.voices[voiceBase].decay;
            cachedDecays[voiceBase + 1] = preset.voices[voiceBase + 1].decay;
            worldrhythm::applyRolePreset(drumSynth, role, styleIndex);
            return;
        }

        float variation = params[VARIATION_PARAM].getValue();
        float restAmount = params[REST_PARAM].getValue();
        if (inputs[REST_CV_INPUT].isConnected()) {
            restAmount += inputs[REST_CV_INPUT].getVoltage() * 0.1f;
            restAmount = clamp(restAmount, 0.0f, 1.0f);
        }
        float humanizeAmount = params[HUMANIZE_PARAM].getValue();
        float swingAmount = params[SWING_PARAM].getValue();

        roleLengths[role] = length;

        const WorldRhythm::StyleProfile& style = *WorldRhythm::STYLES[styleIndex];
        WorldRhythm::Role roleType = static_cast<WorldRhythm::Role>(role);

        // Generate with interlock against other roles if available
        if (role == WorldRhythm::TIMELINE) {
            patterns.patterns[role * 2] = patternGen.generate(roleType, style, length, density, variation);
        } else if (role == WorldRhythm::FOUNDATION) {
            // Foundation avoids Timeline
            WorldRhythm::PatternGenerator::InterlockConfig config =
                WorldRhythm::PatternGenerator::getStyleInterlockConfig(styleIndex);
            if (config.avoidFoundationOnTimeline) {
                patterns.patterns[role * 2] = patternGen.generateFoundationWithInterlock(
                    style, length, density, variation, patterns.patterns[0], config.avoidanceStrength);
            } else {
                patterns.patterns[role * 2] = patternGen.generateFoundation(style, length, density, variation);
            }
        } else if (role == WorldRhythm::GROOVE) {
            // Groove complements Foundation
            WorldRhythm::PatternGenerator::InterlockConfig config =
                WorldRhythm::PatternGenerator::getStyleInterlockConfig(styleIndex);
            if (config.grooveComplementsFoundation) {
                patterns.patterns[role * 2] = patternGen.generateGrooveWithComplement(
                    style, length, density, variation, patterns.patterns[2], patterns.patterns[0], config);
            } else {
                patterns.patterns[role * 2] = patternGen.generate(roleType, style, length, density, variation);
            }
        } else {
            // Lead - optional groove avoidance
            WorldRhythm::PatternGenerator::InterlockConfig config =
                WorldRhythm::PatternGenerator::getStyleInterlockConfig(styleIndex);
            if (config.leadAvoidsGroove) {
                patterns.patterns[role * 2] = patternGen.generateWithInterlock(
                    roleType, style, length, density * 0.6f, variation, patterns.patterns[4]);
            } else {
                patterns.patterns[role * 2] = patternGen.generate(roleType, style, length, density * 0.6f, variation);
            }
        }

        // Generate secondary pattern (complementary)
        // For Gamelan style (5), use KotekanEngine for proper interlocking
        if (styleIndex == 5 && (role == 2 || role == 3)) {  // Gamelan Groove or Lead
            WorldRhythm::KotekanType kotekanType = kotekanEngine.getRecommendedType(styleIndex);
            kotekanEngine.setType(kotekanType);
            kotekanEngine.setIntensity(density);

            WorldRhythm::KotekanPair kotekan = kotekanEngine.splitIntoKotekan(
                patterns.patterns[role * 2], 0.5f);

            patterns.patterns[role * 2] = kotekan.polos;
            patterns.patterns[role * 2 + 1] = kotekan.sangsih;
        }
        // For Breakbeat style (8), use AmenBreakEngine
        else if (styleIndex == 8) {
            if (role == 1) {  // Foundation = Kick
                patterns.patterns[role * 2] = amenBreakEngine.generateKick(length, density);
                patterns.patterns[role * 2 + 1] = amenBreakEngine.generateKick(length, density * 0.7f);
            } else if (role == 2) {  // Groove = Snare
                patterns.patterns[role * 2] = amenBreakEngine.generateSnare(length, density);
                patterns.patterns[role * 2 + 1] = amenBreakEngine.generateSnare(length, density * 0.6f);
            } else if (role == 3) {  // Lead = Hihat + chops
                float chopIntensity = variation;
                patterns.patterns[role * 2] = amenBreakEngine.generateRandomChop(length, density, chopIntensity);
                patterns.patterns[role * 2 + 1] = amenBreakEngine.generateHihat(length, density * 0.8f);
            } else {
                // Timeline uses standard generation
                patterns.patterns[role * 2 + 1] = patternGen.generateWithInterlock(
                    roleType, style, length, density * 0.5f, variation + 0.2f,
                    patterns.patterns[role * 2]);
            }
        }
        else {
            patterns.patterns[role * 2 + 1] = patternGen.generateWithInterlock(
                roleType, style, length, density * 0.5f, variation + 0.2f,
                patterns.patterns[role * 2]);
        }

        // Apply CrossRhythmEngine for African/Cuban/Brazilian styles (0, 1, 2)
        if ((styleIndex == 0 || styleIndex == 1 || styleIndex == 2) && role == 2) {
            WorldRhythm::CrossRhythmType crType = crossRhythmEngine.getStyleCrossRhythm(styleIndex);
            float crIntensity = crossRhythmEngine.getStyleCrossRhythmIntensity(styleIndex);
            crossRhythmEngine.applyCrossRhythmOverlay(patterns.patterns[role * 2], crType, crIntensity, 0.6f);
            crossRhythmEngine.applyCrossRhythmOverlay(patterns.patterns[role * 2 + 1], crType, crIntensity * 0.7f, 0.4f);
        }

        // Apply AsymmetricGroupingEngine for Balkan (3) and Indian (4) styles
        if (styleIndex == 3 || styleIndex == 4) {
            WorldRhythm::GroupingType groupType = WorldRhythm::AsymmetricGroupingEngine::getStyleDefaultGrouping(styleIndex);
            asymmetricEngine.setGroupingType(groupType);
            float intensity = (styleIndex == 3) ? 0.8f : 0.6f;
            float secondaryIntensity = (styleIndex == 3) ? 0.6f : 0.45f;
            asymmetricEngine.applyToPattern(patterns.patterns[role * 2], intensity);
            asymmetricEngine.applyToPattern(patterns.patterns[role * 2 + 1], secondaryIntensity);
        }

        // Apply humanization with swing
        if (humanizeAmount > 0.01f) {
            humanize.setStyle(styleIndex);
            humanize.setSwing(swingAmount);
            humanize.setGrooveForStyle(styleIndex);  // Auto groove based on style
            humanize.humanizePattern(patterns.patterns[role * 2], roleType, currentBar, 4);
            humanize.humanizePattern(patterns.patterns[role * 2 + 1], roleType, currentBar, 4);
        }

        // Generate accents with adjustable probability
        // Generate base accents from style
        patternGen.generateAccents(patterns.patterns[role * 2], roleType, style);
        patternGen.generateAccents(patterns.patterns[role * 2 + 1], roleType, style);

        // Add extra accents based on ACCENT_PARAM (0=none, 1=all onsets become accents)
        float accentAmount = params[ACCENT_PROB_PARAM].getValue();
        if (accentAmount > 0.01f) {
            for (int i = 0; i < patterns.patterns[role * 2].length; i++) {
                if (patterns.patterns[role * 2].hasOnsetAt(i) && !patterns.patterns[role * 2].accents[i]) {
                    bool isStrongBeat = (i % 4 == 0);
                    float prob = isStrongBeat ? accentAmount : accentAmount * 0.5f;
                    if ((float)rand() / RAND_MAX < prob) {
                        patterns.patterns[role * 2].accents[i] = true;
                    }
                }
                if (patterns.patterns[role * 2 + 1].hasOnsetAt(i) && !patterns.patterns[role * 2 + 1].accents[i]) {
                    bool isStrongBeat = (i % 4 == 0);
                    float prob = isStrongBeat ? accentAmount : accentAmount * 0.5f;
                    if ((float)rand() / RAND_MAX < prob) {
                        patterns.patterns[role * 2 + 1].accents[i] = true;
                    }
                }
            }
        }

        // Add ghost notes based on GHOST_PARAM (now applies to all roles)
        float ghostAmount = params[GHOST_PARAM].getValue();
        if (ghostAmount > 0.01f) {
            // Apply more ghost notes to Groove and Lead roles
            float roleMultiplier = (role == WorldRhythm::GROOVE || role == WorldRhythm::LEAD) ? 1.0f : 0.5f;
            patternGen.addGhostNotes(patterns.patterns[role * 2], style, ghostAmount * roleMultiplier);
            patternGen.addGhostNotes(patterns.patterns[role * 2 + 1], style, ghostAmount * roleMultiplier * 0.8f);
        }

        // Save original patterns (before rest) for on-the-fly rest adjustment
        originalPatterns.patterns[role * 2] = patterns.patterns[role * 2];
        originalPatterns.patterns[role * 2 + 1] = patterns.patterns[role * 2 + 1];

        // Apply RestEngine (position-weighted rest)
        if (restAmount > 0.01f) {
            restEngine.setStyle(styleIndex);
            restEngine.applyRest(patterns.patterns[role * 2], roleType, restAmount);
            restEngine.applyRest(patterns.patterns[role * 2 + 1], roleType, restAmount);
        }

        // Apply and cache synth preset for this role
        const worldrhythm::ExtendedStylePreset& preset = worldrhythm::EXTENDED_PRESETS[styleIndex];
        int voiceBase = role * 2;
        cachedFreqs[voiceBase] = preset.voices[voiceBase].freq;
        cachedFreqs[voiceBase + 1] = preset.voices[voiceBase + 1].freq;
        cachedDecays[voiceBase] = preset.voices[voiceBase].decay;
        cachedDecays[voiceBase + 1] = preset.voices[voiceBase + 1].decay;
        worldrhythm::applyRolePreset(drumSynth, role, styleIndex);

        // Apply TUNE/DECAY modifiers
        applySynthModifiers();

        // Update tracking
        lastStyles[role] = styleIndex;
        lastDensities[role] = density;
        lastLengths[role] = length;
    }

    void regenerateAllPatterns() {
        regenerateAllPatternsInterlocked();
    }

    // Reapply rest from original patterns without regenerating rhythm
    void reapplyRest(float restAmount) {
        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            int styleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue());
            styleIndex = clamp(styleIndex, 0, WorldRhythm::NUM_STYLES - 1);
            WorldRhythm::Role roleType = static_cast<WorldRhythm::Role>(role);

            // Copy from original patterns
            patterns.patterns[role * 2] = originalPatterns.patterns[role * 2];
            patterns.patterns[role * 2 + 1] = originalPatterns.patterns[role * 2 + 1];

            // Apply rest if needed
            if (restAmount > 0.01f) {
                restEngine.setStyle(styleIndex);
                restEngine.applyRest(patterns.patterns[role * 2], roleType, restAmount);
                restEngine.applyRest(patterns.patterns[role * 2 + 1], roleType, restAmount);
            }
        }
        appliedRest = restAmount;
    }

    // Trigger voice with articulation type applied
    // Uses ArticulationProfiles to select articulation based on style, role, and amount
    void triggerWithArticulation(int voice, float velocity, bool accent, float sampleRate,
                                  int role = -1, bool isStrongBeat = false) {
        // Get articulation amount from parameter (0 = no articulation, 1 = max)
        float articulationAmount = params[ARTICULATION_PARAM].getValue();

        // Determine role from voice if not provided
        if (role < 0) {
            role = voice / 2;  // Each role has 2 voices
        }

        // Get style for this specific role (each role can have different style)
        int baseParam = role * 5;  // 5 params per role
        int currentStyle = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue());

        // Select articulation using profile system
        WorldRhythm::ArticulationType art = WorldRhythm::selectArticulation(
            currentStyle, role, articulationAmount, accent, isStrongBeat);

        float finalVel = velocity;

        switch (art) {
            case WorldRhythm::ArticulationType::GHOST:
                finalVel = velocity * 0.2f;  // Very soft
                drumSynth.triggerVoice(voice, finalVel);
                gatePulses[voice].trigger(0.001f);
                break;

            case WorldRhythm::ArticulationType::ACCENT:
                finalVel = std::min(1.0f, velocity * 1.3f);  // Emphasized
                drumSynth.triggerVoice(voice, finalVel);
                gatePulses[voice].trigger(0.001f);
                accentPulses[voice].trigger(0.001f);
                break;

            case WorldRhythm::ArticulationType::RIM:
                // Rim shot - slightly higher pitch feel (handled by shorter decay)
                finalVel = velocity * 1.1f;
                drumSynth.triggerVoice(voice, finalVel);
                gatePulses[voice].trigger(0.001f);
                break;

            case WorldRhythm::ArticulationType::FLAM: {
                // Use ArticulationEngine for proper flam generation
                WorldRhythm::ExpandedHit hit = articulationEngine.generateFlam(velocity);
                scheduleExpandedHit(voice, hit, accent, sampleRate, role);
                break;
            }

            case WorldRhythm::ArticulationType::DRAG: {
                // Use ArticulationEngine for proper drag generation
                WorldRhythm::ExpandedHit hit = articulationEngine.generateDrag(velocity);
                scheduleExpandedHit(voice, hit, accent, sampleRate, role);
                break;
            }

            case WorldRhythm::ArticulationType::BUZZ: {
                // Use ArticulationEngine for proper buzz generation
                // Duration of 0.032s gives 4 bounces at default 15ms interval
                WorldRhythm::ExpandedHit hit = articulationEngine.generateBuzz(velocity, 0.032f, 4);
                scheduleExpandedHit(voice, hit, accent, sampleRate, role);
                break;
            }

            case WorldRhythm::ArticulationType::RUFF: {
                // Use ArticulationEngine for proper ruff generation
                WorldRhythm::ExpandedHit hit = articulationEngine.generateRuff(velocity);
                scheduleExpandedHit(voice, hit, accent, sampleRate, role);
                break;
            }

            case WorldRhythm::ArticulationType::NORMAL:
            default:
                drumSynth.triggerVoice(voice, velocity);
                gatePulses[voice].trigger(0.001f);
                break;
        }

        currentVelocities[voice] = finalVel;
        currentAccents[voice] = accent;
        if (accent && art != WorldRhythm::ArticulationType::GHOST) {
            accentPulses[voice].trigger(0.001f);
        }
    }

    // Helper: Schedule ExpandedHit notes as DelayedTriggers
    void scheduleExpandedHit(int voice, const WorldRhythm::ExpandedHit& hit, bool accent, float sampleRate, int role) {
        // Pre-calculate decay multiplier for VCA
        int baseParam = role * 5;
        float decayMult = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
        if (inputs[TIMELINE_DECAY_CV_INPUT + role * 4].isConnected()) {
            decayMult += inputs[TIMELINE_DECAY_CV_INPUT + role * 4].getVoltage() * 0.18f;
            decayMult = clamp(decayMult, 0.2f, 2.0f);
        }
        float vcaDecayMs = 200.0f * decayMult;

        for (size_t i = 0; i < hit.notes.size(); i++) {
            const WorldRhythm::ExpandedNote& note = hit.notes[i];

            // Convert relative timing (seconds) to samples
            // Negative timing means before beat, positive means after
            float timingSeconds = note.timing;

            if (timingSeconds <= 0.0f && i == 0) {
                // First note with zero or negative timing: trigger immediately
                drumSynth.triggerVoice(voice, note.velocity);
                gatePulses[voice].trigger(0.001f);
                currentVelocities[voice] = note.velocity;
                currentAccents[voice] = note.isAccent && accent;
                // Trigger VCA for external audio
                externalVCA[voice].trigger(vcaDecayMs, sampleRate, note.velocity);
                if (note.isAccent && accent) {
                    accentPulses[voice].trigger(0.001f);
                }
            } else {
                // Schedule as delayed trigger
                // For negative timing, we need to offset from the first note
                float delayFromFirst = timingSeconds - hit.notes[0].timing;
                DelayedTrigger dt;
                dt.samplesRemaining = static_cast<int>(sampleRate * delayFromFirst);
                dt.voice = voice;
                dt.velocity = note.velocity;
                dt.isAccent = note.isAccent && accent;
                dt.role = role;  // Pass role for VCA triggering
                dt.isStrongBeat = false;
                dt.isSubNote = true;  // Mark as sub-note (no further articulation needed)
                if (dt.samplesRemaining > 0) {
                    delayedTriggers.push_back(dt);
                } else if (i > 0) {
                    // Immediate trigger for notes at same time as first
                    drumSynth.triggerVoice(voice, note.velocity);
                    gatePulses[voice].trigger(0.001f);
                    currentVelocities[voice] = note.velocity;
                    currentAccents[voice] = note.isAccent && accent;
                    // Trigger VCA for external audio
                    externalVCA[voice].trigger(vcaDecayMs, sampleRate, note.velocity);
                }
            }
        }
    }

    // Generate fill patterns for all roles based on current style
    void generateFillPatterns(float intensity) {
        // Get main style from Timeline for fill type selection
        int mainStyleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM].getValue());
        mainStyleIndex = clamp(mainStyleIndex, 0, 9);

        // Use pre-calculated fill length (already clamped to fit within bar)
        // This ensures fill doesn't exceed bar boundary
        int fillLengthSteps = fillLengthStepsPlanned;

        // Fallback if not pre-planned (e.g., manual FILL trigger)
        if (fillLengthSteps <= 0) {
            int maxLen = *std::max_element(roleLengths, roleLengths + 4);
            int fillLengthBeats = fillGen.getFillLengthBeats(intensity);
            fillLengthSteps = fillLengthBeats * 4;
            // Allow fill up to full bar length (like RhythmEngine)
            fillLengthSteps = std::min(fillLengthSteps, maxLen);
            fillLengthSteps = std::max(fillLengthSteps, 4);
        }

        fillStepsRemaining = fillLengthSteps;

        for (int r = 0; r < 4; r++) {
            int baseParam = r * 5;
            int styleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue());
            styleIndex = clamp(styleIndex, 0, 9);

            WorldRhythm::Role roleType = static_cast<WorldRhythm::Role>(r);

            // Check if this role should participate in fill
            WorldRhythm::FillType fillType = fillGen.selectFillType(styleIndex, roleType);
            if (!fillGen.shouldRoleFill(roleType, fillType)) {
                // Role doesn't do fill - COPY normal pattern (maintains during fill)
                // This is important: Timeline/Foundation should keep playing, not go silent
                int normalLen = patterns.patterns[r * 2].length;
                fillPatterns.patterns[r * 2] = WorldRhythm::Pattern(fillLengthSteps);
                fillPatterns.patterns[r * 2 + 1] = WorldRhythm::Pattern(fillLengthSteps);
                for (int i = 0; i < fillLengthSteps; i++) {
                    int srcIdx = i % normalLen;
                    if (patterns.patterns[r * 2].hasOnsetAt(srcIdx)) {
                        fillPatterns.patterns[r * 2].setOnset(i, patterns.patterns[r * 2].getVelocity(srcIdx));
                        fillPatterns.patterns[r * 2].accents[i] = patterns.patterns[r * 2].accents[srcIdx];
                    }
                    if (patterns.patterns[r * 2 + 1].hasOnsetAt(srcIdx)) {
                        fillPatterns.patterns[r * 2 + 1].setOnset(i, patterns.patterns[r * 2 + 1].getVelocity(srcIdx));
                    }
                }
                continue;
            }

            // Get role-adjusted intensity
            float roleIntensity = fillGen.getRoleFillIntensity(roleType, intensity);

            // For Afro-Cuban style (1), use LlamadaEngine for authentic llamada fills
            if (styleIndex == 1 && (r == 2 || r == 3)) {  // Afro-Cuban Groove or Lead
                // Select llamada type based on intensity
                WorldRhythm::LlamadaType llamadaType;
                if (intensity > 0.8f) {
                    llamadaType = WorldRhythm::LlamadaType::DIABLO;
                } else if (intensity > 0.6f) {
                    llamadaType = WorldRhythm::LlamadaType::MAMBO_CALL;
                } else if (intensity > 0.4f) {
                    llamadaType = WorldRhythm::LlamadaType::MONTUNO_ENTRY;
                } else {
                    llamadaType = WorldRhythm::LlamadaType::STANDARD;
                }
                llamadaEngine.setType(llamadaType);

                // Generate llamada call pattern
                WorldRhythm::Pattern llamadaPattern = llamadaEngine.generateCall(fillLengthSteps, roleIntensity);

                // Apply with variation
                fillPatterns.patterns[r * 2] = llamadaEngine.addVariation(llamadaPattern, 0.2f);

                // Secondary voice gets response pattern
                fillPatterns.patterns[r * 2 + 1] = llamadaEngine.generateResponse(fillLengthSteps, roleIntensity * 0.8f);
            } else {
                // Standard fill generation for other styles
                std::vector<float> fillVelocities = fillGen.generateFillPattern(fillType, fillLengthSteps, roleIntensity);

                // Apply to primary voice pattern
                fillPatterns.patterns[r * 2] = WorldRhythm::Pattern(fillLengthSteps);
                for (int i = 0; i < fillLengthSteps; i++) {
                    if (fillVelocities[i] > 0.01f) {
                        fillPatterns.patterns[r * 2].setOnset(i, fillVelocities[i]);
                        // High velocity = accent
                        if (fillVelocities[i] > 0.75f) {
                            fillPatterns.patterns[r * 2].accents[i] = true;
                        }
                    }
                }

                // Secondary voice - sparser fill (50% of primary)
                fillPatterns.patterns[r * 2 + 1] = WorldRhythm::Pattern(fillLengthSteps);
                for (int i = 0; i < fillLengthSteps; i += 2) {
                    if (fillVelocities[i] > 0.3f) {
                        fillPatterns.patterns[r * 2 + 1].setOnset(i, fillVelocities[i] * 0.7f);
                    }
                }
            }
        }

        currentFillType = fillGen.selectFillType(mainStyleIndex, WorldRhythm::GROOVE);
        fillActive = true;
    }

    void process(const ProcessArgs& args) override {
        // Set sample rate on first process
        static bool initialized = false;
        if (!initialized) {
            drumSynth.setSampleRate(args.sampleRate);
            initialized = true;
        }

        // Process delayed triggers (for swing/groove timing and Flam, Drag, Buzz, Ruff articulations)
        for (auto it = delayedTriggers.begin(); it != delayedTriggers.end(); ) {
            it->samplesRemaining -= 1.0f;
            if (it->samplesRemaining <= 0) {
                // Calculate VCA decay for this role
                int baseParam = it->role * 5;
                float decayMult = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
                if (inputs[TIMELINE_DECAY_CV_INPUT + it->role * 4].isConnected()) {
                    decayMult += inputs[TIMELINE_DECAY_CV_INPUT + it->role * 4].getVoltage() * 0.18f;
                    decayMult = clamp(decayMult, 0.2f, 2.0f);
                }
                float vcaDecayMs = 200.0f * decayMult;

                if (!it->isSubNote) {
                    // Main trigger - apply articulation
                    triggerWithArticulation(it->voice, it->velocity, it->isAccent, args.sampleRate,
                                           it->role, it->isStrongBeat);
                    // Trigger VCA for external audio
                    externalVCA[it->voice].trigger(vcaDecayMs, args.sampleRate, it->velocity);
                } else {
                    // Articulation sub-note - direct trigger (no further articulation)
                    drumSynth.triggerVoice(it->voice, it->velocity);
                    gatePulses[it->voice].trigger(0.001f);
                    currentVelocities[it->voice] = it->velocity;
                    currentAccents[it->voice] = it->isAccent;
                    // Trigger VCA for external audio (sub-notes also trigger VCA)
                    externalVCA[it->voice].trigger(vcaDecayMs, args.sampleRate, it->velocity);
                    if (it->isAccent) {
                        accentPulses[it->voice].trigger(0.001f);
                    }
                }
                it = delayedTriggers.erase(it);
            } else {
                ++it;
            }
        }

        // Check each role for parameter changes
        float variation = params[VARIATION_PARAM].getValue();
        float restAmount = params[REST_PARAM].getValue();
        if (inputs[REST_CV_INPUT].isConnected()) {
            restAmount += inputs[REST_CV_INPUT].getVoltage() * 0.1f;
            restAmount = clamp(restAmount, 0.0f, 1.0f);
        }
        // Note: Swing is read in regenerate functions, not here - changes don't trigger regeneration

        // Only variation triggers full regeneration, REST is applied on-the-fly
        bool globalRegenNeeded = std::abs(variation - lastVariation) > 0.05f;

        // Check per-role FREQ/DECAY changes (don't need full regen, just synth update)
        bool synthUpdateNeeded = false;
        for (int r = 0; r < 4; r++) {
            int baseParam = r * 5;
            float roleFreq = params[TIMELINE_FREQ_PARAM + baseParam].getValue();
            float roleDecay = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
            if (std::abs(roleFreq - lastRoleFreqs[r]) > 0.01f ||
                std::abs(roleDecay - lastRoleDecays[r]) > 0.01f) {
                synthUpdateNeeded = true;
            }
        }

        if (regenerateTrigger.process(inputs[REGENERATE_INPUT].getVoltage()) ||
            regenerateButtonTrigger.process(params[REGENERATE_PARAM].getValue())) {
            globalRegenNeeded = true;
        }

        // Update synth params without full regeneration
        if (synthUpdateNeeded && !globalRegenNeeded) {
            applySynthModifiers();
        }

        for (int r = 0; r < 4; r++) {
            int baseParam = r * 5;

            // Read with CV modulation
            float styleCV = 0.0f;
            if (inputs[TIMELINE_STYLE_CV_INPUT + r * 4].isConnected()) {
                styleCV = inputs[TIMELINE_STYLE_CV_INPUT + r * 4].getVoltage();
            }
            int styleIndex = static_cast<int>(params[TIMELINE_STYLE_PARAM + baseParam].getValue() + styleCV);
            styleIndex = clamp(styleIndex, 0, 9);

            float densityCV = 0.0f;
            if (inputs[TIMELINE_DENSITY_CV_INPUT + r * 4].isConnected()) {
                densityCV = inputs[TIMELINE_DENSITY_CV_INPUT + r * 4].getVoltage() * 0.1f;
            }
            float density = clamp(params[TIMELINE_DENSITY_PARAM + baseParam].getValue() + densityCV, 0.0f, 0.9f);

            int length = static_cast<int>(params[TIMELINE_LENGTH_PARAM + baseParam].getValue());

            // Check if regeneration needed for this role
            // Special case: density=0 should ALWAYS trigger regen to ensure silence
            bool densityBecameZero = (density < 0.01f && lastDensities[r] >= 0.01f);
            bool densityChanged = std::abs(density - lastDensities[r]) > 0.04f;  // Reduced threshold
            bool needsRegen = globalRegenNeeded ||
                              styleIndex != lastStyles[r] ||
                              densityBecameZero ||
                              densityChanged ||
                              length != lastLengths[r];

            if (needsRegen) {
                regenerateRolePattern(r);
            }
        }

        if (globalRegenNeeded) {
            lastVariation = variation;
            appliedRest = restAmount;
        }

        // Check if REST amount changed significantly (reapply without regen)
        if (std::abs(restAmount - appliedRest) > 0.03f) {
            reapplyRest(restAmount);
        }

        // Process reset (input or button)
        bool resetTriggered = resetTrigger.process(inputs[RESET_INPUT].getVoltage()) ||
                              resetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue());
        if (resetTriggered) {
            resetSteps();
        }

        // Process fill trigger (manual via FILL_INPUT)
        float fillAmount = params[FILL_PARAM].getValue();
        if (fillTrigger.process(inputs[FILL_INPUT].getVoltage())) {
            // Manual trigger: always generate fill (use fillAmount as intensity)
            if (fillAmount > 0.01f) {
                generateFillPatterns(fillAmount);
            }
        }

        // Process clock with PPQN division
        // ppqn=4: every clock = 1 step (16th note input)
        // ppqn=2: every clock = 2 steps (8th note input)
        // ppqn=1: every clock = 4 steps (quarter note input)
        if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
            clockPulse.trigger(0.001f);

            // Calculate steps per clock based on PPQN
            // If ppqn=4, external clock is 16th notes, advance 1 step per clock
            // If ppqn=2, external clock is 8th notes, advance 2 steps per clock
            // If ppqn=1, external clock is quarter notes, advance 4 steps per clock
            int stepsPerClock = 4 / ppqn;

            // Get swing amount for timing offset calculation
            float swingAmount = params[SWING_PARAM].getValue();

            // Process each role independently (polymeter support)
            // Get current groove template for timing offsets
            float humanizeAmount = params[HUMANIZE_PARAM].getValue();
            const WorldRhythm::GrooveTemplate& groove = humanize.getCurrentGroove();

            // Track bar (based on longest pattern)
            int maxLen = *std::max_element(roleLengths, roleLengths + 4);

            // ========================================
            // Fill Logic (deterministic rule-based, like breakbeat_cpp)
            // ========================================
            // Bar 2 (barInPhrase == 1): Small fill on beat 4 (last 4 steps)
            // Bar 4 (barInPhrase == 3): Large fill on beats 3-4 (last 8 steps)
            // fillAmount controls intensity, not probability
            if (globalStep == 0 && !fillActive) {
                if (fillAmount > 0.01f) {
                    int barInPhrase = currentBar % 4;  // 0, 1, 2, 3

                    if (barInPhrase == 3) {
                        // Bar 4, 8, 12...: Large fill (beats 3-4 = 8 steps)
                        nextBarHasFill = true;
                        fillLengthStepsPlanned = 8;  // 2 beats = 8 steps
                    } else if (barInPhrase == 1 && fillAmount > 0.3f) {
                        // Bar 2, 6, 10...: Small fill (beat 4 = 4 steps) only if fillAmount > 30%
                        nextBarHasFill = true;
                        fillLengthStepsPlanned = 4;  // 1 beat = 4 steps
                    } else {
                        nextBarHasFill = false;
                    }

                    if (nextBarHasFill) {
                        // Ensure fill fits within bar
                        fillLengthStepsPlanned = std::min(fillLengthStepsPlanned, maxLen);
                        fillStartStep = maxLen - fillLengthStepsPlanned;
                        if (fillStartStep < 0) fillStartStep = 0;
                    }
                } else {
                    nextBarHasFill = false;
                }
            }

            // Step 2: At fillStartStep, trigger the fill
            if (nextBarHasFill && !fillActive && globalStep == fillStartStep) {
                generateFillPatterns(fillAmount);
                nextBarHasFill = false;  // Consumed
            }

            // Process multiple steps if PPQN < 4 (lower resolution clock input)
            for (int stepOffset = 0; stepOffset < stepsPerClock; stepOffset++) {
                for (int r = 0; r < 4; r++) {
                    int step = currentSteps[r];
                    int voiceBase = r * 2;

                    // Determine which pattern to use (fill or normal)
                    int fillStep = fillActive ? (static_cast<int>(fillPatterns.patterns[0].length) - fillStepsRemaining) : step;
                    int useStep = fillActive ? fillStep : step;

                    // Calculate timing delay from groove template + swing
                    // Groove template provides position-specific microtiming (LaidBack, Pushed, etc.)
                    int pos = useStep % 16;
                    float grooveOffsetMs = groove.offsets[pos] * humanizeAmount;

                    // Add swing delay for off-beat positions
                    float swingDelayMs = 0.0f;
                    if ((useStep % 2) == 1 && swingAmount > 0.01f) {
                        swingDelayMs = swingAmount * 40.0f;
                    }

                    // Combined timing: groove offset + swing (can be negative for Pushed groove)
                    float totalDelayMs = grooveOffsetMs + swingDelayMs;
                    float totalDelaySamples = (totalDelayMs / 1000.0f) * args.sampleRate;

                    // Pre-calculate decay multiplier for VCA envelopes (used by both voices)
                    int baseParam = r * 5;  // 5 params per role (SPREAD is separate)
                    float decayMult = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
                    if (inputs[TIMELINE_DECAY_CV_INPUT + r * 4].isConnected()) {
                        decayMult += inputs[TIMELINE_DECAY_CV_INPUT + r * 4].getVoltage() * 0.18f;
                        decayMult = clamp(decayMult, 0.2f, 2.0f);
                    }

                    // Determine if this is a strong beat (positions 0, 4, 8, 12 in 16-step)
                    // Used by both primary and secondary voices
                    bool isStrongBeat = (useStep % 4 == 0);

                    // Primary voice
                    WorldRhythm::Pattern& primaryPattern = fillActive ? fillPatterns.patterns[voiceBase] : patterns.patterns[voiceBase];
                    if (useStep < static_cast<int>(primaryPattern.length) && primaryPattern.hasOnsetAt(useStep)) {
                        float vel = primaryPattern.getVelocity(useStep);
                        // Apply groove velocity modifier
                        vel *= groove.velMods[pos];
                        vel = std::clamp(vel, 0.0f, 1.0f);
                        bool accent = primaryPattern.accents[useStep];

                        if (totalDelaySamples > 1.0f) {
                            // Positive delay: use delayed trigger
                            delayedTriggers.push_back({totalDelaySamples, voiceBase, vel, accent, r, isStrongBeat});
                        } else {
                            // Zero or negative delay: trigger immediately
                            // (negative means "ahead of beat" - we trigger now, which is effectively early)
                            triggerWithArticulation(voiceBase, vel, accent, args.sampleRate, r, isStrongBeat);
                            // Trigger VCA for external audio (use decay parameter for envelope length)
                            // Base decay of 200ms, scaled by decay parameter and velocity
                            float vcaDecayMs = 200.0f * decayMult;
                            externalVCA[voiceBase].trigger(vcaDecayMs, args.sampleRate, vel);
                        }
                    }

                    // Secondary voice
                    WorldRhythm::Pattern& secondaryPattern = fillActive ? fillPatterns.patterns[voiceBase + 1] : patterns.patterns[voiceBase + 1];
                    if (useStep < static_cast<int>(secondaryPattern.length) && secondaryPattern.hasOnsetAt(useStep)) {
                        float vel = secondaryPattern.getVelocity(useStep);
                        // Apply groove velocity modifier
                        vel *= groove.velMods[pos];
                        vel = std::clamp(vel, 0.0f, 1.0f);
                        bool accent = secondaryPattern.accents[useStep];
                        if (totalDelaySamples > 1.0f) {
                            delayedTriggers.push_back({totalDelaySamples, voiceBase + 1, vel, accent, r, isStrongBeat});
                        } else {
                            triggerWithArticulation(voiceBase + 1, vel, accent, args.sampleRate, r, isStrongBeat);
                            // Trigger VCA for external audio (use decay parameter for envelope length)
                            float vcaDecayMs2 = 200.0f * decayMult;
                            externalVCA[voiceBase + 1].trigger(vcaDecayMs2, args.sampleRate, vel);
                        }
                    }

                    // Advance step for this role (always, even during fill)
                    // This keeps currentSteps in sync with globalStep
                    currentSteps[r]++;
                    if (currentSteps[r] >= roleLengths[r]) {
                        currentSteps[r] = 0;
                    }
                }

                // Handle fill progress
                if (fillActive) {
                    fillStepsRemaining--;
                    if (fillStepsRemaining <= 0) {
                        fillActive = false;
                        currentFillType = WorldRhythm::FillType::NONE;
                    }
                }
            }

            // Increment global step after processing
            globalStep += stepsPerClock;

            if (globalStep >= maxLen) {
                globalStep = 0;
                currentBar++;
            }
        }

        // Process audio with internal/external mix and stereo spread
        float mixL = 0.0f;
        float mixR = 0.0f;

        float spread = params[SPREAD_PARAM].getValue();

        // Role-based stereo panning (based on mixing research)
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        // Pan positions for voice1 and voice2 per role (at spread=1.0)
        // Foundation: both center (low freq rule)
        // Timeline: both slightly right (like hi-hat/clave)
        // Groove: split left/right (like congas/toms)
        // Lead: both left (like bongos, balances Timeline on right)
        const float rolePanV1[4] = { 0.20f,  0.0f, -0.30f, -0.40f};  // Timeline, Foundation, Groove, Lead
        const float rolePanV2[4] = { 0.25f,  0.0f,  0.30f, -0.50f};  // Timeline, Foundation, Groove, Lead

        for (int r = 0; r < 4; r++) {
            int voiceBase = r * 2;
            float mix = params[TIMELINE_MIX_PARAM + r].getValue();  // 0.0 = internal, 1.0 = external
            currentMix[r] = mix;

            // Get pan positions for this role
            float pan1 = rolePanV1[r] * spread;
            float pan2 = rolePanV2[r] * spread;

            // Voice 1 (Primary)
            int v1 = voiceBase;
            float synthAudio1 = drumSynth.processVoice(v1) * 5.0f;

            // Process external audio input 1 with VCA envelope
            float extAudio1 = 0.0f;
            if (inputs[TIMELINE_AUDIO_INPUT_1 + r * 2].isConnected()) {
                float externalSignal = inputs[TIMELINE_AUDIO_INPUT_1 + r * 2].getVoltage();
                float vcaGain = externalVCA[v1].process();
                extAudio1 = externalSignal * vcaGain * currentVelocities[v1];
                if (currentAccents[v1]) {
                    extAudio1 *= 1.5f;  // Accent boost
                }
            }

            // Mix internal synth and external audio based on mix parameter
            float combined1 = synthAudio1 * (1.0f - mix) + extAudio1 * mix;

            outputs[VOICE1_AUDIO_OUTPUT + v1].setVoltage(combined1);

            // Apply stereo panning (linear panning)
            float gainL1 = 0.5f * (1.0f - pan1);
            float gainR1 = 0.5f * (1.0f + pan1);
            mixL += combined1 * gainL1;
            mixR += combined1 * gainR1;

            // Voice 2 (Secondary)
            int v2 = voiceBase + 1;
            float synthAudio2 = drumSynth.processVoice(v2) * 5.0f;

            // Process external audio input 2 with VCA envelope
            float extAudio2 = 0.0f;
            if (inputs[TIMELINE_AUDIO_INPUT_2 + r * 2].isConnected()) {
                float externalSignal = inputs[TIMELINE_AUDIO_INPUT_2 + r * 2].getVoltage();
                float vcaGain = externalVCA[v2].process();
                extAudio2 = externalSignal * vcaGain * currentVelocities[v2];
                if (currentAccents[v2]) {
                    extAudio2 *= 1.5f;  // Accent boost
                }
            }

            // Mix internal synth and external audio based on mix parameter
            float combined2 = synthAudio2 * (1.0f - mix) + extAudio2 * mix;

            outputs[VOICE1_AUDIO_OUTPUT + v2].setVoltage(combined2);

            // Apply stereo panning (linear panning)
            float gainL2 = 0.5f * (1.0f - pan2);
            float gainR2 = 0.5f * (1.0f + pan2);
            mixL += combined2 * gainL2;
            mixR += combined2 * gainR2;
        }

        outputs[MIX_L_OUTPUT].setVoltage(std::tanh(mixL) * 5.0f);
        outputs[MIX_R_OUTPUT].setVoltage(std::tanh(mixR) * 5.0f);

        // Output gates, CV, accents and update lights
        bool clockGate = clockPulse.process(args.sampleTime);
        lights[CLOCK_LIGHT].setBrightness(clockGate ? 1.0f : 0.0f);

        // C4 = 261.63 Hz = 0V (1V/Oct standard)
        const float C4_FREQ = 261.63f;

        for (int i = 0; i < 8; i++) {
            bool gate = gatePulses[i].process(args.sampleTime);

            outputs[VOICE1_GATE_OUTPUT + i].setVoltage(gate ? 10.0f : 0.0f);

            // Pitch CV: 1V/Oct, C4 (261.63Hz) = 0V
            float pitchCV = 0.0f;
            if (currentFreqs[i] > 0) {
                pitchCV = std::log2(currentFreqs[i] / C4_FREQ);
            }
            outputs[VOICE1_CV_OUTPUT + i].setVoltage(pitchCV);

            // Velocity CV: 0-10V
            outputs[VOICE1_ACCENT_OUTPUT + i].setVoltage(currentVelocities[i] * 10.0f);

            lights[VOICE1_LIGHT + i].setBrightness(gate ? 1.0f : 0.0f);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "currentBar", json_integer(currentBar));
        json_object_set_new(rootJ, "ppqn", json_integer(ppqn));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);
        json_t* barJ = json_object_get(rootJ, "currentBar");
        if (barJ) currentBar = json_integer_value(barJ);
        json_t* ppqnJ = json_object_get(rootJ, "ppqn");
        if (ppqnJ) ppqn = json_integer_value(ppqnJ);
    }
};

// ============================================================================
// URDynamicRoleTitle::draw implementation (after UniversalRhythm definition)
// ============================================================================

void URDynamicRoleTitle::draw(const DrawArgs &args) {
    NVGcolor color = nvgRGB(255, 255, 255);  // Default white

    if (module) {
        int baseParam = roleIndex * 5;
        int styleIndex = static_cast<int>(module->params[UniversalRhythm::TIMELINE_STYLE_PARAM + baseParam].getValue());
        styleIndex = clamp(styleIndex, 0, 9);
        color = STYLE_COLORS[styleIndex];
    }

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // Draw white outline (1px)
    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
    for (float dx = -1.0f; dx <= 1.0f; dx += 1.0f) {
        for (float dy = -1.0f; dy <= 1.0f; dy += 1.0f) {
            if (dx != 0 || dy != 0) {
                nvgText(args.vg, box.size.x / 2.f + dx, box.size.y / 2.f + dy, text.c_str(), NULL);
            }
        }
    }

    // Draw main text with color
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

void URStyleNameDisplay::draw(const DrawArgs &args) {
    NVGcolor color = nvgRGB(255, 255, 255);  // Default white
    const char* styleName = "W.African";  // Default

    if (module) {
        int baseParam = roleIndex * 5;
        int styleIndex = static_cast<int>(module->params[UniversalRhythm::TIMELINE_STYLE_PARAM + baseParam].getValue());
        styleIndex = clamp(styleIndex, 0, 9);
        color = STYLE_COLORS[styleIndex];
        styleName = STYLE_NAMES[styleIndex];
    }

    nvgFontSize(args.vg, fontSize);
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // Draw with slight outline for readability
    nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 100));
    nvgText(args.vg, box.size.x / 2.f + 0.5f, box.size.y / 2.f + 0.5f, styleName, NULL);

    nvgFillColor(args.vg, color);
    nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, styleName, NULL);
}

// ============================================================================
// Pattern Display Widget
// ============================================================================

struct URPatternDisplay : TransparentWidget {
    UniversalRhythm* module = nullptr;

    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.0f);
        nvgFillColor(args.vg, nvgRGB(25, 25, 25));
        nvgFill(args.vg);

        if (!module) return;

        // Safety check: ensure params vector is initialized
        if (module->params.empty()) return;

        float rowHeight = box.size.y / 8.0f;

        // Get colors from each role's style (2 voices per role, secondary is slightly dimmer)
        NVGcolor colors[8];
        for (int role = 0; role < 4; role++) {
            int baseParam = role * 5;
            int paramIdx = UniversalRhythm::TIMELINE_STYLE_PARAM + baseParam;
            // Safety check for param index
            if (paramIdx < 0 || paramIdx >= static_cast<int>(module->params.size())) continue;
            int styleIndex = static_cast<int>(module->params[paramIdx].getValue());
            styleIndex = clamp(styleIndex, 0, 9);
            NVGcolor baseColor = STYLE_COLORS[styleIndex];
            colors[role * 2] = baseColor;  // Primary voice - full color
            // Secondary voice - slightly dimmer
            colors[role * 2 + 1] = nvgRGBA(baseColor.r * 255 * 0.7f, baseColor.g * 255 * 0.7f, baseColor.b * 255 * 0.7f, 200);
        }

        // Draw patterns (each role may have different length)
        // Display order (top to bottom): Lead, Groove, Timeline, Foundation (frequency high to low)
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        // Display row mapping: row 0-1 = Lead(3), row 2-3 = Groove(2), row 4-5 = Timeline(0), row 6-7 = Foundation(1)
        const int displayToRole[4] = {3, 2, 0, 1};  // Lead, Groove, Timeline, Foundation

        for (int displayRow = 0; displayRow < 4; displayRow++) {
            int role = displayToRole[displayRow];
            int length = module->roleLengths[role];

            // Safety check: skip if length is invalid (prevents division by zero and inf)
            if (length <= 0 || length > 64) continue;

            int step = module->currentSteps[role];
            // Clamp step to valid range
            step = clamp(step, 0, length - 1);

            float stepWidth = box.size.x / length;

            // Current step indicator for this role
            int baseParam = role * 5;
            int paramIdx2 = UniversalRhythm::TIMELINE_STYLE_PARAM + baseParam;
            if (paramIdx2 < 0 || paramIdx2 >= static_cast<int>(module->params.size())) continue;
            int styleIndex = static_cast<int>(module->params[paramIdx2].getValue());
            styleIndex = clamp(styleIndex, 0, 9);
            NVGcolor stepColor = STYLE_COLORS[styleIndex];
            nvgBeginPath(args.vg);
            nvgRect(args.vg, step * stepWidth, displayRow * rowHeight * 2, stepWidth, rowHeight * 2);
            nvgFillColor(args.vg, nvgRGBA(stepColor.r * 255, stepColor.g * 255, stepColor.b * 255, 60));
            nvgFill(args.vg);

            // Draw two voices for this role
            for (int voiceIdx = 0; voiceIdx < 2; voiceIdx++) {
                int v = role * 2 + voiceIdx;
                float y = (displayRow * 2 + voiceIdx) * rowHeight + rowHeight / 2.0f;

                // Safety check: ensure pattern is valid
                if (v < 0 || v >= 8) continue;
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
// Module Widget - 40HP
// ============================================================================

struct UniversalRhythmWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    UniversalRhythmWidget(UniversalRhythm* module) {
        setModule(module);
        panelThemeHelper.init(this, "40HP");

        box.size = Vec(40 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Title (MADDY+ style - left aligned with spacing)
        NVGcolor titleColor = nvgRGB(255, 200, 0);
        addChild(new URTextLabel(Vec(27, 1), Vec(box.size.x, 20), "U N I V E R S A L  R H Y T H M", 12.f, titleColor, true));
        addChild(new URTextLabel(Vec(27, 13), Vec(box.size.x, 20), "MADZINE", 10.f, titleColor, false));

        // Pattern display
        {
            URPatternDisplay* display = new URPatternDisplay();
            display->box.pos = Vec(15, 42);
            display->box.size = Vec(box.size.x - 30, 50);
            display->module = module;
            addChild(display);
        }

        // Clock / Reset / Regen inputs (+2px down)
        float ctrlY = 120;  // +2px more
        float ctrlLabelY = 101;  // Labels stay fixed
        float ctrlSpacing = 38;  // More spacing for labels

        // Labels centered over inputs (adjust X position)
        addChild(new URTextLabel(Vec(5, ctrlLabelY), Vec(40, 12), "CLOCK", 7.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(25, ctrlY + 5), module, UniversalRhythm::CLOCK_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(37, ctrlY - 2), module, UniversalRhythm::CLOCK_LIGHT));

        // Reset button (where RESET input used to be)
        addChild(new URTextLabel(Vec(5 + ctrlSpacing, ctrlLabelY), Vec(40, 12), "RESET", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<VCVButton>(Vec(25 + ctrlSpacing, ctrlY + 5), module, UniversalRhythm::RESET_BUTTON_PARAM));
        // Reset input moved to right of button
        addInput(createInputCentered<PJ301MPort>(Vec(48 + ctrlSpacing, ctrlY + 5), module, UniversalRhythm::RESET_INPUT));

        addChild(new URTextLabel(Vec(5 + ctrlSpacing * 2 + 10, ctrlLabelY), Vec(40, 12), "REGEN", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<VCVButton>(Vec(25 + ctrlSpacing * 2 + 10, ctrlY + 5), module, UniversalRhythm::REGENERATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(48 + ctrlSpacing * 2 + 10, ctrlY + 5), module, UniversalRhythm::REGENERATE_INPUT));

        // Global parameters (right side of control row) - shifted right
        float globalX = 175;
        float globalSpacing = 35;
        addChild(new URTextLabel(Vec(globalX - 20, ctrlLabelY), Vec(40, 12), "VARI", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(globalX, ctrlY + 5), module, UniversalRhythm::VARIATION_PARAM));

        globalX += globalSpacing;
        addChild(new URTextLabel(Vec(globalX - 20, ctrlLabelY), Vec(40, 12), "HUMAN", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(globalX, ctrlY + 5), module, UniversalRhythm::HUMANIZE_PARAM));

        globalX += globalSpacing;
        addChild(new URTextLabel(Vec(globalX - 20, ctrlLabelY), Vec(40, 12), "SWING", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(globalX, ctrlY + 5), module, UniversalRhythm::SWING_PARAM));

        globalX += globalSpacing;
        addChild(new URTextLabel(Vec(globalX - 20, ctrlLabelY), Vec(40, 12), "REST", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(globalX, ctrlY + 5), module, UniversalRhythm::REST_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(globalX + 25, ctrlY + 5), module, UniversalRhythm::REST_CV_INPUT));

        // Fill section (REST CV input X + 35)
        float fillX = globalX + 25 + 35;  // REST CV input + 35
        addChild(new URTextLabel(Vec(fillX - 10, ctrlLabelY), Vec(20, 12), "FILL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(fillX, ctrlY + 5), module, UniversalRhythm::FILL_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(fillX + 25, ctrlY + 5), module, UniversalRhythm::FILL_INPUT));

        // Articulation section
        float artX = fillX + 25 + 35;  // FILL CV input + 35
        addChild(new URTextLabel(Vec(artX - 24, ctrlLabelY), Vec(48, 12), "Articulation", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(artX, ctrlY + 5), module, UniversalRhythm::ARTICULATION_PARAM));

        artX += 33;
        addChild(new URTextLabel(Vec(artX - 13, ctrlLabelY), Vec(26, 12), "GHOST", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(artX, ctrlY + 5), module, UniversalRhythm::GHOST_PARAM));

        artX += 33;
        addChild(new URTextLabel(Vec(artX - 15, ctrlLabelY), Vec(30, 12), "ACCENT", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(artX, ctrlY + 5), module, UniversalRhythm::ACCENT_PROB_PARAM));

        artX += 43;  // +10px extra spacing before SPREAD
        addChild(new URTextLabel(Vec(artX - 15, ctrlLabelY), Vec(30, 12), "SPREAD", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(artX, ctrlY + 5), module, UniversalRhythm::SPREAD_PARAM));

        // Separators (at vertical separator top position Y=151)
        addChild(new URHorizontalLine(Vec(0, 151), Vec(box.size.x, 1)));  // Below global controls

        // ===== Per-Role Section =====
        float roleY = 180;  // +10px more
        // 40HP = 609.6px, divide evenly for 4 roles
        float roleSpacing = 152;  // 609.6 / 4 = ~152
        float roleStartX = 76;    // Center of first role section (152/2)
        float knobVSpacing = 49;  // Vertical spacing between knobs (+4)
        float labelToKnob = 25;   // Distance from label to knob center (+2)

        // Role display order (left to right): Foundation, Timeline, Groove, Lead
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        // UI position mapping: pos 0 = Foundation(1), pos 1 = Timeline(0), pos 2 = Groove(2), pos 3 = Lead(3)
        const int uiToRole[4] = {1, 0, 2, 3};  // Foundation, Timeline, Groove, Lead
        const char* roleNames[4] = {"FOUNDATION", "TIMELINE", "GROOVE", "LEAD"};
        NVGcolor white = nvgRGB(255, 255, 255);

        for (int uiPos = 0; uiPos < 4; uiPos++) {
            int role = uiToRole[uiPos];
            float x = roleStartX + uiPos * roleSpacing;
            int baseParam = role * 5;  // 5 params per role

            // Role name - dynamic color based on style (Y-3 more)
            URDynamicRoleTitle* roleTitle = new URDynamicRoleTitle(Vec(x - 55, roleY - 24), Vec(110, 18), roleNames[uiPos], role, 17.5f, true);
            roleTitle->module = module;
            addChild(roleTitle);

            // Style name display below role title (2x size = 16f, Y-3 more)
            URStyleNameDisplay* styleDisplay = new URStyleNameDisplay(Vec(x - 40, roleY - 9), Vec(80, 14), role, 16.f);
            styleDisplay->module = module;
            addChild(styleDisplay);

            // Left column: STY, DNS, LEN with CV inputs
            float leftCol = x - 45 - 4 - 4;  // X-8 total

            // Style (label above knob) - Y+6
            addChild(new URTextLabel(Vec(leftCol - 14, roleY + 8), Vec(30, 10), "STYLE", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(leftCol, roleY + 8 + labelToKnob), module,
                     UniversalRhythm::TIMELINE_STYLE_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(Vec(leftCol + 28, roleY + 8 + labelToKnob), module,
                     UniversalRhythm::TIMELINE_STYLE_CV_INPUT + role * 4));

            // Density (label above knob) - Y+3
            addChild(new URTextLabel(Vec(leftCol - 14, roleY + 5 + knobVSpacing), Vec(30, 10), "DENSITY", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(leftCol, roleY + 5 + knobVSpacing + labelToKnob), module,
                     UniversalRhythm::TIMELINE_DENSITY_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(Vec(leftCol + 28, roleY + 5 + knobVSpacing + labelToKnob), module,
                     UniversalRhythm::TIMELINE_DENSITY_CV_INPUT + role * 4));

            // Length (label above knob)
            addChild(new URTextLabel(Vec(leftCol - 14, roleY + 2 + knobVSpacing * 2), Vec(30, 10), "LENGTH", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::WhiteKnob>(Vec(leftCol, roleY + 2 + knobVSpacing * 2 + labelToKnob), module,
                     UniversalRhythm::TIMELINE_LENGTH_PARAM + baseParam));

            // Right column: FRQ, DEC with CV inputs
            float rightCol = x + 15;  // X-10 for new CV inputs

            // Freq (label above knob) - Y+6
            addChild(new URTextLabel(Vec(rightCol - 14, roleY + 8), Vec(30, 10), "FREQ", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(rightCol, roleY + 8 + labelToKnob), module,
                     UniversalRhythm::TIMELINE_FREQ_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(Vec(rightCol + 28, roleY + 8 + labelToKnob), module,
                     UniversalRhythm::TIMELINE_FREQ_CV_INPUT + role * 4));

            // Decay (label above knob) - Y+3
            addChild(new URTextLabel(Vec(rightCol - 14, roleY + 5 + knobVSpacing), Vec(30, 10), "DECAY", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(rightCol, roleY + 5 + knobVSpacing + labelToKnob), module,
                     UniversalRhythm::TIMELINE_DECAY_PARAM + baseParam));
            addInput(createInputCentered<PJ301MPort>(Vec(rightCol + 28, roleY + 5 + knobVSpacing + labelToKnob), module,
                     UniversalRhythm::TIMELINE_DECAY_CV_INPUT + role * 4));

            // Row 3 right side: IN1, SPR, IN2 (using same Y as Length row)
            // X coordinates: leftCol + 28 (Density CV), rightCol (Decay), rightCol + 28 (Decay CV)
            float row3LabelY = roleY + 2 + knobVSpacing * 2;
            float row3ElementY = row3LabelY + labelToKnob;

            // EXT IN 1 at leftCol + 28 + 12 position (compensate for leftCol X-8)
            addChild(new URTextLabel(Vec(leftCol + 28 + 12 - 14, row3LabelY), Vec(30, 10), "EXT IN 1", 7.f, white, true));
            addInput(createInputCentered<PJ301MPort>(Vec(leftCol + 28 + 12, row3ElementY), module,
                     UniversalRhythm::TIMELINE_AUDIO_INPUT_1 + role * 2));

            // MIX at rightCol position (same X as Decay knob) - 0=internal, 1=external
            addChild(new URTextLabel(Vec(rightCol - 15, row3LabelY), Vec(30, 10), "MIX", 7.f, white, true));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(rightCol, row3ElementY), module,
                     UniversalRhythm::TIMELINE_MIX_PARAM + role));

            // EXT IN 2 at rightCol + 28 position (same X as Decay CV input)
            addChild(new URTextLabel(Vec(rightCol + 28 - 14, row3LabelY), Vec(30, 10), "EXT IN 2", 7.f, white, true));
            addInput(createInputCentered<PJ301MPort>(Vec(rightCol + 28, row3ElementY), module,
                     UniversalRhythm::TIMELINE_AUDIO_INPUT_2 + role * 2));
        }

        // Vertical separators between roles
        for (int r = 0; r < 3; r++) {
            float sepX = (r + 1) * roleSpacing;
            addChild(new URVerticalLine(Vec(sepX, 151), Vec(1, 110)));
        }

        // ===== White Output Area at Y=330 (MADDY+ style) =====
        addChild(new URWhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        // Output layout: ~598px total
        // Left labels (40px) + 8 voices x 4 outputs + gaps + MIX
        NVGcolor labelColor = nvgRGB(255, 133, 133);  // MADDY+ pink labels
        float row1Y = 343;
        float row2Y = 368;
        float mixY = (row1Y + row2Y) / 2;  // Centered between two rows (355.5)

        // Left side labels (type indicators) - 40px width
        addChild(new URTextLabel(Vec(3, 337), Vec(18, 15), "Audio", 6.f, labelColor, true));
        addChild(new URTextLabel(Vec(21, 337), Vec(18, 15), "Gate", 6.f, labelColor, true));
        addChild(new URTextLabel(Vec(3, 362), Vec(18, 15), "Freq", 6.f, labelColor, true));
        addChild(new URTextLabel(Vec(21, 362), Vec(18, 15), "Velo", 6.f, labelColor, true));

        // 8 voices: each voice has 4 outputs (AUD, GATE on row1; CV, ACC on row2)
        // Port spacing within voice: 30px (26+4), voice name centered above
        float voiceStartX = 50;  // After label area (40px + margin)
        float portSpacing = 30;  // AUD to GATE, CV to ACC
        float voiceWidth = portSpacing * 2;  // Two ports per row
        float voiceGap = 8 - 2;  // Gap between voices, X-2 as requested

        // Voice display order matches role UI order: Foundation, Timeline, Groove, Lead
        const char* voiceNames[8] = {"FD1", "FD2", "TL1", "TL2", "GR1", "GR2", "LD1", "LD2"};
        // Voice index mapping: UI position to actual voice index
        // Role indices: 0=Timeline, 1=Foundation, 2=Groove, 3=Lead
        // UI order: Foundation(1), Timeline(0), Groove(2), Lead(3)
        const int voiceUIToActual[8] = {2, 3, 0, 1, 4, 5, 6, 7};  // FD1,FD2,TL1,TL2,GR1,GR2,LD1,LD2

        for (int i = 0; i < 8; i++) {
            float voiceX = voiceStartX + i * (voiceWidth + voiceGap);
            int actualVoice = voiceUIToActual[i];  // Map UI position to actual voice index

            // Voice name label (centered above the two ports, same Y as MIX output)
            addChild(new URTextLabel(Vec(voiceX + portSpacing/2 - 10, mixY - 5), Vec(20, 10), voiceNames[i], 6.f, labelColor, true));

            // Row 1: AUD output, GATE output
            addOutput(createOutputCentered<PJ301MPort>(Vec(voiceX, row1Y), module, UniversalRhythm::VOICE1_AUDIO_OUTPUT + actualVoice));
            addOutput(createOutputCentered<PJ301MPort>(Vec(voiceX + portSpacing, row1Y), module, UniversalRhythm::VOICE1_GATE_OUTPUT + actualVoice));

            // Row 2: CV output, ACC output
            addOutput(createOutputCentered<PJ301MPort>(Vec(voiceX, row2Y), module, UniversalRhythm::VOICE1_CV_OUTPUT + actualVoice));
            addOutput(createOutputCentered<PJ301MPort>(Vec(voiceX + portSpacing, row2Y), module, UniversalRhythm::VOICE1_ACCENT_OUTPUT + actualVoice));
        }

        // MIX L/R outputs at the end (vertically stacked)
        float mixOutputX = voiceStartX + 8 * (voiceWidth + voiceGap) + 1;  // Output X position
        float mixLabelCenterX = mixOutputX - 18;  // Label center X (left of outputs)
        // MIX label - box.pos.x = centerX - box.size.x/2 for proper centering
        addChild(new URTextLabel(Vec(mixLabelCenterX - 10, mixY - 5), Vec(20, 10), "MIX", 6.f, labelColor, true));
        // MIX outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixOutputX, row1Y), module, UniversalRhythm::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(mixOutputX, row2Y), module, UniversalRhythm::MIX_R_OUTPUT));
    }

    void step() override {
        UniversalRhythm* module = dynamic_cast<UniversalRhythm*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        UniversalRhythm* module = dynamic_cast<UniversalRhythm*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator());

        // PPQN selection menu
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

Model* modelUniversalRhythm = createModel<UniversalRhythm, UniversalRhythmWidget>("UniversalRhythm");
