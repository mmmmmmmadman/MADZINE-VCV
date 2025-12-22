#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <string>
#include <sstream>

// Text label widget
struct SongModeLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    SongModeLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f,
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
struct SongMode;

// Text field for sequence input - custom drawing for compact height
struct SequenceTextField : LedDisplayTextField {
    SongMode* module = nullptr;

    SequenceTextField() {
        fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
        color = nvgRGB(255, 200, 0);
        bgColor = nvgRGBA(0, 0, 0, 200);
    }

    void onChange(const ChangeEvent& e) override;

    void draw(const DrawArgs& args) override {
        // Draw background only - don't call parent draw
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.0f);
        nvgFillColor(args.vg, bgColor);
        nvgFill(args.vg);

        // Draw border
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            // Draw text in layer 1
            if (!text.empty()) {
                std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
                if (font) {
                    nvgFontFaceId(args.vg, font->handle);
                    nvgFontSize(args.vg, 10.f);
                    nvgFillColor(args.vg, color);
                    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                    nvgText(args.vg, 4, box.size.y / 2, text.c_str(), NULL);
                }
            }
            // Draw cursor if focused
            if (APP->event->getSelectedWidget() == this) {
                std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
                if (font) {
                    nvgFontFaceId(args.vg, font->handle);
                    nvgFontSize(args.vg, 10.f);
                    float cursorX = 4;
                    if (cursor > 0 && cursor <= (int)text.length()) {
                        float bounds[4];
                        nvgTextBounds(args.vg, 4, box.size.y / 2, text.c_str(), text.c_str() + cursor, bounds);
                        cursorX = bounds[2];
                    }
                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, cursorX, 2, 1, box.size.y - 4);
                    nvgFillColor(args.vg, color);
                    nvgFill(args.vg);
                }
            }
        }
        // Don't call parent's drawLayer to prevent double text rendering
    }
};

struct SongMode : Module {
    int panelTheme = -1; // -1 = Auto (follow VCV)

    enum ParamId {
        LENGTH_1_PARAM,
        LENGTH_2_PARAM,
        LENGTH_3_PARAM,
        LENGTH_4_PARAM,
        LENGTH_5_PARAM,
        LENGTH_6_PARAM,
        LENGTH_7_PARAM,
        LENGTH_8_PARAM,
        LEARN_1_PARAM,
        LEARN_2_PARAM,
        LEARN_3_PARAM,
        LEARN_4_PARAM,
        LEARN_5_PARAM,
        LEARN_6_PARAM,
        LEARN_7_PARAM,
        LEARN_8_PARAM,
        FADE_CLOCK_PARAM,
        FADE_TIME_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        IN_1_INPUT,
        IN_2_INPUT,
        IN_3_INPUT,
        IN_4_INPUT,
        IN_5_INPUT,
        IN_6_INPUT,
        IN_7_INPUT,
        IN_8_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        OUT_OUTPUT,
        TRIG_1_OUTPUT,
        TRIG_2_OUTPUT,
        TRIG_3_OUTPUT,
        TRIG_4_OUTPUT,
        TRIG_5_OUTPUT,
        TRIG_6_OUTPUT,
        TRIG_7_OUTPUT,
        TRIG_8_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        LEARN_1_LIGHT,
        LEARN_2_LIGHT,
        LEARN_3_LIGHT,
        LEARN_4_LIGHT,
        LEARN_5_LIGHT,
        LEARN_6_LIGHT,
        LEARN_7_LIGHT,
        LEARN_8_LIGHT,
        ACTIVE_1_LIGHT,
        ACTIVE_2_LIGHT,
        ACTIVE_3_LIGHT,
        ACTIVE_4_LIGHT,
        ACTIVE_5_LIGHT,
        ACTIVE_6_LIGHT,
        ACTIVE_7_LIGHT,
        ACTIVE_8_LIGHT,
        LIGHTS_LEN
    };

    // Sequence data
    std::string sequenceText = "12345678";
    std::vector<int> sequence;  // Parsed sequence (0-7 indices)

    // Playback state
    int currentSequenceIndex = 0;  // Current position in sequence
    int currentClockCount = 0;     // Clock count within current segment
    int activeInput = 0;           // Current active input (0-7)

