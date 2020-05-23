#ifndef guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A
#define guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A

#include "math.h"

namespace cage
{
	// aka a mesh but stored in cpu memory
	class CAGE_CORE_API Polyhedron : private Immovable
	{
	public:
		uint32 vertices() const;
		PointerRange<const vec3> positions() const;
		PointerRange<const vec3> normals() const;
		PointerRange<const vec2> uvs() const;
		PointerRange<vec3> positions();
		PointerRange<vec3> normals();
		PointerRange<vec2> uvs();
	};

	CAGE_CORE_API Holder<Polyhedron> newPolyhedron();
}

#endif // guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A
