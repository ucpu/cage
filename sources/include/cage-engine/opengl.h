#ifndef guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B
#define guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B

#include "core.h"
#define GLAPI CAGE_ENGINE_API extern
#include <glad/glad.h>

namespace cage
{
	namespace detail
	{
		CAGE_ENGINE_API void initializeOpengl();
	}
}

#endif // guard_opengl_h_DB5E1C2183FA4991AF766F8ABEFCE93B