    // Learn mode
    bool learning[8] = {false};
    int learnClockCount[8] = {0};

    // Triggers
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger learnTriggers[8];
    dsp::PulseGenerator trigPulses[8];

    // Crossfade state
    bool fading = false;
    int previousInput = 0;
    float fadeProgress = 0.f;  // 0.0 = old input, 1.0 = new input
    float fadeDuration = 0.f;  // in seconds
    float fadeElapsed = 0.f;   // elapsed time in fade

    SongMode() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure LENGTH knobs (1-64)
        for (int i = 0; i < 8; i++) {
            configParam(LENGTH_1_PARAM + i, 1.f, 64.f, 4.f, string::f("Length %d", i + 1), " clocks");
            getParamQuantity(LENGTH_1_PARAM + i)->snapEnabled = true;
        }

        // Configure LEARN buttons
        for (int i = 0; i < 8; i++) {
            configButton(LEARN_1_PARAM + i, string::f("Learn %d", i + 1));
        }

        // Configure inputs
        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        for (int i = 0; i < 8; i++) {
            configInput(IN_1_INPUT + i, string::f("Input %d", i + 1));
        }

        // Configure outputs
        configOutput(OUT_OUTPUT, "Main");
        for (int i = 0; i < 8; i++) {
            configOutput(TRIG_1_OUTPUT + i, string::f("Trigger %d", i + 1));
        }

        // Configure fade parameters
        configParam(FADE_CLOCK_PARAM, 0.f, 16.f, 0.f, "Fade Clock", " clocks");
        getParamQuantity(FADE_CLOCK_PARAM)->snapEnabled = true;
        configParam(FADE_TIME_PARAM, 0.f, 1000.f, 100.f, "Fade Time", " ms");

