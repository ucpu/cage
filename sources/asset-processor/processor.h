#include <cage-core/assetHeader.h>
#include <cage-core/assetProcessor.h>
#include <cage-core/assetsSchemes.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/geometry.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryCompression.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/assetsStructs.h>

using namespace cage;

extern Holder<const AssetProcessor> processor;

void processTexture();
void processShader();
void processPack();
void processObject();
void processModel();
void processSkeleton();
void processAnimation();
void processFont();
void processTexts();
void processSound();
void processCollider();
void processRaw();

int processAnalyze();
void analyzeTexture();
void analyzeFont();
void analyzeSound();
