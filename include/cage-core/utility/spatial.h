#ifndef guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578
#define guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578

namespace cage
{
	class CAGE_API spatialQueryClass
	{
	public:
		uint32 resultCount() const;
		const uint32 *resultArray() const;
		pointerRange<const uint32> result() const;

		void intersection(const line &shape);
		void intersection(const triangle &shape);
		void intersection(const sphere &shape);
		void intersection(const aabb &shape);
	};

	class CAGE_API spatialDataClass
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
		uintPtr memory;
		real gridResolutionCoefficient; // the higher the coefficient, the higher the resolution
		spatialDataCreateConfig();
	};

	CAGE_API holder<spatialDataClass> newSpatialData(const spatialDataCreateConfig &config);
	CAGE_API holder<spatialQueryClass> newSpatialQuery(const spatialDataClass *data);
}

#endif // guard_spatial_h_9A6D87AF6D4243E990D6E274B56CF578