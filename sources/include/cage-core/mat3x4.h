#ifndef header_mat3x4_h_ij4sh9bsd
#define header_mat3x4_h_ij4sh9bsd

#include <cage-core/math.h>

namespace cage
{
	struct CAGE_CORE_API Mat3x4
	{
		Vec4 data[3];

		Mat3x4();
		explicit Mat3x4(Mat3 in);
		explicit Mat3x4(Mat4 in);

		explicit operator Mat4() const;
	};
}

#endif
