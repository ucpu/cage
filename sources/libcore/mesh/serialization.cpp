#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/files.h>
#include <cage-core/string.h>
#include <cage-core/macros.h>
#include <cage-core/meshExport.h>
#include "mesh.h"

namespace cage
{
	namespace
	{
		enum class AttrFlags : uint32
		{
			none = 0,
			positions = 1 << 0,
			normals = 1 << 1,
			tangents = 1 << 2,
			bitangents = 1 << 3,
			uvs = 1 << 4,
			uvs3 = 1 << 5,
			boneIndices = 1 << 6,
			boneWeights = 1 << 7,
		};
	}

	GCHL_ENUM_BITS(AttrFlags);

	namespace
	{
		AttrFlags attrFlags(const MeshImpl *impl)
		{
			AttrFlags flags = AttrFlags::none;
#define GCHL_GENERATE(NAME) if (!impl->NAME.empty()) flags |= AttrFlags::NAME;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
			return flags;
		}

		constexpr uint64 Magic = uint64('c') + (uint64('a') << 8) + (uint64('g') << 16) + (uint64('e') << 24)
			+ (uint64('m') << 32) + (uint64('e') << 40) + (uint64('s') << 48) + (uint64('h') << 56);
		constexpr uint32 Version = 1;

		Holder<PointerRange<char>> exportImpl(const Mesh *mesh)
		{
			const MeshImpl *impl = (const MeshImpl *)mesh;
			MemoryBuffer buff;
			Serializer ser(buff);
			const AttrFlags flags = attrFlags(impl);
			const uint32 flagsi = (uint32)flags, type = (uint32)impl->type, vs = impl->verticesCount(), is = impl->indicesCount();
			ser << Magic << Version << flagsi << type << vs << is;
#define GCHL_GENERATE(NAME) if (any(flags & AttrFlags::NAME)) { ser.write(bufferCast<const char, std::remove_reference<decltype(impl->NAME[0])>::type>(impl->NAME)); }
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
			ser.write(bufferCast<const char, const uint32>(impl->indices));
			return std::move(buff);
		}
	}

	void Mesh::importBuffer(PointerRange<const char> buffer)
	{
		MeshImpl *impl = (MeshImpl *)this;
		impl->clear();
		Deserializer des(buffer);
		uint64 magic;
		uint32 ver, flagsi, type, vs, is;
		des >> magic >> ver >> flagsi >> type >> vs >> is;
		if (magic != Magic || ver != Version)
			CAGE_THROW_ERROR(Exception, "invalid magic or version in mesh deserialization");
		impl->type = (MeshTypeEnum)type;
		const AttrFlags flags = (AttrFlags)flagsi;
#define GCHL_GENERATE(NAME) if (any(flags & AttrFlags::NAME)) { impl->NAME.resize(vs); des.read(bufferCast<char, std::remove_reference<decltype(impl->NAME[0])>::type>(impl->NAME)); }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		impl->indices.resize(is);
		des.read(bufferCast<char, uint32>(impl->indices));
		if (des.available() != 0)
			CAGE_THROW_ERROR(Exception, "deserialization left unread data");
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
