#include "plugin.hpp"

struct Pyramid : Module {
    enum ParamId {
        X_PARAM,
        Y_PARAM,
        Z_PARAM,
        LEVEL_PARAM,
        FILTER_PARAM,
        SEND_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AUDIO_INPUT,
        X_CV_INPUT,
        Y_CV_INPUT,
        Z_CV_INPUT,
        FILTER_CV_INPUT,
        RETURN_L_INPUT,
        RETURN_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        FL_UPPER_OUTPUT,
        FR_UPPER_OUTPUT,
        BL_UPPER_OUTPUT,
        BR_UPPER_OUTPUT,
        FL_LOWER_OUTPUT,
        FR_LOWER_OUTPUT,
        BL_LOWER_OUTPUT,
        BR_LOWER_OUTPUT,
        SEND_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    struct SpeakerPosition {
        float x, y, z;
    };

    SpeakerPosition speakers[8] = {
        {-1.0f, -1.0f,  1.0f},  // FL Upper (was BL Upper)
        { 1.0f, -1.0f,  1.0f},  // FR Upper (was BR Upper)
        {-1.0f, -1.0f, -1.0f},  // BL Upper (was BL Lower)
        { 1.0f, -1.0f, -1.0f},  // BR Upper (was BR Lower)
        {-1.0f,  1.0f,  1.0f},  // FL Lower (was FL Upper)
        { 1.0f,  1.0f,  1.0f},  // FR Lower (was FR Upper)
        {-1.0f,  1.0f, -1.0f},  // BL Lower (was FL Lower)
        { 1.0f,  1.0f, -1.0f}   // BR Lower (was FR Lower)
    };

    dsp::TBiquadFilter<> filter1;
    dsp::TBiquadFilter<> filter2;

    bool sendPreLevel = false;
    int lastFilterMode = 0;
    float lastFilterValue = 0.f;
    float smoothedFilter = 0.f;

    Pyramid() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(X_PARAM, -1.f, 1.f, 0.f, "X Position", "", 0.f, 1.f);
        configParam(Y_PARAM, -1.f, 1.f, 0.f, "Y Position", "", 0.f, 1.f);
        configParam(Z_PARAM, -1.f, 1.f, 0.f, "Z Position", "", 0.f, 1.f);
        configParam(LEVEL_PARAM, 0.f, 1.f, 0.7f, "Level", "%", 0.f, 100.f);
        configParam(FILTER_PARAM, -1.f, 1.f, 0.f, "Filter");
        configParam(SEND_PARAM, 0.f, 1.f, 0.f, "Send", "%", 0.f, 100.f);
        
        configInput(AUDIO_INPUT, "Audio");
        configInput(X_CV_INPUT, "X CV");
        configInput(Y_CV_INPUT, "Y CV");
        configInput(Z_CV_INPUT, "Z CV");
        configInput(FILTER_CV_INPUT, "Filter CV");
        configInput(RETURN_L_INPUT, "Return L");
        configInput(RETURN_R_INPUT, "Return R");
        
        configOutput(FL_UPPER_OUTPUT, "1");
        configOutput(FR_UPPER_OUTPUT, "2");
        configOutput(BL_UPPER_OUTPUT, "3");
        configOutput(BR_UPPER_OUTPUT, "4");
        configOutput(FL_LOWER_OUTPUT, "5");
        configOutput(FR_LOWER_OUTPUT, "6");
        configOutput(BL_LOWER_OUTPUT, "7");
        configOutput(BR_LOWER_OUTPUT, "8");
        configOutput(SEND_OUTPUT, "Send");
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
        float audioIn = inputs[AUDIO_INPUT].getVoltage();
        
        float x = params[X_PARAM].getValue();
        float y = params[Y_PARAM].getValue();
        float z = params[Z_PARAM].getValue();
        float level = params[LEVEL_PARAM].getValue();
        float filter = params[FILTER_PARAM].getValue();
        float send = params[SEND_PARAM].getValue();
        
        if (inputs[X_CV_INPUT].isConnected()) {
            x += inputs[X_CV_INPUT].getVoltage() * 0.2f;
            x = clamp(x, -1.f, 1.f);
        }
        if (inputs[Y_CV_INPUT].isConnected()) {
            y += inputs[Y_CV_INPUT].getVoltage() * 0.2f;
            y = clamp(y, -1.f, 1.f);
        }
        if (inputs[Z_CV_INPUT].isConnected()) {
            z += inputs[Z_CV_INPUT].getVoltage() * 0.2f;
            z = clamp(z, -1.f, 1.f);
        }
        
