#include "mesh.h"

#include <cage-core/meshAlgorithms.h>
#include <cstddef> // fix missing size_t in xatlas
#include <cstdarg> // va_start
#include <cstdio> // vsprintf
#include <xatlas.h>

namespace cage
{
	namespace
	{
		int xAtlasPrint(const char *format, ...)
		{
			char buffer[1000];
			va_list arg;
			va_start(arg, format);
			auto result = vsprintf(buffer, format, arg);
			va_end(arg);
			CAGE_LOG_DEBUG(SeverityEnum::Warning, "xatlas", buffer);
			return result;
		}

		struct Initializer
		{
			Initializer()
			{
				xatlas::SetPrint(&xAtlasPrint, false);
			}
		} initializer;

		constexpr const String xAtlasCategoriesNames[] = {
			"AddModel",
			"ComputeCharts",
			"PackCharts",
			"BuildOutputModeles"
		};

		bool xAtlasProgress(xatlas::ProgressCategory category, int progress, void *userData)
		{
			CAGE_LOG(SeverityEnum::Info, "xatlas", Stringizer() + xAtlasCategoriesNames[(int)category] + ": " + progress + " %");
			return true; // continue processing
		}

		Holder<xatlas::Atlas> newAtlas(bool reportProgress)
		{
			struct Atl : private Immovable
			{
				xatlas::Atlas *a = nullptr;
				Atl()
				{
					a = xatlas::Create();
				}
				~Atl()
				{
					xatlas::Destroy(a);
				}
			};
			Holder<Atl> h = systemMemory().createHolder<Atl>();
			if (reportProgress)
				xatlas::SetProgressCallback(h->a, &xAtlasProgress);
			return Holder<xatlas::Atlas>(h->a, std::move(h));
		}
	}

	uint32 meshUnwrap(Mesh *poly, const MeshUnwrapConfig &config)
	{
		if (poly->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "unwrap requires triangles mesh");
		if ((config.targetResolution == 0) == (config.texelsPerUnit == 0))
			CAGE_THROW_ERROR(Exception, "unwrap requires exactly one of targetResolution or texelsPerUnit");
		if (poly->facesCount() == 0)
			return 0;

		const bool useNormals = !poly->normals().empty();
		Holder<xatlas::Atlas> atlas = newAtlas(config.logProgress);

		{
			xatlas::MeshDecl decl;
			decl.vertexCount = poly->verticesCount();
			decl.vertexPositionData = poly->positions().data();
			decl.vertexPositionStride = sizeof(Vec3);
			if (useNormals)
			{
				decl.vertexNormalData = poly->normals().data();
				decl.vertexNormalStride = sizeof(Vec3);
			}
			if (poly->indicesCount())
			{
				decl.indexFormat = xatlas::IndexFormat::UInt32;
				decl.indexCount = poly->indicesCount();
				decl.indexData = poly->indices().data();
			}
			xatlas::AddMesh(atlas.get(), decl);
		}

		{
			xatlas::ChartOptions chart;
			chart.maxChartArea = config.maxChartArea.value;
			chart.maxBoundaryLength = config.maxChartBoundaryLength.value;
			chart.normalDeviationWeight = config.chartNormalDeviation.value;
			chart.roundnessWeight = config.chartRoundness.value;
			chart.straightnessWeight = config.chartStraightness.value;
			chart.normalSeamWeight = config.chartNormalSeam.value;
			chart.textureSeamWeight = config.chartTextureSeam.value;
			chart.maxCost = config.chartGrowThreshold.value;
			chart.maxIterations = config.maxChartIterations;
			xatlas::ComputeCharts(atlas.get(), chart);
		}

		{
			xatlas::PackOptions pack;
			pack.bilinear = true;
			pack.blockAlign = true;
			pack.padding = config.padding;
			pack.texelsPerUnit = config.texelsPerUnit.value;
			pack.resolution = config.targetResolution;
			xatlas::PackCharts(atlas.get(), pack);
			CAGE_ASSERT(atlas->meshCount == 1);
			CAGE_ASSERT(atlas->atlasCount == 1);
		}

		{
			const xatlas::Mesh *m = atlas->meshes;
			std::vector<Vec3> vs;
			std::vector<Vec3> ns;
			std::vector<Vec2> us;
			// todo copy all other attributes too
			vs.reserve(m->vertexCount);
			if (useNormals)
				ns.reserve(m->vertexCount);
			us.reserve(m->vertexCount);
			const Vec2 whInv = 1 / Vec2(atlas->width, atlas->height);
			for (uint32 i = 0; i < m->vertexCount; i++)
			{
				const xatlas::Vertex &a = m->vertexArray[i];
				vs.push_back(poly->position(a.xref));
				if (useNormals)
					ns.push_back(poly->normal(a.xref));
				us.push_back(Vec2(a.uv[0], a.uv[1]) * whInv);
			}

			poly->clear();
			poly->positions(vs);
			if (useNormals)
				poly->normals(ns);
			poly->uvs(us);
			poly->indices({ m->indexArray, m->indexArray + m->indexCount });
			return atlas->width;
		}
	}
}
