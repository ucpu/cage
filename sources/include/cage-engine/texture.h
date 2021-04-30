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
		void setDebugName(const string &name);

		uint64 animationDuration = 0;
		bool animationLoop = false;

		uint32 getId() const;
		uint32 getTarget() const;
		ivec2 getResolution() const;
		ivec3 getResolution3() const;
		void bind() const;

		void importImage(const Image *img);
		void image2d(uint32 w, uint32 h, uint32 internalFormat);
		void image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer, uintPtr stride = 0);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();

		[[deprecated]] static void multiBind(PointerRange<const uint32> tius, PointerRange<const Texture *const> texs);
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		CAGE_ENGINE_API vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeTexture(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;
}

#endif // guard_texture_h_ds54ghlkj89s77e4g
