#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h> // addToAlign
#include <cage-core/mesh.h>
#include <cage-core/meshIoCommon.h>
#include <cage-core/serialization.h>
#include <cage-engine/gpuBuffer.h>
#include <cage-engine/model.h>

namespace cage
{
	namespace
	{
		class ModelImpl : public Model
		{
		public:
		};
	}

	void Model::setLabel(const String &name)
	{
		label = name;
		ModelImpl *impl = (ModelImpl *)this;
		if (geometryBuffer)
			geometryBuffer->setLabel(name);
		if (materialBuffer)
			materialBuffer->setLabel(name);
	}

	Holder<Model> newModel()
	{
		return systemMemory().createImpl<Model, ModelImpl>();
	}

	Holder<Model> newModel(GraphicsDevice *device, const Mesh *mesh, PointerRange<const char> material)
	{
		Holder<Model> mod = newModel();

		// geometry
		mod->verticesCount = mesh->verticesCount();
		mod->indicesCount = mesh->indicesCount();
		mod->primitivesCount = mesh->facesCount();
		switch (mesh->type())
		{
			case MeshTypeEnum::Points:
				mod->primitiveType = 1;
				break;
			case MeshTypeEnum::Lines:
				mod->primitiveType = 2;
				break;
			case MeshTypeEnum::Triangles:
				mod->primitiveType = 3;
				break;
		}
		mod->geometryBuffer = newGpuBufferGeometry(device);
		{
			const uint32 cnt = mesh->verticesCount();
			MemoryBuffer mem;
			mem.reserve(cnt * 32);
			Serializer ser(mem);
			PointerRange<const Vec3> ps = mesh->positions();
			PointerRange<const Vec3> ns = mesh->normals();
			PointerRange<const Vec4> bws = mesh->boneWeights();
			PointerRange<const Vec4i> bis = mesh->boneIndices();
			PointerRange<const Vec3> uv3s = mesh->uvs3();
			PointerRange<const Vec2> uv2s = mesh->uvs();
			for (uint32 i = 0; i < cnt; i++)
			{
				ser << ps[i];
				if (!ns.empty())
					ser << ns[i];
				if (!bws.empty())
					ser << bws[i];
				if (!bis.empty())
					ser << bis[i];
				if (!uv3s.empty())
					ser << uv3s[i];
				if (!uv2s.empty())
					ser << uv2s[i];
			}
			mod->indicesOffset = mem.size();
			for (uint32 i : mesh->indices())
				ser << i;
			mod->geometryBuffer->write(mem);
		}

		// material
		if (!material.empty())
		{
			mod->materialBuffer = newGpuBufferUniform(device);
			mod->materialBuffer->write(material);
		}
		mod->flags = MeshRenderFlags::Default;

		return mod;
	}
}
