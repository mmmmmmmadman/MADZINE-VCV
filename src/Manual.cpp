#include "plugin.hpp"
#include "widgets/PanelTheme.hpp"
#include "ManualHelpData.hpp"
#include <algorithm>
#include <cctype>

// ============================================================================
// Help data (shared across all Manual module instances)
// ============================================================================

static std::map<std::string, ModuleHelpData>& getHelpData() {
    static std::map<std::string, ModuleHelpData> data = initHelpData();
    return data;
}

static std::string toUpper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

static std::string findEntryText(const std::string& moduleSlug, const std::string& targetName,
                                  const std::string& lang) {
    auto& data = getHelpData();
    auto it = data.find(moduleSlug);
    if (it == data.end()) return "";

    const ModuleHelpData& md = it->second;
    std::string upperTarget = toUpper(targetName);

    // 1. Exact case-insensitive match
    for (const auto& [entryName, entry] : md.entries) {
        if (toUpper(entryName) == upperTarget) {
            return entry.get(lang);
        }
    }

    // 2. Best substring match (minimum 3 chars)
    std::string bestMatch;
    size_t bestLen = 0;
    for (const auto& [entryName, entry] : md.entries) {
        std::string upperEntry = toUpper(entryName);
        if (upperEntry.length() >= 3 && upperTarget.find(upperEntry) != std::string::npos) {
            if (upperEntry.length() > bestLen) {
                bestLen = upperEntry.length();
                bestMatch = entry.get(lang);
            }
        }
        if (upperTarget.length() >= 3 && upperEntry.find(upperTarget) != std::string::npos) {
            if (upperTarget.length() > bestLen) {
                bestLen = upperTarget.length();
                bestMatch = entry.get(lang);
            }
        }
    }
    return bestMatch;
}

static std::string findModuleDesc(const std::string& moduleSlug, const std::string& lang) {
    auto& data = getHelpData();
    auto it = data.find(moduleSlug);
    if (it == data.end()) return "";
    return it->second.description.get(lang);
}

// ============================================================================
// Manual Module
// ============================================================================

struct Manual : Module {
    int panelTheme = madzineDefaultTheme;
    float panelContrast = madzineDefaultContrast;
    int language = 1;  // 1=en, 2=zh, 3=ja
    float fontSize = 20.f;  // Body text font size (default doubled from 10 to 20)

    static constexpr float FONT_SIZE_MIN = 8.f;
    static constexpr float FONT_SIZE_MAX = 32.f;
    static constexpr float FONT_SIZE_STEP = 2.f;

    // Hover state (written by widget, read by display)
    std::string hoveredModuleName;
    std::string hoveredTargetName;
    std::string hoveredTargetType;  // "param", "input", "output", ""
    std::string hoveredHelpText;

    Manual() {
        config(0, 0, 0, 0);
    }

    void process(const ProcessArgs& args) override {}

    std::string getEffectiveLanguage() {
        switch (language) {
            case 2: return "zh";
            case 3: return "ja";
            default: return "en";
        }
    }

    std::string getLanguageDisplayName() {
        if (language == 2) return "\xe7\xb9\x81\xe4\xb8\xad";  // 繁中
        if (language == 3) return "\xe6\x97\xa5\xe6\x9c\xac";  // 日本
        return "EN";
    }

    void cycleLanguage() {
        // Cycle: 1 -> 2 -> 3 -> 1
        language = (language % 3) + 1;
    }

    void increaseFontSize() {
        fontSize = std::min(fontSize + FONT_SIZE_STEP, FONT_SIZE_MAX);
    }

