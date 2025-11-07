#include <array>

#include "mesh.h"

#include <cage-core/containerSerialization.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/meshExport.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
		struct MeshHeader
		{
			std::array<char, 12> cageName = { "cageMesh" };
			uint32 version = 2;
			MeshTypeEnum type = (MeshTypeEnum)0;
		};

		Holder<PointerRange<char>> exportImpl(const Mesh *mesh)
		{
			const MeshImpl *impl = (const MeshImpl *)mesh;
			MemoryBuffer buff;
			Serializer ser(buff);
			MeshHeader header;
			header.type = impl->type;
			ser << header;
			ser << impl->positions;
			ser << impl->normals;
			ser << impl->boneIndices;
			ser << impl->boneWeights;
			ser << impl->uvs3;
			ser << impl->uvs;
			ser << impl->indices;
			return std::move(buff);
		}
	}

	void Mesh::importBuffer(PointerRange<const char> buffer)
	{
		MeshImpl *impl = (MeshImpl *)this;
		impl->clear();
		Deserializer des(buffer);
		MeshHeader header;
		des >> header;
		if (header.cageName != MeshHeader().cageName || header.version != MeshHeader().version)
			CAGE_THROW_ERROR(Exception, "invalid magic or version in mesh deserialization");
		impl->type = header.type;
		des >> impl->positions;
		des >> impl->normals;
		des >> impl->boneIndices;
		des >> impl->boneWeights;
		des >> impl->uvs3;
		des >> impl->uvs;
		des >> impl->indices;
		CAGE_ASSERT(des.available() == 0);
	}

	Holder<PointerRange<char>> Mesh::exportBuffer(const String &format) const
	{
		const String ext = toLower(pathExtractExtension(format));
		if (ext == ".cagemesh")
			return exportImpl(this);
		if (ext == ".obj")
		{
			MeshExportObjConfig cfg;
			cfg.mesh = this;
			return meshExportBuffer(cfg);
		}
		if (ext == ".glb")
		{
			MeshExportGltfConfig cfg;
			cfg.mesh = this;
			return meshExportBuffer(cfg);
		}
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for mesh encoding");
	}

	void Mesh::exportFile(const String &filename) const
	{
		if (filename.empty())
			CAGE_THROW_ERROR(Exception, "file name cannot be empty");
		Holder<PointerRange<char>> buff = exportBuffer(filename);
		Holder<File> f = writeFile(filename);
		f->write(buff);
		f->close();
	}
}
