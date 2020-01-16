namespace cage
{
	class CAGE_API ShaderProgram : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void source(uint32 type, const char *data, uint32 length);
		void relink();
		void validate();

#define GCHL_SHADER_UNIFORM(TYPE) void uniform (uint32 name, const TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_SHADER_UNIFORM, sint32, uint32, real, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_SHADER_UNIFORM

#define GCHL_SHADER_UNIFORM(TYPE) void uniform (uint32 name, const TYPE *values, uint32 count);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_SHADER_UNIFORM, sint32, uint32, real, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_SHADER_UNIFORM
	};

	CAGE_API Holder<ShaderProgram> newShaderProgram();

	CAGE_API AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex, Window *memoryContext);
	static const uint32 AssetSchemeIndexShaderProgram = 10;
}
