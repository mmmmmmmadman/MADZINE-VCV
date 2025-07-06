#include "plugin.hpp"
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
    struct CascadedSOS
    {
        float sample_rate;
        int oversampling_factor;
        int num_sections;
        const SOSCoefficients* coeffs;
    };

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

static const float kFreqKnobMin = 20.f;
static const float kFreqKnobMax = 20000.f;
static const float kFreqKnobVoltage = std::log2f(kFreqKnobMax / kFreqKnobMin);

static const float kResInputR = 22e3f;
static const float kResKnobV = 12.f;
static const float kResKnobR = 62e3f;
static const float kResAmpR = 47e3f;
static const float kResAmpC = 560e-12f;

static const float kFilterMaxCutoff = kFreqKnobMax;
static const float kFilterCellR = 33e3f;
static const float kFilterCellRC = 1.f / (2.f * M_PI * kFilterMaxCutoff);
static const float kFilterCellC = kFilterCellRC / kFilterCellR;
static const float kFilterInputR = 100e3f;
static const float kFilterInputGain = kFilterCellR / kFilterInputR;
static const float kFilterCellSelfModulation = 0.01f;

static const float kFeedbackRt = 22e3f;
static const float kFeedbackRb = 1e3f;
static const float kFeedbackR = kFeedbackRt + kFeedbackRb;
static const float kFeedbackGain = kFeedbackRb / kFeedbackR;

static const float kFeedforwardRt = 300e3f;
static const float kFeedforwardRb = 1e3f;
static const float kFeedforwardR = kFeedforwardRt + kFeedforwardRb;
static const float kFeedforwardGain = kFeedforwardRb / kFeedforwardR;
static const float kFeedforwardC = 220e-9f;

static const float kBP2Gain = -100e3f / 39e3f;

static const float kVtoICollectorVSat = -10.f;
static const float kOpampSatV = 10.6f;

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

struct TechnoStandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    TechnoStandardBlackKnob() {
        box.size = Vec(30, 30);
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
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
    
    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = true;
            e.consume(this);
        }
        else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            isDragging = false;
        }
        ParamWidget::onButton(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!isDragging || !pq) return;
        
        float sensitivity = 0.002f;
        float deltaY = -e.mouseDelta.y;
        
        float range = pq->getMaxValue() - pq->getMinValue();
        float currentValue = pq->getValue();
        float newValue = currentValue + deltaY * sensitivity * range;
        newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
        
        pq->setValue(newValue);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        pq->reset();
        e.consume(this);
    }
};

struct TechnoSnapKnob : ParamWidget {
    float accumDelta = 0.0f;
    
    TechnoSnapKnob() {
        box.size = Vec(30, 30);
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
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, lineX, lineY, 2.0f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFill(args.vg);
    }
    
    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            accumDelta = 0.0f;
            e.consume(this);
        }
        ParamWidget::onButton(e);
    }
    
    void onDragMove(const event::DragMove& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        accumDelta += (e.mouseDelta.x - e.mouseDelta.y);
        
        float threshold = 15.0f;
        
        if (accumDelta >= threshold) {
            float currentValue = pq->getValue();
            float newValue = currentValue + 1.0f;
            newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
            pq->setValue(newValue);
            accumDelta = 0.0f;
        }
        else if (accumDelta <= -threshold) {
            float currentValue = pq->getValue();
            float newValue = currentValue - 1.0f;
            newValue = clamp(newValue, pq->getMinValue(), pq->getMaxValue());
            pq->setValue(newValue);
            accumDelta = 0.0f;
        }
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        pq->reset();
        e.consume(this);
    }
};

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

template <int QUALITY = 8>
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

struct SimpleLPG {
    dsp::SchmittTrigger trigger;
    dsp::BiquadFilter lpf;
    UnifiedEnvelope envelope;
    float sampleRate = 44100.0f;
    
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    void reset() {
        trigger.reset();
        envelope.reset();
    }
    
