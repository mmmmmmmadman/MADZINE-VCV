#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add all Models defined in the module files
    p->addModel(modelSwingLFO);
    p->addModel(modelSwingLiFO);
    p->addModel(modelEuclideanRhythm);
}