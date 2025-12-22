#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

// Industrial color scheme
namespace LaunchpadColors {
    static const NVGcolor EMPTY = nvgRGB(50, 52, 55);
    static const NVGcolor HAS_CONTENT = nvgRGB(140, 100, 70);
    static const NVGcolor PLAYING = nvgRGB(60, 130, 100);
    static const NVGcolor RECORDING = nvgRGB(160, 70, 60);
    static const NVGcolor QUEUED = nvgRGB(180, 150, 80);
    static const NVGcolor STOP_QUEUED = nvgRGB(120, 90, 60);  // Darker, fading out

    // Waveform colors (brighter versions)
    static const NVGcolor WAVE_CONTENT = nvgRGB(180, 140, 100);
    static const NVGcolor WAVE_PLAYING = nvgRGB(100, 180, 140);
    static const NVGcolor WAVE_RECORDING = nvgRGB(200, 110, 100);
}

// Maximum recording length in samples (10 seconds at 48kHz)
static const int MAX_BUFFER_SIZE = 48000 * 10;

// Cell state enum
enum CellState {
    CELL_EMPTY,
    CELL_HAS_CONTENT,
    CELL_PLAYING,
    CELL_RECORDING,
    CELL_QUEUED,
    CELL_STOP_QUEUED  // Waiting for quantize boundary to stop
};

// Cell data structure
struct CellData {
    std::vector<float> buffer;
    int recordedLength = 0;  // Actual recorded samples
    int loopClocks = 0;      // Loop length in clocks
    CellState state = CELL_EMPTY;
    int playPosition = 0;
    int recordPosition = 0;

    // Waveform cache for display (downsampled)
    std::vector<float> waveformCache;
    bool waveformDirty = true;

    CellData() {
        buffer.reserve(MAX_BUFFER_SIZE);
    }

    void clear() {
        buffer.clear();
        recordedLength = 0;
        loopClocks = 0;
        state = CELL_EMPTY;
        playPosition = 0;
        recordPosition = 0;
        waveformCache.clear();
        waveformDirty = true;
    }

    void updateWaveformCache(int displayWidth) {
        if (!waveformDirty && (int)waveformCache.size() == displayWidth) return;

        waveformCache.resize(displayWidth);

        // Use recordPosition during recording, recordedLength otherwise
        int length = (state == CELL_RECORDING) ? recordPosition : recordedLength;

        if (length == 0) {
            std::fill(waveformCache.begin(), waveformCache.end(), 0.f);
            return;
        }

        // Store actual waveform samples (not envelope)
        for (int i = 0; i < displayWidth; i++) {
            int sampleIndex = i * length / displayWidth;
            if (sampleIndex < (int)buffer.size()) {
                waveformCache[i] = buffer[sampleIndex];
            } else {
                waveformCache[i] = 0.f;
            }
        }
        waveformDirty = false;
    }
};

// Forward declaration
struct Launchpad;

