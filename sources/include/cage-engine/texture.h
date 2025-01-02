#ifndef guard_texture_h_ds54ghlkj89s77e4g
#define guard_texture_h_ds54ghlkj89s77e4g

#include <cage-engine/core.h>

namespace cage
{
	class Image;

	class CAGE_ENGINE_API Texture : private Immovable
	{
	protected:
		detail::StringBase<64> debugName;

	public:
		void setDebugName(const String &name);

		uint64 animationDuration = 0;
		bool animationLoop = false;

		uint32 id() const;
		uint32 target() const;
		uint32 internalFormat() const;
		Vec2i resolution() const;
		Vec3i resolution3() const;
		uint32 mipmapLevels() const; // 1 is just base level
		Vec2i mipmapResolution(uint32 mipmapLevel) const;
		Vec3i mipmapResolution3(uint32 mipmapLevel) const;

		void bind(uint32 bindingPoint) const;
		void bindImage(uint32 bindingPoint, bool read, bool write) const; // for use in compute shaders

		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void swizzle(const uint32 values[4]);

		void initialize(Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat); // 2D or Cube
		void initialize(Vec3i resolution, uint32 mipmapLevels, uint32 internalFormat); // 3D or 2D array

		void importImage(const Image *img);
		void image2d(uint32 mipmapLevel, uint32 format, uint32 type, PointerRange<const char> buffer); // 2D or Rectangle
		void image2dCompressed(uint32 mipmapLevel, PointerRange<const char> buffer); // 2D
		void image2dSlice(uint32 mipmapLevel, uint32 sliceIndex, uint32 format, uint32 type, PointerRange<const char> buffer); // 3D slice or 2D layer or Cube face
		void image2dSliceCompressed(uint32 mipmapLevel, uint32 sliceIndex, PointerRange<const char> buffer); // 3D slice or 2D layer or Cube face
		void image3d(uint32 mipmapLevel, uint32 format, uint32 type, PointerRange<const char> buffer); // 3D or 2D array
		void image3dCompressed(uint32 mipmapLevel, PointerRange<const char> buffer); // 3D or 2D array

		void generateMipmaps();
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		// animationOffset = 0..1 normalized offset, independent of animation speed or duration
		CAGE_ENGINE_API Vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset);
	}

	CAGE_ENGINE_API AssetsScheme genAssetSchemeTexture(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;
}

#endif // guard_texture_h_ds54ghlkj89s77e4g
