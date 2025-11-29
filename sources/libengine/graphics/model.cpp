#include <array>

#include <webgpu/webgpu_cpp.h>

#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h> // addToAlign
#include <cage-core/mesh.h>
#include <cage-core/meshIoCommon.h>
#include <cage-core/serialization.h>
#include <cage-engine/graphicsBindings.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/model.h>

namespace cage
{
	namespace
	{
		class ModelImpl : public Model
		{
		public:
			std::array<wgpu::VertexAttribute, 5> attrs;
			wgpu::VertexBufferLayout layout = {};
			GraphicsBindings bindings;

			ModelImpl(const AssetLabel &label_) { this->label = label_; }

			void updateLayout()
			{
				attrs = {};
				layout = {};

				uint32 index = 0;
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Float32x3;
					attrs[index].shaderLocation = 0;
					index++;
					layout.arrayStride += sizeof(Vec3);
				}
				if (any(components & MeshComponentsFlags::Normals))
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Float32x3;
					attrs[index].shaderLocation = 1;
					index++;
					layout.arrayStride += sizeof(Vec3);
				}
				if (any(components & MeshComponentsFlags::Bones))
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Uint32x4;
					attrs[index].shaderLocation = 2;
					index++;
					layout.arrayStride += sizeof(Vec4i);
				}
				if (any(components & MeshComponentsFlags::Bones))
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Float32x4;
					attrs[index].shaderLocation = 3;
					index++;
					layout.arrayStride += sizeof(Vec4);
				}
				CAGE_ASSERT(any(components & MeshComponentsFlags::Uvs3) + any(components & MeshComponentsFlags::Uvs2) < 2)
				if (any(components & MeshComponentsFlags::Uvs3))
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Float32x3;
					attrs[index].shaderLocation = 4;
					index++;
					layout.arrayStride += sizeof(Vec3);
				}
				if (any(components & MeshComponentsFlags::Uvs2))
				{
					attrs[index].offset = layout.arrayStride;
					attrs[index].format = wgpu::VertexFormat::Float32x2;
					attrs[index].shaderLocation = 4;
					index++;
					layout.arrayStride += sizeof(Vec2);
				}
				CAGE_ASSERT(index <= attrs.size());

				layout.attributeCount = index;
				layout.attributes = attrs.data();
				layout.stepMode = wgpu::VertexStepMode::Vertex;
			}
		};
	}

	AssetLabel Model::getLabel() const
	{
		return label;
	}

	void Model::updateLayout()
	{
		ModelImpl *impl = (ModelImpl *)this;
		impl->updateLayout();
	}

	const wgpu::VertexBufferLayout &Model::getLayout() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		return impl->layout;
	}

	GraphicsBindings &Model::bindings()
	{
		ModelImpl *impl = (ModelImpl *)this;
		return impl->bindings;
	}

	Holder<Model> newModel(const AssetLabel &label)
	{
		return systemMemory().createImpl<Model, ModelImpl>(label);
	}

	Holder<Model> newModel(GraphicsDevice *device, const Mesh *mesh, PointerRange<const char> material, const AssetLabel &label)
	{
		Holder<Model> mod = newModel(label);

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
		{
			const uint32 cnt = mesh->verticesCount();
			MemoryBuffer mem;
			mem.reserve(cnt * 32);
			Serializer ser(mem);
			PointerRange<const Vec3> ps = mesh->positions();
			PointerRange<const Vec3> ns = mesh->normals();
			PointerRange<const Vec4i> bis = mesh->boneIndices();
			PointerRange<const Vec4> bws = mesh->boneWeights();
			PointerRange<const Vec3> uv3s = mesh->uvs3();
			PointerRange<const Vec2> uv2s = mesh->uvs();
			for (uint32 i = 0; i < cnt; i++)
			{
				ser << ps[i];
				if (!ns.empty())
					ser << ns[i];
				if (!bis.empty())
					ser << bis[i];
				if (!bws.empty())
					ser << bws[i];
				if (!uv3s.empty())
					ser << uv3s[i];
				if (!uv2s.empty())
					ser << uv2s[i];
			}
			mod->indicesOffset = mem.size();
			for (uint32 i : mesh->indices())
				ser << i;
			mod->geometryBuffer = newGraphicsBufferGeometry(device, mem.size(), label);
			mod->geometryBuffer->writeBuffer(mem);
		}
		mod->components = meshComponentsFlags(mesh);
		mod->updateLayout();

		// material
		if (!material.empty())
		{
			mod->materialBuffer = newGraphicsBuffer(device, material.size(), label);
			mod->materialBuffer->writeBuffer(material);
		}
		mod->renderFlags = MeshRenderFlags::Default;

		return mod;
	}

	MeshComponentsFlags meshComponentsFlags(const Mesh *mesh)
	{
		MeshComponentsFlags flags = MeshComponentsFlags::None;
		if (!mesh->normals().empty())
			flags |= MeshComponentsFlags::Normals;
		CAGE_ASSERT(mesh->boneWeights().empty() == mesh->boneIndices().empty());
		if (!mesh->boneWeights().empty())
			flags |= MeshComponentsFlags::Bones;
		if (!mesh->uvs3().empty())
			flags |= MeshComponentsFlags::Uvs3;
		if (!mesh->uvs().empty())
			flags |= MeshComponentsFlags::Uvs2;
		return flags;
	}
}
