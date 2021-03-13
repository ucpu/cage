#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

#include "utility/assimp.h"

#include <vector>
#include <map>

namespace
{
	void printHierarchy(AssimpSkeleton *skeleton, aiNode *n, uint32 offset)
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
			detail += string() + "has transform matrix";
		}
		if (skeleton->index(n) != m)
		{
			if (!detail.empty())
				detail += ", ";
			aiBone *b = skeleton->bone(n);
			if (b)
				detail += stringizer() + "weights: " + b->mNumWeights;
			else
				detail += string("no bone");
		}
		CAGE_LOG_CONTINUE(SeverityEnum::Info, logComponentName, fill(string(), offset, '\t') + detail);
		for (uint32 i = 0; i < n->mNumChildren; i++)
			printHierarchy(skeleton, n->mChildren[i], offset + 1);
	}
}

void processSkeleton()
{
	Holder<AssimpContext> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();
	Holder<AssimpSkeleton> skeleton = context->skeleton();

	const mat4 axesScale = mat4(axesScaleMatrix());
	const mat4 axesScaleInv = inverse(axesScale);

	const mat4 globalInverse = inverse(conv(scene->mRootNode->mTransformation)) * axesScale;
	const uint32 bonesCount = skeleton->bonesCount();

	std::vector<uint16> ps;
	std::vector<mat4> bs;
	std::vector<mat4> is;
	ps.reserve(bonesCount);
	bs.reserve(bonesCount);
	is.reserve(bonesCount);

	// print the nodes hierarchy
	CAGE_LOG(SeverityEnum::Info, logComponentName, "full node hierarchy:");
	printHierarchy(skeleton.get(), scene->mRootNode, 0);

	// find parents and matrices
	for (uint32 i = 0; i < bonesCount; i++)
	{
		const aiNode *n = skeleton->node(i);
		const aiBone *b = skeleton->bone(i);
		const mat4 t = conv(n->mTransformation);
		const mat4 o = (b ? conv(b->mOffsetMatrix) : mat4()) * axesScaleInv;
		CAGE_ASSERT(t.valid() && o.valid());
		ps.push_back(skeleton->parent(i));
		bs.push_back(t);
		is.push_back(o);
	}

	MemoryBuffer buff;
	Serializer ser(buff);
	ser << globalInverse;
	ser << bonesCount;
	// parent bone indices
	ser.write(bufferCast<char, uint16>(ps));
	// base transformation matrices
	ser.write(bufferCast<char, mat4>(bs));
	// inverted rest matrices
	ser.write(bufferCast<char, mat4>(is));

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = numeric_cast<uint32>(buff.size());
	h.compressedSize = numeric_cast<uint32>(comp.size());
	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
