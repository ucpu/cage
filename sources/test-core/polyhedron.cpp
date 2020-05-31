#include "main.h"
#include <cage-core/geometry.h>
#include <cage-core/polyhedron.h>
#include <cage-core/collider.h>

namespace
{
	Holder<Polyhedron> makeUvSphere(real radius, uint32 segments, uint32 rings)
	{
		Holder<Polyhedron> poly = newPolyhedron();

		const vec2 uvScale = 1 / vec2(segments - 1, rings - 1);
		for (uint32 r = 0; r < rings; r++)
		{
			rads b = degs(180 * real(r) / (rings - 1) - 90);
			for (uint32 s = 0; s < segments; s++)
			{
				rads a = degs(360 * real(s) / (segments - 1));
				vec3 pos = vec3(cos(a) * cos(b), sin(a) * cos(b), sin(b));
				CAGE_ASSERT(abs(distance(pos, normalize(pos))) < 1e-4);
				poly->addVertex(pos * radius, pos, vec2(s, r) * uvScale);
			}
		}

		for (uint32 r = 0; r < rings - 1; r++)
		{
			for (uint32 s = 0; s < segments - 1; s++)
			{
				poly->addTriangle(r * segments + s, (r + 1) * segments + s + 1, (r + 1) * segments + s);
				poly->addTriangle(r * segments + s, r * segments + s + 1, (r + 1) * segments + s + 1);
			}
		}

		return poly;
	}
}

void testPolyhedron()
{
	CAGE_TESTCASE("polyhedron");

#ifdef CAGE_DEBUG
	Holder<Polyhedron> poly = makeUvSphere(10, 16, 8);
#else
	Holder<Polyhedron> poly = makeUvSphere(10, 32, 16);
#endif // CAGE_DEBUG

	poly->exportToObjFile({}, "meshes/sphere_base.obj");
	{
		CAGE_TESTCASE("simplify");
		auto p = poly->copy();
		PolyhedronSimplificationConfig cfg;
#ifdef CAGE_DEBUG
		cfg.iterations = 1;
		cfg.minEdgeLength = 0.5;
		cfg.maxEdgeLength = 2;
		cfg.approximateError = 0.5;
#endif
		p->simplify(cfg);
		p->exportToObjFile({}, "meshes/sphere_simplified.obj");
	}
	{
		CAGE_TESTCASE("regularize");
		auto p = poly->copy();
		PolyhedronRegularizationConfig cfg;
#ifdef CAGE_DEBUG
		cfg.iterations = 1;
		cfg.targetEdgeLength = 3;
#endif
		p->regularize(cfg);
		p->exportToObjFile({}, "meshes/sphere_regularized.obj");
	}
	{
		CAGE_TESTCASE("unwrap");
		auto p = poly->copy();
		PolyhedronUnwrapConfig cfg;
		cfg.targetResolution = 256;
		p->unwrap(cfg);
		p->exportToObjFile({}, "meshes/sphere_unwrap.obj");
	}
	{
		CAGE_TESTCASE("clip");
		auto p = poly->copy();
		p->clip(aabb(vec3(-6, -6, -10), vec3(6, 6, 10)));
		p->exportToObjFile({}, "meshes/sphere_clipped.obj");
	}
	{
		CAGE_TESTCASE("collider");
		auto c = poly->createCollider();
		auto p = c->createPolyhedron();
		CAGE_TEST(p->positions().size() > 10);
	}
}
