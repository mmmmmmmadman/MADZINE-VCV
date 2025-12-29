#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include "Microtuning/MicrotunePresets.hpp"

using namespace Microtuning;

// ============================================================================
// Helper Widgets
// ============================================================================

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

        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
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

// Forward declaration
struct Quantizer;

// ============================================================================
// Quantizer Module
// ============================================================================

struct Quantizer : Module {
    int panelTheme = -1; // -1 = Auto (follow VCV)

    enum ParamIds {
        SCALE_PARAM, OFFSET_PARAM,
        MICROTUNE_PARAM, // 24 microtune params (MICROTUNE_PARAM + 0..23)
        NUM_PARAMS = MICROTUNE_PARAM + 24
    };
    enum InputIds { PITCH_INPUT, PITCH_INPUT_2, PITCH_INPUT_3, SCALE_CV_INPUT, OFFSET_CV_INPUT, NUM_INPUTS };
    enum OutputIds { PITCH_OUTPUT, PITCH_OUTPUT_2, PITCH_OUTPUT_3, NUM_OUTPUTS };
    enum LightIds { NUM_LIGHTS };

    bool enabledNotes[24];  // 24 notes (2 octaves)
    int ranges[48];         // 48 half-semitone slots for 2 octaves
    bool playingNotes[24];

    // Direction tracking
    static const int MAX_POLY = 16;
    float lastNote[3][MAX_POLY] = {};
    bool ascending[3][MAX_POLY] = {};
    float ascCents[12] = {}, descCents[12] = {};
    bool hasDirectional = false;
    int currentPreset = 0;

    // CV 調變顯示用
    float scaleCvMod = 0.0f;
    float offsetCvMod = 0.0f;

    // Note names for 2 octaves
    static constexpr const char* NOTE_NAMES[24] = {
        "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
        "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2"
    };

    // Scale presets (note enabled states) - applied to both octaves
    static const bool SCALES[16][12];

