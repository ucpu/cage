#ifndef guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
#define guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578

namespace cage
{
	class CAGE_API spatialQuery : private immovable
	{
	public:
		uint32 resultCount() const;
		const uint32 *resultData() const;
		pointerRange<const uint32> result() const;

		void intersection(const line &shape);
		void intersection(const triangle &shape);
		void intersection(const plane &shape);
		void intersection(const sphere &shape);
		void intersection(const aabb &shape);
	};

	class CAGE_API spatialData : private immovable
	{
	public:
		void update(uint32 name, const line &other);
		void update(uint32 name, const triangle &other);
		void update(uint32 name, const sphere &other);
		void update(uint32 name, const aabb &other);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_API spatialDataCreateConfig
	{
		uint32 maxItems;
		spatialDataCreateConfig();
	};

	CAGE_API holder<spatialData> newSpatialData(const spatialDataCreateConfig &config);
	CAGE_API holder<spatialQuery> newSpatialQuery(const spatialData *data);
}

#endif // guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