    float process(float triggerInput, float decayParam, float input, float vcaAmount, float sampleTime) {
        float decayTime = 0.001f + decayParam * 0.399f;
        float shapeParam = 0.5f;
        
        float env = envelope.process(sampleTime, triggerInput, decayTime, shapeParam);
        
        float cutoffFreq = 200.0f + env * 18000.0f;
        lpf.setParameters(dsp::BiquadFilter::LOWPASS, cutoffFreq / sampleRate, 0.707f, 1.0f);
        
        float filtered = lpf.process(input);
        float level = vcaAmount * env;
        return filtered * level;
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
    
    OversampledSineVCO sineVCO;
    OversampledSineVCO sineVCO2;
    PinkNoiseGenerator<8> pinkNoiseGenerator;
    PinkNoiseGenerator<8> pinkNoiseGenerator2;
    float lastPink = 0.0f;
    float lastPink2 = 0.0f;
    SimpleLPG lpg;

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
        
        configParam(GLOBAL_LENGTH_PARAM, 1.0f, 32.0f, 16.0f, "Global Length");
        getParamQuantity(GLOBAL_LENGTH_PARAM)->snapEnabled = true;
        
        configParam(MANUAL_RESET_PARAM, 0.0f, 1.0f, 0.0f, "Manual Reset");
        
        configParam(TRACK1_FILL_PARAM, 0.0f, 100.0f, 25.0f, "Track 1 Fill", "%");
        configParam(TRACK1_FREQ_PARAM, std::log2(kFreqKnobMin), std::log2(kFreqKnobMax), std::log2(1000.0f), "Track 1 Frequency", " Hz", 2.f);
        configParam(TRACK1_FM_AMT_PARAM, 0.0f, 1.0f, 0.5f, "Track 1 FM Amount");
        configParam(TRACK1_NOISE_MIX_PARAM, 0.0f, 1.0f, 0.5f, "Track 1 Noise Mix");
        
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
        
        configParam(VCA_DECAY_PARAM, 0.01f, 2.0f, 0.3f, "VCA Decay", " s");
        
        configParam(TRACK1_DECAY_PARAM, 0.01f, 2.0f, 0.3f, "Track 1 Decay", " s");
        configParam(TRACK1_SHAPE_PARAM, 0.0f, 0.99f, 0.5f, "Track 1 Shape");
        
        configParam(TRACK2_SHIFT_PARAM, 0.0f, 7.0f, 0.0f, "Track 2 Shift");
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
        paramQuantities[TRACK2_DIVMULT_PARAM]->defaultValue = 0.0f;
        paramQuantities[TRACK2_DIVMULT_PARAM]->name = "Track 2 Div/Mult";
        paramQuantities[TRACK2_DIVMULT_PARAM]->snapEnabled = true;
        
        configParam(TRACK2_FREQ_PARAM, std::log2(kFreqKnobMin), std::log2(kFreqKnobMax), std::log2(800.0f), "Track 2 Frequency", " Hz", 2.f);
        configParam(TRACK2_DECAY_PARAM, 0.01f, 2.0f, 0.3f, "Track 2 Decay", " s");
        configParam(TRACK2_SHAPE_PARAM, 0.0f, 0.99f, 0.5f, "Track 2 Shape");
        configParam(TRACK2_NOISE_FM_PARAM, 0.0f, 1.0f, 0.0f, "Track 2 Noise FM");
        
        configOutput(TRACK1_OUTPUT, "Track 1 Audio");
        configOutput(TRACK2_OUTPUT, "Track 2 Audio");
        configOutput(MAIN_VCA_ENV_OUTPUT, "Accent VCA Envelope");
        configOutput(TRACK1_FM_ENV_OUTPUT, "Track 1 FM Envelope");
        configOutput(TRACK2_VCA_ENV_OUTPUT, "Track 2 VCA Envelope");
        
        configLight(TRACK1_LIGHT, "Track 1 Light");
        configLight(TRACK2_LIGHT, "Track 2 Light");
        
        sineVCO.setSampleRate(44100.0f);
        sineVCO2.setSampleRate(44100.0f);
        lpg.setSampleRate(44100.0f);
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        sineVCO.setSampleRate(sr);
        sineVCO2.setSampleRate(sr);
        lpg.setSampleRate(sr);
    }

