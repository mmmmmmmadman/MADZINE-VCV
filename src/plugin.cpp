#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

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
}
