#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
struct DECAPyramid : Module {
    enum ParamId {
        X_PARAM_1, Y_PARAM_1, Z_PARAM_1, LEVEL_PARAM_1, FILTER_PARAM_1, SENDA_PARAM_1, SENDB_PARAM_1,
        X_PARAM_2, Y_PARAM_2, Z_PARAM_2, LEVEL_PARAM_2, FILTER_PARAM_2, SENDA_PARAM_2, SENDB_PARAM_2,
        X_PARAM_3, Y_PARAM_3, Z_PARAM_3, LEVEL_PARAM_3, FILTER_PARAM_3, SENDA_PARAM_3, SENDB_PARAM_3,
        X_PARAM_4, Y_PARAM_4, Z_PARAM_4, LEVEL_PARAM_4, FILTER_PARAM_4, SENDA_PARAM_4, SENDB_PARAM_4,
        X_PARAM_5, Y_PARAM_5, Z_PARAM_5, LEVEL_PARAM_5, FILTER_PARAM_5, SENDA_PARAM_5, SENDB_PARAM_5,
        X_PARAM_6, Y_PARAM_6, Z_PARAM_6, LEVEL_PARAM_6, FILTER_PARAM_6, SENDA_PARAM_6, SENDB_PARAM_6,
        X_PARAM_7, Y_PARAM_7, Z_PARAM_7, LEVEL_PARAM_7, FILTER_PARAM_7, SENDA_PARAM_7, SENDB_PARAM_7,
        X_PARAM_8, Y_PARAM_8, Z_PARAM_8, LEVEL_PARAM_8, FILTER_PARAM_8, SENDA_PARAM_8, SENDB_PARAM_8,
        OUTPUT_1_4_LEVEL_PARAM, OUTPUT_5_8_LEVEL_PARAM, MASTER_OUTPUT_LEVEL_PARAM,
        RTN_A_LEVEL_PARAM, RTN_A_FILTER_PARAM, RTN_B_LEVEL_PARAM, RTN_B_FILTER_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AUDIO_INPUT_1, X_CV_INPUT_1, Y_CV_INPUT_1, Z_CV_INPUT_1,
        AUDIO_INPUT_2, X_CV_INPUT_2, Y_CV_INPUT_2, Z_CV_INPUT_2,
        AUDIO_INPUT_3, X_CV_INPUT_3, Y_CV_INPUT_3, Z_CV_INPUT_3,
        AUDIO_INPUT_4, X_CV_INPUT_4, Y_CV_INPUT_4, Z_CV_INPUT_4,
        AUDIO_INPUT_5, X_CV_INPUT_5, Y_CV_INPUT_5, Z_CV_INPUT_5,
        AUDIO_INPUT_6, X_CV_INPUT_6, Y_CV_INPUT_6, Z_CV_INPUT_6,
        AUDIO_INPUT_7, X_CV_INPUT_7, Y_CV_INPUT_7, Z_CV_INPUT_7,
        AUDIO_INPUT_8, X_CV_INPUT_8, Y_CV_INPUT_8, Z_CV_INPUT_8,
        INSERT_RETURN_1, INSERT_RETURN_2, INSERT_RETURN_3, INSERT_RETURN_4,
        INSERT_RETURN_5, INSERT_RETURN_6, INSERT_RETURN_7, INSERT_RETURN_8,
        RETURN_AL_INPUT, RETURN_AR_INPUT, RETURN_BL_INPUT, RETURN_BR_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        SENDA_OUTPUT, SENDB_OUTPUT,
        MASTER_OUTPUT_1, MASTER_OUTPUT_2, MASTER_OUTPUT_3, MASTER_OUTPUT_4,
        MASTER_OUTPUT_5, MASTER_OUTPUT_6, MASTER_OUTPUT_7, MASTER_OUTPUT_8,
        INSERT_SEND_1, INSERT_SEND_2, INSERT_SEND_3, INSERT_SEND_4,
        INSERT_SEND_5, INSERT_SEND_6, INSERT_SEND_7, INSERT_SEND_8,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    struct SpeakerPosition {
        float x, y, z;
    };

    SpeakerPosition speakers[8] = {
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f}
    };

    dsp::TBiquadFilter<> filter1[8];
    dsp::TBiquadFilter<> filter2[8];

    // CV 調變顯示用 [track][axis: 0=X, 1=Y, 2=Z]
    float cvMod[8][3] = {{0.0f}};
    dsp::TBiquadFilter<> rtnAFilter1, rtnAFilter2;
    dsp::TBiquadFilter<> rtnBFilter1, rtnBFilter2;
    dsp::VuMeter2 vuMeterPre[8];
    dsp::VuMeter2 vuMeterPost[8];

    bool sendPreLevel = false;
    int panelTheme = 1;
    float panelContrast = panelContrastDefault;
    int lastRtnAFilterMode = 0;
    int lastRtnBFilterMode = 0;
    float lastRtnAFilterValue = 0.f;
    float lastRtnBFilterValue = 0.f;
    float smoothedRtnAFilter = 0.f;
    float smoothedRtnBFilter = 0.f;
    int lastFilterMode[8] = {0};
    float lastFilterValue[8] = {0.f};
    float smoothedFilter[8] = {0.f};

