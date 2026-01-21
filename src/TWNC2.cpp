#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

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

// TechnoStandardBlackKnob now using TechnoStandardBlackKnob30 from widgets/Knobs.hpp

// SmallTechnoStandardBlackKnob now using SmallTechnoStandardBlackKnob from widgets/Knobs.hpp

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

struct BasicBandpassFilter {
    float x1 = 0.0f, x2 = 0.0f, x3 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f;
    float sampleRate = 44100.0f;
    float lastFreq = 1000.0f;
    float lastQ = 0.5f;
    
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    void setFrequency(float freq, float q = 0.5f) {
        lastFreq = clamp(freq, 20.0f, sampleRate * 0.45f);
        lastQ = q;
    }
    
    float process(float input) {
        float omega = 2.0f * M_PI * lastFreq / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float q = lastQ;
        
        if (q < 0.1f) q = 0.1f;
        
        float alpha = sin_omega / (2.0f * q);
        float norm = 1.0f / (1.0f + alpha);
        float b0 = alpha * norm;
        float b2 = -alpha * norm;
        float a1 = -2.0f * cos_omega * norm;
        float a2 = (1.0f - alpha) * norm;
        
        float output = b0 * input + b2 * x2 - a1 * y1 - a2 * y2;
        
        if (q > 1.5f) {
            float pole3_cutoff = lastFreq * 1.2f;
            float omega3 = 2.0f * M_PI * pole3_cutoff / sampleRate;
            float a3 = -std::cos(omega3);
            float b3 = (1.0f - std::cos(omega3)) / 2.0f;
            
            float stage3 = b3 * output + b3 * x3 - a3 * y3;
            x3 = output;
            y3 = stage3;
            
            float blend = (q - 1.5f) / 1.5f;
            output = output * (1.0f - blend) + stage3 * blend;
        }
        
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        return output;
    }
    
    void reset() {
        x1 = x2 = x3 = y1 = y2 = y3 = 0.0f;
    }
};

struct BasicSineVCO {
    float phase = 0.0f;
    float sampleRate = 44100.0f;
    
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    float process(float freq_hz, float fm_cv, float saturation = 1.0f) {
        float modulated_freq = freq_hz * std::pow(2.0f, fm_cv);
        modulated_freq = clamp(modulated_freq, 1.0f, sampleRate * 0.45f);
        
        float delta_phase = modulated_freq / sampleRate;
        
        phase += delta_phase;
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }
        
        float sine_wave = std::sin(2.0f * M_PI * phase);
        
        if (saturation > 1.0f) {
            sine_wave = std::tanh(sine_wave * saturation) / std::tanh(saturation);
        }
        
        return sine_wave * 5.0f;
    }
};

