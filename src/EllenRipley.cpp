#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"

using namespace rack;
using namespace rack::engine;
using namespace rack::math;

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

// StandardBlackKnob 現在從 widgets/Knobs.hpp 引入


struct ChaosGenerator {
    float x = 0.1f;
    float y = 0.1f;
    float z = 0.1f;
    
    void reset() {
        x = 0.1f;
        y = 0.1f;
        z = 0.1f;
    }
    
    float process(float rate) {
        float dt = rate * 0.001f;
        
        float dx = 7.5f * (y - x);
        float dy = x * (30.9f - z) - y;
        float dz = x * y - 1.02f * z;
        
        x += dx * dt;
        y += dy * dt;
        z += dz * dt;
        
        // Prevent numerical explosion
        if (std::isnan(x) || std::isnan(y) || std::isnan(z) || 
            std::abs(x) > 100.0f || std::abs(y) > 100.0f || std::abs(z) > 100.0f) {
            reset();
        }
        
        return clamp(x * 0.1f, -1.0f, 1.0f);
    }
};

struct ReverbProcessor {
    // Freeverb-style parallel comb filters + series allpass
    static constexpr int COMB_1_SIZE = 1557;  // ~32ms at 48kHz
    static constexpr int COMB_2_SIZE = 1617;  // ~34ms
    static constexpr int COMB_3_SIZE = 1491;  // ~31ms
    static constexpr int COMB_4_SIZE = 1422;  // ~30ms
    static constexpr int COMB_5_SIZE = 1277;  // ~27ms (for stereo)
    static constexpr int COMB_6_SIZE = 1356;  // ~28ms (for stereo)
    static constexpr int COMB_7_SIZE = 1188;  // ~25ms (for stereo)
    static constexpr int COMB_8_SIZE = 1116;  // ~23ms (for stereo)
    
    float combBuffer1[COMB_1_SIZE];
    float combBuffer2[COMB_2_SIZE];
    float combBuffer3[COMB_3_SIZE];
    float combBuffer4[COMB_4_SIZE];
    float combBuffer5[COMB_5_SIZE];
    float combBuffer6[COMB_6_SIZE];
    float combBuffer7[COMB_7_SIZE];
    float combBuffer8[COMB_8_SIZE];
    
    int combIndex1 = 0, combIndex2 = 0, combIndex3 = 0, combIndex4 = 0;
    int combIndex5 = 0, combIndex6 = 0, combIndex7 = 0, combIndex8 = 0;
    
    // Lowpass filters in comb feedback loops
    float combLp1 = 0.0f, combLp2 = 0.0f, combLp3 = 0.0f, combLp4 = 0.0f;
    float combLp5 = 0.0f, combLp6 = 0.0f, combLp7 = 0.0f, combLp8 = 0.0f;
    
    // Highpass filter for reverb output (to remove sub-100Hz)
    float hpState = 0.0f;
    
    // Series allpass filters for diffusion
    static constexpr int ALLPASS_1_SIZE = 556;
    static constexpr int ALLPASS_2_SIZE = 441;
    static constexpr int ALLPASS_3_SIZE = 341;
    static constexpr int ALLPASS_4_SIZE = 225;
    
    float allpassBuffer1[ALLPASS_1_SIZE];
    float allpassBuffer2[ALLPASS_2_SIZE];
    float allpassBuffer3[ALLPASS_3_SIZE];
    float allpassBuffer4[ALLPASS_4_SIZE];
    
    int allpassIndex1 = 0, allpassIndex2 = 0, allpassIndex3 = 0, allpassIndex4 = 0;
    
    ReverbProcessor() { reset(); }
    
    void reset() {
        for (int i = 0; i < COMB_1_SIZE; i++) combBuffer1[i] = 0.0f;
        for (int i = 0; i < COMB_2_SIZE; i++) combBuffer2[i] = 0.0f;
        for (int i = 0; i < COMB_3_SIZE; i++) combBuffer3[i] = 0.0f;
        for (int i = 0; i < COMB_4_SIZE; i++) combBuffer4[i] = 0.0f;
        for (int i = 0; i < COMB_5_SIZE; i++) combBuffer5[i] = 0.0f;
        for (int i = 0; i < COMB_6_SIZE; i++) combBuffer6[i] = 0.0f;
        for (int i = 0; i < COMB_7_SIZE; i++) combBuffer7[i] = 0.0f;
        for (int i = 0; i < COMB_8_SIZE; i++) combBuffer8[i] = 0.0f;
        
        for (int i = 0; i < ALLPASS_1_SIZE; i++) allpassBuffer1[i] = 0.0f;
        for (int i = 0; i < ALLPASS_2_SIZE; i++) allpassBuffer2[i] = 0.0f;
        for (int i = 0; i < ALLPASS_3_SIZE; i++) allpassBuffer3[i] = 0.0f;
        for (int i = 0; i < ALLPASS_4_SIZE; i++) allpassBuffer4[i] = 0.0f;
        
        combIndex1 = combIndex2 = combIndex3 = combIndex4 = 0;
        combIndex5 = combIndex6 = combIndex7 = combIndex8 = 0;
        allpassIndex1 = allpassIndex2 = allpassIndex3 = allpassIndex4 = 0;
        
        combLp1 = combLp2 = combLp3 = combLp4 = 0.0f;
        combLp5 = combLp6 = combLp7 = combLp8 = 0.0f;
        hpState = 0.0f;
    }
    
