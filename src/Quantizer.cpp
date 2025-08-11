#include "plugin.hpp"

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

struct StandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    StandardBlackKnob() {
        box.size = Vec(26, 26);
    }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float normalizedValue = pq->getScaledValue();
        float angle = rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
        return angle;
    }
    
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 4);
        nvgFillColor(args.vg, nvgRGB(50, 50, 50));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 8;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
    }
    
    void onDragStart(const event::DragStart& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
        isDragging = true;
        ParamWidget::onDragStart(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        if (!isDragging) return;
        
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        float sensitivity = 0.002f;
        float newValue = pq->getValue() + e.mouseDelta.y * -sensitivity * (pq->getMaxValue() - pq->getMinValue());
        pq->setValue(newValue);
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        isDragging = false;
        ParamWidget::onDragEnd(e);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        pq->reset();
        e.consume(this);
    }
};

// Microtune knob for 20x20 size
struct MicrotuneKnob : ParamWidget {
    bool isDragging = false;
    
    MicrotuneKnob() {
        box.size = Vec(20, 20);
    }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float normalizedValue = pq->getScaledValue();
        float angle = rescale(normalizedValue, 0.0f, 1.0f, -0.75f * M_PI, 0.75f * M_PI);
        return angle;
    }
    
    void draw(const DrawArgs& args) override {
        float radius = box.size.x / 2.0f;
        float angle = getDisplayAngle();
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgFillColor(args.vg, nvgRGB(30, 30, 30));
        nvgFill(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 1);
        nvgStrokeWidth(args.vg, 0.8f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius - 3);
        nvgFillColor(args.vg, nvgRGB(50, 50, 50));
        nvgFill(args.vg);
        
        float indicatorLength = radius - 5;
        float lineX = radius + indicatorLength * std::sin(angle);
        float lineY = radius - indicatorLength * std::cos(angle);
        
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, radius, radius);
        nvgLineTo(args.vg, lineX, lineY);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStroke(args.vg);
    }
    
    void onDragStart(const event::DragStart& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
        isDragging = true;
        ParamWidget::onDragStart(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        if (!isDragging) return;
        
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        float sensitivity = 0.002f;
        float newValue = pq->getValue() + e.mouseDelta.y * -sensitivity * (pq->getMaxValue() - pq->getMinValue());
        pq->setValue(newValue);
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        isDragging = false;
        ParamWidget::onDragEnd(e);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        pq->reset();
        e.consume(this);
    }
};

struct Quantizer : Module {
    enum ParamIds {
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
        OFFSET_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        PITCH_OUTPUT,
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
        configParam(OFFSET_PARAM, -1.f, 1.f, 0.f, "Pre-offset", " semitones", 0.f, 12.f);
        
        // Configure microtune parameters for each note
        std::string noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        for (int i = 0; i < 12; i++) {
            configParam(C_MICROTUNE_PARAM + i, -50.f, 50.f, 0.f, noteNames[i] + " Microtune", " cents");
        }
        
        configInput(PITCH_INPUT, "1V/octave pitch");
        configInput(OFFSET_CV_INPUT, "Offset CV");
        configOutput(PITCH_OUTPUT, "Pitch");
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
        int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);
        float offsetParam = params[OFFSET_PARAM].getValue();
        
        // Add CV offset
        if (inputs[OFFSET_CV_INPUT].isConnected()) {
            offsetParam += inputs[OFFSET_CV_INPUT].getVoltage();
        }

