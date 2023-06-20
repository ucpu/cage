#include "main.h"

#include <cage-core/files.h>
#include <cage-core/logger.h>

void testMacros();
void testEnums();
void testExceptions();
void testNumericTypes();
void testTypeIndex();
void testStrings();
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
void testFiles();
void testArchives();
void testArchivesRecursion();
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
void testNetworkDiscovery();
void testProcess();
void testLogger();
void testSystemInformation();
void generatePointsOnSphere();
void testCageInstallConsistentPaths();

int main()
{
	Holder<Logger> log1 = newLogger();
	log1->format.bind<logFormatConsole>();
	log1->output.bind<logOutputStdOut>();

	//generatePointsOnSphere();
	testMacros();
	testEnums();
	testExceptions();
	testNumericTypes();
	testTypeIndex();
	testStrings();
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
	testFiles();
	testArchives();
	testArchivesRecursion();
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
	testNetworkDiscovery();
	testProcess();
	testLogger();
	testSystemInformation();
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