        // Parse default sequence
        parseSequence();
    }

    void parseSequence() {
        sequence.clear();

        // Parse character by character - no separator needed
        // Supports: "12345678", "1 2 3", "1,2,3", "1-4" (range)
        size_t i = 0;
        while (i < sequenceText.length()) {
            char c = sequenceText[i];

            // Skip separators (space, comma, tab)
            if (c == ' ' || c == ',' || c == '\t') {
                i++;
                continue;
            }

            // Check for digit
            if (c >= '1' && c <= '8') {
                int num = c - '0';

                // Check if next char is '-' for range
                if (i + 2 < sequenceText.length() && sequenceText[i + 1] == '-') {
                    char endChar = sequenceText[i + 2];
                    if (endChar >= '1' && endChar <= '8') {
                        int endNum = endChar - '0';
                        for (int j = num; j <= endNum; j++) {
                            sequence.push_back(j - 1);
                        }
                        i += 3;
                        continue;
                    }
                }

                // Single digit
                sequence.push_back(num - 1);
            }
            i++;
        }

        // Default to all 8 if empty
        if (sequence.empty()) {
            for (int i = 0; i < 8; i++) {
                sequence.push_back(i);
            }
        }
    }

    void onReset() override {
        currentSequenceIndex = 0;
        currentClockCount = 0;
        activeInput = sequence.empty() ? 0 : sequence[0];
        for (int i = 0; i < 8; i++) {
            learning[i] = false;
            learnClockCount[i] = 0;
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "sequenceText", json_string(sequenceText.c_str()));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
        json_t* seqJ = json_object_get(rootJ, "sequenceText");
        if (seqJ) {
            sequenceText = json_string_value(seqJ);
            parseSequence();
        }
    }

    void process(const ProcessArgs& args) override {
        // Process reset
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f)) {
            onReset();
        }

        // Process learn buttons
        for (int i = 0; i < 8; i++) {
            if (learnTriggers[i].process(params[LEARN_1_PARAM + i].getValue())) {
                if (learning[i]) {
                    // End learning - set the length
                    if (learnClockCount[i] > 0) {
                        params[LENGTH_1_PARAM + i].setValue(clamp((float)learnClockCount[i], 1.f, 64.f));
                    }
                    learning[i] = false;
                } else {
                    // Start learning
                    learning[i] = true;
                    learnClockCount[i] = 0;
                }
            }
            lights[LEARN_1_LIGHT + i].setBrightness(learning[i] ? 1.f : 0.f);
        }

        // Get fade parameters
        int fadeClocks = (int)params[FADE_CLOCK_PARAM].getValue();
        float fadeTimeMs = params[FADE_TIME_PARAM].getValue();
        fadeDuration = fadeTimeMs / 1000.f;  // Convert to seconds

        // Process clock
        bool clockTriggered = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f);

        if (clockTriggered) {
            // Increment learn counters
            for (int i = 0; i < 8; i++) {
                if (learning[i]) {
                    learnClockCount[i]++;
                }
            }

            // Advance playback
            if (!sequence.empty()) {
                currentClockCount++;
                int currentLength = (int)params[LENGTH_1_PARAM + activeInput].getValue();

                // Check if we should start fading (N clocks before end)
                if (fadeClocks > 0 && fadeTimeMs > 0 && !fading) {
                    int fadeStartClock = currentLength - fadeClocks;
                    if (currentClockCount >= fadeStartClock && currentClockCount < currentLength) {
                        // Start fade - prepare next input
                        int nextIndex = currentSequenceIndex + 1;
                        if (nextIndex >= (int)sequence.size()) {
                            nextIndex = 0;
                        }
                        int nextInput = sequence[nextIndex];
                        if (nextInput != activeInput) {
                            fading = true;
                            previousInput = activeInput;
                            fadeElapsed = 0.f;
                        }
                    }
                }

                if (currentClockCount >= currentLength) {
                    // Move to next in sequence
                    currentClockCount = 0;
                    currentSequenceIndex++;
                    if (currentSequenceIndex >= (int)sequence.size()) {
                        currentSequenceIndex = 0;
                    }
                    int newInput = sequence[currentSequenceIndex];

                    // If not already fading, start immediate fade or switch
                    if (!fading && newInput != activeInput) {
                        if (fadeTimeMs > 0) {
                            fading = true;
                            previousInput = activeInput;
                            fadeElapsed = 0.f;
                        }
                    }
                    activeInput = newInput;

                    // Trigger pulse for the new segment
                    trigPulses[activeInput].trigger(0.001f);
                }
            }
        }

        // Update fade progress
        if (fading) {
            fadeElapsed += args.sampleTime;
            if (fadeDuration > 0) {
                fadeProgress = clamp(fadeElapsed / fadeDuration, 0.f, 1.f);
            } else {
                fadeProgress = 1.f;
            }
            if (fadeProgress >= 1.f) {
                fading = false;
                fadeProgress = 1.f;
            }
        } else {
            fadeProgress = 1.f;
        }

        // Set active lights
        for (int i = 0; i < 8; i++) {
            lights[ACTIVE_1_LIGHT + i].setBrightness(i == activeInput ? 1.f : 0.f);
        }

        // Process trigger outputs
        for (int i = 0; i < 8; i++) {
            float trigOut = trigPulses[i].process(args.sampleTime) ? 10.f : 0.f;
            outputs[TRIG_1_OUTPUT + i].setVoltage(trigOut);
        }

        // Route inputs to output with crossfade
        int numChannels = 1;
        if (inputs[IN_1_INPUT + activeInput].isConnected()) {
            numChannels = inputs[IN_1_INPUT + activeInput].getChannels();
        }
        if (fading && inputs[IN_1_INPUT + previousInput].isConnected()) {
            numChannels = std::max(numChannels, inputs[IN_1_INPUT + previousInput].getChannels());
        }

        outputs[OUT_OUTPUT].setChannels(numChannels);

        for (int c = 0; c < numChannels; c++) {
            float newVoltage = 0.f;
            float oldVoltage = 0.f;

            if (inputs[IN_1_INPUT + activeInput].isConnected()) {
                newVoltage = inputs[IN_1_INPUT + activeInput].getVoltage(c);
            }

            if (fading && inputs[IN_1_INPUT + previousInput].isConnected()) {
                oldVoltage = inputs[IN_1_INPUT + previousInput].getVoltage(c);
                // Linear crossfade
                float outVoltage = oldVoltage * (1.f - fadeProgress) + newVoltage * fadeProgress;
                outputs[OUT_OUTPUT].setVoltage(outVoltage, c);
            } else {
                outputs[OUT_OUTPUT].setVoltage(newVoltage, c);
            }
        }
    }
};