    DECAPyramid() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        for (int i = 0; i < 8; i++) {
            configParam(X_PARAM_1 + i * 7, -1.f, 1.f, -1.f, "Track " + std::to_string(i + 1) + " X Position", "", 0.f, 1.f);
            configParam(Y_PARAM_1 + i * 7, -1.f, 1.f, -1.f, "Track " + std::to_string(i + 1) + " Y Position", "", 0.f, 1.f);
            configParam(Z_PARAM_1 + i * 7, -1.f, 1.f, -1.f, "Track " + std::to_string(i + 1) + " Z Position", "", 0.f, 1.f);
            configParam(LEVEL_PARAM_1 + i * 7, 0.f, 1.f, 0.7f, "Track " + std::to_string(i + 1) + " Level", "%", 0.f, 100.f);
            configParam(FILTER_PARAM_1 + i * 7, -1.f, 1.f, 0.f, "Track " + std::to_string(i + 1) + " Filter");
            configParam(SENDA_PARAM_1 + i * 7, 0.f, 1.f, 0.f, "Track " + std::to_string(i + 1) + " Send A", "%", 0.f, 100.f);
            configParam(SENDB_PARAM_1 + i * 7, 0.f, 1.f, 0.f, "Track " + std::to_string(i + 1) + " Send B", "%", 0.f, 100.f);
            
            configInput(AUDIO_INPUT_1 + i * 4, "Track " + std::to_string(i + 1) + " Audio");
            configInput(X_CV_INPUT_1 + i * 4, "Track " + std::to_string(i + 1) + " X CV");
            configInput(Y_CV_INPUT_1 + i * 4, "Track " + std::to_string(i + 1) + " Y CV");
            configInput(Z_CV_INPUT_1 + i * 4, "Track " + std::to_string(i + 1) + " Z CV");
            
            configOutput(INSERT_SEND_1 + i, "Track " + std::to_string(i + 1) + " Insert Send");
            configInput(INSERT_RETURN_1 + i, "Track " + std::to_string(i + 1) + " Insert Return");
            
            configOutput(MASTER_OUTPUT_1 + i, "Master " + std::to_string(i + 1));
        }
        
        configParam(OUTPUT_1_4_LEVEL_PARAM, 0.f, 1.f, 0.7f, "Output 1-4 Level", "%", 0.f, 100.f);
        configParam(OUTPUT_5_8_LEVEL_PARAM, 0.f, 1.f, 0.7f, "Output 5-8 Level", "%", 0.f, 100.f);
        configParam(MASTER_OUTPUT_LEVEL_PARAM, 0.f, 1.f, 0.7f, "Master Output Level", "%", 0.f, 100.f);
        configParam(RTN_A_LEVEL_PARAM, 0.f, 1.f, 0.7f, "Return A Level", "%", 0.f, 100.f);
        configParam(RTN_A_FILTER_PARAM, -1.f, 1.f, 0.f, "Return A Filter");
        configParam(RTN_B_LEVEL_PARAM, 0.f, 1.f, 0.7f, "Return B Level", "%", 0.f, 100.f);
        configParam(RTN_B_FILTER_PARAM, -1.f, 1.f, 0.f, "Return B Filter");
        
        configInput(RETURN_AL_INPUT, "Return A L");
        configInput(RETURN_AR_INPUT, "Return A R");
        configInput(RETURN_BL_INPUT, "Return B L");
        configInput(RETURN_BR_INPUT, "Return B R");
        
