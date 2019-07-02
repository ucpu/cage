#include "utility/assimp.h"
#include <cage-core/collisionMesh.h>
#include <cage-core/memoryBuffer.h>

void processCollider()
{
	holder<assimpContextClass> context = newAssimpContext(0, 0);
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectMesh()];

	switch (am->mPrimitiveTypes)
	{
	case aiPrimitiveType_TRIANGLE: break;
	default: CAGE_THROW_ERROR(exception, "collider works with triangles only");
	}

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "loaded triangles: " + am->mNumFaces);

	holder<collisionMesh> collider = newCollisionMesh();
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
		uint32 deg = am->mNumFaces - collider->trianglesCount();
		if (deg)
		{
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "degenerated triangles: " + deg);
		}
	}

	collider->rebuild();

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "aabb: " + collider->box());

	memoryBuffer buff = collider->serialize();

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "buffer size (before compression): " + buff.size());
	memoryBuffer comp = detail::compress(buff);
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "buffer size (after compression): " + comp.size());

	assetHeader h = initializeAssetHeaderStruct();
	h.originalSize = numeric_cast<uint32>(buff.size());
	h.compressedSize = numeric_cast<uint32>(comp.size());
	holder<file> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	f->write(comp.data(), comp.size());
	f->close();
}
