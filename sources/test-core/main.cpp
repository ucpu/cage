#include <cage-core/files.h>
#include <cage-core/logger.h>

#include "main.h"

uint32 CageTestCase::counter = 0;

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
void testEnumerate();
void testEvents();
void testMath();
void testMathGlm();
void testGeometry();
void testMemoryArena();
void testMemoryBuffers();
void testMemoryAllocators();
void testSerialization();
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
void testMesh();
void testMarchingCubes();
void testSignedDistanceFunctions();
void testAudio();
void testRectPacking();
void testSpatialStructure();
void testColliders();
void testCollisionStructure();
void testEntities();
void testEntitiesSerialization();
void testEntitiesVisitor();
void testVariableInterpolatingBuffer();
void testVariableSmoothingBuffer();
void testCopyAndMove();
void testAssetManager();
void testSwapBufferGuard();
void testScheduler();
void testNetworkTcp();
void testNetworkGinnel();
void testNetworkDiscovery();
void testProcess();
void testLogger();
void testSystemInformation();
void generatePointsOnSphere();

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
	testEnumerate();
	testEvents();
	testMath();
	testMathGlm();
	testGeometry();
	testMemoryArena();
	testMemoryBuffers();
	testMemoryAllocators();
	testSerialization();
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
	testMesh();
	testMarchingCubes();
	testSignedDistanceFunctions();
	testAudio();
	testRectPacking();
	testSpatialStructure();
	testColliders();
	testCollisionStructure();
	testEntities();
	testEntitiesSerialization();
	testEntitiesVisitor();
	testVariableInterpolatingBuffer();
	testVariableSmoothingBuffer();
	testCopyAndMove();
	testAssetManager();
	testSwapBufferGuard();
	testScheduler();
	testNetworkTcp();
	testNetworkGinnel();
	testNetworkDiscovery();
	testProcess();
	testLogger();
	testSystemInformation();

	{
		CAGE_TESTCASE("removing testdir");
		pathRemove("testdir");
	}

	{
		CAGE_TESTCASE("all tests done ok");
	}

	return 0;
}