        configOutput(SENDA_OUTPUT, "Send A");
        configOutput(SENDB_OUTPUT, "Send B");
    }

    float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    void calculateVBAP(float sourceX, float sourceY, float sourceZ, float gains[8]) {
        for (int i = 0; i < 8; i++) {
            float distance = distance3D(sourceX, sourceY, sourceZ, 
                                      speakers[i].x, speakers[i].y, speakers[i].z);
            
            distance = std::max(distance, 0.001f);
            
            float gain = 1.0f / (1.0f + distance + distance * distance * 2.0f);
            
            float fadeOut = 1.0f;
            
            if (sourceX <= -0.8f && speakers[i].x > 0) {
                fadeOut *= std::max(0.0f, (sourceX + 1.0f) / 0.2f);
            }
            if (sourceX >= 0.8f && speakers[i].x < 0) {
                fadeOut *= std::max(0.0f, (1.0f - sourceX) / 0.2f);
            }
            
            if (sourceY <= -0.8f && speakers[i].y > 0) {
                fadeOut *= std::max(0.0f, (sourceY + 1.0f) / 0.2f);
            }
            if (sourceY >= 0.8f && speakers[i].y < 0) {
                fadeOut *= std::max(0.0f, (1.0f - sourceY) / 0.2f);
            }
            
            if (sourceZ <= -0.8f && speakers[i].z > 0) {
                fadeOut *= std::max(0.0f, (sourceZ + 1.0f) / 0.2f);
            }
            if (sourceZ >= 0.8f && speakers[i].z < 0) {
                fadeOut *= std::max(0.0f, (1.0f - sourceZ) / 0.2f);
            }
            
            gains[i] = gain * fadeOut;
        }
        
        float totalGain = 0.0f;
        for (int i = 0; i < 8; i++) {
            totalGain += gains[i] * gains[i];
        }
        
        if (totalGain > 0.0f) {
            float normalizeFactor = 1.0f / std::sqrt(totalGain);
            for (int i = 0; i < 8; i++) {
                gains[i] *= normalizeFactor;
            }
        }
    }

    void process(const ProcessArgs& args) override {
        float sendAOut = 0.0f;
        float sendBOut = 0.0f;
        
        float returnAL = inputs[RETURN_AL_INPUT].getVoltage();
        float returnAR = inputs[RETURN_AR_INPUT].getVoltage();
        float returnBL = inputs[RETURN_BL_INPUT].getVoltage();
        float returnBR = inputs[RETURN_BR_INPUT].getVoltage();
        
        float rtnALevel = params[RTN_A_LEVEL_PARAM].getValue();
        float rtnAFilter = params[RTN_A_FILTER_PARAM].getValue();
        float rtnBLevel = params[RTN_B_LEVEL_PARAM].getValue();
        float rtnBFilter = params[RTN_B_FILTER_PARAM].getValue();
        
        float filterSmooth = 0.005f;
        smoothedRtnAFilter += (rtnAFilter - smoothedRtnAFilter) * filterSmooth;
        smoothedRtnBFilter += (rtnBFilter - smoothedRtnBFilter) * filterSmooth;
        
        if (smoothedRtnAFilter < -0.001f) {
            if (lastRtnAFilterMode != -1) {
                rtnAFilter1.reset();
                rtnAFilter2.reset();
                lastRtnAFilterMode = -1;
            }
            float freq = rescale(smoothedRtnAFilter, -1.f, 0.f, 20.f, 22000.f);
            rtnAFilter1.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            rtnAFilter2.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            returnAL = rtnAFilter2.process(rtnAFilter1.process(returnAL));
            returnAR = rtnAFilter2.process(rtnAFilter1.process(returnAR));
        } else if (smoothedRtnAFilter > 0.001f) {
            if (lastRtnAFilterMode != 1) {
                rtnAFilter1.reset();
                rtnAFilter2.reset();
                lastRtnAFilterMode = 1;
            }
            float freq = rescale(smoothedRtnAFilter, 0.f, 1.f, 10.f, 8000.f);
            rtnAFilter1.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            rtnAFilter2.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            returnAL = rtnAFilter2.process(rtnAFilter1.process(returnAL));
            returnAR = rtnAFilter2.process(rtnAFilter1.process(returnAR));
        } else {
            lastRtnAFilterMode = 0;
        }
        
        if (smoothedRtnBFilter < -0.001f) {
            if (lastRtnBFilterMode != -1) {
                rtnBFilter1.reset();
                rtnBFilter2.reset();
                lastRtnBFilterMode = -1;
            }
            float freq = rescale(smoothedRtnBFilter, -1.f, 0.f, 20.f, 22000.f);
            rtnBFilter1.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            rtnBFilter2.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            returnBL = rtnBFilter2.process(rtnBFilter1.process(returnBL));
            returnBR = rtnBFilter2.process(rtnBFilter1.process(returnBR));
        } else if (smoothedRtnBFilter > 0.001f) {
            if (lastRtnBFilterMode != 1) {
                rtnBFilter1.reset();
                rtnBFilter2.reset();
                lastRtnBFilterMode = 1;
            }
            float freq = rescale(smoothedRtnBFilter, 0.f, 1.f, 10.f, 8000.f);
            rtnBFilter1.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            rtnBFilter2.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            returnBL = rtnBFilter2.process(rtnBFilter1.process(returnBL));
            returnBR = rtnBFilter2.process(rtnBFilter1.process(returnBR));
        } else {
            lastRtnBFilterMode = 0;
        }
        
        returnAL *= rtnALevel;
        returnAR *= rtnALevel;
        returnBL *= rtnBLevel;
        returnBR *= rtnBLevel;
        
        float output14Level = params[OUTPUT_1_4_LEVEL_PARAM].getValue();
        float output58Level = params[OUTPUT_5_8_LEVEL_PARAM].getValue();
        float masterLevel = params[MASTER_OUTPUT_LEVEL_PARAM].getValue();
        
        for (int speaker = 0; speaker < 8; speaker++) {
            outputs[MASTER_OUTPUT_1 + speaker].setVoltage(0.0f);
        }
        
        for (int track = 0; track < 8; track++) {
            float audioIn = inputs[AUDIO_INPUT_1 + track * 4].getVoltage();
            
            vuMeterPre[track].process(args.sampleTime, audioIn);
            
            float x = params[X_PARAM_1 + track * 7].getValue();
            float y = params[Y_PARAM_1 + track * 7].getValue();
            float z = params[Z_PARAM_1 + track * 7].getValue();
            float level = params[LEVEL_PARAM_1 + track * 7].getValue();
            float filter = params[FILTER_PARAM_1 + track * 7].getValue();
            float sendA = params[SENDA_PARAM_1 + track * 7].getValue();
            float sendB = params[SENDB_PARAM_1 + track * 7].getValue();
            
            if (inputs[X_CV_INPUT_1 + track * 4].isConnected()) {
                float cv = inputs[X_CV_INPUT_1 + track * 4].getVoltage();
                x += cv * 0.2f;
                x = clamp(x, -1.f, 1.f);
                cvMod[track][0] = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else {
                cvMod[track][0] = 0.0f;
            }
            if (inputs[Y_CV_INPUT_1 + track * 4].isConnected()) {
                float cv = inputs[Y_CV_INPUT_1 + track * 4].getVoltage();
                y += cv * 0.2f;
                y = clamp(y, -1.f, 1.f);
                cvMod[track][1] = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else {
                cvMod[track][1] = 0.0f;
            }
            if (inputs[Z_CV_INPUT_1 + track * 4].isConnected()) {
                float cv = inputs[Z_CV_INPUT_1 + track * 4].getVoltage();
                z += cv * 0.2f;
                z = clamp(z, -1.f, 1.f);
                cvMod[track][2] = clamp(cv / 10.0f, -1.0f, 1.0f);
            } else {
                cvMod[track][2] = 0.0f;
            }
            
            outputs[INSERT_SEND_1 + track].setVoltage(audioIn);
            
            if (inputs[INSERT_RETURN_1 + track].isConnected()) {
                audioIn = inputs[INSERT_RETURN_1 + track].getVoltage();
            }
            
            audioIn *= level;
            
            vuMeterPost[track].process(args.sampleTime, audioIn);
            
            float sendATrack, sendBTrack;
            if (sendPreLevel) {
                sendATrack = inputs[AUDIO_INPUT_1 + track * 4].getVoltage() * sendA;
                sendBTrack = inputs[AUDIO_INPUT_1 + track * 4].getVoltage() * sendB;
            } else {
                sendATrack = audioIn * sendA;
                sendBTrack = audioIn * sendB;
            }
            
            sendAOut += sendATrack;
            sendBOut += sendBTrack;
            
            float filterSmooth = 0.005f;
            smoothedFilter[track] += (filter - smoothedFilter[track]) * filterSmooth;

            if (smoothedFilter[track] < -0.001f) {
                if (lastFilterMode[track] != -1) {
                    filter1[track].reset();
                    filter2[track].reset();
                    lastFilterMode[track] = -1;
                }
                float freq = rescale(smoothedFilter[track], -1.f, 0.f, 20.f, 22000.f);
                filter1[track].setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
                filter2[track].setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
                audioIn = filter2[track].process(filter1[track].process(audioIn));
            } else if (smoothedFilter[track] > 0.001f) {
                if (lastFilterMode[track] != 1) {
                    filter1[track].reset();
                    filter2[track].reset();
                    lastFilterMode[track] = 1;
                }
                float freq = rescale(smoothedFilter[track], 0.f, 1.f, 10.f, 8000.f);
                filter1[track].setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
                filter2[track].setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
                audioIn = filter2[track].process(filter1[track].process(audioIn));
            } else {
                lastFilterMode[track] = 0;
            }
            
            float gains[8];
            calculateVBAP(x, y, -z, gains);
            
            for (int speaker = 0; speaker < 8; speaker++) {
                float outputVoltage = audioIn * gains[speaker];
                
                if (speaker % 2 == 0) {
                    outputVoltage += returnAL * gains[speaker];
                    outputVoltage += returnBL * gains[speaker];
                } else {
                    outputVoltage += returnAR * gains[speaker];
                    outputVoltage += returnBR * gains[speaker];
                }
                
                float levelMultiplier = (speaker < 4) ? output14Level : output58Level;
                outputVoltage *= levelMultiplier * masterLevel;
                
                outputs[MASTER_OUTPUT_1 + speaker].setVoltage(outputs[MASTER_OUTPUT_1 + speaker].getVoltage() + outputVoltage);
            }
        }
        
        outputs[SENDA_OUTPUT].setVoltage(sendAOut);
        outputs[SENDB_OUTPUT].setVoltage(sendBOut);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_real_value(contrastJ);
    }
};

struct TechnoEnhancedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    TechnoEnhancedTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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
            // 使用描邊模擬粗體效果
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

// 大型背景裝飾標籤（帶黑色外框）
struct OutlinedTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    float outlineWidth;

    OutlinedTextLabel(Vec pos, Vec size, std::string text, float fontSize,
                      NVGcolor color, float outlineWidth = 2.f) {
        box.pos = pos;
        box.size = size;
        this->text = text;
        this->fontSize = fontSize;
        this->color = color;
        this->outlineWidth = outlineWidth;
    }

    void draw(const DrawArgs &args) override {
        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        float cx = box.size.x / 2.f;
        float cy = box.size.y / 2.f;

        // 繪製黑色外框（多次偏移繪製）
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        for (float dx = -outlineWidth; dx <= outlineWidth; dx += 1.f) {
            for (float dy = -outlineWidth; dy <= outlineWidth; dy += 1.f) {
                if (dx != 0 || dy != 0) {
                    nvgText(args.vg, cx + dx, cy + dy, text.c_str(), NULL);
                }
            }
        }

        // 繪製主要顏色文字
        nvgFillColor(args.vg, color);
        nvgText(args.vg, cx, cy, text.c_str(), NULL);
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
struct VolumeMeterWidget : Widget {
    DECAPyramid* module;
    int trackIndex;
    bool preLevel;
    
    VolumeMeterWidget() {
        box.size = Vec(6, 200);
        preLevel = false;
    }
    
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStroke(args.vg);
        
        float level;
        if (preLevel) {
            level = module->vuMeterPre[trackIndex].v;
        } else {
            level = module->vuMeterPost[trackIndex].v;
        }
        
        level = level / 5.0f;
        level = clamp(level, 0.f, 1.2f);
        
        float meterHeight = level * box.size.y;
        
        if (meterHeight > 0) {
            float greenHeight = std::min(meterHeight, box.size.y * 0.8f);
            float yellowHeight = std::min(meterHeight - greenHeight, box.size.y * 0.15f);
            float redHeight = meterHeight - greenHeight - yellowHeight;
            
            float y = box.size.y;
            
            float brightness = preLevel ? 0.6f : 0.7f;
            
            if (greenHeight > 0) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 1, y - greenHeight, box.size.x - 2, greenHeight);
                nvgFillColor(args.vg, nvgRGB(0 * brightness, 255 * brightness, 0 * brightness));
                nvgFill(args.vg);
                y -= greenHeight;
            }
            
            if (yellowHeight > 0) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 1, y - yellowHeight, box.size.x - 2, yellowHeight);
                nvgFillColor(args.vg, nvgRGB(255 * brightness, 255 * brightness, 0 * brightness));
                nvgFill(args.vg);
                y -= yellowHeight;
            }
            
            if (redHeight > 0) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 1, y - redHeight, box.size.x - 2, redHeight);
                nvgFillColor(args.vg, nvgRGB(255 * brightness, 0 * brightness, 0 * brightness));
                nvgFill(args.vg);
            }
        }
    }
};

