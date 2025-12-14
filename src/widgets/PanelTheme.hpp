#pragma once
#include "../plugin.hpp"

// Simple panel theme helper
struct PanelThemeHelper {
    SvgPanel* sashimiPanel = nullptr;
    SvgPanel* boringPanel = nullptr;
    SvgPanel* toiletPaperPanel = nullptr;
    SvgPanel* winePanel = nullptr;

    void init(ModuleWidget* widget, const std::string& baseName) {
        // Create Sashimi (pink) panel - default
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
    }

    template<typename TModule>
    void step(TModule* module) {
        if (module && boringPanel && toiletPaperPanel && winePanel) {
            boringPanel->visible = (module->panelTheme == 1);
            toiletPaperPanel->visible = (module->panelTheme == 2);
            winePanel->visible = (module->panelTheme == 3);
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

    ThemeItem* wineItem = createMenuItem<ThemeItem>("Wine");
    wineItem->module = module;
    wineItem->theme = 3;
    menu->addChild(wineItem);
}
