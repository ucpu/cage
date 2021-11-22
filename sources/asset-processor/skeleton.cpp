#include <cage-core/skeletalAnimation.h>
#include <cage-core/meshImport.h>

#include "processor.h"

MeshImportConfig meshImportConfig(bool allowAxes);
void meshImportNotifyUsedFiles(const MeshImportResult &result);

void processSkeleton()
{
	const MeshImportResult result = meshImportFiles(inputFileName, meshImportConfig(true));
	meshImportNotifyUsedFiles(result);
	if (!result.skeleton)
		CAGE_THROW_ERROR(Exception, "loaded no skeleton");

	Holder<SkeletonRig> rig = result.skeleton.share();
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
