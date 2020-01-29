#include <cage-core/variableSmoothingBuffer.h>

namespace cage
{
	namespace privat
	{
		quat averageQuaternions(const quat *quaternions, uint32 count)
		{
			vec3 f, u;
			for (uint32 i = 0; i < count; i++)
			{
				f += quaternions[i] * vec3(0, 0, -1);
				u += quaternions[i] * vec3(0, 1, 0);
			}
			return quat(f, u);
		}
	}
}

