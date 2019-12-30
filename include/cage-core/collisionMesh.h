#ifndef guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456

namespace cage
{
	class CAGE_API CollisionMesh : private Immovable
	{
	public:
		PointerRange<const triangle> triangles() const;

		void addTriangle(const triangle &t);
		void addTriangles(const triangle *data, uint32 count);
		void addTriangles(const PointerRange<const triangle> &tris);
		void clear();

		void rebuild();
		bool needsRebuild() const;

		const aabb &box() const;

		MemoryBuffer serialize(bool includeAdditionalData = true) const;
		void deserialize(const MemoryBuffer &buffer);
	};

	CAGE_API Holder<CollisionMesh> newCollisionMesh();
	CAGE_API Holder<CollisionMesh> newCollisionMesh(const MemoryBuffer &buffer);

	struct CAGE_API CollisionPair
	{
		// triangle indices
		uint32 a;
		uint32 b;

		CollisionPair();
		CollisionPair(uint32 a, uint32 b);

		bool operator < (const CollisionPair &other) const;
	};

	CAGE_API bool collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at, const transform &bt);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at, const transform &bt, CollisionPair *outputBuffer, uint32 bufferSize);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at, const transform &bt, Holder<PointerRange<CollisionPair>> &outputBuffer);
	CAGE_API bool collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2);
	CAGE_API bool collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, CollisionPair *outputBuffer, uint32 bufferSize);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, Holder<PointerRange<CollisionPair>> &outputBuffer);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, CollisionPair *outputBuffer, uint32 bufferSize);
	CAGE_API uint32 collisionDetection(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, Holder<PointerRange<CollisionPair>> &outputBuffer);

	CAGE_API real distance(const line &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API real distance(const triangle &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API real distance(const plane &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API real distance(const sphere &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API real distance(const aabb &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API real distance(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at, const transform &bt);

	CAGE_API bool intersects(const line &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const triangle &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const plane &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const sphere &shape, const CollisionMesh *collider, const transform &t);
	CAGE_API bool intersects(const aabb &shape, const CollisionMesh *collider, const transform &t);
	inline   bool intersects(const CollisionMesh *ao, const CollisionMesh *bo, const transform &at, const transform &bt) { return collisionDetection(ao, bo, at, bt); }

	CAGE_API vec3 intersection(const line &shape, const CollisionMesh *collider, const transform &t);

	CAGE_API AssetScheme genAssetSchemeCollisionMesh(const uint32 threadIndex);
	static const uint32 assetSchemeIndexCollisionMesh = 3;
}

#endif // guard_colliderMesh_h_qeqwdrwuegfoixwoihediuzerw456
