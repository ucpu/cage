#include "main.h"

#include <cage-core/geometry.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/meshShapes.h>
#include <cage-core/meshImport.h>
#include <cage-core/meshExport.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/files.h>
#include <vector>

namespace
{
	void approxEqual(const Real &a, const Real &b)
	{
		CAGE_TEST(abs(a - b) < 0.02);
	}

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

	void approxEqual(const Image *a, const Image *b)
	{
		CAGE_TEST(a->resolution() == b->resolution());
		const uint32 x = a->resolution()[0] / 2;
		const uint32 h = a->resolution()[1];
		for (uint32 y = 0; y < h; y++)
		{
			const Vec3 s = a->get3(x, y);
			const Vec3 t = b->get3(x, y);
			approxEqual(s, t);
		}
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
		static constexpr const Vec3 verts[] = {
			Vec3(-1.000000,-1.000000,1.000000),
			Vec3(-1.000000,1.000000,1.000000),
			Vec3(-1.000000,-1.000000,-1.000000),
			Vec3(-1.000000,1.000000,-1.000000),
			Vec3(1.000000,-1.000000,1.000000),
			Vec3(-0.033372,-1.000000,-0.186082),
			Vec3(1.000000,-1.000000,-1.000000),
			Vec3(0.000000,-1.000000,0.500000),
			Vec3(-1.000000,-1.000000,-0.333333),
			Vec3(-1.000000,-1.000000,0.333333),
			Vec3(-1.000000,1.000000,0.333333),
			Vec3(-1.000000,1.000000,-0.333333),
			Vec3(1.000000,-1.000000,0.000000),
			Vec3(1.000000,1.000000,-1.000000),
			Vec3(1.000000,1.000000,1.000000),
			Vec3(-1.000000,0.000000,1.000000),
			Vec3(-1.000000,0.000000,-1.000000),
			Vec3(1.000000,0.000000,-1.000000),
			Vec3(1.000000,0.000000,1.000000),
			Vec3(1.000000,0.000000,-0.333333),
			Vec3(1.000000,0.000000,0.333333),
			Vec3(-1.000000,0.000000,0.000000),
			Vec3(1.000000,-1.000000,0.500000),
			Vec3(0.000000,-1.000000,0.166667),
			Vec3(-0.500000,-1.000000,-0.250000),
			Vec3(1.000000,-1.000000,-0.500000),
			Vec3(-0.500000,-1.000000,0.750000),
			Vec3(-1.000000,-1.000000,0.000000),
			Vec3(0.500000,-1.000000,-0.083333),
			Vec3(0.000000,-1.000000,-0.666667),
			Vec3(0.250000,-1.000000,-0.375000),
			Vec3(-0.250000,-1.000000,-0.041667)
		};

		static constexpr const uint32 faces[] = {
			12, 17, 22,
			4, 18, 17,
			21, 15, 19,
			15, 16, 19,
			27, 10, 24,
			15, 11, 2,
			14, 4, 12,
			12, 15, 14,
			30, 3, 7,
			29, 24, 32,
			18, 14, 20,
			14, 21, 20,
			16, 11, 22,
			22, 11, 12,
			16, 10, 1,
			13, 20, 21,
			13, 7, 18,
			19, 1, 5,
			5, 13, 21,
			3, 18, 7,
			9, 17, 3,
			12, 11, 15,
			12, 4, 17,
			16, 2, 11,
			16, 22, 10,
			9, 22, 17,
			14, 15, 21,
			21, 19, 5,
			18, 20, 13,
			15, 2, 16,
			19, 16, 1,
			4, 14, 18,
			3, 17, 18,
			31, 6, 25,
			13, 23, 8,
			5, 27, 23,
			30, 25, 9,
			24, 13, 27,
			28, 32, 10,
			7, 31, 30,
			26, 29, 31,
			6, 32, 25,
			5, 1, 27,
			27, 1, 10,
			32, 24, 10,
			25, 32, 28,
			30, 31, 25,
			7, 26, 31,
			26, 13, 29,
			29, 13, 24,
			30, 9, 3,
			10, 22, 9,
			13, 26, 7,
			25, 28, 9,
			31, 29, 32,
			23, 27, 8
		};

		Holder<Mesh> msh = newMesh();
		msh->positions(verts);
		msh->indices(faces);
		for (uint32 &i : msh->indices())
			i--;
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
			newMeshSphereUv(10, 20, 30)->exportFile("meshes/shapes/uv-sphere.obj");
			newMeshIcosahedron(10)->exportFile("meshes/shapes/icosahedron.obj");
			newMeshSphereRegular(10, 1.5)->exportFile("meshes/shapes/regular-sphere.obj");
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
			CAGE_TEST(p->tangents().size() == msh->tangents().size());
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
			auto p = makeSphere();
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
			MeshMergePlanarConfig cfg;
			meshMergePlanar(+p, cfg);
			p->exportFile("meshes/algorithms/mergePlanarBalls.obj");
		}

		{
			CAGE_TESTCASE("merge planar (cube)");
			auto p = makeComplexCube();
			meshMergeCloseVertices(+p, {});
			MeshMergePlanarConfig cfg;
			meshMergePlanar(+p, cfg);
			p->exportFile("meshes/algorithms/mergePlanarCube.obj");
			CAGE_TEST(p->verticesCount() == 8);
			CAGE_TEST(p->facesCount() == 12);
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
			CAGE_TESTCASE("clip by box");
			auto p = makeSphere();
			static constexpr Aabb box = Aabb(Vec3(-6, -6, -10), Vec3(6, 6, 10));
			meshClip(+p, box);
			p->exportFile("meshes/algorithms/clipByBox.obj");
			approxEqual(p->boundingBox(), box);
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
			p->position(42, Vec3::Nan()); // intentionally corrupt one vertex
			meshRemoveInvalid(+p);
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
			auto p = makeSphere();
			const uint32 initialFacesCount = p->facesCount();
			{
				MeshRemoveSmallConfig cfg;
				cfg.threshold = 4;
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
			CAGE_TESTCASE("simplify");
			auto p = makeSphere();
			MeshSimplifyConfig cfg;
#ifdef CAGE_DEBUG
			cfg.iterations = 1;
			cfg.minEdgeLength = 0.5;
			cfg.maxEdgeLength = 2;
			cfg.approximateError = 0.5;
#endif
			meshSimplify(+p, cfg);
			p->exportFile("meshes/algorithms/simplify.obj");
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
			CAGE_TESTCASE("chunking");
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
				it->exportFile(Stringizer() + "meshes/algorithms/chunking/" + index++ + ".obj");
			}
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
			CAGE_TESTCASE("generate normals");
			auto p = makeSphere();
			meshGenerateNormals(+p, {});
			p->exportFile("meshes/algorithms/generatedNormals.obj");
			CAGE_TEST(p->normals().size() == p->positions().size());
		}
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
}

void testMesh()
{
	CAGE_TESTCASE("mesh");
	testMeshBasics();
	testMeshAlgorithms();
	testMeshImports();
	testMeshExports();
	testMeshRetexture();
}
