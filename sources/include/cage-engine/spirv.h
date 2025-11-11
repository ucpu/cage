#ifndef guard_spirv_erst54fij
#define guard_spirv_erst54fij

#include <string>

#include <cage-engine/core.h>

namespace cage
{
	struct CAGE_ENGINE_API UnintendedKeywordsCombination : public Exception
	{
		using Exception::Exception;
	};

	struct CAGE_ENGINE_API ShaderValidationFailure : public Exception
	{
		using Exception::Exception;
	};

	enum class ShaderStageEnum
	{
		None,
		Vertex,
		Fragment,
		Compute,
	};

	struct SpirvGlslImportConfig
	{
		std::string source;
		ShaderStageEnum stage = ShaderStageEnum::None;
	};

	class CAGE_ENGINE_API Spirv : private Immovable
	{
	public:
		void importBuffer(PointerRange<const char> buffer);
		void importGlsl(PointerRange<const SpirvGlslImportConfig> sources);

		bool hasStage(ShaderStageEnum stage) const;

		Holder<PointerRange<char>> exportBuffer() const;
		PointerRange<const uint32> exportSpirv(ShaderStageEnum stage) const;
		std::string exportSource(ShaderStageEnum stage) const;
		std::string exportDisassembly(ShaderStageEnum stage) const;

		void stripSources();
	};

	CAGE_ENGINE_API Holder<Spirv> newSpirv();

	CAGE_ENGINE_API const char *toString(ShaderStageEnum stage);
}

#endif
