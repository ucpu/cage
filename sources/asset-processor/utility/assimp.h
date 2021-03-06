#include "../processor.h"

#include <assimp/scene.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

struct CmpAiStr
{
	bool operator ()(const aiString &a, const aiString &b) const
	{
		if (a.length == b.length)
			return cage::detail::memcmp(a.data, b.data, a.length) < 0;
		return a.length < b.length;
	}
};

class AssimpSkeleton
{
public:
	uint16 bonesCount() const;
	aiNode *node(aiBone *bone) const;
	aiNode *node(uint16 index) const;
	aiBone *bone(aiNode *node) const;
	aiBone *bone(uint16 index) const;
	uint16 index(const aiString &name) const;
	uint16 index(aiBone *bone) const;
	uint16 index(aiNode *node) const;
	uint16 parent(uint16 index) const;
	aiBone *parent(aiBone *bone) const;
	aiNode *parent(aiNode *node) const; // this may skip some nodes in the original hierarchy - this function returns node that corresponds to another bone
};

class AssimpContext
{
public:
	const aiScene *getScene() const;
	uint32 selectModel() const;
	Holder<AssimpSkeleton> skeleton() const;
	Holder<SkeletonRig> skeletonRig() const;
	Holder<SkeletalAnimation> animation(uint32 index) const;
};

Holder<AssimpContext> newAssimpContext(uint32 addFlags, uint32 removeFlags);

vec3 conv(const aiVector3D &v);
vec3 conv(const aiColor3D &v);
vec4 conv(const aiColor4D &v);
mat4 conv(const aiMatrix4x4 &v);
quat conv(const aiQuaternion &q);

mat3 axesMatrix();
mat3 axesScaleMatrix();
