#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <chrono>

// Enhanced text label (like MADDY+)
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
        }
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, text.c_str(), NULL);
    }
};

// Waveform names for display
static const char* WAVEFORM_NAMES[] = {"Attack", "Triangle", "Decay", "Sine", "Square"};

struct WaveformParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        float v = getValue();
        int index = static_cast<int>(std::round(v));
        index = clamp(index, 0, 4);
        return WAVEFORM_NAMES[index];
    }
};

struct Runshow : Module {
    int panelTheme = -1; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        RESET_PARAM,
        START_STOP_PARAM,
        TIMER_30MIN_PARAM,      // 30分鐘計時器間隔控制
        TIMER_15MIN_PARAM,      // 15分鐘計時器間隔控制
        BAR_1_PARAM,            // 第1小節控制
        BAR_2_PARAM,            // 第2小節控制
        BAR_3_PARAM,            // 第3小節控制
        BAR_4_PARAM,            // 第4小節控制
        PARAMS_LEN
    };
    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        START_STOP_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        TIMER_30MIN_OUTPUT,     // 30分鐘計時器觸發 (每5分鐘)
        TIMER_15MIN_OUTPUT,     // 15分鐘計時器觸發 (每1分鐘)
        BAR_1_OUTPUT,           // 第1小節觸發
        BAR_2_OUTPUT,           // 第2小節觸發
        BAR_3_OUTPUT,           // 第3小節觸發
        BAR_4_OUTPUT,           // 第4小節觸發
        OUTPUTS_LEN
    };
    enum LightId {
        BEAT_LIGHT,
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger startStopTrigger;
    dsp::SchmittTrigger resetButtonTrigger;
    dsp::SchmittTrigger startStopButtonTrigger;

    bool running = false;
    int clockCount = 0;
    int currentBar = 0;
    int quarter_notes = 0;  // 4分音符計數
    int eighth_notes = 0;   // 8分音符計數
    int sixteenth_notes = 0; // 16分音符計數

    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    float elapsedSeconds = 0.f;

    // Waveform generators for timer outputs
    float timer30MinPhase = 0.f;
    float timer15MinPhase = 0.f;
    bool timer30MinActive = false;
    bool timer15MinActive = false;
    float timer30MinDuration = 0.f;
    float timer15MinDuration = 0.f;

    // Waveform generators for bar outputs
    float bar1Phase = 0.f;
    float bar2Phase = 0.f;
    float bar3Phase = 0.f;
    float bar4Phase = 0.f;
    bool bar1Active = false;
    bool bar2Active = false;
    bool bar3Active = false;
    bool bar4Active = false;
    float bar1Duration = 0.f;
    float bar2Duration = 0.f;
    float bar3Duration = 0.f;
    float bar4Duration = 0.f;

    int lastBarInCycle = -1;  // Track bar transitions
    float lastClockTime = 0.f;  // Track timing between clocks
    float clockInterval = 0.1f; // Time between clocks in seconds (default 100ms)

    // Waveform generation function with smooth morphing
    float generateWaveform(float phase, float morph) {
        // Clamp morph to 0.0-4.0 range
        morph = std::max(0.f, std::min(4.f, morph));

        // Generate all base waveforms (all start at 0V, end at 0V)
        // Attack: 慢速上升，垂直下降
        float attack = phase < 0.99f
            ? (phase / 0.99f) * 10.f
            : 0.f;

        // Triangle: 對稱三角波
        float triangle = phase < 0.5f
            ? phase * 20.f
            : (1.f - phase) * 20.f;

        // Decay: 垂直上升，慢速下降
        float decay = phase < 0.01f
            ? 10.f
            : ((1.f - phase) / 0.99f) * 10.f;

        // Sine: 半波正弦
        float sine = std::sin(phase * M_PI) * 10.f;

        // Square: 垂直上升，保持，垂直下降
        float square = (phase > 0.f && phase < 1.f) ? 10.f : 0.f;

        // Smooth morphing between adjacent waveforms
        if (morph < 1.f) {
            // Morph between Attack and Triangle
            return attack * (1.f - morph) + triangle * morph;
        } else if (morph < 2.f) {
            // Morph between Triangle and Decay
            float blend = morph - 1.f;
            return triangle * (1.f - blend) + decay * blend;
        } else if (morph < 3.f) {
            // Morph between Decay and Sine
            float blend = morph - 2.f;
            return decay * (1.f - blend) + sine * blend;
        } else {
            // Morph between Sine and Square
            float blend = morph - 3.f;
            return sine * (1.f - blend) + square * blend;
        }
    }

    Runshow() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");
        configParam(START_STOP_PARAM, 0.f, 1.f, 0.f, "Start/Stop");

        // 配置 6 個新旋鈕參數
        configParam(TIMER_30MIN_PARAM, 1.f, 99.f, 10.f, "Pulse Width (Bar %)", " %");
        configParam(TIMER_15MIN_PARAM, 0.f, 4.f, 4.f, "Waveform");
        // 使用自定義 ParamQuantity 顯示波形名稱
        delete paramQuantities[TIMER_15MIN_PARAM];
        paramQuantities[TIMER_15MIN_PARAM] = new WaveformParamQuantity;
        paramQuantities[TIMER_15MIN_PARAM]->module = this;
        paramQuantities[TIMER_15MIN_PARAM]->paramId = TIMER_15MIN_PARAM;
        paramQuantities[TIMER_15MIN_PARAM]->minValue = 0.0f;
        paramQuantities[TIMER_15MIN_PARAM]->maxValue = 4.0f;
        paramQuantities[TIMER_15MIN_PARAM]->defaultValue = 4.0f;
        paramQuantities[TIMER_15MIN_PARAM]->name = "Waveform";
        paramQuantities[TIMER_15MIN_PARAM]->snapEnabled = true;

        configParam(BAR_1_PARAM, 1.f, 16.f, 16.f, "Bar 1 Length", " clocks");
        configParam(BAR_2_PARAM, 1.f, 16.f, 16.f, "Bar 2 Length", " clocks");
        configParam(BAR_3_PARAM, 1.f, 16.f, 16.f, "Bar 3 Length", " clocks");
        configParam(BAR_4_PARAM, 1.f, 16.f, 16.f, "Bar 4 Length", " clocks");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(START_STOP_INPUT, "Start/Stop");

        configOutput(TIMER_30MIN_OUTPUT, "5Min Timer");
        configOutput(TIMER_15MIN_OUTPUT, "1Min Timer");
        configOutput(BAR_1_OUTPUT, "Bar 1 Timer");
        configOutput(BAR_2_OUTPUT, "Bar 2 Timer");
        configOutput(BAR_3_OUTPUT, "Bar 3 Timer");
        configOutput(BAR_4_OUTPUT, "Bar 4 Timer");

        configLight(BEAT_LIGHT, "Beat");
    }

    void process(const ProcessArgs& args) override {
        bool resetTriggered = resetTrigger.process(inputs[RESET_INPUT].getVoltage()) ||
                              resetButtonTrigger.process(params[RESET_PARAM].getValue());

        bool startStopTriggered = startStopTrigger.process(inputs[START_STOP_INPUT].getVoltage()) ||
                                  startStopButtonTrigger.process(params[START_STOP_PARAM].getValue());

        if (startStopTriggered) {
            running = !running;
            if (running) {
                startTime = std::chrono::steady_clock::now();
                lastUpdateTime = startTime;
            }
        }

        if (resetTriggered) {
            clockCount = 0;
            currentBar = 0;
            quarter_notes = 0;
            eighth_notes = 0;
            sixteenth_notes = 0;
            elapsedSeconds = 0.f;
            startTime = std::chrono::steady_clock::now();
            lastUpdateTime = startTime;
            lastBarInCycle = -1;

            lights[BEAT_LIGHT].setBrightness(0.f);

            // Trigger only timer outputs (5min and 1min) when reset
            float pulseWidthPercent = params[TIMER_30MIN_PARAM].getValue();

            // Trigger 5min timer
            float timer30MinFullDuration = 5.f * 60.f;
            timer30MinDuration = (pulseWidthPercent / 100.0f) * timer30MinFullDuration;
            timer30MinActive = true;
            timer30MinPhase = 0.f;

            // Trigger 1min timer
            float timer15MinFullDuration = 1.f * 60.f;
            timer15MinDuration = (pulseWidthPercent / 100.0f) * timer15MinFullDuration;
            timer15MinActive = true;
            timer15MinPhase = 0.f;

            // Reset bar states (bars are triggered by clock, not reset)
            bar1Active = false;
            bar2Active = false;
            bar3Active = false;
            bar4Active = false;
            bar1Phase = 0.f;
            bar2Phase = 0.f;
            bar3Phase = 0.f;
            bar4Phase = 0.f;
        }

        if (running) {
            auto currentTime = std::chrono::steady_clock::now();
            elapsedSeconds = std::chrono::duration<float>(currentTime - startTime).count();

            if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
                // Track time between clocks for pulse width calculation
                float currentTime = elapsedSeconds;
                if (lastClockTime > 0.f && currentTime > lastClockTime) {
                    clockInterval = currentTime - lastClockTime;
                }
                lastClockTime = currentTime;

                clockCount++;

                // 假設每個 clock 是 16 分音符
                sixteenth_notes++;

                // 每 2 個 16 分音符 = 1 個 8 分音符
                if (clockCount % 2 == 0) {
                    eighth_notes++;
                }

                // 每 4 個 16 分音符 = 1 個 4 分音符 (節拍)
                if (clockCount % 4 == 0) {
                    quarter_notes++;
                    // Beat light flash
                    lights[BEAT_LIGHT].setBrightness(1.f);
                }

                // Progress is now handled by VerticalProgressBar widget, no lights needed

                // Calculate bar transitions based on variable lengths
                float bar0Clocks = params[BAR_1_PARAM].getValue();
                float bar1Clocks = params[BAR_2_PARAM].getValue();
                float bar2Clocks = params[BAR_3_PARAM].getValue();
                float bar3Clocks = params[BAR_4_PARAM].getValue();
                float totalCycleClocks = bar0Clocks + bar1Clocks + bar2Clocks + bar3Clocks;

                int clocksInCycle = clockCount % (int)totalCycleClocks;
                int currentBarInCycle = 0;

                // Find which bar we're currently in
                if (clocksInCycle < bar0Clocks) {
                    currentBarInCycle = 0;
                } else if (clocksInCycle < bar0Clocks + bar1Clocks) {
                    currentBarInCycle = 1;
                } else if (clocksInCycle < bar0Clocks + bar1Clocks + bar2Clocks) {
                    currentBarInCycle = 2;
                } else {
                    currentBarInCycle = 3;
                }

                // Trigger waveform when transitioning to a new bar
                if (currentBarInCycle != lastBarInCycle) {
                    float pulseWidthPercent = params[TIMER_30MIN_PARAM].getValue();

                    // Reset all bar states
                    bar1Active = false;
                    bar2Active = false;
                    bar3Active = false;
                    bar4Active = false;

                    // Activate current bar and set duration
                    switch(currentBarInCycle) {
                        case 0: {
                            float barDuration = bar0Clocks * clockInterval;
                            bar1Duration = (pulseWidthPercent / 100.0f) * barDuration;
                            bar1Active = true;
                            bar1Phase = 0.f;
                            break;
                        }
                        case 1: {
                            float barDuration = bar1Clocks * clockInterval;
                            bar2Duration = (pulseWidthPercent / 100.0f) * barDuration;
                            bar2Active = true;
                            bar2Phase = 0.f;
                            break;
                        }
                        case 2: {
                            float barDuration = bar2Clocks * clockInterval;
                            bar3Duration = (pulseWidthPercent / 100.0f) * barDuration;
                            bar3Active = true;
                            bar3Phase = 0.f;
                            break;
                        }
                        case 3: {
                            float barDuration = bar3Clocks * clockInterval;
                            bar4Duration = (pulseWidthPercent / 100.0f) * barDuration;
                            bar4Active = true;
                            bar4Phase = 0.f;
                            break;
                        }
                    }
                    lastBarInCycle = currentBarInCycle;
                    currentBar++;
                }
            }

            // 時間計時器觸發邏輯
            static float lastTimer30Min = 0.f;
            static float lastTimer15Min = 0.f;

            float pulseWidthPercent = params[TIMER_30MIN_PARAM].getValue();

            // 30分鐘計時器：每5分鐘觸發波形
            float timer30MinInterval = 5.f * 60.f; // 5分鐘
            if (elapsedSeconds >= lastTimer30Min + timer30MinInterval && elapsedSeconds < 30.f * 60.f) {
                float timer30MinFullDuration = 5.f * 60.f; // 5分鐘總時長
                timer30MinDuration = (pulseWidthPercent / 100.0f) * timer30MinFullDuration;
                timer30MinActive = true;
                timer30MinPhase = 0.f;
                lastTimer30Min += timer30MinInterval;
            }

            // 15分鐘計時器：每1分鐘觸發波形
            float timer15MinInterval = 1.f * 60.f; // 1分鐘
            if (elapsedSeconds >= lastTimer15Min + timer15MinInterval && elapsedSeconds < 15.f * 60.f) {
                float timer15MinFullDuration = 1.f * 60.f; // 1分鐘總時長
                timer15MinDuration = (pulseWidthPercent / 100.0f) * timer15MinFullDuration;
                timer15MinActive = true;
                timer15MinPhase = 0.f;
                lastTimer15Min += timer15MinInterval;
            }

            // Reset timer counters when reset is triggered (inside running block)
            if (resetTriggered) {
                lastTimer30Min = 0.f;
                lastTimer15Min = 0.f;
                lastClockTime = 0.f; // Reset clock timing
            }
        }

        // Update waveform phases (outside running block so reset triggers work)
        // Update timer waveforms
        if (timer30MinActive && timer30MinDuration > 0.f) {
            timer30MinPhase += args.sampleTime / timer30MinDuration;
            if (timer30MinPhase >= 1.f) {
                timer30MinActive = false;
                timer30MinPhase = 0.f;
            }
        }
        if (timer15MinActive && timer15MinDuration > 0.f) {
            timer15MinPhase += args.sampleTime / timer15MinDuration;
            if (timer15MinPhase >= 1.f) {
                timer15MinActive = false;
                timer15MinPhase = 0.f;
            }
        }

        // Update bar waveforms
        if (bar1Active && bar1Duration > 0.f) {
            bar1Phase += args.sampleTime / bar1Duration;
            if (bar1Phase >= 1.f) {
                bar1Active = false;
                bar1Phase = 0.f;
            }
        }
        if (bar2Active && bar2Duration > 0.f) {
            bar2Phase += args.sampleTime / bar2Duration;
            if (bar2Phase >= 1.f) {
                bar2Active = false;
                bar2Phase = 0.f;
            }
        }
        if (bar3Active && bar3Duration > 0.f) {
            bar3Phase += args.sampleTime / bar3Duration;
            if (bar3Phase >= 1.f) {
                bar3Active = false;
                bar3Phase = 0.f;
            }
        }
        if (bar4Active && bar4Duration > 0.f) {
            bar4Phase += args.sampleTime / bar4Duration;
            if (bar4Phase >= 1.f) {
                bar4Active = false;
                bar4Phase = 0.f;
            }
        }

        // 設定所有輸出
        float waveShape = params[TIMER_15MIN_PARAM].getValue();

        // Generate morphed waveforms for timer outputs
        outputs[TIMER_30MIN_OUTPUT].setVoltage(timer30MinActive ? generateWaveform(timer30MinPhase, waveShape) : 0.f);
        outputs[TIMER_15MIN_OUTPUT].setVoltage(timer15MinActive ? generateWaveform(timer15MinPhase, waveShape) : 0.f);

        // Generate morphed waveforms for bar outputs
        outputs[BAR_1_OUTPUT].setVoltage(bar1Active ? generateWaveform(bar1Phase, waveShape) : 0.f);
        outputs[BAR_2_OUTPUT].setVoltage(bar2Active ? generateWaveform(bar2Phase, waveShape) : 0.f);
        outputs[BAR_3_OUTPUT].setVoltage(bar3Active ? generateWaveform(bar3Phase, waveShape) : 0.f);
        outputs[BAR_4_OUTPUT].setVoltage(bar4Active ? generateWaveform(bar4Phase, waveShape) : 0.f);

        // Beat light decay
        float lightDecay = 5.f * args.sampleTime;
        lights[BEAT_LIGHT].setBrightness(std::max(0.f, lights[BEAT_LIGHT].getBrightness() - lightDecay * 3.f));
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "running", json_boolean(running));
        json_object_set_new(rootJ, "clockCount", json_integer(clockCount));
        json_object_set_new(rootJ, "currentBar", json_integer(currentBar));
        json_object_set_new(rootJ, "quarter_notes", json_integer(quarter_notes));
        json_object_set_new(rootJ, "eighth_notes", json_integer(eighth_notes));
        json_object_set_new(rootJ, "sixteenth_notes", json_integer(sixteenth_notes));
        json_object_set_new(rootJ, "elapsedSeconds", json_real(elapsedSeconds));
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* runningJ = json_object_get(rootJ, "running");
        if (runningJ)
            running = json_boolean_value(runningJ);

        json_t* clockCountJ = json_object_get(rootJ, "clockCount");
        if (clockCountJ)
            clockCount = json_integer_value(clockCountJ);

        json_t* currentBarJ = json_object_get(rootJ, "currentBar");
        if (currentBarJ)
            currentBar = json_integer_value(currentBarJ);

        json_t* quarter_notesJ = json_object_get(rootJ, "quarter_notes");
        if (quarter_notesJ)
            quarter_notes = json_integer_value(quarter_notesJ);

        json_t* eighth_notesJ = json_object_get(rootJ, "eighth_notes");
        if (eighth_notesJ)
            eighth_notes = json_integer_value(eighth_notesJ);

        json_t* sixteenth_notesJ = json_object_get(rootJ, "sixteenth_notes");
        if (sixteenth_notesJ)
            sixteenth_notes = json_integer_value(sixteenth_notesJ);

        json_t* elapsedSecondsJ = json_object_get(rootJ, "elapsedSeconds");
        if (elapsedSecondsJ)
            elapsedSeconds = json_real_value(elapsedSecondsJ);

        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
    }
};

