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

	holder<file> file = newFile("pointsOnSphere.txt", fileMode(false, true));

	file->writeLine("vec3 pointsOnSphere[] =");
	file->writeLine("{");
	for (uint32 i = 0; i < 255; i++)
		file->writeLine(generatePoint() + ",");
	file->writeLine(generatePoint());
	file->writeLine("}");

	file->close();
}
