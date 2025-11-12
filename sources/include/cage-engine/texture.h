#ifndef guard_graphicsTexture_sdrfgh4d5g
#define guard_graphicsTexture_sdrfgh4d5g

#include <cage-engine/core.h>

namespace wgpu
{
	class Texture;
	class TextureView;
	class Sampler;
	enum class TextureFormat : uint32_t;
}

namespace cage
{
	class Image;
	class GraphicsDevice;

	enum class TextureFlags : uint32
	{
		None = 0,
		Volume3D = 1 << 0,
		Array = 1 << 1,
		Cubemap = 1 << 2,
		Normals = 1 << 3,
		Srgb = 1 << 4,
		Compressed = 1 << 5,
	};

	class CAGE_ENGINE_API Texture : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		Vec2i resolution() const;
		Vec3i resolution3() const;
		uint32 mipLevels() const; // 1 is just base level
		Vec2i mipResolution(uint32 mipmapLevel) const;
		Vec3i mipResolution3(uint32 mipmapLevel) const;

		const wgpu::Texture &nativeTexture();
		const wgpu::TextureView &nativeView();
		const wgpu::Sampler &nativeSampler();

		TextureFlags flags = TextureFlags::None;
	};

	struct CAGE_ENGINE_API ColorTextureCreateConfig
	{
		Vec3i resolution = Vec3i(0, 0, 1);
		uint32 channels = 4;
		uint32 mipLevels = 1;
		TextureFlags flags = TextureFlags::None;
		bool sampling = true;
		bool renderable = false;
	};

	struct CAGE_ENGINE_API TransientTextureCreateConfig
	{
		AssetLabel name;
		Vec3i resolution = Vec3i(0, 0, 1);
		uint32 mipLevelCount = 1;
		wgpu::TextureFormat format = (wgpu::TextureFormat)0;
		TextureFlags flags = TextureFlags::None;
		uint32 entityId = 0;
		bool samplerVariant = false;

		bool operator==(const TransientTextureCreateConfig &) const = default;
	};

	CAGE_ENGINE_API Holder<Texture> newTexture(GraphicsDevice *device, const ColorTextureCreateConfig &config, const AssetLabel &label);
	CAGE_ENGINE_API Holder<Texture> newTexture(GraphicsDevice *device, const TransientTextureCreateConfig &config);
	CAGE_ENGINE_API Holder<Texture> newTexture(GraphicsDevice *device, const Image *image, const AssetLabel &label);
	CAGE_ENGINE_API Holder<Texture> newTexture(wgpu::Texture texture, wgpu::TextureView view, wgpu::Sampler sampler, const AssetLabel &label);
}

#endif
