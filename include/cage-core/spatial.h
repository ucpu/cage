#ifndef guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
#define guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578

namespace cage
{
	class CAGE_API SpatialQuery : private Immovable
	{
	public:
		PointerRange<uint32> result() const;

		void intersection(const vec3 &shape);
		void intersection(const line &shape);
		void intersection(const triangle &shape);
		void intersection(const plane &shape);
		void intersection(const sphere &shape);
		void intersection(const aabb &shape);
	};

	class CAGE_API SpatialData : private Immovable
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

	struct CAGE_API SpatialDataCreateConfig
	{
		uint32 maxItems;
		SpatialDataCreateConfig();
	};

	CAGE_API Holder<SpatialData> newSpatialData(const SpatialDataCreateConfig &config);
	CAGE_API Holder<SpatialQuery> newSpatialQuery(const SpatialData *data); // the SpatialData must outlive the query
}

#endif // guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