        if (inputs[FILTER_CV_INPUT].isConnected()) {
            filter += inputs[FILTER_CV_INPUT].getVoltage() * 0.2f;
            filter = clamp(filter, -1.f, 1.f);
        }

        float filterSmooth = 0.002f;
        smoothedFilter = smoothedFilter * (1.0f - filterSmooth) + filter * filterSmooth;

        audioIn *= level;
        
        float sendOut;
        if (sendPreLevel) {
            sendOut = inputs[AUDIO_INPUT].getVoltage() * send;
        } else {
            sendOut = audioIn * send;
        }
        
        if (filter < -0.001f) {
            if (lastFilterMode != -1) {
                filter1.reset();
                filter2.reset();
                lastFilterMode = -1;
            }
            float freq = rescale(filter, -1.f, 0.f, 20.f, 22000.f);
            filter1.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            filter2.setParameters(dsp::TBiquadFilter<>::LOWPASS, freq / args.sampleRate, 0.707f, 1.f);
            audioIn = filter2.process(filter1.process(audioIn));
        } else if (filter > 0.001f) {
            if (lastFilterMode != 1) {
                filter1.reset();
                filter2.reset();
                lastFilterMode = 1;
            }
            float freq = rescale(filter, 0.f, 1.f, 10.f, 8000.f);
            filter1.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            filter2.setParameters(dsp::TBiquadFilter<>::HIGHPASS, freq / args.sampleRate, 0.707f, 1.f);
            audioIn = filter2.process(filter1.process(audioIn));
        } else {
            lastFilterMode = 0;
        }
        
        outputs[SEND_OUTPUT].setVoltage(sendOut);
        
        float returnL = inputs[RETURN_L_INPUT].getVoltage();
        float returnR = inputs[RETURN_R_INPUT].getVoltage();
        
        float gains[8];
        calculateVBAP(x, y, z, gains);
        
        for (int i = 0; i < 8; i++) {
            float outputVoltage = audioIn * gains[i];
            
            if (i % 2 == 0) {
                outputVoltage += returnL * gains[i];
            } else {
                outputVoltage += returnR * gains[i];
            }
            
            outputs[FL_UPPER_OUTPUT + i].setVoltage(outputVoltage);
        }
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

struct StandardBlackKnob : ParamWidget {
    bool isDragging = false;
    
    StandardBlackKnob() {
        box.size = Vec(26, 26);
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
        
        float sensitivity = 0.01f;
        float deltaValue = -e.mouseDelta.y * sensitivity;
        pq->setValue(pq->getValue() + deltaValue);
        e.consume(this);
    }
    
    void onDoubleClick(const event::DoubleClick& e) override {
        ParamQuantity* pq = getParamQuantity();
        if (!pq) return;
        
        pq->reset();
        e.consume(this);
    }
};

struct PyramidGraphicWidget : Widget {
    PyramidGraphicWidget(Vec pos, Vec size) {
        box.pos = pos;
        box.size = size;
    }
    
    void draw(const DrawArgs &args) override {
        float centerX = box.size.x / 2.0f;
        float centerY = box.size.y / 2.0f;
        
        nvgSave(args.vg);
        nvgTranslate(args.vg, centerX, centerY);
        nvgRotate(args.vg, 9.0f * M_PI / 180.0f);
        nvgTranslate(args.vg, -centerX, -centerY);
        
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 200, 0));
        
        float baseSize = box.size.x * 0.85f;
        float height = box.size.y * 0.9f;
        
        float baseLeft = centerX - baseSize/2;
        float baseRight = centerX + baseSize/2;
        float baseFront = centerY + height/3;
        float baseBack = centerY + height/3 - baseSize * 0.25f;
        float apex = centerY - height/2;
        
        // Base square (visible edges only)
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, baseLeft, baseFront);
        nvgLineTo(args.vg, baseRight, baseFront);
        nvgLineTo(args.vg, baseRight, baseBack);
        nvgStroke(args.vg);
        
