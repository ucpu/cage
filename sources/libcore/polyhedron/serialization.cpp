#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/macros.h>
#include "polyhedron.h"

namespace cage
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

	GCHL_ENUM_BITS(AttrFlags);

	AttrFlags attrFlags(const PolyhedronImpl *impl)
	{
		AttrFlags flags = AttrFlags::none;
#define GCHL_GENERATE(NAME) if (!impl->NAME.empty()) flags |= AttrFlags::NAME;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		return flags;
	}

	constexpr uint32 Magic = uint32('p') + (uint32('o') << 8) + (uint32('l') << 16) + (uint32('y') << 24);
	constexpr uint32 Version = 1;

	void Polyhedron::deserialize(PointerRange<const char> buffer)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		impl->clear();
		Deserializer des(buffer);
		uint32 magic, ver, flagsi, type, vs, is;
		des >> magic >> ver >> flagsi >> type >> vs >> is;
		if (magic != Magic || ver != Version)
			CAGE_THROW_ERROR(Exception, "invalid magic or version in polyhedron deserialization");
		impl->type = (PolyhedronTypeEnum)type;
		const AttrFlags flags = (AttrFlags)flagsi;
#define GCHL_GENERATE(NAME) if (any(flags & AttrFlags::NAME)) { impl->NAME.resize(vs); des.read(bufferCast<char, std::remove_reference<decltype(impl->NAME[0])>::type>(impl->NAME)); }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		impl->indices.resize(is);
		des.read(bufferCast<char, uint32>(impl->indices));
		if (des.available() != 0)
			CAGE_THROW_ERROR(Exception, "deserialization left unread data");
	}

	MemoryBuffer Polyhedron::serialize() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		MemoryBuffer buff;
		Serializer ser(buff);
		const AttrFlags flags = attrFlags(impl);
		const uint32 flagsi = (uint32)flags, type = (uint32)impl->type, vs = impl->verticesCount(), is = impl->indicesCount();
		ser << Magic << Version << flagsi << type << vs << is;
#define GCHL_GENERATE(NAME) if (any(flags & AttrFlags::NAME)) { ser.write(bufferCast<const char, std::remove_reference<decltype(impl->NAME[0])>::type>(impl->NAME)); }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		ser.write(bufferCast<const char, const uint32>(impl->indices));
		return buff;
	}
}
