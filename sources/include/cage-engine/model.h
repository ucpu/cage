#ifndef guard_model_dthu41r5df
#define guard_model_dthu41r5df

#include <array>

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace cage
{
	class Mesh;
	class Collider;
	class GraphicsDevice;
	class GpuBuffer;
	enum class MeshRenderFlags : uint32;

	class CAGE_ENGINE_API Model : private Immovable
	{
	protected:
		detail::StringBase<128> label;

	public:
		void setLabel(const String &name);

		// general
		Mat4 importTransform;
		Aabb boundingBox = Aabb::Universe();
		Holder<const Collider> collider;

		// geometry
		Holder<GpuBuffer> geometryBuffer;
		uint32 verticesCount = 0;
		uint32 bonesCount = 0;
		uint32 indicesCount = 0;
		uint32 indicesOffset = 0;
		uint32 primitivesCount = 0;
		uint32 primitiveType = 0;

		// material
		Holder<GpuBuffer> materialBuffer;
		std::array<uint32, MaxTexturesCountPerMaterial> textureNames = {};
		uint32 shaderName = 0;
		sint32 layer = 0;
		MeshRenderFlags flags = {};
	};

	CAGE_ENGINE_API Holder<Model> newModel();
	CAGE_ENGINE_API Holder<Model> newModel(GraphicsDevice *device, const Mesh *mesh, PointerRange<const char> material);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeModel(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexModel = 12;
}

#endif // guard_model_dthu41r5df
