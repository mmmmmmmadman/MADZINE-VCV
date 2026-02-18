#include "plugin.hpp"
#include "widgets/Knobs.hpp"
#include "widgets/PanelTheme.hpp"
#include <cmath>
#include <osdialog.h>
#include <sst/filters/HalfRateFilter.h>

// ============================================================================
// Shared Widgets
// ============================================================================

struct theKICKTextLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;
    bool bold;

    theKICKTextLabel(Vec pos, Vec size, std::string text, float fontSize = 8.f,
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

struct theKICKWhiteBox : Widget {
    theKICKWhiteBox(Vec pos, Vec size) {
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

// ============================================================================
// Module
// ============================================================================

struct theKICK : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;

    enum ParamId {
        PITCH_PARAM,
        SWEEP_PARAM,
        BEND_PARAM,
        DECAY_PARAM,
        FOLD_PARAM,
        SAMPLE_PARAM,
        FB_PARAM,
        TONE_PARAM,
        MODE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        TRIGGER_INPUT,
        PITCH_CV_INPUT,
        SWEEP_CV_INPUT,
        BEND_CV_INPUT,
        DECAY_CV_INPUT,
        FOLD_CV_INPUT,
        FB_CV_INPUT,
        TONE_CV_INPUT,
        SAMPLE_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        MODE_LIGHT_RED,
        MODE_LIGHT_GREEN,
        MODE_LIGHT_BLUE,
        LIGHTS_LEN
    };

    // --- DSP state ---
    float phase = 0.f;
    float pitchEnvTime = 0.f;
    float ampEnvTime = 0.f;
    bool active = false;
    dsp::SchmittTrigger triggerDetect;

    // Feedback FM state
    float fbY1 = 0.f;
    float fbY2 = 0.f;

    // LPF state (4-pole, 24dB/oct)
    float lpfState[4] = {};

    // Sample FM playback position
    float samplePlayPos = 0.f;

    // --- Sample-as-Transfer ---
    static constexpr int TABLE_SIZE = 1024;
    float sampleTable[TABLE_SIZE] = {};
    bool hasSample = false;
    std::string samplePath;

    // --- Mode (waveshaper type) ---
    // 0=Saturate(red), 1=Fold(green), 2=Asymmetric(blue), 3=Destroy(white)
    int modeValue = 0;
    dsp::SchmittTrigger modeTrigger;

    // --- 2x Oversampling (from NIGOQ) ---
    static constexpr int BLOCK_SIZE = 8;
    int oversampleRate = 2;
    sst::filters::HalfRate::HalfRateFilter downFilter{6, true};
    static constexpr int MAX_BLOCK_SIZE_OS = BLOCK_SIZE * 8;
    float outputBuffer[MAX_BLOCK_SIZE_OS] = {};
    float outputDownsampled[BLOCK_SIZE] = {};
    int processPosition = BLOCK_SIZE + 1;

    // --- CV modulation display ---
    float pitchCvMod = 0.f;
    float sweepCvMod = 0.f;
    float bendCvMod = 0.f;
    float decayCvMod = 0.f;
    float foldCvMod = 0.f;
    float sampleCvMod = 0.f;
    float fbCvMod = 0.f;
    float toneCvMod = 0.f;

    // --- Cached params for processSingleSample ---
    struct ProcessState {
        float pitch, sweep, bend, decayMs, fold, sampleFm, fb, toneCutoff;
    };

    // ========================================================================
    // Constructor
    // ========================================================================

    theKICK() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(PITCH_PARAM, 20.f, 200.f, 46.3f, "Pitch", " Hz");
        configParam(SWEEP_PARAM, 0.f, 500.f, 158.f, "Sweep", " Hz");
        configParam(BEND_PARAM, 0.5f, 4.f, 1.f, "Bend");
        configParam(DECAY_PARAM, 10.f, 1000.f, 136.f, "Decay", " ms");
        configParam(FOLD_PARAM, 0.f, 10.f, 0.3f, "Fold");
        configParam(SAMPLE_PARAM, 0.f, 10.f, 0.f, "Sample");
        configParam(FB_PARAM, 0.f, 1.f, 0.f, "Feedback");
        configParam(TONE_PARAM, 0.f, 10.f, 10.f, "Tone");
        configParam(MODE_PARAM, 0.f, 3.f, 0.f, "Waveshaper Mode");
        getParamQuantity(MODE_PARAM)->snapEnabled = true;

        configInput(TRIGGER_INPUT, "Trigger");
        configInput(PITCH_CV_INPUT, "Pitch CV (V/Oct)");
        configInput(SWEEP_CV_INPUT, "Sweep CV");
        configInput(BEND_CV_INPUT, "Bend CV");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(FOLD_CV_INPUT, "Fold CV");
        configInput(FB_CV_INPUT, "Feedback CV");
        configInput(TONE_CV_INPUT, "Tone CV");
        configInput(SAMPLE_CV_INPUT, "Sample CV");

        configOutput(OUT_OUTPUT, "Kick Output");

        configLight(MODE_LIGHT_RED, "Mode Red");
        configLight(MODE_LIGHT_GREEN, "Mode Green");
        configLight(MODE_LIGHT_BLUE, "Mode Blue");
    }

    // ========================================================================
    // Oversampling setup
    // ========================================================================

    void setupOversamplingFilters() {
        downFilter.reset();
        processPosition = BLOCK_SIZE + 1;
        for (int i = 0; i < MAX_BLOCK_SIZE_OS; i++) outputBuffer[i] = 0.f;
        for (int i = 0; i < BLOCK_SIZE; i++) outputDownsampled[i] = 0.f;
    }

    void onSampleRateChange() override {
        setupOversamplingFilters();
    }

    void onReset() override {
        phase = 0.f;
        pitchEnvTime = 0.f;
        ampEnvTime = 0.f;
        active = false;
        fbY1 = 0.f;
        fbY2 = 0.f;
        for (int i = 0; i < 4; i++) lpfState[i] = 0.f;
        samplePlayPos = 0.f;
        modeValue = 0;
        hasSample = false;
        samplePath.clear();
        for (int i = 0; i < TABLE_SIZE; i++) sampleTable[i] = 0.f;
        setupOversamplingFilters();
    }

    // ========================================================================
    // Sample loading
    // ========================================================================

    void loadSampleFromFile() {
        char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL,
                                   osdialog_filters_parse("WAV:wav"));
        if (!path) return;
        loadSampleTable(std::string(path));
        OSDIALOG_FREE(path);
    }

    void loadSampleTable(const std::string& path) {
        FILE* file = std::fopen(path.c_str(), "rb");
        if (!file) {
            WARN("theKICK: Could not open WAV: %s", path.c_str());
            return;
        }

        // Read RIFF header
        char riff[4], wave[4];
        uint32_t fileSize;
        std::fread(riff, 1, 4, file);
        std::fread(&fileSize, 4, 1, file);
        std::fread(wave, 1, 4, file);

        if (std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(wave, "WAVE", 4) != 0) {
            std::fclose(file);
            WARN("theKICK: Invalid WAV: %s", path.c_str());
            return;
        }

        // Find fmt and data chunks
        uint16_t numChannels = 0;
        uint32_t sampleRate = 0;
        uint16_t bitsPerSample = 0;
        uint32_t dataSize = 0;
        long dataPos = 0;

        while (!std::feof(file)) {
            char chunkId[4];
            uint32_t chunkSize;
            if (std::fread(chunkId, 1, 4, file) != 4) break;
            if (std::fread(&chunkSize, 4, 1, file) != 1) break;

            if (std::memcmp(chunkId, "fmt ", 4) == 0) {
                uint16_t audioFormat;
                std::fread(&audioFormat, 2, 1, file);
                std::fread(&numChannels, 2, 1, file);
                std::fread(&sampleRate, 4, 1, file);
                std::fseek(file, 6, SEEK_CUR);
                std::fread(&bitsPerSample, 2, 1, file);
                std::fseek(file, chunkSize - 16, SEEK_CUR);
            } else if (std::memcmp(chunkId, "data", 4) == 0) {
                dataSize = chunkSize;
                dataPos = std::ftell(file);
                break;
            } else {
                std::fseek(file, chunkSize, SEEK_CUR);
            }
        }

        if (dataSize == 0 || dataPos == 0) {
            std::fclose(file);
            WARN("theKICK: No audio data in WAV: %s", path.c_str());
            return;
        }

        std::fseek(file, dataPos, SEEK_SET);

        int bytesPerSample = bitsPerSample / 8;
        int numFrames = dataSize / (numChannels * bytesPerSample);

        // Read all samples into temp buffer (mono, first channel)
        std::vector<float> rawSamples;
        rawSamples.reserve(numFrames);

        for (int i = 0; i < numFrames; i++) {
            float sample = 0.f;
            if (bitsPerSample == 16) {
                int16_t s16;
                std::fread(&s16, 2, 1, file);
                sample = s16 / 32768.f;
                if (numChannels >= 2)
                    std::fseek(file, (numChannels - 1) * 2, SEEK_CUR);
            } else if (bitsPerSample == 24) {
                uint8_t bytes[3];
                std::fread(bytes, 1, 3, file);
                int32_t s24 = (bytes[2] << 24) | (bytes[1] << 16) | (bytes[0] << 8);
                s24 >>= 8;
                sample = s24 / 8388608.f;
                if (numChannels >= 2)
                    std::fseek(file, (numChannels - 1) * 3, SEEK_CUR);
            } else {
                // Unsupported bit depth, skip
                std::fseek(file, numChannels * bytesPerSample, SEEK_CUR);
            }
            rawSamples.push_back(sample);
        }
        std::fclose(file);

        if (rawSamples.empty()) return;

        // Resample to TABLE_SIZE using linear interpolation
        float peak = 0.f;
        for (float s : rawSamples) peak = std::max(peak, std::fabs(s));
        if (peak < 0.0001f) peak = 1.f;

        for (int i = 0; i < TABLE_SIZE; i++) {
            float pos = (float)i / (float)(TABLE_SIZE - 1) * (float)(rawSamples.size() - 1);
            int idx = (int)pos;
            float frac = pos - idx;
            int next = std::min(idx + 1, (int)rawSamples.size() - 1);
            sampleTable[i] = (rawSamples[idx] * (1.f - frac) + rawSamples[next] * frac) / peak;
        }

        hasSample = true;
        samplePath = path;
        INFO("theKICK: Loaded sample table from %s (%d frames)", path.c_str(), numFrames);
    }

    void clearSample() {
        hasSample = false;
        samplePath.clear();
        for (int i = 0; i < TABLE_SIZE; i++) sampleTable[i] = 0.f;
    }

    // ========================================================================
    // Waveshaper functions
    // ========================================================================

    // Lookup sample transfer table with linear interpolation
    float lookupSampleTable(float x) {
        // x in [-1, 1] -> index in [0, TABLE_SIZE-1]
        float normalized = (x + 1.f) * 0.5f;
        float pos = normalized * (TABLE_SIZE - 1);
        int idx = clamp((int)pos, 0, TABLE_SIZE - 2);
        float frac = pos - idx;
        return sampleTable[idx] * (1.f - frac) + sampleTable[idx + 1] * frac;
    }

    // Apply waveshaper based on mode
    float applyWaveshaper(float x, float gain, int mode) {
        switch (mode) {
            case 0: { // Saturate: tanh soft clip
                float tanhG = std::tanh(gain);
                if (tanhG < 0.001f) return x;
                return std::tanh(x * gain) / tanhG;
            }
            case 1: { // Fold: sine wavefolder
                return std::sin(x * gain);
            }
            case 2: { // Asymmetric: tube-like even harmonics
                float asymK = 0.3f;
                float input = x * gain + asymK * gain * x * x;
                float normFactor = std::tanh(gain * (1.f + asymK));
                if (normFactor < 0.001f) return x;
                return std::tanh(input) / normFactor;
            }
            case 3: { // Destroy: compound fold + saturate
                return std::sin(gain * std::tanh(2.f * x));
            }
            default:
                return x;
        }
    }

    // ========================================================================
    // Single sample DSP (called at oversampled rate)
    // ========================================================================

    float processSingleSample(const ProcessState& state, float sampleTime) {
        if (!active) return 0.f;

        // Pitch envelope: freq = pitch + sweep * exp(-t / (0.015 / bend))
        float pitchTau = 0.015f / state.bend;
        float pitchEnv = state.sweep * std::exp(-pitchEnvTime / pitchTau);
        float freq = state.pitch + pitchEnv;

        // Self-feedback PM
        float fbPhase = 0.f;
        if (state.fb > 0.001f) {
            fbPhase = state.fb * 0.5f * (fbY1 + fbY2);
        }

        // Sample PM: sample table modulates phase, envelope follows pitch sweep
        float samplePhase = 0.f;
        if (hasSample && state.sampleFm > 0.01f) {
            // Read sample table at current playback position (wrapping)
            float tablePos = samplePlayPos * TABLE_SIZE;
            int idx = ((int)tablePos) % TABLE_SIZE;
            if (idx < 0) idx += TABLE_SIZE;
            int next = (idx + 1) % TABLE_SIZE;
            float frac = tablePos - std::floor(tablePos);
            float sampleVal = sampleTable[idx] * (1.f - frac) + sampleTable[next] * frac;

            // FM index from SAMPLE knob (0-10 -> 0-PI)
            float fmIndex = state.sampleFm * M_PI / 10.f;

            // Envelope follows pitch sweep (decays with BEND)
            float sampleEnv = std::exp(-pitchEnvTime / pitchTau);
            samplePhase = fmIndex * sampleVal * sampleEnv;

            // Advance sample playback at oscillator frequency
            samplePlayPos += freq * sampleTime;
            while (samplePlayPos >= 1.f) samplePlayPos -= 1.f;
        }

        // Phase accumulator
        phase += freq * sampleTime;
        while (phase >= 1.f) phase -= 1.f;
        while (phase < 0.f) phase += 1.f;

        // Sine oscillator with feedback + sample phase modulation
        float osc = std::sin(2.f * M_PI * phase + fbPhase + samplePhase);

        // Waveshaper
        float shaped;
        if (state.fold > 0.01f) {
            float gain;
            switch (modeValue) {
                case 0: gain = 1.f + state.fold * 2.f; break;
                case 1: gain = state.fold * M_PI * 0.5f; break;
                case 2: gain = 1.f + state.fold * 2.f; break;
                case 3: gain = 1.f + state.fold * 1.5f; break;
                default: gain = 1.f; break;
            }
            shaped = applyWaveshaper(osc, gain, modeValue);
        } else {
            shaped = osc;
        }

        // Update feedback state
        fbY2 = fbY1;
        fbY1 = shaped;

        // Tone LPF (4-pole, 24dB/oct cascaded one-pole with frequency warping)
        float fc = state.toneCutoff * sampleTime;
        fc = clamp(fc, 0.0001f, 0.4999f);
        float wc = std::tan(M_PI * fc);
        float lpAlpha = wc / (1.f + wc);
        lpfState[0] = shaped * lpAlpha + lpfState[0] * (1.f - lpAlpha);
        lpfState[1] = lpfState[0] * lpAlpha + lpfState[1] * (1.f - lpAlpha);
        lpfState[2] = lpfState[1] * lpAlpha + lpfState[2] * (1.f - lpAlpha);
        lpfState[3] = lpfState[2] * lpAlpha + lpfState[3] * (1.f - lpAlpha);
        float filtered = lpfState[3];

        // Amplitude envelope: exponential decay
        float decaySec = state.decayMs * 0.001f;
        float ampEnv = std::exp(-ampEnvTime / decaySec);

        // Output (±5V)
        float output = filtered * ampEnv * 5.f;

        // Advance envelope times
        pitchEnvTime += sampleTime;
        ampEnvTime += sampleTime;

        // Deactivate when silent
        if (ampEnv < 0.001f) {
            active = false;
        }

        return output;
    }

    // ========================================================================
    // Main process
    // ========================================================================

    void process(const ProcessArgs& args) override {
        // Read parameters
        float pitch = params[PITCH_PARAM].getValue();
        float sweep = params[SWEEP_PARAM].getValue();
        float bend = params[BEND_PARAM].getValue();
        float decayMs = params[DECAY_PARAM].getValue();
        float fold = params[FOLD_PARAM].getValue();
        float sampleMix = params[SAMPLE_PARAM].getValue();
        float fb = params[FB_PARAM].getValue();
        float toneKnob = params[TONE_PARAM].getValue();

        // Apply CV modulation
        if (inputs[PITCH_CV_INPUT].isConnected()) {
            float cv = inputs[PITCH_CV_INPUT].getVoltage();
            pitch *= std::pow(2.f, cv);
            pitchCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { pitchCvMod = 0.f; }

        if (inputs[SWEEP_CV_INPUT].isConnected()) {
            float cv = inputs[SWEEP_CV_INPUT].getVoltage();
            sweep += cv * 50.f;
            sweep = clamp(sweep, 0.f, 1000.f);
            sweepCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { sweepCvMod = 0.f; }

        if (inputs[BEND_CV_INPUT].isConnected()) {
            float cv = inputs[BEND_CV_INPUT].getVoltage();
            bend += cv * 0.35f;
            bend = clamp(bend, 0.5f, 4.f);
            bendCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { bendCvMod = 0.f; }

        if (inputs[DECAY_CV_INPUT].isConnected()) {
            float cv = inputs[DECAY_CV_INPUT].getVoltage();
            decayMs += cv * 100.f;
            decayMs = clamp(decayMs, 10.f, 2000.f);
            decayCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { decayCvMod = 0.f; }

        if (inputs[FOLD_CV_INPUT].isConnected()) {
            float cv = inputs[FOLD_CV_INPUT].getVoltage();
            fold += cv;
            fold = clamp(fold, 0.f, 10.f);
            foldCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { foldCvMod = 0.f; }

        if (inputs[SAMPLE_CV_INPUT].isConnected()) {
            float cv = inputs[SAMPLE_CV_INPUT].getVoltage();
            sampleMix += cv;
            sampleMix = clamp(sampleMix, 0.f, 10.f);
            sampleCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { sampleCvMod = 0.f; }

        if (inputs[FB_CV_INPUT].isConnected()) {
            float cv = inputs[FB_CV_INPUT].getVoltage();
            fb += cv * 0.1f;
            fb = clamp(fb, 0.f, 1.f);
            fbCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { fbCvMod = 0.f; }

        if (inputs[TONE_CV_INPUT].isConnected()) {
            float cv = inputs[TONE_CV_INPUT].getVoltage();
            toneKnob += cv;
            toneKnob = clamp(toneKnob, 0.f, 10.f);
            toneCvMod = clamp(cv / 5.f, -1.f, 1.f);
        } else { toneCvMod = 0.f; }

        // Tone knob to frequency: 0=40Hz, 10=20kHz (logarithmic)
        float toneCutoff = 40.f * std::pow(500.f, toneKnob / 10.f);

        // Mode LED button
        if (modeTrigger.process(params[MODE_PARAM].getValue())) {
            modeValue = (modeValue + 1) % 4;
            params[MODE_PARAM].setValue((float)modeValue);
        }

        // Update mode LED colors (midpoint between muted and pure YRGB)
        switch (modeValue) {
            case 0: // Saturate: vivid amber
                lights[MODE_LIGHT_RED].setBrightness(0.890f);
                lights[MODE_LIGHT_GREEN].setBrightness(0.731f);
                lights[MODE_LIGHT_BLUE].setBrightness(0.039f);
                break;
            case 1: // Fold: vivid rose
                lights[MODE_LIGHT_RED].setBrightness(0.890f);
                lights[MODE_LIGHT_GREEN].setBrightness(0.080f);
                lights[MODE_LIGHT_BLUE].setBrightness(0.102f);
                break;
            case 2: // Asymmetric: vivid green
                lights[MODE_LIGHT_RED].setBrightness(0.080f);
                lights[MODE_LIGHT_GREEN].setBrightness(0.820f);
                lights[MODE_LIGHT_BLUE].setBrightness(0.127f);
                break;
            case 3: // Destroy: vivid blue
                lights[MODE_LIGHT_RED].setBrightness(0.102f);
                lights[MODE_LIGHT_GREEN].setBrightness(0.127f);
                lights[MODE_LIGHT_BLUE].setBrightness(0.890f);
                break;
        }

        // Trigger detection
        if (triggerDetect.process(inputs[TRIGGER_INPUT].getVoltage(), 0.1f, 2.f)) {
            phase = 0.f;
            pitchEnvTime = 0.f;
            ampEnvTime = 0.f;
            fbY1 = 0.f;
            fbY2 = 0.f;
            for (int i = 0; i < 4; i++) lpfState[i] = 0.f;
            samplePlayPos = 0.f;
            active = true;
        }

        // Build process state
        ProcessState state;
        state.pitch = pitch;
        state.sweep = sweep;
        state.bend = bend;
        state.decayMs = decayMs;
        state.fold = fold;
        state.sampleFm = sampleMix;
        state.fb = fb;
        state.toneCutoff = toneCutoff;

        // Process with oversampling
        float outputFinal;

        if (oversampleRate == 1) {
            outputFinal = processSingleSample(state, args.sampleTime);
        } else {
            // Block-based 2x oversampling
            if (processPosition >= BLOCK_SIZE) {
                processPosition = 0;
                float osSampleTime = args.sampleTime / 2.f;
                int blockSizeOS = BLOCK_SIZE * 2;

                for (int i = 0; i < blockSizeOS; i++) {
                    outputBuffer[i] = processSingleSample(state, osSampleTime);
                }

                // Downsample 2x -> 1x
                downFilter.process_block_D2(outputBuffer, outputBuffer, BLOCK_SIZE * 2);

                // 2x gain compensation
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    outputDownsampled[i] = outputBuffer[i] * 2.f;
                }
            }

            outputFinal = outputDownsampled[processPosition];
            processPosition++;
        }

        outputs[OUT_OUTPUT].setVoltage(outputFinal);
    }

    // ========================================================================
    // JSON serialization
    // ========================================================================

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_object_set_new(rootJ, "modeValue", json_integer(modeValue));
        json_object_set_new(rootJ, "oversampleRate", json_integer(oversampleRate));

        // Save sample table
        if (hasSample) {
            json_object_set_new(rootJ, "hasSample", json_true());
            if (!samplePath.empty())
                json_object_set_new(rootJ, "samplePath", json_string(samplePath.c_str()));
            json_t* tableJ = json_array();
            for (int i = 0; i < TABLE_SIZE; i++) {
                json_array_append_new(tableJ, json_real(sampleTable[i]));
            }
            json_object_set_new(rootJ, "sampleTable", tableJ);
        }

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* themeJ = json_object_get(rootJ, "panelTheme");
        if (themeJ) panelTheme = json_integer_value(themeJ);

        json_t* contrastJ = json_object_get(rootJ, "panelContrast");
        if (contrastJ) panelContrast = json_real_value(contrastJ);

        json_t* modeJ = json_object_get(rootJ, "modeValue");
        if (modeJ) {
            modeValue = json_integer_value(modeJ);
            params[MODE_PARAM].setValue((float)modeValue);
        }

        json_t* osJ = json_object_get(rootJ, "oversampleRate");
        if (osJ) {
            oversampleRate = json_integer_value(osJ);
            if (oversampleRate != 1 && oversampleRate != 2)
                oversampleRate = 2;
        }

        json_t* hasSampleJ = json_object_get(rootJ, "hasSample");
        if (hasSampleJ && json_is_true(hasSampleJ)) {
            json_t* tableJ = json_object_get(rootJ, "sampleTable");
            if (tableJ && json_is_array(tableJ)) {
                int len = std::min((int)json_array_size(tableJ), TABLE_SIZE);
                for (int i = 0; i < len; i++) {
                    sampleTable[i] = json_number_value(json_array_get(tableJ, i));
                }
                hasSample = true;
            }
            json_t* pathJ = json_object_get(rootJ, "samplePath");
            if (pathJ) samplePath = json_string_value(pathJ);
        }

        setupOversamplingFilters();
    }
};

// ============================================================================
// Mode LED right-click overlay
// ============================================================================

struct theKICKModeOverlay : OpaqueWidget {
    theKICK* module = nullptr;

    theKICKModeOverlay() {
        box.size = Vec(16, 16);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (module) {
                module->modeValue = (module->modeValue + 1) % 4;
                module->params[theKICK::MODE_PARAM].setValue((float)module->modeValue);
            }
            e.consume(this);
            return;
        }
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (module) {
                ui::Menu* menu = createMenu();
                menu->addChild(createMenuLabel("Waveshaper Mode"));
                const char* names[] = {"Saturate", "Fold", "Asymmetric", "Destroy"};
                for (int i = 0; i < 4; i++) {
                    menu->addChild(createMenuItem(names[i], CHECKMARK(module->modeValue == i), [=]() {
                        module->modeValue = i;
                        module->params[theKICK::MODE_PARAM].setValue((float)i);
                    }));
                }
            }
            e.consume(this);
            return;
        }
        OpaqueWidget::onButton(e);
    }
};

// ============================================================================
// Load Sample button (standalone, next to SAMPLE knob)
// ============================================================================

struct theKICKLoadButton : OpaqueWidget {
    theKICK* module = nullptr;
    float scrollPos = 0.f;

    theKICKLoadButton() {
        box.size = Vec(28, 14);
    }

    void step() override {
        if (module && module->hasSample && !module->samplePath.empty()) {
            scrollPos += 0.3f;
        } else {
            scrollPos = 0.f;
        }
        OpaqueWidget::step();
    }

    void drawTextWithOutline(NVGcontext* vg, float x, float y, const char* text, NVGcolor color) {
        float off = 0.6f;
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgText(vg, x - off, y, text, NULL);
        nvgText(vg, x + off, y, text, NULL);
        nvgText(vg, x, y - off, text, NULL);
        nvgText(vg, x, y + off, text, NULL);
        nvgFillColor(vg, color);
        nvgText(vg, x, y, text, NULL);
    }

    void draw(const DrawArgs& args) override {
        bool loaded = module && module->hasSample;

        // Button background (dark when loaded for white outline visibility)
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.f);
        nvgFillColor(args.vg, loaded ? nvgRGB(50, 50, 50) : nvgRGB(70, 70, 70));
        nvgFill(args.vg);
        nvgStrokeColor(args.vg, loaded ? nvgRGB(255, 200, 0) : nvgRGB(120, 120, 120));
        nvgStrokeWidth(args.vg, 1.f);
        nvgStroke(args.vg);

        nvgFontSize(args.vg, 8.f);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        float cy = box.size.y / 2.f;

        if (!loaded) {
            // "LOAD" centered
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(args.vg, nvgRGB(180, 180, 180));
            nvgText(args.vg, box.size.x / 2.f, cy, "LOAD", NULL);
        } else {
            // Scrolling filename, white text, no outline, no extension
            std::string filename = module->samplePath.empty() ? "Sample" : system::getFilename(module->samplePath);
            // Strip extension
            size_t dotPos = filename.rfind('.');
            if (dotPos != std::string::npos) filename = filename.substr(0, dotPos);

            // Measure text width
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            float bounds[4];
            nvgTextBounds(args.vg, 0, 0, filename.c_str(), NULL, bounds);
            float textW = bounds[2] - bounds[0];
            float innerW = box.size.x - 4.f;

            nvgSave(args.vg);
            nvgScissor(args.vg, 1, 0, box.size.x - 2, box.size.y);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));

            if (textW <= innerW) {
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(args.vg, box.size.x / 2.f, cy, filename.c_str(), NULL);
            } else {
                float gap = 25.f;
                float totalW = textW + gap;
                float offset = std::fmod(scrollPos, totalW);

                nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                for (int rep = 0; rep < 2; rep++) {
                    float x = 2.f - offset + rep * totalW;
                    nvgText(args.vg, x, cy, filename.c_str(), NULL);
                }
            }

            nvgRestore(args.vg);
        }
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (module) {
                module->loadSampleFromFile();
            }
            e.consume(this);
            return;
        }
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (module && module->hasSample) {
                ui::Menu* menu = createMenu();
                std::string clearLabel = "Clear Sample";
                if (!module->samplePath.empty()) {
                    clearLabel = "Clear: " + system::getFilename(module->samplePath);
                }
                menu->addChild(createMenuItem(clearLabel, "", [=]() {
                    module->clearSample();
                }));
            }
            e.consume(this);
            return;
        }
        OpaqueWidget::onButton(e);
    }
};

