#ifndef guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456

namespace cage
{
	class CAGE_API collisionMesh : private immovable
	{
	public:
		uint32 trianglesCount() const;
		const triangle *trianglesData() const;
		const triangle &triangleData(uint32 idx) const;
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

	CAGE_API holder<collisionMesh> newCollisionMesh();
	CAGE_API holder<collisionMesh> newCollisionMesh(const memoryBuffer &buffer);

	struct CAGE_API collisionPair
	{
		// triangle indices
		uint32 a;
		uint32 b;

		collisionPair();
		collisionPair(uint32 a, uint32 b);

		bool operator < (const collisionPair &other) const;
	};

	CAGE_API bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt);
	CAGE_API uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt, collisionPair *outputBuffer, uint32 bufferSize);
	CAGE_API bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2);
	CAGE_API bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact);
	CAGE_API uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, collisionPair *outputBuffer, uint32 bufferSize);
	CAGE_API uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, collisionPair *outputBuffer, uint32 bufferSize);

	CAGE_API real distance(const line &shape, const collisionMesh *collider, const transform &t);
	CAGE_API real distance(const triangle &shape, const collisionMesh *collider, const transform &t);
	CAGE_API real distance(const plane &shape, const collisionMesh *collider, const transform &t);
	CAGE_API real distance(const sphere &shape, const collisionMesh *collider, const transform &t);
	CAGE_API real distance(const aabb &shape, const collisionMesh *collider, const transform &t);
	CAGE_API real distance(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt);

	CAGE_API bool intersects(const line &shape, const collisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const triangle &shape, const collisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const plane &shape, const collisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const sphere &shape, const collisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const aabb &shape, const collisionMesh *collider, const transform &t);
	inline   bool intersects(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt) { return collisionDetection(ao, bo, at, bt); }

	CAGE_API vec3 intersection(const line &shape, const collisionMesh *collider, const transform &t);

	CAGE_API assetScheme genAssetSchemeCollisionMesh(const uint32 threadIndex);
	static const uint32 assetSchemeIndexCollisionMesh = 3;
}

#endif // guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456