struct DECAPyramidMasterDisplay : LedDisplay {
    DECAPyramid* module;
    ModuleWidget* moduleWidget;
    
    DECAPyramidMasterDisplay() {
        box.size = Vec(100, 100);
    }
    
    Vec project3D(float x, float y, float z) {
        float iso_x = (x - z) * cos(30.0f * M_PI / 180.0f);
        float iso_y = (x + z) * sin(30.0f * M_PI / 180.0f) - y;
        
        float scale = box.size.x * 0.375f;
        return Vec(box.size.x/2 + iso_x * scale, 
                   box.size.y/2 + iso_y * scale);
    }
    
    void drawSpeakerCube(const DrawArgs& args) {
        if (!module) return;
        
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 100));
        
        for (int i = 0; i < 8; i++) {
            Vec pos = project3D(module->speakers[i].x * 0.5f, 
                              module->speakers[i].y * 0.5f, 
                              module->speakers[i].z * 0.5f);
            
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, pos.x, pos.y, 3.0f);
            nvgStroke(args.vg);
            
            nvgFontSize(args.vg, 8.0f);
            nvgFontFaceId(args.vg, APP->window->uiFont->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            
            std::string speakerNum = std::to_string(i + 1);
            nvgText(args.vg, pos.x, pos.y, speakerNum.c_str(), NULL);
        }
        
        Vec corners[8];
        for (int i = 0; i < 8; i++) {
            corners[i] = project3D(module->speakers[i].x * 0.5f, 
                                 module->speakers[i].y * 0.5f, 
                                 module->speakers[i].z * 0.5f);
        }
        
        int edges[12][2] = {
            {0, 1}, {1, 3}, {3, 2}, {2, 0},
            {4, 5}, {5, 7}, {7, 6}, {6, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        
        for (int i = 0; i < 12; i++) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, corners[edges[i][0]].x, corners[edges[i][0]].y);
            nvgLineTo(args.vg, corners[edges[i][1]].x, corners[edges[i][1]].y);
            nvgStroke(args.vg);
        }
    }
    
    void drawAllAudioSources(const DrawArgs& args) {
        if (!module) return;
        
        NVGcolor trackColors[8] = {
            nvgRGB(10, 149, 149),   // Track 1: 深青色 +10
            nvgRGB(89, 121, 153),   // Track 2: 石墨藍 +10
            nvgRGB(112, 61, 163),   // Track 3: 雅緻紫 +10
            nvgRGB(194, 144, 21),   // Track 4: 深金黃 +10
            nvgRGB(117, 152, 45),   // Track 5: 橄欖綠 +10
            nvgRGB(10, 117, 73),    // Track 6: 深翠綠 +10
            nvgRGB(124, 57, 65),    // Track 7: 深酒紅 +10
            nvgRGB(152, 135, 200)   // Track 8: 深紫灰 +10
        };
        
        for (int trackIndex = 0; trackIndex < 8; trackIndex++) {
            float x = module->params[DECAPyramid::X_PARAM_1 + trackIndex * 7].getValue();
            float y = module->params[DECAPyramid::Y_PARAM_1 + trackIndex * 7].getValue();
            float z = module->params[DECAPyramid::Z_PARAM_1 + trackIndex * 7].getValue();
            
            if (module->inputs[DECAPyramid::X_CV_INPUT_1 + trackIndex * 4].isConnected()) {
                x += module->inputs[DECAPyramid::X_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
                x = clamp(x, -1.f, 1.f);
            }
            if (module->inputs[DECAPyramid::Y_CV_INPUT_1 + trackIndex * 4].isConnected()) {
                y += module->inputs[DECAPyramid::Y_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
                y = clamp(y, -1.f, 1.f);
            }
            if (module->inputs[DECAPyramid::Z_CV_INPUT_1 + trackIndex * 4].isConnected()) {
                z += module->inputs[DECAPyramid::Z_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
                z = clamp(z, -1.f, 1.f);
            }
            
            Vec pos = project3D(x * 0.5f, y * 0.5f, -z * 0.5f);
            
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, pos.x, pos.y, 3.0f);
            nvgFillColor(args.vg, trackColors[trackIndex]);
            nvgFill(args.vg);
            
            nvgStrokeWidth(args.vg, 0.5f);
            nvgStrokeColor(args.vg, trackColors[trackIndex]);
            nvgStroke(args.vg);
        }
    }
    
    void drawBackground(const DrawArgs& args) {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStroke(args.vg);
    }
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;
        
        drawBackground(args);
        drawSpeakerCube(args);
        drawAllAudioSources(args);
    }
};

