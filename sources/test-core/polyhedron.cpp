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

	void approxEqual(const vec3 &a, const vec3 &b)
	{
		CAGE_TEST(distance(a, b) < 1);
	}

	void approxEqual(const aabb &a, const aabb &b)
	{
		approxEqual(a.a, b.a);
		approxEqual(a.b, b.b);
	}
}

void testPolyhedron()
{
	CAGE_TESTCASE("polyhedron");

#ifdef CAGE_DEBUG
	const Holder<const Polyhedron> poly = makeUvSphere(10, 16, 8).cast<const Polyhedron>();
#else
	const Holder<const Polyhedron> poly = makeUvSphere(10, 32, 16).cast<const Polyhedron>();
#endif // CAGE_DEBUG

	poly->exportObjFile({}, "meshes/sphere_base.obj");
	CAGE_TEST(poly->verticesCount() > 10);
	CAGE_TEST(poly->indicesCount() > 10);
	CAGE_TEST(poly->indicesCount() == poly->facesCount() * 3);

	{
		CAGE_TESTCASE("bounding box");
		approxEqual(poly->boundingBox(), aabb(vec3(-10), vec3(10)));
	}

	{
		CAGE_TESTCASE("apply transformation");
		auto p = poly->copy();
		p->applyTransform(transform(vec3(0, 5, 0)));
		approxEqual(p->boundingBox(), aabb(vec3(-10, -5, -10), vec3(10, 15, 10)));
	}

	{
		CAGE_TESTCASE("discard invalid");
		auto p = poly->copy();
		p->position(42, vec3::Nan()); // intentionally corrupt one vertex
		p->discardInvalid();
		const uint32 f = p->facesCount();
		CAGE_TEST(f > 10 && f < poly->facesCount());
		p->exportObjFile({}, "meshes/sphere_discardInvalid.obj");
	}

	{
		CAGE_TESTCASE("merge close vertices");
		auto p = poly->copy();
		p->mergeCloseVertices(1e-3);
		const uint32 f = p->facesCount();
		CAGE_TEST(f > 10 && f < poly->facesCount());
		p->exportObjFile({}, "meshes/sphere_mergeCloseVertices.obj");
	}

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
		p->exportObjFile({}, "meshes/sphere_simplify.obj");
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
		p->exportObjFile({}, "meshes/sphere_regularize.obj");
	}

	{
		CAGE_TESTCASE("unwrap");
		auto p = poly->copy();
		PolyhedronUnwrapConfig cfg;
		cfg.targetResolution = 256;
		p->unwrap(cfg);
		p->exportObjFile({}, "meshes/sphere_unwrap.obj");
	}

	{
		CAGE_TESTCASE("clip");
		auto p = poly->copy();
		p->clip(aabb(vec3(-6, -6, -10), vec3(6, 6, 10)));
		p->exportObjFile({}, "meshes/sphere_clip.obj");
	}

	{
		CAGE_TESTCASE("collider");
		Holder<Collider> c = newCollider();
		c->importPolyhedron(poly.get());
		CAGE_TEST(c->triangles().size() > 10);
		Holder<Polyhedron> p = newPolyhedron();
		p->importCollider(c.get());
		CAGE_TEST(p->positions().size() > 10);
		CAGE_TEST(p->facesCount() == poly->facesCount());
	}
}
