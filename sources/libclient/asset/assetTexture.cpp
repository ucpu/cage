#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
#include <cage-core/assetStructs.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include <cage-client/assetStructs.h>

namespace cage
{
	namespace
	{
		configUint32 textureDownscale("cage-client.graphics.textureDownscale", 1);

		void processLoad(const assetContext *context, void *schemePointer)
		{
			textureHeaderStruct *data = (textureHeaderStruct*)context->originalData;

			textureClass *tex = nullptr;
			if (context->assetHolder)
			{
				tex = static_cast<textureClass*>(context->assetHolder.get());
				CAGE_ASSERT_RUNTIME(tex->getTarget() == data->target, "texture target cannot change");
				tex->bind();
			}
			else
			{
				context->assetHolder = newTexture(data->target).cast<void>();
				tex = static_cast<textureClass*>(context->assetHolder.get());
				tex->setDebugName(context->textName);
			}
			context->returnData = tex;

			char *values = ((char*)context->originalData) + sizeof(textureHeaderStruct);

			{
				uint32 bytesSize = data->dimX * data->dimY * data->dimZ * data->bpp;
				CAGE_ASSERT_RUNTIME((values + bytesSize) == ((char*)context->originalData + context->originalSize), bytesSize, context->originalSize);
			}

			if (data->target == GL_TEXTURE_3D || data->target == GL_TEXTURE_2D_ARRAY)
				tex->image3d(data->dimX, data->dimY, data->dimZ, data->internalFormat, data->copyFormat, data->copyType, values);
			else if (data->target == GL_TEXTURE_CUBE_MAP)
				tex->imageCube(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values, data->stride);
			else
				tex->image2d(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values);
			tex->filters(data->filterMin, data->filterMag, data->filterAniso);
			tex->wraps(data->wrapX, data->wrapY, data->wrapZ);
			if ((data->flags & textureFlags::GenerateMipmaps) == textureFlags::GenerateMipmaps)
				tex->generateMipmaps();

			tex->animationDuration = data->animationDuration;
			tex->animationLoop = (data->flags & textureFlags::AnimationLoop) == textureFlags::AnimationLoop;
		}
	}

	assetScheme genAssetSchemeTexture(uint32 threadIndex, windowClass *memoryContext)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
