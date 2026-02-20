#pragma once
#include <rack.hpp>

using namespace rack;

// Forward-declare each Model
extern Model* modelSwingLFO;
extern Model* modelEuclideanRhythm;
extern Model* modelADGenerator;
extern Model* modelPinpple;
extern Model* modelPPaTTTerning;
extern Model* modelMADDY;
extern Model* modelTWNC;
extern Model* modelTWNCLight;
extern Model* modelTWNC2;
extern Model* modelQQ;
extern Model* modelObserver;
extern Model* modelU8;
extern Model* modelYAMANOTE;
extern Model* modelKIMO;
extern Model* modelObserfour;
extern Model* modelPyramid;
extern Model* modelDECAPyramid;
extern Model* modelKEN;
extern Model* modelQuantizer;
extern Model* modelEllenRipley;
extern Model* modelMADDYPlus;
extern Model* modelNIGOQ;
extern Model* modelRunshow;
extern Model* modelEnvVCA6;
extern Model* modelWeiiiDocumenta;
extern Model* modelUniversalRhythm;
extern Model* modelUniRhythm;
extern Model* modelSongMode;
extern Model* modelLaunchpad;
extern Model* modelRunner;
extern Model* modelFacehugger;
extern Model* modelOvomorph;
extern Model* modelALEXANDERPLATZ;
extern Model* modelSHINJUKU;
extern Model* modelPortal;
extern Model* modelDrummmmmmer;
extern Model* modeltheKICK;
extern Model* modelManual;

// Declare the Plugin instance
extern Plugin* pluginInstance;

// ============================================================================
// Global Panel Settings (saved to MADZINE.json)
// ============================================================================

// Global default values for new modules
extern float madzineDefaultContrast;  // Default: 255.0 (max brightness)
extern int madzineDefaultTheme;       // Default: -1 (auto/follow VCV setting)

// Load/save global settings
void madzineLoadSettings();
void madzineSaveSettings();

// Apply settings to all MADZINE modules in current patch
void madzineApplyContrastToAll(float contrast);
void madzineApplyThemeToAll(int theme);
