#include <array>
#include <optional>
#include <unordered_set>
#include <vector>

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <source/opt/build_module.h>
#include <source/opt/ir_context.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/linker.hpp>
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
			static constexpr spv_target_env TargetEnv = spv_target_env::SPV_ENV_VULKAN_1_1;
			static constexpr EShMessages GlslangOptions = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgLinkTimeOptimization | EShMsgValidateCrossStageIO);

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
					glslProgram();
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

			void glslProgram()
			{
				vertex.spirv = compileGlslShader(vertex.source, EShLanguage::EShLangVertex);
				fragment.spirv = compileGlslShader(fragment.source, EShLanguage::EShLangFragment);
				optimizeShader(fragment.spirv);
				optimizeShader(vertex.spirv);

				pruneVertexOutputs();
				optimizeShader(vertex.spirv);
				linkVertexFragment();

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "optimizing individual modules");
				optimizeShader(vertex.spirv);
				optimizeShader(fragment.spirv);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "disassembling");
				vertex.disassembly = disassembleShader(vertex.spirv);
				fragment.disassembly = disassembleShader(fragment.spirv);
			}

			void pruneVertexOutputs()
			{
				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "pruning vertex outputs");

				std::unordered_set<uint32_t> usedLocations;

				const auto &findLocation = [](spvtools::opt::Module *module, uint32 id) -> std::optional<uint32>
				{
					for (auto &deco : module->annotations())
						if (deco.opcode() == spv::Op::OpDecorate && deco.GetSingleWordInOperand(0) == id && deco.GetSingleWordInOperand(1) == (uint32)spv::Decoration::Location)
							return deco.GetSingleWordInOperand(2);
					return {};
				};

				const auto &removeLocation = [](spvtools::opt::Module *module, uint32 id)
				{
					for (auto it = module->annotation_begin(); it != module->annotation_end();)
					{
						if (it->opcode() == spv::Op::OpDecorate && it->GetSingleWordInOperand(0) == id)
							it = it.Erase();
						else
							++it;
					}
				};

				const auto &newPtrType = [](spvtools::opt::IRContext *context, uint32 originalTypeId) -> uint32
				{
					auto module = context->module();

					// look at original type
					auto ptrTypeInst = context->get_def_use_mgr()->GetDef(originalTypeId);
					uint32 pointeeType = ptrTypeInst->GetSingleWordInOperand(1);

					// prepare new type
					spvtools::opt::Instruction::OperandList operandList{ spvtools::opt::Operand{ SPV_OPERAND_TYPE_LITERAL_INTEGER, { (uint32)spv::StorageClass::Private } }, spvtools::opt::Operand{ SPV_OPERAND_TYPE_ID, { pointeeType } } };
					std::unique_ptr<spvtools::opt::Instruction> newType = std::make_unique<spvtools::opt::Instruction>(context, spv::Op::OpTypePointer, 0, context->TakeNextId(), std::move(operandList));

					// find position of the first OpVariable in the module
					auto insertPos = module->types_values_begin();
					while (insertPos != module->types_values_end() && insertPos->opcode() != spv::Op::OpVariable)
						++insertPos;

					auto it = insertPos.InsertBefore(std::move(newType));
					context->get_def_use_mgr()->AnalyzeInstDef(&*it);
					context->InvalidateAnalysesExceptFor(spvtools::opt::IRContext::Analysis(~0));
					context->BuildInvalidAnalyses(spvtools::opt::IRContext::Analysis(~0));
					return it->result_id();
				};

				{ // find input variables used in fragment stage
					auto context = spvtools::BuildModule(TargetEnv, messageConsumer, fragment.spirv.data(), fragment.spirv.size());
					auto module = context->module();

					for (auto &inst : module->types_values())
					{
						if (inst.opcode() != spv::Op::OpVariable)
							continue;
						if (static_cast<spv::StorageClass>(inst.GetSingleWordInOperand(0)) != spv::StorageClass::Input)
							continue;

						const auto loc = findLocation(module, inst.result_id());
						if (loc)
							usedLocations.insert(*loc);
					}
				}

				{ // remove unused output variables
					auto context = spvtools::BuildModule(TargetEnv, messageConsumer, vertex.spirv.data(), vertex.spirv.size());
					auto module = context->module();

					std::unordered_set<uint32> removedVariables;

					// change unused variables to private storage
					for (auto &inst : module->types_values())
					{
						if (inst.opcode() != spv::Op::OpVariable)
							continue;
						if (static_cast<spv::StorageClass>(inst.GetSingleWordInOperand(0)) != spv::StorageClass::Output)
							continue;
						const auto loc = findLocation(module, inst.result_id());
						if (!loc)
							continue;
						if (usedLocations.find(*loc) != usedLocations.end())
							continue;

						// update pointer type of the variable
						inst.SetResultType(newPtrType(context.get(), inst.type_id()));

						// change storage class from Output to Private
						inst.SetInOperand(0, { (uint32)spv::StorageClass::Private });

						// remove now-obsolete decoration
						removeLocation(module, inst.result_id());

						removedVariables.insert(inst.result_id());
					}

					// remove unused variables from entry point definitions
					for (auto &entry : module->entry_points())
					{
						for (uint32 i = 3; i < entry.NumOperands();)
						{
							uint32_t varId = entry.GetSingleWordOperand(i);
							if (removedVariables.count(varId))
								entry.RemoveOperand(i);
							else
								i++;
						}
					}

					std::vector<uint32_t> res;
					context->module()->ToBinary(&res, false);
					validateShader(res);
					std::swap(vertex.spirv, res);
				}
			}

			void linkVertexFragment()
			{
				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "linking");
				std::vector<std::vector<uint32>> inputs;
				inputs.push_back(vertex.spirv);
				inputs.push_back(fragment.spirv);
				std::vector<uint32> linked;
				spvtools::Context context(TargetEnv);
				context.SetMessageConsumer(messageConsumer);
				spvtools::LinkerOptions options;
				const auto res = spvtools::Link(context, inputs, &linked, options);
				if (res != spv_result_t::SPV_SUCCESS)
					CAGE_THROW_ERROR(Exception, "linking failed");

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "optimizing linked program");
				optimizeShader(linked);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "separating program into modules");
				vertex.spirv = extractStage(linked, ShaderStageEnum::Vertex);
				fragment.spirv = extractStage(linked, ShaderStageEnum::Fragment);
				validateShader(vertex.spirv);
				validateShader(fragment.spirv);
			}

			void glslCompute()
			{
				compute.spirv = compileGlslShader(compute.source, EShLanguage::EShLangCompute);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "optimizing");
				optimizeShader(compute.spirv);

				CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "disassembling");
				compute.disassembly = disassembleShader(compute.spirv);
			}

			std::vector<uint32> compileGlslShader(const std::string &source, const EShLanguage stage)
			{
				try
				{
					glslang::TShader shader(stage);
					if (source.empty())
						return {};

					CAGE_LOG(SeverityEnum::Info, "shader", Stringizer() + "compiling shader (" + EShLanguageToString(stage) + ")");

					const char *src = source.c_str();
					shader.setStrings(&src, 1);
					shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 0);
					shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
					shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
					shader.setDebugInfo(true);

					if (!shader.parse(GetDefaultResources(), 0, true, GlslangOptions))
					{
						std::string log = shader.getInfoLog();
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

					std::vector<uint32> res;
					glslang::GlslangToSpv(*shader.getIntermediate(), res);
					validateShader(res);
					return res;
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

			std::vector<uint32> extractStage(const std::vector<uint32> &input, ShaderStageEnum stage)
			{
				const auto &conv = [](spv::ExecutionModel execModel) -> ShaderStageEnum
				{
					switch (execModel)
					{
						case spv::ExecutionModel::Vertex:
							return ShaderStageEnum::Vertex;
						case spv::ExecutionModel::Fragment:
							return ShaderStageEnum::Fragment;
						case spv::ExecutionModel::GLCompute:
							return ShaderStageEnum::Compute;
					}
					return ShaderStageEnum::None;
				};

				auto context = spvtools::BuildModule(TargetEnv, messageConsumer, input.data(), input.size());
				auto module = context->module();

				std::unordered_set<uint32> keepEntryIds;

				for (auto it = module->entry_points().begin(); it != module->entry_points().end();)
				{
					CAGE_ASSERT(it->opcode() == spv::Op::OpEntryPoint);
					if (conv((spv::ExecutionModel)it->GetSingleWordInOperand(0)) != stage)
						it = it.Erase();
					else
					{
						keepEntryIds.insert(it->GetSingleWordInOperand(1)); // function ID
						++it;
					}
				}

				for (auto it = module->execution_modes().begin(); it != module->execution_modes().end();)
				{
					uint32_t funcId = it->GetSingleWordInOperand(0);
					if (!keepEntryIds.count(funcId))
						it = it.Erase();
					else
						++it;
				}

				std::vector<uint32> res;
				context->module()->ToBinary(&res, false);
				validateShader(res);
				return res;
			}

			void optimizeShader(std::vector<uint32> &source)
			{
				std::vector<uint32> result;
				spvtools::Optimizer opt(TargetEnv);
				opt.SetMessageConsumer(messageConsumer);
				opt.RegisterPass(spvtools::CreateSplitCombinedImageSamplerPass());
				opt.RegisterPass(spvtools::CreateResolveBindingConflictsPass());
				opt.RegisterPerformancePasses();
				if (!opt.Run(source.data(), source.size(), &result))
					CAGE_THROW_ERROR(Exception, "failed shader optimization");
				validateShader(result);
				std::swap(source, result);
			}

			void validateShader(const std::vector<uint32> &spirv)
			{
				spvtools::SpirvTools tools(TargetEnv);
				tools.SetMessageConsumer(messageConsumer);
				if (!tools.Validate(spirv))
				{
					logInfo(disassembleShader(spirv));
					CAGE_THROW_ERROR(ShaderValidationFailure, "failed to validate generated spirv");
				}
			}

			std::string disassembleShader(const std::vector<uint32> &source)
			{
				spvtools::SpirvTools tools(TargetEnv);
				std::string text;
				static constexpr auto options = SPV_BINARY_TO_TEXT_OPTION_INDENT | SPV_BINARY_TO_TEXT_OPTION_COMMENT | SPV_BINARY_TO_TEXT_OPTION_NESTED_INDENT | SPV_BINARY_TO_TEXT_OPTION_REORDER_BLOCKS;
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