    float processComb(float input, float* buffer, int size, int& index, float feedback, float& lp, float damping) {
        float output = buffer[index];
        
        // Apply lowpass filter to feedback signal
        lp = lp + (output - lp) * damping;
        
        // Write input + filtered feedback
        buffer[index] = input + lp * feedback;
        index = (index + 1) % size;
        
        return output;
    }
    
    float processAllpass(float input, float* buffer, int size, int& index, float gain) {
        float delayed = buffer[index];
        float output = -input * gain + delayed;
        buffer[index] = input + delayed * gain;
        index = (index + 1) % size;
        return output;
    }
    
    float process(float inputL, float inputR, float grainDensity,
                  float roomSize, float damping, float decay, bool isLeftChannel,
                  bool chaosEnabled, float chaosOutput, float sampleRate) {
        
        // Use proper stereo input instead of mixing to mono
        float input = isLeftChannel ? inputL : inputR;
        
        // Calculate feedback based on decay - much wider range for 10+ second tails
        float feedback = 0.5f + decay * 0.485f; // 0.5 to 0.985 (near infinite at max)
        if (chaosEnabled) {
            feedback += chaosOutput * 0.5f; // Enhanced chaos effect 10x from 0.05f
            feedback = clamp(feedback, 0.0f, 0.995f);
        }
        
        // Damping: low value = more damping (darker), high value = less damping (brighter)
        float dampingCoeff = 0.05f + damping * 0.9f;
        
        // Room size affects delay buffer read positions dramatically
        float roomScale = 0.3f + roomSize * 1.4f; // 0.3 to 1.7 scaling
        
        float combOut = 0.0f;
        
        if (isLeftChannel) {
            // Room size creates variable delay taps for room simulation
            // Ensure room offsets are always positive
            int roomOffset1 = std::max(0, (int)(roomSize * 400 + chaosOutput * 50)); // 0-450 samples
            int roomOffset2 = std::max(0, (int)(roomSize * 350 + chaosOutput * 40));
            // Unused room offsets (corresponding read indices are not used)
            // int roomOffset3 = std::max(0, (int)(roomSize * 300 + chaosOutput * 60));
            // int roomOffset4 = std::max(0, (int)(roomSize * 500 + chaosOutput * 70));
            
            // Use room-modulated delay reads with safe modulo operation
            int readIdx1 = ((combIndex1 - roomOffset1) % COMB_1_SIZE + COMB_1_SIZE) % COMB_1_SIZE;
            int readIdx2 = ((combIndex2 - roomOffset2) % COMB_2_SIZE + COMB_2_SIZE) % COMB_2_SIZE;
            // Unused read indices
            // int readIdx3 = ((combIndex3 - roomOffset3) % COMB_3_SIZE + COMB_3_SIZE) % COMB_3_SIZE;
            // int readIdx4 = ((combIndex4 - roomOffset4) % COMB_4_SIZE + COMB_4_SIZE) % COMB_4_SIZE;
            
            float roomInput = input * roomScale;
            combOut += processComb(roomInput, combBuffer1, COMB_1_SIZE, combIndex1, feedback, combLp1, dampingCoeff);
            combOut += processComb(roomInput, combBuffer2, COMB_2_SIZE, combIndex2, feedback, combLp2, dampingCoeff);
            combOut += processComb(roomInput, combBuffer3, COMB_3_SIZE, combIndex3, feedback, combLp3, dampingCoeff);
            combOut += processComb(roomInput, combBuffer4, COMB_4_SIZE, combIndex4, feedback, combLp4, dampingCoeff);
            
            // Add room reflections
            combOut += combBuffer1[readIdx1] * roomSize * 0.15f;
            combOut += combBuffer2[readIdx2] * roomSize * 0.12f;
        } else {
            // Right channel: different room characteristics
            // Ensure room offsets are always positive
            int roomOffset5 = std::max(0, (int)(roomSize * 380 + chaosOutput * 45));
            int roomOffset6 = std::max(0, (int)(roomSize * 420 + chaosOutput * 55));
            // Unused room offsets (corresponding read indices are not used)
            // int roomOffset7 = std::max(0, (int)(roomSize * 280 + chaosOutput * 35));
            // int roomOffset8 = std::max(0, (int)(roomSize * 460 + chaosOutput * 65));
            
            // Use safe modulo operation
            int readIdx5 = ((combIndex5 - roomOffset5) % COMB_5_SIZE + COMB_5_SIZE) % COMB_5_SIZE;
            int readIdx6 = ((combIndex6 - roomOffset6) % COMB_6_SIZE + COMB_6_SIZE) % COMB_6_SIZE;
            // Unused read indices
            // int readIdx7 = ((combIndex7 - roomOffset7) % COMB_7_SIZE + COMB_7_SIZE) % COMB_7_SIZE;
            // int readIdx8 = ((combIndex8 - roomOffset8) % COMB_8_SIZE + COMB_8_SIZE) % COMB_8_SIZE;
            
            float roomInput = input * roomScale;
            combOut += processComb(roomInput, combBuffer5, COMB_5_SIZE, combIndex5, feedback, combLp5, dampingCoeff);
            combOut += processComb(roomInput, combBuffer6, COMB_6_SIZE, combIndex6, feedback, combLp6, dampingCoeff);
            combOut += processComb(roomInput, combBuffer7, COMB_7_SIZE, combIndex7, feedback, combLp7, dampingCoeff);
            combOut += processComb(roomInput, combBuffer8, COMB_8_SIZE, combIndex8, feedback, combLp8, dampingCoeff);
            
            // Add room reflections
            combOut += combBuffer5[readIdx5] * roomSize * 0.13f;
            combOut += combBuffer6[readIdx6] * roomSize * 0.11f;
        }
        
        // Scale comb output
        combOut *= 0.25f;
        
        // Series allpass diffusion
        float diffused = combOut;
        diffused = processAllpass(diffused, allpassBuffer1, ALLPASS_1_SIZE, allpassIndex1, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer2, ALLPASS_2_SIZE, allpassIndex2, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer3, ALLPASS_3_SIZE, allpassIndex3, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer4, ALLPASS_4_SIZE, allpassIndex4, 0.5f);
        
        // Apply highpass filter to remove frequencies below ~100Hz
        // Cutoff frequency calculation for 100Hz at 48kHz: fc = 100/(48000/2) = 0.00416
        float hpCutoff = 100.0f / (sampleRate * 0.5f); // Normalized frequency
        hpCutoff = clamp(hpCutoff, 0.001f, 0.1f); // Safety clamp
        hpState += (diffused - hpState) * hpCutoff;
        float hpOutput = diffused - hpState;
        
        return hpOutput;
    }
};

