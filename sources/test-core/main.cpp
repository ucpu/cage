#include <cstdlib>

#include "main.h"
#include <cage-core/filesystem.h>

using namespace cage;

uint32 cageTestCaseStruct::counter = 0;

void testMacros();
void testEnums();
void testExceptions();
void testNumericCast();
void testTemplates();
void testStrings();
void testDelegates();
void testHolder();
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
void testHashTable();
void testRandom();
void testIni();
void testConfig();
void testColor();
void testPng();
void testNoise();
void testSpatial();
void testCollisions();
void testSceneEntities();
void testSceneSerialize();
void testVariableInterpolatingBuffer();
void testVariableSmoothingBufferStruct();
void testCopyAndMove();
void testSwapBufferController();
void testTcp();
void testUdp();
void testUdpDiscovery();
void testProgram();
void generatePointsOnSphere();

int main()
{
	holder<loggerClass> log1 = newLogger();
	log1->filter.bind<logFilterPolicyPass>();
	log1->format.bind<logFormatPolicyConsole>();
	log1->output.bind<logOutputPolicyStdOut>();

	newFilesystem()->remove("testdir");
	//generatePointsOnSphere();
	testMacros();
	testEnums();
	testExceptions();
	testNumericCast();
	testTemplates();
	testStrings();
	testDelegates();
	testHolder();
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
	testHashTable();
	testRandom();
	testIni();
	testConfig();
	testColor();
	testPng();
	testNoise();
	testSpatial();
	testCollisions();
	testSceneEntities();
	testSceneSerialize();
	testVariableInterpolatingBuffer();
	testVariableSmoothingBufferStruct();
	testCopyAndMove();
	testSwapBufferController();
	testTcp();
	testUdp();
	testUdpDiscovery();
	testProgram();
	newFilesystem()->remove("testdir");

	{
		CAGE_TESTCASE("all tests done");
	}

	return 0;
}
