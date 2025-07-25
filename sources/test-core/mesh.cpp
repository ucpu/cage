#include <algorithm>
#include <array>
#include <vector>

#include "main.h"

#include <cage-core/collider.h>
#include <cage-core/files.h>
#include <cage-core/geometry.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/meshExport.h>
#include <cage-core/meshImport.h>
#include <cage-core/meshShapes.h>

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

	Holder<Mesh> splitSphereIntoTwo(const Mesh *msh)
	{
		auto p = msh->copy();
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
		meshRemoveInvalid(+p);
		return p;
	}

	void unwrapThetaPhi(Mesh *msh)
	{
		std::vector<Vec2> uv;
		uv.reserve(msh->verticesCount());
		for (Vec3 p : msh->positions())
		{
			p = normalize(p);
			Rads theta = acos(p[2]);
			Rads phi = atan2(p[1], p[0]);
			Real u = theta.value / Real::Pi();
			Real v = (phi.value / Real::Pi()) * 0.5 + 0.5;
			uv.push_back(saturate(Vec2(u, v)));
		}
		msh->uvs(uv);
	}

	void testUnwrapThetaPhi(const Mesh *msh)
	{
		PointerRange<const Vec2> uvs = msh->uvs();
		CAGE_TEST(uvs.size() == msh->positions().size());
		uint32 index = 0;
		for (Vec3 p : msh->positions())
		{
			p = normalize(p);
			Rads theta = acos(p[2]);
			Rads phi = atan2(p[1], p[0]);
			Real u = theta.value / Real::Pi();
			Real v = (phi.value / Real::Pi()) * 0.5 + 0.5;
			Vec2 expected = Vec2(u, v);
			Vec2 loaded = uvs[index++];
			CAGE_TEST(distance(expected, loaded) < 0.01);
		}
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

	Holder<Mesh> makeSphere()
	{
#ifdef CAGE_DEBUG
		return newMeshSphereUv(10, 16, 8);
#else
		return newMeshSphereUv(10, 32, 16);
#endif // CAGE_DEBUG
	}

	// generates sphere with invalid/degenerated triangles
	Holder<Mesh> makeSphereRaw()
	{
		constexpr Real radius = 10;
		constexpr uint32 segments = 16;
		constexpr uint32 rings = 8;
		Holder<Mesh> mesh = newMesh();
		const Vec2 uvScale = 1 / Vec2(segments - 1, rings - 1);
		for (uint32 r = 0; r < rings; r++)
		{
			const Rads b = Degs(180 * Real(r) / (rings - 1) - 90);
			for (uint32 s = 0; s < segments; s++)
			{
				const Rads a = Degs(360 * Real(s) / (segments - 1));
				const Real cosb = cos(b);
				const Vec3 pos = Vec3(cos(a) * cosb, sin(a) * cosb, sin(b));
				CAGE_ASSERT(abs(distance(pos, normalize(pos))) < 1e-4);
				mesh->addVertex(pos * radius, pos, Vec2(s, r) * uvScale);
			}
		}
		for (uint32 r = 0; r < rings - 1; r++)
		{
			for (uint32 s = 0; s < segments - 1; s++)
			{
				mesh->addTriangle(r * segments + s, (r + 1) * segments + s + 1, (r + 1) * segments + s);
				mesh->addTriangle(r * segments + s, r * segments + s + 1, (r + 1) * segments + s + 1);
			}
		}
		return mesh;
	}

	Holder<Mesh> makeDoubleBalls()
	{
		auto p = makeSphere();
		auto k = makeSphere();
		meshApplyTransform(+k, Transform(Vec3(5, 0, 0)));
		const uint32 incnt = k->indicesCount();
		const auto ins = k->indices();
		const auto pos = k->positions();
		for (uint32 i = 0; i < incnt; i += 3)
			p->addTriangle(Triangle(pos[ins[i + 0]], pos[ins[i + 1]], pos[ins[i + 2]]));
		return p;
	}

	Holder<Mesh> makeComplexCube()
	{
		static constexpr const Vec3 verts[] = { Vec3(-1.000000, -1.000000, 1.000000), Vec3(-1.000000, 1.000000, 1.000000), Vec3(-1.000000, -1.000000, -1.000000), Vec3(-1.000000, 1.000000, -1.000000), Vec3(1.000000, -1.000000, 1.000000), Vec3(-0.033372, -1.000000, -0.186082), Vec3(1.000000, -1.000000, -1.000000), Vec3(0.000000, -1.000000, 0.500000), Vec3(-1.000000, -1.000000, -0.333333), Vec3(-1.000000, -1.000000, 0.333333), Vec3(-1.000000, 1.000000, 0.333333), Vec3(-1.000000, 1.000000, -0.333333), Vec3(1.000000, -1.000000, 0.000000), Vec3(1.000000, 1.000000, -1.000000), Vec3(1.000000, 1.000000, 1.000000), Vec3(-1.000000, 0.000000, 1.000000), Vec3(-1.000000, 0.000000, -1.000000), Vec3(1.000000, 0.000000, -1.000000), Vec3(1.000000, 0.000000, 1.000000), Vec3(1.000000, 0.000000, -0.333333), Vec3(1.000000, 0.000000, 0.333333), Vec3(-1.000000, 0.000000, 0.000000), Vec3(1.000000, -1.000000, 0.500000), Vec3(0.000000, -1.000000, 0.166667), Vec3(-0.500000, -1.000000, -0.250000),
			Vec3(1.000000, -1.000000, -0.500000), Vec3(-0.500000, -1.000000, 0.750000), Vec3(-1.000000, -1.000000, 0.000000), Vec3(0.500000, -1.000000, -0.083333), Vec3(0.000000, -1.000000, -0.666667), Vec3(0.250000, -1.000000, -0.375000), Vec3(-0.250000, -1.000000, -0.041667) };

		static constexpr const uint32 faces[] = { 12, 17, 22, 4, 18, 17, 21, 15, 19, 15, 16, 19, 27, 10, 24, 15, 11, 2, 14, 4, 12, 12, 15, 14, 30, 3, 7, 29, 24, 32, 18, 14, 20, 14, 21, 20, 16, 11, 22, 22, 11, 12, 16, 10, 1, 13, 20, 21, 13, 7, 18, 19, 1, 5, 5, 13, 21, 3, 18, 7, 9, 17, 3, 12, 11, 15, 12, 4, 17, 16, 2, 11, 16, 22, 10, 9, 22, 17, 14, 15, 21, 21, 19, 5, 18, 20, 13, 15, 2, 16, 19, 16, 1, 4, 14, 18, 3, 17, 18, 31, 6, 25, 13, 23, 8, 5, 27, 23, 30, 25, 9, 24, 13, 27, 28, 32, 10, 7, 31, 30, 26, 29, 31, 6, 32, 25, 5, 1, 27, 27, 1, 10, 32, 24, 10, 25, 32, 28, 30, 31, 25, 7, 26, 31, 26, 13, 29, 29, 13, 24, 30, 9, 3, 10, 22, 9, 13, 26, 7, 25, 28, 9, 31, 29, 32, 23, 27, 8 };

		Holder<Mesh> msh = newMesh();
		msh->positions(verts);
		msh->indices(faces);
		for (uint32 &i : msh->indices())
			i--;
		return msh;
	}

	Holder<Mesh> makeGrid()
	{
		std::vector<Vec3> p;
		p.reserve(121);
		for (uint32 y = 0; y < 11; y++)
			for (uint32 x = 0; x < 11; x++)
				p.push_back(Vec3(Real(x) - 5, Real(y) - 5, 0));
		std::vector<uint32> is;
		is.reserve(100 * 2 * 3);
		for (uint32 y = 0; y < 10; y++)
		{
			for (uint32 x = 0; x < 10; x++)
			{
				const uint32 a = y * 11 + x;
				static constexpr uint32 o[6] = { 0, 1, 11, 1, 12, 11 };
				for (uint32 b : o)
					is.push_back(a + b);
			}
		}
		Holder<Mesh> msh = newMesh();
		msh->positions(p);
		msh->indices(is);
		return msh;
	}

	Holder<Mesh> makeLargeGrid()
	{
		std::vector<Vec3> p;
		p.reserve(1000);
		for (uint32 y = 0; y < 101; y++)
			for (uint32 x = 0; x < 101; x++)
				p.push_back(Vec3(Real(x) - 50, Real(y) - 50, 0));
		std::vector<uint32> is;
		is.reserve(1000);
		for (uint32 y = 0; y < 100; y++)
		{
			for (uint32 x = 0; x < 100; x++)
			{
				const uint32 a = y * 101 + x;
				static constexpr uint32 o[6] = { 0, 1, 101, 1, 102, 101 };
				for (uint32 b : o)
					is.push_back(a + b);
			}
		}
		Holder<Mesh> msh = newMesh();
		msh->positions(p);
		msh->indices(is);
		return msh;
	}

	Holder<Mesh> makeCube()
	{
		static constexpr Vec3 verts[] = {
			Vec3(-1, -1, -1), // 0
			Vec3(1, -1, -1), // 1
			Vec3(1, 1, -1), // 2
			Vec3(-1, 1, -1), // 3
			Vec3(-1, -1, 1), // 4
			Vec3(1, -1, 1), // 5
			Vec3(1, 1, 1), // 6
			Vec3(-1, 1, 1) // 7
		};

		static constexpr uint32 faces[] = { // Back face
			0, 1, 2, 2, 3, 0,
			// Front face
			4, 5, 6, 6, 7, 4,
			// Left face
			0, 3, 7, 7, 4, 0,
			// Right face
			1, 5, 6, 6, 2, 1,
			// Bottom face
			0, 1, 5, 5, 4, 0,
			// Top face
			3, 2, 6, 6, 7, 3
		};

		Holder<Mesh> msh = newMesh();
		msh->positions(verts);
		msh->indices(faces);
		return msh;
	}

	void testMeshBasics()
	{
		const Holder<const Mesh> msh = makeSphere();
		CAGE_TEST(msh->verticesCount() > 10);
		CAGE_TEST(msh->indicesCount() > 10);
		CAGE_TEST(msh->indicesCount() == msh->facesCount() * 3);

		{
			CAGE_TESTCASE("valid vertices");
			for (const Vec3 &p : msh->positions())
			{
				CAGE_TEST(valid(p));
			}
			for (const Vec3 &p : msh->normals())
			{
				CAGE_TEST(valid(p));
				CAGE_TEST(abs(length(p) - 1) < 1e-5);
			}
		}

		{
			CAGE_TESTCASE("bounding box");
			approxEqual(msh->boundingBox(), Aabb(Vec3(-10), Vec3(10)));
		}

		{
			CAGE_TESTCASE("bounding sphere");
			approxEqual(msh->boundingSphere(), Sphere(Vec3(), 10));
		}

		{
			CAGE_TESTCASE("collider");
			const uint32 initialFacesCount = msh->facesCount();
			Holder<Collider> c = newCollider();
			c->importMesh(+msh);
			CAGE_TEST(c->triangles().size() > 10);
			Holder<Mesh> p = newMesh();
			p->importCollider(+c);
			CAGE_TEST(p->positions().size() > 10);
			CAGE_TEST(p->facesCount() == initialFacesCount);
		}

		{
			CAGE_TESTCASE("shapes");
			{
				auto s = newMeshSphereUv(10, 20, 30);
				CAGE_TEST(!meshDetectInvalid(+s));
				s->exportFile("meshes/shapes/uv-sphere.obj");
			}
			{
				auto s = newMeshIcosahedron(10);
				CAGE_TEST(!meshDetectInvalid(+s));
				s->exportFile("meshes/shapes/icosahedron.obj");
			}
			{
				auto s = newMeshSphereRegular(10, 1.5);
				CAGE_TEST(!meshDetectInvalid(+s));
				s->exportFile("meshes/shapes/regular-sphere.obj");
			}
		}

		{
			CAGE_TESTCASE("serialize");
			Holder<PointerRange<char>> buff = msh->exportBuffer();
			CAGE_TEST(buff.size() > 10);
			Holder<Mesh> p = newMesh();
			p->importBuffer(buff);
			CAGE_TEST(p->verticesCount() == msh->verticesCount());
			CAGE_TEST(p->indicesCount() == msh->indicesCount());
			CAGE_TEST(p->positions().size() == msh->positions().size());
			CAGE_TEST(p->normals().size() == msh->normals().size());
			CAGE_TEST(p->uvs().size() == msh->uvs().size());
			CAGE_TEST(p->uvs3().size() == msh->uvs3().size());
			CAGE_TEST(p->boneIndices().size() == msh->boneIndices().size());
		}
	}

	void testMeshAlgorithms()
	{
		{
			CAGE_TESTCASE("apply transformation");
			auto p = makeSphere();
			meshApplyTransform(+p, Transform(Vec3(0, 5, 0)));
			approxEqual(p->boundingBox(), Aabb(Vec3(-10, -5, -10), Vec3(10, 15, 10)));
		}

		{
			CAGE_TESTCASE("flip normals");
			auto p = makeSphere();
			meshFlipNormals(+p);
			p->exportFile("meshes/algorithms/flipNormals.obj");
		}

		{
			CAGE_TESTCASE("duplicate sides");
			auto p = makeSphere();
			meshDuplicateSides(+p);
			p->exportFile("meshes/algorithms/duplicateSides.obj");
		}

		{
			CAGE_TESTCASE("merge close vertices");
			auto p = makeSphereRaw();
			const uint32 initialFacesCount = p->facesCount();
			{
				MeshMergeCloseVerticesConfig cfg;
				cfg.distanceThreshold = 1e-3;
				meshMergeCloseVertices(+p, cfg);
			}
			const uint32 f = p->facesCount();
			CAGE_TEST(f > 10 && f < initialFacesCount);
			p->exportFile("meshes/algorithms/mergeCloseVertices.obj");
		}

		{
			CAGE_TESTCASE("merge planar (balls)");
			auto p = makeDoubleBalls();
			meshSplitIntersecting(+p, {});
			meshRemoveOccluded(+p, {});
			meshMergeCloseVertices(+p, {});
			meshMergePlanar(+p);
			p->exportFile("meshes/algorithms/mergePlanarBalls.obj");
		}

		{
			CAGE_TESTCASE("merge planar (cube)");
			auto p = makeComplexCube();
			meshMergeCloseVertices(+p, {});
			meshMergePlanar(+p);
			p->exportFile("meshes/algorithms/mergePlanarCube.obj");
			CAGE_TEST(p->verticesCount() == 8);
			CAGE_TEST(p->facesCount() == 12);
		}

		{
			CAGE_TESTCASE("merge planar (grid)");
			auto p = makeGrid();
			meshMergePlanar(+p);
			p->exportFile("meshes/algorithms/mergePlanarGrid.obj");
			CAGE_TEST(p->verticesCount() == 4);
			CAGE_TEST(p->facesCount() == 2);
		}

		{
			CAGE_TESTCASE("separate disconnected");
			auto p = splitSphereIntoTwo(+makeSphere());
			auto ps = meshSeparateDisconnected(+p);
			CAGE_TEST(ps.size() == 2);
			ps[0]->exportFile("meshes/algorithms/separateDisconnected_1.obj");
			ps[1]->exportFile("meshes/algorithms/separateDisconnected_2.obj");
		}

		{
			CAGE_TESTCASE("split long");
			auto p = makeSphere();
			meshApplyTransform(+p, Mat4::scale(Vec3(0.5, 5, 0.5)));
			MeshSplitLongConfig cfg;
			cfg.length = 10;
			meshSplitLong(+p, cfg);
			p->exportFile("meshes/algorithms/splitLong.obj");
		}

		{
			CAGE_TESTCASE("split intersecting");
			auto p = makeDoubleBalls();
			MeshSplitIntersectingConfig cfg;
			cfg.parallelize = true;
			meshSplitIntersecting(+p, cfg);
			p->exportFile("meshes/algorithms/splitIntersecting.obj");
		}

		{
			CAGE_TESTCASE("clip by box (sphere)");
			auto p = makeSphere();
			static constexpr Aabb box = Aabb(Vec3(-6, -6, -10), Vec3(6, 6, 10));
			meshClip(+p, box);
			p->exportFile("meshes/algorithms/clipByBoxFromSphere.obj");
			approxEqual(p->boundingBox(), box);
		}

		{
			CAGE_TESTCASE("clip by box (grid)");
			auto p = makeGrid();
			static constexpr Aabb box = Aabb(Vec3(-3.5), Vec3(3.5));
			meshClip(+p, box);
			p->exportFile("meshes/algorithms/clipByBoxFromGrid.obj");
			Aabb b = box;
			b.a[2] = 0;
			b.b[2] = 0;
			approxEqual(p->boundingBox(), b);
		}

		{
			CAGE_TESTCASE("clip by plane");
			auto p = makeSphere();
			const Plane pln = Plane(Vec3(0, -1, 0), normalize(Vec3(1)));
			meshClip(+p, pln);
			p->exportFile("meshes/algorithms/clipByPlane.obj");
		}

		{
			CAGE_TESTCASE("remove invalid");
			auto p = makeSphere();
			const uint32 initialFacesCount = p->facesCount();
			CAGE_TEST(!meshDetectInvalid(+p));
			p->position(42, Vec3::Nan()); // intentionally corrupt one vertex
			CAGE_TEST(meshDetectInvalid(+p));
			meshRemoveInvalid(+p);
			CAGE_TEST(!meshDetectInvalid(+p));
			const uint32 f = p->facesCount();
			CAGE_TEST(f > 10 && f < initialFacesCount);
			p->exportFile("meshes/algorithms/removeInvalid.obj");
		}

		{
			CAGE_TESTCASE("remove disconnected");
			auto p = splitSphereIntoTwo(+makeSphere());
			meshRemoveDisconnected(+p);
			p->exportFile("meshes/algorithms/removeDisconnected.obj");
		}

		{
			CAGE_TESTCASE("remove small");
			auto p = makeSphereRaw();
			const uint32 initialFacesCount = p->facesCount();
			{
				MeshRemoveSmallConfig cfg;
				cfg.threshold = 1;
				meshRemoveSmall(+p, cfg);
			}
			const uint32 f = p->facesCount();
			CAGE_TEST(f > 10 && f < initialFacesCount);
			p->exportFile("meshes/algorithms/removeSmall.obj");
		}

		{
			CAGE_TESTCASE("remove invisible");
			auto p = makeDoubleBalls();
			MeshSplitIntersectingConfig cfg1;
			cfg1.parallelize = true;
			meshSplitIntersecting(+p, cfg1);
			MeshRemoveOccludedConfig cfg2;
			cfg2.raysPerUnitArea = 5;
			cfg2.parallelize = true;
			meshRemoveOccluded(+p, cfg2);
			p->exportFile("meshes/algorithms/removeInvisible.obj");
		}

		{
			CAGE_TESTCASE("simplify (sphere)");
			auto p = makeSphere();
			MeshSimplifyConfig cfg;
#ifdef CAGE_DEBUG
			cfg.iterations = 1;
			cfg.minEdgeLength = 0.5;
			cfg.maxEdgeLength = 2;
			cfg.approximateError = 0.5;
#endif
			meshSimplify(+p, cfg);
			p->exportFile("meshes/algorithms/simplifySphere.obj");
		}

		{
			CAGE_TESTCASE("simplify (grid after cut)");
			auto p = makeGrid();
			static constexpr Aabb box = Aabb(Vec3(-3.5), Vec3(3.5));
			meshClip(+p, box);
			meshSimplify(+p, {});
			p->exportFile("meshes/algorithms/simplifyGrid.obj");
		}

		{
			CAGE_TESTCASE("regularize");
			auto p = makeSphere();
			MeshRegularizeConfig cfg;
#ifdef CAGE_DEBUG
			cfg.iterations = 1;
			cfg.targetEdgeLength = 3;
#endif
			meshRegularize(+p, cfg);
			p->exportFile("meshes/algorithms/regularize.obj");
		}

		{
			CAGE_TESTCASE("smoothing");
			auto p = makeSphere();
			for (Vec3 &v : p->positions())
				v += randomDirection3() * 0.5;
			MeshSmoothingConfig cfg;
#ifdef CAGE_DEBUG
			cfg.iterations = 1;
#endif
			meshSmoothing(+p, cfg);
			p->exportFile("meshes/algorithms/smoothing.obj");
		}

		{
			CAGE_TESTCASE("chunking (serial)");
			auto p = makeSphere();
			MeshChunkingConfig cfg;
			constexpr Real initialAreaImplicit = 4 * Real::Pi() * sqr(10);
			constexpr Real targetChunks = 10;
			cfg.maxSurfaceArea = initialAreaImplicit / targetChunks;
			const auto res = meshChunking(+p, cfg);
			CAGE_TEST(res.size() > targetChunks / 2 && res.size() < targetChunks * 2);
			uint32 index = 0;
			for (const auto &it : res)
			{
				CAGE_TEST(it->facesCount() > 0);
				CAGE_TEST(it->indicesCount() > 0);
				CAGE_TEST(it->type() == MeshTypeEnum::Triangles);
				it->exportFile(Stringizer() + "meshes/algorithms/chunkingSerial/" + index++ + ".obj");
			}
		}

		{
			CAGE_TESTCASE("chunking (parallel)");
			auto p = makeSphere();
			MeshChunkingConfig cfg;
			constexpr Real initialAreaImplicit = 4 * Real::Pi() * sqr(10);
			constexpr Real targetChunks = 10;
			cfg.maxSurfaceArea = initialAreaImplicit / targetChunks;
			const auto res = meshChunking(+p, cfg);
			CAGE_TEST(res.size() > targetChunks / 2 && res.size() < targetChunks * 2);
			uint32 index = 0;
			for (const auto &it : res)
			{
				CAGE_TEST(it->facesCount() > 0);
				CAGE_TEST(it->indicesCount() > 0);
				CAGE_TEST(it->type() == MeshTypeEnum::Triangles);
				it->exportFile(Stringizer() + "meshes/algorithms/chunkingParallel/" + index++ + ".obj");
			}
		}

		{
			CAGE_TESTCASE("convex hull");
			auto p = makeDoubleBalls();
			MeshConvexHullConfig cfg;
			const auto hull = meshConvexHull(+p, cfg);
			hull->exportFile("meshes/algorithms/convexHull.obj");
		}

		{
			CAGE_TESTCASE("unwrap");
			auto p = makeSphere();
			uint32 res = 0;
			{
				MeshUnwrapConfig cfg;
				cfg.targetResolution = 256;
				res = meshUnwrap(+p, cfg);
				p->exportFile("meshes/algorithms/unwrap.obj");
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
				cfg.generator.bind<MeshImage *, &genTex>(&data);
				meshGenerateTexture(+p, cfg);
				img->exportFile("meshes/algorithms/unwrap.png");
			}
		}

		{
			CAGE_TESTCASE("generate flat normals");
			auto p = makeSphere();
			MeshGenerateNormalsConfig cfg;
			cfg.flat = true;
			meshGenerateNormals(+p, cfg);
			CAGE_TEST(p->normals().size() == p->positions().size());
			p->exportFile("meshes/algorithms/generatedNormals_flat.obj");
		}

		{
			CAGE_TESTCASE("generate smooth normals");
			auto p = makeSphere();
			meshGenerateNormals(+p, {});
			CAGE_TEST(p->normals().size() == p->positions().size());
			p->exportFile("meshes/algorithms/generatedNormals_smooth.obj");
		}

		/*
		{
			CAGE_TESTCASE("mesh merge");
			const auto &make = []()
			{
				auto msh = makeSphere();
				meshGenerateNormals(+msh, {});
				meshUnwrap(+msh, MeshUnwrapConfig{ .targetResolution = 256 });
				auto tex = newImage();
				tex->initialize(Vec2i(256), 3);
				MeshImage tmp;
				tmp.msh = +msh;
				tmp.img = +tex;
				MeshGenerateTextureConfig cfg;
				cfg.width = cfg.height = 256;
				cfg.generator.bind<MeshImage *, &genTex>(&tmp);
				meshGenerateTexture(+msh, cfg);
				return std::pair{ std::move(msh), std::move(tex) };
			};
			auto a = make();
			auto b = make();
			imageInvertColors(+b.second);
			meshApplyTransform(+a.first, Transform(Vec3(-11, 0, 0)));
			meshApplyTransform(+b.first, Transform(Vec3(+11, 0, 0)));
			MeshMergeInput in[2];
			const Image *at[1] = { +a.second };
			const Image *bt[1] = { +a.second };
			in[0].mesh = +a.first;
			in[0].textures = at;
			in[1].mesh = +b.first;
			in[1].textures = bt;
			auto r = meshMerge(in, {});
			CAGE_TEST(r.textures.size() == 1);
			MeshExportGltfConfig exportCfg;
			exportCfg.name = "merged";
			exportCfg.mesh = +r.mesh;
			exportCfg.albedo.image = +r.textures[0];
			meshExportFiles("meshes/algorithms/merge.glb", exportCfg);
		}
		*/
	}

	void testMeshImports()
	{
		{
			CAGE_TESTCASE("simple obj import");
			Holder<Mesh> orig = newMeshIcosahedron(10);
			{
				MeshExportObjConfig cfg;
				cfg.materialName = "aquamarine";
				cfg.objectName = "simple";
				cfg.mesh = +orig;
				meshExportFiles("meshes/imports/simple.obj", cfg);
			}

			{
				CAGE_TESTCASE("relative path");
				const MeshImportResult result = meshImportFiles("meshes/imports/simple.obj");
				CAGE_TEST(result.parts.size() == 1);
				CAGE_TEST(!result.skeleton);
				CAGE_TEST(result.animations.empty());
				const auto &part = result.parts[0];
				CAGE_TEST(part.materialName == "aquamarine");
				CAGE_TEST(part.objectName == "simple");
				CAGE_TEST(part.textures.empty());
				Holder<Mesh> msh = part.mesh.share();
				CAGE_TEST(msh->facesCount() == orig->facesCount());
				approxEqual(part.boundingBox, orig->boundingBox());
				CAGE_TEST(result.paths.size() == 1);
				CAGE_TEST(result.paths[0] == pathToAbs("meshes/imports/simple.obj"));
			}

			{
				CAGE_TESTCASE("absolute path");
				const MeshImportResult result = meshImportFiles(pathToAbs("meshes/imports/simple.obj"));
				CAGE_TEST(result.parts.size() == 1);
				CAGE_TEST(result.paths[0] == pathToAbs("meshes/imports/simple.obj"));
			}
		}

		{
			CAGE_TESTCASE("obj with uv");
			Holder<Mesh> orig = newMeshIcosahedron(10);
			unwrapThetaPhi(+orig);
			{
				MeshExportObjConfig cfg;
				cfg.materialName = "ruby";
				cfg.objectName = "withuv";
				cfg.mesh = +orig;
				meshExportFiles("meshes/imports/uv.obj", cfg);
			}

			const MeshImportResult result = meshImportFiles("meshes/imports/uv.obj");
			CAGE_TEST(result.parts.size() == 1);
			CAGE_TEST(!result.skeleton);
			CAGE_TEST(result.animations.empty());
			const auto &part = result.parts[0];
			CAGE_TEST(part.materialName == "ruby");
			CAGE_TEST(part.objectName == "withuv");
			CAGE_TEST(part.textures.empty());
			Holder<Mesh> msh = part.mesh.share();
			CAGE_TEST(msh->facesCount() == orig->facesCount());
			approxEqual(part.boundingBox, orig->boundingBox());
			testUnwrapThetaPhi(+msh);
			CAGE_TEST(result.paths.size() == 1);
			CAGE_TEST(result.paths[0] == pathToAbs("meshes/imports/uv.obj"));
		}

		{
			CAGE_TESTCASE("glb with uv");
			Holder<Mesh> orig = newMeshIcosahedron(10);
			unwrapThetaPhi(+orig);
			{
				MeshExportGltfConfig cfg;
				cfg.mesh = +orig;
				cfg.name = "uv2";
				meshExportFiles("meshes/imports/uv2.glb", cfg);
			}

			const MeshImportResult result = meshImportFiles("meshes/imports/uv2.glb");
			CAGE_TEST(result.parts.size() == 1);
			CAGE_TEST(!result.skeleton);
			CAGE_TEST(result.animations.empty());
			const auto &part = result.parts[0];
			CAGE_TEST(part.materialName == "uv2");
			CAGE_TEST(part.objectName == "uv2");
			CAGE_TEST(part.textures.empty());
			Holder<Mesh> msh = part.mesh.share();
			CAGE_TEST(msh->facesCount() == orig->facesCount());
			approxEqual(part.boundingBox, orig->boundingBox());
			testUnwrapThetaPhi(+msh);
			CAGE_TEST(result.paths.size() == 1);
			CAGE_TEST(result.paths[0] == pathToAbs("meshes/imports/uv2.glb"));
		}
	}

	void loadAndExport(const String &in, const String &out)
	{
		MeshImportResult result = meshImportFiles(in);
		meshImportNormalizeFormats(result);
		CAGE_TEST(result.parts.size() == 1);
		const auto &part = result.parts[0];
		Holder<Image> albedo, roughness, metallic, normal;
		for (const auto &it : result.parts[0].textures)
		{
			switch (it.type)
			{
				case MeshImportTextureType::Albedo:
					albedo = std::move(it.images.parts[0].image);
					break;
				case MeshImportTextureType::Roughness:
					roughness = std::move(it.images.parts[0].image);
					break;
				case MeshImportTextureType::Metallic:
					metallic = std::move(it.images.parts[0].image);
					break;
				case MeshImportTextureType::Normal:
					normal = std::move(it.images.parts[0].image);
					break;
				default:
					break;
			}
		}
		const Image *arr[] = { nullptr, +roughness, +metallic };
		Holder<Image> pbr = roughness || metallic ? imageChannelsJoin(arr) : Holder<Image>();
		MeshExportGltfConfig cfg;
		cfg.name = pathExtractFilenameNoExtension(out);
		cfg.mesh = +part.mesh;
		cfg.albedo.image = +albedo;
		cfg.pbr.image = +pbr;
		cfg.normal.image = +normal;
		cfg.renderFlags = part.renderFlags;
		meshExportFiles(out, cfg);
	}

	void testMeshExports()
	{
		CAGE_TESTCASE("export gltf with textures");
		auto p = makeSphere();
		uint32 res = 0;
		{
			CAGE_TESTCASE("unwrap");
			MeshUnwrapConfig cfg;
			cfg.targetResolution = 256;
			res = meshUnwrap(+p, cfg);
		}
		Holder<Image> albedo, pbr, normal;
		{
			CAGE_TESTCASE("texturing albedo");
			albedo = newImage();
			albedo->initialize(Vec2i(res), 3);
			MeshImage data;
			data.msh = +p;
			data.img = +albedo;
			MeshGenerateTextureConfig cfg;
			cfg.width = cfg.height = res;
			cfg.generator.bind<MeshImage *, &genTex>(&data);
			meshGenerateTexture(+p, cfg);
			imageDilation(+albedo, 4);
		}
		{
			CAGE_TESTCASE("texturing pbr");
			pbr = newImage();
			pbr->initialize(Vec2i(100, 50), 3);
			imageFill(+pbr, Vec3(0.1, 0.2, 0.3));
		}
		{
			CAGE_TESTCASE("texturing normal");
			normal = newImage();
			normal->initialize(Vec2i(100, 50), 3);
			imageFill(+normal, Vec3(0.5, 0.5, 1));
		}

		{
			CAGE_TESTCASE("export with 1 texture");
			MeshExportGltfConfig cfg;
			cfg.name = "oneTexture";
			cfg.mesh = +p;
			cfg.albedo.image = +albedo;
			meshExportFiles("meshes/exports/oneTexture.glb", cfg);
		}

		loadAndExport("meshes/exports/oneTexture.glb", "meshes/exports/oneTexture_2.glb");
		loadAndExport("meshes/exports/oneTexture_2.glb", "meshes/exports/oneTexture_3.glb");

		{
			CAGE_TESTCASE("export with 3 textures");
			MeshExportGltfConfig cfg;
			cfg.name = "threeTextures";
			cfg.mesh = +p;
			cfg.albedo.image = +albedo;
			cfg.pbr.image = +pbr;
			cfg.normal.image = +normal;
			cfg.parallelize = true;
			meshExportFiles("meshes/exports/threeTextures.glb", cfg);
		}

		loadAndExport("meshes/exports/threeTextures.glb", "meshes/exports/threeTextures_2.glb");
		loadAndExport("meshes/exports/threeTextures_2.glb", "meshes/exports/threeTextures_3.glb");

		{
			CAGE_TESTCASE("export with external texture");
			MeshExportGltfConfig cfg;
			cfg.name = "externalTexture";
			cfg.mesh = +p;
			cfg.albedo.image = +albedo;
			cfg.albedo.filename = "externalAlbedo.png";
			meshExportFiles("meshes/exports/externalTextures.glb", cfg);
		}

		loadAndExport("meshes/exports/externalTextures.glb", "meshes/exports/externalTextures_2.glb");

		Holder<Image> transparent = newImage();
		transparent->initialize(albedo->resolution(), 4);
		for (uint32 y = 0; y < albedo->height(); y++)
			for (uint32 x = 0; x < albedo->width(); x++)
				transparent->set(x, y, Vec4(albedo->get3(x, y), Real(y) / albedo->height()));

		{
			CAGE_TESTCASE("export with transparent texture");
			MeshExportGltfConfig cfg;
			cfg.name = "transparentTexture";
			cfg.mesh = +p;
			cfg.albedo.image = +transparent;
			cfg.renderFlags |= MeshRenderFlags::Transparent;
			meshExportFiles("meshes/exports/transparentTexture.glb", cfg);
		}

		loadAndExport("meshes/exports/transparentTexture.glb", "meshes/exports/transparentTexture_2.glb");
		loadAndExport("meshes/exports/transparentTexture_2.glb", "meshes/exports/transparentTexture_3.glb");
	}

	void testMeshRetexture()
	{
		CAGE_TESTCASE("retexture");

		Holder<Mesh> a = makeSphere();
		{
			CAGE_TESTCASE("initial cut");
			meshClip(+a, Aabb(Vec3(-11, -11, -9), Vec3(11, 11, 9)));
		}
		{
			CAGE_TESTCASE("initial unwrap");
			unwrapThetaPhi(+a);
		}
		Holder<Image> ai = newImage();
		{
			CAGE_TESTCASE("initial texture");
			ai->initialize(300, 200, 3);
			for (uint32 y = 0; y < 200; y++)
				for (uint32 x = 0; x < 300; x++)
					ai->set(x, y, Vec3(sqr(x / 300.0), sqr(y / 200.0), 0));
		}
		{
			CAGE_TESTCASE("initial export");
			{
				MeshExportObjConfig cfg;
				cfg.materialLibraryName = "init.mtl";
				cfg.materialName = "init";
				cfg.objectName = "init";
				cfg.mesh = +a;
				meshExportFiles("meshes/retexture/init.obj", cfg);
			}
			ai->exportFile("meshes/retexture/init.png");
			Holder<File> f = writeFile("meshes/retexture/init.mtl");
			f->writeLine("newmtl init");
			f->writeLine("map_Kd init.png");
			f->close();
		}

		Holder<Mesh> b = a->copy();
		{
			CAGE_TESTCASE("final transform");
			meshApplyTransform(+b, Transform(Vec3(), Quat(Degs(), Degs(90), Degs())));
		}
		uint32 res = 0;
		{
			CAGE_TESTCASE("final unwrap");
			MeshUnwrapConfig cfg;
			cfg.targetResolution = 256;
			res = meshUnwrap(+b, cfg);
		}
		Holder<Image> bi;
		{
			CAGE_TESTCASE("final retexture");
			MeshRetextureConfig cfg;
			const Image *in[1] = { +ai };
			cfg.inputs = in;
			cfg.source = +a;
			cfg.target = +b;
			cfg.resolution = Vec2i(res);
			cfg.maxDistance = 0.5;
			cfg.parallelize = false;
			auto res = meshRetexture(cfg);
			CAGE_TEST(res.size() == 1);
			bi = std::move(res[0]);
		}
		{
			CAGE_TESTCASE("final texture dilation");
			imageDilation(+bi, 4);
		}
		{
			CAGE_TESTCASE("final export");
			{
				MeshExportObjConfig cfg;
				cfg.materialLibraryName = "final.mtl";
				cfg.materialName = "final";
				cfg.objectName = "final";
				cfg.mesh = +b;
				meshExportFiles("meshes/retexture/final.obj", cfg);
			}
			bi->exportFile("meshes/retexture/final.png");
			Holder<File> f = writeFile("meshes/retexture/final.mtl");
			f->writeLine("newmtl final");
			f->writeLine("map_Kd final.png");
			f->close();
		}
	}

	void testMeshLines()
	{
		CAGE_TESTCASE("mesh lines");
		Holder<Mesh> msh = newMesh();
		msh->type(MeshTypeEnum::Lines);
		for (uint32 i = 0; i < 100; i++)
		{
			Real j = Real(i) / 99;
			Real x = sin(Degs(360 * j));
			Real y = cos(Degs(360 * j));
			Real z = (j - 0.5) * 4;
			msh->addVertex(Vec3(x, y, z));
		}
		meshConvertToIndexed(+msh);
		approxEqual(msh->boundingBox(), Aabb(Vec3(-1, -1, -2), Vec3(1, 1, 2)));
		meshApplyTransform(+msh, Mat4(Mat3(10, 0, 0, 0, 0, 10, 0, -10, 0)));
		approxEqual(msh->boundingBox(), Aabb(Vec3(-10, -20, -10), Vec3(10, 20, 10)));

		{
			CAGE_TESTCASE("export gltf");
			MeshExportGltfConfig cfg;
			cfg.mesh = +msh;
			meshExportFiles("meshes/exports/lines.glb", cfg);
		}

		{
			CAGE_TESTCASE("import gltf");
			const auto imp = meshImportFiles("meshes/exports/lines.glb");
			CAGE_TEST(imp.parts.size() == 1);
			CAGE_TEST(imp.parts[0].mesh->type() == MeshTypeEnum::Lines);
			approxEqual(imp.parts[0].mesh->boundingBox(), Aabb(Vec3(-10, -20, -10), Vec3(10, 20, 10)));
			CAGE_TEST(imp.parts[0].mesh->verticesCount() == msh->verticesCount());
			CAGE_TEST(imp.parts[0].mesh->indicesCount() == msh->indicesCount());
		}
	}

	void testMeshConsistentWinding()
	{
		CAGE_TESTCASE("mesh consistent winding");
		Holder<Mesh> msh = makeDoubleBalls();
		meshConvertToExpanded(+msh);
		PointerRange<Vec3> tris = msh->positions();
		CAGE_ASSERT((tris.size() % 3) == 0);
		for (uint32 i = 0; i < tris.size(); i += 3)
		{
			PointerRange<Vec3> sub = { tris.data() + i, tris.data() + i + 3 };
			std::sort(sub.begin(), sub.end(), [](const Vec3 &a, const Vec3 &b) { return detail::memcmp(&a, &b, sizeof(Vec3)) < 0; });
		}
		meshConvertToIndexed(+msh);
		const uint32 cnt = msh->indicesCount();
		CAGE_TEST(cnt > 100 && cnt < 10'000); // sanity check
		{
			Holder<Mesh> cp = msh->copy();
			CAGE_TEST_THROWN(meshSimplify(+cp, {})); // sanity check
		}
		msh->exportFile("meshes/algorithms/consistentWindingBefore.obj");
		meshConsistentWinding(+msh);
		msh->exportFile("meshes/algorithms/consistentWindingAfter.obj");
		CAGE_TEST(msh->indicesCount() == cnt);
		meshSimplify(+msh, {}); // make sure that the resulting mesh has usable topology
	}

	void testMeshClipping()
	{
		CAGE_TESTCASE("mesh clipping");

		for (Real offset : { Real(-2), Real(-1.5), Real(0), Real(0.5), Real(1.33) })
		{
			Holder<Mesh> grid = makeGrid();
			meshApplyTransform(+grid, Transform(Vec3(), Quat(), 0.5));
			Holder<Mesh> box = makeCube();
			meshApplyTransform(+box, Transform(Vec3(offset, 0, 0), Quat(), 1));
			Aabb aabb = box->boundingBox();
			meshClip(+grid, aabb);

			{ // merge the box with the grid for better visual check
				const auto inds = box->indices();
				const auto poss = box->positions();
				const uint32 tris = numeric_cast<uint32>(inds.size() / 3);
				for (uint32 tri = 0; tri < tris; tri++)
				{
					const uint32 *ids = inds.data() + tri * 3;
					const Vec3 &a = poss[ids[0]];
					const Vec3 &b = poss[ids[1]];
					const Vec3 &c = poss[ids[2]];
					const Triangle t = Triangle(a, b, c);
					grid->addTriangle(t);
				}
			}
			grid->exportFile(Stringizer() + "meshes/algorithms/testClipping/" + offset + ".obj");
		}
	}

	Real meshSurfaceArea(const Mesh *mesh)
	{
		const auto inds = mesh->indices();
		const auto poss = mesh->positions();
		const uint32 cnt = numeric_cast<uint32>(inds.size() / 3);
		Real result = 0;
		for (uint32 ti = 0; ti < cnt; ti++)
		{
			const Triangle t = Triangle(poss[inds[ti * 3 + 0]], poss[inds[ti * 3 + 1]], poss[inds[ti * 3 + 2]]);
			result += t.area();
		}
		return result;
	}

	void testMeshChunking()
	{
		CAGE_TESTCASE("mesh chunking");

		Holder<Mesh> grid = makeLargeGrid();
		const Aabb initialBox = grid->boundingBox();
		approxEqual(initialBox, Aabb(Vec3(-50, -50, 0), Vec3(50, 50, 0)));
		const Real initialArea = meshSurfaceArea(+grid);
		CAGE_TEST(abs(initialArea - 10'000) < 1);
		MeshChunkingConfig cfg;
		cfg.maxSurfaceArea = 10'000 / randomRange(10, 30);
		auto r = meshChunking(+grid, cfg);
		Real sum = 0;
		Aabb box;
		uint32 i = 0;
		for (const auto &it : r)
		{
			sum += meshSurfaceArea(+it);
			box += it->boundingBox();
			it->exportFile(Stringizer() + "meshes/algorithms/testChunking/" + (i++) + ".obj");
		}
		CAGE_TEST(abs(sum - initialArea) < 1);
		approxEqual(box, initialBox);
	}
}

void testMesh()
{
	CAGE_TESTCASE("mesh");
	testMeshBasics();
	testMeshAlgorithms();
	testMeshImports();
	testMeshExports();
	testMeshRetexture();
	testMeshLines();
	testMeshConsistentWinding();
	testMeshClipping();
	testMeshChunking();
}