struct GrainProcessor {
    static constexpr int GRAIN_BUFFER_SIZE = 8192;
    float grainBuffer[GRAIN_BUFFER_SIZE];
    int grainWriteIndex = 0;
    
    struct Grain {
        bool active = false;
        float position = 0.0f;
        float size = 0.0f;
        float envelope = 0.0f;
        float direction = 1.0f;
        float pitch = 1.0f;
    };
    
    static constexpr int MAX_GRAINS = 16;
    Grain grains[MAX_GRAINS];
    
    float phase = 0.0f;
    dsp::SchmittTrigger grainTrigger;
    
    void reset() {
        for (int i = 0; i < GRAIN_BUFFER_SIZE; i++) {
            grainBuffer[i] = 0.0f;
        }
        grainWriteIndex = 0;
        
        for (int i = 0; i < MAX_GRAINS; i++) {
            grains[i].active = false;
        }
        phase = 0.0f;
    }
    
    float process(float input, float grainSize, float density, float position, 
                  bool chaosEnabled, float chaosOutput, float sampleRate) {
        
        grainBuffer[grainWriteIndex] = input;
        grainWriteIndex = (grainWriteIndex + 1) % GRAIN_BUFFER_SIZE;
        
        float grainSizeMs = grainSize * 99.0f + 1.0f;
        float grainSamples = (grainSizeMs / 1000.0f) * sampleRate;
        
        float densityValue = density;
        if (chaosEnabled) {
            densityValue += chaosOutput * 0.3f;
        }
        densityValue = clamp(densityValue, 0.0f, 1.0f);
        
        float triggerRate = densityValue * 50.0f + 1.0f;
        phase += triggerRate / sampleRate;
        
        if (phase >= 1.0f) {
            phase -= 1.0f;
            
            for (int i = 0; i < MAX_GRAINS; i++) {
                if (!grains[i].active) {
                    grains[i].active = true;
                    grains[i].size = grainSamples;
                    grains[i].envelope = 0.0f;
                    
                    float pos = position;
                    if (chaosEnabled) {
                        pos += chaosOutput * 20.0f; // Enhanced shift 10x from 2.0f
                        if (random::uniform() < 0.3f) {
                            grains[i].direction = -1.0f;
                        } else {
                            grains[i].direction = 1.0f;
                        }
                        
                        if (densityValue > 0.7f && random::uniform() < 0.2f) {
                            grains[i].pitch = random::uniform() < 0.5f ? 0.5f : 2.0f;
                        } else {
                            grains[i].pitch = 1.0f;
                        }
                    } else {
                        grains[i].direction = 1.0f;
                        grains[i].pitch = 1.0f;
                    }
                    
                    pos = clamp(pos, 0.0f, 1.0f);
                    grains[i].position = pos * GRAIN_BUFFER_SIZE;
                    break;
                }
            }
        }
        
        float output = 0.0f;
        int activeGrains = 0;
        
        for (int i = 0; i < MAX_GRAINS; i++) {
            if (grains[i].active) {
                float envPhase = grains[i].envelope / grains[i].size;
                
                if (envPhase >= 1.0f) {
                    grains[i].active = false;
                    continue;
                }
                
                float env = 0.5f * (1.0f - cos(envPhase * 2.0f * M_PI));
                
                int readPos = (int)grains[i].position;
                // Ensure readPos is always valid
                readPos = ((readPos % GRAIN_BUFFER_SIZE) + GRAIN_BUFFER_SIZE) % GRAIN_BUFFER_SIZE;
                
                float sample = grainBuffer[readPos];
                output += sample * env;
                
                // Update position with proper boundary handling
                grains[i].position += grains[i].direction * grains[i].pitch;
                
                // Handle position wrapping to prevent accumulated floating point errors
                while (grains[i].position >= GRAIN_BUFFER_SIZE) {
                    grains[i].position -= GRAIN_BUFFER_SIZE;
                }
                while (grains[i].position < 0) {
                    grains[i].position += GRAIN_BUFFER_SIZE;
                }
                
                grains[i].envelope += 1.0f;
                activeGrains++;
            }
        }
        
        if (activeGrains > 0) {
            output /= sqrt(activeGrains);
        }
        
        return output;
    }
};

