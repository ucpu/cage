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
		void bind() const;

		void importImage(const Image *img);
		void image2d(Vec2i resolution, uint32 internalFormat);
		void image2d(Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image2dCompressed(Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void imageCube(Vec2i resolution, uint32 internalFormat);
		void imageCube(Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer, uintPtr stride);
		void imageCubeCompressed(Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer, uintPtr stride);
		void image3d(Vec3i resolution, uint32 internalFormat);
		void image3d(Vec3i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void image3dCompressed(Vec3i resolution, uint32 internalFormat, PointerRange<const char> buffer);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void swizzle(const uint32 values[4]);
		void generateMipmaps();
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		CAGE_ENGINE_API Vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 emitTime, uint64 animationStart, Real animationSpeed, Real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeTexture(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;
}

#endif // guard_texture_h_ds54ghlkj89s77e4g
