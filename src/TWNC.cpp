#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <vector>
#include <algorithm>

namespace ripples
{
struct SOSCoefficients
{
    float b[3];
    float a[2];
};

template <typename T, int max_num_sections>
class SOSFilter
{
public:
    SOSFilter()
    {
        Init(0);
    }
    SOSFilter(int num_sections)
    {
        Init(num_sections);
    }
    void Init(int num_sections)
    {
        num_sections_ = num_sections;
        Reset();
    }
    void Init(int num_sections, const SOSCoefficients* sections)
    {
        num_sections_ = num_sections;
        Reset();
        SetCoefficients(sections);
    }
    void Reset()
    {
        for (int n = 0; n < num_sections_; n++)
        {
            x_[n][0] = 0.f;
            x_[n][1] = 0.f;
            x_[n][2] = 0.f;
        }
        x_[num_sections_][0] = 0.f;
        x_[num_sections_][1] = 0.f;
        x_[num_sections_][2] = 0.f;
    }
    void SetCoefficients(const SOSCoefficients* sections)
    {
        for (int n = 0; n < num_sections_; n++)
        {
            sections_[n].b[0] = sections[n].b[0];
            sections_[n].b[1] = sections[n].b[1];
            sections_[n].b[2] = sections[n].b[2];
            sections_[n].a[0] = sections[n].a[0];
            sections_[n].a[1] = sections[n].a[1];
        }
    }
    T Process(T in)
    {
        for (int n = 0; n < num_sections_; n++)
        {
            x_[n][2] = x_[n][1];
            x_[n][1] = x_[n][0];
            x_[n][0] = in;
            T out = 0.f;
            out += sections_[n].b[0] * x_[n][0];
            out += sections_[n].b[1] * x_[n][1];
            out += sections_[n].b[2] * x_[n][2];
            out -= sections_[n].a[0] * x_[n+1][0];
            out -= sections_[n].a[1] * x_[n+1][1];
            in = out;
        }
        x_[num_sections_][2] = x_[num_sections_][1];
        x_[num_sections_][1] = x_[num_sections_][0];
        x_[num_sections_][0] = in;
        return in;
    }
protected:
    int num_sections_;
    SOSCoefficients sections_[max_num_sections];
    T x_[max_num_sections + 1][3];
};

template <typename T>
class AAFilter
{
public:
    void Init(float sample_rate)
    {
        InitFilter(sample_rate);
    }

    T ProcessUp(T in)
    {
        return up_filter_.Process(in);
    }

    T ProcessDown(T in)
    {
        return down_filter_.Process(in);
    }

    int GetOversamplingFactor(void)
    {
        return oversampling_factor_;
    }

protected:
    static constexpr int kMaxNumSections = 7;

    SOSFilter<T, kMaxNumSections> up_filter_;
    SOSFilter<T, kMaxNumSections> down_filter_;
    int oversampling_factor_;

    void InitFilter(float sample_rate)
    {
        const SOSCoefficients kFilter48000x3[6] = 
        {
            { {1.96007199e-04,  3.15285921e-04,  1.96007199e-04,  }, {-1.49750952e+00, 5.79487424e-01,  } },
            { {1.00000000e+00,  1.64502383e-01,  1.00000000e+00,  }, {-1.43900370e+00, 6.63196513e-01,  } },
            { {1.00000000e+00,  -5.92180251e-01, 1.00000000e+00,  }, {-1.36241892e+00, 7.75058824e-01,  } },
            { {1.00000000e+00,  -9.07488127e-01, 1.00000000e+00,  }, {-1.30223398e+00, 8.69165582e-01,  } },
            { {1.00000000e+00,  -1.04177534e+00, 1.00000000e+00,  }, {-1.26951947e+00, 9.34679234e-01,  } },
            { {1.00000000e+00,  -1.09276235e+00, 1.00000000e+00,  }, {-1.26454687e+00, 9.80322986e-01,  } },
        };
        
        up_filter_.Init(6, kFilter48000x3);
        down_filter_.Init(6, kFilter48000x3);
        oversampling_factor_ = 3;
    }
};

}

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

// TechnoStandardBlackKnob30 現在從 widgets/Knobs.hpp 引入

// TechnoSnapKnob30 現在從 widgets/Knobs.hpp 引入

struct TechnoDivMultParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        switch (value) {
            case 0: return "1/2x";
            case 1: return "1x";
            case 2: return "1.5x";
            case 3: return "2x";
            case 4: return "3x";
            default: return "1x";
        }
    }
};