// Text label widget
struct LaunchpadLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    LaunchpadLabel(Vec pos, Vec size, std::string text, float fontSize = 10.f,
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
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

// Cell widget for grid display
struct CellWidget : OpaqueWidget {
    Launchpad* module = nullptr;
    int row = 0;
    int col = 0;
    float pressTime = 0.f;
    bool pressed = false;
    static constexpr float HOLD_TIME = 0.5f;  // 500ms for clear

    CellWidget() {
        box.size = Vec(40, 40);
    }

    void onButton(const event::Button& e) override;
    void onDragStart(const event::DragStart& e) override;
    void onDragEnd(const event::DragEnd& e) override;

    void draw(const DrawArgs& args) override;
    void drawWaveform(const DrawArgs& args, CellData& cell);
};

struct Launchpad : Module {
    int panelTheme = -1;

    enum ParamId {
        QUANTIZE_PARAM,
        STOP_ALL_PARAM,
        // Scene triggers
        SCENE_1_PARAM, SCENE_2_PARAM, SCENE_3_PARAM, SCENE_4_PARAM,
        SCENE_5_PARAM, SCENE_6_PARAM, SCENE_7_PARAM, SCENE_8_PARAM,
        // Per-row controls (8 rows × 4 params)
        SEND_A_1_PARAM, SEND_B_1_PARAM, PAN_1_PARAM, LEVEL_1_PARAM,
        SEND_A_2_PARAM, SEND_B_2_PARAM, PAN_2_PARAM, LEVEL_2_PARAM,
        SEND_A_3_PARAM, SEND_B_3_PARAM, PAN_3_PARAM, LEVEL_3_PARAM,
        SEND_A_4_PARAM, SEND_B_4_PARAM, PAN_4_PARAM, LEVEL_4_PARAM,
        SEND_A_5_PARAM, SEND_B_5_PARAM, PAN_5_PARAM, LEVEL_5_PARAM,
        SEND_A_6_PARAM, SEND_B_6_PARAM, PAN_6_PARAM, LEVEL_6_PARAM,
        SEND_A_7_PARAM, SEND_B_7_PARAM, PAN_7_PARAM, LEVEL_7_PARAM,
        SEND_A_8_PARAM, SEND_B_8_PARAM, PAN_8_PARAM, LEVEL_8_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        // Row inputs
        ROW_1_INPUT, ROW_2_INPUT, ROW_3_INPUT, ROW_4_INPUT,
        ROW_5_INPUT, ROW_6_INPUT, ROW_7_INPUT, ROW_8_INPUT,
        // Return inputs
        RETURN_A_L_INPUT, RETURN_A_R_INPUT,
        RETURN_B_L_INPUT, RETURN_B_R_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        // Row outputs
        ROW_1_OUTPUT, ROW_2_OUTPUT, ROW_3_OUTPUT, ROW_4_OUTPUT,
        ROW_5_OUTPUT, ROW_6_OUTPUT, ROW_7_OUTPUT, ROW_8_OUTPUT,
        // Send outputs
        SEND_A_L_OUTPUT, SEND_A_R_OUTPUT,
        SEND_B_L_OUTPUT, SEND_B_R_OUTPUT,
        // Mix output
        MIX_L_OUTPUT, MIX_R_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHTS_LEN
    };

    // 8x8 grid of cells
    CellData cells[8][8];

    // Clock tracking
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger sceneTriggers[8];
    dsp::SchmittTrigger stopAllTrigger;
    int clockCount = 0;
    int samplesPerClock = 0;
    int samplesSinceLastClock = 0;
    float lastClockTime = 0.f;

    // Recording state
    int recordingRow = -1;
    int recordingCol = -1;
    int recordStartClock = 0;

    // Quantize values: 0=Free, 1=1, 2=8, 3=16, 4=32, 5=64
    const int quantizeValues[6] = {0, 1, 8, 16, 32, 64};

    Launchpad() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Quantize knob
        configSwitch(QUANTIZE_PARAM, 0.f, 5.f, 0.f, "Quantize",
            {"Free", "1", "8", "16", "32", "64"});

        // Stop all button
        configButton(STOP_ALL_PARAM, "Stop All");

        // Scene triggers
        for (int i = 0; i < 8; i++) {
            configButton(SCENE_1_PARAM + i, string::f("Scene %d", i + 1));
        }

        // Per-row controls
        for (int i = 0; i < 8; i++) {
            configParam(SEND_A_1_PARAM + i * 4, 0.f, 1.f, 0.f, string::f("Row %d Send A", i + 1));
            configParam(SEND_B_1_PARAM + i * 4, 0.f, 1.f, 0.f, string::f("Row %d Send B", i + 1));
            configParam(PAN_1_PARAM + i * 4, -1.f, 1.f, 0.f, string::f("Row %d Pan", i + 1));
            configParam(LEVEL_1_PARAM + i * 4, 0.f, 1.f, 1.f, string::f("Row %d Level", i + 1));
        }

        // Inputs
        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        for (int i = 0; i < 8; i++) {
            configInput(ROW_1_INPUT + i, string::f("Row %d", i + 1));
        }
        configInput(RETURN_A_L_INPUT, "Return A Left");
        configInput(RETURN_A_R_INPUT, "Return A Right");
        configInput(RETURN_B_L_INPUT, "Return B Left");
        configInput(RETURN_B_R_INPUT, "Return B Right");

        // Outputs
        for (int i = 0; i < 8; i++) {
            configOutput(ROW_1_OUTPUT + i, string::f("Row %d", i + 1));
        }
        configOutput(SEND_A_L_OUTPUT, "Send A Left");
        configOutput(SEND_A_R_OUTPUT, "Send A Right");
        configOutput(SEND_B_L_OUTPUT, "Send B Left");
        configOutput(SEND_B_R_OUTPUT, "Send B Right");
        configOutput(MIX_L_OUTPUT, "Mix Left");
        configOutput(MIX_R_OUTPUT, "Mix Right");
    }

    void onReset() override {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                cells[r][c].clear();
            }
        }
        clockCount = 0;
        recordingRow = -1;
        recordingCol = -1;
    }

    // Cell interaction
    void onCellClick(int row, int col) {
        CellData& cell = cells[row][col];

        if (cell.state == CELL_EMPTY) {
            // Start recording
            startRecording(row, col);
        } else if (cell.state == CELL_RECORDING) {
            // Stop recording
            stopRecording();
        } else if (cell.state == CELL_PLAYING) {
            // Stop playing
            cell.state = CELL_HAS_CONTENT;
            cell.playPosition = 0;
        } else if (cell.state == CELL_HAS_CONTENT || cell.state == CELL_QUEUED) {
            // Start playing (with quantize)
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize == 0) {
                // Free - immediate
                startPlaying(row, col);
            } else {
                // Queue for next quantize boundary
                cell.state = CELL_QUEUED;
            }
        }
    }

    void onCellHold(int row, int col) {
        // Clear cell
        cells[row][col].clear();
    }

    void startRecording(int row, int col) {
        // Stop any current recording
        if (recordingRow >= 0) {
            stopRecording();
        }

        CellData& cell = cells[row][col];
        cell.buffer.clear();
        cell.buffer.resize(MAX_BUFFER_SIZE, 0.f);
        cell.recordPosition = 0;
        cell.recordedLength = 0;
        cell.state = CELL_RECORDING;
        cell.waveformDirty = true;

        recordingRow = row;
        recordingCol = col;
        recordStartClock = clockCount;
    }

    void stopRecording() {
        if (recordingRow < 0) return;

        CellData& cell = cells[recordingRow][recordingCol];
        cell.recordedLength = cell.recordPosition;
        cell.loopClocks = clockCount - recordStartClock;
        if (cell.loopClocks < 1) cell.loopClocks = 1;
        cell.state = cell.recordedLength > 0 ? CELL_HAS_CONTENT : CELL_EMPTY;
        cell.waveformDirty = true;

        recordingRow = -1;
        recordingCol = -1;
    }

    void startPlaying(int row, int col) {
        // Session mode: stop other cells in the same row
        for (int c = 0; c < 8; c++) {
            if (c != col && (cells[row][c].state == CELL_PLAYING || cells[row][c].state == CELL_QUEUED)) {
                cells[row][c].state = CELL_HAS_CONTENT;
                cells[row][c].playPosition = 0;
            }
        }

        CellData& cell = cells[row][col];
        cell.state = CELL_PLAYING;
        cell.playPosition = 0;
    }

    void stopAll() {
        // Stop all playing and queued cells
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                CellData& cell = cells[r][c];
                if (cell.state == CELL_PLAYING || cell.state == CELL_QUEUED) {
                    cell.state = CELL_HAS_CONTENT;
                    cell.playPosition = 0;
                }
            }
        }
    }

    void triggerScene(int col) {
        // Trigger cells in the target column (Ableton Live style)
        // Scene acts as a "snapshot" - cells not in this scene get stopped
        for (int r = 0; r < 8; r++) {
            CellData& cell = cells[r][col];
            if (cell.state == CELL_HAS_CONTENT) {
                // Scene cell has content: play it
                int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
                if (quantize == 0) {
                    // Free mode: immediate start (startPlaying stops other cells)
                    startPlaying(r, col);
                } else {
                    // Quantize mode: queue this cell
                    // Cancel any other QUEUED in this row (only one can be queued)
                    for (int c = 0; c < 8; c++) {
                        if (c != col && cells[r][c].state == CELL_QUEUED) {
                            cells[r][c].state = CELL_HAS_CONTENT;
                        }
                    }
                    cell.state = CELL_QUEUED;
                }
            } else if (cell.state == CELL_EMPTY) {
                // Scene cell is empty: stop any playing cells in this row
                int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
                for (int c = 0; c < 8; c++) {
                    if (cells[r][c].state == CELL_PLAYING) {
                        if (quantize == 0) {
                            // Free mode: immediate stop
                            cells[r][c].state = CELL_HAS_CONTENT;
                            cells[r][c].playPosition = 0;
                        } else {
                            // Quantize mode: queue for stop
                            cells[r][c].state = CELL_STOP_QUEUED;
                        }
                    } else if (cells[r][c].state == CELL_QUEUED) {
                        // Cancel queued play
                        cells[r][c].state = CELL_HAS_CONTENT;
                    }
                }
            }
        }
    }

    void process(const ProcessArgs& args) override {
        // Process reset
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f)) {
            clockCount = 0;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    if (cells[r][c].state == CELL_PLAYING) {
                        cells[r][c].playPosition = 0;
                    }
                }
            }
        }

        // Process clock
        bool clockTriggered = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f);

        if (clockTriggered) {
            // Calculate samples per clock for accurate loop timing
            if (samplesSinceLastClock > 0) {
                samplesPerClock = samplesSinceLastClock;
            }
            samplesSinceLastClock = 0;
            clockCount++;

            // Check quantize triggers
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize > 0 && (clockCount % quantize) == 0) {
                // Process queued cells
                for (int r = 0; r < 8; r++) {
                    for (int c = 0; c < 8; c++) {
                        if (cells[r][c].state == CELL_QUEUED) {
                            startPlaying(r, c);
                        } else if (cells[r][c].state == CELL_STOP_QUEUED) {
                            // Stop at quantize boundary
                            cells[r][c].state = CELL_HAS_CONTENT;
                            cells[r][c].playPosition = 0;
                        }
                    }
                }
            }
        }
        samplesSinceLastClock++;

        // Process stop all
        if (stopAllTrigger.process(params[STOP_ALL_PARAM].getValue())) {
            stopAll();
        }

        // Process scene triggers
        for (int i = 0; i < 8; i++) {
            if (sceneTriggers[i].process(params[SCENE_1_PARAM + i].getValue())) {
                triggerScene(i);
            }
        }

        // Process recording
        if (recordingRow >= 0 && recordingCol >= 0) {
            CellData& cell = cells[recordingRow][recordingCol];
            float inputVoltage = inputs[ROW_1_INPUT + recordingRow].getVoltage();

            if (cell.recordPosition < MAX_BUFFER_SIZE) {
                cell.buffer[cell.recordPosition++] = inputVoltage;
                cell.waveformDirty = true;
            } else {
                // Buffer full, stop recording
                stopRecording();
            }
        }

        // Mixing
        float mixL = 0.f, mixR = 0.f;
        float sendAL = 0.f, sendAR = 0.f;
        float sendBL = 0.f, sendBR = 0.f;

        for (int r = 0; r < 8; r++) {
            float rowOutput = 0.f;

            // Find playing cell in this row
            for (int c = 0; c < 8; c++) {
                CellData& cell = cells[r][c];
                // STOP_QUEUED continues playing until quantize boundary
                if ((cell.state == CELL_PLAYING || cell.state == CELL_STOP_QUEUED) && cell.recordedLength > 0) {
                    rowOutput = cell.buffer[cell.playPosition];

                    // Advance play position
                    cell.playPosition++;
                    if (cell.playPosition >= cell.recordedLength) {
                        cell.playPosition = 0;  // Loop
                    }
                    break;  // Session mode: only one cell per row
                }
            }

            // Apply level
            float level = params[LEVEL_1_PARAM + r * 4].getValue();
            rowOutput *= level;

            // Apply pan
            float pan = params[PAN_1_PARAM + r * 4].getValue();
            float panL = (pan <= 0) ? 1.f : (1.f - pan);
            float panR = (pan >= 0) ? 1.f : (1.f + pan);

            float rowL = rowOutput * panL;
            float rowR = rowOutput * panR;

            // Send to outputs
            outputs[ROW_1_OUTPUT + r].setVoltage(rowOutput);

            // Accumulate sends
            float sendA = params[SEND_A_1_PARAM + r * 4].getValue();
            float sendB = params[SEND_B_1_PARAM + r * 4].getValue();
            sendAL += rowL * sendA;
            sendAR += rowR * sendA;
            sendBL += rowL * sendB;
            sendBR += rowR * sendB;

            // Accumulate mix
            mixL += rowL;
            mixR += rowR;
        }

        // Add returns to mix
        mixL += inputs[RETURN_A_L_INPUT].getVoltage() + inputs[RETURN_B_L_INPUT].getVoltage();
        mixR += inputs[RETURN_A_R_INPUT].getVoltage() + inputs[RETURN_B_R_INPUT].getVoltage();

        // Output sends
        outputs[SEND_A_L_OUTPUT].setVoltage(sendAL);
        outputs[SEND_A_R_OUTPUT].setVoltage(sendAR);
        outputs[SEND_B_L_OUTPUT].setVoltage(sendBL);
        outputs[SEND_B_R_OUTPUT].setVoltage(sendBR);

        // Output mix
        outputs[MIX_L_OUTPUT].setVoltage(mixL);
        outputs[MIX_R_OUTPUT].setVoltage(mixR);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

        // Save cell data
        json_t* cellsJ = json_array();
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                json_t* cellJ = json_object();
                CellData& cell = cells[r][c];

                json_object_set_new(cellJ, "loopClocks", json_integer(cell.loopClocks));
                json_object_set_new(cellJ, "recordedLength", json_integer(cell.recordedLength));

                // Save buffer as base64 or skip if empty
                if (cell.recordedLength > 0) {
                    json_t* bufferJ = json_array();
                    for (int i = 0; i < cell.recordedLength; i++) {
                        json_array_append_new(bufferJ, json_real(cell.buffer[i]));
                    }
                    json_object_set_new(cellJ, "buffer", bufferJ);
                }

                json_array_append_new(cellsJ, cellJ);
            }
        }
        json_object_set_new(rootJ, "cells", cellsJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* cellsJ = json_object_get(rootJ, "cells");
        if (cellsJ) {
            int index = 0;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    json_t* cellJ = json_array_get(cellsJ, index++);
                    if (!cellJ) continue;

                    CellData& cell = cells[r][c];

                    json_t* loopClocksJ = json_object_get(cellJ, "loopClocks");
                    if (loopClocksJ) cell.loopClocks = json_integer_value(loopClocksJ);

                    json_t* recordedLengthJ = json_object_get(cellJ, "recordedLength");
                    if (recordedLengthJ) cell.recordedLength = json_integer_value(recordedLengthJ);

                    json_t* bufferJ = json_object_get(cellJ, "buffer");
                    if (bufferJ && cell.recordedLength > 0) {
                        cell.buffer.resize(MAX_BUFFER_SIZE, 0.f);
                        for (int i = 0; i < cell.recordedLength && i < (int)json_array_size(bufferJ); i++) {
                            cell.buffer[i] = json_real_value(json_array_get(bufferJ, i));
                        }
                        cell.state = CELL_HAS_CONTENT;
                        cell.waveformDirty = true;
                    }
                }
            }
        }
    }
};