        for (int c = 0; c < channels; c++) {
            float pitch = inputs[PITCH_INPUT].getVoltage(c);
            pitch += offsetParam;
            
            // Apply microtune
            pitch = applyMicrotune(pitch);
            
            int range = std::floor(pitch * 24);
            int octave = eucDiv(range, 24);
            range -= octave * 24;
            int note = ranges[range] + octave * 12;
            playingNotes[eucMod(note, 12)] = true;
            pitch = float(note) / 12;
            outputs[PITCH_OUTPUT].setVoltage(pitch, c);
        }
        outputs[PITCH_OUTPUT].setChannels(channels);
        std::memcpy(this->playingNotes, playingNotes, sizeof(playingNotes));
    }
    
    float applyMicrotune(float pitch) {
        // Convert CV to semitones
        float semitones = pitch * 12.f;
        
        // Get the note within an octave (0-11)
        int note = ((int)std::round(semitones) % 12 + 12) % 12;
        int octave = (int)std::floor(semitones / 12.f);
        
        // Apply microtune offset
        float microtuneOffset = params[C_MICROTUNE_PARAM + note].getValue() / 100.f; // Convert cents to semitones
        
        // Calculate final CV
        float finalSemitones = octave * 12.f + note + microtuneOffset;
        return finalSemitones / 12.f;
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

    void rotateNotes(int delta) {
        delta = eucMod(-delta, 12);
        std::rotate(&enabledNotes[0], &enabledNotes[delta], &enabledNotes[12]);
        updateRanges();
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

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* enabledNotesJ = json_array();
        for (int i = 0; i < 12; i++) {
            json_array_insert_new(enabledNotesJ, i, json_boolean(enabledNotes[i]));
        }
        json_object_set_new(rootJ, "enabledNotes", enabledNotesJ);
        json_object_set_new(rootJ, "currentPreset", json_integer(currentPreset));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
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
    QuantizerWidget(Quantizer* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/SwingLFO.svg")));

        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Title
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Quantizer", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // Offset knob - moved to right by 10px
        addChild(new EnhancedTextLabel(Vec(31, 60), Vec(30, 15), "OFFSET", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(46, 85), module, Quantizer::OFFSET_PARAM));
        
        // CV IN label and input - 25 pixels below the knob
        addChild(new EnhancedTextLabel(Vec(31, 100), Vec(30, 15), "CV IN", 6.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 125), module, Quantizer::OFFSET_CV_INPUT));

        // Quantizer piano display - moved right and scaled to match internal content
        QuantizerDisplay* quantizerDisplay = createWidget<QuantizerDisplay>(mm2px(Vec(1.0, 13.039)));
        quantizerDisplay->box.size = mm2px(Vec(15.24 * 0.66, 55.88 * 0.72));
        quantizerDisplay->setModule(module);
        addChild(quantizerDisplay);

        // 12 microtune knobs with correct black/white key mapping
        std::string noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        bool isBlackKey[12] = {false, true, false, true, false, false, true, false, true, false, true, false};
        
        // Map positions to correct notes with C at bottom
        int leftPositions[5] = {1, 3, 6, 8, 10}; // C#, D#, F#, G#, A# (black keys)
        int rightPositions[7] = {0, 2, 4, 5, 7, 9, 11}; // C, D, E, F, G, A, B (white keys)
        
        Vec leftCoords[5] = {
            Vec(15, 310),  // C# (top left) - moved down 20px
            Vec(15, 285),  // D# - moved down 20px
            Vec(15, 235),  // F# - moved down 20px
            Vec(15, 210),  // G# - moved down 20px
            Vec(15, 185)   // A# (bottom left) - moved down 20px
        };
        
        Vec rightCoords[7] = {
            Vec(45, 320),  // C (bottom right) - moved down 20px
            Vec(45, 295),  // D - moved down 20px
            Vec(45, 270),  // E - moved down 20px
            Vec(45, 245),  // F - moved down 20px
            Vec(45, 220),  // G - moved down 20px
            Vec(45, 195),  // A - moved down 20px
            Vec(45, 170)   // B (top right) - moved down 20px
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

        // Input and output - moved down additional 10px
        addChild(new EnhancedTextLabel(Vec(5, 340), Vec(20, 15), "IN", 6.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 365), module, Quantizer::PITCH_INPUT));

        addChild(new EnhancedTextLabel(Vec(35, 340), Vec(20, 15), "OUT", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 365), module, Quantizer::PITCH_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        Quantizer* module = getModule<Quantizer>();
        if (!module) return;

        menu->addChild(new MenuSeparator);

        menu->addChild(createMenuItem("Shift notes up", "", [=]() {
            module->rotateNotes(1);
        }));
        menu->addChild(createMenuItem("Shift notes down", "", [=]() {
            module->rotateNotes(-1);
        }));
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Microtune Presets"));
        
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
    }
};

Model* modelQuantizer = createModel<Quantizer, QuantizerWidget>("Quantizer");