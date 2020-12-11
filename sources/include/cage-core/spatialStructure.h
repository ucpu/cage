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
		bool intersection(const line &shape);
		bool intersection(const triangle &shape);
		bool intersection(const plane &shape);
		bool intersection(const sphere &shape);
		bool intersection(const aabb &shape);
	};

	class CAGE_CORE_API SpatialStructure : private Immovable
	{
	public:
		void update(uint32 name, const vec3 &other);
		void update(uint32 name, const line &other);
		void update(uint32 name, const triangle &other);
		void update(uint32 name, const sphere &other);
		void update(uint32 name, const aabb &other);
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
