#include "main.h"
#include <cage-core/files.h>
#include <cage-core/logger.h>

#include <cstdlib>

uint32 CageTestCase::counter = 0;

void testMacros();
void testEnums();
void testExceptions();
void testNumericTypes();
void testStrings();
void testDelegates();
void testHolder();
void testClasses();
void testPointerRange();
void testEnumerate();
void testEvents();
void testMath();
void testMathGlm();
void testGeometry();
void testMemoryArenas();
void testMemoryPools();
void testMemoryPerformance();
void testMemoryBuffers();
void testSerialization();
void testConcurrent();
void testConcurrentQueue();
void testLruCache();
void testFiles();
void testArchives();
void testLineReader();
void testRandom();
void testConfigIni();
void testConfig();
void testColor();
void testImage();
void testPolyhedron();
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
void testSwapBufferGuard();
void testScheduler();
void testTcp();
void testUdp();
void testUdpDiscovery();
void testProgram();
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
	testDelegates();
	testHolder();
	testClasses();
	testPointerRange();
	testEnumerate();
	testEvents();
	testMath();
	testMathGlm();
	testGeometry();
	testMemoryArenas();
	testMemoryPools();
	testMemoryPerformance();
	testMemoryBuffers();
	testSerialization();
	testConcurrent();
	testConcurrentQueue();
	testLruCache();
	testFiles();
	testArchives();
	testLineReader();
	testRandom();
	testConfigIni();
	testConfig();
	testColor();
	testImage();
	testPolyhedron();
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
	testSwapBufferGuard();
	testScheduler();
	testTcp();
	testUdp();
	testUdpDiscovery();
	testProgram();
	testSystemInformation();
	pathRemove("testdir");

	{
		CAGE_TESTCASE("all tests done ok");
	}

	return 0;
}
