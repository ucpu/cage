#ifndef guard_spirv_erst54fij
#define guard_spirv_erst54fij

#include <string>

#include <cage-engine/core.h>

namespace cage
{
	struct GlslConfig
	{
		std::string vertex;
		std::string fragment;
	};

	class CAGE_ENGINE_API Spirv : private Immovable
	{
	public:
		void importGlsl(const GlslConfig &glsl);
		void importBuffer(PointerRange<const char> buffer);

		PointerRange<const uint32> exportSpirvVertex() const;
		PointerRange<const uint32> exportSpirvFragment() const;
		Holder<PointerRange<char>> exportBuffer() const;
	};

	CAGE_ENGINE_API Holder<Spirv> newSpirv();
}

#endif
