#include "../processor.h"

#define NULL 0
#include <assimp/scene.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#undef NULL

using namespace Assimp;

struct assimpContextClass
{
	const aiScene *getScene() const;
	uint32 selectMesh() const;
};

holder<assimpContextClass> newAssimpContext(uint32 flags);
