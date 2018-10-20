#ifndef guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_collider_h_qeqwdrwuegfoixwoihediuzerw456

namespace cage
{
	class CAGE_API colliderClass
	{
	public:
		uint32 trianglesCount() const;
		const triangle &triangleData(uint32 idx) const;
		const triangle *triangleData() const;
		pointerRange<const triangle> triangles() const;

		void addTriangle(const triangle &t);
		void addTriangles(const triangle *data, uint32 count);
		void clear();

		void rebuild();
		bool needsRebuild() const;

		const aabb &box() const;

		memoryBuffer serialize(bool includeAdditionalData = true) const;
		void deserialize(const memoryBuffer &buffer);
	};

	CAGE_API holder<colliderClass> newCollider();
	CAGE_API holder<colliderClass> newCollider(const memoryBuffer &buffer);

	struct CAGE_API collisionPairStruct
	{
		// triangle indices
		uint32 a;
		uint32 b;

		collisionPairStruct();
		collisionPairStruct(uint32 a, uint32 b);

		bool operator < (const collisionPairStruct &other) const;
	};

	CAGE_API bool collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at, const transform &bt);
	CAGE_API uint32 collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at, const transform &bt, collisionPairStruct *outputBuffer, uint32 bufferSize);
	CAGE_API bool collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2);
	CAGE_API bool collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact);
	CAGE_API uint32 collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, collisionPairStruct *outputBuffer, uint32 bufferSize);
	CAGE_API uint32 collisionDetection(const colliderClass *ao, const colliderClass *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, collisionPairStruct *outputBuffer, uint32 bufferSize);

	CAGE_API real distance(const line &shape, const colliderClass *collider, const transform &t);
	CAGE_API real distance(const triangle &shape, const colliderClass *collider, const transform &t);
	CAGE_API real distance(const plane &shape, const colliderClass *collider, const transform &t);
	CAGE_API real distance(const sphere &shape, const colliderClass *collider, const transform &t);
	CAGE_API real distance(const aabb &shape, const colliderClass *collider, const transform &t);
	CAGE_API real distance(const colliderClass *ao, const colliderClass *bo, const transform &at, const transform &bt);

	CAGE_API bool intersects(const line &shape, const colliderClass *collider, const transform &t);
	CAGE_API bool intersects(const triangle &shape, const colliderClass *collider, const transform &t);
	CAGE_API bool intersects(const plane &shape, const colliderClass *collider, const transform &t);
	CAGE_API bool intersects(const sphere &shape, const colliderClass *collider, const transform &t);
	CAGE_API bool intersects(const aabb &shape, const colliderClass *collider, const transform &t);
	inline   bool intersects(const colliderClass *ao, const colliderClass *bo, const transform &at, const transform &bt) { return collisionDetection(ao, bo, at, bt); }

	CAGE_API vec3 intersection(const line &shape, const colliderClass *collider, const transform &t);
}

#endif // guard_collider_h_qeqwdrwuegfoixwoihediuzerw456