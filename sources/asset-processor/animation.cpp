#include <cage-core/skeletalAnimation.h>

#include "utility/assimp.h"

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

	Holder<SkeletalAnimation> anim = context->animation(chosenAnimationIndex);
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