        // Front face
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, baseLeft, baseFront);
        nvgLineTo(args.vg, centerX, apex);
        nvgLineTo(args.vg, baseRight, baseFront);
        nvgStroke(args.vg);
        
        // Right face
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, baseRight, baseFront);
        nvgLineTo(args.vg, centerX, apex);
        nvgLineTo(args.vg, baseRight, baseBack);
        nvgStroke(args.vg);
        
        // Brick pattern
        nvgStrokeWidth(args.vg, 0.8f);
        
        // Horizontal lines (layers)
        int layers = 5;
        for (int i = 1; i < layers; i++) {
            float ratio = (float)i / layers;
            float layerY = baseFront - (baseFront - apex) * ratio;
            
            // Front face horizontal lines
            float leftX = baseLeft + (centerX - baseLeft) * ratio;
            float rightX = baseRight - (baseRight - centerX) * ratio;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, leftX, layerY);
            nvgLineTo(args.vg, rightX, layerY);
            nvgStroke(args.vg);
            
            // Right face horizontal lines
            float backX = baseRight - (baseRight - centerX) * ratio;
            float backY = baseBack - (baseBack - apex) * ratio;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, rightX, layerY);
            nvgLineTo(args.vg, backX, backY);
            nvgStroke(args.vg);
        }
        
        // Vertical lines (brick divisions)
        for (int layer = 0; layer < layers - 1; layer++) {
            float ratio1 = (float)layer / layers;
            float ratio2 = (float)(layer + 1) / layers;
            float y1 = baseFront - (baseFront - apex) * ratio1;
            float y2 = baseFront - (baseFront - apex) * ratio2;
            
            // Front face vertical divisions
            float leftX1 = baseLeft + (centerX - baseLeft) * ratio1;
            float rightX1 = baseRight - (baseRight - centerX) * ratio1;
            float leftX2 = baseLeft + (centerX - baseLeft) * ratio2;
            float rightX2 = baseRight - (baseRight - centerX) * ratio2;
            
            float layerWidth = rightX1 - leftX1;
            int bricksInLayer = std::max(2, (int)(4 - layer * 0.6f));
            
            // Offset every other layer by half brick width
            float offset = (layer % 2) * 0.5f;
            
            for (int brick = 1; brick < bricksInLayer; brick++) {
                float brickPosition = fmod(brick + offset, bricksInLayer);
                if (brickPosition > 0 && brickPosition < bricksInLayer) {
                    float ratio = brickPosition / bricksInLayer;
                    float x1 = leftX1 + layerWidth * ratio;
                    float x2 = leftX2 + (rightX2 - leftX2) * ratio;
                    
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, x1, y1);
                    nvgLineTo(args.vg, x2, y2);
                    nvgStroke(args.vg);
                }
            }
            
            // Right face vertical divisions
            int rightBricks = std::max(1, 3 - layer);
            float rightOffset = (layer % 2 == 1) ? 0.5f : 0.0f;
            
            for (int brick = 1; brick < rightBricks; brick++) {
                float brickRatio = (brick + rightOffset) / rightBricks;
                if (brickRatio > 0 && brickRatio < 1) {
                    float frontRatio = ratio1 + brickRatio * (ratio2 - ratio1);
                    float frontX = baseRight - (baseRight - centerX) * frontRatio;
                    float frontY = baseFront - (baseFront - apex) * frontRatio;
                    float backX = baseRight - (baseRight - centerX) * frontRatio;
                    float backY = baseBack - (baseBack - apex) * frontRatio;
                    
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, frontX, frontY);
                    nvgLineTo(args.vg, backX, backY);
                    nvgStroke(args.vg);
                }
            }
        }
        
        nvgRestore(args.vg);
    }
};

struct Pyramid3DDisplay : LedDisplay {
    Pyramid* module;
    ModuleWidget* moduleWidget;
    
