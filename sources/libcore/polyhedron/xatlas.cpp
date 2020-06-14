#include <cstddef> // fix missing size_t in xatlas
#include <cstdarg> // va_start
#include <cstdio> // vsprintf
#include <xatlas.h>

#include "polyhedron.h"

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

		const string xAtlasCategoriesNames[] = {
			"AddMesh",
			"ComputeCharts",
			"ParameterizeCharts",
			"PackCharts",
			"BuildOutputMeshes"
		};

		bool xAtlasProgress(xatlas::ProgressCategory::Enum category, int progress, void *userData)
		{
			CAGE_LOG(SeverityEnum::Info, "xatlas", stringizer() + xAtlasCategoriesNames[category] + ": " + progress + " %");
			return true; // continue processing
		}

		void destroyAtlas(void *ptr)
		{
			xatlas::Destroy((xatlas::Atlas*)ptr);
		}

		Holder<xatlas::Atlas> newAtlas(bool reportProgress)
		{
			xatlas::Atlas *a = xatlas::Create();
			if (reportProgress)
				xatlas::SetProgressCallback(a, &xAtlasProgress);
			return Holder<xatlas::Atlas>(a, a, Delegate<void(void*)>().bind<&destroyAtlas>());
		}
	}

	uint32 Polyhedron::unwrap(const PolyhedronUnwrapConfig &config)
	{
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Triangles);
		CAGE_ASSERT((config.targetResolution == 0) != (config.texelsPerUnit == 0));

		if (facesCount() == 0)
			return 0;

		const bool useNormals = !normals().empty();
		Holder<xatlas::Atlas> atlas = newAtlas(config.logProgress);

		{
			xatlas::MeshDecl decl;
			decl.vertexCount = verticesCount();
			decl.vertexPositionData = positions().data();
			decl.vertexPositionStride = sizeof(vec3);
			if (useNormals)
			{
				decl.vertexNormalData = normals().data();
				decl.vertexNormalStride = sizeof(vec3);
			}
			if (indicesCount())
			{
				decl.indexFormat = xatlas::IndexFormat::UInt32;
				decl.indexCount = indicesCount();
				decl.indexData = indices().data();
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
			xatlas::ParameterizeCharts(atlas.get());
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
			std::vector<vec3> vs;
			std::vector<vec3> ns;
			std::vector<vec2> us;
			// todo copy all other attributes too
			vs.reserve(m->vertexCount);
			if (useNormals)
				ns.reserve(m->vertexCount);
			us.reserve(m->vertexCount);
			const vec2 whInv = 1 / vec2(atlas->width, atlas->height);
			for (uint32 i = 0; i < m->vertexCount; i++)
			{
				const xatlas::Vertex &a = m->vertexArray[i];
				vs.push_back(position(a.xref));
				if (useNormals)
					ns.push_back(normal(a.xref));
				us.push_back(vec2(a.uv[0], a.uv[1]) * whInv);
			}

			clear();
			positions(vs);
			if (useNormals)
				normals(ns);
			uvs(us);
			indices({ m->indexArray, m->indexArray + m->indexCount });
			return atlas->width;
		}
	}
}
