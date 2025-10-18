#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
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

// StandardBlackKnob 現在從 widgets/Knobs.hpp 引入
// Microtune knob for 20x20 size
// MicrotuneKnob 現在從 widgets/Knobs.hpp 引入
struct Quantizer : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamIds {
        SCALE_PARAM,
        OFFSET_PARAM,
        // Microtune parameters for 12 notes
        C_MICROTUNE_PARAM,
        CS_MICROTUNE_PARAM,
        D_MICROTUNE_PARAM,
        DS_MICROTUNE_PARAM,
        E_MICROTUNE_PARAM,
        F_MICROTUNE_PARAM,
        FS_MICROTUNE_PARAM,
        G_MICROTUNE_PARAM,
        GS_MICROTUNE_PARAM,
        A_MICROTUNE_PARAM,
        AS_MICROTUNE_PARAM,
        B_MICROTUNE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        PITCH_INPUT_2,
        PITCH_INPUT_3,
        OFFSET_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        PITCH_OUTPUT,
        PITCH_OUTPUT_2,
        PITCH_OUTPUT_3,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool enabledNotes[12];
    // Intervals [i / 24, (i+1) / 24) V mapping to the closest enabled note
    int ranges[24];
    bool playingNotes[12];
    
    // Microtune presets (in cents)
    static const float EQUAL_TEMPERAMENT[12];
    static const float JUST_INTONATION[12];
    static const float PYTHAGOREAN[12];
    static const float ARABIC_MAQAM[12];
    static const float INDIAN_RAGA[12];
    static const float GAMELAN_PELOG[12];
    static const float JAPANESE_GAGAKU[12];
    static const float TURKISH_MAKAM[12];
    static const float PERSIAN_DASTGAH[12];
    static const float QUARTER_TONE[12];
    
    int currentPreset = 0;

