#ifndef guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B
#define guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B

#include <glad/glad.h>

namespace cage
{
	namespace detail
	{
		inline void initializeOpengl()
		{
			gladLoadGL();
		}
	}
}

#endif // guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B
