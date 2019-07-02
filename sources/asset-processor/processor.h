#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/files.h>
#include <cage-core/config.h>
#include <cage-core/assetStructs.h>

#include <cage-client/core.h>
#include <cage-client/assetStructs.h>

using namespace cage;

extern string inputDirectory;
extern string inputName;
extern string outputDirectory;
extern string outputName;
extern string assetPath;
extern string schemePath;
extern string cachePath;

extern string inputFileName;
extern string outputFileName;
extern string inputFile;
extern string inputSpec;
extern string inputIdentifier;

extern const char *logComponentName;

void writeLine(const string &other);
string properties(const string &name);

assetHeader initializeAssetHeaderStruct();

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
