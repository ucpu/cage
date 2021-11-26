#ifndef guard_model_h_sdx54gfgh24jksd5f4
#define guard_model_h_sdx54gfgh24jksd5f4

#include <cage-core/meshMaterial.h>

#include "core.h"

namespace cage
{
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
		void setTextureNames(PointerRange<const uint32> textureNames);
		void setTextureName(uint32 index, uint32 name);
		void setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);

		uint32 verticesCount() const;
		uint32 indicesCount() const;
		uint32 primitivesCount() const;
		Aabb boundingBox() const;
		PointerRange<const uint32> textureNames() const;
		uint32 textureName(uint32 index) const;

		void dispatch() const;
		void dispatch(uint32 instances) const;

		MeshRenderFlags flags = MeshRenderFlags::None;
		uint32 skeletonBones = 0;
	};

	CAGE_ENGINE_API Holder<Model> newModel();

	CAGE_ENGINE_API AssetScheme genAssetSchemeModel(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexModel = 12;
}

#endif // guard_model_h_sdx54gfgh24jksd5f4
