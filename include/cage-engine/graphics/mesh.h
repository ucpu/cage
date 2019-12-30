namespace cage
{
	class CAGE_API Mesh : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void setFlags(MeshRenderFlags flags);
		void setPrimitiveType(uint32 type);
		void setBoundingBox(const aabb &box);
		void setTextureNames(const uint32 *textureNames);
		void setTextureName(uint32 index, uint32 name);
		void setBuffers(uint32 verticesCount, uint32 vertexSize, const void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, const void *MaterialData);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);
		void setSkeleton(uint32 name, uint32 bones);
		void setInstancesLimitHint(uint32 limit);

		uint32 getVerticesCount() const;
		uint32 getIndicesCount() const;
		uint32 getPrimitivesCount() const;
		MeshRenderFlags getFlags() const;
		aabb getBoundingBox() const;
		const uint32 *getTextureNames() const;
		uint32 getTextureName(uint32 texIdx) const;
		uint32 getSkeletonName() const;
		uint32 getSkeletonBones() const;
		uint32 getInstancesLimitHint() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;
	};

	CAGE_API Holder<Mesh> newMesh();

	CAGE_API AssetScheme genAssetSchemeMesh(uint32 threadIndex, Window *memoryContext);
	static const uint32 assetSchemeIndexMesh = 12;
}