    Quantizer() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SCALE_PARAM, 0.f, 2.f, 1.f, "Scale", "%", 0.f, 100.f);
        configParam(OFFSET_PARAM, -1.f, 1.f, 0.f, "Pre-offset", " semitones", 0.f, 12.f);

        for (int i = 0; i < 24; i++)
            configParam(MICROTUNE_PARAM + i, -50.f, 50.f, 0.f, std::string(NOTE_NAMES[i]) + " Microtune", " cents");

        configInput(PITCH_INPUT, "CV1");
        configInput(PITCH_INPUT_2, "CV2");
        configInput(PITCH_INPUT_3, "CV3");
        configInput(SCALE_CV_INPUT, "Scale CV");
        configInput(OFFSET_CV_INPUT, "Offset CV");
        configOutput(PITCH_OUTPUT, "Pitch");
        configOutput(PITCH_OUTPUT_2, "Pitch 2");
        configOutput(PITCH_OUTPUT_3, "Pitch 3");
        configBypass(PITCH_INPUT, PITCH_OUTPUT);
        onReset();
    }

    void onReset() override {
        for (int i = 0; i < 24; i++)
            enabledNotes[i] = true;
        for (int i = 0; i < 12; i++)
            ascCents[i] = descCents[i] = 0.f;
        for (int t = 0; t < 3; t++)
            for (int c = 0; c < MAX_POLY; c++) {
                lastNote[t][c] = 0.f;
                ascending[t][c] = true;
            }
        hasDirectional = false;
        updateRanges();
    }

    void onRandomize() override {
        for (int i = 0; i < 24; i++)
            enabledNotes[i] = random::uniform() < 0.5f;
        updateRanges();
    }

    void process(const ProcessArgs& args) override {
        bool playing[24] = {};
        float scale = params[SCALE_PARAM].getValue();
        if (inputs[SCALE_CV_INPUT].isConnected()) {
            float cv = inputs[SCALE_CV_INPUT].getVoltage();
            scale += cv * 0.2f;  // ±1V = ±0.2 scale
            scaleCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            scaleCvMod = 0.0f;
        }
        float offset = params[OFFSET_PARAM].getValue();
        if (inputs[OFFSET_CV_INPUT].isConnected()) {
            float cv = inputs[OFFSET_CV_INPUT].getVoltage();
            offset += cv;
            offsetCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            offsetCvMod = 0.0f;
        }

        for (int t = 0; t < 3; t++) {
            int ch = std::max(inputs[PITCH_INPUT + t].getChannels(), 1);
            for (int c = 0; c < ch; c++) {
                float pitch = inputs[PITCH_INPUT + t].getVoltage(c);
                pitch = (pitch + offset) * scale;

                // Map to 48 slots (2 octaves × 24 half-semitone slots)
                // pitch * 12 = semitones, * 2 = half-semitone slots
                int slot = std::floor(pitch * 24);
                int twoOctaveBlock = eucDiv(slot, 48);
                slot = eucMod(slot, 48);

                // ranges[] stores note index within 2 octaves (0-23)
                int qNote24 = ranges[slot];  // 0-23 within the 2-octave block
                int qNoteSemitone = twoOctaveBlock * 24 + qNote24;  // absolute semitone

                int noteIdx24 = eucMod(qNote24, 24);
                int noteIdx12 = eucMod(qNote24, 12);
                playing[noteIdx24] = true;

                // Direction detection & microtune
                float mt;
                if (hasDirectional) {
                    float diff = (float)qNoteSemitone - lastNote[t][c];
                    if (diff > 0.5f) ascending[t][c] = true;
                    else if (diff < -0.5f) ascending[t][c] = false;
                    mt = ascending[t][c] ? ascCents[noteIdx12] : descCents[noteIdx12];
                } else {
                    mt = params[MICROTUNE_PARAM + noteIdx24].getValue();
                }
                lastNote[t][c] = (float)qNoteSemitone;

                // Output: 1V/octave = 12 semitones per volt
                outputs[PITCH_OUTPUT + t].setVoltage((float)qNoteSemitone / 12.f + mt / 1200.f, c);
            }
            outputs[PITCH_OUTPUT + t].setChannels(ch);
        }
        std::memcpy(playingNotes, playing, sizeof(playing));
    }

    void updateRanges() {
        bool any = false;
        for (int i = 0; i < 24 && !any; i++) any = enabledNotes[i];

        // 48 slots for 2 octaves (24 semitones × 2 slots each)
        for (int i = 0; i < 48; i++) {
            int closest = 0, minDist = INT_MAX;
            // Search within the 2-octave range plus neighbors
            for (int n = -12; n <= 36; n++) {
                int noteIdx = eucMod(n, 24);
                if (any && !enabledNotes[noteIdx]) continue;
                // i/2 gives semitone position (0-23), n is candidate semitone
                int d = std::abs((i + 1) / 2 - n);
                if (d < minDist) { closest = n; minDist = d; }
                else if (d > minDist) break;
            }
            ranges[i] = closest;
        }
    }

    void applyPreset(int idx) {
        const float* p = getPreset(idx);
        for (int i = 0; i < 24; i++)
            params[MICROTUNE_PARAM + i].setValue(p[i % 12]);
        hasDirectional = false;
    }

    void applyDirectional(int idx) {
        if (idx >= 0 && idx < DIRECTIONAL_COUNT) {
            const float* asc = getAscending(idx);
            const float* desc = getDescending(idx);
            for (int i = 0; i < 12; i++) {
                ascCents[i] = asc[i];
                descCents[i] = desc[i];
            }
            for (int i = 0; i < 24; i++)
                params[MICROTUNE_PARAM + i].setValue(asc[i % 12]);
            hasDirectional = true;
        }
    }

    void applyScale(int idx) {
        if (idx >= 0 && idx < 16) {
            for (int i = 0; i < 24; i++)
                enabledNotes[i] = SCALES[idx][i % 12];
            updateRanges();
        }
    }

    json_t* dataToJson() override {
        json_t* root = json_object();
        json_object_set_new(root, "panelTheme", json_integer(panelTheme));
        json_object_set_new(root, "currentPreset", json_integer(currentPreset));
        json_object_set_new(root, "hasDirectional", json_boolean(hasDirectional));

        json_t* notes = json_array();
        for (int i = 0; i < 24; i++)
            json_array_append_new(notes, json_boolean(enabledNotes[i]));
        json_object_set_new(root, "enabledNotes", notes);

        json_t* asc = json_array();
        json_t* desc = json_array();
        for (int i = 0; i < 12; i++) {
            json_array_append_new(asc, json_real(ascCents[i]));
            json_array_append_new(desc, json_real(descCents[i]));
        }
        json_object_set_new(root, "ascCents", asc);
        json_object_set_new(root, "descCents", desc);
        return root;
    }

    void dataFromJson(json_t* root) override {
        json_t* j;
        if ((j = json_object_get(root, "panelTheme"))) panelTheme = json_integer_value(j);
        if ((j = json_object_get(root, "currentPreset"))) currentPreset = json_integer_value(j);
        if ((j = json_object_get(root, "hasDirectional"))) hasDirectional = json_boolean_value(j);

        if ((j = json_object_get(root, "enabledNotes"))) {
            size_t len = json_array_size(j);
            if (len == 12) {
                // Legacy: 12-note format, expand to 24
                for (int i = 0; i < 12; i++) {
                    bool v = json_boolean_value(json_array_get(j, i));
                    enabledNotes[i] = enabledNotes[i + 12] = v;
                }
            } else {
                for (int i = 0; i < 24; i++)
                    if (json_t* n = json_array_get(j, i)) enabledNotes[i] = json_boolean_value(n);
            }
        }

        if ((j = json_object_get(root, "ascCents")))
            for (int i = 0; i < 12; i++)
                if (json_t* n = json_array_get(j, i)) ascCents[i] = json_real_value(n);

        if ((j = json_object_get(root, "descCents")))
            for (int i = 0; i < 12; i++)
                if (json_t* n = json_array_get(j, i)) descCents[i] = json_real_value(n);

        updateRanges();
    }
};

