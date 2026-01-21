#pragma once
#include "../plugin.hpp"
#include <settings.hpp>

// Panel theme values:
// -1 = Follow VCV dark panel setting (default)
//  0 = Sashimi (pink)
//  1 = Boring (dark gray)
//  2 = Toilet Paper (light gray)
//  3 = Wine (wine red)

// Panel contrast constants (like Impromptu)
static constexpr float panelContrastDefault = 255.0f;
static constexpr float panelContrastMin = 160.0f;
static constexpr float panelContrastMax = 255.0f;

// Global default contrast value (saved across sessions)
static float globalPanelContrast = panelContrastDefault;

// Panel contrast overlay widget
// Uses semi-transparent black overlay to darken the panel
// contrast 255 = no darkening (full brightness), contrast 160 = maximum darkening
struct PanelContrastWidget : TransparentWidget {
    float* contrastSrc = nullptr;

    PanelContrastWidget(Vec size, float* src) {
        box.size = size;
        contrastSrc = src;
    }

    void draw(const DrawArgs& args) override {
        if (!contrastSrc) return;

        float contrast = clamp(*contrastSrc, panelContrastMin, panelContrastMax);

        // Only draw overlay if contrast < max (255)
        if (contrast < panelContrastMax) {
            // Calculate alpha: contrast 255 = alpha 0, contrast 160 = alpha ~0.37
            float alpha = (panelContrastMax - contrast) / panelContrastMax;

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, (int)(alpha * 255)));
            nvgFill(args.vg);
        }
    }
};

// Simple panel theme helper
struct PanelThemeHelper {
    SvgPanel* sashimiPanel = nullptr;
    SvgPanel* boringPanel = nullptr;
    SvgPanel* toiletPaperPanel = nullptr;
    SvgPanel* winePanel = nullptr;
    PanelContrastWidget* contrastWidget = nullptr;

    void init(ModuleWidget* widget, const std::string& baseName, float* contrastSrc = nullptr) {
        // Create Sashimi (pink) panel - default light
        sashimiPanel = createPanel(asset::plugin(pluginInstance, "res/" + baseName + "_Sashimi.svg"));
        widget->setPanel(sashimiPanel);

        // Create Boring (dark gray) panel as overlay
        boringPanel = new SvgPanel();
        boringPanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_Boring.svg")));
        boringPanel->visible = false;
        widget->addChild(boringPanel);

        // Create Toilet Paper (light gray) panel as overlay
        toiletPaperPanel = new SvgPanel();
        toiletPaperPanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_ToiletPaper.svg")));
        toiletPaperPanel->visible = false;
        widget->addChild(toiletPaperPanel);

        // Create Wine (wine red) panel as overlay
        winePanel = new SvgPanel();
        winePanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_Wine.svg")));
        winePanel->visible = false;
        widget->addChild(winePanel);

        // Create contrast overlay widget (on top of everything)
        if (contrastSrc) {
            contrastWidget = new PanelContrastWidget(widget->box.size, contrastSrc);
            widget->addChild(contrastWidget);
        }
    }

    template<typename TModule>
    void step(TModule* module) {
        if (!module || !boringPanel || !toiletPaperPanel || !winePanel) return;

        int theme = module->panelTheme;

        // Theme mapping:
        // <= 0: Auto (follow VCV dark panel setting)
        // 1: Boring, 2: Toilet Paper, 3: Wine, 4: Sashimi (force)
        int effectiveTheme;
        if (theme <= 0) {
            effectiveTheme = settings::preferDarkPanels ? 1 : 0;  // Boring or Sashimi (changed from Wine)
        } else if (theme == 4) {
            effectiveTheme = 0;  // Force Sashimi (no overlay)
        } else {
            effectiveTheme = theme;
        }

        boringPanel->visible = (effectiveTheme == 1);
        toiletPaperPanel->visible = (effectiveTheme == 2);
        winePanel->visible = (effectiveTheme == 3);
    }
};

// Panel contrast slider quantity (displays as 0-100%)
// Internal value: 160-255, Display: 0-100%
// 0% = darkest (internal 160), 100% = brightest (internal 255)
struct PanelContrastQuantity : Quantity {
    float* contrastSrc = nullptr;
    static constexpr float range = panelContrastMax - panelContrastMin; // 95

    PanelContrastQuantity(float* src) : contrastSrc(src) {}

    // Convert percentage (0-100) to internal value (160-255)
    void setValue(float percentage) override {
        if (contrastSrc) {
            float internal = panelContrastMin + (percentage / 100.0f) * range;
            *contrastSrc = clamp(internal, panelContrastMin, panelContrastMax);
        }
    }

    // Convert internal value (160-255) to percentage (0-100)
    float getValue() override {
        if (!contrastSrc) return getDefaultValue();
        return ((*contrastSrc - panelContrastMin) / range) * 100.0f;
    }

