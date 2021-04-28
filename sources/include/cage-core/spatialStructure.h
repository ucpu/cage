#ifndef guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
#define guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578

#include "core.h"

namespace cage
{
	class CAGE_CORE_API SpatialQuery : private Immovable
	{
	public:
		PointerRange<uint32> result() const;

		bool intersection(const vec3 &shape);
		bool intersection(const Line &shape);
		bool intersection(const Triangle &shape);
		bool intersection(const Plane &shape);
		bool intersection(const Sphere &shape);
		bool intersection(const Aabb &shape);
		bool intersection(const Cone &shape);
		bool intersection(const Frustum &shape);
	};

	class CAGE_CORE_API SpatialStructure : private Immovable
	{
	public:
		void update(uint32 name, const vec3 &other);
		void update(uint32 name, const Line &other);
		void update(uint32 name, const Triangle &other);
		void update(uint32 name, const Sphere &other);
		void update(uint32 name, const Aabb &other);
		void update(uint32 name, const Cone &other);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_CORE_API SpatialStructureCreateConfig
	{};

	CAGE_CORE_API Holder<SpatialStructure> newSpatialStructure(const SpatialStructureCreateConfig &config);
	CAGE_CORE_API Holder<SpatialQuery> newSpatialQuery(const SpatialStructure *data); // the SpatialStructure must outlive the query
}

#endif // guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
