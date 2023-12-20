#ifndef guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
#define guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578

#include <cage-core/core.h>

namespace cage
{
	class CAGE_CORE_API SpatialQuery : private Immovable
	{
	public:
		PointerRange<uint32> result() const;

		bool intersection(Vec3 shape);
		bool intersection(Line shape);
		bool intersection(Triangle shape);
		bool intersection(Plane shape);
		bool intersection(Sphere shape);
		bool intersection(Aabb shape);
		bool intersection(Cone shape);
		bool intersection(const Frustum &shape);
	};

	class CAGE_CORE_API SpatialStructure : private Immovable
	{
	public:
		void update(uint32 name, Vec3 other);
		void update(uint32 name, Line other);
		void update(uint32 name, Triangle other);
		void update(uint32 name, Sphere other);
		void update(uint32 name, Aabb other);
		void update(uint32 name, Cone other);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_CORE_API SpatialStructureCreateConfig
	{
		uint32 reserve = 0;
	};

	CAGE_CORE_API Holder<SpatialStructure> newSpatialStructure(const SpatialStructureCreateConfig &config);
	CAGE_CORE_API Holder<SpatialQuery> newSpatialQuery(Holder<const SpatialStructure> data);
}

#endif // guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
