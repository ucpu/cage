#include "../processor.h"

#include <assimp/scene.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

struct assimpContextClass
{
	const aiScene *getScene() const;
	uint32 selectMesh() const;
};

holder<assimpContextClass> newAssimpContext(uint32 flags);

