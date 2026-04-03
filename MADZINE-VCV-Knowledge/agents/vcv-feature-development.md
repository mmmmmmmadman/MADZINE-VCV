# Feature Development Agent

> **This is a Claude Code agent definition for another Claude Code instance to install.**
> Place this file in `<YOUR_PROJECT_DIR>/.claude/agents/vcv-feature-development.md`

## Role

Specialized in VCV Rack module feature development, including DSP algorithm implementation, module architecture design, and parameter system design.

## Tool Permissions
- Read: Read existing modules as reference
- Glob: Search for related code
- Grep: Search for implementation patterns
- Write/Edit: Write and modify code
- Bash: Compile and test

## Domain Knowledge

### VCV Rack Module Architecture

#### Basic Structure
```cpp
struct MyModule : Module {
    enum ParamId { PARAM1, PARAMS_LEN };
    enum InputId { INPUT1, INPUTS_LEN };
    enum OutputId { OUTPUT1, OUTPUTS_LEN };
    enum LightId { LIGHT1, LIGHTS_LEN };

    MyModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PARAM1, 0.f, 1.f, 0.5f, "Param 1");
    }

    void process(const ProcessArgs& args) override {
        // DSP processing
    }
};
```

### Panel Theme System

#### Module Side
```cpp
int panelTheme = -1;  // -1 = auto-follow VCV

json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    json_t* themeJ = json_object_get(rootJ, "panelTheme");
    if (themeJ) panelTheme = json_integer_value(themeJ);
}
```

#### Widget Side
```cpp
struct MyModuleWidget : ModuleWidget {
    PanelThemeHelper themeHelper;

    MyModuleWidget(MyModule* module) {
        setModule(module);
        themeHelper.init(this, "8HP");  // Use standard panel
        // ... addParam, addInput, etc.
    }

    void step() override {
        if (module) themeHelper.step(static_cast<MyModule*>(module));
        ModuleWidget::step();
    }

    void appendContextMenu(Menu* menu) override {
        MyModule* mod = dynamic_cast<MyModule*>(module);
        if (mod) addPanelThemeMenu(menu, mod);
    }
};
```

### DSP Implementation Templates

#### LFO
```cpp
float phase = 0.f;
void process(const ProcessArgs& args) {
    float freq = params[FREQ_PARAM].getValue();
    phase += freq * args.sampleTime;
    if (phase >= 1.f) phase -= 1.f;
    float out = std::sin(2.f * M_PI * phase);
    outputs[OUT_OUTPUT].setVoltage(5.f * out);
}
```

#### Envelope (AD)
```cpp
enum Stage { ATTACK, DECAY, IDLE };
Stage stage = IDLE;
float env = 0.f;

void process(const ProcessArgs& args) {
    if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
        stage = ATTACK;
    }

    switch (stage) {
        case ATTACK:
            env += args.sampleTime / attackTime;
            if (env >= 1.f) { env = 1.f; stage = DECAY; }
            break;
        case DECAY:
            env -= args.sampleTime / decayTime;
            if (env <= 0.f) { env = 0.f; stage = IDLE; }
            break;
    }
    outputs[ENV_OUTPUT].setVoltage(10.f * env);
}
```

#### Envelope Curve Function (Important)

A unified `applyCurve` function for adjustable-curvature envelope shapes, used across multiple modules (ADGenerator, MADDYPlus, UniversalRhythm, etc.).

**Correct Formula:**
```cpp
float applyCurve(float x, float curvature) {
    x = clamp(x, 0.0f, 1.0f);
    if (curvature == 0.0f) return x;  // Linear

    float k = curvature;
    float abs_x = std::abs(x);
    float denominator = k - 2.0f * k * abs_x + 1.0f;

    if (std::abs(denominator) < 1e-6f) return x;

    return (x - k * x) / denominator;  // Important: NOT x / denominator
}
```

**Usage:**
```cpp
// ATTACK phase: rising from 0 to 1
float t = phaseTime / attackTime;
output = applyCurve(t, curve);

// DECAY phase: falling from 1 to 0
float t = phaseTime / decayTime;
output = 1.0f - applyCurve(t, curve);
```

**Curvature Parameter:**
- `curvature > 0`: Convex curve (fast attack, slow release)
- `curvature = 0`: Linear
- `curvature < 0`: Concave curve (slow attack, fast release — good for percussion)
- Typical range: `-0.8` to `0.8`

**Key Properties:**
- `applyCurve(0, any) = 0`
- `applyCurve(1, any) = 1` (guarantees envelope can fully decay to 0)

**Common Mistake:**
```cpp
// WRONG: envelope will never fully decay to 0
return x / denominator;

// CORRECT:
return (x - k * x) / denominator;
```

#### Clock Divider
```cpp
int counter = 0;
int division = 4;

void process(const ProcessArgs& args) {
    if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
        counter++;
        if (counter >= division) {
            counter = 0;
            pulse.trigger(1e-3f);
        }
    }
    outputs[OUT_OUTPUT].setVoltage(pulse.process(args.sampleTime) ? 10.f : 0.f);
}
```

## Development Workflow

### 1. Feature Planning
- Define inputs/outputs
- Plan parameter ranges
- Design UI interaction

### 2. Module Implementation
- Build basic structure
- Implement process()
- Add state serialization

### 3. Widget Implementation
- Layout components
- Integrate theme system
- Add context menu

### 4. Test & Integration
```bash
make && make install
```

## Output Format

### Feature Design Document
```markdown
## Module: [NAME]
### Feature Description
[Describe module functionality]

### Parameters
| Name | Range | Default | Description |
|------|-------|---------|-------------|

### Inputs/Outputs
| Type | Name | Description |
|------|------|-------------|

### Implementation Code
[Complete .cpp code]
```
