#include "plugin.hpp"
#include <cmath>
#include <algorithm>
#include <random>

using namespace rack;

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

static const float kFreqKnobDisplayBase = kFreqKnobMax / kFreqKnobMin;
static const float kFreqKnobDisplayMultiplier = kFreqKnobMin;

static const float kVCAGainConstant = -33e-3f;
static const float kPlus6dB = 20.f * std::log10(2.f);
static const float kFreqAmpGain = kVCAGainConstant * kPlus6dB;
static const float kFreqInputR = 100e3f;
static const float kFreqAmpR = -kFreqAmpGain * kFreqInputR;
static const float kFreqAmpC = 560e-12f;

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

struct Pinpple : Module {
    enum ParamId {
        FREQ_PARAM,
        RESONANCE_PARAM,
        FM_AMOUNT_PARAM,
        FREQ_CV_ATTEN_PARAM,
        RESONANCE_CV_ATTEN_PARAM,
        FM_MOD_CV_ATTEN_PARAM,
        MUTE_PARAM,
        VOLUME_PARAM,
        NOISE_MIX_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        FM_INPUT,
        FREQ_CV_INPUT,
        RESONANCE_CV_INPUT,
        TRIG_INPUT,
        FM_MOD_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        MUTE_LIGHT,
        LIGHTS_LEN
    };

    struct RipplesBPFEngine {
        float sample_time_;
        simd::float_4 cell_voltage_;
        ripples::AAFilter<simd::float_4> aa_filter_;
        dsp::TRCFilter<simd::float_4> rc_filters_;
        
        RipplesBPFEngine() {
            setSampleRate(44100.f);
        }
        
        void setSampleRate(float sample_rate) {
            sample_time_ = 1.f / sample_rate;
            cell_voltage_ = simd::float_4(0.f);
            aa_filter_.Init(sample_rate);
            
            float oversample_rate = sample_rate * aa_filter_.GetOversamplingFactor();
            float freq_cut = 1.f / (2.f * M_PI * kFreqAmpR * kFreqAmpC);
            float res_cut  = 1.f / (2.f * M_PI * kResAmpR  * kResAmpC);
            float ff_cut = 1.f / (2.f * M_PI * kFeedforwardR * kFeedforwardC);
            
            auto cutoffs = simd::float_4(ff_cut, freq_cut, res_cut, 0.f);
            rc_filters_.setCutoffFreq(cutoffs / oversample_rate);
        }
        
        float process(float input, float freq_knob, float res_knob, float fm_cv) {
            float v_oct = 0.f;
            v_oct += (freq_knob - 1.f) * kFreqKnobVoltage;
            v_oct += fm_cv;
            v_oct = std::min(v_oct, 0.f);
            
            float i_reso = VtoIConverter(kResAmpR, 0.f, kResInputR, res_knob * kResKnobV, kResKnobR);
            
            int oversampling_factor = aa_filter_.GetOversamplingFactor();
            float timestep = sample_time_ / oversampling_factor;
            float audio_input = input + 1e-6 * (random::uniform() - 0.5f);
            auto inputs = simd::float_4(audio_input, v_oct, i_reso, 0.f);
            inputs *= oversampling_factor;
            simd::float_4 outputs;
            
            for (int i = 0; i < oversampling_factor; i++) {
                inputs = aa_filter_.ProcessUp((i == 0) ? inputs : 0.f);
                outputs = CoreProcess(inputs, timestep);
                outputs = aa_filter_.ProcessDown(outputs);
            }
            
            return outputs[0];
        }
        
