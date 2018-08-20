namespace cage
{
	CAGE_API memoryBuffer entitiesSerialize(groupClass *entities, componentClass *component);
	CAGE_API void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager);
	CAGE_API void entitiesDeserialize(const void *buffer, uintPtr size, entityManagerClass *manager);
}
