#ifndef guard_graphicsTexture_sdrfgh4d5g
#define guard_graphicsTexture_sdrfgh4d5g

#include <cage-engine/core.h>

namespace wgpu
{
	class Texture;
	class TextureView;
}

namespace cage
{
	class Image;
	class GraphicsDevice;

	class CAGE_ENGINE_API Texture : private Immovable
	{
	protected:
		detail::StringBase<128> label;

	public:
		void setLabel(const String &name);

		Vec2i resolution() const;
		Vec3i resolution3() const;
		uint32 mipLevels() const; // 1 is just base level
		Vec2i mipResolution(uint32 mipmapLevel) const;
		Vec3i mipResolution3(uint32 mipmapLevel) const;

		wgpu::Texture nativeTexture();
		wgpu::TextureView nativeView();
	};

	struct CAGE_ENGINE_API TextureCreateConfig
	{
		Vec3i resolution = Vec3i(0, 0, 1);
		uint32 channels = 3;
		uint32 mipLevels = 1;
		bool srgb = false;
	};

	CAGE_ENGINE_API Holder<Texture> newTexture(GraphicsDevice *device, const TextureCreateConfig &config);
	CAGE_ENGINE_API Holder<Texture> newTexture(GraphicsDevice *device, const Image *image);
	CAGE_ENGINE_API Holder<Texture> newTexture(wgpu::Texture texture);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeTexture(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexTexture = 11;
}

#endif
