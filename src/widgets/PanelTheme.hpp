#pragma once
#include "../plugin.hpp"

// Simple panel theme helper
struct PanelThemeHelper {
    SvgPanel* lightPanel = nullptr;
    SvgPanel* darkPanel = nullptr;
    SvgPanel* toiletPaperPanel = nullptr;

    void init(ModuleWidget* widget, const std::string& baseName) {
        // Create light (Sashimi) panel
        lightPanel = createPanel(asset::plugin(pluginInstance, "res/" + baseName + ".svg"));
        widget->setPanel(lightPanel);

        // Create dark (Boring) panel as overlay
        darkPanel = new SvgPanel();
        darkPanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_Boring.svg")));
        darkPanel->visible = false;
        widget->addChild(darkPanel);

        // Create white (Toilet Paper) panel as overlay
        toiletPaperPanel = new SvgPanel();
        toiletPaperPanel->setBackground(Svg::load(asset::plugin(pluginInstance, "res/" + baseName + "_ToiletPaper.svg")));
        toiletPaperPanel->visible = false;
        widget->addChild(toiletPaperPanel);
    }

    template<typename TModule>
    void step(TModule* module) {
        if (module && darkPanel && toiletPaperPanel) {
            darkPanel->visible = (module->panelTheme == 1);
            toiletPaperPanel->visible = (module->panelTheme == 2);
        }
    }
};

// Add panel theme menu items
template<typename TModule>
inline void addPanelThemeMenu(ui::Menu* menu, TModule* module) {
    menu->addChild(new ui::MenuSeparator);
    menu->addChild(createMenuLabel("Panel Theme"));

    struct ThemeItem : ui::MenuItem {
        TModule* module;
        int theme;

        void onAction(const event::Action& e) override {
            if (module) module->panelTheme = theme;
        }

        void step() override {
            if (module) {
                rightText = (module->panelTheme == theme) ? "✔" : "";
            }
            MenuItem::step();
        }
    };

    ThemeItem* sashimiItem = createMenuItem<ThemeItem>("Sashimi");
    sashimiItem->module = module;
    sashimiItem->theme = 0;
    menu->addChild(sashimiItem);

    ThemeItem* boringItem = createMenuItem<ThemeItem>("Boring");
    boringItem->module = module;
    boringItem->theme = 1;
    menu->addChild(boringItem);

    ThemeItem* toiletPaperItem = createMenuItem<ThemeItem>("Toilet Paper");
    toiletPaperItem->module = module;
    toiletPaperItem->theme = 2;
    menu->addChild(toiletPaperItem);
}