    private:
        simd::float_4 CoreProcess(simd::float_4 inputs, float timestep) {
            rc_filters_.process(inputs);
            
            simd::float_4 control = rc_filters_.lowpass();
            float v_oct = control[1];
            float i_reso = control[2];
            
            float feedforward = rc_filters_.highpass()[0];
            
            simd::float_4 rad_per_s = -std::exp2f(v_oct) / kFilterCellRC;
            
            cell_voltage_ = StepRK2(timestep, cell_voltage_, [&](simd::float_4 vout) {
                simd::float_4 vin = _mm_shuffle_ps(vout.v, vout.v, _MM_SHUFFLE(2, 1, 0, 3));
                
                float vp = feedforward * kFeedforwardGain;
                float vn = vout[3] * kFeedbackGain;
                float res = kFilterCellR * OTAVCA(vp, vn, i_reso);
                simd::float_4 in = inputs[0] * kFilterInputGain + res;
                
                vin = _mm_move_ss(vin.v, in.v);
                
                simd::float_4 vsum = vin + vout;
                simd::float_4 dvout = rad_per_s * vsum;
                
                dvout *= (1.f + vsum * kFilterCellSelfModulation);
                
                return dvout;
            });
            
            cell_voltage_ = simd::clamp(cell_voltage_, -kOpampSatV, kOpampSatV);
            
            float lp1 = cell_voltage_[0];
            float lp2 = cell_voltage_[1];
            float bp2 = (lp1 + lp2) * kBP2Gain;
            
            return simd::float_4(bp2, 0.f, 0.f, 0.f);
        }
        
        template <typename T, typename F>
        T StepRK2(float dt, T y, F f) {
            T k1 = f(y);
            T k2 = f(y + k1 * dt / 2.f);
            return y + dt * k2;
        }
        
        float VtoIConverter(float rfb, float vc, float rc, float vp = 0.f, float rp = 1e12f) {
            float vnom = -(vc * rfb / rc + vp * rfb / rp);
            float vout = std::max(vnom, kVtoICollectorVSat);
            float nrc = rp * rfb;
            float nrp = rc * rfb;
            float nrfb = rc * rp;
            float vneg = (vc * nrc + vp * nrp + vout * nrfb) / (nrc + nrp + nrfb);
            float iout = (vneg - vout) / rfb;
            return std::max(iout, 0.f);
        }
        
        template <typename T>
        T OTAVCA(T vp, T vn, T i_abc) {
            const float kTemperature = 40.f;
            const float kKoverQ = 8.617333262145e-5;
            const float kKelvin = 273.15f;
            const float kVt = kKoverQ * (kTemperature + kKelvin);
            const float kZlim = 2.f * std::sqrt(3.f);
            
            T vi = vp - vn;
            T zlim = kZlim;
            T z = simd::clamp(vi / (2 * kVt), -zlim, zlim);
            
            T z2 = z * z;
            T q = 12.f + z2;
            T p = 12.f * z * q / (36.f * z2 + q * q);
            
            return i_abc * p;
        }
    };

    struct RandomModulation {
        float freqOffset = 0.0f;
        float decayOffset = 0.0f;
        
        void trigger() {
            freqOffset = (random::normal() * 0.00006f);
            decayOffset = (random::normal() * 0.00006f);
        }
    };

    struct TriggerGenerator {
        dsp::SchmittTrigger inputTrigger;
        dsp::PulseGenerator outputPulse;
        
        bool process(float input) {
            if (inputTrigger.process(input)) {
                outputPulse.trigger(0.002f);
                return true;
            }
            return false;
        }
        
        float getTrigger(float sampleTime) {
            return outputPulse.process(sampleTime) ? 10.0f : 0.0f;
        }
    };

    struct SimpleLPG {
        dsp::SchmittTrigger trigger;
        dsp::BiquadFilter lpf;
        float env = 0.0f;
        float attackTime = 0.00001f;
        float decayTime = 0.05f;
        bool attacking = false;
        bool decaying = false;
        float sampleRate = 44100.0f;
        
        void setSampleRate(float sr) {
            sampleRate = sr;
        }
        
        void reset() {
            trigger.reset();
            env = 0.0f;
            attacking = false;
            decaying = false;
        }
        