    void decreaseFontSize() {
        fontSize = std::max(fontSize - FONT_SIZE_STEP, FONT_SIZE_MIN);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));
        json_object_set_new(rootJ, "language", json_integer(language));
        json_object_set_new(rootJ, "fontSize", json_real(fontSize));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* j;
        j = json_object_get(rootJ, "panelTheme");
        if (j) panelTheme = json_integer_value(j);
        j = json_object_get(rootJ, "panelContrast");
        if (j) panelContrast = json_number_value(j);
        j = json_object_get(rootJ, "language");
        if (j) {
            language = json_integer_value(j);
            if (language < 1 || language > 3) language = 1;  // Migrate old Auto(0) to EN
        }
        j = json_object_get(rootJ, "fontSize");
        if (j) fontSize = clamp((float)json_number_value(j), FONT_SIZE_MIN, FONT_SIZE_MAX);
    }
};

// ============================================================================
// Display Widget
// ============================================================================

struct ManualDisplay : TransparentWidget {
    Manual* module = nullptr;

    ManualDisplay() {
        box.size = Vec(12 * RACK_GRID_WIDTH - 10, 325);
    }

    void draw(const DrawArgs& args) override {
        // Background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(args.vg);

        auto font = APP->window->uiFont;
        if (!font) return;

        float pad = 6.f;
        float maxW = box.size.x - pad * 2;
        float bodySize = module ? module->fontSize : 20.f;

        if (!module) {
            // Module browser preview
            drawContent(args, font, pad, maxW, bodySize, "Manual", "", "",
                        "Move cursor over a\nMADZINE module to\nsee its description.");
            return;
        }

        if (module->hoveredModuleName.empty()) {
            // No hover - show instructions
            std::string lang = module->getEffectiveLanguage();
            std::string instr;
            if (lang == "zh")
                instr = "\xe5\xb0\x87\xe6\xb8\xb8\xe6\xa8\x99\xe7\xa7\xbb\xe5\x88\xb0 MADZINE \xe6\xa8\xa1\xe7\xb5\x84\xe4\xb8\x8a\n\xe4\xbb\xa5\xe6\x9f\xa5\xe7\x9c\x8b\xe8\xaa\xaa\xe6\x98\x8e\xe3\x80\x82";
            else if (lang == "ja")
                instr = "MADZINE\xe3\x83\xa2\xe3\x82\xb8\xe3\x83\xa5\xe3\x83\xbc\xe3\x83\xab\xe3\x81\xab\n\xe3\x82\xab\xe3\x83\xbc\xe3\x82\xbd\xe3\x83\xab\xe3\x82\x92\xe5\x90\x88\xe3\x82\x8f\xe3\x81\x9b\xe3\x82\x8b\xe3\x81\xa8\n\xe8\xaa\xac\xe6\x98\x8e\xe3\x81\x8c\xe8\xa1\xa8\xe7\xa4\xba\xe3\x81\x95\xe3\x82\x8c\xe3\x81\xbe\xe3\x81\x99\xe3\x80\x82";
            else
                instr = "Move cursor over a\nMADZINE module to\nsee its description.";
            drawContent(args, font, pad, maxW, bodySize, "Manual", "", "", instr);
        } else {
            drawContent(args, font, pad, maxW, bodySize,
                        module->hoveredModuleName,
                        module->hoveredTargetName,
                        module->hoveredTargetType,
                        module->hoveredHelpText);
        }
    }

    void drawContent(const DrawArgs& args, std::shared_ptr<window::Font> font,
                     float pad, float maxW, float bodySize,
                     const std::string& title,
                     const std::string& target,
                     const std::string& targetType,
                     const std::string& body) {
        float y = pad;

        nvgFontFaceId(args.vg, font->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

        // Module name (scaled: bodySize * 1.3)
        float titleSize = bodySize * 1.3f;
        nvgFontSize(args.vg, titleSize);
        nvgFillColor(args.vg, nvgRGB(255, 200, 0));
        // Bold via stroke
        nvgText(args.vg, pad, y, title.c_str(), NULL);
        nvgStrokeColor(args.vg, nvgRGB(255, 200, 0));
        nvgStrokeWidth(args.vg, 0.3f);
        y += titleSize * 1.3f;

        // Param/port name (same size as body)
        if (!target.empty()) {
            nvgFontSize(args.vg, bodySize);
            if (targetType == "input")
                nvgFillColor(args.vg, nvgRGB(180, 200, 255));
            else if (targetType == "output")
                nvgFillColor(args.vg, nvgRGB(255, 133, 133));
            else
                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            nvgText(args.vg, pad, y, target.c_str(), NULL);
            y += bodySize * 1.4f;
        }

        // Separator
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, pad, y);
        nvgLineTo(args.vg, pad + maxW, y);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 50));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStroke(args.vg);
        y += 5;

        // Body text (word-wrapped by nvgTextBox)
        nvgFontSize(args.vg, bodySize);
        nvgFillColor(args.vg, nvgRGB(210, 210, 210));
        nvgTextBox(args.vg, pad, y, maxW, body.c_str(), NULL);
    }
};

