#include <webgpu/webgpu_cpp.h>

#include <cage-engine/graphicsDevice.h>
#include <cage-engine/shader.h>
#include <cage-engine/spirv.h>

namespace cage
{
	namespace
	{
		class ShaderImpl : public Shader
		{
		public:
			wgpu::ShaderModule vertex, fragment;

			ShaderImpl(GraphicsDevice *device, const Spirv *spirv)
			{
				vertex = makeShader(device, spirv->exportSpirvVertex());
				fragment = makeShader(device, spirv->exportSpirvFragment());
			}

			wgpu::ShaderModule makeShader(GraphicsDevice *device, PointerRange<const uint32> code)
			{
				wgpu::ShaderModuleSPIRVDescriptor s;
				s.codeSize = code.size();
				s.code = code.data();
				wgpu::ShaderModuleDescriptor desc;
				desc.nextInChain = &s;
				return device->nativeDevice().CreateShaderModule(&desc);
			}
		};
	}

	wgpu::ShaderModule Shader::nativeVertex()
	{
		ShaderImpl *impl = (ShaderImpl *)this;
		return impl->vertex;
	}

	wgpu::ShaderModule Shader::nativeFragment()
	{
		ShaderImpl *impl = (ShaderImpl *)this;
		return impl->fragment;
	}

	Holder<Shader> newShader(GraphicsDevice *device, const Spirv *spirv)
	{
		return systemMemory().createImpl<Shader, ShaderImpl>(device, spirv);
	}
}
