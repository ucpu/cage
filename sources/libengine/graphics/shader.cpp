#include <algorithm>
#include <unordered_map>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include <cage-core/hashString.h>
#include <cage-core/lineReader.h>
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
			wgpu::ShaderModule vertex, fragment, compute;

			ShaderImpl(GraphicsDevice *device, const Spirv *spirv, const AssetLabel &label_, uint32 variant_)
			{
				this->label = label_;
				this->variant = variant_;

				vertex = makeShader(device, spirv, ShaderStageEnum::Vertex);
				fragment = makeShader(device, spirv, ShaderStageEnum::Fragment);
				compute = makeShader(device, spirv, ShaderStageEnum::Compute);
			}

			wgpu::ShaderModule makeShader(GraphicsDevice *device, const Spirv *spirv, ShaderStageEnum stage)
			{
				if (!spirv->hasStage(stage))
					return {};
				const auto code = spirv->exportSpirv(stage);
				wgpu::ShaderModuleSPIRVDescriptor s = {};
				s.codeSize = code.size();
				s.code = code.data();
				wgpu::ShaderModuleDescriptor desc = {};
				desc.nextInChain = &s;
				desc.label = label.c_str();
				return device->nativeDevice()->CreateShaderModule(&desc);
			}

			void logMultilineString(const std::string &str)
			{
				Holder<LineReader> lr = newLineReader(str);
				String l;
				while (lr->readLine(l))
				{
					CAGE_LOG(SeverityEnum::Note, "shader", l);
				}
			}
		};
	}

	const wgpu::ShaderModule &Shader::nativeVertex()
	{
		ShaderImpl *impl = (ShaderImpl *)this;
		return impl->vertex;
	}

	const wgpu::ShaderModule &Shader::nativeFragment()
	{
		ShaderImpl *impl = (ShaderImpl *)this;
		return impl->fragment;
	}

	Holder<Shader> newShader(GraphicsDevice *device, const Spirv *spirv, const AssetLabel &label, uint32 variant)
	{
		return systemMemory().createImpl<Shader, ShaderImpl>(device, spirv, label, variant);
	}

	namespace
	{
		class MultiShaderImpl : public MultiShader
		{
		public:
			std::vector<detail::StringBase<20>> keywords;
			std::vector<uint32> keycodes;
			std::unordered_map<uint32, Holder<Shader>> shaders;

			MultiShaderImpl(const AssetLabel &label_) { this->label = label_; }
		};
	}

	void MultiShader::setKeywords(PointerRange<detail::StringBase<20>> keywords)
	{
		MultiShaderImpl *impl = (MultiShaderImpl *)this;
		impl->keywords.clear();
		impl->keycodes.clear();
		for (const auto &it : keywords)
		{
			impl->keywords.push_back(it);
			impl->keycodes.push_back(HashString(it));
		}
		std::sort(impl->keycodes.begin(), impl->keycodes.end());
	}

	PointerRange<const detail::StringBase<20>> MultiShader::getKeywords() const
	{
		const MultiShaderImpl *impl = (const MultiShaderImpl *)this;
		return impl->keywords;
	}

	void MultiShader::addVariant(GraphicsDevice *device, uint32 variant, const Spirv *spirv)
	{
		MultiShaderImpl *impl = (MultiShaderImpl *)this;
		impl->shaders[variant] = newShader(device, spirv, label, variant);
	}

	bool MultiShader::checkKeyword(uint32 keyword) const
	{
		const MultiShaderImpl *impl = (const MultiShaderImpl *)this;
		const auto &c = impl->keycodes;
		return std::binary_search(c.begin(), c.end(), keyword);
	}

	Holder<Shader> MultiShader::get(uint32 variant)
	{
		MultiShaderImpl *impl = (MultiShaderImpl *)this;
		auto it = impl->shaders.find(variant);
		if (it == impl->shaders.end())
		{
			CAGE_LOG_THROW(Stringizer() + "shader name: " + label);
			CAGE_THROW_ERROR(Exception, "unknown shader variant");
		}
		return it->second.share();
	}

	Holder<MultiShader> newMultiShader(const AssetLabel &label)
	{
		return systemMemory().createImpl<MultiShader, MultiShaderImpl>(label);
	}
}
