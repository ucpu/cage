#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>

MeshImportConfig meshImportConfig();
void meshImportNotifyUsedFiles(const MeshImportResult &result);

void processAnimation()
{
	const MeshImportResult result = meshImportFiles(processor->inputFileName, meshImportConfig());
	meshImportNotifyUsedFiles(result);

	uint32 chosenAnimationIndex = m;
	if (result.animations.size() > 1 || !processor->inputSpec.empty())
	{
		for (uint32 i = 0; i < result.animations.size(); i++)
		{
			if (processor->inputSpec == result.animations[i].name)
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

	Holder<SkeletalAnimation> anim = result.animations[chosenAnimationIndex].animation.share();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "duration: " + anim->duration() + " microseconds");

	const uint32 skeletonName = []()
	{
		String n = processor->property("skeleton");
		if (n.empty())
			n = processor->inputFile + ";skeleton";
		else
			n = pathJoin(pathExtractDirectory(processor->inputName), n);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "using skeleton name: " + n);
		processor->writeLine(String("ref = ") + n);
		return HashString(n);
	}();
	anim->skeletonName(skeletonName);

	Holder<PointerRange<char>> buff = anim->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = memoryCompress(buff);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = processor->initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	h.dependenciesCount = 1;
	Holder<File> f = writeFile(processor->outputFileName);
	f->write(bufferView(h));
	f->write(bufferView(skeletonName));
	f->write(comp);
	f->close();
}
