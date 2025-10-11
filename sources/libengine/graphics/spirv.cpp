#include <array>
#include <vector>

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <cage-core/containerSerialization.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-engine/spirv.h>

namespace cage
{
	namespace
	{
		struct SpirvHeader
		{
			std::array<char, 12> cageName = { "cageSpirv" };
			uint32 version = 1;
		};

		class SpirvImpl : public Spirv
		{
		public:
			std::vector<uint32> vertex, fragment;

			SpirvImpl()
			{
				static int dummy = []()
				{
					glslang::InitializeProcess();
					return 0;
				}();
			}

			void importGlsl(const GlslConfig &glsl)
			{
				clear();

				glslang::TShader v = compileGlslShader(glsl.vertex, EShLanguage::EShLangVertex);
				glslang::TShader f = compileGlslShader(glsl.fragment, EShLanguage::EShLangFragment);

				glslang::TProgram program;
				program.addShader(&v);
				program.addShader(&f);
				const EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
				const bool linked = program.link(messages);
				logInfo(program.getInfoLog());
				if (!linked)
					CAGE_THROW_ERROR(Exception, "shader linking failed");

				glslang::GlslangToSpv(*v.getIntermediate(), vertex);
				glslang::GlslangToSpv(*f.getIntermediate(), fragment);
			}

			void importBuffer(PointerRange<const char> buffer)
			{
				clear();
				Deserializer des(buffer);
				SpirvHeader header;
				des >> header;
				if (header.cageName != SpirvHeader().cageName || header.version != SpirvHeader().version)
					CAGE_THROW_ERROR(Exception, "invalid magic or version in spir-v deserialization");
				des >> vertex;
				des >> fragment;
				CAGE_ASSERT(des.available() == 0);
			}

			Holder<PointerRange<char>> exportBuffer() const
			{
				MemoryBuffer buf;
				Serializer ser(buf);
				SpirvHeader header;
				ser << header;
				ser << vertex;
				ser << fragment;
				return std::move(buf);
			}

		private:
			void clear()
			{
				vertex.clear();
				fragment.clear();
			}

			glslang::TShader compileGlslShader(const std::string &source, const EShLanguage stage)
			{
				glslang::TShader shader(stage);
				if (source.empty())
					return shader;

				const char *src = source.c_str();
				shader.setStrings(&src, 1);
				shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 0);
				shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
				shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

				const EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
				const bool parsed = shader.parse(GetDefaultResources(), 0, true, messages);
				logInfo(shader.getInfoLog());
				if (!parsed)
					CAGE_THROW_ERROR(Exception, "shader parsing failed");
				return shader;
			}

			void logInfo(const std::string &msg)
			{
				Holder<LineReader> lr = newLineReader(msg);
				String line;
				while (lr->readLine(line))
					CAGE_LOG(SeverityEnum::Info, "shader-log", line);
			}
		};
	}

	void Spirv::importGlsl(const GlslConfig &glsl)
	{
		SpirvImpl *impl = (SpirvImpl *)this;
		impl->importGlsl(glsl);
	}

	void Spirv::importBuffer(PointerRange<const char> buffer)
	{
		SpirvImpl *impl = (SpirvImpl *)this;
		impl->importBuffer(buffer);
	}

	PointerRange<const uint32> Spirv::exportSpirvVertex() const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->vertex;
	}

	PointerRange<const uint32> Spirv::exportSpirvFragment() const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->fragment;
	}

	Holder<PointerRange<char>> Spirv::exportBuffer() const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->exportBuffer();
	}

	Holder<Spirv> newSpirv()
	{
		return systemMemory().createImpl<Spirv, SpirvImpl>();
	}
}
