#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/files.h>
#include <cage-core/config.h>
#include <cage-core/assetHeader.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

#include <cage-engine/assetStructs.h>

using namespace cage;

extern string inputDirectory;
extern string inputName;
extern string outputDirectory;
extern string outputName;

extern string inputFileName;
extern string outputFileName;
extern string inputFile;
extern string inputSpec;
extern string inputIdentifier;

extern const char *logComponentName;

void writeLine(const string &other);
string properties(const string &name);

AssetHeader initializeAssetHeader();

void processTexture();
void processShader();
void processPack();
void processObject();
void processMesh();
void processSkeleton();
void processAnimation();
void processFont();
void processTextpack();
void processSound();
void processCollider();
void processRaw();

int processAnalyze();
void analyzeTexture();
void analyzeAssimp();
void analyzeFont();
void analyzeSound();
