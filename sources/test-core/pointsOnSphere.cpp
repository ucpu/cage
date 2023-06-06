#include "main.h"

#include <cage-core/files.h>
#include <cage-core/math.h>

namespace
{
	String generatePoint()
	{
		Vec3 p = randomDirection3();
		return Stringizer() + "vec3(" + p[0] + ", " + p[1] + ", " + p[2] + ")";
	}
}

void generatePointsOnSphere()
{
	CAGE_TESTCASE("generatePointsOnSphere");

	Holder<File> file = writeFile("pointsOnSphere.txt");

	file->writeLine("vec3 pointsOnSphere[] =");
	file->writeLine("{");
	for (uint32 i = 0; i < 255; i++)
		file->writeLine(generatePoint() + ",");
	file->writeLine(generatePoint());
	file->writeLine("}");

	file->close();
}
