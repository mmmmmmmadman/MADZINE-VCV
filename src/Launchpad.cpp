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
    static const NVGcolor RECORD_QUEUED = nvgRGB(180, 100, 80);  // Waiting to record

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
    CELL_STOP_QUEUED,       // Waiting for quantize boundary to stop
    CELL_RECORD_QUEUED      // Waiting for quantize boundary to start recording
};

// Fade duration in samples (2ms at 48kHz)
static const int FADE_SAMPLES = 96;

// Cell data structure
struct CellData {
    std::vector<float> buffer;
    int recordedLength = 0;  // Actual recorded samples
    int loopClocks = 0;      // Loop length in clocks
    CellState state = CELL_EMPTY;
    int playPosition = 0;
    int recordPosition = 0;

    // Fade envelope state
    float fadeGain = 0.0f;       // Current fade gain (0.0 to 1.0)
    bool fadingIn = false;       // Currently fading in
    bool fadingOut = false;      // Currently fading out
    int fadeSamples = 0;         // Samples remaining in fade

    // Waveform cache for display (downsampled)
    std::vector<float> waveformCache;
    bool waveformDirty = true;

    // Loop clocks string cache (avoid snprintf every frame)
    std::string loopClocksStr;
    int loopClocksCached = -1;

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
        fadeGain = 0.0f;
        fadingIn = false;
        fadingOut = false;
        fadeSamples = 0;
        waveformCache.clear();
        waveformDirty = true;
        loopClocksStr.clear();
        loopClocksCached = -1;
    }

    // Start fade in
    void startFadeIn() {
        fadingIn = true;
        fadingOut = false;
        fadeSamples = FADE_SAMPLES;
        fadeGain = 0.0f;
    }

    // Start fade out
    void startFadeOut() {
        fadingIn = false;
        fadingOut = true;
        fadeSamples = FADE_SAMPLES;
        // fadeGain keeps current value
    }

    // Process fade and return current gain
    float processFade() {
        if (fadingIn) {
            fadeGain += 1.0f / FADE_SAMPLES;
            fadeSamples--;
            if (fadeSamples <= 0 || fadeGain >= 1.0f) {
                fadeGain = 1.0f;
                fadingIn = false;
            }
        } else if (fadingOut) {
            fadeGain -= 1.0f / FADE_SAMPLES;
            fadeSamples--;
            if (fadeSamples <= 0 || fadeGain <= 0.0f) {
                fadeGain = 0.0f;
                fadingOut = false;
            }
        }
        return fadeGain;
    }

    // Check if fade out is complete
    bool isFadeOutComplete() const {
        return !fadingOut && fadeGain <= 0.0f;
    }

    const std::string& getLoopClocksStr() {
        if (loopClocksCached != loopClocks) {
            loopClocksStr = std::to_string(loopClocks);
            loopClocksCached = loopClocks;
        }
        return loopClocksStr;
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

    // Static drag state (shared across all cells)
    static CellWidget* dragSource;
    static bool dragCopyMode;
    static Vec dragOffset;  // Track mouse offset from start
    static int targetRow;   // Pre-calculated drag target
    static int targetCol;

    CellWidget() {
        box.size = Vec(40, 40);
    }

    void onButton(const event::Button& e) override;
    void onDragStart(const event::DragStart& e) override;
    void onDragEnd(const event::DragEnd& e) override;
    void onDragMove(const event::DragMove& e) override;

    void draw(const DrawArgs& args) override;
    void drawWaveform(const DrawArgs& args, CellData& cell);
};

// Static member initialization
CellWidget* CellWidget::dragSource = nullptr;
bool CellWidget::dragCopyMode = false;
Vec CellWidget::dragOffset = Vec(0, 0);
int CellWidget::targetRow = -1;
int CellWidget::targetCol = -1;

