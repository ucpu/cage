#ifndef guard_model_h_sdx54gfgh24jksd5f4
#define guard_model_h_sdx54gfgh24jksd5f4

#include <cage-engine/core.h>

namespace cage
{
	class Mesh;
	enum class MeshRenderFlags : uint32;

	class CAGE_ENGINE_API Model : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		uint32 id() const;
		void bind() const;

		void importMesh(const Mesh *poly, PointerRange<const char> materialBuffer);

		void setPrimitiveType(uint32 type);
		void setBoundingBox(const Aabb &box);
		void setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);

		uint32 verticesCount() const;
		uint32 indicesCount() const;
		uint32 primitivesCount() const;
		Aabb boundingBox() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;

		Mat4 importTransform;
		uint32 textureNames[MaxTexturesCountPerMaterial] = {};
		uint32 shaderName = 0;
		MeshRenderFlags flags = {};
		sint32 layer = 0;
		uint32 bones = 0;
	};

	CAGE_ENGINE_API Holder<Model> newModel();

	CAGE_ENGINE_API AssetsScheme genAssetSchemeModel(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexModel = 12;
}

#endif // guard_model_h_sdx54gfgh24jksd5f4
