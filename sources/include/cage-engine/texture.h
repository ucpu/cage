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
		uint32 mipLevels = 1;
		bool volume3D = false; // true = 3D texture; false = layers, cube
	};

	CAGE_ENGINE_API Holder<Texture> newGraphicsTexture(GraphicsDevice *device, const TextureCreateConfig &config);
}

#endif