    void onReset() override {
        secondsSinceLastClock = -1.0f;
        globalClockSeconds = 0.5f;
        for (int i = 0; i < 2; ++i) {
            tracks[i].reset();
        }
        quarterClock.reset();
        mainVCA.reset();
        lpg.reset();
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
                
                float selectedNoise = (noiseMixParam < 0.5f) ? pinkNoise : blueNoise;
                float noiseFMAmount = noiseMixParam;
                float scaledNoiseInput = selectedNoise * noiseFMAmount;
                
                float fmAmount = params[TRACK1_FM_AMT_PARAM].getValue();
                float processedFM = lpg.process(triggerOutput, 0.001f + decayParam * 0.399f, scaledNoiseInput, fmAmount, args.sampleTime);
                
                float freqParam = std::pow(2.0f, params[TRACK1_FREQ_PARAM].getValue());
                float envelopeFM = envelopeOutput * fmAmount * 4.0f;
                float totalFM = envelopeFM + processedFM;
                
                float audioOutput = sineVCO.process(freqParam, totalFM);
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam, shapeParam);
                
                float vcaDecayParam = params[VCA_DECAY_PARAM].getValue();
                float mainVCAOutput = mainVCA.process(args.sampleTime, vcaTrigger, vcaDecayParam, 0.5f);
                
                float finalAudioOutput = audioOutput * vcaEnvelopeOutput * mainVCAOutput;
                outputs[TRACK1_OUTPUT].setVoltage(finalAudioOutput);
                
                outputs[MAIN_VCA_ENV_OUTPUT].setVoltage(mainVCAOutput * 10.0f);
                outputs[TRACK1_FM_ENV_OUTPUT].setVoltage(envelopeOutput * 10.0f);
                