// ============================================================================
// Dynamic mode label for FOLD knob (changes text based on waveshaper mode)
// ============================================================================

struct theKICKDynamicModeLabel : TransparentWidget {
    theKICK* module = nullptr;
    float fontSize;
    bool bold;

    theKICKDynamicModeLabel(Vec pos, Vec size, float fontSize = 8.f, bool bold = true) {
        box.pos = pos;
        box.size = size;
        this->fontSize = fontSize;
        this->bold = bold;
    }

    void draw(const DrawArgs &args) override {
        const char* modeNames[] = {"SATURATE", "FOLD", "ASYMMETRIC", "DESTROY"};
        // Colors matching LED: amber, rose, sage, periwinkle
        const NVGcolor modeColors[] = {
            nvgRGB(227, 187, 10),   // vivid amber
            nvgRGB(227, 21, 26),    // vivid rose
            nvgRGB(21, 209, 33),    // vivid green
            nvgRGB(26, 33, 227),    // vivid blue
        };
        int mode = 0;
        if (module) {
            mode = clamp(module->modeValue, 0, 3);
        }
        std::string text = modeNames[mode];
        NVGcolor color = modeColors[mode];

        nvgFontSize(args.vg, fontSize);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        float cx = box.size.x / 2.f;
        float cy = box.size.y / 2.f;

        // White outline: draw text offset in 4 directions
        NVGcolor outline = nvgRGB(255, 255, 255);
        nvgFillColor(args.vg, outline);
        float off = 0.8f;
        nvgText(args.vg, cx - off, cy, text.c_str(), NULL);
        nvgText(args.vg, cx + off, cy, text.c_str(), NULL);
        nvgText(args.vg, cx, cy - off, text.c_str(), NULL);
        nvgText(args.vg, cx, cy + off, text.c_str(), NULL);

        // Colored fill on top
        nvgFillColor(args.vg, color);
        nvgText(args.vg, cx, cy, text.c_str(), NULL);
    }
};

