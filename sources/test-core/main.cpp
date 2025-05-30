#include "main.h"

#include <cage-core/files.h>
#include <cage-core/logger.h>

void testMacros();
void testEnums();
void testExceptions();
void testNumericTypes();
void testTypeIndex();
void testStrings();
void testUnicode();
void testPaths();
void testDelegates();
void testHolder();
void testClasses();
void testPointerRange();
void testAny();
void testEnumerate();
void testEvents();
void testMath();
void testMathGlm();
void testGeometry();
void testMemoryArena();
void testMemoryBuffers();
void testMemoryAllocators();
void testStdHash();
void testHashes();
void testBlockContainer();
void testSerialization();
void testContainerSerialization();
void testConcurrent();
void testConcurrentQueue();
void testProfiling();
void testTasks();
void testLruCache();
void testFlatSet();
void testFlatBag();
void testFiles();
void testArchivesZip();
void testArchivesCarch();
void testArchivesRecursion();
void testArchivesBigFiles();
void testLineReader();
void testRandom();
void testConfigIni();
void testConfig();
void testColor();
void testImage();
void testNoise();
void testSpatialStructure();
void testMesh();
void testMarchingCubes();
void testSignedDistanceFunctions();
void testAudio();
void testRectPacking();
void testColliders();
void testCollisionStructure();
void testEntities();
void testEntitiesSerialization();
void testEntitiesVisitor();
void testEntitiesCopy();
void testVariableInterpolatingBuffer();
void testVariableSmoothingBuffer();
void testCopyAndMove();
void testAssetManager();
void testAssetOnDemand();
void testSwapBufferGuard();
void testScheduler();
void testNetworkTcp();
void testNetworkWebsocket();
void testNetworkGinnel();
void testNetworkSteam();
void testNetworkDiscovery();
void testNetworkHttp();
void testProcess();
void testLogger();
void testSystemInformation();
void testWasm();
void testCageInstallConsistentPaths();
void generatePointsOnSphere();

int main()
{
	initializeConsoleLogger();

	//generatePointsOnSphere();
	testMacros();
	testEnums();
	testExceptions();
	testNumericTypes();
	testTypeIndex();
	testStrings();
	testUnicode();
	testPaths();
	testDelegates();
	testHolder();
	testClasses();
	testPointerRange();
	testAny();
	testEnumerate();
	testEvents();
	testMath();
	testMathGlm();
	testGeometry();
	testMemoryArena();
	testMemoryBuffers();
	testMemoryAllocators();
	testStdHash();
	testHashes();
	testBlockContainer();
	testSerialization();
	testContainerSerialization();
	testConcurrent();
	testConcurrentQueue();
	testProfiling();
	testTasks();
	testLruCache();
	testFlatSet();
	testFlatBag();
	testFiles();
	testArchivesZip();
	testArchivesCarch();
	testArchivesRecursion();
	testArchivesBigFiles();
	testLineReader();
	testRandom();
	testConfigIni();
	testConfig();
	testColor();
	testImage();
	testNoise();
	testSpatialStructure();
	testMesh();
	testMarchingCubes();
	testSignedDistanceFunctions();
	testAudio();
	testRectPacking();
	testColliders();
	testCollisionStructure();
	testEntities();
	testEntitiesSerialization();
	testEntitiesVisitor();
	testEntitiesCopy();
	testVariableInterpolatingBuffer();
	testVariableSmoothingBuffer();
	testCopyAndMove();
	testAssetManager();
	testAssetOnDemand();
	testSwapBufferGuard();
	testScheduler();
	testNetworkTcp();
	testNetworkWebsocket();
	testNetworkGinnel();
	testNetworkSteam();
	testNetworkDiscovery();
	testNetworkHttp();
	testProcess();
	testLogger();
	testSystemInformation();
	testWasm();
	testCageInstallConsistentPaths();

	{
		CAGE_TESTCASE("removing testdir");
		pathRemove("testdir");
	}

	{
		CAGE_TESTCASE("all tests done ok");
	}

	return 0;
}
