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
	uint16 skeletonBonesCacheSize();
	const uint16 *skeletonBonesCacheParents() const;
	aiNode *skeletonBonesCacheNode(uint32 index) const;
	uint16 skeletonBonesCacheIndex(const aiString &name) const;
};

holder<assimpContextClass> newAssimpContext(uint32 flags);

extern const uint32 assimpDefaultLoadFlags;