// CellWidget implementations
void CellWidget::onButton(const event::Button& e) {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        pressed = true;
        pressTime = 0.f;
        e.consume(this);
    }
}

void CellWidget::onDragStart(const event::DragStart& e) {
    pressed = true;
    pressTime = 0.f;
}

void CellWidget::onDragEnd(const event::DragEnd& e) {
    if (pressed && module) {
        if (pressTime >= HOLD_TIME) {
            // Hold - clear
            module->onCellHold(row, col);
        } else {
            // Click - toggle
            module->onCellClick(row, col);
        }
    }
    pressed = false;
}

void CellWidget::draw(const DrawArgs& args) {
    // Update press time
    if (pressed) {
        pressTime += APP->engine->getSampleTime() * 1000;  // Approximate
    }

    // Get cell state
    CellState state = CELL_EMPTY;
    if (module) {
        state = module->cells[row][col].state;
    }

    // Choose color based on state
    NVGcolor bgColor;
    switch (state) {
        case CELL_HAS_CONTENT: bgColor = LaunchpadColors::HAS_CONTENT; break;
        case CELL_PLAYING: bgColor = LaunchpadColors::PLAYING; break;
        case CELL_RECORDING: bgColor = LaunchpadColors::RECORDING; break;
        case CELL_QUEUED: bgColor = LaunchpadColors::QUEUED; break;
        case CELL_STOP_QUEUED: bgColor = LaunchpadColors::STOP_QUEUED; break;
        default: bgColor = LaunchpadColors::EMPTY; break;
    }

    // Draw cell background with depth effect
    float x = 0, y = 0, w = box.size.x, h = box.size.y;

    // Outer shadow
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, x + 1, y + 1, w, h, 3);
    nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 60));
    nvgFill(args.vg);

    // Main background
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, x, y, w, h, 3);
    nvgFillColor(args.vg, bgColor);
    nvgFill(args.vg);

    // Inner highlight (top-left)
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, x + 1, y + 1, w - 2, h / 2, 2);
    nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 15));
    nvgFill(args.vg);

    // Draw waveform if has content
    if (module && (state == CELL_HAS_CONTENT || state == CELL_PLAYING || state == CELL_RECORDING || state == CELL_QUEUED || state == CELL_STOP_QUEUED)) {
        drawWaveform(args, module->cells[row][col]);
    }

    // Draw loop length indicator
    if (module && module->cells[row][col].loopClocks > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", module->cells[row][col].loopClocks);

        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 180));
        nvgText(args.vg, w - 3, h - 2, buf, NULL);
    }

    // Border
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, x + 0.5f, y + 0.5f, w - 1, h - 1, 3);
    nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 100));
    nvgStrokeWidth(args.vg, 1.0f);
    nvgStroke(args.vg);

    // Pressed effect
    if (pressed) {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, x, y, w, h, 3);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 30));
        nvgFill(args.vg);
    }
}