struct VCAShiftParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int value = (int)std::round(getValue());
        return string::f("%d step", value);
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

std::vector<bool> generateTechnoEuclideanRhythm(int length, int fill, int shift) {
    std::vector<bool> pattern(length, false);
    if (fill == 0 || length == 0) return pattern;
    if (fill > length) fill = length;

    shift = shift % length;
    if (shift < 0) shift += length;

    for (int i = 0; i < fill; ++i) {
        int index = (int)std::floor((float)i * length / fill);
        pattern[index] = true;
    }
    
    std::rotate(pattern.begin(), pattern.begin() + shift, pattern.end());
    return pattern;
}

template <int QUALITY = 6>
struct PinkNoiseGenerator {
    int frame = -1;
    float values[QUALITY] = {};

    float process() {
        int lastFrame = frame;
        frame++;
        if (frame >= (1 << QUALITY))
            frame = 0;
        int diff = lastFrame ^ frame;

        float sum = 0.f;
        for (int i = 0; i < QUALITY; i++) {
            if (diff & (1 << i)) {
                values[i] = random::uniform() - 0.5f;
            }
            sum += values[i];
        }
        return sum;
    }
};

struct UnifiedEnvelope {
    dsp::SchmittTrigger trigTrigger;
    dsp::PulseGenerator trigPulse;
    float phase = 0.f;
    bool gateState = false;
    static constexpr float ATTACK_TIME = 0.001f;
    
    void reset() {
        trigTrigger.reset();
        trigPulse.reset();
        phase = 0.f;
        gateState = false;
    }
    
    float smoothDecayEnvelope(float t, float totalTime, float shapeParam) {
        if (t >= totalTime) return 0.f;
        
        float normalizedT = t / totalTime;
        
        float frontK = -0.9f + shapeParam * 0.5f;
        float backK = -1.0f + 1.6f * std::pow(shapeParam, 0.3f);
        
        float transition = normalizedT * normalizedT * (3.f - 2.f * normalizedT);
        float k = frontK + (backK - frontK) * transition;
        
        float absT = std::abs(normalizedT);
        float denominator = k - 2.f * k * absT + 1.f;
        if (std::abs(denominator) < 1e-10f) {
            return 1.f - normalizedT;
        }
        
        float curveResult = (normalizedT - k * normalizedT) / denominator;
        return 1.f - curveResult;
    }
    
    float process(float sampleTime, float triggerVoltage, float decayTime, float shapeParam) {
        bool triggered = trigTrigger.process(triggerVoltage, 0.1f, 2.f);
        
        if (triggered) {
            phase = 0.f;
            gateState = true;
            trigPulse.trigger(0.03f);
        }
        
        float envOutput = 0.f;
        
        if (gateState) {
            if (phase < ATTACK_TIME) {
                envOutput = phase / ATTACK_TIME;
            } else {
                float decayPhase = phase - ATTACK_TIME;
                
                if (decayPhase >= decayTime) {
                    gateState = false;
                    envOutput = 0.f;
                } else {
                    envOutput = smoothDecayEnvelope(decayPhase, decayTime, shapeParam);
                }
            }
            
            phase += sampleTime;
        }
        
        return clamp(envOutput, 0.f, 1.f);
    }
    
    float getTrigger(float sampleTime) {
        return trigPulse.process(sampleTime) ? 10.0f : 0.0f;
    }
};

struct OversampledSineVCO {
    float phase = 0.0f;
    float sampleRate = 44100.0f;
    ripples::AAFilter<float> aa_filter_;
    
    OversampledSineVCO() {
        setSampleRate(44100.0f);
    }
    
    void setSampleRate(float sr) {
        sampleRate = sr;
        aa_filter_.Init(sr);
    }
    
    float process(float freq_hz, float fm_cv) {
        int oversampling_factor = aa_filter_.GetOversamplingFactor();
        float output = 0.0f;
        
        for (int i = 0; i < oversampling_factor; i++) {
            float modulated_freq = freq_hz * std::pow(2.0f, fm_cv);
            modulated_freq = clamp(modulated_freq, 1.0f, sampleRate * oversampling_factor * 0.45f);
            
            float delta_phase = modulated_freq / (sampleRate * oversampling_factor);
            
            phase += delta_phase;
            if (phase >= 1.0f) {
                phase -= 1.0f;
            }
            
            float sine_wave = std::sin(2.0f * M_PI * phase);
            
            sine_wave = aa_filter_.ProcessUp(sine_wave);
            output = aa_filter_.ProcessDown(sine_wave);
        }
        
        return output * 5.0f;
    }
};