struct Launchpad : Module {
    int panelTheme = -1;
    float panelContrast = panelContrastDefault;

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
        // Scene trigger inputs
        SCENE_1_TRIG_INPUT, SCENE_2_TRIG_INPUT, SCENE_3_TRIG_INPUT, SCENE_4_TRIG_INPUT,
        SCENE_5_TRIG_INPUT, SCENE_6_TRIG_INPUT, SCENE_7_TRIG_INPUT, SCENE_8_TRIG_INPUT,
        // Stop all trigger input
        STOP_ALL_TRIG_INPUT,
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
    dsp::SchmittTrigger sceneTriggers[8];        // For button params
    dsp::SchmittTrigger sceneInputTriggers[8];   // For input jacks
    dsp::SchmittTrigger stopAllTrigger;          // For button param
    dsp::SchmittTrigger stopAllInputTrigger;     // For input jack
    int clockCount = 0;
    int samplesPerClock = 0;
    int samplesSinceLastClock = 0;
    float lastClockTime = 0.f;

    // Queued actions for quantize timing
    bool queuedScenes[8] = {false};
    bool queuedStopAll = false;
    bool queuedRecordStop = false;  // Queue recording stop for quantize

    // Recording state
    int recordingRow = -1;
    int recordingCol = -1;
    int recordStartClock = 0;

