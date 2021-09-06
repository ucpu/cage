#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/config.h>
#include <cage-core/typeIndex.h>

#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		ConfigUint32 textureDownscale("cage/graphics/textureDownscale", 1); // todo

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader data;
			des >> data;

			Holder<Texture> tex = newTexture(data.target);
			tex->setDebugName(context->textName);

			const uint32 bytesSize = data.resolution[0] * data.resolution[1] * data.resolution[2] * data.channels;
			PointerRange<const char> values = des.read(bytesSize);

			if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
				tex->image3d(data.resolution, data.internalFormat, data.copyFormat, data.copyType, values);
			else if (data.target == GL_TEXTURE_CUBE_MAP)
				tex->imageCube(Vec2i(data.resolution), data.internalFormat, data.copyFormat, data.copyType, values, data.stride);
			else
				tex->image2d(Vec2i(data.resolution), data.internalFormat, data.copyFormat, data.copyType, values);
			tex->filters(data.filterMin, data.filterMag, data.filterAniso);
			tex->wraps(data.wrapX, data.wrapY, data.wrapZ);
			if (any(data.flags & TextureFlags::GenerateMipmaps))
				tex->generateMipmaps();

			tex->animationDuration = data.animationDuration;
			tex->animationLoop = any(data.flags & TextureFlags::AnimationLoop);

			context->assetHolder = std::move(tex).cast<void>();
		}
	}

	AssetScheme genAssetSchemeTexture(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeIndex = detail::typeIndex<Texture>();
		return s;
	}
}