    Pyramid3DDisplay() {
        box.size = Vec(120, 120);
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
            Vec pos = project3D(module->speakers[i].x * 0.6f, 
                              module->speakers[i].y * 0.6f, 
                              module->speakers[i].z * 0.6f);
            
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
            corners[i] = project3D(module->speakers[i].x * 0.6f, 
                                 module->speakers[i].y * 0.6f, 
                                 module->speakers[i].z * 0.6f);
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
        
        float x = module->params[Pyramid::X_PARAM].getValue();
        float y = module->params[Pyramid::Y_PARAM].getValue();
        float z = module->params[Pyramid::Z_PARAM].getValue();
        
        if (module->inputs[Pyramid::X_CV_INPUT].isConnected()) {
            x += module->inputs[Pyramid::X_CV_INPUT].getVoltage() * 0.2f;
            x = clamp(x, -1.f, 1.f);
        }
        if (module->inputs[Pyramid::Y_CV_INPUT].isConnected()) {
            y += module->inputs[Pyramid::Y_CV_INPUT].getVoltage() * 0.2f;
            y = clamp(y, -1.f, 1.f);
        }
        if (module->inputs[Pyramid::Z_CV_INPUT].isConnected()) {
            z += module->inputs[Pyramid::Z_CV_INPUT].getVoltage() * 0.2f;
            z = clamp(z, -1.f, 1.f);
        }
        
        Vec pos = project3D(x * 0.6f, y * 0.6f, z * 0.6f);
        
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, pos.x, pos.y, 4.0f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 0));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStrokeColor(args.vg, nvgRGB(255, 200, 0));
        nvgStroke(args.vg);
    }
    
    void drawBackground(const DrawArgs& args) {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        nvgStrokeWidth(args.vg, 1.0f);
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

struct PyramidWidget : ModuleWidget {
    PyramidWidget(Pyramid* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/EuclideanRhythm.svg")));
        
        box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addChild(new TechnoEnhancedTextLabel(Vec(0, 1), Vec(box.size.x, 20), "Pyramid", 12.f, nvgRGB(255, 200, 0), true));
        addChild(new TechnoEnhancedTextLabel(Vec(0, 13), Vec(box.size.x, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0), false));

        addChild(new PyramidGraphicWidget(Vec(75, 50), Vec(38, 38)));

        addChild(new TechnoEnhancedTextLabel(Vec(17-15, 47), Vec(30, 10), "LEVEL", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(17, 70), module, Pyramid::LEVEL_PARAM));
        addChild(new TechnoEnhancedTextLabel(Vec(44-15, 47), Vec(30, 10), "INPUT", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(44, 70), module, Pyramid::AUDIO_INPUT));

        Pyramid3DDisplay* display3D = new Pyramid3DDisplay();
        display3D->box.pos = Vec(0, 90);
        display3D->module = module;
        display3D->moduleWidget = this;
        addChild(display3D);

        addChild(new TechnoEnhancedTextLabel(Vec(7, 220), Vec(50, 10), "X", 32.f, nvgRGB(160, 160, 160), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(17, 240), module, Pyramid::X_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(7, 255), Vec(50, 10), "Y", 32.f, nvgRGB(160, 160, 160), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(17, 275), module, Pyramid::Y_PARAM));

        addChild(new TechnoEnhancedTextLabel(Vec(7, 290), Vec(50, 10), "Z", 32.f, nvgRGB(160, 160, 160), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(17, 310), module, Pyramid::Z_PARAM));
        
        addChild(new TechnoEnhancedTextLabel(Vec(75-15, 220), Vec(30, 10), "SEND", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(75, 242), module, Pyramid::SEND_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(75, 270), module, Pyramid::SEND_OUTPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(102-15, 220), Vec(30, 10), "RTN", 8.f, nvgRGB(255, 255, 255), true));
        addInput(createInputCentered<PJ301MPort>(Vec(102, 242), module, Pyramid::RETURN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(102, 270), module, Pyramid::RETURN_R_INPUT));

        addChild(new TechnoEnhancedTextLabel(Vec(65, 290), Vec(50, 10), "FILTER", 8.f, nvgRGB(255, 255, 255), true));
        addParam(createParamCentered<StandardBlackKnob>(Vec(75, 312), module, Pyramid::FILTER_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(102, 312), module, Pyramid::FILTER_CV_INPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(44, 240), module, Pyramid::X_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(44, 275), module, Pyramid::Y_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(44, 310), module, Pyramid::Z_CV_INPUT));

        addChild(new WhiteBackgroundBox(Vec(0, 330), Vec(box.size.x, 50)));
        
        addOutput(createOutputCentered<PJ301MPort>(Vec(13, 343), module, Pyramid::FL_UPPER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 343), module, Pyramid::FR_UPPER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(75, 343), module, Pyramid::BL_UPPER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(104, 343), module, Pyramid::BR_UPPER_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(13, 368), module, Pyramid::FL_LOWER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 368), module, Pyramid::FR_LOWER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(75, 368), module, Pyramid::BL_LOWER_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(104, 368), module, Pyramid::BR_LOWER_OUTPUT));
    }
    
    void appendContextMenu(Menu* menu) override {
        Pyramid* module = getModule<Pyramid>();
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createBoolPtrMenuItem("Send Pre-Level", "", &module->sendPreLevel));
    }
};

Model* modelPyramid = createModel<Pyramid, PyramidWidget>("Pyramid");