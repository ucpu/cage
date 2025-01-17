#include "processor.h"

#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>

MeshImportConfig meshImportConfig();
void meshImportNotifyUsedFiles(const MeshImportResult &result);

void processSkeleton()
{
	MeshImportResult result = meshImportFiles(processor->inputFileName, meshImportConfig());
	meshImportNotifyUsedFiles(result);
	if (!result.skeleton)
		CAGE_THROW_ERROR(Exception, "loaded no skeleton");

	Holder<SkeletonRig> rig = result.skeleton.share();
	Holder<PointerRange<char>> buff = rig->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = memoryCompress(buff);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = processor->initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(processor->outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
