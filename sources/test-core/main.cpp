#include <cstdlib>

#include "main.h"
#include <cage-core/files.h>
#include <cage-core/logger.h>

using namespace cage;

uint32 cageTestCaseStruct::counter = 0;

void testMacros();
void testEnums();
void testExceptions();
void testClassCast();
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
void testFiles();
void testArchives();
void testLineReader();
void testRandom();
void testConfigIni();
void testConfig();
void testColor();
void testImage();
void testNoise();
void testSpatial();
void testCollisions();
void testSceneEntities();
void testSceneSerialize();
void testVariableInterpolatingBuffer();
void testVariableSmoothingBufferStruct();
void testCopyAndMove();
void testSwapBufferGuard();
void testTcp();
void testUdp();
void testUdpDiscovery();
void testProgram();
void testSystemInformation();
void generatePointsOnSphere();

int main()
{
	holder<logger> log1 = newLogger();
	log1->format.bind<logFormatConsole>();
	log1->output.bind<logOutputStdOut>();

	//generatePointsOnSphere();
	testMacros();
	testEnums();
	testExceptions();
	testClassCast();
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
	testFiles();
	testArchives();
	testLineReader();
	testRandom();
	testConfigIni();
	testConfig();
	testColor();
	testImage();
	testNoise();
	testSpatial();
	testCollisions();
	testSceneEntities();
	testSceneSerialize();
	testVariableInterpolatingBuffer();
	testVariableSmoothingBufferStruct();
	testCopyAndMove();
	testSwapBufferGuard();
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
