#include "utility/assimp.h"
#include <cage-core/collisionMesh.h>
#include <cage-core/memoryBuffer.h>

void processCollider()
{
	Holder<AssimpContext> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectMesh()];

	switch (am->mPrimitiveTypes)
	{
	case aiPrimitiveType_TRIANGLE: break;
	default: CAGE_THROW_ERROR(Exception, "collider works with triangles only");
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loaded triangles: " + am->mNumFaces);

	Holder<CollisionMesh> collider = newCollisionMesh();
	mat3 axesScale = axesScaleMatrix();
	for (uint32 i = 0; i < am->mNumFaces; i++)
	{
		triangle tri;
		tri = triangle(vec3(), vec3(), vec3());
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
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "degenerated triangles: " + deg);
		}
	}

	collider->rebuild();

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "aabb: " + collider->box());

	MemoryBuffer buff = collider->serialize();

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
	MemoryBuffer comp = detail::compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = numeric_cast<uint32>(buff.size());
	h.compressedSize = numeric_cast<uint32>(comp.size());
	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(&h, sizeof(h));
	f->write(comp.data(), comp.size());
	f->close();
}