void CellWidget::drawWaveform(const DrawArgs& args, CellData& cell) {
    // Use recordPosition during recording, recordedLength otherwise
    int length = (cell.state == CELL_RECORDING) ? cell.recordPosition : cell.recordedLength;
    if (length == 0) return;

    int displayWidth = (int)box.size.x - 8;
    cell.updateWaveformCache(displayWidth);

    // Choose waveform color
    NVGcolor waveColor;
    switch (cell.state) {
        case CELL_PLAYING: waveColor = LaunchpadColors::WAVE_PLAYING; break;
        case CELL_STOP_QUEUED: waveColor = LaunchpadColors::WAVE_PLAYING; break;  // Still playing
        case CELL_RECORDING: waveColor = LaunchpadColors::WAVE_RECORDING; break;
        default: waveColor = LaunchpadColors::WAVE_CONTENT; break;
    }

    float centerY = box.size.y / 2;
    float maxHeight = box.size.y / 2 - 4;  // Leave 4px margin top/bottom

    // Draw actual waveform as connected line (±10V scaling)
    nvgBeginPath(args.vg);
    bool first = true;
    for (int i = 0; i < displayWidth; i++) {
        float voltage = cell.waveformCache[i];
        float y = centerY - (voltage / 10.f) * maxHeight;  // ±10V scaling
        y = clamp(y, 2.f, box.size.y - 2.f);

        float x = 4 + i;
        if (first) {
            nvgMoveTo(args.vg, x, y);
            first = false;
        } else {
            nvgLineTo(args.vg, x, y);
        }
    }
    nvgStrokeColor(args.vg, waveColor);
    nvgStrokeWidth(args.vg, 1.0f);
    nvgStroke(args.vg);

    // Draw recording progress bar at bottom
    if (cell.state == CELL_RECORDING && cell.recordPosition > 0) {
        float progress = (float)cell.recordPosition / MAX_BUFFER_SIZE;
        float barWidth = progress * (box.size.x - 4);

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 2, box.size.y - 3, barWidth, 2);
        nvgFillColor(args.vg, nvgRGBA(255, 100, 100, 200));
        nvgFill(args.vg);
    }

    // Draw playhead if playing (including STOP_QUEUED which continues until quantize)
    if ((cell.state == CELL_PLAYING || cell.state == CELL_STOP_QUEUED) && cell.recordedLength > 0) {
        float playPos = (float)cell.playPosition / cell.recordedLength;
        float x = 4 + playPos * displayWidth;

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, x, 2);
        nvgLineTo(args.vg, x, box.size.y - 2);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }
}