struct TWNC : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast; // -1 = Auto (follow VCV) // 0 = Sashimi, 1 = Boring

    enum ParamId {
        GLOBAL_LENGTH_PARAM,
        MANUAL_RESET_PARAM,
        TRACK1_FILL_PARAM,
        TRACK1_FREQ_PARAM,
        TRACK1_FM_AMT_PARAM,
        TRACK1_NOISE_MIX_PARAM,
        VCA_SHIFT_PARAM,
        VCA_DECAY_PARAM,
        TRACK1_DECAY_PARAM,
        TRACK1_SHAPE_PARAM,
        TRACK2_SHIFT_PARAM,
        TRACK2_FILL_PARAM,
        TRACK2_DIVMULT_PARAM,
        TRACK2_FREQ_PARAM,
        TRACK2_DECAY_PARAM,
        TRACK2_SHAPE_PARAM,
        TRACK2_NOISE_FM_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        GLOBAL_CLOCK_INPUT,
        RESET_INPUT,
        DRUM_FREQ_CV_INPUT,
        DRUM_DECAY_CV_INPUT,
        HATS_FREQ_CV_INPUT,
        HATS_DECAY_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        TRACK1_OUTPUT,
        TRACK2_OUTPUT,
        MAIN_VCA_ENV_OUTPUT,
        TRACK1_FM_ENV_OUTPUT,
        TRACK2_VCA_ENV_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        TRACK1_LIGHT,
        TRACK2_LIGHT,
        LIGHTS_LEN
    };

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger manualResetTrigger;
    
    float globalClockSeconds = 0.5f;
    float secondsSinceLastClock = -1.0f;
    
    dsp::PulseGenerator track1FlashPulse;
    dsp::PulseGenerator track2FlashPulse;

    // CV 調變顯示用
    float drumFreqCvMod = 0.0f;
    float drumDecayCvMod = 0.0f;
    float hatsFreqCvMod = 0.0f;
    float hatsDecayCvMod = 0.0f;
    
    OversampledSineVCO sineVCO;
    OversampledSineVCO sineVCO2;
    PinkNoiseGenerator<6> pinkNoiseGenerator;
    PinkNoiseGenerator<6> pinkNoiseGenerator2;
    float lastPink = 0.0f;
    float lastPink2 = 0.0f;

    struct QuarterNoteClock {
        int currentStep = 0;
        int shiftAmount = 1;
        dsp::PulseGenerator trigPulse;
        
        void reset() {
            currentStep = 0;
        }
        
        bool processStep(bool globalClockTriggered, int globalLength, int shift) {
            shiftAmount = shift;
            if (globalClockTriggered) {
                currentStep = (currentStep + 1) % 4;
                
                int targetStep = shiftAmount % 4;
                if (currentStep == targetStep) {
                    trigPulse.trigger(0.01f);
                    return true;
                }
            }
            return false;
        }
        
        float getTrigger(float sampleTime) {
            return trigPulse.process(sampleTime) ? 10.0f : 0.0f;
        }
    };

    struct TrackState {
        int divMultValue = 0;
        int division = 1;
        int multiplication = 1;
        float dividedClockSeconds = 0.5f;
        float multipliedClockSeconds = 0.5f;
        float dividedProgressSeconds = 0.0f;
        float gateSeconds = 0.0f;
        int dividerCount = 0;
        bool shouldStep = false;
        bool prevMultipliedGate = false;
        
        int currentStep = 0;
        int length = 16;
        int fill = 4;
        int shift = 0;
        std::vector<bool> pattern;
        bool gateState = false;
        dsp::PulseGenerator trigPulse;
        
        UnifiedEnvelope envelope;
        UnifiedEnvelope vcaEnvelope;

        void reset() {
            dividedProgressSeconds = 0.0f;
            dividerCount = 0;
            shouldStep = false;
            prevMultipliedGate = false;
            currentStep = 0;
            pattern.clear();
            gateState = false;
            envelope.reset();
            vcaEnvelope.reset();
        }
        
        void updateDivMult(int divMultParam) {
            divMultValue = divMultParam;
            switch (divMultParam) {
                case 0:
                    division = 2;
                    multiplication = 1;
                    break;
                case 1:
                    division = 1;
                    multiplication = 1;
                    break;
                case 2:
                    division = 2;
                    multiplication = 3;
                    break;
                case 3:
                    division = 1;
                    multiplication = 2;
                    break;
                case 4:
                    division = 1;
                    multiplication = 3;
                    break;
                default:
                    division = 1;
                    multiplication = 1;
                    break;
            }
        }
        
        bool processClockDivMult(bool globalClock, float globalClockSeconds, float sampleTime) {
            dividedClockSeconds = globalClockSeconds * (float)division;
            multipliedClockSeconds = dividedClockSeconds / (float)multiplication;
            gateSeconds = std::max(0.001f, multipliedClockSeconds * 0.5f);
            
            if (globalClock) {
                if (dividerCount < 1) {
                    dividedProgressSeconds = 0.0f;
                } else {
                    dividedProgressSeconds += sampleTime;
                }
                ++dividerCount;
                if (dividerCount >= division) {
                    dividerCount = 0;
                }
            } else {
                dividedProgressSeconds += sampleTime;
            }
            
            shouldStep = false;
            if (dividedProgressSeconds < dividedClockSeconds) {
                float multipliedProgressSeconds = dividedProgressSeconds / multipliedClockSeconds;
                multipliedProgressSeconds -= (float)(int)multipliedProgressSeconds;
                multipliedProgressSeconds *= multipliedClockSeconds;
                
                bool currentMultipliedGate = multipliedProgressSeconds <= gateSeconds;
                
                if (currentMultipliedGate && !prevMultipliedGate) {
                    shouldStep = true;
                }
                prevMultipliedGate = currentMultipliedGate;
            }
            
            return shouldStep;
        }
        
        void stepTrack() {
            currentStep = (currentStep + 1) % length;
            gateState = !pattern.empty() && pattern[currentStep];
            if (gateState) {
                trigPulse.trigger(0.01f);
            }
        }
    };
    TrackState tracks[2];
    QuarterNoteClock quarterClock;
    UnifiedEnvelope mainVCA;

    TWNC() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configInput(GLOBAL_CLOCK_INPUT, "Global Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(DRUM_FREQ_CV_INPUT, "Drum Frequency CV");
        configInput(DRUM_DECAY_CV_INPUT, "Drum Decay CV");
        configInput(HATS_FREQ_CV_INPUT, "Hats Frequency CV");
        configInput(HATS_DECAY_CV_INPUT, "Hats Decay CV");
        
        configParam(GLOBAL_LENGTH_PARAM, 1.0f, 32.0f, 32.0f, "Global Length");
        getParamQuantity(GLOBAL_LENGTH_PARAM)->snapEnabled = true;

        configParam(MANUAL_RESET_PARAM, 0.0f, 1.0f, 0.0f, "Manual Reset");

        configParam(TRACK1_FILL_PARAM, 0.0f, 100.0f, 84.500015258789062f, "Track 1 Fill", "%");
        configParam(TRACK1_FREQ_PARAM, std::log2(20.0f), std::log2(20000.0f), 5.1989173889160156f, "Track 1 Frequency", " Hz", 2.f);
        configParam(TRACK1_FM_AMT_PARAM, 0.0f, 1.0f, 0.65000015497207642f, "Track 1 FM Amount");
        configParam(TRACK1_NOISE_MIX_PARAM, 0.0f, 1.0f, 0.15200015902519226f, "Track 1 Noise Mix");
        
        configParam(VCA_SHIFT_PARAM, 1.0f, 7.0f, 1.0f, "VCA Shift");
        getParamQuantity(VCA_SHIFT_PARAM)->snapEnabled = true;
        delete paramQuantities[VCA_SHIFT_PARAM];
        paramQuantities[VCA_SHIFT_PARAM] = new VCAShiftParamQuantity;
        paramQuantities[VCA_SHIFT_PARAM]->module = this;
        paramQuantities[VCA_SHIFT_PARAM]->paramId = VCA_SHIFT_PARAM;
        paramQuantities[VCA_SHIFT_PARAM]->minValue = 1.0f;
        paramQuantities[VCA_SHIFT_PARAM]->maxValue = 7.0f;
        paramQuantities[VCA_SHIFT_PARAM]->defaultValue = 1.0f;
        paramQuantities[VCA_SHIFT_PARAM]->name = "VCA Shift";
        paramQuantities[VCA_SHIFT_PARAM]->snapEnabled = true;
        
        configParam(VCA_DECAY_PARAM, 0.01f, 2.0f, 0.39605924487113953f, "VCA Decay", " s");

        configParam(TRACK1_DECAY_PARAM, 0.01f, 2.0f, 0.72042977809906006f, "Track 1 Decay", " s");
        configParam(TRACK1_SHAPE_PARAM, 0.0f, 0.99f, 0.0f, "Track 1 Shape");

        configParam(TRACK2_SHIFT_PARAM, 0.0f, 7.0f, 2.0f, "Track 2 Shift");
        getParamQuantity(TRACK2_SHIFT_PARAM)->snapEnabled = true;
        configParam(TRACK2_FILL_PARAM, 0.0f, 100.0f, 25.0f, "Track 2 Fill", "%");
        configParam(TRACK2_DIVMULT_PARAM, 0.0f, 4.0f, 1.0f, "Track 2 Div/Mult");
        getParamQuantity(TRACK2_DIVMULT_PARAM)->snapEnabled = true;
        delete paramQuantities[TRACK2_DIVMULT_PARAM];
        paramQuantities[TRACK2_DIVMULT_PARAM] = new TechnoDivMultParamQuantity;
        paramQuantities[TRACK2_DIVMULT_PARAM]->module = this;
        paramQuantities[TRACK2_DIVMULT_PARAM]->paramId = TRACK2_DIVMULT_PARAM;
        paramQuantities[TRACK2_DIVMULT_PARAM]->minValue = 0.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->maxValue = 4.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->defaultValue = 1.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->name = "Track 2 Div/Mult";
        paramQuantities[TRACK2_DIVMULT_PARAM]->snapEnabled = true;
        
        configParam(TRACK2_FREQ_PARAM, std::log2(20.0f), std::log2(20000.0f), 14.287712097167969f, "Track 2 Frequency", " Hz", 2.f);
        configParam(TRACK2_DECAY_PARAM, 0.01f, 2.0f, 0.13139002025127411f, "Track 2 Decay", " s");
        configParam(TRACK2_SHAPE_PARAM, 0.0f, 0.99f, 0.055439997464418411f, "Track 2 Shape");
        configParam(TRACK2_NOISE_FM_PARAM, 0.0f, 1.0f, 0.71399968862533569f, "Track 2 Noise FM");
        
        configOutput(TRACK1_OUTPUT, "Track 1 Audio");
        configOutput(TRACK2_OUTPUT, "Track 2 Audio");
        configOutput(MAIN_VCA_ENV_OUTPUT, "Accent VCA Envelope");
        configOutput(TRACK1_FM_ENV_OUTPUT, "Track 1 FM Envelope");
        configOutput(TRACK2_VCA_ENV_OUTPUT, "Track 2 VCA Envelope");
        
        configLight(TRACK1_LIGHT, "Track 1 Light");
        configLight(TRACK2_LIGHT, "Track 2 Light");
        
        sineVCO.setSampleRate(44100.0f);
        sineVCO2.setSampleRate(44100.0f);
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        sineVCO.setSampleRate(sr);
        sineVCO2.setSampleRate(sr);
    }

    void onReset() override {
        secondsSinceLastClock = -1.0f;
        globalClockSeconds = 0.5f;
        for (int i = 0; i < 2; ++i) {
            tracks[i].reset();
        }
        quarterClock.reset();
        mainVCA.reset();
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
        bool globalClockActive = inputs[GLOBAL_CLOCK_INPUT].isConnected();
        bool globalClockTriggered = false;
        
        if (globalClockActive) {
            float clockVoltage = inputs[GLOBAL_CLOCK_INPUT].getVoltage();
            globalClockTriggered = clockTrigger.process(clockVoltage);
        }
        
        bool globalResetTriggered = false;
        bool manualResetTriggered = false;
        
        if (inputs[RESET_INPUT].isConnected()) {
            globalResetTriggered = resetTrigger.process(inputs[RESET_INPUT].getVoltage());
        }
        
        manualResetTriggered = manualResetTrigger.process(params[MANUAL_RESET_PARAM].getValue());
        
        if (globalResetTriggered || manualResetTriggered) {
            onReset();
            return;
        }
        
        if (globalClockTriggered) {
            if (secondsSinceLastClock > 0.0f) {
                globalClockSeconds = secondsSinceLastClock;
                globalClockSeconds = clamp(globalClockSeconds, 0.01f, 10.0f);
            }
            secondsSinceLastClock = 0.0f;
        }
        
        if (secondsSinceLastClock >= 0.0f) {
            secondsSinceLastClock += args.sampleTime;
        }

        int globalLength = (int)std::round(params[GLOBAL_LENGTH_PARAM].getValue());
        globalLength = clamp(globalLength, 1, 32);
        
        int vcaShift = (int)std::round(params[VCA_SHIFT_PARAM].getValue());
        bool vcaTriggered = quarterClock.processStep(globalClockTriggered, globalLength, vcaShift);
        float vcaTrigger = quarterClock.getTrigger(args.sampleTime);
        
        for (int i = 0; i < 2; ++i) {
            TrackState& track = tracks[i];
            
            if (i == 1) {
                int divMultParam = (int)std::round(params[TRACK2_DIVMULT_PARAM].getValue());
                track.updateDivMult(divMultParam);
            } else {
                track.updateDivMult(1);
            }

            track.length = globalLength;

            float fillParam = (i == 0) ? params[TRACK1_FILL_PARAM].getValue() : params[TRACK2_FILL_PARAM].getValue();
            float fillPercentage = clamp(fillParam, 0.0f, 100.0f);
            track.fill = (int)std::round((fillPercentage / 100.0f) * track.length);

            if (i == 1) {
                track.shift = (int)std::round(params[TRACK2_SHIFT_PARAM].getValue());
                track.shift = clamp(track.shift, 0, 7);
            } else {
                track.shift = 0;
            }

            track.pattern = generateTechnoEuclideanRhythm(track.length, track.fill, track.shift);

            bool trackClockTrigger = track.processClockDivMult(globalClockTriggered, globalClockSeconds, args.sampleTime);

            if (trackClockTrigger && !track.pattern.empty() && globalClockActive) {
                track.stepTrack();
            }
            
            if (i == 0) {
                float decayParam = params[TRACK1_DECAY_PARAM].getValue();
                if (inputs[DRUM_DECAY_CV_INPUT].isConnected()) {
                    float cv = inputs[DRUM_DECAY_CV_INPUT].getVoltage();
                    decayParam += cv / 10.0f;
                    decayParam = clamp(decayParam, 0.01f, 2.0f);
                    drumDecayCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
                } else {
                    drumDecayCvMod = 0.0f;
                }
                float shapeParam = params[TRACK1_SHAPE_PARAM].getValue();
                
                float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
                float envelopeOutput = track.envelope.process(args.sampleTime, triggerOutput, decayParam * 0.5f, shapeParam);
                
                float noiseMixParam = params[TRACK1_NOISE_MIX_PARAM].getValue();
                
                float pinkNoise = pinkNoiseGenerator.process() / 0.816f;
                float blueNoise = (pinkNoise - lastPink) / 0.705f;
                lastPink = pinkNoise;
                
                const float noiseGain = 5.f / std::sqrt(2.f);
                pinkNoise *= noiseGain * 0.8f;
                blueNoise *= noiseGain * 1.5f;
                
                float mixedNoise = pinkNoise * (1.0f - noiseMixParam) + blueNoise * noiseMixParam;
                
                float fmAmount = params[TRACK1_FM_AMT_PARAM].getValue();
                
                float freqParam = params[TRACK1_FREQ_PARAM].getValue();
                if (inputs[DRUM_FREQ_CV_INPUT].isConnected()) {
                    float cv = inputs[DRUM_FREQ_CV_INPUT].getVoltage();
                    freqParam += cv;
                    drumFreqCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
                } else {
                    drumFreqCvMod = 0.0f;
                }
                freqParam = std::pow(2.0f, freqParam);
                
                float envelopeFM = envelopeOutput * fmAmount * 4.0f;
                float noiseFM = mixedNoise * noiseMixParam * 0.5f;
                float totalFM = envelopeFM + noiseFM;
                
                float audioOutput = sineVCO.process(freqParam, totalFM);
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam, shapeParam);
                
                float vcaDecayParam = params[VCA_DECAY_PARAM].getValue();
                float mainVCAOutput = mainVCA.process(args.sampleTime, vcaTrigger, vcaDecayParam, 0.5f);
                
                float finalAudioOutput = audioOutput * vcaEnvelopeOutput * mainVCAOutput * 1.4f;
                outputs[TRACK1_OUTPUT].setVoltage(finalAudioOutput);
                
                outputs[MAIN_VCA_ENV_OUTPUT].setVoltage(mainVCAOutput * 10.0f);
                outputs[TRACK1_FM_ENV_OUTPUT].setVoltage(envelopeOutput * 10.0f);
                
                if (envelopeOutput > 0.1f || vcaEnvelopeOutput > 0.1f || mainVCAOutput > 0.1f) {
                    track1FlashPulse.trigger(0.03f);
                }
            } else {
                float decayParam = params[TRACK2_DECAY_PARAM].getValue();
                if (inputs[HATS_DECAY_CV_INPUT].isConnected()) {
                    float cv = inputs[HATS_DECAY_CV_INPUT].getVoltage();
                    decayParam += cv / 10.0f;
                    decayParam = clamp(decayParam, 0.01f, 2.0f);
                    hatsDecayCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
                } else {
                    hatsDecayCvMod = 0.0f;
                }
                float shapeParam = params[TRACK2_SHAPE_PARAM].getValue();
                
                float triggerOutput = track.trigPulse.process(args.sampleTime) ? 10.0f : 0.0f;
                
                float noiseFMParam = params[TRACK2_NOISE_FM_PARAM].getValue();
                float noiseBlend = 0.0f;
                
                if (noiseFMParam > 0.0f) {
                    float pinkNoise2 = pinkNoiseGenerator2.process() / 0.816f;
                    float blueNoise2 = (pinkNoise2 - lastPink2) / 0.705f;
                    lastPink2 = pinkNoise2;
                    
                    const float noiseGain2 = 5.f / std::sqrt(2.f);
                    pinkNoise2 *= noiseGain2 * 0.8f;
                    blueNoise2 *= noiseGain2 * 1.5f;
                    
                    float selectedNoise2 = (noiseFMParam < 0.5f) ? pinkNoise2 : blueNoise2;
                    noiseBlend = selectedNoise2 * noiseFMParam * 0.5f;
                }
                
                float freqParam = params[TRACK2_FREQ_PARAM].getValue();
                if (inputs[HATS_FREQ_CV_INPUT].isConnected()) {
                    float cv = inputs[HATS_FREQ_CV_INPUT].getVoltage();
                    freqParam += cv;
                    hatsFreqCvMod = clamp(cv / 10.0f, -1.0f, 1.0f);
                } else {
                    hatsFreqCvMod = 0.0f;
                }
                freqParam = std::pow(2.0f, freqParam);
                float audioOutput = sineVCO2.process(freqParam, noiseBlend);
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam * 0.5f, shapeParam);
                
                float finalAudioOutput = audioOutput * vcaEnvelopeOutput * 0.7f;
                outputs[TRACK2_OUTPUT].setVoltage(finalAudioOutput);
                
                outputs[TRACK2_VCA_ENV_OUTPUT].setVoltage(vcaEnvelopeOutput * 10.0f);
                
                if (vcaEnvelopeOutput > 0.1f) {
                    track2FlashPulse.trigger(0.03f);
                }
            }
        }
        
        lights[TRACK1_LIGHT].setBrightness(track1FlashPulse.process(args.sampleTime) ? 1.0f : 0.0f);
        lights[TRACK2_LIGHT].setBrightness(track2FlashPulse.process(args.sampleTime) ? 1.0f : 0.0f);
    }
};