// ============================================================================
// Widget
// ============================================================================

struct theKICKWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    madzine::widgets::StandardBlackKnob* pitchKnob = nullptr;
    madzine::widgets::StandardBlackKnob* sweepKnob = nullptr;
    madzine::widgets::StandardBlackKnob* bendKnob = nullptr;
    madzine::widgets::StandardBlackKnob* decayKnob = nullptr;
    madzine::widgets::WhiteKnob* foldKnob = nullptr;
    madzine::widgets::WhiteKnob* sampleKnob = nullptr;
    madzine::widgets::WhiteKnob* fbKnob = nullptr;
    madzine::widgets::WhiteKnob* toneKnob = nullptr;

    theKICKWidget(theKICK* module) {
        setModule(module);
        panelThemeHelper.init(this, "8HP", module ? &module->panelContrast : nullptr);

        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        // ================================================================
        // Layout constants — Two-column layout
        // Left column: Synthesis (black knobs) — PITCH, SWEEP, BEND, DECAY
        // Right column: Timbre (white knobs) — FOLD, SAMPLE, FB, TONE
        // ================================================================
        float colL = 32.f;    // Left column center X
        float colR = 90.f;    // Right column center X

        // Vertical positions: 4 rows (adjusted spacing)
        // Row 1 & Row 3/4 shifted toward Row 2 by 2px each
        float row1Y = 60.f;   // Row 1 knob center (was 58, +2)
        float row2Y = 135.f;  // Row 2 knob center (unchanged)
        float row3Y = 210.f;  // Row 3 knob center (was 212, -2)
        float row4Y = 285.f;  // Row 4 knob center (was 289, -4 toward row3)

        float cvOffset = 28.f;   // Knob center to CV port center (15+12+1)
        float labelOffset = 30.f; // 30px knob label offset (per vcv-layout rule 3)

        // Symmetric offset for FOLD sub-elements (CV left, MODE right)
        float foldSubOffset = 17.f;

        // Output area
        float whiteBoxY = 330.f;
        float ioY = 356.f;    // I/O port center in white area

        // Label box dimensions
        float labelW = 60.f;
        float labelH = 14.f;

        // ================================================================
        // Layer 1: Background elements (bottom layer)
        // ================================================================

        // White output area — must start at Y=330
        addChild(new theKICKWhiteBox(Vec(0, whiteBoxY), Vec(box.size.x, box.size.y - whiteBoxY)));

        // ================================================================
        // Layer 2: Title (above background)
        // ================================================================
        addChild(new theKICKTextLabel(Vec(0, 1), Vec(box.size.x, 20), "theKICK", 14.f, nvgRGB(255, 200, 0), true));
        addChild(new theKICKTextLabel(Vec(0, 14), Vec(box.size.x, 16), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        // ================================================================
        // Layer 3: Knobs, ports, buttons (interactive elements)
        // ================================================================

        // --- Left column: Synthesis section (StandardBlackKnob 30px) ---

        // Row 1: PITCH knob + CV
        pitchKnob = createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(colL, row1Y), module, theKICK::PITCH_PARAM);
        addParam(pitchKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colL, row1Y + cvOffset), module, theKICK::PITCH_CV_INPUT));

        // Row 2: SWEEP knob + CV
        sweepKnob = createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(colL, row2Y), module, theKICK::SWEEP_PARAM);
        addParam(sweepKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colL, row2Y + cvOffset), module, theKICK::SWEEP_CV_INPUT));

        // Row 3: BEND knob + CV
        bendKnob = createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(colL, row3Y), module, theKICK::BEND_PARAM);
        addParam(bendKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colL, row3Y + cvOffset), module, theKICK::BEND_CV_INPUT));

        // Row 4: DECAY knob + CV
        decayKnob = createParamCentered<madzine::widgets::StandardBlackKnob>(Vec(colL, row4Y), module, theKICK::DECAY_PARAM);
        addParam(decayKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colL, row4Y + cvOffset), module, theKICK::DECAY_CV_INPUT));

        // --- Right column: Timbre section (WhiteKnob 30px) ---

        // Row 1: FOLD knob + CV (left-below) + MODE (right-below, symmetric)
        foldKnob = createParamCentered<madzine::widgets::WhiteKnob>(Vec(colR, row1Y), module, theKICK::FOLD_PARAM);
        addParam(foldKnob);
        // FOLD CV input: symmetric left-below knob
        addInput(createInputCentered<PJ301MPort>(Vec(colR - foldSubOffset, row1Y + cvOffset), module, theKICK::FOLD_CV_INPUT));
        // MODE LED + Button + overlay: symmetric right-below knob
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(colR + foldSubOffset, row1Y + cvOffset), module, theKICK::MODE_LIGHT_RED));
        addParam(createParamCentered<VCVButton>(Vec(colR + foldSubOffset, row1Y + cvOffset), module, theKICK::MODE_PARAM));
        {
            auto* overlay = new theKICKModeOverlay();
            overlay->box.pos = Vec(colR + foldSubOffset - 8.f, row1Y + cvOffset - 8.f);
            overlay->module = module;
            addChild(overlay);
        }

        // Row 2: SAMPLE knob + LOAD button (above) + SAMPLE CV (below)
        sampleKnob = createParamCentered<madzine::widgets::WhiteKnob>(Vec(colR, row2Y), module, theKICK::SAMPLE_PARAM);
        addParam(sampleKnob);
        // LOAD button at original label position (above knob)
        {
            auto* loadBtn = new theKICKLoadButton();
            loadBtn->box.pos = Vec(colR - 14.f, row2Y - labelOffset - 1.f);
            loadBtn->module = module;
            addChild(loadBtn);
        }
        // SAMPLE CV input (below knob)
        addInput(createInputCentered<PJ301MPort>(Vec(colR, row2Y + cvOffset), module, theKICK::SAMPLE_CV_INPUT));

        // Row 3: FB knob + CV
        fbKnob = createParamCentered<madzine::widgets::WhiteKnob>(Vec(colR, row3Y), module, theKICK::FB_PARAM);
        addParam(fbKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colR, row3Y + cvOffset), module, theKICK::FB_CV_INPUT));

        // Row 4: TONE knob + CV
        toneKnob = createParamCentered<madzine::widgets::WhiteKnob>(Vec(colR, row4Y), module, theKICK::TONE_PARAM);
        addParam(toneKnob);
        addInput(createInputCentered<PJ301MPort>(Vec(colR, row4Y + cvOffset), module, theKICK::TONE_CV_INPUT));

        // --- I/O in white area ---

        // TRIGGER input (left column)
        addInput(createInputCentered<PJ301MPort>(Vec(colL, ioY), module, theKICK::TRIGGER_INPUT));

        // OUT output (right column)
        addOutput(createOutputCentered<PJ301MPort>(Vec(colR, ioY), module, theKICK::OUT_OUTPUT));

        // ================================================================
        // Layer 4: Labels (top layer — added last so never obscured)
        // ================================================================

        // Left column labels (white, 20.f bold, Y < 330)
        addChild(new theKICKTextLabel(Vec(colL - labelW / 2.f, row1Y - labelOffset), Vec(labelW, labelH), "PITCH", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new theKICKTextLabel(Vec(colL - labelW / 2.f, row2Y - labelOffset), Vec(labelW, labelH), "SWEEP", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new theKICKTextLabel(Vec(colL - labelW / 2.f, row3Y - labelOffset), Vec(labelW, labelH), "BEND", 10.f, nvgRGB(255, 255, 255), true));
        addChild(new theKICKTextLabel(Vec(colL - labelW / 2.f, row4Y - labelOffset), Vec(labelW, labelH), "DECAY", 10.f, nvgRGB(255, 255, 255), true));

        // Right column labels (white, 20.f bold, Y < 330)
        // Row 1: Dynamic mode label (SATURATE/FOLD/ASYMMETRIC/DESTROY)
        {
            auto* modeLabel = new theKICKDynamicModeLabel(Vec(colR - labelW / 2.f, row1Y - labelOffset), Vec(labelW, labelH), 10.f, true);
            modeLabel->module = module;
            addChild(modeLabel);
        }
        // Row 2: No SAMPLE label (replaced by LOAD button above knob)
        // Row 3: FEEDBACK
        addChild(new theKICKTextLabel(Vec(colR - labelW / 2.f, row3Y - labelOffset), Vec(labelW, labelH), "FEEDBACK", 10.f, nvgRGB(255, 255, 255), true));
        // Row 4: TONE
        addChild(new theKICKTextLabel(Vec(colR - labelW / 2.f, row4Y - labelOffset), Vec(labelW, labelH), "TONE", 10.f, nvgRGB(255, 255, 255), true));

        // I/O area labels (Y >= 330, on white background)
        addChild(new theKICKTextLabel(Vec(colL - labelW / 2.f, ioY - 24.f), Vec(labelW, labelH), "TRIG", 10.f, nvgRGB(0, 0, 0), true));
        addChild(new theKICKTextLabel(Vec(colR - labelW / 2.f, ioY - 24.f), Vec(labelW, labelH), "OUT", 10.f, nvgRGB(255, 133, 133), true));
    }

    void step() override {
        theKICK* module = dynamic_cast<theKICK*>(this->module);
        if (module) {
            panelThemeHelper.step(module);

            // CV modulation ring display
            auto updateKnobMod = [](auto* knob, bool connected, float mod) {
                if (!knob) return;
                knob->setModulationEnabled(connected);
                if (connected) knob->setModulation(mod);
            };

            updateKnobMod(pitchKnob, module->inputs[theKICK::PITCH_CV_INPUT].isConnected(), module->pitchCvMod);
            updateKnobMod(sweepKnob, module->inputs[theKICK::SWEEP_CV_INPUT].isConnected(), module->sweepCvMod);
            updateKnobMod(bendKnob, module->inputs[theKICK::BEND_CV_INPUT].isConnected(), module->bendCvMod);
            updateKnobMod(decayKnob, module->inputs[theKICK::DECAY_CV_INPUT].isConnected(), module->decayCvMod);
            updateKnobMod(foldKnob, module->inputs[theKICK::FOLD_CV_INPUT].isConnected(), module->foldCvMod);
            updateKnobMod(sampleKnob, module->inputs[theKICK::SAMPLE_CV_INPUT].isConnected(), module->sampleCvMod);
            updateKnobMod(fbKnob, module->inputs[theKICK::FB_CV_INPUT].isConnected(), module->fbCvMod);
            updateKnobMod(toneKnob, module->inputs[theKICK::TONE_CV_INPUT].isConnected(), module->toneCvMod);
        }
        ModuleWidget::step();
    }

    void appendContextMenu(ui::Menu* menu) override {
        theKICK* module = dynamic_cast<theKICK*>(this->module);
        if (!module) return;

        addPanelThemeMenu(menu, module);

        menu->addChild(new MenuSeparator());
        menu->addChild(createMenuLabel("Oversampling"));

        menu->addChild(createMenuItem("2x Oversample", CHECKMARK(module->oversampleRate == 2), [=]() {
            module->oversampleRate = (module->oversampleRate == 2) ? 1 : 2;
            module->setupOversamplingFilters();
        }));
    }
};

Model* modeltheKICK = createModel<theKICK, theKICKWidget>("theKICK");
