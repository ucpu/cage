#ifndef guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A
#define guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A

#include "core.h"

namespace cage
{
	// aka a mesh but stored in cpu memory
	class CAGE_CORE_API Polyhedron : private Immovable
	{
	public:
	};

	CAGE_CORE_API Holder<Polyhedron> newPolyhedron();
}

#endif // guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A
