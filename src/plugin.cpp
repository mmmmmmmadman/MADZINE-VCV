#include "plugin.hpp"
#include <fstream>

Plugin* pluginInstance;

// ============================================================================
// Global Panel Settings
// ============================================================================

float madzineDefaultContrast = 255.0f;  // Max brightness (no darkening)
int madzineDefaultTheme = -1;           // Auto (follow VCV setting)

// Settings file path
static std::string getSettingsPath() {
    return asset::user("MADZINE.json");
}

void madzineLoadSettings() {
    std::string path = getSettingsPath();
    FILE* file = std::fopen(path.c_str(), "r");
    if (!file) return;

    DEFER({ std::fclose(file); });

    json_error_t error;
    json_t* rootJ = json_loadf(file, 0, &error);
    if (!rootJ) return;

    DEFER({ json_decref(rootJ); });

    // Load contrast
    json_t* contrastJ = json_object_get(rootJ, "defaultContrast");
    if (contrastJ) {
        madzineDefaultContrast = json_number_value(contrastJ);
    }

    // Load theme
    json_t* themeJ = json_object_get(rootJ, "defaultTheme");
    if (themeJ) {
        madzineDefaultTheme = json_integer_value(themeJ);
    }
}

void madzineSaveSettings() {
    json_t* rootJ = json_object();

    json_object_set_new(rootJ, "defaultContrast", json_real(madzineDefaultContrast));
    json_object_set_new(rootJ, "defaultTheme", json_integer(madzineDefaultTheme));

    std::string path = getSettingsPath();
    FILE* file = std::fopen(path.c_str(), "w");
    if (file) {
        json_dumpf(rootJ, file, JSON_INDENT(2));
        std::fclose(file);
    }

    json_decref(rootJ);
}

void madzineApplyContrastToAll(float contrast) {
    // Save as default
    madzineDefaultContrast = contrast;
    madzineSaveSettings();

    // Apply to all MADZINE modules in current patch
    std::vector<int64_t> moduleIds = APP->engine->getModuleIds();
    for (int64_t id : moduleIds) {
        Module* m = APP->engine->getModule(id);
        if (m && m->model && m->model->plugin && m->model->plugin->slug == "MADZINE") {
            // Use reflection to set panelContrast if the module has it
            // All MADZINE modules should have panelContrast as a public member
            // We access it via JSON interface
            json_t* dataJ = m->dataToJson();
            if (dataJ) {
                json_object_set_new(dataJ, "panelContrast", json_real(contrast));
                m->dataFromJson(dataJ);
                json_decref(dataJ);
            }
        }
    }
}

void madzineApplyThemeToAll(int theme) {
    // Save as default
    madzineDefaultTheme = theme;
    madzineSaveSettings();

    // Apply to all MADZINE modules in current patch
    std::vector<int64_t> moduleIds = APP->engine->getModuleIds();
    for (int64_t id : moduleIds) {
        Module* m = APP->engine->getModule(id);
        if (m && m->model && m->model->plugin && m->model->plugin->slug == "MADZINE") {
            json_t* dataJ = m->dataToJson();
            if (dataJ) {
                json_object_set_new(dataJ, "panelTheme", json_integer(theme));
                m->dataFromJson(dataJ);
                json_decref(dataJ);
            }
        }
    }
}

void init(Plugin* p) {
    pluginInstance = p;

    // Load global settings
    madzineLoadSettings();

    // Add all Models defined in the module files
    p->addModel(modelSwingLFO);
    p->addModel(modelEuclideanRhythm);
    p->addModel(modelADGenerator);
    p->addModel(modelPinpple);
    p->addModel(modelPPaTTTerning);
    p->addModel(modelMADDY);
    p->addModel(modelTWNC);
    p->addModel(modelTWNCLight);
    p->addModel(modelTWNC2);
    p->addModel(modelQQ);
    p->addModel(modelObserver);
    p->addModel(modelU8);
    p->addModel(modelYAMANOTE);
    p->addModel(modelKIMO);
    p->addModel(modelObserfour);
    p->addModel(modelPyramid);
    p->addModel(modelDECAPyramid);
    p->addModel(modelKEN);
    p->addModel(modelQuantizer);
    p->addModel(modelEllenRipley);
    p->addModel(modelMADDYPlus);
    p->addModel(modelNIGOQ);
    p->addModel(modelRunshow);
    p->addModel(modelEnvVCA6);
    p->addModel(modelWeiiiDocumenta);
    p->addModel(modelUniversalRhythm);
    p->addModel(modelUniRhythm);
    p->addModel(modelSongMode);
    p->addModel(modelLaunchpad);
    p->addModel(modelRunner);
    p->addModel(modelFacehugger);
    p->addModel(modelOvomorph);
    p->addModel(modelALEXANDERPLATZ);
    p->addModel(modelSHINJUKU);
    p->addModel(modelPortal);
    p->addModel(modelWorldDrum);
}