struct TWNC2 : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        KICK_VOLUME_PARAM,
        KICK_FREQ_PARAM,
        KICK_FM_AMT_PARAM,
        KICK_PUNCH_PARAM,
        SNARE_VOLUME_PARAM,
        SNARE_FREQ_PARAM,
        SNARE_NOISE_TONE_PARAM,
        SNARE_NOISE_MIX_PARAM,
        HATS_VOLUME_PARAM,
        HATS_TONE_PARAM,
        HATS_DECAY_PARAM,
        DUCK_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        KICK_ENV_INPUT,
        KICK_ACCENT_INPUT,
        KICK_FREQ_CV_INPUT,
        KICK_FM_CV_INPUT,
        KICK_PUNCH_CV_INPUT,
        SNARE_ENV_INPUT,
        SNARE_NOISE_MIX_CV_INPUT,
        HATS_ENV_INPUT,
        HATS_DECAY_CV_INPUT,
        EXTERNAL_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        KICK_OUTPUT,
        SNARE_OUTPUT,
        HATS_OUTPUT1,
        MIX_OUTPUT_L,
        MIX_OUTPUT_R,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };
    
    BasicSineVCO kickVCO;
    BasicSineVCO snareVCO;
    BasicBandpassFilter snareNoiseFilter;
    BasicBandpassFilter hatsFilter;

    // CV 調變顯示用
    float kickFreqCvMod = 0.0f;
    float kickFmCvMod = 0.0f;
    float kickPunchCvMod = 0.0f;
    float snareNoiseMixCvMod = 0.0f;
    float hatsDecayCvMod = 0.0f;
    
    struct HatsOscillator {
        float phases[6] = {0.0f};
        float sampleRate = 44100.0f;
        float offsets[6] = {100.0f, 250.0f, 400.0f, 550.0f, 600.0f, 1000.0f};
        
        void setSampleRate(float sr) {
            sampleRate = sr;
        }
        
        float process(float baseFreq) {
            float output = 0.0f;
            for (int i = 0; i < 6; i++) {
                float freq = baseFreq + offsets[i];
                float phaseInc = freq / sampleRate;
                phases[i] += phaseInc;
                if (phases[i] >= 1.0f) phases[i] -= 1.0f;
                
                float triangle;
                if (phases[i] < 0.5f) {
                    triangle = 4.0f * phases[i] - 1.0f;
                } else {
                    triangle = 3.0f - 4.0f * phases[i];
                }
                output += triangle * 5.0f/6.0f;
            }
            return output;
        }
    } hatsOsc;
    
    struct DelayLine {
        static const int maxDelay = 1440;
        float buffer[maxDelay] = {0.0f};
        int writePos = 0;
        float sampleRate = 44100.0f;
        
        void setSampleRate(float sr) {
            sampleRate = sr;
        }
        
        float process(float input, float delayMs) {
            int delaySamples = (int)(delayMs * sampleRate / 1000.0f);
            delaySamples = clamp(delaySamples, 0, maxDelay - 1);
            
            buffer[writePos] = input;
            int readPos = (writePos - delaySamples + maxDelay) % maxDelay;
            float output = buffer[readPos];
            
            writePos = (writePos + 1) % maxDelay;
            return output;
        }
    } hatsDelay;

    TWNC2() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(KICK_ENV_INPUT, "Kick Envelope CV");
        configInput(KICK_ACCENT_INPUT, "Kick Accent CV");
        configInput(KICK_FREQ_CV_INPUT, "Kick Frequency CV");
        configInput(KICK_FM_CV_INPUT, "Kick FM CV");
        configInput(KICK_PUNCH_CV_INPUT, "Kick Punch CV");
        configInput(SNARE_ENV_INPUT, "Snare Envelope CV");
        configInput(SNARE_NOISE_MIX_CV_INPUT, "Snare Noise Mix CV");
        configInput(HATS_ENV_INPUT, "Hats Envelope CV");
        configInput(EXTERNAL_INPUT, "External Input");
        
        configParam(KICK_VOLUME_PARAM, 0.0f, 1.0f, 1.0f, "Kick Volume");
        configParam(KICK_FREQ_PARAM, std::log2(24.0f), std::log2(500.0f), 4.5849623680114746f, "Kick Frequency", " Hz", 2.f);
        configParam(KICK_FM_AMT_PARAM, 0.0f, 1.0f, 0.15700007975101471f, "Kick FM Amount");
        configParam(KICK_PUNCH_PARAM, 0.0f, 1.0f, 0.16800001263618469f, "Kick Punch Amount");
        
        configParam(SNARE_VOLUME_PARAM, 0.0f, 1.0f, 1.0f, "Snare Volume");
        configParam(SNARE_FREQ_PARAM, std::log2(100.0f), std::log2(300.0f), 6.9100170135498047f, "Snare Frequency", " Hz", 2.f);
        configParam(SNARE_NOISE_TONE_PARAM, 0.0f, 1.0f, 0.71700006723403931f, "Snare Noise Tone");
        configParam(SNARE_NOISE_MIX_PARAM, 0.0f, 1.0f, 0.28799989819526672f, "Snare Noise Mix");
        
        configParam(HATS_VOLUME_PARAM, 0.0f, 1.0f, 1.0f, "Hats Volume");
        configParam(HATS_TONE_PARAM, 0.0f, 1.0f, 0.9649999737739563f, "Hats Tone");
        configParam(HATS_DECAY_PARAM, 0.0f, 1.0f, 0.0f, "Hats Decay");
        configParam(DUCK_PARAM, 0.0f, 1.0f, 0.0f, "Duck Amount");
        
        configOutput(KICK_OUTPUT, "Kick Audio");
        configOutput(SNARE_OUTPUT, "Snare Audio");
        configOutput(HATS_OUTPUT1, "Hats Audio 1");
        configOutput(MIX_OUTPUT_L, "Mix Output L");
        configOutput(MIX_OUTPUT_R, "Mix Output R");
        
        kickVCO.setSampleRate(44100.0f);
        snareVCO.setSampleRate(44100.0f);
        snareNoiseFilter.setSampleRate(44100.0f);
        hatsFilter.setSampleRate(44100.0f);
        hatsOsc.setSampleRate(44100.0f);
        hatsDelay.setSampleRate(44100.0f);
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        kickVCO.setSampleRate(sr);
        snareVCO.setSampleRate(sr);
        snareNoiseFilter.setSampleRate(sr);
        hatsFilter.setSampleRate(sr);
        hatsOsc.setSampleRate(sr);
        hatsDelay.setSampleRate(sr);
    }

    void onReset() override {
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) {
            panelTheme = json_integer_value(themeJ);
        }
        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) {
            panelContrast = json_real_value(contrastJ);
        }
    }

    void process(const ProcessArgs& args) override {
        float kickEnvCV = clamp(inputs[KICK_ENV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        float kickAccentCV = clamp(inputs[KICK_ACCENT_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        float duckAmount = params[DUCK_PARAM].getValue();
        float sidechainCV = 1.0f - (kickAccentCV * duckAmount * 3.0f);
        
        float kickVolumeParam = params[KICK_VOLUME_PARAM].getValue();
        float kickPunchAmount = params[KICK_PUNCH_PARAM].getValue();
        if (inputs[KICK_PUNCH_CV_INPUT].isConnected()) {
            float cv = inputs[KICK_PUNCH_CV_INPUT].getVoltage();
            kickPunchAmount += cv / 10.0f;
            kickPunchAmount = clamp(kickPunchAmount, 0.0f, 1.0f);
            kickPunchCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            kickPunchCvMod = 0.0f;
        }

        float kickFmAmount = params[KICK_FM_AMT_PARAM].getValue() * 20.0f;
        if (inputs[KICK_FM_CV_INPUT].isConnected()) {
            float cv = inputs[KICK_FM_CV_INPUT].getVoltage();
            kickFmAmount += (cv / 10.0f) * 20.0f;
            kickFmAmount = clamp(kickFmAmount, 0.0f, 20.0f);
            kickFmCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            kickFmCvMod = 0.0f;
        }

        float kickFreqParam = std::pow(2.0f, params[KICK_FREQ_PARAM].getValue());
        if (inputs[KICK_FREQ_CV_INPUT].isConnected()) {
            float cv = inputs[KICK_FREQ_CV_INPUT].getVoltage();
            float freqCV = params[KICK_FREQ_PARAM].getValue() + cv;
            kickFreqParam = std::pow(2.0f, freqCV);
            kickFreqParam = clamp(kickFreqParam, std::pow(2.0f, std::log2(24.0f)), std::pow(2.0f, std::log2(500.0f)));
            kickFreqCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            kickFreqCvMod = 0.0f;
        }
        
        float kickFmCV = kickEnvCV * kickEnvCV;
        float kickVcaCV = std::sqrt(kickEnvCV);
        
        float kickEnvelopeFM = kickFmCV * kickFmAmount;
        float kickSaturation = 1.0f + (kickPunchAmount * 4.0f);
        float kickAudioOutput = kickVCO.process(kickFreqParam, kickEnvelopeFM, kickSaturation);
        float kickFinalOutput = kickAudioOutput * kickVcaCV * kickAccentCV * kickVolumeParam * 0.8f;
        
        float snareEnvCV = clamp(inputs[SNARE_ENV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        
        float snareVolumeParam = params[SNARE_VOLUME_PARAM].getValue();
        float snareNoiseTone = params[SNARE_NOISE_TONE_PARAM].getValue();
        float snareNoiseMix = params[SNARE_NOISE_MIX_PARAM].getValue();
        if (inputs[SNARE_NOISE_MIX_CV_INPUT].isConnected()) {
            float cv = inputs[SNARE_NOISE_MIX_CV_INPUT].getVoltage();
            snareNoiseMix += cv / 10.0f;
            snareNoiseMix = clamp(snareNoiseMix, 0.0f, 1.0f);
            snareNoiseMixCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            snareNoiseMixCvMod = 0.0f;
        }
        
        float snareBaseFreq = std::pow(2.0f, params[SNARE_FREQ_PARAM].getValue());
        float snareVcaCV = std::sqrt(snareEnvCV);
        
        float snareBodyOutput = snareVCO.process(snareBaseFreq, 0.0f) * 0.75f;
        
        float snareNoiseRaw = (std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        float baseFilterFreq = snareBaseFreq * 5.0f;
        float noiseFilterFreq = baseFilterFreq + (snareNoiseTone * 5000.0f) + (snareEnvCV * 2000.0f);
        
        snareNoiseFilter.setFrequency(noiseFilterFreq, 0.5f);
        float snareNoiseFiltered = snareNoiseFilter.process(snareNoiseRaw) * 4.0f;
        
        float snareMixedOutput = (snareBodyOutput * (1.0f - snareNoiseMix)) + (snareNoiseFiltered * snareNoiseMix);
        float sidechain = 0.02f + (sidechainCV * 0.98f);
        float snareFinalOutput = snareMixedOutput * snareVcaCV * snareVolumeParam * sidechain * 4.0f;
        
        float bitRange = 1024.0f;
        
        auto processLimiter = [](float input) -> float {
            const float threshold = 5.0f;  // 降低閾值
            if (input > threshold) {
                return threshold + std::tanh((input - threshold) * 0.5f) * 2.0f;  // 軟限制
            } else if (input < -threshold) {
                return -threshold + std::tanh((input + threshold) * 0.5f) * 2.0f;
            }
            return input;
        };
        
        float kickQuantized = std::round(kickFinalOutput * bitRange) / bitRange;
        float snareQuantized = std::round(snareFinalOutput * bitRange) / bitRange;
        
        outputs[KICK_OUTPUT].setVoltage(kickQuantized);
        outputs[SNARE_OUTPUT].setVoltage(snareQuantized);
        
        float hatsEnvCV = clamp(inputs[HATS_ENV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        
        float hatsVolumeParam = params[HATS_VOLUME_PARAM].getValue();
        float hatsTone = params[HATS_TONE_PARAM].getValue();
        float hatsSpread = 20.0f;
        float hatsDecay = params[HATS_DECAY_PARAM].getValue();
        if (inputs[HATS_DECAY_CV_INPUT].isConnected()) {
            float cv = inputs[HATS_DECAY_CV_INPUT].getVoltage();
            hatsDecay += cv / 10.0f;
            hatsDecay = clamp(hatsDecay, 0.0f, 1.0f);
            hatsDecayCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
        } else {
            hatsDecayCvMod = 0.0f;
        }
        
        float hatsBaseFreq = 1000.0f + (hatsTone * 4500.0f);
        float hatsSquareWave = hatsOsc.process(hatsBaseFreq);
        
        float hatsFilterFreq = hatsBaseFreq + (hatsTone * 4000.0f);
        hatsFilter.setFrequency(hatsFilterFreq, 0.5f);
        float hatsFiltered = hatsFilter.process(hatsSquareWave);
        
        float hatsNoiseRaw = (std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        float hatsNoiseFiltered = snareNoiseFilter.process(hatsNoiseRaw);
        float hatsNoiseAmount = hatsDecay * 0.8f;
        
        float hatsMixed = hatsFiltered + (hatsNoiseFiltered * hatsNoiseAmount);
        
        float hatsVcaDecay = 2.0f - (hatsDecay * 1.5f);
        float hatsVcaCV = std::pow(hatsEnvCV, hatsVcaDecay);
        
        float hatsReducedSidechain = 0.8f + (sidechainCV * 0.2f);
        float hatsFinalOutput = hatsMixed * hatsVcaCV * hatsVolumeParam * hatsReducedSidechain * 0.7f;
        
        float hatsQuantized = std::round(hatsFinalOutput * bitRange) / bitRange;
        float hatsDelayed = hatsDelay.process(hatsQuantized, hatsSpread);
        
        outputs[HATS_OUTPUT1].setVoltage(hatsQuantized);
        
        float externalInput = inputs[EXTERNAL_INPUT].getVoltage();
        externalInput *= sidechain;
        
        float mixOutputL = kickQuantized + snareQuantized + hatsQuantized + externalInput;
        mixOutputL = processLimiter(mixOutputL);
        
        float mixOutputR = kickQuantized + snareQuantized + hatsDelayed + externalInput;
        mixOutputR = processLimiter(mixOutputR);

        outputs[MIX_OUTPUT_L].setVoltage(mixOutputL);
        outputs[MIX_OUTPUT_R].setVoltage(mixOutputR);
    }
};

struct TWNC2Widget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    madzine::widgets::TechnoStandardBlackKnob30* kickFreqKnob = nullptr;
    madzine::widgets::TechnoStandardBlackKnob30* kickFmKnob = nullptr;
    madzine::widgets::TechnoStandardBlackKnob30* kickPunchKnob = nullptr;
    madzine::widgets::TechnoStandardBlackKnob30* snareNoiseMixKnob = nullptr;
    madzine::widgets::TechnoStandardBlackKnob30* hatsDecayKnob = nullptr;

    TWNC2Widget(TWNC2* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "TWNC 2", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 24), Vec(box.size.x, 12), "Taiwan is never China", 8.f, nvgRGB(255, 200, 0), false));

        float track1Y = 35;
        addChild(new TechnoEnhancedTextLabel(Vec(30, track1Y + 5), Vec(15, 10), "BD", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 11), Vec(30, 10), "VOL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track1Y + 34), module, TWNC2::KICK_VOLUME_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 11), Vec(30, 10), "ENV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, track1Y + 34), module, TWNC2::KICK_ENV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 11), Vec(30, 10), "[ACCNT]", 8.f, nvgRGB(0, 0, 0), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, track1Y + 34), module, TWNC2::KICK_ACCENT_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 48), Vec(30, 10), "TUNE", 8.f, nvgRGB(255, 255, 255), true));
        kickFreqKnob = createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track1Y + 71), module, TWNC2::KICK_FREQ_PARAM);
        addParam(kickFreqKnob);

        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 48), Vec(30, 10), "FM", 8.f, nvgRGB(255, 255, 255), true));
        kickFmKnob = createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(60, track1Y + 71), module, TWNC2::KICK_FM_AMT_PARAM);
        addParam(kickFmKnob);

        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 48), Vec(30, 10), "PUNCH", 8.f, nvgRGB(255, 255, 255), true));
        kickPunchKnob = createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(100, track1Y + 71), module, TWNC2::KICK_PUNCH_PARAM);
        addParam(kickPunchKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 85), Vec(30, 10), "CV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(20, track1Y + 108), module, TWNC2::KICK_FREQ_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 85), Vec(30, 10), "CV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, track1Y + 108), module, TWNC2::KICK_FM_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 85), Vec(30, 10), "CV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, track1Y + 108), module, TWNC2::KICK_PUNCH_CV_INPUT));

        float track2Y = 150;
        addChild(new TechnoEnhancedTextLabel(Vec(27, track2Y + 5), Vec(25, 10), "SN", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track2Y + 11), Vec(30, 10), "VOL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track2Y + 34), module, TWNC2::SNARE_VOLUME_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track2Y + 11), Vec(30, 10), "ENV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, track2Y + 34), module, TWNC2::SNARE_ENV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track2Y + 11), Vec(30, 10), "N.BPF", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(100, track2Y + 34), module, TWNC2::SNARE_NOISE_TONE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track2Y + 48), Vec(30, 10), "TUNE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track2Y + 71), module, TWNC2::SNARE_FREQ_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track2Y + 48), Vec(30, 10), "N.MIX", 8.f, nvgRGB(255, 255, 255), true));
        snareNoiseMixKnob = createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(60, track2Y + 71), module, TWNC2::SNARE_NOISE_MIX_PARAM);
        addParam(snareNoiseMixKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track2Y + 48), Vec(30, 10), "N.MIX", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, track2Y + 71), module, TWNC2::SNARE_NOISE_MIX_CV_INPUT));

        float track3Y = 235;
        addChild(new TechnoEnhancedTextLabel(Vec(30, track3Y + 5), Vec(15, 10), "HH", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track3Y + 11), Vec(30, 10), "VOL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track3Y + 34), module, TWNC2::HATS_VOLUME_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track3Y + 11), Vec(30, 10), "ENV", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, track3Y + 34), module, TWNC2::HATS_ENV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track3Y + 11), Vec(30, 10), "TONE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(100, track3Y + 34), module, TWNC2::HATS_TONE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track3Y + 48), Vec(30, 10), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        hatsDecayKnob = createParamCentered<madzine::widgets::TechnoStandardBlackKnob30>(Vec(20, track3Y + 71), module, TWNC2::HATS_DECAY_PARAM);
        addParam(hatsDecayKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track3Y + 48), Vec(30, 10), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(60, track3Y + 71), module, TWNC2::HATS_DECAY_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track3Y + 48), Vec(30, 10), "EXT", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, track3Y + 71), module, TWNC2::EXTERNAL_INPUT));

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-4, 337), Vec(20, 15), "BD", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(22, 343), module, TWNC2::KICK_OUTPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(0, 362), Vec(20, 15), "[DUCK]", 5.f, nvgRGB(0, 0, 0), true));
        addParam(createParamCentered<madzine::widgets::MicrotuneKnob>(Vec(26, 368), module, TWNC2::DUCK_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(36, 337), Vec(20, 15), "SN", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(62, 343), module, TWNC2::SNARE_OUTPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(36, 362), Vec(20, 15), "L", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(62, 368), module, TWNC2::MIX_OUTPUT_L));

        addChild(new TechnoEnhancedTextLabel(Vec(73, 337), Vec(20, 15), "HH", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, 343), module, TWNC2::HATS_OUTPUT1));

        addChild(new TechnoEnhancedTextLabel(Vec(73, 362), Vec(20, 15), "R", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, 368), module, TWNC2::MIX_OUTPUT_R));
    }

    void step() override {
        TWNC2* module = dynamic_cast<TWNC2*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // 更新 CV 調變顯示
            auto updateKnob = [&](madzine::widgets::TechnoStandardBlackKnob30* knob, int inputId, float cvMod) {
                if (knob) {
                    bool connected = module->inputs[inputId].isConnected();
                    knob->setModulationEnabled(connected);
                    if (connected) knob->setModulation(cvMod);
                }
            };

            updateKnob(kickFreqKnob, TWNC2::KICK_FREQ_CV_INPUT, module->kickFreqCvMod);
            updateKnob(kickFmKnob, TWNC2::KICK_FM_CV_INPUT, module->kickFmCvMod);
            updateKnob(kickPunchKnob, TWNC2::KICK_PUNCH_CV_INPUT, module->kickPunchCvMod);
            updateKnob(snareNoiseMixKnob, TWNC2::SNARE_NOISE_MIX_CV_INPUT, module->snareNoiseMixCvMod);
            updateKnob(hatsDecayKnob, TWNC2::HATS_DECAY_CV_INPUT, module->hatsDecayCvMod);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        TWNC2* module = dynamic_cast<TWNC2*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelTWNC2 = createModel<TWNC2, TWNC2Widget>("TWNC2");