    Quantizer() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SCALE_PARAM, 0.0f, 2.0f, 1.0f, "Scale", "%", 0.f, 100.f);
        configParam(OFFSET_PARAM, -1.f, 1.f, 0.f, "Pre-offset", " semitones", 0.f, 12.f);
        
        // Configure microtune parameters for each note
        std::string noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        for (int i = 0; i < 12; i++) {
            configParam(C_MICROTUNE_PARAM + i, -50.f, 50.f, 0.f, noteNames[i] + " Microtune", " cents");
        }
        
        configInput(PITCH_INPUT, "CV1");
        configInput(PITCH_INPUT_2, "CV2");
        configInput(PITCH_INPUT_3, "CV3");
        configInput(OFFSET_CV_INPUT, "Offset CV");
        configOutput(PITCH_OUTPUT, "Pitch");
        configOutput(PITCH_OUTPUT_2, "Pitch 2");
        configOutput(PITCH_OUTPUT_3, "Pitch 3");
        configBypass(PITCH_INPUT, PITCH_OUTPUT);

        onReset();
    }

    void onReset() override {
        for (int i = 0; i < 12; i++) {
            enabledNotes[i] = true;
        }
        updateRanges();
    }

    void onRandomize() override {
        for (int i = 0; i < 12; i++) {
            enabledNotes[i] = (random::uniform() < 0.5f);
        }
        updateRanges();
    }

    void process(const ProcessArgs& args) override {
        bool playingNotes[12] = {};
        float scaleParam = params[SCALE_PARAM].getValue();
        float offsetParam = params[OFFSET_PARAM].getValue();
        
        // Add CV offset
        if (inputs[OFFSET_CV_INPUT].isConnected()) {
            offsetParam += inputs[OFFSET_CV_INPUT].getVoltage();
        }

        // Process all three tracks
        for (int track = 0; track < 3; track++) {
            int inputId = PITCH_INPUT + track;
            int outputId = PITCH_OUTPUT + track;
            
            int channels = std::max(inputs[inputId].getChannels(), 1);
            
            for (int c = 0; c < channels; c++) {
                float pitch = inputs[inputId].getVoltage(c);

                // Apply offset first (before quantization)
                pitch += offsetParam;

                // Apply scale
                pitch *= scaleParam;

                // Quantize to enabled notes
                int range = std::floor(pitch * 24);
                int octave = eucDiv(range, 24);
                range -= octave * 24;
                int quantizedNote = ranges[range] + octave * 12;
                int noteInOctave = eucMod(quantizedNote, 12);
                playingNotes[noteInOctave] = true;

                // Apply microtune to the quantized note
                float microtuneOffset = params[C_MICROTUNE_PARAM + noteInOctave].getValue() / 1200.f; // Convert cents to volts (1200 cents = 1V)
                pitch = float(quantizedNote) / 12.f + microtuneOffset;

                outputs[outputId].setVoltage(pitch, c);
            }
            outputs[outputId].setChannels(channels);
        }
        std::memcpy(this->playingNotes, playingNotes, sizeof(playingNotes));
    }
    
    void updateRanges() {
        // Check if no notes are enabled
        bool anyEnabled = false;
        for (int note = 0; note < 12; note++) {
            if (enabledNotes[note]) {
                anyEnabled = true;
                break;
            }
        }
        // Find closest notes for each range
        for (int i = 0; i < 24; i++) {
            int closestNote = 0;
            int closestDist = INT_MAX;
            for (int note = -12; note <= 24; note++) {
                int dist = std::abs((i + 1) / 2 - note);
                // Ignore enabled state if no notes are enabled
                if (anyEnabled && !enabledNotes[eucMod(note, 12)]) {
                    continue;
                }
                if (dist < closestDist) {
                    closestNote = note;
                    closestDist = dist;
                }
                else {
                    // If dist increases, we won't find a better one.
                    break;
                }
            }
            ranges[i] = closestNote;
        }
    }

    void applyMicrotunePreset(int presetIndex) {
        const float* preset = nullptr;
        
        switch (presetIndex) {
            case 0: preset = EQUAL_TEMPERAMENT; break;
            case 1: preset = JUST_INTONATION; break;
            case 2: preset = PYTHAGOREAN; break;
            case 3: preset = ARABIC_MAQAM; break;
            case 4: preset = INDIAN_RAGA; break;
            case 5: preset = GAMELAN_PELOG; break;
            case 6: preset = JAPANESE_GAGAKU; break;
            case 7: preset = TURKISH_MAKAM; break;
            case 8: preset = PERSIAN_DASTGAH; break;
            case 9: preset = QUARTER_TONE; break;
        }
        
        if (preset) {
            for (int i = 0; i < 12; i++) {
                params[C_MICROTUNE_PARAM + i].setValue(preset[i]);
            }
        }
    }
    
    void applyScalePreset(int scaleIndex) {
        // Scale presets: true = note enabled, false = note disabled
        // Note order: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
        static const bool SCALE_PRESETS[16][12] = {
            {true, true, true, true, true, true, true, true, true, true, true, true}, // Chromatic
            {true, false, true, false, true, true, false, true, false, true, false, true}, // Major (Ionian)
            {true, false, true, true, false, true, false, true, true, false, true, false}, // Minor (Aeolian)
            {true, false, true, false, true, false, false, true, false, true, false, true}, // Pentatonic Major
            {true, false, false, true, false, true, false, true, false, false, true, false}, // Pentatonic Minor
            {true, false, true, true, false, true, false, true, false, true, true, false}, // Dorian
            {true, true, false, true, false, true, false, true, true, false, true, false}, // Phrygian
            {true, false, true, false, true, false, true, true, false, true, false, true}, // Lydian
            {true, false, true, false, true, true, false, true, false, true, true, false}, // Mixolydian
            {true, true, false, true, false, true, true, false, true, false, true, false}, // Locrian
            {true, false, false, false, true, false, false, true, false, false, false, false}, // Major Triad
            {true, false, false, true, false, false, false, true, false, false, false, false}, // Minor Triad
            {true, false, true, true, false, true, false, true, true, false, true, true}, // Blues
            {true, true, false, true, true, false, true, true, true, false, true, true}, // Arabic
            {true, false, true, true, true, false, true, true, false, true, true, false}, // Japanese
            {true, false, true, false, true, true, true, true, false, true, false, true}  // Whole Tone
        };
        
        if (scaleIndex >= 0 && scaleIndex < 16) {
            for (int i = 0; i < 12; i++) {
                enabledNotes[i] = SCALE_PRESETS[scaleIndex][i];
            }
            updateRanges();
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

        json_t* enabledNotesJ = json_array();
        for (int i = 0; i < 12; i++) {
            json_array_insert_new(enabledNotesJ, i, json_boolean(enabledNotes[i]));
        }
        json_object_set_new(rootJ, "enabledNotes", enabledNotesJ);
        json_object_set_new(rootJ, "currentPreset", json_integer(currentPreset));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }

        json_t* enabledNotesJ = json_object_get(rootJ, "enabledNotes");
        if (enabledNotesJ) {
            for (int i = 0; i < 12; i++) {
                json_t* enabledNoteJ = json_array_get(enabledNotesJ, i);
                if (enabledNoteJ)
                    enabledNotes[i] = json_boolean_value(enabledNoteJ);
            }
        }
        
        json_t* presetJ = json_object_get(rootJ, "currentPreset");
        if (presetJ) {
            currentPreset = json_integer_value(presetJ);
        }
        
        updateRanges();
    }
};

