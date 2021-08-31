#include <cage-core/collider.h>

#include "utility/assimp.h"

void processCollider()
{
	Holder<AssimpContext> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectModel()];

	switch (am->mPrimitiveTypes)
	{
	case aiPrimitiveType_TRIANGLE: break;
	default: CAGE_THROW_ERROR(Exception, "collider works with triangles only");
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loaded triangles: " + am->mNumFaces);

	Holder<Collider> collider = newCollider();
	Mat3 axesScale = axesScaleMatrix();
	for (uint32 i = 0; i < am->mNumFaces; i++)
	{
		Triangle tri;
		tri = Triangle(Vec3(), Vec3(), Vec3());
		for (uint32 j = 0; j < 3; j++)
		{
			uint32 idx = numeric_cast<uint32>(am->mFaces[i].mIndices[j]);
			tri[j] = axesScale * conv(am->mVertices[idx]);
		}
		if (!tri.degenerated())
			collider->addTriangle(tri);
	}

	{ // count degenerated
		uint32 deg = numeric_cast<uint32>(am->mNumFaces - collider->triangles().size());
		if (deg)
		{
			CAGE_LOG(SeverityEnum::Warning, logComponentName, Stringizer() + "degenerated triangles: " + deg);
		}
	}

	collider->rebuild();

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