struct EllenRipley : rack::engine::Module {
    int panelTheme = 0; // 0 = Sashimi, 1 = Boring

    enum ParamIds {
        DELAY_TIME_L_PARAM,
        DELAY_TIME_R_PARAM,
        DELAY_FEEDBACK_PARAM,
        DELAY_CHAOS_PARAM,
        WET_DRY_PARAM,
        CHAOS_RATE_PARAM,
        GRAIN_SIZE_PARAM,
        GRAIN_DENSITY_PARAM,
        GRAIN_POSITION_PARAM,
        GRAIN_CHAOS_PARAM,
        GRAIN_WET_DRY_PARAM,
        REVERB_ROOM_SIZE_PARAM,
        REVERB_DAMPING_PARAM,
        REVERB_DECAY_PARAM,
        REVERB_CHAOS_PARAM,
        REVERB_WET_DRY_PARAM,
        CHAOS_AMOUNT_PARAM,
        CHAOS_SHAPE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        LEFT_AUDIO_INPUT,
        RIGHT_AUDIO_INPUT,
        DELAY_TIME_L_CV_INPUT,
        DELAY_TIME_R_CV_INPUT,
        DELAY_FEEDBACK_CV_INPUT,
        GRAIN_SIZE_CV_INPUT,
        GRAIN_DENSITY_CV_INPUT,
        GRAIN_POSITION_CV_INPUT,
        REVERB_ROOM_SIZE_CV_INPUT,
        REVERB_DAMPING_CV_INPUT,
        REVERB_DECAY_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        LEFT_AUDIO_OUTPUT,
        RIGHT_AUDIO_OUTPUT,
        CHAOS_CV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        DELAY_CHAOS_LIGHT,
        GRAIN_CHAOS_LIGHT,
        REVERB_CHAOS_LIGHT,
        CHAOS_SHAPE_LIGHT,
        NUM_LIGHTS
    };

    static constexpr int DELAY_BUFFER_SIZE = 96000;
    static constexpr int MAX_POLY = 16;
    float leftDelayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    float rightDelayBuffer[MAX_POLY][DELAY_BUFFER_SIZE];
    int delayWriteIndex[MAX_POLY];

    ChaosGenerator chaosGen[MAX_POLY];
    GrainProcessor leftGrainProcessor[MAX_POLY];
    GrainProcessor rightGrainProcessor[MAX_POLY];
    ReverbProcessor leftReverbProcessor[MAX_POLY];
    ReverbProcessor rightReverbProcessor[MAX_POLY];
    
    bool delayChaosMod = false;
    bool grainChaosMod = false;
    bool reverbChaosMod = false;
    
    EllenRipley() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(DELAY_TIME_L_PARAM, 0.001f, 2.0f, 0.25f, "Delay Time L", " s");
        configParam(DELAY_TIME_R_PARAM, 0.001f, 2.0f, 0.25f, "Delay Time R", " s");
        configParam(DELAY_FEEDBACK_PARAM, 0.0f, 0.95f, 0.3f, "Feedback", "%", 0.f, 100.f);
        configParam(DELAY_CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Delay Chaos");
        configParam(WET_DRY_PARAM, 0.0f, 1.0f, 0.0f, "Delay Wet/Dry", "%", 0.f, 100.f);
        configParam(CHAOS_RATE_PARAM, 0.0f, 1.0f, 0.01f, "Chaos Rate");

        configParam(GRAIN_SIZE_PARAM, 0.0f, 1.0f, 0.3f, "Grain Size");
        configParam(GRAIN_DENSITY_PARAM, 0.0f, 1.0f, 0.4f, "Grain Density/Glitch");
        configParam(GRAIN_POSITION_PARAM, 0.0f, 1.0f, 0.5f, "Grain Position/Chaos");
        configParam(GRAIN_CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Grain Chaos");
        configParam(GRAIN_WET_DRY_PARAM, 0.0f, 1.0f, 0.0f, "Gratch Wet/Dry", "%", 0.f, 100.f);

        configParam(REVERB_ROOM_SIZE_PARAM, 0.0f, 1.0f, 0.5f, "Room Size");
        configParam(REVERB_DAMPING_PARAM, 0.0f, 1.0f, 0.4f, "Damping");
        configParam(REVERB_DECAY_PARAM, 0.0f, 1.0f, 0.6f, "Decay");
        configParam(REVERB_CHAOS_PARAM, 0.0f, 1.0f, 0.0f, "Reverb Chaos");
        configParam(REVERB_WET_DRY_PARAM, 0.0f, 1.0f, 0.0f, "Reverb Wet/Dry", "%", 0.f, 100.f);
        configParam(CHAOS_AMOUNT_PARAM, 0.0f, 1.0f, 1.0f, "Chaos Amount");
        configParam(CHAOS_SHAPE_PARAM, 0.0f, 1.0f, 0.0f, "Chaos Shape");

        configInput(LEFT_AUDIO_INPUT, "Left Audio");
        configInput(RIGHT_AUDIO_INPUT, "Right Audio");
        configInput(DELAY_TIME_L_CV_INPUT, "Delay Time L CV");
        configInput(DELAY_TIME_R_CV_INPUT, "Delay Time R CV");
        configInput(DELAY_FEEDBACK_CV_INPUT, "Feedback CV");
        configInput(GRAIN_SIZE_CV_INPUT, "Grain Size CV");
        configInput(GRAIN_DENSITY_CV_INPUT, "Grain Density CV");
        configInput(GRAIN_POSITION_CV_INPUT, "Grain Position CV");
        configInput(REVERB_ROOM_SIZE_CV_INPUT, "Reverb Room Size CV");
        configInput(REVERB_DAMPING_CV_INPUT, "Reverb Damping CV");
        configInput(REVERB_DECAY_CV_INPUT, "Reverb Decay CV");

        configOutput(LEFT_AUDIO_OUTPUT, "Left Audio");
        configOutput(RIGHT_AUDIO_OUTPUT, "Right Audio");
        configOutput(CHAOS_CV_OUTPUT, "Chaos CV");

        configLight(DELAY_CHAOS_LIGHT, "Delay Chaos");
        configLight(GRAIN_CHAOS_LIGHT, "Grain Chaos");
        configLight(REVERB_CHAOS_LIGHT, "Reverb Chaos");
        configLight(CHAOS_SHAPE_LIGHT, "Chaos Shape");

        // Initialize buffers for all polyphonic channels
        for (int c = 0; c < MAX_POLY; c++) {
            for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
                leftDelayBuffer[c][i] = 0.0f;
                rightDelayBuffer[c][i] = 0.0f;
            }
            delayWriteIndex[c] = 0;
        }
    }
    
