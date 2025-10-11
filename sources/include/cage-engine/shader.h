#ifndef guard_shader_cxvbjhgr85
#define guard_shader_cxvbjhgr85

#include <cage-engine/core.h>

namespace wgpu
{
	class ShaderModule;
}

namespace cage
{
	class GraphicsDevice;
	class Spirv;

	class CAGE_ENGINE_API Shader : private Immovable
	{
	public:
		wgpu::ShaderModule nativeVertex();
		wgpu::ShaderModule nativeFragment();
	};

	CAGE_ENGINE_API Holder<Shader> newShader(GraphicsDevice *device, const Spirv *spirv);

	CAGE_ENGINE_API AssetsScheme genAssetSchemeShaderProgram(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexShaderProgram = 10;
}

#endif