struct DECAPyramid3DDisplay : LedDisplay {
    DECAPyramid* module;
    ModuleWidget* moduleWidget;
    int trackIndex;
    
    DECAPyramid3DDisplay() {
        box.size = Vec(60, 50);
    }
    
    Vec project3D(float x, float y, float z) {
        float iso_x = (x - z) * cos(30.0f * M_PI / 180.0f);
        float iso_y = (x + z) * sin(30.0f * M_PI / 180.0f) - y;
        
        float scale = box.size.x * 0.375f;
        return Vec(box.size.x/2 + iso_x * scale, 
                   box.size.y/2 + iso_y * scale);
    }
    
    void drawSpeakerCube(const DrawArgs& args) {
        if (!module) return;
        
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 100));
        
        for (int i = 0; i < 8; i++) {
            Vec pos = project3D(module->speakers[i].x * 0.5f, 
                              module->speakers[i].y * 0.5f, 
                              module->speakers[i].z * 0.5f);
            
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, pos.x, pos.y, 1.5f);
            nvgStroke(args.vg);
            
            nvgFontSize(args.vg, 6.0f);
            nvgFontFaceId(args.vg, APP->window->uiFont->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            
            std::string speakerNum = std::to_string(i + 1);
            nvgText(args.vg, pos.x, pos.y, speakerNum.c_str(), NULL);
        }
        
        Vec corners[8];
        for (int i = 0; i < 8; i++) {
            corners[i] = project3D(module->speakers[i].x * 0.5f, 
                                 module->speakers[i].y * 0.5f, 
                                 module->speakers[i].z * 0.5f);
        }
        
        int edges[12][2] = {
            {0, 1}, {1, 3}, {3, 2}, {2, 0},
            {4, 5}, {5, 7}, {7, 6}, {6, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        
        for (int i = 0; i < 12; i++) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, corners[edges[i][0]].x, corners[edges[i][0]].y);
            nvgLineTo(args.vg, corners[edges[i][1]].x, corners[edges[i][1]].y);
            nvgStroke(args.vg);
        }
    }
    
    void drawAudioSource(const DrawArgs& args) {
        if (!module) return;
        
        float x = module->params[DECAPyramid::X_PARAM_1 + trackIndex * 7].getValue();
        float y = module->params[DECAPyramid::Y_PARAM_1 + trackIndex * 7].getValue();
        float z = module->params[DECAPyramid::Z_PARAM_1 + trackIndex * 7].getValue();
        
        if (module->inputs[DECAPyramid::X_CV_INPUT_1 + trackIndex * 4].isConnected()) {
            x += module->inputs[DECAPyramid::X_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
            x = clamp(x, -1.f, 1.f);
        }
        if (module->inputs[DECAPyramid::Y_CV_INPUT_1 + trackIndex * 4].isConnected()) {
            y += module->inputs[DECAPyramid::Y_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
            y = clamp(y, -1.f, 1.f);
        }
        if (module->inputs[DECAPyramid::Z_CV_INPUT_1 + trackIndex * 4].isConnected()) {
            z += module->inputs[DECAPyramid::Z_CV_INPUT_1 + trackIndex * 4].getVoltage() * 0.2f;
            z = clamp(z, -1.f, 1.f);
        }
        
        Vec pos = project3D(x * 0.5f, y * 0.5f, -z * 0.5f);
        
        NVGcolor trackColors[8] = {
            nvgRGB(10, 149, 149),   // Track 1: 深青色 +10
            nvgRGB(89, 121, 153),   // Track 2: 石墨藍 +10
            nvgRGB(112, 61, 163),   // Track 3: 雅緻紫 +10
            nvgRGB(194, 144, 21),   // Track 4: 深金黃 +10
            nvgRGB(117, 152, 45),   // Track 5: 橄欖綠 +10
            nvgRGB(10, 117, 73),    // Track 6: 深翠綠 +10
            nvgRGB(124, 57, 65),    // Track 7: 深酒紅 +10
            nvgRGB(152, 135, 200)   // Track 8: 深紫灰 +10
        };
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, pos.x, pos.y, 2.0f);
        nvgFillColor(args.vg, trackColors[trackIndex]);
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, trackColors[trackIndex]);
        nvgStroke(args.vg);
    }
    
    void drawBackground(const DrawArgs& args) {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStroke(args.vg);
    }
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;
        
        drawBackground(args);
        drawSpeakerCube(args);
        drawAudioSource(args);
    }
};

