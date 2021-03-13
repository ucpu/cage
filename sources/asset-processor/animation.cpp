#include <cage-core/skeletalAnimation.h>

#include "utility/assimp.h"

#include <vector>

namespace
{
	struct Bone
	{
		std::vector<real> posTimes;
		std::vector<vec3> posVals;
		std::vector<real> rotTimes;
		std::vector<quat> rotVals;
		std::vector<real> sclTimes;
		std::vector<vec3> sclVals;
	};
}

void processAnimation()
{
	Holder<AssimpContext> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();

	if (scene->mNumAnimations == 0)
		CAGE_THROW_ERROR(Exception, "no animations available");
	uint32 chosenAnimationIndex = m;
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
	if (chosenAnimationIndex == m)
		CAGE_THROW_ERROR(Exception, "no animation name matches the specifier");
	const aiAnimation *ani = scene->mAnimations[chosenAnimationIndex];
	if (ani->mNumChannels == 0 || ani->mNumMeshChannels != 0 || ani->mNumMorphMeshChannels != 0)
		CAGE_THROW_ERROR(Exception, "the animation has unsupported type");

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "duration: " + ani->mDuration + " ticks");
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "ticks per second: " + ani->mTicksPerSecond);

	Holder<AssimpSkeleton> skeleton = context->skeleton();

	const uint64 duration = numeric_cast<uint64>(1e6 * ani->mDuration / (ani->mTicksPerSecond > 0 ? ani->mTicksPerSecond : 25.0));
	const uint32 skeletonBonesCount = skeleton->bonesCount();

	uint32 totalKeys = 0;
	std::vector<Bone> bones;
	bones.reserve(ani->mNumChannels);
	std::vector<uint16> boneIndices;
	boneIndices.reserve(ani->mNumChannels);
	for (uint32 channelIndex = 0; channelIndex < ani->mNumChannels; channelIndex++)
	{
		aiNodeAnim *n = ani->mChannels[channelIndex];
		uint16 idx = skeleton->index(n->mNodeName);
		if (idx == m)
		{
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "channel index: " + channelIndex + ", name: '" + n->mNodeName.data + "', has no corresponding bone and will be ignored");
			continue;
		}
		boneIndices.push_back(idx);
		bones.emplace_back();
		Bone &b = *bones.rbegin();
		// positions
		b.posTimes.reserve(n->mNumPositionKeys);
		b.posVals.reserve(n->mNumPositionKeys);
		for (uint32 i = 0; i < n->mNumPositionKeys; i++)
		{
			aiVectorKey &k = n->mPositionKeys[i];
			b.posTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
			b.posVals.push_back(conv(k.mValue));
		}
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
		totalKeys += n->mNumScalingKeys;
	}
	const uint32 animationBonesCount = numeric_cast<uint32>(bones.size());
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "animated bones: " + animationBonesCount);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "total keys: " + totalKeys);

	Holder<SkeletalAnimation> anim = newSkeletalAnimation();
	anim->duration(duration);
	{
		std::vector<uint16> mapping;
		mapping.resize(skeletonBonesCount, m);
		for (uint16 i = 0; i < bones.size(); i++)
		{
			CAGE_ASSERT(mapping[boneIndices[i]] == m);
			mapping[boneIndices[i]] = i;
		}
		anim->channelsMapping(skeletonBonesCount, animationBonesCount, mapping);
	}
	{
		std::vector<PointerRange<const real>> times;
		times.reserve(animationBonesCount);
		{
			std::vector<PointerRange<const vec3>> positions;
			positions.reserve(animationBonesCount);
			for (const Bone &b : bones)
			{
				times.push_back(b.posTimes);
				positions.push_back(b.posVals);
			}
			anim->positionsData(times, positions);
			times.clear();
		}
		{
			std::vector<PointerRange<const quat>> rotations;
			rotations.reserve(animationBonesCount);
			for (const Bone &b : bones)
			{
				times.push_back(b.rotTimes);
				rotations.push_back(b.rotVals);
			}
			anim->rotationsData(times, rotations);
			times.clear();
		}
		{
			std::vector<PointerRange<const vec3>> scales;
			scales.reserve(animationBonesCount);
			for (const Bone &b : bones)
			{
				times.push_back(b.sclTimes);
				scales.push_back(b.sclVals);
			}
			anim->scaleData(times, scales);
			times.clear();
		}
	}

	Holder<PointerRange<char>> buff = anim->serialize();
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
