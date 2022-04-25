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
		void setDebugName(const String &name);

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
		GCHL_GENERATE(Vec2i);
		GCHL_GENERATE(Vec3i);
		GCHL_GENERATE(Vec4i);
		GCHL_GENERATE(Real);
		GCHL_GENERATE(Vec2);
		GCHL_GENERATE(Vec3);
		GCHL_GENERATE(Vec4);
		GCHL_GENERATE(Quat);
		GCHL_GENERATE(Mat3);
		GCHL_GENERATE(Mat4);
#undef GCHL_GENERATE
	};

	class CAGE_ENGINE_API MultiShaderProgram : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		void setKeywords(PointerRange<detail::StringBase<20>> keywords);
		void setSource(uint32 type, PointerRange<const char> buffer);
		void compile();

		Holder<ShaderProgram> get(uint32 variant); // sum of hashes of keywords
	};

	CAGE_ENGINE_API Holder<ShaderProgram> newShaderProgram();
	CAGE_ENGINE_API Holder<MultiShaderProgram> newMultiShaderProgram();

	CAGE_ENGINE_API AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexShaderProgram = 10;
}

#endif // guard_shaderProgram_h_qdsxc187uijf8c7frg