// Static member definitions
const float Quantizer::EQUAL_TEMPERAMENT[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const float Quantizer::JUST_INTONATION[12] = {0, -29.3, -3.9, 15.6, -13.7, -2.0, -31.3, 2.0, -27.4, -15.6, 17.6, -11.7};
const float Quantizer::PYTHAGOREAN[12] = {0, -90.2, 3.9, -5.9, 7.8, -2.0, -92.2, 2.0, -88.3, 5.9, -3.9, 9.8};
const float Quantizer::ARABIC_MAQAM[12] = {0, 0, -50, 0, 0, 0, 0, 0, -50, 0, -50, 0};
const float Quantizer::INDIAN_RAGA[12] = {0, 22, -28, 22, -28, 0, 22, 0, 22, -28, 22, -28};
const float Quantizer::GAMELAN_PELOG[12] = {0, 0, 40, 0, -20, 20, 0, 0, 40, 0, -20, 20};
const float Quantizer::JAPANESE_GAGAKU[12] = {0, 0, -14, 0, 16, 0, 0, 0, -14, 16, 0, 16};
const float Quantizer::TURKISH_MAKAM[12] = {0, 24, -24, 24, 0, 24, -24, 0, 24, -24, 24, 0};
const float Quantizer::PERSIAN_DASTGAH[12] = {0, 0, -34, 0, 16, 0, 0, 0, -34, 16, 0, 16};
const float Quantizer::QUARTER_TONE[12] = {0, 50, 0, 50, 0, 0, 50, 0, 50, 0, 50, 0};

struct QuantizerButton : OpaqueWidget {
    int note;
    Quantizer* module;

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1)
            return;

        Rect r = box.zeroPos();
        const float margin = mm2px(1.0);
        Rect rMargin = r.grow(Vec(margin, margin));

        nvgBeginPath(args.vg);
        nvgRect(args.vg, RECT_ARGS(rMargin));
        nvgFillColor(args.vg, nvgRGB(0x12, 0x12, 0x12));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRect(args.vg, RECT_ARGS(r));
        if (module ? module->playingNotes[note] : (note == 0)) {
            nvgFillColor(args.vg, SCHEME_YELLOW);
        }
        else if (module ? module->enabledNotes[note] : true) {
            nvgFillColor(args.vg, nvgRGB(0x7f, 0x6b, 0x0a));
        }
        else {
            nvgFillColor(args.vg, nvgRGB(0x40, 0x40, 0x40));
        }
        nvgFill(args.vg);
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            module->enabledNotes[note] ^= true;
            module->updateRanges();
        }
        OpaqueWidget::onDragStart(e);
    }

    void onDragEnter(const event::DragEnter& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            QuantizerButton* origin = dynamic_cast<QuantizerButton*>(e.origin);
            if (origin) {
                module->enabledNotes[note] = module->enabledNotes[origin->note];;
                module->updateRanges();
            }
        }
        OpaqueWidget::onDragEnter(e);
    }
};

