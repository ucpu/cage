#include "main.h"
#include <cage-core/geometry.h>
#include <cage-core/mesh.h>
#include <cage-core/meshShapes.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/image.h>

namespace
{
	void approxEqual(const Vec3 &a, const Vec3 &b)
	{
		CAGE_TEST(distance(a, b) < 1);
	}

	void approxEqual(const Aabb &a, const Aabb &b)
	{
		approxEqual(a.a, b.a);
		approxEqual(a.b, b.b);
	}

	void approxEqual(const Sphere &a, const Sphere &b)
	{
		approxEqual(a.center, b.center);
		CAGE_TEST(abs(a.radius - b.radius) < 1);
	}

	Holder<Mesh> splitSphereIntoTwo(const Mesh *poly)
	{
		auto p = poly->copy();
		{
			MeshMergeCloseVerticesConfig cfg;
			cfg.distanceThreshold = 1e-3;
			meshMergeCloseVertices(+p, cfg);
		}
		for (Vec3 &v : p->positions())
		{
			Real &x = v[0];
			if (x > 2 && x < 4)
				x = Real::Nan();
		}
		meshDiscardInvalid(+p);
		return p;
	}

	struct MeshImage
	{
		Mesh *msh = nullptr;
		Image *img = nullptr;
	};

	void genTex(MeshImage *data, const Vec2i &xy, const Vec3i &ids, const Vec3 &weights)
	{
		const Vec3 position = data->msh->positionAt(ids, weights);
		const Vec3 normal = data->msh->normalAt(ids, weights);
		data->img->set(xy, abs(normal));
	}
}

