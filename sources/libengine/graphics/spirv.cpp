#include <vector>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <cage-core/lineReader.h>
#include <cage-engine/spirv.h>

namespace cage
{
	namespace
	{
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
				CAGE_THROW_CRITICAL(Exception, "spirv import buffer not yet implemented");
				// todo
			}

			Holder<PointerRange<char>> exportBuffer() const
			{
				CAGE_THROW_CRITICAL(Exception, "spirv export buffer not yet implemented");
				// todo
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
				DirStackFileIncluder includer;
				const bool parsed = shader.parse(GetDefaultResources(), 0, true, messages, includer);
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
