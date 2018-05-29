#include "utility/assimp.h"

#include <vector>

namespace
{
	struct bone
	{
		std::vector<float> posTimes;
		std::vector<vec3> posVals;
		std::vector<float> rotTimes;
		std::vector<quat> rotVals;
		std::vector<float> sclTimes;
		std::vector<vec3> sclVals;
	};
}

void processAnimation()
{
	holder<assimpContextClass> context;
	const aiScene *scene = nullptr;

	{
		uint32 flags = assimpDefaultLoadFlags;
		context = newAssimpContext(flags);
		scene = context->getScene();
	}

	if (scene->mNumAnimations == 0)
		CAGE_THROW_ERROR(exception, "no animations available");
	uint32 chosenAnimationIndex = -1;
	if (scene->mNumAnimations > 1 || !inputSpec.empty())
	{
		for (uint32 i = 0; i < scene->mNumAnimations; i++)
		{
			if (inputSpec == scene->mAnimations[i]->mName.data)
			{
				chosenAnimationIndex = i;
				break;
			}
		}
	}
	else
		chosenAnimationIndex = 0;
	if (chosenAnimationIndex == -1)
		CAGE_THROW_ERROR(exception, "no animation name matches the specifier");
	aiAnimation *ani = scene->mAnimations[chosenAnimationIndex];
	if (ani->mNumChannels == 0 || ani->mNumMeshChannels != 0 || ani->mNumMorphMeshChannels != 0)
		CAGE_THROW_ERROR(exception, "the animation is of wrong type");

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "duration: " + ani->mDuration + " ticks");
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "ticks per second: " + ani->mTicksPerSecond);

	holder<assimpSkeletonClass> skeleton = context->skeleton();

	animationHeaderStruct a;
	a.duration = numeric_cast<uint64>(1e6 * ani->mDuration / (ani->mTicksPerSecond > 0 ? ani->mTicksPerSecond : 25.0));
	a.skeletonBonesCount = skeleton->bonesCount();

	uint32 totalKeys = 0;
	uint32 size = 0;
	std::vector<bone> bones;
	bones.reserve(ani->mNumChannels);
	std::vector<uint16> boneIndices;
	boneIndices.reserve(ani->mNumChannels);
	for (uint32 channelIndex = 0; channelIndex < ani->mNumChannels; channelIndex++)
	{
		aiNodeAnim *n = ani->mChannels[channelIndex];
		uint16 idx = skeleton->index(n->mNodeName);
		if (idx == (uint16)-1)
		{
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "channel index: " + channelIndex + ", name: '" + n->mNodeName.data + "', has no corresponding bone and will be ignored");
			continue;
		}
		boneIndices.push_back(idx);
		bones.emplace_back();
		bone &b = *bones.rbegin();
		// positions
		b.posTimes.reserve(n->mNumPositionKeys);
		b.posVals.reserve(n->mNumPositionKeys);
		for (uint32 i = 0; i < n->mNumPositionKeys; i++)
		{
			aiVectorKey &k = n->mPositionKeys[i];
			b.posTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
			b.posVals.push_back(conv(k.mValue));
		}
		size += n->mNumPositionKeys * (sizeof(float) + sizeof(vec3));
		totalKeys += n->mNumPositionKeys;
		// rotations
		b.rotTimes.reserve(n->mNumRotationKeys);
		b.rotVals.reserve(n->mNumRotationKeys);
		for (uint32 i = 0; i < n->mNumRotationKeys; i++)
		{
			aiQuatKey &k = n->mRotationKeys[i];
			b.rotTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
			b.rotVals.push_back(conv(k.mValue));
		}
		size += n->mNumRotationKeys * (sizeof(float) + sizeof(quat));
		totalKeys += n->mNumRotationKeys;
		// scales
		b.sclTimes.reserve(n->mNumScalingKeys);
		b.sclVals.reserve(n->mNumScalingKeys);
		for (uint32 i = 0; i < n->mNumScalingKeys; i++)
		{
			aiVectorKey &k = n->mScalingKeys[i];
			b.sclTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
			b.sclVals.push_back(conv(k.mValue));
		}
		size += n->mNumScalingKeys * (sizeof(float) + sizeof(vec3));
		totalKeys += n->mNumScalingKeys;
	}
	a.animationBonesCount = numeric_cast<uint32>(bones.size());
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "animated bones: " + a.animationBonesCount);
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "total keys: " + totalKeys);

	assetHeaderStruct h = initializeAssetHeaderStruct();
	h.originalSize = sizeof(a) + a.animationBonesCount * sizeof(uint16) * 4 + size;

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	f->write(&a, sizeof(a));

	// bone indices
	f->write(boneIndices.data(), boneIndices.size() * sizeof(uint16));

	// position frames counts
	for (bone &b : bones)
	{
		uint16 c = numeric_cast<uint16>(b.posTimes.size());
		f->write(&c, sizeof(uint16));
	}

	// rotation frames counts
	for (bone &b : bones)
	{
		uint16 c = numeric_cast<uint16>(b.rotTimes.size());
		f->write(&c, sizeof(uint16));
	}

	// scale frames counts
	for (bone &b : bones)
	{
		uint16 c = numeric_cast<uint16>(b.sclTimes.size());
		f->write(&c, sizeof(uint16));
	}

	for (bone &b : bones)
	{
		f->write(b.posTimes.data(), b.posTimes.size() * sizeof(float));
		f->write(b.posVals.data(), b.posVals.size() * sizeof(vec3));
		f->write(b.rotTimes.data(), b.rotTimes.size() * sizeof(float));
		f->write(b.rotVals.data(), b.rotVals.size() * sizeof(quat));
		f->write(b.sclTimes.data(), b.sclTimes.size() * sizeof(float));
		f->write(b.sclVals.data(), b.sclVals.size() * sizeof(vec3));
	}
}
