#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include "mesh.h"

#include <set>
#include <algorithm> // lower_bound
#include <numeric> // iota

namespace cage
{
	namespace
	{
		void writeLine(Serializer &ser, const String &s)
		{
			ser.writeLine(s);
		}

		Stringizer writeAttr(const Vec3 &v)
		{
			return Stringizer() + v[0] + " " + v[1] + " " + v[2];
		}

		Stringizer writeAttr(const Vec2 &v)
		{
			return Stringizer() + v[0] + " " + v[1];
		}

		template<class T>
		struct Attribute
		{
		private:
			const std::vector<T> &data;
			std::vector<T> vec;

			struct Cmp
			{
				bool operator ()(const T &a, const T &b) const
				{
					return detail::memcmp(&a, &b, sizeof(a)) < 0;
				}
			};

			uint32 index(const T &a) const
			{
				CAGE_ASSERT(valid(a));
				auto it = std::lower_bound(vec.begin(), vec.end(), a, Cmp());
				CAGE_ASSERT(it != vec.end());
				CAGE_ASSERT(*it == a);
				return numeric_cast<uint32>(it - vec.begin()) + 1;
			}

		public:
			Attribute(const std::vector<T> &data) : data(data)
			{
				std::set<T, Cmp> set(data.begin(), data.end());
				vec = std::vector<T>(set.begin(), set.end());
			}

			void writeOut(Serializer &ser, const String &prefix) const
			{
				for (const T &v : vec)
					writeLine(ser, Stringizer() + prefix + " " + writeAttr(v));
			}

			void remap(std::vector<uint32> &indices) const
			{
				for (uint32 &i : indices)
					i = index(data[i]);
			}
		};

		Stringizer str(const std::vector<uint32> &pi, const std::vector<uint32> &ni, const std::vector<uint32> &ti, uint32 index)
		{
			Stringizer ss;
			ss + pi[index];
			if (!ti.empty() || !ni.empty())
				ss + "/";
			if (!ti.empty())
				ss + ti[index];
			if (!ni.empty())
				ss + "/" + ni[index];
			return ss;
		}
	}

	Holder<PointerRange<char>> Mesh::exportObjBuffer(const MeshExportObjConfig &config) const
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(impl->uvs.empty() || impl->uvs3.empty());

		MemoryBuffer buffer;
		Serializer ser(buffer);
		writeLine(ser, "# mesh exported by Cage");
		if (!config.materialLibraryName.empty())
			writeLine(ser, Stringizer() + "mtllib " + config.materialLibraryName);
		if (!config.objectName.empty())
			writeLine(ser, Stringizer() + "o " + config.objectName);
		if (!config.materialName.empty())
			writeLine(ser, Stringizer() + "usemtl " + config.materialName);

		Attribute<Vec3> pos(impl->positions);
		pos.writeOut(ser, "v");
		Attribute<Vec3> norms(impl->normals);
		norms.writeOut(ser, "vn");
		Attribute<Vec2> uvs(impl->uvs);
		uvs.writeOut(ser, "vt");
		Attribute<Vec3> uvs3(impl->uvs3);
		uvs3.writeOut(ser, "vt");

		std::vector<uint32> origIndices;
		if (impl->indices.empty())
		{
			origIndices.resize(impl->positions.size());
			std::iota(origIndices.begin(), origIndices.end(), (uint32)0);
		}
		else
			origIndices = impl->indices;

		std::vector<uint32> pi = origIndices;
		std::vector<uint32> ni;
		std::vector<uint32> ti;
		pos.remap(pi);
		if (!impl->normals.empty())
		{
			ni = origIndices;
			norms.remap(ni);
		}
		if (!impl->uvs.empty())
		{
			ti = origIndices;
			uvs.remap(ti);
		}
		if (!impl->uvs3.empty())
		{
			ti = origIndices;
			uvs3.remap(ti);
		}
		CAGE_ASSERT(ni.empty() || ni.size() == pi.size());
		CAGE_ASSERT(ti.empty() || ti.size() == pi.size());

		switch (impl->type)
		{
		case MeshTypeEnum::Points:
		{
			const uint32 faces = numeric_cast<uint32>(pi.size());
			for (uint32 f = 0; f < faces; f++)
				writeLine(ser, Stringizer() + "f " + str(pi, ni, ti, f));
		} break;
		case MeshTypeEnum::Lines:
		{
			const uint32 faces = numeric_cast<uint32>(pi.size()) / 2;
			for (uint32 f = 0; f < faces; f++)
				writeLine(ser, Stringizer() + "f " + str(pi, ni, ti, f * 2 + 0) + " " + str(pi, ni, ti, f * 2 + 1));
		} break;
		case MeshTypeEnum::Triangles:
		{
			const uint32 faces = numeric_cast<uint32>(pi.size()) / 3;
			for (uint32 f = 0; f < faces; f++)
				writeLine(ser, Stringizer() + "f " + str(pi, ni, ti, f * 3 + 0) + " " + str(pi, ni, ti, f * 3 + 1) + " " + str(pi, ni, ti, f * 3 + 2));
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid mesh type enum");
		}

		return std::move(buffer);
	}

	void Mesh::exportObjFile(const MeshExportObjConfig &config, const String &filename) const
	{
		MeshImpl *impl = (MeshImpl *)this;
		Holder<PointerRange<char>> buff = exportObjBuffer(config);
		writeFile(filename)->write(buff);
	}
}
