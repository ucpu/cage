#include "utility/assimp.h"

#include <vector>

namespace
{
	struct Bone
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
	uint32 size = 0;
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
	const uint32 animationBonesCount = numeric_cast<uint32>(bones.size());
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "animated bones: " + animationBonesCount);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "total keys: " + totalKeys);

	MemoryBuffer buff;
	Serializer ser(buff);
	ser << duration;
	ser << skeletonBonesCount;
	ser << animationBonesCount;

	// bone indices
	ser.write(bufferCast<char, uint16>(boneIndices));

	// position frames counts
	for (Bone &b : bones)
		ser << numeric_cast<uint16>(b.posTimes.size());

	// rotation frames counts
	for (Bone &b : bones)
		ser << numeric_cast<uint16>(b.rotTimes.size());

	// scale frames counts
	for (Bone &b : bones)
		ser << numeric_cast<uint16>(b.sclTimes.size());

	for (Bone &b : bones)
	{
		ser.write(bufferCast<char, float>(b.posTimes));
		ser.write(bufferCast<char, vec3>(b.posVals));
		ser.write(bufferCast<char, float>(b.rotTimes));
		ser.write(bufferCast<char, quat>(b.rotVals));
		ser.write(bufferCast<char, float>(b.sclTimes));
		ser.write(bufferCast<char, vec3>(b.sclVals));
	}

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
