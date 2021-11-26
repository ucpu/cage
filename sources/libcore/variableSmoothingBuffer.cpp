#include <cage-core/variableSmoothingBuffer.h>

namespace cage
{
	namespace privat
	{
		Quat averageQuaternions(PointerRange<const Quat> quaternions)
		{
			Vec3 f, u;
			for (const Quat &q : quaternions)
			{
				f += q * Vec3(0, 0, -1);
				u += q * Vec3(0, 1, 0);
			}
			return Quat(f, u);
		}
	}
}