struct DECAPyramidWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    // [track][axis: 0=X, 1=Y, 2=Z]
    StandardBlackKnob26* xyzKnobs[8][3] = {{nullptr}};

    DECAPyramidWidget(DECAPyramid* module) {
        setModule(module);
        panelThemeHelper.init(this, "40HP", module ? &module->panelContrast : nullptr);
        
        box.size = Vec(40 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(480, 1), Vec(120, 20), "DECAPYRAMID", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(480, 13), Vec(120, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        float trackY[] = {35, 85, 145, 205};

        // X/Y/Z 大型背景裝飾標籤 - 在旋鈕之前添加，讓旋鈕渲染在上層
        // 帶黑色外框，類似 259m 風格
        addChild(new OutlinedTextLabel(Vec(7, 80), Vec(50, 10), "X", 80.f, nvgRGB(160, 160, 160), 2.f));
        addChild(new OutlinedTextLabel(Vec(7, 145), Vec(50, 10), "Y", 80.f, nvgRGB(160, 160, 160), 2.f));
        addChild(new OutlinedTextLabel(Vec(7, 215), Vec(50, 10), "Z", 80.f, nvgRGB(160, 160, 160), 2.f));
        std::string trackLabels[] = {"T", "X", "Y", "Z"};
        int trackParams[] = {0, DECAPyramid::X_PARAM_1, DECAPyramid::Y_PARAM_1, DECAPyramid::Z_PARAM_1};
        int trackInputs[] = {DECAPyramid::AUDIO_INPUT_1, DECAPyramid::X_CV_INPUT_1, DECAPyramid::Y_CV_INPUT_1, DECAPyramid::Z_CV_INPUT_1};
        float inputOffsets[] = {22, 42, 40, 40};

        for (int i = 0; i < 8; i++) {
            float baseX = 30.0f + i * 60.0f;
            
            NVGcolor trackColors[8] = {
                nvgRGB(10, 149, 149),   // Track 1: 深青色 +10
                nvgRGB(89, 121, 153),   // Track 2: 石墨藍 +10
                nvgRGB(112, 61, 163),   // Track 3: 雅緻紫 +10
                nvgRGB(194, 144, 21),   // Track 4: 深金黃 +10
                nvgRGB(117, 152, 45),   // Track 5: 橄欖綠 +10
                nvgRGB(10, 117, 73),    // Track 6: 深翠綠 +10
                nvgRGB(124, 57, 65),    // Track 7: 深酒紅 +10
                nvgRGB(152, 135, 200)   // Track 8: 深紫灰 +10
            };
            
            for (int j = 0; j < 4; j++) {
                NVGcolor labelColor = (j == 0) ? trackColors[i] : nvgRGB(255, 255, 255);
                addChild(new TechnoEnhancedTextLabel(Vec(baseX - 15, trackY[j]), Vec(30, 10), 
                    trackLabels[j] + (j == 0 ? std::to_string(i + 1) : ""), 8.f, labelColor, true));
                
                if (j == 0) {
                    addInput(createInputCentered<PJ301MPort>(Vec(baseX, trackY[j] + inputOffsets[j]), module, trackInputs[j] + i * 4));
                } else {
                    StandardBlackKnob26* knob = createParamCentered<StandardBlackKnob26>(Vec(baseX, trackY[j] + 10), module, trackParams[j] + i * 7);
                    xyzKnobs[i][j - 1] = knob;  // j-1: 1->0(X), 2->1(Y), 3->2(Z)
                    addParam(knob);
                    addInput(createInputCentered<PJ301MPort>(Vec(baseX, trackY[j] + inputOffsets[j]), module, trackInputs[j] + i * 4));
                }
            }
            
            // Add Insert Send and Return
            addOutput(createOutputCentered<PJ301MPort>(Vec(baseX - 15, 20), module, DECAPyramid::INSERT_SEND_1 + i));
            addInput(createInputCentered<PJ301MPort>(Vec(baseX + 15, 20), module, DECAPyramid::INSERT_RETURN_1 + i));
            
            float leftKnobX = baseX - 15.0f;
            float rightKnobX = baseX + 15.0f;
            
            struct KnobLayout {
                float x, y;
                std::string label;
                int paramId;
            };
            
            KnobLayout knobLayouts[] = {
                {leftKnobX, 285, "LVL", DECAPyramid::LEVEL_PARAM_1 + i * 7},
                {rightKnobX, 285, "FLT", DECAPyramid::FILTER_PARAM_1 + i * 7},
                {leftKnobX, 315, "SDA", DECAPyramid::SENDA_PARAM_1 + i * 7},
                {rightKnobX, 315, "SDB", DECAPyramid::SENDB_PARAM_1 + i * 7}
            };
            
            for (auto& knob : knobLayouts) {
                addChild(new TechnoEnhancedTextLabel(Vec(knob.x - 15, knob.y - 10), Vec(30, 10), knob.label, 8.f, nvgRGB(255, 255, 255), true));
                addParam(createParamCentered<StandardBlackKnob26>(Vec(knob.x, knob.y), module, knob.paramId));
            }
            
            DECAPyramid3DDisplay* display3D = new DECAPyramid3DDisplay();
            display3D->box.pos = Vec(baseX - 30, 330);
            display3D->module = module;
            display3D->moduleWidget = this;
            display3D->trackIndex = i;
            addChild(display3D);
        }
        
        VolumeMeterWidget* volumeMeterPre = new VolumeMeterWidget();
        volumeMeterPre->box.pos = Vec(10, 60);
        volumeMeterPre->module = module;
        volumeMeterPre->trackIndex = 0;
        volumeMeterPre->preLevel = true;
        addChild(volumeMeterPre);
        
        VolumeMeterWidget* volumeMeterPost = new VolumeMeterWidget();
        volumeMeterPost->box.pos = Vec(45, 60);
        volumeMeterPost->module = module;
        volumeMeterPost->trackIndex = 0;
        volumeMeterPost->preLevel = false;
        addChild(volumeMeterPost);
        
        for (int i = 1; i < 8; i++) {
            float baseX = 30.0f + i * 60.0f;
            
            VolumeMeterWidget* volumeMeterPreTrack = new VolumeMeterWidget();
            volumeMeterPreTrack->box.pos = Vec(baseX - 20, 60);
            volumeMeterPreTrack->module = module;
            volumeMeterPreTrack->trackIndex = i;
            volumeMeterPreTrack->preLevel = true;
            addChild(volumeMeterPreTrack);
            
            VolumeMeterWidget* volumeMeterPostTrack = new VolumeMeterWidget();
            volumeMeterPostTrack->box.pos = Vec(baseX + 15, 60);
            volumeMeterPostTrack->module = module;
            volumeMeterPostTrack->trackIndex = i;
            volumeMeterPostTrack->preLevel = false;
            addChild(volumeMeterPostTrack);
        }
        
        struct IOLayout {
            float x, y;
            std::string label;
            int ioId;
            bool isOutput;
        };
        
        IOLayout ioLayouts[] = {
            {524, 285, "", DECAPyramid::SENDA_OUTPUT, true},
            {555, 285, "", DECAPyramid::RETURN_AL_INPUT, false},
            {586, 285, "", DECAPyramid::RETURN_AR_INPUT, false},
            {524, 315, "", DECAPyramid::SENDB_OUTPUT, true},
            {555, 315, "", DECAPyramid::RETURN_BL_INPUT, false},
            {586, 315, "", DECAPyramid::RETURN_BR_INPUT, false}
        };
        
        for (auto& io : ioLayouts) {
            if (io.isOutput) {
                addOutput(createOutputCentered<PJ301MPort>(Vec(io.x, io.y), module, io.ioId));
            } else {
                addInput(createInputCentered<PJ301MPort>(Vec(io.x, io.y), module, io.ioId));
            }
        }
        
        addChild(new TechnoEnhancedTextLabel(Vec(478, 285), Vec(30, 10), "AUX A", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(478, 315), Vec(30, 10), "AUX B", 8.f, nvgRGB(255, 255, 255), true));
        
        DECAPyramidMasterDisplay* masterDisplay = new DECAPyramidMasterDisplay();
        masterDisplay->box.pos = Vec(490, 145);
        masterDisplay->module = module;
        masterDisplay->moduleWidget = this;
        addChild(masterDisplay);
        
        // Add seven new knobs above the 3D display, aligned with outputs below
        // Top row: RTN A/B controls
        addChild(new TechnoEnhancedTextLabel(Vec(478, 40), Vec(30, 10), "RTN A", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(478, 50), Vec(30, 10), "LVL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(493, 75), module, DECAPyramid::RTN_A_LEVEL_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(509, 40), Vec(30, 10), "RTN A", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(509, 50), Vec(30, 10), "FLT", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(524, 75), module, DECAPyramid::RTN_A_FILTER_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(540, 40), Vec(30, 10), "RTN B", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(540, 50), Vec(30, 10), "LVL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(555, 75), module, DECAPyramid::RTN_B_LEVEL_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(571, 40), Vec(30, 10), "RTN B", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new TechnoEnhancedTextLabel(Vec(571, 50), Vec(30, 10), "FLT", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(586, 75), module, DECAPyramid::RTN_B_FILTER_PARAM));

        // Bottom row: Output controls, aligned with output jacks below
        addChild(new TechnoEnhancedTextLabel(Vec(478, 125), Vec(30, 10), "OUTPUT", 8.f, nvgRGB(255, 255, 255), true));

        addChild(new TechnoEnhancedTextLabel(Vec(509, 100), Vec(30, 10), "1-4", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(524, 125), module, DECAPyramid::OUTPUT_1_4_LEVEL_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(540, 100), Vec(30, 10), "5-8", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(555, 125), module, DECAPyramid::OUTPUT_5_8_LEVEL_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(571, 100), Vec(30, 10), "MASTER", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(586, 125), module, DECAPyramid::MASTER_OUTPUT_LEVEL_PARAM));
        
        for (int i = 0; i < 8; i++) {
            float outputX = 13 + (i % 4) * 31 + 480;
            float outputY = (i < 4) ? 343 : 368;
            addOutput(createOutputCentered<PJ301MPort>(Vec(outputX, outputY), module, DECAPyramid::MASTER_OUTPUT_1 + i));
        }
    }
    
    void step() override {
        DECAPyramid* module = getModule<DECAPyramid>();
        if (module) {
            panelThemeHelper.step(module);

            // CV 調變顯示更新
            for (int track = 0; track < 8; track++) {
                for (int axis = 0; axis < 3; axis++) {
                    StandardBlackKnob26* knob = xyzKnobs[track][axis];
                    if (knob) {
                        // axis: 0=X, 1=Y, 2=Z
                        int inputId = DECAPyramid::X_CV_INPUT_1 + axis + track * 4;
                        bool connected = module->inputs[inputId].isConnected();
                        knob->setModulationEnabled(connected);
                        if (connected) {
                            knob->setModulation(module->cvMod[track][axis]);
                        }
                    }
                }
            }
        }
        ModuleWidget::step();
    }
    
    void appendContextMenu(Menu* menu) override {
        DECAPyramid* module = getModule<DECAPyramid>();
        if (!module) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createBoolPtrMenuItem("Send Pre-Level", "", &module->sendPreLevel));

        addPanelThemeMenu(menu, module);
    }
};

Model* modelDECAPyramid = createModel<DECAPyramid, DECAPyramidWidget>("DECAPyramid");