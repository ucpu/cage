#ifndef guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_collider_h_qeqwdrwuegfoixwoihediuzerw456

#include "math.h"

namespace cage
{
	class CAGE_CORE_API Collider : private Immovable
	{
	public:
		void clear();
		Holder<PointerRange<char>> serialize(bool includeAdditionalData = true) const;
		void deserialize(PointerRange<const char> buffer);
		void importMesh(const Mesh *mesh);

		PointerRange<const triangle> triangles() const;

		void addTriangle(const triangle &t);
		void addTriangles(PointerRange<const triangle> tris);

		void rebuild();
		bool needsRebuild() const;

		const aabb &box() const;
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
	struct CAGE_CORE_API CollisionDetectionParams
	{
		CollisionDetectionParams() = default;
		explicit CollisionDetectionParams(const Collider *ao, const Collider *bo) : ao(ao), bo(bo) {}
		explicit CollisionDetectionParams(const Collider *ao, const Collider *bo, const transform &at, const transform &bt) : ao(ao), bo(bo), at1(at), at2(at), bt1(bt), bt2(bt) {}

		// inputs
		const Collider *ao = nullptr;
		const Collider *bo = nullptr;
		transform at1;
		transform bt1;
		transform at2;
		transform bt2;

		// outputs
		real fractionBefore = real::Nan();
		real fractionContact = real::Nan();
		Holder<PointerRange<CollisionPair>> collisionPairs;
	};

	CAGE_CORE_API bool collisionDetection(CollisionDetectionParams &params);

	CAGE_CORE_API real distance(const line &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API real distance(const triangle &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API real distance(const plane &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API real distance(const sphere &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API real distance(const aabb &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API real distance(const Collider *ao, const Collider *bo, const transform &at, const transform &bt);

	CAGE_CORE_API bool intersects(const line &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API bool intersects(const triangle &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API bool intersects(const plane &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API bool intersects(const sphere &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API bool intersects(const aabb &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API bool intersects(const Collider *ao, const Collider *bo, const transform &at, const transform &bt);

	CAGE_CORE_API vec3 intersection(const line &shape, const Collider *collider, const transform &t);
	CAGE_CORE_API vec3 intersection(const line &shape, const Collider *collider, const transform &t, uint32 &triangleIndex);

	CAGE_CORE_API AssetScheme genAssetSchemeCollider();
	constexpr uint32 AssetSchemeIndexCollider = 3;
}

#endif // guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