struct QuantizerDisplay : LedDisplay {
    void setModule(Quantizer* module) {
        // Use exact VCV Rack original positions and sizes but scaled to 80%
        std::vector<Vec> noteAbsPositions = {
            mm2px(Vec(2.242 * 0.8, 60.54 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 58.416 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 52.043 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 49.919 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 45.67 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 39.298 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 37.173 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 30.801 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 28.677 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 22.304 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 20.18 * 0.8)),
            mm2px(Vec(2.242 * 0.8, 15.931 * 0.8)),
        };
        std::vector<Vec> noteSizes = {
            mm2px(Vec(10.734 * 0.8, 5.644 * 0.8)),
            mm2px(Vec(8.231 * 0.8, 3.52 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 7.769 * 0.8)),
            mm2px(Vec(8.231 * 0.8, 3.52 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 5.644 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 5.644 * 0.8)),
            mm2px(Vec(8.231 * 0.8, 3.52 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 7.769 * 0.8)),
            mm2px(Vec(8.231 * 0.8, 3.52 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 7.768 * 0.8)),
            mm2px(Vec(8.231 * 0.8, 3.52 * 0.8)),
            mm2px(Vec(10.734 * 0.8, 5.644 * 0.8)),
        };

        // White notes
        static const std::vector<int> whiteNotes = {0, 2, 4, 5, 7, 9, 11};
        for (int note : whiteNotes) {
            QuantizerButton* quantizerButton = new QuantizerButton();
            quantizerButton->box.pos = noteAbsPositions[note] - box.pos;
            quantizerButton->box.size = noteSizes[note];
            quantizerButton->module = module;
            quantizerButton->note = note;
            addChild(quantizerButton);
        }
        // Black notes
        static const std::vector<int> blackNotes = {1, 3, 6, 8, 10};
        for (int note : blackNotes) {
            QuantizerButton* quantizerButton = new QuantizerButton();
            quantizerButton->box.pos = noteAbsPositions[note] - box.pos;
            quantizerButton->box.size = noteSizes[note];
            quantizerButton->module = module;
            quantizerButton->note = note;
            addChild(quantizerButton);
        }
    }
};

