#include <cage-core/skeletalAnimation.h>
#include <cage-core/hashString.h>

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

	const uint32 skeletonName = []() {
		string n = properties("skeleton");
		if (n.empty())
			n = inputFile + ";skeleton";
		else
			n = pathJoin(pathExtractDirectory(inputName), n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "using skeleton name: '" + n + "'");
		writeLine(string("ref = ") + n);
		return HashString(n);
	}();

	Holder<SkeletalAnimation> anim = context->animation(chosenAnimationIndex);
	anim->skeletonName(skeletonName);
	Holder<PointerRange<char>> buff = anim->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	h.dependenciesCount = 1;
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(bufferView(skeletonName));
	f->write(comp);
	f->close();
}
