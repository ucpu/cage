#include <array>
#include <vector>

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include <cage-core/containerSerialization.h>
#include <cage-core/debug.h>
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
			uint32 version = 2;
		};

		struct Stage
		{
			std::vector<uint32> spirv;
			std::string source;
			std::string disassembly;
		};

		Serializer &operator<<(Serializer &s, const Stage &v)
		{
			s << v.spirv << v.source << v.disassembly;
			return s;
		}

		Deserializer &operator>>(Deserializer &s, Stage &v)
		{
			s >> v.spirv >> v.source >> v.disassembly;
			return s;
		}

		class SpirvImpl : public Spirv
		{
			static constexpr EShMessages glslangOptions = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgLinkTimeOptimization | EShMsgValidateCrossStageIO);

		public:
			Stage vertex, fragment, compute;

			SpirvImpl()
			{
				static int dummy = []()
				{
					glslang::InitializeProcess();
					return 0;
				}();
				(void)dummy;
			}

			const Stage &get(ShaderStageEnum stage) const { return const_cast<SpirvImpl *>(this)->get(stage); }

			Stage &get(ShaderStageEnum stage)
			{
				switch (stage)
				{
					case ShaderStageEnum::Vertex:
						return vertex;
					case ShaderStageEnum::Fragment:
						return fragment;
					case ShaderStageEnum::Compute:
						return compute;
					default:
						CAGE_THROW_ERROR(Exception, "invalid shader stage enum value");
				}
			}

			void importGlsl(PointerRange<const SpirvGlslImportConfig> sources)
			{
				assignGlslSources(sources);
				if (!compute.source.empty())
				{
					CAGE_ASSERT(vertex.source.empty());
					CAGE_ASSERT(fragment.source.empty());
					glslCompute();
				}
				else
				{
					CAGE_ASSERT(!vertex.source.empty());
					CAGE_ASSERT(compute.source.empty());
					try
					{
						glslProgram(true);
					}
					catch (const ShaderValidationFailure &)
					{
						// program linking sometimes produces corrupted spirv
						// retry without linking
						assignGlslSources(sources);
						glslProgram(false);
					}
				}
			}

			void importBuffer(PointerRange<const char> buffer)
			{
				clear();
				Deserializer des(buffer);
				SpirvHeader header;
				des >> header;
				if (header.cageName != SpirvHeader().cageName || header.version != SpirvHeader().version)
					CAGE_THROW_ERROR(Exception, "invalid magic or version in spir-v deserialization");
				des >> vertex >> fragment >> compute;
				CAGE_ASSERT(des.available() == 0);
			}

			Holder<PointerRange<char>> exportBuffer() const
			{
				MemoryBuffer buf;
				Serializer ser(buf);
				SpirvHeader header;
				ser << header;
				ser << vertex << fragment << compute;
				return std::move(buf);
			}

		private:
			void clear() { vertex = fragment = compute = {}; }

			void assignGlslSources(PointerRange<const SpirvGlslImportConfig> sources)
			{
				clear();
				for (const auto &it : sources)
				{
					auto &s = get(it.stage);
					CAGE_ASSERT(s.source.empty());
					CAGE_ASSERT(!it.source.empty());
					s.source = it.source;
				}
			}

			void glslProgram(bool performLinking)
			{
				glslang::TShader v = compileGlslShader(vertex.source, EShLanguage::EShLangVertex);
				glslang::TShader f = compileGlslShader(fragment.source, EShLanguage::EShLangFragment);

				glslang::TProgram program; // must outlive
				if (performLinking)
				{
					CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "linking");
					program.addShader(&v);
					program.addShader(&f);
					const bool linked = program.link(glslangOptions);
					logInfo(program.getInfoLog());
					if (!linked)
						CAGE_THROW_ERROR(Exception, "shader linking failed");
				}

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "converting intermediate to spirv binary");
				glslang::GlslangToSpv(*v.getIntermediate(), vertex.spirv);
				glslang::GlslangToSpv(*f.getIntermediate(), fragment.spirv);
				validateShader(ShaderStageEnum::Vertex, !performLinking);
				validateShader(ShaderStageEnum::Fragment, !performLinking);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "optimizing");
				optimizeShader(vertex.spirv);
				optimizeShader(fragment.spirv);
				validateShader(ShaderStageEnum::Vertex);
				validateShader(ShaderStageEnum::Fragment);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "disassembling");
				vertex.disassembly = disassembleShader(vertex.spirv);
				fragment.disassembly = disassembleShader(fragment.spirv);
			}

			void glslCompute()
			{
				glslang::TShader c = compileGlslShader(compute.source, EShLanguage::EShLangCompute);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "converting intermediate to spirv binary");
				glslang::GlslangToSpv(*c.getIntermediate(), compute.spirv);
				validateShader(ShaderStageEnum::Compute);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "optimizing");
				optimizeShader(compute.spirv);
				validateShader(ShaderStageEnum::Compute);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "disassembling");
				compute.disassembly = disassembleShader(compute.spirv);
			}

			glslang::TShader compileGlslShader(const std::string &source, const EShLanguage stage)
			{
				try
				{
					glslang::TShader shader(stage);
					if (source.empty())
						return shader;

					CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "compiling shader (" + EShLanguageToString(stage) + ")");

					const char *src = source.c_str();
					shader.setStrings(&src, 1);
					shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 0);
					shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
					shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
					shader.setDebugInfo(true);

					const bool parsed = shader.parse(GetDefaultResources(), 0, true, glslangOptions);
					std::string log = shader.getInfoLog();
					if (!parsed)
					{
						if (log.find("unintended combination of keywords") == std::string::npos)
						{
							logInfo(log);
							CAGE_THROW_ERROR(Exception, "shader parsing failed");
						}
						else
						{
							detail::OverrideBreakpoint ob;
							CAGE_THROW_ERROR(UnintendedKeywordsCombination, "unintended keywords combination");
						}
					}
					return shader;
				}
				catch (const UnintendedKeywordsCombination &)
				{
					throw; // do not print source
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Info, "shader", "shader source:");
					logSource(source);
					throw;
				}
			}

			void validateShader(ShaderStageEnum stage, bool dumpDisassembly = true)
			{
				const Stage &s = get(stage);
				spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);
				tools.SetMessageConsumer(messageConsumer);
				if (!tools.Validate(s.spirv))
				{
					if (dumpDisassembly)
						logInfo(disassembleShader(s.spirv));
					CAGE_THROW_ERROR(ShaderValidationFailure, "failed to validate generated spirv");
				}
			}

			void optimizeShader(std::vector<uint32> &source)
			{
				std::vector<uint32> result;
				spvtools::Optimizer opt(SPV_ENV_VULKAN_1_1);
				opt.SetMessageConsumer(messageConsumer);
				opt.RegisterPass(spvtools::CreateSplitCombinedImageSamplerPass());
				opt.RegisterPass(spvtools::CreateResolveBindingConflictsPass());
				opt.RegisterPerformancePasses();
				if (opt.Run(source.data(), source.size(), &result))
					std::swap(source, result);
				else
					CAGE_THROW_ERROR(Exception, "failed shader optimization");
			}

			std::string disassembleShader(const std::vector<uint32> &source)
			{
				spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);
				std::string text;
				static constexpr auto options = SPV_BINARY_TO_TEXT_OPTION_INDENT | /* SPV_BINARY_TO_TEXT_OPTION_NO_HEADER | */ SPV_BINARY_TO_TEXT_OPTION_COMMENT | SPV_BINARY_TO_TEXT_OPTION_NESTED_INDENT | SPV_BINARY_TO_TEXT_OPTION_REORDER_BLOCKS;
				if (!tools.Disassemble(source, &text, options))
					CAGE_THROW_ERROR(Exception, "failed shader disassemlby");
				return text;
			}

			static void messageConsumer(spv_message_level_t level, const char *source, const spv_position_t &position, const char *message) { logInfo(message); }

			static void logInfo(const std::string &msg)
			{
				Holder<LineReader> lr = newLineReader(msg);
				String line;
				while (lr->readLine(line))
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "shader-log", line);
			}

			static void logSource(const std::string &msg)
			{
				Holder<LineReader> lr = newLineReader(msg);
				String txt;
				uint32 line = 1;
				while (lr->readLine(txt))
				{
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "shader-log", Stringizer() + line + ": " + txt);
					line++;
				}
			}

			static const char *EShLanguageToString(EShLanguage stage)
			{
				switch (stage)
				{
					case EShLangVertex:
						return "Vertex";
					case EShLangTessControl:
						return "Tessellation Control";
					case EShLangTessEvaluation:
						return "Tessellation Evaluation";
					case EShLangGeometry:
						return "Geometry";
					case EShLangFragment:
						return "Fragment";
					case EShLangCompute:
						return "Compute";
					case EShLangRayGen:
						return "Ray Generation";
					case EShLangIntersect:
						return "Intersection";
					case EShLangAnyHit:
						return "Any Hit";
					case EShLangClosestHit:
						return "Closest Hit";
					case EShLangMiss:
						return "Miss";
					case EShLangCallable:
						return "Callable";
					case EShLangTaskNV:
						return "TaskNV";
					case EShLangMeshNV:
						return "MeshNV";
					default:
						return "Unknown";
				}
			}
		};
	}

	void Spirv::importBuffer(PointerRange<const char> buffer)
	{
		SpirvImpl *impl = (SpirvImpl *)this;
		impl->importBuffer(buffer);
	}

	void Spirv::importGlsl(PointerRange<const SpirvGlslImportConfig> sources)
	{
		SpirvImpl *impl = (SpirvImpl *)this;
		impl->importGlsl(sources);
	}

	bool Spirv::hasStage(ShaderStageEnum stage) const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return !impl->get(stage).spirv.empty();
	}

	Holder<PointerRange<char>> Spirv::exportBuffer() const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->exportBuffer();
	}

	PointerRange<const uint32> Spirv::exportSpirv(ShaderStageEnum stage) const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->get(stage).spirv;
	}

	std::string Spirv::exportSource(ShaderStageEnum stage) const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->get(stage).source;
	}

	std::string Spirv::exportDisassembly(ShaderStageEnum stage) const
	{
		const SpirvImpl *impl = (const SpirvImpl *)this;
		return impl->get(stage).disassembly;
	}

	Holder<Spirv> newSpirv()
	{
		return systemMemory().createImpl<Spirv, SpirvImpl>();
	}

	const char *toString(ShaderStageEnum stage)
	{
		switch (stage)
		{
			case ShaderStageEnum::None:
				return "none";
			case ShaderStageEnum::Vertex:
				return "vertex";
			case ShaderStageEnum::Fragment:
				return "fragment";
			case ShaderStageEnum::Compute:
				return "compute";
		}
		return "unknown";
	}
}
