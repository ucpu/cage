#include <cage-core/skeletalAnimation.h>

#include "utility/assimp.h"

void processSkeleton()
{
	Holder<AssimpContext> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();

	Holder<SkeletonRig> rig = context->skeletonRig();
	Holder<PointerRange<char>> buff = rig->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