// Implement onChange after SongMode is fully defined
void SequenceTextField::onChange(const ChangeEvent& e) {
    if (module) {
        module->sequenceText = getText();
        module->parseSequence();
    }
    LedDisplayTextField::onChange(e);
}

// White background panel for bottom section
struct WhiteBottomPanel : TransparentWidget {
    void draw(const DrawArgs& args) override {
        // Draw white background from Y=330 to bottom
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 330, box.size.x, box.size.y - 330);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

struct SongModeWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    SequenceTextField* textField = nullptr;

    SongModeWidget(SongMode* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP");

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Add white background panel for bottom section (Y=330 and below)
        WhiteBottomPanel* whitePanel = new WhiteBottomPanel();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // Title
        addChild(new SongModeLabel(Vec(0, 1), Vec(box.size.x, 20), "SONG MODE", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new SongModeLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // Clock and Reset inputs
        addChild(new SongModeLabel(Vec(18, 32), Vec(30, 12), "CLK", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(33, 53), module, SongMode::CLOCK_INPUT));

        addChild(new SongModeLabel(Vec(62, 32), Vec(30, 12), "RST", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(77, 53), module, SongMode::RESET_INPUT));

        // Sequence text field (10px above first input row)
        textField = createWidget<SequenceTextField>(Vec(5, 66));
        textField->box.size = Vec(box.size.x - 10, 14);
        textField->module = module;
        if (module) {
            textField->setText(module->sequenceText);
        } else {
            textField->setText("12345678");
        }
        addChild(textField);

        // 8 input rows (text field ends at Y=80, 10px gap, rows start at Y=90)
        float startY = 90;
        float rowHeight = 28;

        for (int i = 0; i < 8; i++) {
            float y = startY + i * rowHeight;

            // Row number
            addChild(new SongModeLabel(Vec(0, y - 2), Vec(14, 12), std::to_string(i + 1), 9.f, nvgRGB(255, 200, 0), true));

            // Input jack
            addInput(createInputCentered<PJ301MPort>(Vec(22, y + 6), module, SongMode::IN_1_INPUT + i));

            // Length knob (MediumGrayKnob)
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(50, y + 6), module, SongMode::LENGTH_1_PARAM + i));

            // Learn button with centered light (like weiii documenta play/loop)
            addParam(createParamCentered<VCVButton>(Vec(74, y + 6), module, SongMode::LEARN_1_PARAM + i));
            addChild(createLightCentered<MediumLight<RedLight>>(Vec(74, y + 6), module, SongMode::LEARN_1_LIGHT + i));

            // Trigger output
            addOutput(createOutputCentered<PJ301MPort>(Vec(100, y + 6), module, SongMode::TRIG_1_OUTPUT + i));

            // Active light (3px right)
            addChild(createLightCentered<SmallLight<GreenLight>>(Vec(115, y + 6), module, SongMode::ACTIVE_1_LIGHT + i));
        }

        // Bottom section: Fade knobs and main output
        // Labels: top row (Fade, Switch), bottom row (Clock, Time, Out)
        addChild(new SongModeLabel(Vec(14, 332), Vec(44, 12), "Fade", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new SongModeLabel(Vec(78, 332), Vec(44, 12), "Switch", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new SongModeLabel(Vec(0, 367), Vec(44, 12), "Clock", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new SongModeLabel(Vec(28, 367), Vec(44, 12), "Time", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new SongModeLabel(Vec(78, 367), Vec(44, 12), "Out", 10.f, nvgRGB(255, 133, 133), true));

        // Fade Clock knob (X=22, same as input jacks)
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(22, 355), module, SongMode::FADE_CLOCK_PARAM));

        // Fade Time knob (X=50, same as length knobs)
        addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(50, 355), module, SongMode::FADE_TIME_PARAM));

        // Main output (X=100, same as TRIG outputs)
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, 355), module, SongMode::OUT_OUTPUT));
    }

    void step() override {
        SongMode* module = dynamic_cast<SongMode*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
            // Sync text field with module
            if (textField && textField->getText() != module->sequenceText) {
                textField->setText(module->sequenceText);
            }
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        SongMode* module = dynamic_cast<SongMode*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelSongMode = createModel<SongMode, SongModeWidget>("SongMode");