                if (envelopeOutput > 0.1f || vcaEnvelopeOutput > 0.1f || mainVCAOutput > 0.1f) {
                    track1FlashPulse.trigger(0.03f);
                }
            } else {
                float decayParam = params[TRACK2_DECAY_PARAM].getValue();
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
                
                float freqParam = std::pow(2.0f, params[TRACK2_FREQ_PARAM].getValue());
                float audioOutput = sineVCO2.process(freqParam, noiseBlend);
                
                float vcaEnvelopeOutput = track.vcaEnvelope.process(args.sampleTime, triggerOutput, decayParam * 0.5f, shapeParam);
                
                float finalAudioOutput = audioOutput * vcaEnvelopeOutput;
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
    TWNCWidget(TWNC* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "TWNC", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 24), Vec(box.size.x, 12), "Taiwan is not China", 6.f, nvgRGB(255, 200, 0), false));

        addChild(new TechnoEnhancedTextLabel(Vec(5, 42), Vec(30, 15), "CLK", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(20, 68), module, TWNC::GLOBAL_CLOCK_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, 42), Vec(30, 15), "LENGTH", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob>(Vec(60, 71), module, TWNC::GLOBAL_LENGTH_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, 42), Vec(30, 15), "RST", 7.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(100, 68), module, TWNC::RESET_INPUT));
        addParam(createParamCentered<VCVButton>(Vec(100, 92), module, TWNC::MANUAL_RESET_PARAM));

        float track1Y = 87;
        addChild(new TechnoEnhancedTextLabel(Vec(52, track1Y + 10), Vec(15, 10), "Drum", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 20), Vec(30, 10), "FILL", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(20, track1Y + 44), module, TWNC::TRACK1_FILL_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 20), Vec(30, 10), "FREQ", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(60, track1Y + 43), module, TWNC::TRACK1_FREQ_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 20), Vec(30, 10), "FM", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(100, track1Y + 44), module, TWNC::TRACK1_FM_AMT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 58), Vec(30, 10), "NOISE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(20, track1Y + 82), module, TWNC::TRACK1_NOISE_MIX_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 58), Vec(30, 10), "ACCENT", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob>(Vec(60, track1Y + 82), module, TWNC::VCA_SHIFT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 58), Vec(30, 10), "DELAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(100, track1Y + 82), module, TWNC::VCA_DECAY_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(5, track1Y + 99), Vec(30, 10), "DECAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(20, track1Y + 123), module, TWNC::TRACK1_DECAY_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(45, track1Y + 99), Vec(30, 10), "SHAPE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(60, track1Y + 123), module, TWNC::TRACK1_SHAPE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(85, track1Y + 99), Vec(30, 10), "OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, track1Y + 123), module, TWNC::TRACK1_OUTPUT));

        float track2Y = 228;
        addChild(new TechnoEnhancedTextLabel(Vec(48, track2Y + 2), Vec(25, 10), "HATs", 8.f, nvgRGB(255, 200, 100), true));
        
        addChild(new TechnoEnhancedTextLabel(Vec(0, track2Y + 14), Vec(30, 10), "SHIFT", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob>(Vec(15, track2Y + 38), module, TWNC::TRACK2_SHIFT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(30, track2Y + 14), Vec(30, 10), "FILL", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, track2Y + 38), module, TWNC::TRACK2_FILL_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(60, track2Y + 14), Vec(30, 10), "D/M", 7.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoSnapKnob>(Vec(75, track2Y + 38), module, TWNC::TRACK2_DIVMULT_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(90, track2Y + 14), Vec(30, 10), "NOISE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(105, track2Y + 38), module, TWNC::TRACK2_NOISE_FM_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(0, track2Y + 56), Vec(30, 10), "FREQ", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(15, track2Y + 80), module, TWNC::TRACK2_FREQ_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(30, track2Y + 56), Vec(30, 10), "DECAY", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(45, track2Y + 80), module, TWNC::TRACK2_DECAY_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(60, track2Y + 56), Vec(30, 10), "SHAPE", 6.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<TechnoStandardBlackKnob>(Vec(75, track2Y + 80), module, TWNC::TRACK2_SHAPE_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(90, track2Y + 56), Vec(30, 10), "OUT", 7.f, nvgRGB(255, 255, 255), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, track2Y + 80), module, TWNC::TRACK2_OUTPUT));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-10, 329), Vec(30, 10), "D.F", 6.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(17, 343), module, TWNC::DRUM_FREQ_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(18, 329), Vec(30, 10), "D.D", 6.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(47, 343), module, TWNC::DRUM_DECAY_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(48, 329), Vec(30, 10), "H.F", 6.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(77, 343), module, TWNC::HATS_FREQ_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(78, 329), Vec(30, 10), "H.D", 6.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(107, 343), module, TWNC::HATS_DECAY_CV_INPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(-3, 362), Vec(20, 6), "VCA", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(-3, 368), Vec(20, 6), "ENV", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(24, 368), module, TWNC::MAIN_VCA_ENV_OUTPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(30, 360), Vec(30, 6), "DRUM", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(35, 366), Vec(20, 6), "FM", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(37, 372), Vec(20, 6), "ENV", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64, 368), module, TWNC::TRACK1_FM_ENV_OUTPUT));
        
        addChild(new TechnoEnhancedTextLabel(Vec(69, 360), Vec(30, 6), "HATS", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(74, 366), Vec(20, 6), "VCA", 6.f, nvgRGB(255, 133, 133), true));
        addChild(new TechnoEnhancedTextLabel(Vec(74, 372), Vec(20, 6), "ENV", 6.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(102, 368), module, TWNC::TRACK2_VCA_ENV_OUTPUT));
    }
};

Model* modelTWNC = createModel<TWNC, TWNCWidget>("TWNC");