// Scale presets
const bool Quantizer::SCALES[16][12] = {
    {1,1,1,1,1,1,1,1,1,1,1,1}, // Chromatic
    {1,0,1,0,1,1,0,1,0,1,0,1}, // Major
    {1,0,1,1,0,1,0,1,1,0,1,0}, // Minor
    {1,0,1,0,1,0,0,1,0,1,0,0}, // Penta Major
    {1,0,0,1,0,1,0,1,0,0,1,0}, // Penta Minor
    {1,0,1,1,0,1,0,1,0,1,1,0}, // Dorian
    {1,1,0,1,0,1,0,1,1,0,1,0}, // Phrygian
    {1,0,1,0,1,0,1,1,0,1,0,1}, // Lydian
    {1,0,1,0,1,1,0,1,0,1,1,0}, // Mixolydian
    {1,1,0,1,0,1,1,0,1,0,1,0}, // Locrian
    {1,0,0,0,1,0,0,1,0,0,0,0}, // Major Triad
    {1,0,0,1,0,0,0,1,0,0,0,0}, // Minor Triad
    {1,0,1,1,0,1,0,1,1,0,1,1}, // Blues
    {1,1,0,1,1,0,1,1,1,0,1,1}, // Arabic
    {1,0,1,1,1,0,1,1,0,1,1,0}, // Japanese
    {1,0,1,0,1,1,1,1,0,1,0,1}  // Whole Tone
};

// ============================================================================
// Widget Classes
// ============================================================================

// Note toggle switch (small square) - Yellow/Dark colors
struct NoteToggle : OpaqueWidget {
    Quantizer* module = nullptr;
    int noteIdx = 0;

    NoteToggle() {
        box.size = Vec(8, 8);
    }

    void draw(const DrawArgs& args) override {
        bool enabled = module ? module->enabledNotes[noteIdx] : true;
        bool playing = module ? module->playingNotes[noteIdx] : false;

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 1.5f);

        if (playing)
            nvgFillColor(args.vg, nvgRGB(255, 220, 0));  // Bright yellow when playing
        else if (enabled)
            nvgFillColor(args.vg, nvgRGB(200, 170, 0));  // Yellow when enabled
        else
            nvgFillColor(args.vg, nvgRGB(20, 20, 20));   // Deep black when disabled
        nvgFill(args.vg);

        nvgStrokeColor(args.vg, nvgRGB(60, 60, 60));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
            if (module) {
                module->enabledNotes[noteIdx] ^= true;
                module->updateRanges();
            }
            e.consume(this);
        }
        OpaqueWidget::onButton(e);
    }

    void onDragEnter(const event::DragEnter& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (auto* origin = dynamic_cast<NoteToggle*>(e.origin)) {
                if (module) {
                    module->enabledNotes[noteIdx] = module->enabledNotes[origin->noteIdx];
                    module->updateRanges();
                }
            }
        }
        OpaqueWidget::onDragEnter(e);
    }
};

// Microtune slider bar (horizontal) - Line indicator instead of dot
struct MicrotuneSlider : OpaqueWidget {
    Quantizer* module = nullptr;
    int noteIdx = 0;
    bool isBlackKey = false;
    float dragStartValue = 0.f;

    MicrotuneSlider() {
        box.size = Vec(40, 9);
    }

    void draw(const DrawArgs& args) override {
        bool enabled = module ? module->enabledNotes[noteIdx] : true;
        bool playing = module ? module->playingNotes[noteIdx] : false;
        float value = module ? module->params[Quantizer::MICROTUNE_PARAM + noteIdx].getValue() : 0.f;

        // Background bar
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.f);

