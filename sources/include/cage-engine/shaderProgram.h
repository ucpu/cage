#ifndef guard_shaderProgram_h_qdsxc187uijf8c7frg
#define guard_shaderProgram_h_qdsxc187uijf8c7frg

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API ShaderProgram : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 id() const;
		void bind() const;

		void source(uint32 type, PointerRange<const char> buffer);
		void relink();
		void validate() const;

#define GCHL_GENERATE(TYPE) \
		void uniform(uint32 name, const TYPE &value); \
		void uniform(uint32 name, PointerRange<const TYPE> values);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(ivec2);
		GCHL_GENERATE(ivec3);
		GCHL_GENERATE(ivec4);
		GCHL_GENERATE(real);
		GCHL_GENERATE(vec2);
		GCHL_GENERATE(vec3);
		GCHL_GENERATE(vec4);
		GCHL_GENERATE(quat);
		GCHL_GENERATE(mat3);
		GCHL_GENERATE(mat4);
#undef GCHL_GENERATE
	};

	CAGE_ENGINE_API Holder<ShaderProgram> newShaderProgram();

	CAGE_ENGINE_API AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexShaderProgram = 10;
}

#endif // guard_shaderProgram_h_qdsxc187uijf8c7frg
