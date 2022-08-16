#include "main.h"
#include <cage-core/geometry.h>
#include <cage-core/mesh.h>
#include <cage-core/meshShapes.h>
#include <cage-core/meshImport.h>
#include <cage-core/meshExport.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/image.h>
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

	poly->exportObjFile("meshes/base.obj");
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
		p->exportObjFile("meshes/discardInvalid.obj");
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
		p->exportObjFile("meshes/mergeCloseVertices.obj");
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
		p->exportObjFile("meshes/simplify.obj");
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
		p->exportObjFile("meshes/regularize.obj");
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
			p->exportObjFile("meshes/unwrap.obj");
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
		CAGE_TESTCASE("retexture");

		Holder<Mesh> a = poly->copy();
		{
			CAGE_TESTCASE("initial cut");
			meshClip(+a, Aabb(Vec3(-11, -11, -9), Vec3(11, 11, 9)));
		}
		{
			CAGE_TESTCASE("initial unwrap");
			std::vector<Vec2> uv;
			uv.reserve(a->verticesCount());
			for (Vec3 p : a->positions())
			{
				p = normalize(p);
				Rads theta = acos(p[2]);
				Rads phi = atan2(p[1], p[0]);
				Real u = theta.value / Real::Pi();
				Real v = (phi.value / Real::Pi()) * 0.5 + 0.5;
				uv.push_back(saturate(Vec2(u, v)));
			}
			a->uvs(uv);
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
			MeshExportObjConfig cfg;
			cfg.materialLibraryName = "retexture.mtl";
			cfg.materialName = "retex_init";
			cfg.objectName = "retex_init";
			a->exportObjFile("meshes/retextureInit.obj", cfg);
			ai->exportFile("meshes/retextureInit.png");
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
			MeshExportObjConfig cfg;
			cfg.materialLibraryName = "retexture.mtl";
			cfg.materialName = "retex_fin";
			cfg.objectName = "retex_fin";
			b->exportObjFile("meshes/retextureFinal.obj", cfg);
			bi->exportFile("meshes/retextureFinal.png");
		}
		{
			CAGE_TESTCASE("material write");
			Holder<File> f = writeFile("meshes/retexture.mtl");
			f->writeLine("newmtl retex_init");
			f->writeLine("map_Kd retextureInit.png");
			f->writeLine("newmtl retex_fin");
			f->writeLine("map_Kd retextureFinal.png");
			f->close();
		}
	}

	{
		CAGE_TESTCASE("generate normals");
		auto p = poly->copy();
		meshGenerateNormals(+p, {});
		p->exportObjFile("meshes/generatedNormals.obj");
		CAGE_TEST(p->normals().size() == p->positions().size());
	}

	{
		CAGE_TESTCASE("clip by box");
		auto p = poly->copy();
		static constexpr Aabb box = Aabb(Vec3(-6, -6, -10), Vec3(6, 6, 10));
		meshClip(+p, box);
		p->exportObjFile("meshes/clipByBox.obj");
		approxEqual(p->boundingBox(), box);
	}

	/*
	{
		CAGE_TESTCASE("clip by plane");
		auto p = poly->copy();
		const Plane pln = Plane(Vec3(0, -1, 0), normalize(Vec3(1)));
		meshClip(+p, pln);
		p->exportObjFile("meshes/clipByPlane.obj");
	}
	*/

	{
		CAGE_TESTCASE("separateDisconnected");
		auto p = splitSphereIntoTwo(+poly);
		auto ps = meshSeparateDisconnected(+p);
		// CAGE_TEST(ps.size() == 2); // todo fix this -> it should really be 2 but is 3
		ps[0]->exportObjFile("meshes/separateDisconnected_1.obj");
		ps[1]->exportObjFile("meshes/separateDisconnected_2.obj");
	}

	{
		CAGE_TESTCASE("discardDisconnected");
		auto p = splitSphereIntoTwo(+poly);
		meshDiscardDisconnected(+p);
		p->exportObjFile("meshes/discardDisconnected.obj");
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
		newMeshSphereUv(10, 20, 30)->exportObjFile("meshes/shapes/uv-sphere.obj");
		newMeshIcosahedron(10)->exportObjFile("meshes/shapes/icosahedron.obj");
		newMeshSphereRegular(10, 1.5)->exportObjFile("meshes/shapes/regular-sphere.obj");
	}

	{
		CAGE_TESTCASE("import");
		Holder<Mesh> orig = newMeshIcosahedron(10);
		{
			MeshExportObjConfig cfg;
			cfg.materialName = "aquamarine";
			cfg.objectName = "icosahedron";
			orig->exportObjFile("meshes/testImport.obj", cfg);
		}

		{
			CAGE_TESTCASE("default config");
			const MeshImportResult result = meshImportFiles("meshes/testImport.obj");
			CAGE_TEST(result.parts.size() == 1);
			CAGE_TEST(!result.skeleton);
			CAGE_TEST(result.animations.empty());
			const auto &part = result.parts[0];
			CAGE_TEST(part.materialName == "aquamarine");
			CAGE_TEST(part.objectName == "icosahedron");
			CAGE_TEST(part.textures.empty());
			Holder<Mesh> msh = part.mesh.share();
			CAGE_TEST(msh->facesCount() == orig->facesCount());
			approxEqual(part.boundingBox, orig->boundingBox());
		}

		{
			CAGE_TESTCASE("absolute path");
			const MeshImportResult result = meshImportFiles(pathToAbs("meshes/testImport.obj"));
			CAGE_TEST(result.parts.size() == 1);
		}
	}

	{
		CAGE_TESTCASE("export gltf (no textures)");
		Holder<Mesh> orig = newMeshIcosahedron(10);
		orig->exportGltfFile("meshes/testExport.glb");
		{
			CAGE_TESTCASE("verify import");
			const MeshImportResult result = meshImportFiles("meshes/testExport.glb");
			CAGE_TEST(result.parts.size() == 1);
			CAGE_TEST(!result.skeleton);
			CAGE_TEST(result.animations.empty());
			const auto &part = result.parts[0];
			CAGE_TEST(part.objectName == "testExport");
			CAGE_TEST(part.textures.empty());
			Holder<Mesh> msh = part.mesh.share();
			CAGE_TEST(msh->facesCount() == orig->facesCount());
			approxEqual(part.boundingBox, orig->boundingBox());
		}
	}

	{
		CAGE_TESTCASE("export gltf (with textures)");
		auto p = poly->copy();
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
			CAGE_TESTCASE("export");
			MeshExportConfig cfg;
			cfg.name = "textured_test";
			cfg.mesh = +p;
			cfg.albedo.image = +albedo;
			meshExportFiles("meshes/testExport1.glb", cfg);
		}
		{
			CAGE_TESTCASE("verify import");
			MeshImportResult result = meshImportFiles("meshes/testExport1.glb");
			meshImportNormalizeFormats(result);
			CAGE_TEST(result.parts.size() == 1);
			CAGE_TEST(!result.skeleton);
			CAGE_TEST(result.animations.empty());
			const auto &part = result.parts[0];
			CAGE_TEST(part.objectName == "textured_test");
			CAGE_TEST(part.textures.size() == 1);
			CAGE_TEST(part.textures[0].type == MeshImportTextureType::Albedo);
			CAGE_TEST(part.textures[0].images.parts.size() == 1);
			CAGE_TEST(part.textures[0].images.parts[0].image);
			CAGE_TEST(part.textures[0].images.parts[0].image->resolution() == albedo->resolution());
			Holder<Mesh> msh = part.mesh.share();
			CAGE_TEST(msh->facesCount() == p->facesCount());
			approxEqual(part.boundingBox, p->boundingBox());
		}
		{
			CAGE_TESTCASE("export with 3 textures");
			MeshExportConfig cfg;
			cfg.name = "textured_test_2";
			cfg.mesh = +p;
			cfg.albedo.image = +albedo;
			cfg.pbr.image = +pbr;
			cfg.normal.image = +normal;
			meshExportFiles("meshes/testExport3.glb", cfg);
		}
		{
			CAGE_TESTCASE("verify import");
			MeshImportResult result = meshImportFiles("meshes/testExport3.glb");
			meshImportNormalizeFormats(result);
			bool foundAlbedo = false, foundRoughness = false, foundMetallic = false, foundNormal = false;
			for (const auto &it : result.parts[0].textures)
			{
				switch (it.type)
				{
				case MeshImportTextureType::Albedo:
				{
					foundAlbedo = true;
					CAGE_TEST(it.images.parts[0].image->resolution() == albedo->resolution());
				} break;
				case MeshImportTextureType::Roughness:
				{
					foundRoughness = true;
					CAGE_TEST(it.images.parts[0].image->resolution() == pbr->resolution());
					const Real val = it.images.parts[0].image->get1(20, 20);
					approxEqual(val, Real(0.2));
				} break;
				case MeshImportTextureType::Metallic:
				{
					foundMetallic = true;
					CAGE_TEST(it.images.parts[0].image->resolution() == pbr->resolution());
					const Real val = it.images.parts[0].image->get1(20, 20);
					approxEqual(val, Real(0.3));
				} break;
				case MeshImportTextureType::Normal:
				{
					foundNormal = true;
					CAGE_TEST(it.images.parts[0].image->resolution() == pbr->resolution());
					const Vec3 val = it.images.parts[0].image->get3(20, 20);
					approxEqual(val * 10, Vec3(0.5, 0.5, 1) * 10);
				} break;
				}
			}
			CAGE_TEST(foundAlbedo && foundRoughness && foundMetallic && foundNormal);
		}
	}
}
