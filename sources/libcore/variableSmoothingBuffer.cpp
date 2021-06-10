#include <cage-core/variableSmoothingBuffer.h>

namespace cage
{
	namespace privat
	{
		quat averageQuaternions(PointerRange<const quat> quaternions)
		{
			vec3 f, u;
			for (const quat &q : quaternions)
			{
				f += q * vec3(0, 0, -1);
				u += q * vec3(0, 1, 0);
			}
			return quat(f, u);
		}
	}
}

