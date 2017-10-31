namespace cage
{
	CAGE_API uintPtr entitiesSerialize(void *buffer, uintPtr bufferSize, groupClass *entities, componentClass *component);
	CAGE_API memoryBuffer entitiesSerialize(groupClass *entities, componentClass *component);
	CAGE_API void entitiesDeserialize(const void *buffer, uintPtr bufferSize, entityManagerClass *manager);
	CAGE_API void entitiesDeserialize(const memoryBuffer &buffer, entityManagerClass *manager);
}