struct TWNCWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;
    TechnoStandardBlackKnob30* drumFreqKnob = nullptr;
    TechnoStandardBlackKnob30* drumDecayKnob = nullptr;
    TechnoStandardBlackKnob30* hatsFreqKnob = nullptr;
    TechnoStandardBlackKnob30* hatsDecayKnob = nullptr;

    TWNCWidget(TWNC* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "TWNC", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 24), Vec(box.size.x, 12), "Taiwan is not China", 8.f, nvgRGB(255, 200, 0), false));

        addChild(new TechnoEnhancedTextLabel(Vec(5, 42), Vec(30, 15), "CLK", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 68), module, TWNC::GLOBAL_CLOCK_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, 42), Vec(30, 15), "LENGTH", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(60, 71), module, TWNC::GLOBAL_LENGTH_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, 42), Vec(30, 15), "RST", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, 68), module, TWNC::RESET_INPUT));
        addParam(createParamCentered<VCVButton>(Vec(100, 92), module, TWNC::MANUAL_RESET_PARAM));

        float track1Y = 87;
        addChild(new TechnoEnhancedTextLabel(Vec(52, track1Y + 10), Vec(15, 10), "Drum", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 20), Vec(30, 10), "FILL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(20, track1Y + 44), module, TWNC::TRACK1_FILL_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 20), Vec(30, 10), "FREQ", 8.f, nvgRGB(255, 255, 255), true));
        drumFreqKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(60, track1Y + 43), module, TWNC::TRACK1_FREQ_PARAM);
        addParam(drumFreqKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 20), Vec(30, 10), "FM", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(100, track1Y + 44), module, TWNC::TRACK1_FM_AMT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 58), Vec(30, 10), "NOISE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(20, track1Y + 82), module, TWNC::TRACK1_NOISE_MIX_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 58), Vec(30, 10), "ACCENT", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(60, track1Y + 82), module, TWNC::VCA_SHIFT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 58), Vec(30, 10), "DELAY", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(100, track1Y + 82), module, TWNC::VCA_DECAY_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 99), Vec(30, 10), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        drumDecayKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(20, track1Y + 123), module, TWNC::TRACK1_DECAY_PARAM);
        addParam(drumDecayKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 99), Vec(30, 10), "SHAPE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(60, track1Y + 123), module, TWNC::TRACK1_SHAPE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 99), Vec(30, 10), "OUT", 8.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, track1Y + 123), module, TWNC::TRACK1_OUTPUT));

        float track2Y = 228;
        addChild(new TechnoEnhancedTextLabel(Vec(48, track2Y + 2), Vec(25, 10), "HATs", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(0, track2Y + 14), Vec(30, 10), "SHIFT", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(15, track2Y + 38), module, TWNC::TRACK2_SHIFT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(30, track2Y + 14), Vec(30, 10), "FILL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(45, track2Y + 38), module, TWNC::TRACK2_FILL_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(60, track2Y + 14), Vec(30, 10), "D/M", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob30>(Vec(75, track2Y + 38), module, TWNC::TRACK2_DIVMULT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(90, track2Y + 14), Vec(30, 10), "NOISE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(105, track2Y + 38), module, TWNC::TRACK2_NOISE_FM_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(0, track2Y + 56), Vec(30, 10), "FREQ", 8.f, nvgRGB(255, 255, 255), true));
        hatsFreqKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(15, track2Y + 80), module, TWNC::TRACK2_FREQ_PARAM);
        addParam(hatsFreqKnob);

        addChild(new TechnoEnhancedTextLabel(Vec(30, track2Y + 56), Vec(30, 10), "DECAY", 8.f, nvgRGB(255, 255, 255), true));
        hatsDecayKnob = createParamCentered<TechnoStandardBlackKnob30>(Vec(45, track2Y + 80), module, TWNC::TRACK2_DECAY_PARAM);
        addParam(hatsDecayKnob);
        
        addChild(new TechnoEnhancedTextLabel(Vec(60, track2Y + 56), Vec(30, 10), "SHAPE", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob30>(Vec(75, track2Y + 80), module, TWNC::TRACK2_SHAPE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(90, track2Y + 56), Vec(30, 10), "OUT", 8.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, track2Y + 80), module, TWNC::TRACK2_OUTPUT));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-10, 329), Vec(30, 10), "D.F", 7.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(17, 343), module, TWNC::DRUM_FREQ_CV_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(18, 329), Vec(30, 10), "D.D", 7.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(47, 343), module, TWNC::DRUM_DECAY_CV_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(48, 329), Vec(30, 10), "H.F", 7.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(77, 343), module, TWNC::HATS_FREQ_CV_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(78, 329), Vec(30, 10), "H.D", 7.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(107, 343), module, TWNC::HATS_DECAY_CV_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(-3, 362), Vec(20, 6), "VCA", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(-3, 368), Vec(20, 6), "ENV", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 368), module, TWNC::MAIN_VCA_ENV_OUTPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(30, 360), Vec(30, 6), "DRUM", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(35, 366), Vec(20, 6), "FM", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(37, 372), Vec(20, 6), "ENV", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 368), module, TWNC::TRACK1_FM_ENV_OUTPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(69, 360), Vec(30, 6), "HATS", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(74, 366), Vec(20, 6), "VCA", 7.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(74, 372), Vec(20, 6), "ENV", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 368), module, TWNC::TRACK2_VCA_ENV_OUTPUT));
    }

    void step() override {
        TWNC* module = dynamic_cast<TWNC*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // 更新 CV 調變顯示
            auto updateKnob = [&](TechnoStandardBlackKnob30* knob, int inputId, float cvMod) {
                if (knob) {
                    bool connected = module->inputs[inputId].isConnected();
                    knob->setModulationEnabled(connected);
                    if (connected) knob->setModulation(cvMod);
                }
            };

            updateKnob(drumFreqKnob, TWNC::DRUM_FREQ_CV_INPUT, module->drumFreqCvMod);
            updateKnob(drumDecayKnob, TWNC::DRUM_DECAY_CV_INPUT, module->drumDecayCvMod);
            updateKnob(hatsFreqKnob, TWNC::HATS_FREQ_CV_INPUT, module->hatsFreqCvMod);
            updateKnob(hatsDecayKnob, TWNC::HATS_DECAY_CV_INPUT, module->hatsDecayCvMod);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        TWNC* module = dynamic_cast<TWNC*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelTWNC = createModel<TWNC, TWNCWidget>("TWNC");