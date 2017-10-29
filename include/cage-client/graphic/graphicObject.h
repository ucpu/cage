namespace cage
{
	class CAGE_API objectClass
	{
	public:
		uint32 shadower;
		uint32 collider;
		real worldSize;
		real pixelsSize;

		void setLodLevels(uint32 count);
		void setLodThreshold(uint32 lod, float threshold);
		void setLodMeshes(uint32 lod, uint32 count);
		void setMeshName(uint32 lod, uint32 index, uint32 name);

		uint32 lodsCount() const;
		real lodsThreshold(uint32 lod) const;
		uint32 meshesCount(uint32 lod) const;
		uint32 meshesName(uint32 lod, uint32 index) const;
		templates::pointerRange<const uint32> meshes(uint32 lod) const;
	};

	CAGE_API holder<objectClass> newObject();
}
