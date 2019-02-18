namespace cage
{
	class CAGE_API meshClass
	{
	public:
		uint32 getId() const;
		void bind() const;

		void setFlags(meshFlags flags);
		void setPrimitiveType(uint32 type);
		void setBoundingBox(const aabb &box);
		void setTextures(const uint32 *textureNames);
		void setBuffers(uint32 verticesCount, uint32 vertexSize, const void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, const void *materialData);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);
		void setSkeleton(uint32 name, uint32 bones);
		void setInstancesLimitHint(uint32 limit);

		uint32 getVerticesCount() const;
		uint32 getIndicesCount() const;
		uint32 getPrimitivesCount() const;
		meshFlags getFlags() const;
		aabb getBoundingBox() const;
		uint32 textureName(uint32 texIdx) const;
		uint32 getSkeletonName() const;
		uint32 getSkeletonBones() const;
		uint32 getInstancesLimitHint() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;
	};

	CAGE_API holder<meshClass> newMesh();
}