        if (!enabled) {
            nvgFillColor(args.vg, nvgRGB(40, 40, 40));  // Dark when disabled
        } else if (isBlackKey) {
            nvgFillColor(args.vg, playing ? nvgRGB(80, 80, 80) : nvgRGB(30, 30, 30));  // Black key
        } else {
            nvgFillColor(args.vg, playing ? nvgRGB(255, 255, 200) : nvgRGB(240, 240, 240));  // White key
        }
        nvgFill(args.vg);

        // Border
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);

        // Value indicator - vertical line only (no dot)
        if (enabled) {
            float centerX = box.size.x / 2.f;
            float lineX = centerX + (value / 50.f) * (box.size.x / 2.f - 2.f);

            // Position line
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, lineX, 1);
            nvgLineTo(args.vg, lineX, box.size.y - 1);
            nvgStrokeColor(args.vg, playing ? nvgRGB(255, 100, 0) : nvgRGB(255, 150, 0));
            nvgStrokeWidth(args.vg, 2.f);
            nvgStroke(args.vg);
        }
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
            dragStartValue = module->params[Quantizer::MICROTUNE_PARAM + noteIdx].getValue();
        }
        OpaqueWidget::onDragStart(e);
    }

    void onDragMove(const event::DragMove& e) override {
        if (module) {
            float delta = e.mouseDelta.x * 0.5f;  // Sensitivity
            float newValue = clamp(dragStartValue + delta, -50.f, 50.f);
            dragStartValue = newValue;
            module->params[Quantizer::MICROTUNE_PARAM + noteIdx].setValue(newValue);
        }
        OpaqueWidget::onDragMove(e);
    }

    void onDoubleClick(const event::DoubleClick& e) override {
        if (module) {
            module->params[Quantizer::MICROTUNE_PARAM + noteIdx].setValue(0.f);
        }
        OpaqueWidget::onDoubleClick(e);
    }
};

// Complete note row (toggle + slider)
struct NoteRow : Widget {
    NoteToggle* toggle = nullptr;
    MicrotuneSlider* slider = nullptr;

    void init(Quantizer* module, int noteIdx, bool isBlackKey, float y, float moduleWidth) {
        float toggleX = 2.f;
        float sliderX = 12.f;  // Both black and white keys left-aligned
        float sliderW = moduleWidth - 14.f;  // Full width for white keys
        if (isBlackKey) {
            sliderW = sliderW * 0.7f;  // Black keys 30% shorter
        }

        toggle = new NoteToggle();
        toggle->module = module;
        toggle->noteIdx = noteIdx;
        toggle->box.pos = Vec(toggleX, y);

        slider = new MicrotuneSlider();
        slider->module = module;
        slider->noteIdx = noteIdx;
        slider->isBlackKey = isBlackKey;
        slider->box.pos = Vec(sliderX, y);
        slider->box.size.x = sliderW;
    }
};

