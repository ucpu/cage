#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
#include <cage-core/assetStructs.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		ConfigUint32 textureDownscale("cage/graphics/textureDownscale", 1);

		void processLoad(const AssetContext *context, void *schemePointer)
		{
			TextureHeader *data = (TextureHeader*)context->originalData;

			Texture *tex = nullptr;
			if (context->assetHolder)
			{
				tex = static_cast<Texture*>(context->assetHolder.get());
				CAGE_ASSERT(tex->getTarget() == data->target, "texture target cannot change");
				tex->bind();
			}
			else
			{
				context->assetHolder = newTexture(data->target).cast<void>();
				tex = static_cast<Texture*>(context->assetHolder.get());
				tex->setDebugName(context->textName);
			}
			context->returnData = tex;

			char *values = ((char*)context->originalData) + sizeof(TextureHeader);

			{
				uint32 bytesSize = data->dimX * data->dimY * data->dimZ * data->bpp;
				CAGE_ASSERT((values + bytesSize) == ((char*)context->originalData + context->originalSize), bytesSize, context->originalSize);
			}

			if (data->target == GL_TEXTURE_3D || data->target == GL_TEXTURE_2D_ARRAY)
				tex->image3d(data->dimX, data->dimY, data->dimZ, data->internalFormat, data->copyFormat, data->copyType, values);
			else if (data->target == GL_TEXTURE_CUBE_MAP)
				tex->imageCube(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values, data->stride);
			else
				tex->image2d(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values);
			tex->filters(data->filterMin, data->filterMag, data->filterAniso);
			tex->wraps(data->wrapX, data->wrapY, data->wrapZ);
			if ((data->flags & TextureFlags::GenerateMipmaps) == TextureFlags::GenerateMipmaps)
				tex->generateMipmaps();

			tex->animationDuration = data->animationDuration;
			tex->animationLoop = (data->flags & TextureFlags::AnimationLoop) == TextureFlags::AnimationLoop;
		}
	}

	AssetScheme genAssetSchemeTexture(uint32 threadIndex, Window *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
