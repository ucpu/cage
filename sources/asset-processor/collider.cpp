#include "processor.h"

#include <cage-core/mesh.h>
#include <cage-core/collider.h>
#include <cage-core/meshImport.h>

MeshImportConfig meshImportConfig();
void meshImportTransform(MeshImportResult &result);
void meshImportNotifyUsedFiles(const MeshImportResult &result);
uint32 meshImportSelectIndex(const MeshImportResult &result);

void processCollider()
{
	MeshImportResult result = meshImportFiles(inputFileName, meshImportConfig());
	meshImportTransform(result);
	meshImportNotifyUsedFiles(result);
	const uint32 partIndex = meshImportSelectIndex(result);
	const MeshImportPart &part = result.parts[partIndex];

	if (part.mesh->type() != MeshTypeEnum::Triangles)
		CAGE_THROW_ERROR(Exception, "collider works with triangles only");

	Holder<Collider> collider = newCollider();
	collider->importMesh(+part.mesh);
	collider->rebuild();

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "triangles: " + collider->triangles().size());
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "aabb: " + collider->box());

	Holder<PointerRange<char>> buff = collider->exportBuffer();
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
