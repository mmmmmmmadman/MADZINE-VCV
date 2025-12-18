#pragma once
#include "../plugin.hpp"
#include <settings.hpp>

// Panel theme values:
// -1 = Follow VCV dark panel setting (default)
//  0 = Sashimi (pink)
//  1 = Boring (dark gray)
//  2 = Toilet Paper (light gray)
//  3 = Wine (wine red)

// Simple panel theme helper
struct PanelThemeHelper {
    SvgPanel* sashimiPanel = nullptr;
    SvgPanel* boringPanel = nullptr;
    SvgPanel* toiletPaperPanel = nullptr;
    SvgPanel* winePanel = nullptr;

    void init(ModuleWidget* widget, const std::string& baseName) {
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

        // Create Wine (wine red) panel as overlay - used as dark panel
        winePanel = new SvgPanel();
        winePanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_Wine.svg")));
        winePanel->visible = false;
        widget->addChild(winePanel);
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
            effectiveTheme = settings::preferDarkPanels ? 3 : 0;  // Wine or Sashimi
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

    // Auto option follows VCV's "Use dark panels" setting
    ThemeItem* autoItem = createMenuItem<ThemeItem>("Auto (Follow VCV)");
    autoItem->module = module;
    autoItem->theme = -1;
    autoItem->isAuto = true;
    menu->addChild(autoItem);

    menu->addChild(new ui::MenuSeparator);

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
}
