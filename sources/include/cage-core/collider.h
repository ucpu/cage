#ifndef guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
#define guard_collider_h_qeqwdrwuegfoixwoihediuzerw456

#include "core.h"

namespace cage
{
	class CAGE_CORE_API Collider : private Immovable
	{
	public:
		void clear();
		MemoryBuffer serialize(bool includeAdditionalData = true) const;
		void deserialize(PointerRange<const char> buffer);
		void importPolyhedron(const Polyhedron *polyhedron);

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

	// ao and bo are the two meshes to test for collision
	// at1 and at2 are initial and final transformations for the first mesh
	// bt1 and bt2 are initial and final transformations for the second mesh
	// fractionBefore is the ratio between initial and final transformations just before the meshes start to collide
	// fractionContact is the ratio between initial and final transformations just when the meshes are colliding
	// note that fractionBefore and fractionContact are just approximations
	// the outputBuffer will contain all of the CollisionPairs corresponding to the fractionContact
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at, const transform &bt);
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at, const transform &bt, Holder<PointerRange<CollisionPair>> &outputBuffer);
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2);
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact);
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, Holder<PointerRange<CollisionPair>> &outputBuffer);
	CAGE_CORE_API bool collisionDetection(const Collider *ao, const Collider *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, Holder<PointerRange<CollisionPair>> &outputBuffer);

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
	inline        bool intersects(const Collider *ao, const Collider *bo, const transform &at, const transform &bt) { return collisionDetection(ao, bo, at, bt); }

	CAGE_CORE_API vec3 intersection(const line &shape, const Collider *collider, const transform &t);

	CAGE_CORE_API AssetScheme genAssetSchemeCollider();
	constexpr uint32 AssetSchemeIndexCollider = 3;
}

#endif // guard_collider_h_qeqwdrwuegfoixwoihediuzerw456