        float process(float triggerInput, float resonanceParam, float input, float vcaAmount, float sampleTime) {
            if (trigger.process(triggerInput)) {
                attacking = true;
                decaying = false;
                env = 0.0f;
            }
            
            if (attacking) {
                env += sampleTime / attackTime;
                if (env >= 1.0f) {
                    env = 1.0f;
                    attacking = false;
                    decaying = true;
                }
            }
            
            if (decaying) {
                decayTime = 0.01f + resonanceParam * 0.5f;
                float decayRate = 1.0f / decayTime;
                env -= env * decayRate * sampleTime * 10.0f;
                if (env <= 0.001f) {
                    env = 0.0f;
                    decaying = false;
                }
            }
            
            float cutoffFreq = 200.0f + env * 18000.0f;
            lpf.setParameters(dsp::BiquadFilter::LOWPASS, cutoffFreq / sampleRate, 0.707f, 1.0f);
            
            float filtered = lpf.process(input);
            float level = vcaAmount * env;
            return filtered * level;
        }
    };

    RipplesBPFEngine bpfEngine;
    TriggerGenerator trigGen;
    SimpleLPG lpg;
    PinkNoiseGenerator<8> pinkNoiseGenerator;
    float lastPink = 0.0f;
    
public:
    RandomModulation randomMod;
    
    float originalFreqParam = 0.5f;
    float originalResonanceParam = 0.5f;
    
    dsp::SchmittTrigger muteTrigger;
    bool muteState = false;
    