// ============================================================================
// Title Label
// ============================================================================

struct ManualTitleLabel : TransparentWidget {
    std::string text;
    float fontSize;
    NVGcolor color;

    ManualTitleLabel(Vec pos, const std::string& t, float fs, NVGcolor c) {
        box.pos = pos;
        box.size = Vec(100, 20);
        text = t;
        fontSize = fs;
        color = c;
    }

    void draw(const DrawArgs& args) override {
        auto font = APP->window->uiFont;
        if (!font) return;
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, fontSize);
        nvgFillColor(args.vg, color);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        float cy = box.size.y / 2.f;
        nvgText(args.vg, 0, cy, text.c_str(), NULL);
        // Bold stroke
        nvgStrokeColor(args.vg, color);
        nvgStrokeWidth(args.vg, 0.3f);
    }
};

// ============================================================================
// Language Switch Button (clickable, cycles through languages)
// ============================================================================

struct LanguageSwitchButton : TransparentWidget {
    Manual* module = nullptr;

    LanguageSwitchButton() {
        box.size = Vec(50, 14);
    }

    void draw(const DrawArgs& args) override {
        auto font = APP->window->uiFont;
        if (!font) return;

        // Background pill
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 30));
        nvgFill(args.vg);

        // Language text
        std::string langText = "EN";
        if (module) {
            langText = module->getLanguageDisplayName();
        }

        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 9.f);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 200));
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, langText.c_str(), NULL);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (module) {
                module->cycleLanguage();
            }
            e.consume(this);
        }
        TransparentWidget::onButton(e);
    }
};

// ============================================================================
// Font Size Button (A- / A+)
// ============================================================================

struct FontSizeButton : TransparentWidget {
    Manual* module = nullptr;
    bool isIncrease = true;  // true = A+, false = A-

    FontSizeButton() {
        box.size = Vec(22, 14);
    }

    void draw(const DrawArgs& args) override {
        auto font = APP->window->uiFont;
        if (!font) return;

        // Background pill
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 30));
        nvgFill(args.vg);

        std::string label = isIncrease ? "A+" : "A-";

        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 8.f);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 200));
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, label.c_str(), NULL);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (module) {
                if (isIncrease)
                    module->increaseFontSize();
                else
                    module->decreaseFontSize();
            }
            e.consume(this);
        }
        TransparentWidget::onButton(e);
    }
};

// ============================================================================
// Manual Widget
// ============================================================================

struct ManualWidget : ModuleWidget {
    PanelThemeHelper panelThemeHelper;

