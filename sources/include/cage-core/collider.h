#ifndef guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_collider_h_qeqwdrwuegfoixwoihediuzerw456

#include <cage-core/math.h>

namespace cage
{
	class Mesh;

	class CAGE_CORE_API Collider : private Immovable
	{
	public:
		void clear();
		Holder<Collider> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		void importMesh(const Mesh *mesh);

		Holder<PointerRange<char>> exportBuffer(bool includeAdditionalData = true) const;
		Holder<Mesh> exportMesh() const;

		PointerRange<const Triangle> triangles() const;

		void addTriangle(const Triangle &t);
		void addTriangles(PointerRange<const Triangle> tris);

		void optimize(); // removes duplicated triangles

		void rebuild();
		bool needsRebuild() const;

		const Aabb &box() const;
	};

	CAGE_CORE_API Holder<Collider> newCollider();

	struct CAGE_CORE_API CollisionPair
	{
		// triangle indices
		uint32 a = m;
		uint32 b = m;
	};

	// ao and bo are the two models to test for collision
	// at1 and at2 are initial and final transformations for the first model
	// bt1 and bt2 are initial and final transformations for the second model
	// fractionBefore is the ratio between initial and final transformations just before the models start to collide
	// fractionContact is the ratio between initial and final transformations just when the models are colliding
	// note that fractionBefore and fractionContact are just approximations
	// the collisionPairs will contain all of the CollisionPairs corresponding to the fractionContact
	struct CAGE_CORE_API CollisionDetectionConfig
	{
		CollisionDetectionConfig() = default;
		explicit CollisionDetectionConfig(const Collider *ao, const Collider *bo) : ao(ao), bo(bo) {}
		explicit CollisionDetectionConfig(const Collider *ao, const Collider *bo, const Transform &at, const Transform &bt) : ao(ao), bo(bo), at1(at), at2(at), bt1(bt), bt2(bt) {}

		// inputs
		const Collider *ao = nullptr;
		const Collider *bo = nullptr;
		Transform at1;
		Transform bt1;
		Transform at2;
		Transform bt2;

		// outputs
		Real fractionBefore = Real::Nan();
		Real fractionContact = Real::Nan();
		Holder<PointerRange<CollisionPair>> collisionPairs;
	};

	CAGE_CORE_API bool collisionDetection(CollisionDetectionConfig &params);

	CAGE_CORE_API Real distance(const Line &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Triangle &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Plane &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Sphere &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Aabb &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Cone &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Frustum &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Real distance(const Collider *ao, const Collider *bo, const Transform &at, const Transform &bt);

	CAGE_CORE_API bool intersects(const Line &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Triangle &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Plane &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Sphere &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Aabb &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Cone &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Frustum &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API bool intersects(const Collider *ao, const Collider *bo, const Transform &at, const Transform &bt);

	CAGE_CORE_API Vec3 intersection(const Line &shape, const Collider *collider, const Transform &t);
	CAGE_CORE_API Vec3 intersection(const Line &shape, const Collider *collider, const Transform &t, uint32 &triangleIndex);

	CAGE_CORE_API AssetScheme genAssetSchemeCollider();
	constexpr uint32 AssetSchemeIndexCollider = 3;
}

#endif // guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