    Pinpple() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(FREQ_PARAM, std::log2(kFreqKnobMin), std::log2(kFreqKnobMax), std::log2(kFreqKnobMax), "Frequency", " Hz", 2.f);
        configParam(RESONANCE_PARAM, 0.0f, 1.0f, 0.5f, "Decay");
        configParam(FM_AMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "FM Amount");
        configParam(FREQ_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Freq CV Attenuverter");
        configParam(RESONANCE_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Decay CV Attenuverter");
        configParam(FM_MOD_CV_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "FM AMT CV Attenuverter");
        configParam(MUTE_PARAM, 0.0f, 1.0f, 0.0f, "Mute");
        configParam(VOLUME_PARAM, 0.0f, 1.0f, 0.7f, "Volume", "%", 0.f, 100.f);
        configParam(NOISE_MIX_PARAM, 0.0f, 1.0f, 0.5f, "Noise Mix");
        
        configInput(FM_INPUT, "FM");
        configInput(FREQ_CV_INPUT, "1V/Oct Frequency CV");
        configInput(RESONANCE_CV_INPUT, "Decay CV");
        configInput(TRIG_INPUT, "Trigger");
        configInput(FM_MOD_CV_INPUT, "FM AMT CV");
        configOutput(OUT_OUTPUT, "Audio");
        
        originalFreqParam = std::log2(kFreqKnobMax);
        originalResonanceParam = 0.5f;
    }

    void onSampleRateChange() override {
        float sr = APP->engine->getSampleRate();
        bpfEngine.setSampleRate(sr);
        lpg.setSampleRate(sr);
    }

    void process(const ProcessArgs& args) override {
        if (muteTrigger.process(params[MUTE_PARAM].getValue())) {
            muteState = !muteState;
            params[MUTE_PARAM].setValue(muteState ? 1.0f : 0.0f);
        }
        
        float triggerInput = inputs[TRIG_INPUT].getVoltage();
        bool newTrigger = trigGen.process(triggerInput);
        if (newTrigger) {
            randomMod.trigger();
        }
        float trigger2ms = trigGen.getTrigger(args.sampleTime);
        
        float freqParam = rescale(params[FREQ_PARAM].getValue(), std::log2(kFreqKnobMin), std::log2(kFreqKnobMax), 0.f, 1.f);
        float freqCV = 0.0f;
        if (inputs[FREQ_CV_INPUT].isConnected()) {
            float freqCVAttenuation = params[FREQ_CV_ATTEN_PARAM].getValue();
            freqCV = inputs[FREQ_CV_INPUT].getVoltage() * freqCVAttenuation;
        }
        float finalFreq = clamp(freqParam + freqCV * 0.1f + randomMod.freqOffset, 0.0f, 1.0f);
        
        float resonanceParam = params[RESONANCE_PARAM].getValue();
        float resonanceCV = 0.0f;
        if (inputs[RESONANCE_CV_INPUT].isConnected()) {
            float resonanceCVAttenuation = params[RESONANCE_CV_ATTEN_PARAM].getValue();
            resonanceCV = inputs[RESONANCE_CV_INPUT].getVoltage() / 10.0f * resonanceCVAttenuation;
        }
        float finalResonance = clamp(resonanceParam + resonanceCV + randomMod.decayOffset, 0.0f, 1.0f);
        
        float fmAmountParam = params[FM_AMOUNT_PARAM].getValue();
        
        float fmModCV = 0.0f;
        if (inputs[FM_MOD_CV_INPUT].isConnected()) {
            float fmModCVAttenuation = params[FM_MOD_CV_ATTEN_PARAM].getValue();
            fmModCV = inputs[FM_MOD_CV_INPUT].getVoltage() / 10.0f * fmModCVAttenuation;
        }
        
        float dynamicFMAmount = clamp(fmAmountParam + fmModCV, 0.0f, 1.0f);
        
        float noiseMixParam = params[NOISE_MIX_PARAM].getValue();
        
        float pinkNoise = pinkNoiseGenerator.process() / 0.816f;
        float blueNoise = (pinkNoise - lastPink) / 0.705f;
        lastPink = pinkNoise;
        
        const float noiseGain = 5.f / std::sqrt(2.f);
        pinkNoise *= noiseGain * 0.8f;
        blueNoise *= noiseGain * 1.5f;
        
        float fmInput = inputs[FM_INPUT].getVoltage();
        float mixedInput;
        
        if (noiseMixParam <= 0.5f) {
            float mix = noiseMixParam * 2.0f;
            mixedInput = pinkNoise * (1.0f - mix) + fmInput * mix;
        } else {
            float mix = (noiseMixParam - 0.5f) * 2.0f;
            mixedInput = fmInput * (1.0f - mix) + blueNoise * mix;
        }
        
        float processedFM = lpg.process(trigger2ms, finalResonance, mixedInput, dynamicFMAmount, args.sampleTime);
        
        float pingInput = newTrigger ? 10.0f : 0.0f;
        float bpfOutput = bpfEngine.process(pingInput, finalFreq, finalResonance, processedFM);
        
        bool isMuted = muteState;
        float volume = params[VOLUME_PARAM].getValue();
        float finalOutput = isMuted ? 0.0f : bpfOutput * volume;
        
        lights[MUTE_LIGHT].setBrightness(isMuted ? 1.0f : 0.0f);
        
        outputs[OUT_OUTPUT].setVoltage(finalOutput);
    }
};

struct RandomizedKnob : ParamWidget {
    Module* module = nullptr;
    int paramId = -1;
    bool isDragging = false;
    
    RandomizedKnob() {
        box.size = Vec(30, 30);
    }
    
    float getVisualValue() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float baseValue = pq->getValue();
        float randomOffset = 0.0f;
        
        if (module) {
            Pinpple* pinppleModule = dynamic_cast<Pinpple*>(module);
            if (pinppleModule) {
                if (paramId == Pinpple::FREQ_PARAM) {
                    randomOffset = pinppleModule->randomMod.freqOffset * 80.0f;
                } else if (paramId == Pinpple::RESONANCE_PARAM) {
                    randomOffset = pinppleModule->randomMod.decayOffset * 80.0f;
                }
            }
        }
        
        float visualValue = baseValue + randomOffset;
        return clamp(visualValue, pq->getMinValue(), pq->getMaxValue());
    }
    
