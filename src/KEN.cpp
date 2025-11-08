#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
struct KEN : Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamId {
        LEVEL_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_1,
        INPUT_2,
        INPUT_3,
        INPUT_4,
        INPUT_5,
        INPUT_6,
        INPUT_7,
        INPUT_8,
        INPUTS_LEN
    };
    enum OutputId {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    struct SpeakerPosition {
        float x, y, z;
        float azimuth, elevation;
        float distance;
    };

    SpeakerPosition speakers[8] = {
        {-1.0f, 0.0f,  1.0f, -45.0f, 30.0f, 0.5f},   // FL Upper
        { 1.0f, 0.0f,  1.0f,  45.0f, 30.0f, 0.5f},   // FR Upper
        {-1.0f, 0.0f, -1.0f, -135.0f, 30.0f, 2.0f},  // BL Upper
        { 1.0f, 0.0f, -1.0f,  135.0f, 30.0f, 2.0f},  // BR Upper
        {-1.0f, 0.0f,  1.0f, -45.0f, -30.0f, 0.5f},  // FL Lower
        { 1.0f, 0.0f,  1.0f,  45.0f, -30.0f, 0.5f},  // FR Lower
        {-1.0f, 0.0f, -1.0f, -135.0f, -30.0f, 2.0f}, // BL Lower
        { 1.0f, 0.0f, -1.0f,  135.0f, -30.0f, 2.0f}  // BR Lower
    };

    float hrtfGains[8][2];
    float smoothedGains[8][2];
    
    float delayBufferL[8][128];
    float delayBufferR[8][128];
    int delayPosL[8];
    int delayPosR[8];
    int delaySamplesL[8];
    int delaySamplesR[8];

    dsp::TBiquadFilter<> distanceFilters[8][2];
    dsp::TBiquadFilter<> elevationFilters[8][2];
    dsp::TBiquadFilter<> reverbFilters[8][2];

    KEN() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(LEVEL_PARAM, 0.f, 1.f, 0.7f, "Level", "%", 0.f, 100.f);
        
        configInput(INPUT_1, "1");
        configInput(INPUT_2, "2");
        configInput(INPUT_3, "3");
        configInput(INPUT_4, "4");
        configInput(INPUT_5, "5");
        configInput(INPUT_6, "6");
        configInput(INPUT_7, "7");
        configInput(INPUT_8, "8");
        
        configOutput(LEFT_OUTPUT, "Left");
        configOutput(RIGHT_OUTPUT, "Right");

        initializeHRTF();
        initializeDelays();
        initializeDistanceFilters();
        initializeElevationFilters();
        initializeReverbFilters();
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 2; j++) {
                smoothedGains[i][j] = hrtfGains[i][j];
            }
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
    }

    void initializeHRTF() {
        for (int i = 0; i < 8; i++) {
            float azimuth = speakers[i].azimuth * M_PI / 180.0f;
            float elevation = speakers[i].elevation * M_PI / 180.0f;
            
            hrtfGains[i][0] = calculateAdvancedHRTF(azimuth, elevation, 0, i);
            hrtfGains[i][1] = calculateAdvancedHRTF(azimuth, elevation, 1, i);
        }
    }

    void initializeDistanceFilters() {
        for (int i = 0; i < 8; i++) {
            for (int ear = 0; ear < 2; ear++) {
                float sampleRate = 44100.0f;
                float distance = speakers[i].distance;
                float cutoffFreq = 20000.0f / (1.0f + distance * 3.0f);
                cutoffFreq = clamp(cutoffFreq, 1000.0f, 20000.0f);
                
                distanceFilters[i][ear].setParameters(
                    dsp::TBiquadFilter<>::LOWPASS, 
                    cutoffFreq / sampleRate,
                    0.8f,
                    1.0f
                );
            }
        }
    }

    void initializeElevationFilters() {
        for (int i = 0; i < 8; i++) {
            for (int ear = 0; ear < 2; ear++) {
                float sampleRate = 44100.0f;
                float elevation = speakers[i].elevation;
                
                float centerFreq, gain, q;
                if (elevation > 0) {
                    centerFreq = 8000.0f + elevation * 40.0f;
                    gain = 2.0f;
                    q = 1.5f;
                } else {
                    centerFreq = 7000.0f - abs(elevation) * 30.0f;
                    gain = 0.3f;
                    q = 2.0f;
                }
                
                if (elevation > 0) {
                    elevationFilters[i][ear].setParameters(
                        dsp::TBiquadFilter<>::PEAK,
                        centerFreq / sampleRate,
                        q,
                        gain
                    );
                } else {
                    elevationFilters[i][ear].setParameters(
                        dsp::TBiquadFilter<>::LOWPASS,
                        centerFreq / sampleRate,
                        q,
                        gain
                    );
                }
            }
        }
    }

    void initializeReverbFilters() {
        for (int i = 0; i < 8; i++) {
            for (int ear = 0; ear < 2; ear++) {
                float sampleRate = 44100.0f;
                float distance = speakers[i].distance;
                
                float reverbGain = 0.1f + distance * 0.4f;
                
                reverbFilters[i][ear].setParameters(
                    dsp::TBiquadFilter<>::HIGHPASS,
                    3000.0f / sampleRate,
                    0.7f,
                    reverbGain
                );
            }
        }
    }

    void initializeDelays() {
        float sampleRate = 44100.0f;
        float headWidth = 0.18f;
        float soundSpeed = 343.0f;
        
        for (int i = 0; i < 8; i++) {
            float azimuth = speakers[i].azimuth * M_PI / 180.0f;
            
            float itdSeconds = (headWidth / soundSpeed) * sin(azimuth);
            int itdSamples = (int)(itdSeconds * sampleRate);
            
            if (itdSamples > 0) {
                delaySamplesL[i] = itdSamples;
                delaySamplesR[i] = 0;
            } else {
                delaySamplesL[i] = 0;
                delaySamplesR[i] = -itdSamples;
            }
            
            delayPosL[i] = 0;
            delayPosR[i] = 0;
            for (int j = 0; j < 128; j++) {
                delayBufferL[i][j] = 0.0f;
                delayBufferR[i][j] = 0.0f;
            }
        }
    }

    float calculateAdvancedHRTF(float azimuth, float elevation, int ear, int speakerIndex) {
        float gain = 1.0f;
        
        float ildEffect = 1.0f;
        if (ear == 0) { 
            if (azimuth > 0) { 
                ildEffect = 1.0f - (azimuth / M_PI) * 0.8f;
            } else { 
                ildEffect = 1.0f + (-azimuth / M_PI) * 0.3f;
            }
        } else { 
            if (azimuth < 0) { 
                ildEffect = 1.0f - (-azimuth / M_PI) * 0.8f;
            } else { 
                ildEffect = 1.0f + (azimuth / M_PI) * 0.3f;
            }
        }
        
        float headShadowEffect = 1.0f;
        if (abs(azimuth) > M_PI / 2) { 
            headShadowEffect *= 0.6f;
        }
        
        float elevationEffect = 1.0f;
        if (elevation > 0) {
            elevationEffect = 1.0f + elevation * 0.8f;
        } else {
            elevationEffect = 0.7f - abs(elevation) * 0.4f;
        }
        
        float distance = speakers[speakerIndex].distance;
        float distanceGain = 1.0f / (1.0f + distance * distance);
        
        gain = ildEffect * headShadowEffect * elevationEffect * distanceGain;
        gain = clamp(gain, 0.1f, 1.5f);
        
        return gain;
    }

    float processDelay(float input, int speaker, int ear) {
        float output;
        
        if (ear == 0) { 
            delayBufferL[speaker][delayPosL[speaker]] = input;
            int readPos = (delayPosL[speaker] - delaySamplesL[speaker] + 128) % 128;
            output = delayBufferL[speaker][readPos];
            delayPosL[speaker] = (delayPosL[speaker] + 1) % 128;
        } else { 
            delayBufferR[speaker][delayPosR[speaker]] = input;
            int readPos = (delayPosR[speaker] - delaySamplesR[speaker] + 128) % 128;
            output = delayBufferR[speaker][readPos];
            delayPosR[speaker] = (delayPosR[speaker] + 1) % 128;
        }
        
        return output;
    }

    float processDistanceFiltering(float input, int speaker, int ear) {
        return distanceFilters[speaker][ear].process(input);
    }

    float processElevationFiltering(float input, int speaker, int ear) {
        return elevationFilters[speaker][ear].process(input);
    }

    float processReverbFiltering(float input, int speaker, int ear) {
        return reverbFilters[speaker][ear].process(input);
    }

    void process(const ProcessArgs& args) override {
        float level = params[LEVEL_PARAM].getValue();
        float leftOut = 0.0f;
        float rightOut = 0.0f;

        for (int i = 0; i < 8; i++) {
            if (inputs[INPUT_1 + i].isConnected()) {
                float input = inputs[INPUT_1 + i].getVoltage();
                
                float delayedLeft = processDelay(input, i, 0);
                float delayedRight = processDelay(input, i, 1);
                
                delayedLeft = processDistanceFiltering(delayedLeft, i, 0);
                delayedRight = processDistanceFiltering(delayedRight, i, 1);
                
                delayedLeft = processElevationFiltering(delayedLeft, i, 0);
                delayedRight = processElevationFiltering(delayedRight, i, 1);
                
                float reverbLeft = processReverbFiltering(delayedLeft, i, 0);
                float reverbRight = processReverbFiltering(delayedRight, i, 1);
                
                float distance = speakers[i].distance;
                float reverbMix = distance * 0.3f;
                
                delayedLeft = delayedLeft * (1.0f - reverbMix) + reverbLeft * reverbMix;
                delayedRight = delayedRight * (1.0f - reverbMix) + reverbRight * reverbMix;
                
                float smoothing = 0.001f;
                smoothedGains[i][0] = smoothedGains[i][0] * (1.0f - smoothing) + hrtfGains[i][0] * smoothing;
                smoothedGains[i][1] = smoothedGains[i][1] * (1.0f - smoothing) + hrtfGains[i][1] * smoothing;
                
                leftOut += delayedLeft * smoothedGains[i][0] * level;
                rightOut += delayedRight * smoothedGains[i][1] * level;
            }
        }

        outputs[LEFT_OUTPUT].setVoltage(leftOut);
        outputs[RIGHT_OUTPUT].setVoltage(rightOut);
    }
};

struct TechnoEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;
    
    TechnoEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 12.f, 
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
struct KENWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    KENWidget(KEN* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP");
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "KEN", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new TechnoEnhancedTextLabel(Vec(15, 43), Vec(30, 10), "LEVEL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(30, 70), module, KEN::LEVEL_PARAM));

        float inputStartY = 110;
        float inputSpacing = 28;
        
        for (int i = 0; i < 8; i++) {
            float y = inputStartY + i * inputSpacing;
            
            addChild(new TechnoEnhancedTextLabel(Vec(3, y-5), Vec(20, 10), std::to_string(i + 1), 8.f, nvgRGB(255, 255, 255), true));
            addInput(createInputCentered<PJ301MPort>(Vec(30, y), module, KEN::INPUT_1 + i));
        }

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, 333), Vec(20, 10), "L", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(15, 355), module, KEN::LEFT_OUTPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(35, 333), Vec(20, 10), "R", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 355), module, KEN::RIGHT_OUTPUT));
    }

    void step() override {
        KEN* module = dynamic_cast<KEN*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        KEN* module = dynamic_cast<KEN*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelKEN = createModel<KEN, KENWidget>("KEN");