struct QuantizerWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    madzine::widgets::MediumGrayKnob* scaleKnob = nullptr;
    madzine::widgets::MediumGrayKnob* offsetKnob = nullptr;

    QuantizerWidget(Quantizer* module) {
        setModule(module);
        panelThemeHelper.init(this, "4HP");
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        float W = box.size.x;

        // Title with MADZINE brand (standard size)
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(W, 20), "Quanti2er", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(W, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // Knobs area with labels (using MediumGrayKnob like MADDY+)
        addChild(new EnhancedTextLabel(Vec(0, 32), Vec(30, 10), "Amount", 6.f, nvgRGB(255, 255, 255), true));
        scaleKnob = createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(15, 52), module, Quantizer::SCALE_PARAM);
        addParam(scaleKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(15, 76), module, Quantizer::SCALE_CV_INPUT));

        addChild(new EnhancedTextLabel(Vec(30, 32), Vec(30, 10), "Offset", 6.f, nvgRGB(255, 255, 255), true));
        offsetKnob = createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(45, 52), module, Quantizer::OFFSET_PARAM);
        addParam(offsetKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(45, 76), module, Quantizer::OFFSET_CV_INPUT));

        // Note rows (Y: 88-328, 24 rows, ~10px each)
        // Order: B2, A#2, A2, ... C2, B1, A#1, ... C1 (top to bottom)
        // Note indices: 23=B2, 22=A#2, ... 12=C2, 11=B1, ... 0=C1
        const float startY = 88.f;
        const float rowH = 10.f;
        const int noteOrder[24] = {23,22,21,20,19,18,17,16,15,14,13,12, 11,10,9,8,7,6,5,4,3,2,1,0};
        const bool isBlack[12] = {false,true,false,true,false,false,true,false,true,false,true,false}; // C,C#,D,D#,E,F,F#,G,G#,A,A#,B

        for (int row = 0; row < 24; row++) {
            int noteIdx = noteOrder[row];
            bool blackKey = isBlack[noteIdx % 12];
            float y = startY + row * rowH;

            NoteRow noteRow;
            noteRow.init(module, noteIdx, blackKey, y, W);
            addChild(noteRow.toggle);
            addChild(noteRow.slider);

            // Octave separator line after C2 (row 11)
            if (row == 11) {
                auto* sep = new Widget();
                sep->box.pos = Vec(2, y + rowH - 1);
                sep->box.size = Vec(W - 4, 1);
                addChild(sep);
            }
        }

        // I/O area (Y: 330+) - unchanged
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(W, 50)));
        for (int i = 0; i < 3; i++) {
            float y = 342 + i * 16;
            addInput(createInputCentered<PJ301MPort>(Vec(15, y), module, Quantizer::PITCH_INPUT + i));
            addOutput(createOutputCentered<PJ301MPort>(Vec(45, y), module, Quantizer::PITCH_OUTPUT + i));
        }
    }

    void step() override {
        if (auto* m = dynamic_cast<Quantizer*>(module)) {
            panelThemeHelper.step(m);

            // 更新 CV 調變顯示
            auto updateKnob = [&](madzine::widgets::MediumGrayKnob* knob, int inputId, float cvMod) {
                if (knob) {
                    bool connected = m->inputs[inputId].isConnected();
                    knob->setModulationEnabled(connected);
                    if (connected) knob->setModulation(cvMod);
                }
            };

            updateKnob(scaleKnob, Quantizer::SCALE_CV_INPUT, m->scaleCvMod);
            updateKnob(offsetKnob, Quantizer::OFFSET_CV_INPUT, m->offsetCvMod);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        auto* m = getModule<Quantizer>();
        if (!m) return;

        menu->addChild(new MenuSeparator);

        // Scale Presets
        const char* scaleNames[16] = {
            "Chromatic", "Major", "Minor", "Penta Major", "Penta Minor",
            "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
            "Major Triad", "Minor Triad", "Blues", "Arabic", "Japanese", "Whole Tone"
        };
        menu->addChild(createSubmenuItem("Scale Presets", "", [=](Menu* sub) {
            for (int i = 0; i < 16; i++)
                sub->addChild(createMenuItem(scaleNames[i], "", [=]() { m->applyScale(i); }));
        }));

        // Microtune Presets
        menu->addChild(createSubmenuItem("Microtune Presets", "", [=](Menu* sub) {
            const char* names[] = {
                "Equal Temperament", "Just Intonation", "Pythagorean", "Quarter-tone",
                "Maqam Rast (2-Oct)", "Maqam Bayati (2-Oct)", "Maqam Hijaz (2-Oct)", "Maqam Saba (2-Oct)", "Maqam Nahawand (2-Oct)", "Maqam Kurd (2-Oct)",
                "Makam Rast (2-Oct)", "Makam Ussak (2-Oct)", "Makam Hicaz (2-Oct)", "Makam Segah (2-Oct)",
                "Dastgah Shur (2-Oct)", "Dastgah Segah (2-Oct)",
                "Shruti (2-Oct)", "Raga Bhairav (2-Oct)", "Raga Yaman (2-Oct)", "Raga Bhairavi (2-Oct)",
                "Gagaku (2-Oct)", "In Scale (2-Oct)", "Yo Scale (2-Oct)", "Ryukyu (2-Oct)",
                "Slendro", "Pelog", "Thai 7-TET",
                "Chinese Pentatonic"
            };
            int separators[] = {4, 10, 14, 16, 20, 24, 27};
            int sepIdx = 0;
            for (int i = 0; i < 28; i++) {
                sub->addChild(createMenuItem(names[i], "", [=]() { m->applyPreset(i); m->currentPreset = i; }));
                if (sepIdx < 7 && i == separators[sepIdx] - 1) {
                    sub->addChild(new MenuSeparator);
                    sepIdx++;
                }
            }
            sub->addChild(new MenuSeparator);
            sub->addChild(createMenuLabel("Directional (Asc/Desc)"));
            const char* dirNames[] = {"Turkish Rast ↑↓", "Arabic Hijaz ↑↓", "Miyako-bushi ↑↓"};
            for (int i = 0; i < 3; i++)
                sub->addChild(createMenuItem(dirNames[i], "", [=]() { m->applyDirectional(i); m->currentPreset = 100 + i; }));
        }));

        addPanelThemeMenu(menu, m);
    }
};

Model* modelQuantizer = createModel<Quantizer, QuantizerWidget>("Quantizer");