    float getDisplayAngle() {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return 0.0f;
        
        float visualValue = getVisualValue();
        float normalizedValue = pq->toScaled(visualValue);
        
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
    
    void step() override {
        ParamQuantity* pq = getParamQuantity();
        if (pq) {
            module = pq->module;
            paramId = pq->paramId;
        }
        ParamWidget::step();
    }
};

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

struct PinppleWidget : ModuleWidget {
    PinppleWidget(Pinpple* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwingLFO.svg")));
        
        box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        
        float centerX = box.size.x / 2;
        
        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Pinpple", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));
        
        addParam(createParamCentered<VCVButton>(Vec(centerX - 15, 40), module, Pinpple::MUTE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(centerX - 15, 40), module, Pinpple::MUTE_LIGHT));
        addParam(createParamCentered<Trimpot>(Vec(centerX + 15, 40), module, Pinpple::VOLUME_PARAM));
        
        addChild(new EnhancedTextLabel(Vec(0, 50), Vec(box.size.x, 20), "FREQ", 12.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<RandomizedKnob>(Vec(centerX, 84), module, Pinpple::FREQ_PARAM));
        
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 108), module, Pinpple::FREQ_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 108), module, Pinpple::FREQ_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(0, 123), Vec(box.size.x, 20), "DECAY", 12.f, nvgRGB(255, 255, 255), true));
addParam(createParamCentered<RandomizedKnob>(Vec(centerX, 155), module, Pinpple::RESONANCE_PARAM));

addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 179), module, Pinpple::RESONANCE_CV_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 179), module, Pinpple::RESONANCE_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(0, 194), Vec(box.size.x, 20), "FM AMT", 12.f, nvgRGB(255, 255, 255), true));
addParam(createParamCentered<RandomizedKnob>(Vec(centerX, 226), module, Pinpple::FM_AMOUNT_PARAM));
        
      
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 265), module, Pinpple::FM_INPUT));
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 265), module, Pinpple::NOISE_MIX_PARAM));
        addParam(createParamCentered<Trimpot>(Vec(centerX - 15, 292), module, Pinpple::FM_MOD_CV_ATTEN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 15, 292), module, Pinpple::FM_MOD_CV_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(18, 238), Vec(25, 20), "LPG IN MIX", 8.f, nvgRGB(255, 255, 255), true));
        
        
        addChild(new EnhancedTextLabel(Vec(0, 299), Vec(25, 20), "NO", 8.f, nvgRGB(255, 255, 255), true));
        addChild(new EnhancedTextLabel(Vec(20, 309), Vec(30, 20), "BEHRINGER", 8.f, nvgRGB(255, 255, 255), true));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(60, 50)));
        
        addChild(new EnhancedTextLabel(Vec(32, 335), Vec(25, 20), "TRIG IN", 8.f, nvgRGB(255, 133, 133), true));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX - 15, 343), module, Pinpple::TRIG_INPUT));
        
        addChild(new EnhancedTextLabel(Vec(5, 360), Vec(20, 20), "OUTPUT", 8.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(centerX + 15, 368), module, Pinpple::OUT_OUTPUT));

        if (module) {
            struct NoiseMixParamQuantity : ParamQuantity {
                    std::string getDisplayValueString() override {
                        float value = getValue();
                        if (value <= 0.25f) {
                            return "Pink";
                        } else if (value >= 0.75f) {
                            return "Blue";
                        } else {
                            return "External";
                        }
                    }
                
                    std::string getLabel() override {
                        return "LPG IN MIX";
                    }
                };
            
                delete module->paramQuantities[Pinpple::NOISE_MIX_PARAM];
                NoiseMixParamQuantity* noiseMixQuantity = new NoiseMixParamQuantity;
                noiseMixQuantity->module = module;
                noiseMixQuantity->paramId = Pinpple::NOISE_MIX_PARAM;
                noiseMixQuantity->minValue = 0.0f;
                noiseMixQuantity->maxValue = 1.0f;
                noiseMixQuantity->defaultValue = 0.5f;
                noiseMixQuantity->name = "LPG IN MIX";
                module->paramQuantities[Pinpple::NOISE_MIX_PARAM] = noiseMixQuantity;
        }
    }
};

Model* modelPinpple = createModel<Pinpple, PinppleWidget>("Pinpple");