    // Pending fade-out stops (cells that need to complete fade before fully stopping)
    struct PendingStop {
        int row = -1;
        int col = -1;
        bool active = false;
    };
    PendingStop pendingStops[64];  // Max 8x8 cells

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
            configInput(SCENE_1_TRIG_INPUT + i, string::f("Scene %d Trigger", i + 1));
        }
        configInput(STOP_ALL_TRIG_INPUT, "Stop All Trigger");
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
        for (int i = 0; i < 8; i++) {
            queuedScenes[i] = false;
        }
        queuedStopAll = false;
        queuedRecordStop = false;
        for (int i = 0; i < 64; i++) {
            pendingStops[i].active = false;
        }
    }

    // Cell interaction
    void onCellClick(int row, int col) {
        CellData& cell = cells[row][col];

        if (cell.state == CELL_EMPTY) {
            // Start recording (with quantize)
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize == 0) {
                startRecording(row, col);
            } else {
                // Queue for next quantize boundary
                cell.state = CELL_RECORD_QUEUED;
            }
        } else if (cell.state == CELL_RECORD_QUEUED) {
            // Cancel queued recording
            cell.state = CELL_EMPTY;
        } else if (cell.state == CELL_RECORDING) {
            // Stop recording (with quantize)
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize == 0) {
                stopRecording();
            } else {
                // Queue for next quantize boundary
                queuedRecordStop = true;
            }
        } else if (cell.state == CELL_PLAYING) {
            // Stop playing (with quantize)
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize == 0) {
                // Free mode: start fade out, then stop
                cell.startFadeOut();
                addPendingStop(row, col);
            } else {
                // Quantize mode: queue for stop at next boundary
                cell.state = CELL_STOP_QUEUED;
            }
        } else if (cell.state == CELL_STOP_QUEUED) {
            // Cancel stop queue - resume playing
            cell.state = CELL_PLAYING;
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

    // Add a cell to pending stop list (waiting for fade out to complete)
    void addPendingStop(int row, int col) {
        for (int i = 0; i < 64; i++) {
            if (!pendingStops[i].active) {
                pendingStops[i].row = row;
                pendingStops[i].col = col;
                pendingStops[i].active = true;
                return;
            }
        }
    }

    // Process pending stops (call each sample)
    void processPendingStops() {
        for (int i = 0; i < 64; i++) {
            if (pendingStops[i].active) {
                CellData& cell = cells[pendingStops[i].row][pendingStops[i].col];
                if (cell.isFadeOutComplete()) {
                    cell.state = CELL_HAS_CONTENT;
                    cell.playPosition = 0;
                    cell.fadeGain = 0.0f;
                    pendingStops[i].active = false;
                }
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
        // Session mode: stop other cells in the same row (with fade out)
        for (int c = 0; c < 8; c++) {
            if (c != col && (cells[row][c].state == CELL_PLAYING || cells[row][c].state == CELL_QUEUED || cells[row][c].state == CELL_STOP_QUEUED)) {
                if (cells[row][c].state == CELL_PLAYING || cells[row][c].state == CELL_STOP_QUEUED) {
                    // Start fade out for currently playing cell
                    cells[row][c].startFadeOut();
                    addPendingStop(row, c);
                } else {
                    cells[row][c].state = CELL_HAS_CONTENT;
                    cells[row][c].playPosition = 0;
                }
            }
        }

        CellData& cell = cells[row][col];
        cell.state = CELL_PLAYING;
        cell.playPosition = 0;
        cell.startFadeIn();  // Start with fade in
    }

    void stopAll() {
        // Stop all playing and queued cells (respects quantize setting)
        int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                CellData& cell = cells[r][c];
                if (cell.state == CELL_PLAYING) {
                    if (quantize == 0) {
                        // Free mode: fade out then stop
                        cell.startFadeOut();
                        addPendingStop(r, c);
                    } else {
                        // Quantize mode: queue for stop at next boundary
                        cell.state = CELL_STOP_QUEUED;
                    }
                } else if (cell.state == CELL_QUEUED) {
                    // Queued cells can be cancelled immediately
                    cell.state = CELL_HAS_CONTENT;
                    cell.playPosition = 0;
                }
            }
        }
    }

    // Stop cell at quantize boundary (with fade)
    void stopCellAtQuantize(int row, int col) {
        CellData& cell = cells[row][col];
        cell.startFadeOut();
        addPendingStop(row, col);
    }

    void moveCell(int srcRow, int srcCol, int dstRow, int dstCol) {
        if (srcRow == dstRow && srcCol == dstCol) return;
        CellData& src = cells[srcRow][srcCol];
        CellData& dst = cells[dstRow][dstCol];

        // Stop playing if source is playing
        if (src.state == CELL_PLAYING || src.state == CELL_STOP_QUEUED) {
            src.state = CELL_HAS_CONTENT;
            src.playPosition = 0;
        }

        // Move data to destination
        dst.buffer = std::move(src.buffer);
        dst.recordedLength = src.recordedLength;
        dst.loopClocks = src.loopClocks;
        dst.waveformCache = std::move(src.waveformCache);
        dst.state = (dst.recordedLength > 0) ? CELL_HAS_CONTENT : CELL_EMPTY;
        dst.playPosition = 0;

        // Clear source
        src.buffer.clear();
        src.recordedLength = 0;
        src.loopClocks = 0;
        src.waveformCache.clear();
        src.state = CELL_EMPTY;
        src.playPosition = 0;
    }

    void copyCell(int srcRow, int srcCol, int dstRow, int dstCol) {
        if (srcRow == dstRow && srcCol == dstCol) return;
        CellData& src = cells[srcRow][srcCol];
        CellData& dst = cells[dstRow][dstCol];

        // Copy data to destination
        dst.buffer = src.buffer;  // Copy, not move
        dst.recordedLength = src.recordedLength;
        dst.loopClocks = src.loopClocks;
        dst.waveformCache = src.waveformCache;
        dst.state = (dst.recordedLength > 0) ? CELL_HAS_CONTENT : CELL_EMPTY;
        dst.playPosition = 0;
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
                            // Stop at quantize boundary with fade
                            stopCellAtQuantize(r, c);
                        } else if (cells[r][c].state == CELL_RECORD_QUEUED) {
                            // Start recording at quantize boundary
                            startRecording(r, c);
                        }
                    }
                }

                // Process queued record stop at quantize boundary
                if (queuedRecordStop && recordingRow >= 0) {
                    stopRecording();
                    queuedRecordStop = false;
                }

                // Process queued scene triggers at quantize boundary
                for (int i = 0; i < 8; i++) {
                    if (queuedScenes[i]) {
                        triggerScene(i);
                        queuedScenes[i] = false;
                    }
                }

                // Process queued stop all at quantize boundary
                if (queuedStopAll) {
                    stopAll();
                    queuedStopAll = false;
                }
            }
        }
        samplesSinceLastClock++;

        // Process stop all button (immediate or queue based on quantize)
        if (stopAllTrigger.process(params[STOP_ALL_PARAM].getValue())) {
            stopAll();
        }

        // Process stop all input trigger (respects quantize)
        if (stopAllInputTrigger.process(inputs[STOP_ALL_TRIG_INPUT].getVoltage(), 0.1f, 1.f)) {
            int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
            if (quantize == 0) {
                stopAll();
            } else {
                queuedStopAll = true;
            }
        }

        // Process scene button triggers (immediate)
        for (int i = 0; i < 8; i++) {
            if (sceneTriggers[i].process(params[SCENE_1_PARAM + i].getValue())) {
                triggerScene(i);
            }
        }

        // Process scene input triggers (respects quantize)
        for (int i = 0; i < 8; i++) {
            if (sceneInputTriggers[i].process(inputs[SCENE_1_TRIG_INPUT + i].getVoltage(), 0.1f, 1.f)) {
                int quantize = quantizeValues[(int)params[QUANTIZE_PARAM].getValue()];
                if (quantize == 0) {
                    triggerScene(i);
                } else {
                    queuedScenes[i] = true;
                }
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

        // Process pending fade-out stops
        processPendingStops();

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
                    // Get sample with loop crossfade
                    float sample = cell.buffer[cell.playPosition];

                    // Apply crossfade at loop boundaries
                    int samplesFromEnd = cell.recordedLength - cell.playPosition;
                    if (samplesFromEnd <= FADE_SAMPLES && cell.recordedLength > FADE_SAMPLES * 2) {
                        // Approaching end: crossfade with beginning
                        float fadeOut = (float)samplesFromEnd / FADE_SAMPLES;
                        float fadeIn = 1.0f - fadeOut;
                        int crossfadePos = FADE_SAMPLES - samplesFromEnd;
                        if (crossfadePos >= 0 && crossfadePos < cell.recordedLength) {
                            sample = sample * fadeOut + cell.buffer[crossfadePos] * fadeIn;
                        }
                    }

                    // Apply fade envelope (for start/stop fades)
                    float fadeGain = cell.processFade();
                    rowOutput = sample * fadeGain;

                    // Advance play position
                    cell.playPosition++;
                    if (cell.playPosition >= cell.recordedLength) {
                        // Loop - skip samples already played during crossfade
                        if (cell.recordedLength > FADE_SAMPLES * 2) {
                            cell.playPosition = FADE_SAMPLES;
                        } else {
                            cell.playPosition = 0;
                        }
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
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));

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
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) {
            panelContrast = json_real_value(contrastJ);
        }

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
    // Must consume left-click to enable drag system
    if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
        e.consume(this);
    }
}