// Time Code Display Widget
struct TimeCodeDisplay : LedDisplay {
    Runshow* module;
    std::string timeString = "00:00:00";
    std::string barString = "001:1:1";

    TimeCodeDisplay() {
        box.size = Vec(70, 40);
    }

    void draw(const DrawArgs& args) override {
        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGB(60, 60, 60));
        nvgStrokeWidth(args.vg, 1.0);
        nvgStroke(args.vg);

        if (!module) return;

        // Update time string
        int minutes = (int)(module->elapsedSeconds / 60.0f) % 1000; // Limit to 0-999 (百位)
        int seconds = (int)module->elapsedSeconds % 60;
        int milliseconds = (int)((module->elapsedSeconds - floor(module->elapsedSeconds)) * 100);
        timeString = string::f("%d:%02d:%02d", minutes, seconds, milliseconds);

        // Update bar string (Bar:Beat:Tick)
        int bar = module->currentBar + 1;  // 1-based display
        int beat = (module->clockCount / 4) % 4 + 1;  // 1-4
        int tick = module->clockCount % 4 + 1;  // 1-4
        barString = string::f("%03d:%d:%d", bar, beat, tick);

        // Draw time code
        nvgFontSize(args.vg, 14);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        // Time display
        nvgFillColor(args.vg, nvgRGB(0, 255, 100));
        nvgText(args.vg, box.size.x / 2, 12, timeString.c_str(), NULL);