void testMesh()
{
	CAGE_TESTCASE("mesh");

#ifdef CAGE_DEBUG
	const Holder<const Mesh> poly = newMeshSphereUv(10, 16, 8);
#else
	const Holder<const Mesh> poly = newMeshSphereUv(10, 32, 16);
#endif // CAGE_DEBUG

	poly->exportObjFile({}, "meshes/base.obj");
	CAGE_TEST(poly->verticesCount() > 10);
	CAGE_TEST(poly->indicesCount() > 10);
	CAGE_TEST(poly->indicesCount() == poly->facesCount() * 3);

	{
		CAGE_TESTCASE("bounding box");
		approxEqual(poly->boundingBox(), Aabb(Vec3(-10), Vec3(10)));
	}

	{
		CAGE_TESTCASE("bounding sphere");
		approxEqual(poly->boundingSphere(), Sphere(Vec3(), 10));
	}

	{
		CAGE_TESTCASE("apply transformation");
		auto p = poly->copy();
		meshApplyTransform(+p, Transform(Vec3(0, 5, 0)));
		approxEqual(p->boundingBox(), Aabb(Vec3(-10, -5, -10), Vec3(10, 15, 10)));
	}

	{
		CAGE_TESTCASE("discard invalid");
		auto p = poly->copy();
		p->position(42, Vec3::Nan()); // intentionally corrupt one vertex
		meshDiscardInvalid(+p);
		const uint32 f = p->facesCount();
		CAGE_TEST(f > 10 && f < poly->facesCount());
		p->exportObjFile({}, "meshes/discardInvalid.obj");
	}

	{
		CAGE_TESTCASE("merge close vertices");
		auto p = poly->copy();
		{
			MeshMergeCloseVerticesConfig cfg;
			cfg.distanceThreshold = 1e-3;
			meshMergeCloseVertices(+p, cfg);
		}
		const uint32 f = p->facesCount();
		CAGE_TEST(f > 10 && f < poly->facesCount());
		p->exportObjFile({}, "meshes/mergeCloseVertices.obj");
	}

	{
		CAGE_TESTCASE("simplify");
		auto p = poly->copy();
		MeshSimplifyConfig cfg;
#ifdef CAGE_DEBUG
		cfg.iterations = 1;
		cfg.minEdgeLength = 0.5;
		cfg.maxEdgeLength = 2;
		cfg.approximateError = 0.5;
#endif
		meshSimplify(+p, cfg);
		p->exportObjFile({}, "meshes/simplify.obj");
	}

	{
		CAGE_TESTCASE("regularize");
		auto p = poly->copy();
		MeshRegularizeConfig cfg;
#ifdef CAGE_DEBUG
		cfg.iterations = 1;
		cfg.targetEdgeLength = 3;
#endif
		meshRegularize(+p, cfg);
		p->exportObjFile({}, "meshes/regularize.obj");
	}

	{
		CAGE_TESTCASE("chunking");
		auto p = poly->copy();
		MeshChunkingConfig cfg;
		constexpr Real initialAreaImplicit = 4 * Real::Pi() * sqr(10);
		constexpr Real targetChunks = 10;
		cfg.maxSurfaceArea = initialAreaImplicit / targetChunks;
		const auto res = meshChunking(+p, cfg);
		CAGE_TEST(res.size() > targetChunks / 2 && res.size() < targetChunks * 2);
		for (const auto &it : res)
		{
			CAGE_TEST(it->facesCount() > 0);
			CAGE_TEST(it->indicesCount() > 0);
			CAGE_TEST(it->type() == MeshTypeEnum::Triangles);
		}
	}

	{
		CAGE_TESTCASE("unwrap");
		auto p = poly->copy();
		uint32 res = 0;
		{
			MeshUnwrapConfig cfg;
			cfg.targetResolution = 256;
			res = meshUnwrap(+p, cfg);
			p->exportObjFile({}, "meshes/unwrap.obj");
		}
		{
			CAGE_TESTCASE("texturing");
			Holder<Image> img = newImage();
			img->initialize(Vec2i(res), 3);
			MeshImage data;
			data.msh = +p;
			data.img = +img;
			MeshGenerateTextureConfig cfg;
			cfg.width = cfg.height = res;
			cfg.generator.bind<MeshImage*, &genTex>(&data);
			meshGenerateTexture(+p, cfg);
			img->exportFile("meshes/texture.png");
		}
	}

	{
		CAGE_TESTCASE("clip");
		auto p = poly->copy();
		meshClip(+p, Aabb(Vec3(-6, -6, -10), Vec3(6, 6, 10)));
		p->exportObjFile({}, "meshes/clip.obj");
	}

	{
		CAGE_TESTCASE("separateDisconnected");
		auto p = splitSphereIntoTwo(+poly);
		auto ps = meshSeparateDisconnected(+p);
		// CAGE_TEST(ps.size() == 2); // todo fix this -> it should really be 2 but is 3
		ps[0]->exportObjFile({}, "meshes/separateDisconnected_1.obj");
		ps[1]->exportObjFile({}, "meshes/separateDisconnected_2.obj");
	}

	{
		CAGE_TESTCASE("discardDisconnected");
		auto p = splitSphereIntoTwo(+poly);
		meshDiscardDisconnected(+p);
		p->exportObjFile({}, "meshes/discardDisconnected.obj");
	}

	{
		CAGE_TESTCASE("serialize");
		Holder<PointerRange<char>> buff = poly->exportBuffer();
		CAGE_TEST(buff.size() > 10);
		Holder<Mesh> p = newMesh();
		p->importBuffer(buff);
		CAGE_TEST(p->verticesCount() == poly->verticesCount());
		CAGE_TEST(p->indicesCount() == poly->indicesCount());
		CAGE_TEST(p->positions().size() == poly->positions().size());
		CAGE_TEST(p->normals().size() == poly->normals().size());
		CAGE_TEST(p->uvs().size() == poly->uvs().size());
		CAGE_TEST(p->uvs3().size() == poly->uvs3().size());
		CAGE_TEST(p->tangents().size() == poly->tangents().size());
		CAGE_TEST(p->boneIndices().size() == poly->boneIndices().size());
	}

	{
		CAGE_TESTCASE("collider");
		Holder<Collider> c = newCollider();
		c->importMesh(+poly);
		CAGE_TEST(c->triangles().size() > 10);
		Holder<Mesh> p = newMesh();
		p->importCollider(c.get());
		CAGE_TEST(p->positions().size() > 10);
		CAGE_TEST(p->facesCount() == poly->facesCount());
	}

	{
		CAGE_TESTCASE("shapes");
		newMeshSphereUv(10, 20, 30)->exportObjFile({}, "meshes/shapes/uv-sphere.obj");
		newMeshIcosahedron(10)->exportObjFile({}, "meshes/shapes/icosahedron.obj");
		newMeshSphereRegular(10, 1.5)->exportObjFile({}, "meshes/shapes/regular-sphere.obj");
	}
}