// White background panel for bottom section
struct WhiteBottomPanel40HP : TransparentWidget {
    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 330, box.size.x, box.size.y - 330);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
};

struct LaunchpadWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    std::vector<CellWidget*> cellWidgets;

    LaunchpadWidget(Launchpad* module) {
        setModule(module);
        panelThemeHelper.init(this, "40HP");

        box.size = Vec(40 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // White bottom panel
        WhiteBottomPanel40HP* whitePanel = new WhiteBottomPanel40HP();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // Title - doubled size
        addChild(new LaunchpadLabel(Vec(0, 1), Vec(200, 30), "LAUNCHPAD", 24.f, nvgRGB(255, 200, 0), true));
        addChild(new LaunchpadLabel(Vec(0, 22), Vec(200, 25), "MADZINE", 20.f, nvgRGB(255, 200, 0), false));

        // Clock, Reset, Quantize - upper right, labels up 2px
        addChild(new LaunchpadLabel(Vec(455, 26), Vec(50, 12), "Clock", 10.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(480, 50), module, Launchpad::CLOCK_INPUT));

        addChild(new LaunchpadLabel(Vec(500, 26), Vec(50, 12), "Reset", 10.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(525, 50), module, Launchpad::RESET_INPUT));

        addChild(new LaunchpadLabel(Vec(540, 26), Vec(60, 12), "Quantize", 10.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(570, 50), module, Launchpad::QUANTIZE_PARAM));

        // Stop All button (same Y as scene buttons, label up 7px total)
        addChild(new LaunchpadLabel(Vec(2, 48), Vec(50, 12), "Stop All", 10.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<VCVButton>(Vec(27, 70), module, Launchpad::STOP_ALL_PARAM));

        // Scene buttons (aligned with cells)
        float cellStartX = 70;
        float cellSpacing = 44;
        for (int i = 0; i < 8; i++) {
            float x = cellStartX + i * cellSpacing;
            addParam(createParamCentered<VCVButton>(Vec(x, 70), module, Launchpad::SCENE_1_PARAM + i));
        }

        // Column labels - Pan/Level moved left 3px total
        addChild(new LaunchpadLabel(Vec(2, 82), Vec(30, 12), "IN", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new LaunchpadLabel(Vec(410, 79), Vec(45, 12), "Send A", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new LaunchpadLabel(Vec(448, 79), Vec(45, 12), "Send B", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new LaunchpadLabel(Vec(488, 79), Vec(35, 12), "Pan", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new LaunchpadLabel(Vec(521, 79), Vec(45, 12), "Level", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new LaunchpadLabel(Vec(557, 79), Vec(40, 12), "OUT", 10.f, nvgRGB(255, 255, 255), true));

        // 8 rows
        float rowStartY = 100;
        float rowSpacing = 28;

        for (int r = 0; r < 8; r++) {
            float y = rowStartY + r * rowSpacing;

            // Row input
            addInput(createInputCentered<PJ301MPort>(Vec(27, y + 3), module, Launchpad::ROW_1_INPUT + r));

            // 8 cells per row
            for (int c = 0; c < 8; c++) {
                CellWidget* cellWidget = new CellWidget();
                cellWidget->module = module;
                cellWidget->row = r;
                cellWidget->col = c;
                cellWidget->box.pos = Vec(cellStartX + c * cellSpacing - 20, y - 11);
                cellWidget->box.size = Vec(40, 28);  // Adjusted for row spacing
                addChild(cellWidget);
                cellWidgets.push_back(cellWidget);
            }

            // Per-row knobs - moved left and up 3px
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(430, y + 3), module, Launchpad::SEND_A_1_PARAM + r * 4));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(468, y + 3), module, Launchpad::SEND_B_1_PARAM + r * 4));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(506, y + 3), module, Launchpad::PAN_1_PARAM + r * 4));
            addParam(createParamCentered<madzine::widgets::MediumGrayKnob>(Vec(544, y + 3), module, Launchpad::LEVEL_1_PARAM + r * 4));

            // Row output - moved up 3px
            addOutput(createOutputCentered<PJ301MPort>(Vec(577, y + 3), module, Launchpad::ROW_1_OUTPUT + r));
        }

        // Bottom section (Y=330+)
        // Send A
        addChild(new LaunchpadLabel(Vec(20, 332), Vec(60, 12), "Send A", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(20, 370), Vec(30, 12), "L", 9.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(50, 370), Vec(30, 12), "R", 9.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(35, 355), module, Launchpad::SEND_A_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(65, 355), module, Launchpad::SEND_A_R_OUTPUT));

        // Return A
        addChild(new LaunchpadLabel(Vec(100, 332), Vec(60, 12), "Return A", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(100, 370), Vec(30, 12), "L", 9.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(130, 370), Vec(30, 12), "R", 9.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(115, 355), module, Launchpad::RETURN_A_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(145, 355), module, Launchpad::RETURN_A_R_INPUT));

        // Send B
        addChild(new LaunchpadLabel(Vec(200, 332), Vec(60, 12), "Send B", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(200, 370), Vec(30, 12), "L", 9.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(230, 370), Vec(30, 12), "R", 9.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(215, 355), module, Launchpad::SEND_B_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(245, 355), module, Launchpad::SEND_B_R_OUTPUT));

        // Return B
        addChild(new LaunchpadLabel(Vec(280, 332), Vec(60, 12), "Return B", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(280, 370), Vec(30, 12), "L", 9.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(310, 370), Vec(30, 12), "R", 9.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(295, 355), module, Launchpad::RETURN_B_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(325, 355), module, Launchpad::RETURN_B_R_INPUT));

        // Mix
        addChild(new LaunchpadLabel(Vec(520, 332), Vec(60, 12), "Mix", 10.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(520, 370), Vec(30, 12), "L", 9.f, nvgRGB(255, 133, 133), true));
        addChild(new LaunchpadLabel(Vec(550, 370), Vec(30, 12), "R", 9.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(535, 355), module, Launchpad::MIX_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(565, 355), module, Launchpad::MIX_R_OUTPUT));
    }

    void step() override {
        Launchpad* module = dynamic_cast<Launchpad*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Launchpad* module = dynamic_cast<Launchpad*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelLaunchpad = createModel<Launchpad, LaunchpadWidget>("Launchpad");
