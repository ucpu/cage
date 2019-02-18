#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>
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

		void processDecompress(const assetContextStruct *context, void *schemePointer)
		{
			static const uintPtr sth = sizeof(textureHeaderStruct);
			detail::memcpy(context->originalData, context->compressedData, sth);
			uintPtr res = detail::decompress((char*)context->compressedData + sth, numeric_cast<uintPtr>(context->compressedSize - sth), (char*)context->originalData + sth, numeric_cast<uintPtr>(context->originalSize - sth));
			CAGE_ASSERT_RUNTIME(res == context->originalSize - sth, res, context->originalSize, sth);
		}

		void processLoad(const assetContextStruct *context, void *schemePointer)
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
				context->assetHolder = newTexture(data->target).transfev();
				tex = static_cast<textureClass*>(context->assetHolder.get());
			}
			context->returnData = tex;

			char *values = ((char*)context->originalData) + sizeof(textureHeaderStruct);

			{
				uint32 bytesSize = data->dimX * data->dimY * data->dimZ * data->bpp;
				CAGE_ASSERT_RUNTIME((values + bytesSize) == ((char*)context->originalData + context->originalSize), bytesSize, context->originalSize);
			}

			/*
			{
				uint32 downscale = textureDownscale;
				while (downscale > 1)
				{
					uint32 ox = data->dimX;
					uint32 oy = data->dimY;
					uint32 oz = data->dimZ;
					uint32 nx = max(ox / 2u, 1u);
					uint32 ny = max(oy / 2u, 1u);
					uint32 nz = (data->target == GL_TEXTURE_3D) ? max(oz / 2u, 1u) : oz;

					for (uint32 z = 0; z < nz; z++)
					{
						for (uint32 y = 0; y < ny; y++)
						{
							for (uint32 x = 0; x < nx; x++)
							{
								// todo
							}
						}
					}

					data->dimX = nx;
					data->dimY = ny;
					data->dimZ = nz;
					downscale--;
				}
			}
			*/

			if (data->target == GL_TEXTURE_3D || data->target == GL_TEXTURE_2D_ARRAY)
				tex->image3d(data->dimX, data->dimY, data->dimZ, data->internalFormat, data->copyFormat, data->copyType, values);
			else if (data->target == GL_TEXTURE_CUBE_MAP)
				tex->imageCube(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values, data->stride);
			else
				tex->image2d(data->dimX, data->dimY, data->internalFormat, data->copyFormat, data->copyType, values);
			tex->filters(data->filterMin, data->filterMag, data->filterAniso);
			tex->wraps(data->wrapX, data->wrapY, data->wrapZ);
			if ((data->flags & textureFlags::GenerateBitmap) == textureFlags::GenerateBitmap)
				tex->generateMipmaps();

			tex->animationDuration = data->animationDuration;
			tex->animationLoop = (data->flags & textureFlags::AnimationLoop) == textureFlags::AnimationLoop;
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeTexture(uint32 threadIndex, windowClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		s.decompress.bind<&processDecompress>();
		return s;
	}
}
