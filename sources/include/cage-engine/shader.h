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
	protected:
		AssetLabel label;
		uint32 variant = 0;

	public:
		const wgpu::ShaderModule &nativeVertex();
		const wgpu::ShaderModule &nativeFragment();
	};

	CAGE_ENGINE_API Holder<Shader> newShader(GraphicsDevice *device, const Spirv *spirv, const AssetLabel &label, uint32 variant = 0);

	class CAGE_ENGINE_API MultiShader : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		void setKeywords(PointerRange<detail::StringBase<20>> keywords);
		PointerRange<const detail::StringBase<20>> getKeywords() const;

		void addVariant(GraphicsDevice *device, uint32 variant, const Spirv *spirv);

		bool checkKeyword(uint32 keyword) const;

		Holder<Shader> get(uint32 variant); // sum of hashes of keywords

		uint32 customDataCount = 0; // number of floats passed from the game to the shader, per instance
	};

	CAGE_ENGINE_API Holder<MultiShader> newMultiShader(const AssetLabel &label);
}

#endif
