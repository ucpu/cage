#include "main.h"

#include <cage-core/math.h>
#include <cage-core/files.h>

namespace
{
	string generatePoint()
	{
		vec3 p = randomDirection3();
		return string() + "vec3(" + p[0] + ", " + p[1] + ", " + p[2] + ")";
	}
}

void generatePointsOnSphere()
{
	CAGE_TESTCASE("generatePointsOnSphere");

	holder<fileHandle> fileHandle = newFile("pointsOnSphere.txt", fileMode(false, true));

	fileHandle->writeLine("vec3 pointsOnSphere[] =");
	fileHandle->writeLine("{");
	for (uint32 i = 0; i < 255; i++)
		fileHandle->writeLine(generatePoint() + ",");
	fileHandle->writeLine(generatePoint());
	fileHandle->writeLine("}");

	fileHandle->close();
}