    ManualWidget(Manual* module) {
        setModule(module);
        panelThemeHelper.init(this, "12HP", module ? &module->panelContrast : nullptr);
        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        float panelW = box.size.x;  // 182.88

        // Title
        addChild(new ManualTitleLabel(Vec(5, 5), "Manual", 14.f, nvgRGB(255, 200, 0)));

        // Brand
        addChild(new ManualTitleLabel(Vec(5, 20), "MADZINE", 10.f, nvgRGB(255, 200, 0)));

        // Control row: Language switch + A- + A+ (Y=34)
        float controlY = 34.f;

        // Language switch button (left side)
        auto* langBtn = new LanguageSwitchButton();
        langBtn->module = module;
        langBtn->box.pos = Vec(5, controlY);
        addChild(langBtn);

        // A- button (right side, before A+)
        auto* fontDecBtn = new FontSizeButton();
        fontDecBtn->module = module;
        fontDecBtn->isIncrease = false;
        fontDecBtn->box.pos = Vec(panelW - 10 - 22 - 3 - 22, controlY);  // 125.88
        addChild(fontDecBtn);

        // A+ button (rightmost)
        auto* fontIncBtn = new FontSizeButton();
        fontIncBtn->module = module;
        fontIncBtn->isIncrease = true;
        fontIncBtn->box.pos = Vec(panelW - 10 - 22, controlY);  // 150.88
        addChild(fontIncBtn);

        // Display (starts below control row)
        auto* display = new ManualDisplay();
        display->module = module;
        display->box.pos = Vec(5, 50);
        addChild(display);
    }

    void step() override {
        Manual* m = dynamic_cast<Manual*>(module);
        if (m) updateHoverState(m);
        panelThemeHelper.step(m);
        ModuleWidget::step();
    }

    void updateHoverState(Manual* m) {
        widget::Widget* hovered = APP->event->hoveredWidget;
        if (!hovered) {
            clearHoverState(m);
            return;
        }

        std::string lang = m->getEffectiveLanguage();

        // Check ParamWidget
        if (auto* pw = dynamic_cast<app::ParamWidget*>(hovered)) {
            if (isMadzineModule(pw->module)) {
                std::string slug = pw->module->model->slug;
                if (slug == "Manual") return;  // Don't show help for self
                std::string paramName = pw->getParamQuantity() ? pw->getParamQuantity()->name : "";

                m->hoveredModuleName = pw->module->model->name;
                m->hoveredTargetName = paramName;
                m->hoveredTargetType = "param";

                std::string text = findEntryText(slug, paramName, lang);
                m->hoveredHelpText = text.empty() ? findModuleDesc(slug, lang) : text;
                return;
            }
        }

        // Check PortWidget
        if (auto* portw = dynamic_cast<app::PortWidget*>(hovered)) {
            if (isMadzineModule(portw->module)) {
                std::string slug = portw->module->model->slug;
                if (slug == "Manual") return;
                engine::PortInfo* info = portw->getPortInfo();
                std::string portName = info ? info->name : "";
                bool isInput = (portw->type == engine::Port::INPUT);

                m->hoveredModuleName = portw->module->model->name;
                m->hoveredTargetName = portName;
                m->hoveredTargetType = isInput ? "input" : "output";

                std::string text = findEntryText(slug, portName, lang);
                m->hoveredHelpText = text.empty() ? findModuleDesc(slug, lang) : text;
                return;
            }
        }

        // Check ModuleWidget (hovering panel background)
        if (auto* mw = hovered->getAncestorOfType<app::ModuleWidget>()) {
            if (mw->model && mw->model->plugin && mw->model->plugin->slug == "MADZINE") {
                std::string slug = mw->model->slug;
                if (slug == "Manual") return;

                m->hoveredModuleName = mw->model->name;
                m->hoveredTargetName = "";
                m->hoveredTargetType = "";
                m->hoveredHelpText = findModuleDesc(slug, m->getEffectiveLanguage());
                return;
            }
        }

        // Not hovering MADZINE module
        clearHoverState(m);
    }

    bool isMadzineModule(engine::Module* mod) {
        return mod && mod->model && mod->model->plugin &&
               mod->model->plugin->slug == "MADZINE";
    }

    void clearHoverState(Manual* m) {
        m->hoveredModuleName = "";
        m->hoveredTargetName = "";
        m->hoveredTargetType = "";
        m->hoveredHelpText = "";
    }

    void appendContextMenu(ui::Menu* menu) override {
        Manual* m = dynamic_cast<Manual*>(module);
        if (!m) return;

        addPanelThemeMenu(menu, m);
    }
};

Model* modelManual = createModel<Manual, ManualWidget>("Manual");