        // Bar:Beat:Tick display
        nvgFillColor(args.vg, nvgRGB(255, 200, 0));
        nvgText(args.vg, box.size.x / 2, 28, barString.c_str(), NULL);
    }

    void step() override {
        LedDisplay::step();
    }
};

// Six Vertical Progress Bars Widget (LedDisplay-based for Meta Module compatibility)
struct FourProgressBars : LedDisplay {
    Runshow* module;

    FourProgressBars() {
        box.size = Vec(150, 200);  // Wider to accommodate 6 bars
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;

        // Background for all 6 bars
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        if (!module) return;

        // Calculate current position in 4-bar cycle
        int currentStep = module->clockCount % 64;  // 64 steps total (16 per bar)
        int currentBar = currentStep / 16;  // Which bar we're in (0-3)
        float currentBarProgress = (currentStep % 16) / 16.0f;  // Progress within current bar

        // Draw 6 separate progress bars (2 extra on left, 4 main bars on right)
        float barWidth = 20;  // Width of each bar
        float barSpacing = 4;  // Space between bars
        float totalWidth = 6 * barWidth + 5 * barSpacing;
        float startX = (box.size.x - totalWidth) / 2;  // Center the bars

        for (int i = 0; i < 6; i++) {
            float x = startX + i * (barWidth + barSpacing);

            // Draw bar background/border
            nvgBeginPath(args.vg);
            nvgRect(args.vg, x, 0, barWidth, box.size.y);
            nvgStrokeColor(args.vg, nvgRGB(60, 60, 60));
            nvgStrokeWidth(args.vg, 1.0);
            nvgStroke(args.vg);

            // Determine fill height for this bar
            float fillHeight = 0;

            if (i == 0) {
                // First bar: 5-minute timer (30 minutes total)
                float totalMinutes = module->elapsedSeconds / 60.0f;
                float fiveMinuteBlocks = totalMinutes / 5.0f;
                fillHeight = box.size.y * std::min(fiveMinuteBlocks / 6.0f, 1.0f);
            } else if (i == 1) {
                // Second bar: 1-minute timer (15 minutes total)
                float totalMinutes = module->elapsedSeconds / 60.0f;
                fillHeight = box.size.y * std::min(totalMinutes / 15.0f, 1.0f);
            } else {
                // Bars 2-5 represent the 4-bar cycle, each with adjustable length
                int barIndex = i - 2;  // Map to 0-3 for the 4 main bars

                // Get the clock lengths for all bars
                float bar0Clocks = module->params[Runshow::BAR_1_PARAM].getValue();
                float bar1Clocks = module->params[Runshow::BAR_2_PARAM].getValue();
                float bar2Clocks = module->params[Runshow::BAR_3_PARAM].getValue();
                float bar3Clocks = module->params[Runshow::BAR_4_PARAM].getValue();

                // Calculate total cycle length
                float totalCycleClocks = bar0Clocks + bar1Clocks + bar2Clocks + bar3Clocks;

                // Get current position in the cycle
                int clocksInCycle = module->clockCount % (int)totalCycleClocks;

                // Calculate which bar we're in and the progress within it
                int currentBarInCycle = 0;
                int clocksInCurrentBar = clocksInCycle;

                // Find which bar we're currently in by accumulating clock counts
                if (clocksInCycle < bar0Clocks) {
                    currentBarInCycle = 0;
                    clocksInCurrentBar = clocksInCycle;
                } else if (clocksInCycle < bar0Clocks + bar1Clocks) {
                    currentBarInCycle = 1;
                    clocksInCurrentBar = clocksInCycle - bar0Clocks;
                } else if (clocksInCycle < bar0Clocks + bar1Clocks + bar2Clocks) {
                    currentBarInCycle = 2;
                    clocksInCurrentBar = clocksInCycle - bar0Clocks - bar1Clocks;
                } else {
                    currentBarInCycle = 3;
                    clocksInCurrentBar = clocksInCycle - bar0Clocks - bar1Clocks - bar2Clocks;
                }

                // Get the clock length for this specific bar
                float barClocks = 16.0f; // default
                switch(barIndex) {
                    case 0: barClocks = bar0Clocks; break;
                    case 1: barClocks = bar1Clocks; break;
                    case 2: barClocks = bar2Clocks; break;
                    case 3: barClocks = bar3Clocks; break;
                }

                if (barIndex == currentBarInCycle) {
                    // This is the active bar: show progress
                    float progress = (float)clocksInCurrentBar / barClocks;
                    fillHeight = box.size.y * progress * (barClocks / 16.0f);
                } else if (barIndex < currentBarInCycle ||
                          (currentBarInCycle == 0 && barIndex > 0 && clocksInCycle >= totalCycleClocks - 1)) {
                    // This bar was completed in this cycle
                    fillHeight = box.size.y * (barClocks / 16.0f);
                } else {
                    // This bar hasn't started yet in this cycle
                    fillHeight = 0;
                }
            }

            // Draw the fill with gaps at division lines
            if (fillHeight > 0) {
                float gapSize = 1.0f; // Size of gap for division lines

                if (i == 0) {
                    // First bar: 5-minute timer (6 divisions for 30 minutes total)
                    float segHeight = box.size.y / 6.0f;
                    for (int seg = 0; seg < 6; seg++) {
                        float segTop = box.size.y - ((seg + 1) * segHeight);
                        float segBottom = box.size.y - (seg * segHeight);

                        // Leave gap at division line (except at top)
                        if (seg > 0) segBottom -= gapSize / 2;
                        if (seg < 5) segTop += gapSize / 2;

                        // Only draw if this segment intersects with the fill
                        float fillTop = box.size.y - fillHeight;
                        if (fillTop < segBottom && segBottom > fillTop) {
                            float actualTop = std::max(fillTop, segTop);
                            float actualBottom = segBottom;

                            if (actualTop < actualBottom) {
                                nvgBeginPath(args.vg);
                                nvgRect(args.vg, x + 1, actualTop, barWidth - 2, actualBottom - actualTop);
                                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                                nvgFill(args.vg);
                            }
                        }
                    }
                } else if (i == 1) {
                    // Second bar: 1-minute timer (15 divisions for 15 minutes total)
                    float segHeight = box.size.y / 15.0f;
                    for (int seg = 0; seg < 15; seg++) {
                        float segTop = box.size.y - ((seg + 1) * segHeight);
                        float segBottom = box.size.y - (seg * segHeight);

                        // Leave gap at division line (except at top)
                        if (seg > 0) segBottom -= gapSize / 2;
                        if (seg < 14) segTop += gapSize / 2;

                        // Only draw if this segment intersects with the fill
                        float fillTop = box.size.y - fillHeight;
                        if (fillTop < segBottom && segBottom > fillTop) {
                            float actualTop = std::max(fillTop, segTop);
                            float actualBottom = segBottom;

                            if (actualTop < actualBottom) {
                                nvgBeginPath(args.vg);
                                nvgRect(args.vg, x + 1, actualTop, barWidth - 2, actualBottom - actualTop);
                                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                                nvgFill(args.vg);
                            }
                        }
                    }
                } else {
                    // Bars 2-5: 4 beat divisions
                    float segHeight = box.size.y / 4.0f;
                    for (int seg = 0; seg < 4; seg++) {
                        float segTop = box.size.y - ((seg + 1) * segHeight);
                        float segBottom = box.size.y - (seg * segHeight);

                        // Leave gap at division line (except at top)
                        if (seg > 0) segBottom -= gapSize / 2;
                        if (seg < 3) segTop += gapSize / 2;

                        // Only draw if this segment intersects with the fill
                        float fillTop = box.size.y - fillHeight;
                        if (fillTop < segBottom && segBottom > fillTop) {
                            float actualTop = std::max(fillTop, segTop);
                            float actualBottom = segBottom;

                            if (actualTop < actualBottom) {
                                nvgBeginPath(args.vg);
                                nvgRect(args.vg, x + 1, actualTop, barWidth - 2, actualBottom - actualTop);
                                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                                nvgFill(args.vg);
                            }
                        }
                    }
                }
            }

            // Draw current position indicator on active bar
            if (i >= 2 && i - 2 == currentBar && module->running) {
                float indicatorY = box.size.y - fillHeight;
                nvgBeginPath(args.vg);
                nvgRect(args.vg, x, indicatorY - 1, barWidth, 2);
                nvgFillColor(args.vg, nvgRGB(255, 133, 133));  // Pink indicator
                nvgFill(args.vg);
            }

            // Draw bar labels at the bottom
            nvgFontSize(args.vg, 10);
            nvgFontFaceId(args.vg, APP->window->uiFont->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));  // White text
            char barLabel[6];
            if (i == 0) {
                // Label first bar as "5m" (5 minute intervals)
                snprintf(barLabel, 6, "5m");
            } else if (i == 1) {
                // Label second bar as "1m" (1 minute intervals)
                snprintf(barLabel, 6, "1m");
            } else {
                // Label bars 2-5 as 1-4
                snprintf(barLabel, 6, "%d", i - 1);
            }
            nvgText(args.vg, x + barWidth/2, box.size.y + 2, barLabel, NULL);
        }
    }
};

