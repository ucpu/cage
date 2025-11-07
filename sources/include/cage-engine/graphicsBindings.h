#ifndef guard_graphicsBindings_esdr41ttzu
#define guard_graphicsBindings_esdr41ttzu

#include <svector.h>
#include <webgpu/webgpu_cpp.h>

#include <cage-engine/graphicsCommon.h>

namespace cage
{
	struct CAGE_ENGINE_API GraphicsBindings
	{
		wgpu::BindGroupLayout layout = {};
		wgpu::BindGroup group = {};
		uint32 dynamicBuffersCount = 0;

		operator bool() const
		{
			CAGE_ASSERT(!!layout == !!group);
			return !!layout;
		}
	};

	struct CAGE_ENGINE_API GraphicsBindingsCreateConfig
	{
		struct BufferBindingConfig
		{
			GraphicsBuffer *buffer = nullptr;
			uint32 binding = m;
			uint32 size = m;
			bool dynamic = false;
		};

		struct TextureBindingConfig
		{
			Texture *texture = nullptr;
			uint32 binding = m;
			bool bindTexture = true;
			bool bindSampler = true;
		};

		ankerl::svector<BufferBindingConfig, 1> buffers;
		ankerl::svector<TextureBindingConfig, 1> textures;
	};

	CAGE_ENGINE_API GraphicsBindings newGraphicsBindings(GraphicsDevice *device, const GraphicsBindingsCreateConfig &config);

	CAGE_ENGINE_API GraphicsBindings getEmptyBindings(GraphicsDevice *device);

	// material buffer: 0
	// albedo: image: 1 (sampler: 2)
	// special: 3 (4)
	// normal: 5 (6)
	// custom: 7 (8)
	CAGE_ENGINE_API void prepareModelBindings(GraphicsDevice *device, const AssetsManager *assets, Model *model);
}

#endif
