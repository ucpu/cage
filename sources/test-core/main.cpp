#include <cage-core/files.h>
#include <cage-core/logger.h>

#include "main.h"

uint32 CageTestCase::counter = 0;

void testMacros();
void testEnums();
void testExceptions();
void testNumericTypes();
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
void testMemoryBuffers();
void testMemoryAllocators();
void testSerialization();
void testConcurrent();
void testConcurrentQueue();
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
void testMesh();
void testAudio();
void testMarchingCubes();
void testNoise();
void testRectPacking();
void testSpatialStructure();
void testColliders();
void testCollisionStructure();
void testEntities();
void testEntitiesSerialization();
void testVariableInterpolatingBuffer();
void testVariableSmoothingBuffer();
void testCopyAndMove();
void testAssetManager();
void testSwapBufferGuard();
void testScheduler();
void testTcp();
void testUdp();
void testUdpDiscovery();
void testProcess();
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
	testMemoryBuffers();
	testMemoryAllocators();
	testSerialization();
	testConcurrent();
	testConcurrentQueue();
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
	testMesh();
	testAudio();
	testMarchingCubes();
	testNoise();
	testRectPacking();
	testSpatialStructure();
	testColliders();
	testCollisionStructure();
	testEntities();
	testEntitiesSerialization();
	testVariableInterpolatingBuffer();
	testVariableSmoothingBuffer();
	testCopyAndMove();
	testAssetManager();
	testSwapBufferGuard();
	testScheduler();
	testTcp();
	testUdp();
	testUdpDiscovery();
	testProcess();
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