void CellWidget::onDragStart(const event::DragStart& e) {
    if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;

    pressed = true;
    pressTime = 0.f;
    dragSource = this;
    dragOffset = Vec(0, 0);
    dragCopyMode = (APP->window->getMods() & GLFW_MOD_SHIFT);
}

void CellWidget::onDragMove(const event::DragMove& e) {
    // Accumulate mouse movement
    dragOffset = dragOffset.plus(e.mouseDelta);

    // Pre-calculate target cell (only once per move, not per cell per frame)
    if (dragSource) {
        const float cellSpacingX = 44.f;
        const float cellSpacingY = 28.f;
        int deltaCol = (int)std::round(dragOffset.x / cellSpacingX);
        int deltaRow = (int)std::round(dragOffset.y / cellSpacingY);
        targetRow = clamp(dragSource->row + deltaRow, 0, 7);
        targetCol = clamp(dragSource->col + deltaCol, 0, 7);
    }
}

void CellWidget::onDragEnd(const event::DragEnd& e) {
    if (!module) {
        dragSource = nullptr;
        targetRow = -1;
        targetCol = -1;
        pressed = false;
        return;
    }

    // Check if we moved to a different valid cell (use pre-calculated target)
    bool movedToOtherCell = (targetRow != row || targetCol != col) &&
                            targetRow >= 0 && targetRow < 8 &&
                            targetCol >= 0 && targetCol < 8;

    if (movedToOtherCell && module->cells[row][col].state != CELL_EMPTY) {
        // Drag to another cell - move or copy
        bool copyMode = (APP->window->getMods() & GLFW_MOD_SHIFT);
        if (copyMode) {
            module->copyCell(row, col, targetRow, targetCol);
        } else {
            module->moveCell(row, col, targetRow, targetCol);
        }
    } else {
        // Click or hold on same cell
        if (pressTime >= HOLD_TIME) {
            module->onCellHold(row, col);
        } else {
            module->onCellClick(row, col);
        }
    }

    // Reset state
    dragSource = nullptr;
    dragCopyMode = false;
    targetRow = -1;
    targetCol = -1;
    pressed = false;
}

