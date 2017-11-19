#ifndef guard_collision_h_erthg456ter4h56r1th64rth
#define guard_collision_h_erthg456ter4h56r1th64rth

namespace cage
{
	class CAGE_API collisionQueryClass
	{
	public:
		uint32 name() const;
		real fractionBefore() const;
		real fractionContact() const;
		uint32 collisionPairsCount() const;
		const collisionPairStruct *collisionPairsData() const;
		pointerRange<const collisionPairStruct> collisionPairs() const;

		void query(const colliderClass *collider, const transform &t);
		void query(const colliderClass *collider, const transform &t1, const transform &t2);
	};

	class CAGE_API collisionDataClass
	{
	public:
		void update(uint32 name, const colliderClass *collider, const transform &t);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_API collisionDataCreateConfig
	{
		spatialDataCreateConfig *spatialConfig;
		collisionDataCreateConfig();
	};

	CAGE_API holder<collisionDataClass> newCollisionData(const collisionDataCreateConfig &config);
	CAGE_API holder<collisionQueryClass> newCollisionQuery(const collisionDataClass *data);
}

#endif // guard_collision_h_erthg456ter4h56r1th64rth