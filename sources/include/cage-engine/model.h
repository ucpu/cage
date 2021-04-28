#ifndef guard_model_h_sdx54gfgh24jksd5f4
#define guard_model_h_sdx54gfgh24jksd5f4

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API Model : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void importMesh(const Mesh *poly, PointerRange<const char> materialBuffer);

		void setFlags(ModelRenderFlags flags);
		void setPrimitiveType(uint32 type);
		void setBoundingBox(const Aabb &box);
		void setTextureNames(PointerRange<const uint32> textureNames);
		void setTextureName(uint32 index, uint32 name);
		void setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);
		void setSkeleton(uint32 name, uint32 bones);
		void setInstancesLimitHint(uint32 limit);

		uint32 getVerticesCount() const;
		uint32 getIndicesCount() const;
		uint32 getPrimitivesCount() const;
		ModelRenderFlags getFlags() const;
		Aabb getBoundingBox() const;
		PointerRange<const uint32> getTextureNames() const;
		uint32 getTextureName(uint32 index) const;
		uint32 getSkeletonName() const;
		uint32 getSkeletonBones() const;
		uint32 getInstancesLimitHint() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;
	};

	CAGE_ENGINE_API Holder<Model> newModel();

	CAGE_ENGINE_API AssetScheme genAssetSchemeModel(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexModel = 12;
}

#endif // guard_model_h_sdx54gfgh24jksd5f4