void CellWidget::draw(const DrawArgs& args) {
    // Update press time (using frame time ~60fps)
    if (pressed) {
        pressTime += 1.f / 60.f;
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
        case CELL_RECORD_QUEUED: bgColor = LaunchpadColors::RECORD_QUEUED; break;
        default: bgColor = LaunchpadColors::EMPTY; break;
    }

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

    // Draw waveform only when playing (performance optimization)
    if (module && (state == CELL_PLAYING || state == CELL_STOP_QUEUED)) {
        drawWaveform(args, module->cells[row][col]);
    }

    // Draw loop length indicator (using cached string)
    if (module && module->cells[row][col].loopClocks > 0) {
        const std::string& loopStr = module->cells[row][col].getLoopClocksStr();
        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 180));
        nvgText(args.vg, w - 3, h - 2, loopStr.c_str(), NULL);
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

    // Drag visual feedback (use pre-calculated targetRow/targetCol)
    if (dragSource && module) {
        if (dragSource == this && module->cells[row][col].state != CELL_EMPTY) {
            // Source cell: yellow border
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, x + 1, y + 1, w - 2, h - 2, 2);
            nvgStrokeColor(args.vg, nvgRGB(255, 255, 0));
            nvgStrokeWidth(args.vg, 2.0f);
            nvgStroke(args.vg);

            // Copy mode indicator "+"
            if (APP->window->getMods() & GLFW_MOD_SHIFT) {
                nvgFontSize(args.vg, 14);
                nvgFontFaceId(args.vg, APP->window->uiFont->handle);
                nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgFillColor(args.vg, nvgRGB(255, 255, 0));
                nvgText(args.vg, 3, 1, "+", NULL);
            }
        }

        // Drop target: green border (use pre-calculated target, simple comparison)
        if (row == targetRow && col == targetCol &&
            !(dragSource->row == row && dragSource->col == col) &&
            dragSource->module->cells[dragSource->row][dragSource->col].state != CELL_EMPTY) {
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, x + 1, y + 1, w - 2, h - 2, 2);
            nvgStrokeColor(args.vg, nvgRGB(0, 255, 128));
            nvgStrokeWidth(args.vg, 3.0f);
            nvgStroke(args.vg);
        }
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

    // Find max amplitude for auto-scaling
    float maxAmp = 0.001f;  // Minimum to avoid division by zero
    for (int i = 0; i < displayWidth; i++) {
        float absVal = std::abs(cell.waveformCache[i]);
        if (absVal > maxAmp) maxAmp = absVal;
    }

    // Draw actual waveform as connected line (auto-scaled)
    nvgBeginPath(args.vg);
    bool first = true;
    for (int i = 0; i < displayWidth; i++) {
        float voltage = cell.waveformCache[i];
        float y = centerY - (voltage / maxAmp) * maxHeight;  // Auto-scaled
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
        panelThemeHelper.init(this, "40HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(40 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // White bottom panel
        WhiteBottomPanel40HP* whitePanel = new WhiteBottomPanel40HP();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // Title labels (2x size compared to other modules)
        NVGcolor titleColor = nvgRGB(255, 200, 0);  // Yellow
        addChild(new LaunchpadLabel(Vec(0, 1), Vec(200, 30), "LAUNCHPAD", 24.f, titleColor, true));
        addChild(new LaunchpadLabel(Vec(160, 9), Vec(100, 20), "MADZINE", 20.f, titleColor, false));

        // Clock, Reset, Quantize - upper right (with labels)
        addChild(new LaunchpadLabel(Vec(475 - 25, 25), Vec(50, 12), "Clock", 8.f));
        addInput(createInputCentered<PJ301MPort>(Vec(475, 50), module, Launchpad::CLOCK_INPUT));
        addChild(new LaunchpadLabel(Vec(520 - 25, 25), Vec(50, 12), "Reset", 8.f));
        addInput(createInputCentered<PJ301MPort>(Vec(520, 50), module, Launchpad::RESET_INPUT));
        addChild(new LaunchpadLabel(Vec(565 - 30, 25), Vec(60, 12), "Quantize", 8.f));
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(565, 50), module, Launchpad::QUANTIZE_PARAM));

        // Scene trigger inputs (above scene buttons)
        float cellStartX = 70;
        float cellSpacing = 44;
        for (int i = 0; i < 8; i++) {
            float x = cellStartX + i * cellSpacing;
            addInput(createInputCentered<PJ301MPort>(Vec(x, 48), module, Launchpad::SCENE_1_TRIG_INPUT + i));
        }

        // Scene buttons (aligned with cells)
        for (int i = 0; i < 8; i++) {
            float x = cellStartX + i * cellSpacing;
            addParam(createParamCentered<VCVButton>(Vec(x, 75), module, Launchpad::SCENE_1_PARAM + i));
        }

        // Column headers for row controls (X = knob center - labelWidth/2)
        addChild(new LaunchpadLabel(Vec(430 - 18, 79), Vec(36, 12), "Send A", 8.f));   // knob at 430
        addChild(new LaunchpadLabel(Vec(468 - 18, 79), Vec(36, 12), "Send B", 8.f));   // knob at 468
        addChild(new LaunchpadLabel(Vec(506 - 12, 79), Vec(24, 12), "Pan", 8.f));      // knob at 506
        addChild(new LaunchpadLabel(Vec(544 - 15, 79), Vec(30, 12), "Level", 8.f));    // knob at 544
        addChild(new LaunchpadLabel(Vec(577 - 12, 79), Vec(24, 12), "Out", 8.f));      // port at 577

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

        // Bottom section (Y=330+) with labels (pink color)
        NVGcolor pinkText = nvgRGB(232, 112, 112);  // Sashimi pink

        // Stop All (left side of bottom panel)
        addChild(new LaunchpadLabel(Vec(30 - 20, 332), Vec(40, 12), "Stop All", 8.f, pinkText));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 355), module, Launchpad::STOP_ALL_TRIG_INPUT));
        addParam(createParamCentered<TL1105>(Vec(45, 355), module, Launchpad::STOP_ALL_PARAM));

        // Send A
        addChild(new LaunchpadLabel(Vec(95 - 30, 332), Vec(60, 12), "Send A", 9.f, pinkText));
        addOutput(createOutputCentered<PJ301MPort>(Vec(80, 355), module, Launchpad::SEND_A_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(110, 355), module, Launchpad::SEND_A_R_OUTPUT));

        // Return A
        addChild(new LaunchpadLabel(Vec(170 - 35, 332), Vec(70, 12), "Return A", 9.f, pinkText));
        addInput(createInputCentered<PJ301MPort>(Vec(155, 355), module, Launchpad::RETURN_A_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(185, 355), module, Launchpad::RETURN_A_R_INPUT));

        // Send B
        addChild(new LaunchpadLabel(Vec(260 - 30, 332), Vec(60, 12), "Send B", 9.f, pinkText));
        addOutput(createOutputCentered<PJ301MPort>(Vec(245, 355), module, Launchpad::SEND_B_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(275, 355), module, Launchpad::SEND_B_R_OUTPUT));

        // Return B
        addChild(new LaunchpadLabel(Vec(345 - 35, 332), Vec(70, 12), "Return B", 9.f, pinkText));
        addInput(createInputCentered<PJ301MPort>(Vec(330, 355), module, Launchpad::RETURN_B_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(360, 355), module, Launchpad::RETURN_B_R_INPUT));

        // Mix
        addChild(new LaunchpadLabel(Vec(550 - 20, 332), Vec(40, 12), "Mix", 9.f, pinkText));
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
