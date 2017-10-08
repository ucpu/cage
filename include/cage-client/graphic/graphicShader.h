namespace cage
{
	class CAGE_API shaderClass
	{
	public:
		uint32 getId() const;
		void bind() const;

		void source(uint32 type, void *data, uint32 length);
		void relink();
		void validate();

#define GCHL_SHADER_UNIFORM(TYPE) void uniform (uint32 name, const TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_SHADER_UNIFORM, sint32, uint32, real, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_SHADER_UNIFORM

#define GCHL_SHADER_UNIFORM(TYPE) void uniform (uint32 name, const TYPE *values, uint32 count);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_SHADER_UNIFORM, sint32, uint32, real, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_SHADER_UNIFORM

		void subroutine(uint32 stage, uint32 name, uint32 value);
		void applySubroutines() const;
	};

	CAGE_API holder<shaderClass> newShader(windowClass *context);
}