// Custom Pink Light
template <typename TBase = GrayModuleLightWidget>
struct TPinkLight : TBase {
    TPinkLight() {
        this->addBaseColor(nvgRGB(255, 133, 133));
    }
};
using PinkLight = TPinkLight<>;

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

struct RunshowWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;


    RunshowWidget(Runshow* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");
        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // Screws removed for cleaner look

        // Add white background panel for bottom section (Y=330 and below)
        WhiteBottomPanel* whitePanel = new WhiteBottomPanel();
        whitePanel->box.size = box.size;
        addChild(whitePanel);

        // Add module name and brand labels (like MADDY+)
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "R U N S H O W", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        addChild(new EnhancedTextLabel(Vec(0, 27), Vec(box.size.x, 12), "Collaborated with jan0ritter", 10.f, nvgRGB(255, 255, 255), false));

        // Add Time Code Display at the top
        TimeCodeDisplay* timeCodeDisplay = new TimeCodeDisplay();
        timeCodeDisplay->module = module;
        timeCodeDisplay->box.pos = Vec(68, 64);
        addChild(timeCodeDisplay);

        // 新的對稱佈局設計

        // 第一排 (Y: 70) - 主要功能，往下移動5px
        addParam(createParamCentered<VCVButton>(Vec(30, 70), module, Runshow::START_STOP_PARAM));      // Start 按鈕
        addInput(createInputCentered<PJ301MPort>(Vec(54, 70), module, Runshow::CLOCK_INPUT));          // Clock 輸入
        addParam(createParamCentered<VCVButton>(Vec(152, 70), module, Runshow::RESET_PARAM));          // Reset 按鈕

        // 第二排 (Y: 98-102) - 對應 CV 輸入和指示燈，往上移動2px
        addInput(createInputCentered<PJ301MPort>(Vec(30, 96), module, Runshow::START_STOP_INPUT));     // Start CV
        addChild(createLightCentered<LargeLight<PinkLight>>(Vec(54, 95), module, Runshow::BEAT_LIGHT)); // Beat 燈
        addInput(createInputCentered<PJ301MPort>(Vec(152, 96), module, Runshow::RESET_INPUT));         // Reset CV

        // Add Six Progress Bars (centered)
        FourProgressBars* progressBars = new FourProgressBars();
        progressBars->module = module;
        progressBars->box.pos = Vec(15, 110);  // Centered position adjusted for wider widget
        addChild(progressBars);

        // 6個控制旋鈕 (Y: 343) - 對應 6 條垂直進度條的控制
        addParam(createParamCentered<StandardBlackKnob26>(Vec(15, 343), module, Runshow::TIMER_30MIN_PARAM));                   // 30分鐘計時器控制
        addParam(createParamCentered<StandardBlackKnob26>(Vec(46, 343), module, Runshow::TIMER_15MIN_PARAM));                   // 波形形狀控制
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(76, 343), module, Runshow::BAR_1_PARAM));                 // 第1小節控制 (snap)
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(107, 343), module, Runshow::BAR_2_PARAM));                // 第2小節控制 (snap)
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(137, 343), module, Runshow::BAR_3_PARAM));                // 第3小節控制 (snap)
        addParam(createParamCentered<madzine::widgets::SnapKnob>(Vec(168, 343), module, Runshow::BAR_4_PARAM));                // 第4小節控制 (snap)

        // 6個觸發輸出 (Y: 368) - 對應 6 條垂直進度條，往下移動
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 368), module, Runshow::TIMER_30MIN_OUTPUT));   // 30分鐘計時器
        addOutput(createOutputCentered<PJ301MPort>(Vec(46, 368), module, Runshow::TIMER_15MIN_OUTPUT));   // 15分鐘計時器
        addOutput(createOutputCentered<PJ301MPort>(Vec(76, 368), module, Runshow::BAR_1_OUTPUT));         // 第1小節
        addOutput(createOutputCentered<PJ301MPort>(Vec(107, 368), module, Runshow::BAR_2_OUTPUT));        // 第2小節
        addOutput(createOutputCentered<PJ301MPort>(Vec(137, 368), module, Runshow::BAR_3_OUTPUT));        // 第3小節
        addOutput(createOutputCentered<PJ301MPort>(Vec(168, 368), module, Runshow::BAR_4_OUTPUT));        // 第4小節
    }

    void step() override {
        Runshow* module = dynamic_cast<Runshow*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        Runshow* module = dynamic_cast<Runshow*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelRunshow = createModel<Runshow, RunshowWidget>("Runshow");