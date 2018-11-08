#include <vector>
#include <map>

#include "utility/assimp.h"

namespace
{
	void printHierarchy(assimpSkeletonClass *skeleton, aiNode *n, uint32 offset)
	{
		string detail;
		if (n->mName.length)
		{
			if (!detail.empty())
				detail += ", ";
			detail += string() + "'" + n->mName.data + "'";
		}
		if (conv(n->mTransformation) != mat4())
		{
			if (!detail.empty())
				detail += ", ";
			detail += string() + "transform: " + conv(n->mTransformation);
		}
		if (skeleton->index(n) != (uint16)-1)
		{
			if (!detail.empty())
				detail += ", ";
			aiBone *b = skeleton->bone(n);
			if (b)
				detail += string() + "offset: " + conv(b->mOffsetMatrix) + ", weights: " + b->mNumWeights;
			else
				detail += string("no bone");
		}
		CAGE_LOG_CONTINUE(severityEnum::Info, logComponentName, string().fill(offset, '\t') + detail);
		for (uint32 i = 0; i < n->mNumChildren; i++)
			printHierarchy(skeleton, n->mChildren[i], offset + 1);
	}
}

void processSkeleton()
{
	holder<assimpContextClass> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();
	holder<assimpSkeletonClass> skeleton = context->skeleton();

	mat4 axesScale = mat4(axesScaleMatrix());
	mat4 axesScaleInv = axesScale.inverse();

	skeletonHeaderStruct s;
	s.globalInverse = conv(scene->mRootNode->mTransformation).inverse() * axesScale;
	s.bonesCount = skeleton->bonesCount();

	std::vector<uint16> ps;
	std::vector<mat4> bs;
	std::vector<mat4> is;
	ps.reserve(s.bonesCount);
	bs.reserve(s.bonesCount);
	is.reserve(s.bonesCount);

	// print the nodes hierarchy
	CAGE_LOG(severityEnum::Info, logComponentName, "full node hierarchy:");
	printHierarchy(skeleton.get(), scene->mRootNode, 0);

	// find parents and matrices
	for (uint32 i = 0; i < s.bonesCount; i++)
	{
		aiNode *n = skeleton->node(i);
		aiBone *b = skeleton->bone(i);
		mat4 t = conv(n->mTransformation);
		mat4 o = (b ? conv(b->mOffsetMatrix) : mat4()) * axesScaleInv;
		CAGE_ASSERT_RUNTIME(t.valid() && o.valid(), t, o);
		ps.push_back(skeleton->parent(i));
		bs.push_back(t);
		is.push_back(o);
	}

	assetHeaderStruct h = initializeAssetHeaderStruct();
	h.originalSize = sizeof(s) + s.bonesCount * (sizeof(uint16) + sizeof(mat4) + sizeof(mat4));

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	f->write(&s, sizeof(s));
	// parent bone indices
	f->write(ps.data(), s.bonesCount * sizeof(uint16));
	// base transformation matrices
	f->write(bs.data(), s.bonesCount * sizeof(mat4));
	// inverted rest matrices
	f->write(is.data(), s.bonesCount * sizeof(mat4));
}
