#include <cage-core/typeRegistry.h>
#include <cage-core/enumerate.h>
#include <cage-core/macros.h>

#include <vector>

namespace cage
{
	namespace privat
	{
		uint32 typeRegistryInit(void *ptr)
		{
			struct Data
			{
				std::vector<void *> values;

				Data()
				{
					values.reserve(100);
				}

				uint32 val(void *ptr)
				{
					for (auto it : cage::enumerate(values))
						if (*it == ptr)
							return numeric_cast<uint32>(it.index);
					values.push_back(ptr);
					return numeric_cast<uint32>(values.size() - 1);
				}
			};

			static Data data;
			return data.val(ptr);
		}
	}

	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_REGISTER_TYPE, uint8, uint16, uint32, uint64, sint8, sint16, sint32, sint64, float, double));
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_REGISTER_TYPE, string, stringizer, const char *, char*, void*));
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_REGISTER_TYPE, real, degs, rads, vec2, vec3, vec4, ivec2, ivec3, ivec4, quat, mat3, mat4, transform));
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_REGISTER_TYPE, Line, Triangle, Plane, Sphere, Aabb, Cone, Frustum));
}