    void onReset() override {
        for (int c = 0; c < MAX_POLY; c++) {
            chaosGen[c].reset();
            leftGrainProcessor[c].reset();
            rightGrainProcessor[c].reset();
            leftReverbProcessor[c].reset();
            rightReverbProcessor[c].reset();
            for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
                leftDelayBuffer[c][i] = 0.0f;
                rightDelayBuffer[c][i] = 0.0f;
            }
            delayWriteIndex[c] = 0;
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
    
    void process(const ProcessArgs& args) override {
        // Defensive checks
        if (args.sampleRate <= 0) return;
        if (!std::isfinite(args.sampleTime)) return;

        // Get polyphonic channel count
        int leftChannels = inputs[LEFT_AUDIO_INPUT].getChannels();
        int rightChannels = inputs[RIGHT_AUDIO_INPUT].getChannels();
        int channels = std::max({1, leftChannels, rightChannels});

        // Set output channels
        outputs[LEFT_AUDIO_OUTPUT].setChannels(channels);
        outputs[RIGHT_AUDIO_OUTPUT].setChannels(channels);
        outputs[CHAOS_CV_OUTPUT].setChannels(channels);

        delayChaosMod = params[DELAY_CHAOS_PARAM].getValue() > 0.5f;
        grainChaosMod = params[GRAIN_CHAOS_PARAM].getValue() > 0.5f;
        reverbChaosMod = params[REVERB_CHAOS_PARAM].getValue() > 0.5f;

        // Set lights (monophonic)
        lights[DELAY_CHAOS_LIGHT].setBrightness(delayChaosMod ? 1.0f : 0.0f);
        lights[GRAIN_CHAOS_LIGHT].setBrightness(grainChaosMod ? 1.0f : 0.0f);
        lights[REVERB_CHAOS_LIGHT].setBrightness(reverbChaosMod ? 1.0f : 0.0f);
        lights[CHAOS_SHAPE_LIGHT].setBrightness(params[CHAOS_SHAPE_PARAM].getValue() > 0.5f ? 1.0f : 0.0f);

        // Process each polyphonic channel
        for (int c = 0; c < channels; c++) {
            float chaosRateParam = params[CHAOS_RATE_PARAM].getValue();
            bool chaosStep = params[CHAOS_SHAPE_PARAM].getValue() > 0.5f;
            float chaosRate;

            if (chaosStep) {
                // Shape ON: 1.0-10.0 range
                chaosRate = 1.0f + chaosRateParam * 9.0f;
            } else {
                // Shape OFF: 0.01-1.0 range
                chaosRate = 0.01f + chaosRateParam * 0.99f;
            }
            float chaosAmount = params[CHAOS_AMOUNT_PARAM].getValue();
            float chaosRaw = chaosGen[c].process(chaosRate) * chaosAmount;

            float chaosOutput;
            if (chaosStep) {
                // Use chaos rate to control step update frequency per channel
                static float lastStep[MAX_POLY] = {};
                static float stepPhase[MAX_POLY] = {};
                float stepRate = chaosRate * 10.0f; // Scale rate for step frequency
                stepPhase[c] += stepRate / args.sampleRate;
                if (stepPhase[c] >= 1.0f) {
                    lastStep[c] = chaosRaw;
                    stepPhase[c] = 0.0f;
                }
                chaosOutput = lastStep[c];
            } else {
                chaosOutput = chaosRaw;
            }

            outputs[CHAOS_CV_OUTPUT].setVoltage(chaosOutput * 5.0f, c);

            // Get input voltages for this channel
            float leftInput = (c < leftChannels) ? inputs[LEFT_AUDIO_INPUT].getPolyVoltage(c) : 0.0f;
            float rightInput = 0.0f;
            if (inputs[RIGHT_AUDIO_INPUT].isConnected()) {
                rightInput = (c < rightChannels) ? inputs[RIGHT_AUDIO_INPUT].getPolyVoltage(c) : 0.0f;
            } else {
                rightInput = leftInput;
            }

            // Validate input signals
            if (!std::isfinite(leftInput)) leftInput = 0.0f;
            if (!std::isfinite(rightInput)) rightInput = 0.0f;

            // Get CV inputs (use channel 0 if polyphonic CV not available for this channel)
            auto getCVInput = [](Input& input, int channel) -> float {
                if (!input.isConnected()) return 0.0f;
                int cvChannels = input.getChannels();
                int useChan = (channel < cvChannels) ? channel : 0;
                return input.getPolyVoltage(useChan);
            };

            float delayTimeL = params[DELAY_TIME_L_PARAM].getValue();
            if (inputs[DELAY_TIME_L_CV_INPUT].isConnected()) {
                float cv = getCVInput(inputs[DELAY_TIME_L_CV_INPUT], c);
                delayTimeL += cv * 0.2f;
            }
            if (delayChaosMod) {
                delayTimeL += chaosOutput * 0.1f;
            }
            delayTimeL = clamp(delayTimeL, 0.001f, 2.0f);

            float delayTimeR = params[DELAY_TIME_R_PARAM].getValue();
            if (inputs[DELAY_TIME_R_CV_INPUT].isConnected()) {
                float cv = getCVInput(inputs[DELAY_TIME_R_CV_INPUT], c);
                delayTimeR += cv * 0.2f;
            }
            if (delayChaosMod) {
                delayTimeR += chaosOutput * 0.1f;
            }
            delayTimeR = clamp(delayTimeR, 0.001f, 2.0f);

            float feedback = params[DELAY_FEEDBACK_PARAM].getValue();
            if (inputs[DELAY_FEEDBACK_CV_INPUT].isConnected()) {
                float cv = getCVInput(inputs[DELAY_FEEDBACK_CV_INPUT], c);
                feedback += cv * 0.1f;
            }
            if (delayChaosMod) {
                feedback += chaosOutput * 0.1f;
            }
            feedback = clamp(feedback, 0.0f, 0.95f);

            int delaySamplesL = (int)(delayTimeL * args.sampleRate);
            delaySamplesL = clamp(delaySamplesL, 1, DELAY_BUFFER_SIZE - 1);

            int delaySamplesR = (int)(delayTimeR * args.sampleRate);
            delaySamplesR = clamp(delaySamplesR, 1, DELAY_BUFFER_SIZE - 1);

            int readIndexL = (delayWriteIndex[c] - delaySamplesL + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
            int readIndexR = (delayWriteIndex[c] - delaySamplesR + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;

            float leftDelayedSignal = leftDelayBuffer[c][readIndexL];
            float rightDelayedSignal = rightDelayBuffer[c][readIndexR];

            float grainSize = params[GRAIN_SIZE_PARAM].getValue();
            if (inputs[GRAIN_SIZE_CV_INPUT].isConnected()) {
                grainSize += getCVInput(inputs[GRAIN_SIZE_CV_INPUT], c) * 0.1f;
            }
            grainSize = clamp(grainSize, 0.0f, 1.0f);

            float grainDensity = params[GRAIN_DENSITY_PARAM].getValue();
            if (inputs[GRAIN_DENSITY_CV_INPUT].isConnected()) {
                grainDensity += getCVInput(inputs[GRAIN_DENSITY_CV_INPUT], c) * 0.1f;
            }
            grainDensity = clamp(grainDensity, 0.0f, 1.0f);

            float grainPosition = params[GRAIN_POSITION_PARAM].getValue();
            if (inputs[GRAIN_POSITION_CV_INPUT].isConnected()) {
                grainPosition += getCVInput(inputs[GRAIN_POSITION_CV_INPUT], c) * 0.1f;
            }
            grainPosition = clamp(grainPosition, 0.0f, 1.0f);

            float reverbRoomSize = params[REVERB_ROOM_SIZE_PARAM].getValue();
            if (inputs[REVERB_ROOM_SIZE_CV_INPUT].isConnected()) {
                reverbRoomSize += getCVInput(inputs[REVERB_ROOM_SIZE_CV_INPUT], c) * 0.1f;
            }
            reverbRoomSize = clamp(reverbRoomSize, 0.0f, 1.0f);

            float reverbDamping = params[REVERB_DAMPING_PARAM].getValue();
            if (inputs[REVERB_DAMPING_CV_INPUT].isConnected()) {
                reverbDamping += getCVInput(inputs[REVERB_DAMPING_CV_INPUT], c) * 0.1f;
            }
            reverbDamping = clamp(reverbDamping, 0.0f, 1.0f);

            float reverbDecay = params[REVERB_DECAY_PARAM].getValue();
            if (inputs[REVERB_DECAY_CV_INPUT].isConnected()) {
                reverbDecay += getCVInput(inputs[REVERB_DECAY_CV_INPUT], c) * 0.1f;
            }
            reverbDecay = clamp(reverbDecay, 0.0f, 1.0f);

            float leftDelayInput = leftInput + leftDelayedSignal * feedback;
            float rightDelayInput = rightInput + rightDelayedSignal * feedback;

            leftDelayBuffer[c][delayWriteIndex[c]] = leftDelayInput;
            rightDelayBuffer[c][delayWriteIndex[c]] = rightDelayInput;
            delayWriteIndex[c] = (delayWriteIndex[c] + 1) % DELAY_BUFFER_SIZE;

            // True serial chain: each stage feeds the next

            // Stage 1: Delay wet/dry mix
            float delayWetDryMix = params[WET_DRY_PARAM].getValue();
            float leftStage1 = leftInput * (1.0f - delayWetDryMix) + leftDelayedSignal * delayWetDryMix;
            float rightStage1 = rightInput * (1.0f - delayWetDryMix) + rightDelayedSignal * delayWetDryMix;

            // Stage 2: Grain processing on stage 1 output
            float leftGrainOutput = leftGrainProcessor[c].process(leftStage1, grainSize, grainDensity, grainPosition, grainChaosMod, chaosOutput, args.sampleRate);
            float rightGrainOutput = rightGrainProcessor[c].process(rightStage1, grainSize, grainDensity, grainPosition, grainChaosMod, chaosOutput * -1.0f, args.sampleRate);

            float grainWetDryMix = params[GRAIN_WET_DRY_PARAM].getValue();
            float leftStage2 = leftStage1 * (1.0f - grainWetDryMix) + leftGrainOutput * grainWetDryMix;
            float rightStage2 = rightStage1 * (1.0f - grainWetDryMix) + rightGrainOutput * grainWetDryMix;

            // Stage 3: Reverb processing on stage 2 output
            float leftReverbOutput = leftReverbProcessor[c].process(leftStage2, rightStage2, grainDensity, reverbRoomSize, reverbDamping, reverbDecay, true, reverbChaosMod, chaosOutput, args.sampleRate);
            float rightReverbOutput = rightReverbProcessor[c].process(leftStage2, rightStage2, grainDensity, reverbRoomSize, reverbDamping, reverbDecay, false, reverbChaosMod, chaosOutput, args.sampleRate);

            float reverbWetDryMix = params[REVERB_WET_DRY_PARAM].getValue();
            float leftFinal = leftStage2 * (1.0f - reverbWetDryMix) + leftReverbOutput * reverbWetDryMix;
            float rightFinal = rightStage2 * (1.0f - reverbWetDryMix) + rightReverbOutput * reverbWetDryMix;

            // Add reverb feedback to delay input for next frame (creates extended decay)
            float reverbFeedbackAmount = reverbDecay * 0.3f;
            leftDelayBuffer[c][delayWriteIndex[c]] += leftReverbOutput * reverbFeedbackAmount;
            rightDelayBuffer[c][delayWriteIndex[c]] += rightReverbOutput * reverbFeedbackAmount;

            // Final output validation
            if (!std::isfinite(leftFinal)) leftFinal = 0.0f;
            if (!std::isfinite(rightFinal)) rightFinal = 0.0f;

            outputs[LEFT_AUDIO_OUTPUT].setVoltage(leftFinal, c);
            outputs[RIGHT_AUDIO_OUTPUT].setVoltage(rightFinal, c);
        } // End of polyphonic channel loop
    }

    void processBypass(const ProcessArgs& args) override {
        int leftChannels = inputs[LEFT_AUDIO_INPUT].getChannels();
        int rightChannels = inputs[RIGHT_AUDIO_INPUT].getChannels();
        int channels = std::max({1, leftChannels, rightChannels});

        outputs[LEFT_AUDIO_OUTPUT].setChannels(channels);
        outputs[RIGHT_AUDIO_OUTPUT].setChannels(channels);

        for (int c = 0; c < channels; c++) {
            float leftInput = (c < leftChannels) ? inputs[LEFT_AUDIO_INPUT].getPolyVoltage(c) : 0.0f;
            float rightInput = 0.0f;
            if (inputs[RIGHT_AUDIO_INPUT].isConnected()) {
                rightInput = (c < rightChannels) ? inputs[RIGHT_AUDIO_INPUT].getPolyVoltage(c) : 0.0f;
            } else {
                rightInput = leftInput;
            }

            outputs[LEFT_AUDIO_OUTPUT].setVoltage(leftInput, c);
            outputs[RIGHT_AUDIO_OUTPUT].setVoltage(rightInput, c);
        }
    }
};

struct EllenRipleyWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    EllenRipleyWidget(EllenRipley* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP");
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new EnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Ellen Ripley", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new EnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new EnhancedTextLabel(Vec(0, 30), Vec(box.size.x, 15), "DELAY", 10.f, nvgRGB(255, 255, 255), true));
        
        float delayY = 46;
        float x = 1;
        
        addChild(new EnhancedTextLabel(Vec(x, delayY), Vec(25, 10), "TIME L", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, delayY + 22), module, EllenRipley::DELAY_TIME_L_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, delayY + 47), module, EllenRipley::DELAY_TIME_L_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, delayY), Vec(25, 10), "TIME R", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, delayY + 22), module, EllenRipley::DELAY_TIME_R_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, delayY + 47), module, EllenRipley::DELAY_TIME_R_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, delayY), Vec(25, 10), "FDBK", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, delayY + 22), module, EllenRipley::DELAY_FEEDBACK_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, delayY + 47), module, EllenRipley::DELAY_FEEDBACK_CV_INPUT));
        x += 30;
        
        addChild(new EnhancedTextLabel(Vec(x, delayY), Vec(25, 10), "C", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(x + 12, delayY + 22), module, EllenRipley::DELAY_CHAOS_PARAM, EllenRipley::DELAY_CHAOS_LIGHT));
        
        addChild(new EnhancedTextLabel(Vec(0, 112), Vec(box.size.x, 15), "GRATCH", 10.f, nvgRGB(255, 255, 255), true));
        
        float grainY = 128;
        x = 1;
        
        addChild(new EnhancedTextLabel(Vec(x, grainY), Vec(25, 10), "SIZE", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, grainY + 22), module, EllenRipley::GRAIN_SIZE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, grainY + 47), module, EllenRipley::GRAIN_SIZE_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, grainY), Vec(25, 10), "BREAK", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, grainY + 22), module, EllenRipley::GRAIN_DENSITY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, grainY + 47), module, EllenRipley::GRAIN_DENSITY_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, grainY), Vec(25, 10), "SHIFT", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, grainY + 22), module, EllenRipley::GRAIN_POSITION_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, grainY + 47), module, EllenRipley::GRAIN_POSITION_CV_INPUT));
        x += 30;
        
        addChild(new EnhancedTextLabel(Vec(x, grainY), Vec(25, 10), "C", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(x + 12, grainY + 22), module, EllenRipley::GRAIN_CHAOS_PARAM, EllenRipley::GRAIN_CHAOS_LIGHT));
        
        addChild(new EnhancedTextLabel(Vec(0, 194), Vec(box.size.x, 15), "REVERB", 10.f, nvgRGB(255, 255, 255), true));
        
        float reverbY = 210;
        x = 1;
        
        addChild(new EnhancedTextLabel(Vec(x, reverbY), Vec(25, 10), "ROOM", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, reverbY + 22), module, EllenRipley::REVERB_ROOM_SIZE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, reverbY + 47), module, EllenRipley::REVERB_ROOM_SIZE_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, reverbY), Vec(25, 10), "TONE", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, reverbY + 22), module, EllenRipley::REVERB_DAMPING_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, reverbY + 47), module, EllenRipley::REVERB_DAMPING_CV_INPUT));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, reverbY), Vec(25, 10), "DECAY", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, reverbY + 22), module, EllenRipley::REVERB_DECAY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x + 12, reverbY + 47), module, EllenRipley::REVERB_DECAY_CV_INPUT));
        x += 30;
        
        addChild(new EnhancedTextLabel(Vec(x, reverbY), Vec(25, 10), "C", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(x + 12, reverbY + 22), module, EllenRipley::REVERB_CHAOS_PARAM, EllenRipley::REVERB_CHAOS_LIGHT));
        
        addChild(new EnhancedTextLabel(Vec(0, 276), Vec(box.size.x, 15), "CHAOS", 10.f, nvgRGB(255, 255, 255), true));
        
        // Chaos shape button next to CHAOS text, above reverb wet/dry
        addChild(new EnhancedTextLabel(Vec(95, 276), Vec(25, 10), "SHAPE", 6.f, nvgRGB(255, 133, 133), true));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(107, 282), module, EllenRipley::CHAOS_SHAPE_PARAM, EllenRipley::CHAOS_SHAPE_LIGHT));
        
        float chaosY = 292;
        x = 1;
        
        addChild(new EnhancedTextLabel(Vec(x, chaosY), Vec(25, 10), "RATE", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, chaosY + 22), module, EllenRipley::CHAOS_RATE_PARAM));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, chaosY), Vec(25, 10), "DELAY", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, chaosY + 22), module, EllenRipley::WET_DRY_PARAM));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, chaosY), Vec(25, 10), "GRATCH", 7.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, chaosY + 22), module, EllenRipley::GRAIN_WET_DRY_PARAM));
        x += 31;
        
        addChild(new EnhancedTextLabel(Vec(x, chaosY), Vec(25, 10), "REVERB", 6.f, nvgRGB(200, 200, 200), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(x + 12, chaosY + 22), module, EllenRipley::REVERB_WET_DRY_PARAM));
        
        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addInput(createInputCentered<PJ301MPort>(Vec(15, 343), module, EllenRipley::LEFT_AUDIO_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(15, 368), module, EllenRipley::RIGHT_AUDIO_INPUT));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 343), module, EllenRipley::LEFT_AUDIO_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(105, 368), module, EllenRipley::RIGHT_AUDIO_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 335), Vec(30, 10), "CHAOS OUT", 7.f, nvgRGB(255, 133, 133), true));
        addOutput(createOutputCentered<PJ301MPort>(Vec(80, 343), module, EllenRipley::CHAOS_CV_OUTPUT));
        
        addChild(new EnhancedTextLabel(Vec(35, 365), Vec(30, 10), "AMOUNT", 7.f, nvgRGB(255, 133, 133), true));
        addParam(createParamCentered<StandardBlackKnob26>(Vec(80, 368), module, EllenRipley::CHAOS_AMOUNT_PARAM));
    }

    void step() override {
        EllenRipley* module = dynamic_cast<EllenRipley*>(this->module);
        if (module) {
            panelThemeHelper.step(module);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        EllenRipley* module = dynamic_cast<EllenRipley*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);
    }
};

Model* modelEllenRipley = createModel<EllenRipley, EllenRipleyWidget>("EllenRipley");