    float getMinValue() override { return 0.0f; }
    float getMaxValue() override { return 100.0f; }
    float getDefaultValue() override {
        // Default 220 → (220-160)/95*100 ≈ 63%
        return ((panelContrastDefault - panelContrastMin) / range) * 100.0f;
    }

    std::string getLabel() override { return "Panel contrast"; }
    std::string getUnit() override { return ""; }

    int getDisplayPrecision() override { return 0; }

    std::string getDisplayValueString() override {
        return std::to_string((int)std::round(getValue())) + "%";
    }
};

// Panel contrast slider widget
struct PanelContrastSlider : ui::Slider {
    PanelContrastSlider(float* contrastSrc) {
        quantity = new PanelContrastQuantity(contrastSrc);
        box.size.x = 200.0f;
    }

    ~PanelContrastSlider() {
        delete quantity;
    }
};

// Add panel theme menu items
// Theme values: <=0 = Auto (follow VCV), 1 = Boring, 2 = Toilet Paper, 3 = Wine, 4 = Sashimi (force)
template<typename TModule>
inline void addPanelThemeMenu(ui::Menu* menu, TModule* module) {
    menu->addChild(new ui::MenuSeparator);
    menu->addChild(createMenuLabel("Panel Theme"));

    struct ThemeItem : ui::MenuItem {
        TModule* module;
        int theme;
        bool isAuto = false;

        void onAction(const event::Action& e) override {
            if (module) module->panelTheme = theme;
        }

        void step() override {
            if (module) {
                // Auto is selected if panelTheme <= 0
                if (isAuto) {
                    rightText = (module->panelTheme <= 0) ? "✔" : "";
                } else {
                    rightText = (module->panelTheme == theme) ? "✔" : "";
                }
            }
            MenuItem::step();
        }
    };

    ThemeItem* sashimiItem = createMenuItem<ThemeItem>("Sashimi");
    sashimiItem->module = module;
    sashimiItem->theme = 4;  // New value for explicit Sashimi
    menu->addChild(sashimiItem);

    ThemeItem* boringItem = createMenuItem<ThemeItem>("Boring");
    boringItem->module = module;
    boringItem->theme = 1;
    menu->addChild(boringItem);

    ThemeItem* toiletPaperItem = createMenuItem<ThemeItem>("Toilet Paper");
    toiletPaperItem->module = module;
    toiletPaperItem->theme = 2;
    menu->addChild(toiletPaperItem);

    ThemeItem* wineItem = createMenuItem<ThemeItem>("Wine");
    wineItem->module = module;
    wineItem->theme = 3;
    menu->addChild(wineItem);

    // Theme save options
    if (module) {
        menu->addChild(new ui::MenuSeparator);

        // Save theme as default
        struct SaveThemeDefaultItem : ui::MenuItem {
            TModule* mod;
            void onAction(const event::Action& e) override {
                if (mod) {
                    madzineDefaultTheme = mod->panelTheme;
                    madzineSaveSettings();
                }
            }
        };
        SaveThemeDefaultItem* saveThemeItem = createMenuItem<SaveThemeDefaultItem>("Save theme as default");
        saveThemeItem->mod = module;
        menu->addChild(saveThemeItem);

        // Apply theme to all MADZINE modules
        struct ApplyThemeAllItem : ui::MenuItem {
            TModule* mod;
            void onAction(const event::Action& e) override {
                if (mod) {
                    madzineApplyThemeToAll(mod->panelTheme);
                }
            }
        };
        ApplyThemeAllItem* applyThemeItem = createMenuItem<ApplyThemeAllItem>("Apply theme to all MADZINE modules");
        applyThemeItem->mod = module;
        menu->addChild(applyThemeItem);
    }

    // Panel contrast slider (below theme selection)
    if (module) {
        menu->addChild(new ui::MenuSeparator);
        menu->addChild(createMenuLabel("Panel Contrast"));
        menu->addChild(new PanelContrastSlider(&module->panelContrast));

        // Save contrast as default
        struct SaveContrastDefaultItem : ui::MenuItem {
            TModule* mod;
            void onAction(const event::Action& e) override {
                if (mod) {
                    madzineDefaultContrast = mod->panelContrast;
                    madzineSaveSettings();
                }
            }
        };
        SaveContrastDefaultItem* saveContrastItem = createMenuItem<SaveContrastDefaultItem>("Save contrast as default");
        saveContrastItem->mod = module;
        menu->addChild(saveContrastItem);

        // Apply contrast to all MADZINE modules
        struct ApplyContrastAllItem : ui::MenuItem {
            TModule* mod;
            void onAction(const event::Action& e) override {
                if (mod) {
                    madzineApplyContrastToAll(mod->panelContrast);
                }
            }
        };
        ApplyContrastAllItem* applyContrastItem = createMenuItem<ApplyContrastAllItem>("Apply contrast to all MADZINE modules");
        applyContrastItem->mod = module;
        menu->addChild(applyContrastItem);
    }
}
