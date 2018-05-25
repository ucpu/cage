#include "utility/assimp.h"

void processSkeleton()
{
	holder<assimpContextClass> context;
	const aiScene *scene = nullptr;

	{
		uint32 flags = assimpDefaultLoadFlags;
		context = newAssimpContext(flags);
		scene = context->getScene();
	}

	// todo
}
