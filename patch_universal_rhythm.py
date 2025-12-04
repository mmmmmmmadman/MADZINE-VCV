#!/usr/bin/env python3
"""
Patch script to add external audio inputs and stereo spread to UniversalRhythm.cpp
"""

import re

def patch_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # 1. Add VCA Envelope struct and member variables before UniversalRhythm() constructor
    vca_code = '''
    // External audio VCA envelopes (per voice)
    struct VCAEnvelope {
        float amplitude = 0.0f;
        float decayRate = 0.0f;

        void trigger(float decayTimeMs, float sampleRate) {
            amplitude = 1.0f;
            // Convert decay time to decay rate per sample
            decayRate = 1.0f / (decayTimeMs * 0.001f * sampleRate);
        }

        float process() {
            if (amplitude > 0.0f) {
                float current = amplitude;
                amplitude -= decayRate;
                if (amplitude < 0.0f) amplitude = 0.0f;
                return current;
            }
            return 0.0f;
        }

        bool isActive() const {
            return amplitude > 0.001f;
        }
    };

    VCAEnvelope externalVCA[8];  // One VCA per voice for external audio gating
    float currentSpread[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Current spread value per role

'''

    # Insert VCA code before UniversalRhythm() constructor
    content = re.sub(
        r'(    float lastSwing = 0\.5f;\n)\n(    UniversalRhythm\(\) \{)',
        r'\1' + vca_code + r'\2',
        content
    )

    # 2. Add SPREAD parameter configuration in constructor (after DECAY config)
    spread_config = '''
        // SPREAD parameters (per-role)
        for (int r = 0; r < 4; r++) {
            configParam(TIMELINE_SPREAD_PARAM + r, 0.0f, 1.0f, 0.0f,
                       std::string(roleNames[r]) + " Spread", "%", 0.0f, 100.0f);
        }

'''

    # Insert after the for loop that configures per-role parameters
    content = re.sub(
        r'(            configParam\(TIMELINE_DECAY_PARAM \+ baseParam, 0\.2f, 2\.0f, 1\.0f,\n' +
        r'                        std::string\(roleNames\[r\]\) \+ " Decay", "x"\);\n' +
        r'        \}\n)\n',
        r'\1' + spread_config,
        content
    )

    # 3. Add INPUT configuration for external audio inputs (after per-role CV inputs)
    input_config = '''
        // External audio inputs (2 per role)
        for (int r = 0; r < 4; r++) {
            configInput(TIMELINE_AUDIO_INPUT_1 + r * 2, std::string(roleNames[r]) + " Audio Input 1");
            configInput(TIMELINE_AUDIO_INPUT_2 + r * 2, std::string(roleNames[r]) + " Audio Input 2");
        }

'''

    # Insert after the CV input configuration loop
    content = re.sub(
        r'(        for \(int r = 0; r < 4; r\+\+\) \{\n' +
        r'            configInput\(TIMELINE_STYLE_CV_INPUT \+ r \* 4.*?\n' +
        r'            configInput\(TIMELINE_DENSITY_CV_INPUT \+ r \* 4.*?\n' +
        r'            configInput\(TIMELINE_FREQ_CV_INPUT \+ r \* 4.*?\n' +
        r'            configInput\(TIMELINE_DECAY_CV_INPUT \+ r \* 4.*?\n' +
        r'        \}\n)\n',
        r'\1' + input_config,
        content,
        flags=re.DOTALL
    )

    # 4. Update output configuration (MIX_L_OUTPUT + MIX_R_OUTPUT instead of MIX_OUTPUT)
    content = re.sub(
        r'        // Outputs\n        configOutput\(MIX_OUTPUT, "Mix"\);',
        r'        // Outputs\n        configOutput(MIX_L_OUTPUT, "Mix L");\n        configOutput(MIX_R_OUTPUT, "Mix R");',
        content
    )

    # 5. Modify process() function - replace the audio processing section
    # Find and replace the audio output section
    old_audio_section = r'        // Process audio\n        float mix = 0\.0f;\n' + \
                       r'        for \(int i = 0; i < 8; i\+\+\) \{\n' + \
                       r'            float audio = drumSynth\.processVoice\(i\);\n' + \
                       r'            float scaled = audio \* 5\.0f;\n' + \
                       r'            outputs\[VOICE1_AUDIO_OUTPUT \+ i\]\.setVoltage\(scaled\);\n' + \
                       r'            mix \+= scaled \* 0\.2f;\n' + \
                       r'        \}\n' + \
                       r'        outputs\[MIX_OUTPUT\]\.setVoltage\(std::tanh\(mix\) \* 5\.0f\);'

    new_audio_section = '''        // Process audio with external inputs and stereo spread
        float mixL = 0.0f;
        float mixR = 0.0f;

        for (int r = 0; r < 4; r++) {
            int voiceBase = r * 2;
            float spread = params[TIMELINE_SPREAD_PARAM + r].getValue();  // 0.0 to 1.0
            currentSpread[r] = spread;

            // Voice 1 (Primary) - panned based on spread
            int v1 = voiceBase;
            float synthAudio1 = drumSynth.processVoice(v1) * 5.0f;

            // Process external audio input 1 with VCA envelope
            float extAudio1 = 0.0f;
            if (inputs[TIMELINE_AUDIO_INPUT_1 + r * 2].isConnected()) {
                float externalSignal = inputs[TIMELINE_AUDIO_INPUT_1 + r * 2].getVoltage();
                float vcaGain = externalVCA[v1].process();
                extAudio1 = externalSignal * vcaGain * currentVelocities[v1];
                if (currentAccents[v1]) {
                    extAudio1 *= 1.5f;  // Accent boost
                }
            }

            // Combine internal synth + external audio for voice 1
            float combined1 = synthAudio1 + extAudio1;

            // Pan voice 1: at spread=0, center; at spread=1.0, full left
            float pan1L = 1.0f - (spread * 0.5f);  // 1.0 to 0.5
            float pan1R = 1.0f - (spread * 0.5f);  // 1.0 to 0.5 (but will be attenuated more)
            if (spread > 0.0f) {
                pan1R = 1.0f - spread;  // Goes from 1.0 to 0.0 as spread increases
            }

            outputs[VOICE1_AUDIO_OUTPUT + v1].setVoltage(combined1);
            mixL += combined1 * pan1L * 0.25f;
            mixR += combined1 * pan1R * 0.25f;

            // Voice 2 (Secondary) - panned opposite based on spread
            int v2 = voiceBase + 1;
            float synthAudio2 = drumSynth.processVoice(v2) * 5.0f;

            // Process external audio input 2 with VCA envelope
            float extAudio2 = 0.0f;
            if (inputs[TIMELINE_AUDIO_INPUT_2 + r * 2].isConnected()) {
                float externalSignal = inputs[TIMELINE_AUDIO_INPUT_2 + r * 2].getVoltage();
                float vcaGain = externalVCA[v2].process();
                extAudio2 = externalSignal * vcaGain * currentVelocities[v2];
                if (currentAccents[v2]) {
                    extAudio2 *= 1.5f;  // Accent boost
                }
            }

            // Combine internal synth + external audio for voice 2
            float combined2 = synthAudio2 + extAudio2;

            // Pan voice 2: at spread=0, center; at spread=1.0, full right
            float pan2L = 1.0f - (spread * 0.5f);
            float pan2R = 1.0f - (spread * 0.5f);
            if (spread > 0.0f) {
                pan2L = 1.0f - spread;  // Goes from 1.0 to 0.0 as spread increases
            }

            outputs[VOICE1_AUDIO_OUTPUT + v2].setVoltage(combined2);
            mixL += combined2 * pan2L * 0.25f;
            mixR += combined2 * pan2R * 0.25f;
        }

        outputs[MIX_L_OUTPUT].setVoltage(std::tanh(mixL) * 5.0f);
        outputs[MIX_R_OUTPUT].setVoltage(std::tanh(mixR) * 5.0f);'''

    content = re.sub(old_audio_section, new_audio_section, content, flags=re.DOTALL)

    # 6. Add VCA envelope triggering in the voice trigger section
    # Find the triggerWithArticulation function call locations and add VCA trigger
    # We need to add VCA triggering where voices are triggered in the clock processing loop

    # Add VCA trigger logic in the clock processing section
    # After primary voice trigger
    primary_trigger_old = r'(                        triggerWithArticulation\(voiceBase, vel, accent, args\.sampleRate\);)'
    primary_trigger_new = r'''\1
                        // Trigger VCA for external audio (use decay parameter for envelope length)
                        int baseParam = r * 6;  // Now 6 params per role (includes SPREAD)
                        float decayMult = params[TIMELINE_DECAY_PARAM + baseParam].getValue();
                        if (inputs[TIMELINE_DECAY_CV_INPUT + r * 4].isConnected()) {
                            decayMult += inputs[TIMELINE_DECAY_CV_INPUT + r * 4].getVoltage() * 0.18f;
                            decayMult = clamp(decayMult, 0.2f, 2.0f);
                        }
                        // Base decay of 200ms, scaled by decay parameter
                        float vcaDecayMs = 200.0f * decayMult;
                        externalVCA[voiceBase].trigger(vcaDecayMs, args.sampleRate);'''

    content = re.sub(primary_trigger_old, primary_trigger_new, content)

    # After secondary voice trigger
    secondary_trigger_old = r'(                            triggerWithArticulation\(voiceBase \+ 1, vel, accent, args\.sampleRate\);)'
    secondary_trigger_new = r'''\1
                        // Trigger VCA for external audio (use decay parameter for envelope length)
                        float vcaDecayMs2 = 200.0f * decayMult;
                        externalVCA[voiceBase + 1].trigger(vcaDecayMs2, args.sampleRate);'''

    content = re.sub(secondary_trigger_old, secondary_trigger_new, content)

    # Fix the baseParam calculation issue - it was r * 5, now should account for new structure
    # Actually, looking at the enum, SPREAD params are at the end, so baseParam calculation stays r * 5
    # But we access SPREAD as TIMELINE_SPREAD_PARAM + r

    # Correct the baseParam comment and usage
    content = re.sub(
        r'            int baseParam = r \* 5;  // STYLE, DENSITY, LENGTH, FREQ, DECAY per role',
        r'            int baseParam = r * 6;  // STYLE, DENSITY, LENGTH, FREQ, DECAY, SPREAD per role',
        content
    )

    # Wait, looking at the enum structure more carefully, SPREAD params are added AT THE END
    # So the for-loop indexing needs to stay as r*5 for the first 5 params
    # Let me revert that change
    content = re.sub(
        r'            int baseParam = r \* 6;  // STYLE, DENSITY, LENGTH, FREQ, DECAY, SPREAD per role',
        r'            int baseParam = r * 5;  // STYLE, DENSITY, LENGTH, FREQ, DECAY per role',
        content
    )

    # Also fix the VCA trigger code to use the correct baseParam
    content = re.sub(
        r'                        int baseParam = r \* 6;  // Now 6 params per role \(includes SPREAD\)',
        r'                        int baseParam = r * 5;  // 5 params per role (SPREAD is separate)',
        content
    )

    return content

if __name__ == '__main__':
    filepath = '/Users/madzine/Documents/VCV-Dev/MADZINE/src/UniversalRhythm.cpp'
    patched_content = patch_file(filepath)

    with open(filepath, 'w') as f:
        f.write(patched_content)

    print("UniversalRhythm.cpp patched successfully!")
