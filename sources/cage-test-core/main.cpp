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
void testRandom();
void testMemoryArenas();
void testMemoryPools();
void testMemoryPerformance();
void testConcurrent();
void testFiles();
void testTcp();
void testUdp();
void testUdpDiscovery();
void testConfig();
void testSceneEntities();
void testSceneSerialize();
void testSpatial();
void testCollisions();
void testPng();
void testNoise();
void testHashTable();
void testIni();
void testColor();
void testInterpolator();
void testProgram();
void testMemoryBuffers();
void testCopyAndMove();

int main()
{
	try
	{
		holder<loggerClass> log1 = newLogger();
		log1->filter.bind<logFilterPolicyPass>();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		newFilesystem()->remove("testdir");
		testMacros();
		testEnums();
		testExceptions();
		testNumericCast();
		testTemplates();
		testStrings();
		testDelegates();
		testHolder();
		testEvents();
		testRandom();
		testMath();
		testMathGlm();
		testGeometry();
		testHashTable();
		testSpatial();
		testCollisions();
		testSceneEntities();
		testSceneSerialize();
		testMemoryArenas();
		testMemoryPools();
		testMemoryPerformance();
		testMemoryBuffers();
		testConcurrent();
		testFiles();
		testIni();
		testConfig();
		testColor();
		testPng();
		testNoise();
		testInterpolator();
		testProgram();
		testTcp();
		testUdp();
		testUdpDiscovery();
		testCopyAndMove();
		newFilesystem()->remove("testdir");

		{
			CAGE_TESTCASE("all tests done");
		}

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}