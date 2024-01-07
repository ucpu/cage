#include <cage-core/assetHeader.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/geometry.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryCompression.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>
#include <cage-engine/assetStructs.h>

using namespace cage;

extern String inputDirectory;
extern String inputName;
extern String outputDirectory;
extern String outputName;

extern String inputFileName;
extern String outputFileName;
extern String inputFile;
extern String inputSpec;
extern String inputIdentifier;

void writeLine(const String &other);
String properties(const String &name);

AssetHeader initializeAssetHeader();

void processTexture();
void processShader();
void processPack();
void processObject();
void processModel();
void processSkeleton();
void processAnimation();
void processFont();
void processTextpack();
void processSound();
void processCollider();
void processRaw();

int processAnalyze();
void analyzeTexture();
void analyzeFont();
void analyzeSound();

// relative path is interpreted relative to the input file (unless specified otherwise)
// absolute path is interpreted as relative to input root path
String convertAssetPath(const String &input, const String &relativeTo = "", bool markAsReferenced = true);
String convertFilePath(const String &input, const String &relativeTo = "", bool markAsUsed = true);