struct QuantizerWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    QuantizerWidget(Quantizer* module) {
        setModule(module);
        panelThemeHelper.init(this, "SwingLFO");

        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Title
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Quantizer", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // Scale knob - added above offset
        addChild(new EnhancedTextLabel(Vec(31, 30), Vec(30, 15), "SCALE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(46, 55), module, Quantizer::SCALE_PARAM));

        // Offset knob - moved down to make room for scale
        addChild(new EnhancedTextLabel(Vec(31, 75), Vec(30, 15), "OFFSET", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(46, 100), module, Quantizer::OFFSET_PARAM));
        
        // CV IN label and input - moved down accordingly
        addChild(new EnhancedTextLabel(Vec(31, 115), Vec(30, 15), "CV IN", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 140), module, Quantizer::OFFSET_CV_INPUT));

        // Quantizer piano display - moved up by 1mm
        QuantizerDisplay* quantizerDisplay = createWidget<QuantizerDisplay>(mm2px(Vec(1.0, 12.0)));
        quantizerDisplay->box.size = mm2px(Vec(15.24 * 0.66, 55.88 * 0.75));
        quantizerDisplay->setModule(module);
        addChild(quantizerDisplay);

        // 12 microtune knobs with correct black/white key mapping
        std::string noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        
        // Map positions to correct notes with C at bottom
        int leftPositions[5] = {1, 3, 6, 8, 10}; // C#, D#, F#, G#, A# (black keys)
        int rightPositions[7] = {0, 2, 4, 5, 7, 9, 11}; // C, D, E, F, G, A, B (white keys)
        
        Vec leftCoords[5] = {
            Vec(15, 310),  // C# (top left) - moved up 10px
            Vec(15, 285),  // D# - moved up 10px
            Vec(15, 235),  // F# - moved up 10px
            Vec(15, 210),  // G# - moved up 10px
            Vec(15, 185)   // A# (bottom left) - moved up 10px
        };
        
        Vec rightCoords[7] = {
            Vec(45, 320),  // C (bottom right) - moved up 10px
            Vec(45, 295),  // D - moved up 10px
            Vec(45, 270),  // E - moved up 10px
            Vec(45, 245),  // F - moved up 10px
            Vec(45, 220),  // G - moved up 10px
            Vec(45, 195),  // A - moved up 10px
            Vec(45, 170)   // B (top right) - moved up 10px
        };

        // Place black keys (left side) - removed labels
        for (int i = 0; i < 5; i++) {
            int noteIndex = leftPositions[i];
            addParam(createParamCentered<MicrotuneKnob>(leftCoords[i], module, Quantizer::C_MICROTUNE_PARAM + noteIndex));
        }
        
        // Place white keys (right side) - removed labels
        for (int i = 0; i < 7; i++) {
            int noteIndex = rightPositions[i];
            addParam(createParamCentered<MicrotuneKnob>(rightCoords[i], module, Quantizer::C_MICROTUNE_PARAM + noteIndex));
        }

        // White background for inputs/outputs
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));

        // Track 1 - positioned at Y=340
        addInput(createInputCentered<PJ301MPort>(Vec(15, 340), module, Quantizer::PITCH_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 340), module, Quantizer::PITCH_OUTPUT));
        
        // Track 2 - positioned at Y=358
        addInput(createInputCentered<PJ301MPort>(Vec(15, 358), module, Quantizer::PITCH_INPUT_2));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 358), module, Quantizer::PITCH_OUTPUT_2));
        
        // Track 3 - positioned at Y=374
        addInput(createInputCentered<PJ301MPort>(Vec(15, 374), module, Quantizer::PITCH_INPUT_3));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 374), module, Quantizer::PITCH_OUTPUT_3));
    }

    void step() override {
        Quantizer* module = dynamic_cast<Quantizer*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Quantizer* module = getModule<Quantizer>();
        if (!module) return;

        menu->addChild(new MenuSeparator);

        // Scale Presets submenu
        menu->addChild(createSubmenuItem("Scale Presets", "", [=](Menu* menu) {
            std::string scaleNames[16] = {
                "Chromatic",
                "Major (Ionian)",
                "Minor (Aeolian)",
                "Pentatonic Major",
                "Pentatonic Minor",
                "Dorian",
                "Phrygian",
                "Lydian",
                "Mixolydian",
                "Locrian",
                "Major Triad",
                "Minor Triad",
                "Blues",
                "Arabic",
                "Japanese",
                "Whole Tone"
            };

            for (int i = 0; i < 16; i++) {
                menu->addChild(createMenuItem(scaleNames[i], "", [=]() {
                    module->applyScalePreset(i);
                }));
            }
        }));

        // Microtune Presets submenu
        menu->addChild(createSubmenuItem("Microtune Presets", "", [=](Menu* menu) {
            std::string presetNames[10] = {
                "Equal Temperament",
                "Just Intonation",
                "Pythagorean",
                "Arabic Maqam",
                "Indian Raga",
                "Gamelan Pelog",
                "Japanese Gagaku",
                "Turkish Makam",
                "Persian Dastgah",
                "Quarter-tone"
            };

            for (int i = 0; i < 10; i++) {
                menu->addChild(createMenuItem(presetNames[i], "", [=]() {
                    module->applyMicrotunePreset(i);
                    module->currentPreset = i;
                }));
            }
        }));

        addPanelThemeMenu(menu, module);
    }
};

Model* modelQuantizer = createModel<Quantizer, QuantizerWidget>("Quantizer");