#ifndef guard_texture_h_ds54ghlkj89s77e4g
#define guard_texture_h_ds54ghlkj89s77e4g

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API Texture : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		uint64 animationDuration = 0;
		bool animationLoop = false;

		uint32 id() const;
		uint32 target() const;
		Vec2i resolution() const;
		Vec3i resolution3() const;
		uint32 maxMipmapLevel() const;
		void bind() const;

		void importImage(const Image *img);
		void image2d(Vec2i resolution, uint32 internalFormat);
		void image2d(Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image2d(uint32 mipmapLevel, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image2dCompressed(Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void image2dCompressed(uint32 mipmapLevel, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void imageCube(Vec2i resolution, uint32 internalFormat);
		void imageCube(uint32 faceIndex, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void imageCube(uint32 mipmapLevel, uint32 faceIndex, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void imageCubeCompressed(uint32 faceIndex, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void imageCubeCompressed(uint32 mipmapLevel, uint32 faceIndex, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void image3d(Vec3i resolution, uint32 internalFormat);
		void image3d(Vec3i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image3d(uint32 mipmapLevel, Vec3i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image3dCompressed(Vec3i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void image3dCompressed(uint32 mipmapLevel, Vec3i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void swizzle(const uint32 values[4]);
		void maxMipmapLevel(uint32 level);
		void generateMipmaps();
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		// animationOffset = 0..1 normalized offset, independent of animation speed or duration
		CAGE_ENGINE_API Vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeTexture(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;
}

#endif // guard_texture_h_ds54ghlkj89s77e4g
