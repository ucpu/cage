#ifndef guard_model_dthu41r5df
#define guard_model_dthu41r5df

#include <array>

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace wgpu
{
	struct VertexBufferLayout;
}

namespace cage
{
	class Mesh;
	class Collider;
	class GraphicsDevice;
	class GraphicsBuffer;
	enum class MeshRenderFlags : uint32;
	enum class TextureFlags : uint32;
	struct GraphicsBindings;

	class CAGE_ENGINE_API Model : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		AssetLabel getLabel() const;

		void updateLayout();
		const wgpu::VertexBufferLayout &getLayout() const;
		GraphicsBindings &bindings();

		// general
		Mat4 importTransform;
		Aabb boundingBox = Aabb::Universe();
		Holder<const Collider> collider;

		// geometry
		Holder<GraphicsBuffer> geometryBuffer;
		uint32 verticesCount = 0;
		uint32 bonesCount = 0;
		uint32 indicesCount = 0;
		uint32 indicesOffset = 0;
		uint32 primitivesCount = 0;
		uint32 primitiveType = 0;
		MeshComponentsFlags components = MeshComponentsFlags::None;

		// material
		Holder<GraphicsBuffer> materialBuffer;
		std::array<uint32, MaxTexturesCountPerMaterial> textureNames = {};
		uint32 shaderName = 0;
		sint32 renderLayer = 0;
		MeshRenderFlags renderFlags = {};
		TextureFlags texturesFlags = {};
	};

	CAGE_ENGINE_API Holder<Model> newModel(const AssetLabel &label);
	CAGE_ENGINE_API Holder<Model> newModel(GraphicsDevice *device, const Mesh *mesh, PointerRange<const char> material, const AssetLabel &label);

	CAGE_ENGINE_API MeshComponentsFlags meshComponentsFlags(const Mesh *mesh);
}

#endif // guard_model_dthu41r5df
