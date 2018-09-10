#include <list>
#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/lineReader.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include "private.h"

namespace cage
{
	configBool shaderIntrospection("cage-client.graphics.shaderIntrospection", false);

	namespace
	{
		class shaderImpl : public shaderClass
		{
		public:
			uint32 id;
			std::list <uint32> sources;
			std::vector<uint32> subroutinesVertex;
			std::vector<uint32> subroutinesTessControl;
			std::vector<uint32> subroutinesTessEvaluation;
			std::vector<uint32> subroutinesGeometry;
			std::vector<uint32> subroutinesFragment;
			std::vector<uint32> subroutinesCompute;

			shaderImpl() : id(0)
			{
				id = glCreateProgram();
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			~shaderImpl()
			{
				glDeleteProgram(id);
			}
		};
	}

	uint32 shaderClass::getId() const
	{
		return ((shaderImpl*)this)->id;
	}

	void shaderClass::bind() const
	{
		shaderImpl *impl = (shaderImpl*)this;
		glUseProgram(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<shaderClass>(impl->id);
	}

	void shaderClass::uniform(uint32 name, const sint32 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1i(name, u);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const uint32 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1ui(name, u);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const real &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1f(name, u.value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec2 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform2fv(name, 1, &const_cast<vec2&>(u)[0].value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec3 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform3fv(name, 1, &const_cast<vec3&>(u)[0].value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec4 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform4fv(name, 1, &const_cast<vec4&>(u)[0].value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const quat &u)
	{
		uniform(name, vec4(u[0], u[1], u[2], u[3]));
	}

	void shaderClass::uniform(uint32 name, const mat3 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniformMatrix3fv(name, 1, GL_FALSE, &const_cast<mat3&>(u)[0].value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const mat4 &u)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniformMatrix4fv(name, 1, GL_FALSE, &const_cast<mat4&>(u)[0].value);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const sint32 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1iv(name, count, u);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const uint32 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1uiv(name, count, u);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const real *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform1fv(name, count, (float*)u);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec2 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform2fv(name, count, (float*)const_cast<vec2*>(u));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec3 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform3fv(name, count, (float*)const_cast<vec3*>(u));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const vec4 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniform4fv(name, count, (float*)const_cast<vec4*>(u));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const quat *u, uint32 count)
	{
		uniform(name, (const vec4*)u, count);
	}

	void shaderClass::uniform(uint32 name, const mat3 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniformMatrix3fv(name, count, GL_FALSE, (float*)const_cast<mat3*>(u));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::uniform(uint32 name, const mat4 *u, uint32 count)
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		glUniformMatrix4fv(name, count, GL_FALSE, (float*)const_cast<mat4*>(u));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void shaderClass::subroutine(uint32 stage, uint32 name, uint32 value)
	{
		shaderImpl *impl = (shaderImpl*)this;
		switch (stage)
		{
		case GL_VERTEX_SHADER: if (name < impl->subroutinesVertex.size()) impl->subroutinesVertex[name] = value; break;
		case GL_TESS_CONTROL_SHADER: if (name < impl->subroutinesTessControl.size()) impl->subroutinesTessControl[name] = value; break;
		case GL_TESS_EVALUATION_SHADER: if (name < impl->subroutinesTessEvaluation.size()) impl->subroutinesTessEvaluation[name] = value; break;
		case GL_GEOMETRY_SHADER: if (name < impl->subroutinesGeometry.size()) impl->subroutinesGeometry[name] = value; break;
		case GL_FRAGMENT_SHADER: if (name < impl->subroutinesFragment.size()) impl->subroutinesFragment[name] = value; break;
		case GL_COMPUTE_SHADER: if (name < impl->subroutinesCompute.size()) impl->subroutinesCompute[name] = value; break;
		default: CAGE_THROW_CRITICAL(exception, "invalid shader stage");
		}
	}

	void shaderClass::applySubroutines() const
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);
		if (!impl->subroutinesVertex.empty())
			glUniformSubroutinesuiv(GL_VERTEX_SHADER, numeric_cast<uint32>(impl->subroutinesVertex.size()), &impl->subroutinesVertex[0]);
		if (!impl->subroutinesTessControl.empty())
			glUniformSubroutinesuiv(GL_TESS_CONTROL_SHADER, numeric_cast<uint32>(impl->subroutinesTessControl.size()), &impl->subroutinesTessControl[0]);
		if (!impl->subroutinesTessEvaluation.empty())
			glUniformSubroutinesuiv(GL_TESS_EVALUATION_SHADER, numeric_cast<uint32>(impl->subroutinesTessEvaluation.size()), &impl->subroutinesTessEvaluation[0]);
		if (!impl->subroutinesGeometry.empty())
			glUniformSubroutinesuiv(GL_GEOMETRY_SHADER, numeric_cast<uint32>(impl->subroutinesGeometry.size()), &impl->subroutinesGeometry[0]);
		if (!impl->subroutinesFragment.empty())
			glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, numeric_cast<uint32>(impl->subroutinesFragment.size()), &impl->subroutinesFragment[0]);
		if (!impl->subroutinesCompute.empty())
			glUniformSubroutinesuiv(GL_COMPUTE_SHADER, numeric_cast<uint32>(impl->subroutinesCompute.size()), &impl->subroutinesCompute[0]);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	namespace
	{
		void checkGlErrorDeleteShader(uint32 shader)
		{
			try
			{
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			catch (const graphicsException &)
			{
				glDeleteShader(shader);
				throw;
			}
		}

		const string glTypeToName(uint32 type)
		{
			switch (type)
			{
			case GL_FLOAT: return "float";
			case GL_FLOAT_VEC2: return "vec2";
			case GL_FLOAT_VEC3: return "vec3";
			case GL_FLOAT_VEC4: return "vec4";
			case GL_DOUBLE: return "double";
			case GL_DOUBLE_VEC2: return "dvec2";
			case GL_DOUBLE_VEC3: return "dvec3";
			case GL_DOUBLE_VEC4: return "dvec4";
			case GL_INT: return "int";
			case GL_INT_VEC2: return "ivec2";
			case GL_INT_VEC3: return "ivec3";
			case GL_INT_VEC4: return "ivec4";
			case GL_UNSIGNED_INT: return "unsigned int";
			case GL_UNSIGNED_INT_VEC2: return "uvec2";
			case GL_UNSIGNED_INT_VEC3: return "uvec3";
			case GL_UNSIGNED_INT_VEC4: return "uvec4";
			case GL_BOOL: return "bool";
			case GL_BOOL_VEC2: return "bvec2";
			case GL_BOOL_VEC3: return "bvec3";
			case GL_BOOL_VEC4: return "bvec4";
			case GL_FLOAT_MAT2: return "mat2";
			case GL_FLOAT_MAT3: return "mat3";
			case GL_FLOAT_MAT4: return "mat4";
			case GL_FLOAT_MAT2x3: return "mat2x3";
			case GL_FLOAT_MAT2x4: return "mat2x4";
			case GL_FLOAT_MAT3x2: return "mat3x2";
			case GL_FLOAT_MAT3x4: return "mat3x4";
			case GL_FLOAT_MAT4x2: return "mat4x2";
			case GL_FLOAT_MAT4x3: return "mat4x3";
			case GL_DOUBLE_MAT2: return "dmat2";
			case GL_DOUBLE_MAT3: return "dmat3";
			case GL_DOUBLE_MAT4: return "dmat4";
			case GL_DOUBLE_MAT2x3: return "dmat2x3";
			case GL_DOUBLE_MAT2x4: return "dmat2x4";
			case GL_DOUBLE_MAT3x2: return "dmat3x2";
			case GL_DOUBLE_MAT3x4: return "dmat3x4";
			case GL_DOUBLE_MAT4x2: return "dmat4x2";
			case GL_DOUBLE_MAT4x3: return "dmat4x3";
			case GL_SAMPLER_1D: return "sampler1D";
			case GL_SAMPLER_2D: return "sampler2D";
			case GL_SAMPLER_3D: return "sampler3D";
			case GL_SAMPLER_CUBE: return "samplerCube";
			case GL_SAMPLER_1D_SHADOW: return "sampler1DShadow";
			case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
			case GL_SAMPLER_1D_ARRAY: return "sampler1DArray";
			case GL_SAMPLER_2D_ARRAY: return "sampler2DArray";
			case GL_SAMPLER_1D_ARRAY_SHADOW: return "sampler1DArrayShadow";
			case GL_SAMPLER_2D_ARRAY_SHADOW: return "sampler2DArrayShadow";
			case GL_SAMPLER_2D_MULTISAMPLE: return "sampler2DMS";
			case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return "sampler2DMSArray";
			case GL_SAMPLER_CUBE_SHADOW: return "samplerCubeShadow";
			case GL_SAMPLER_BUFFER: return "samplerBuffer";
			case GL_SAMPLER_2D_RECT: return "sampler2DRect";
			case GL_SAMPLER_2D_RECT_SHADOW: return "sampler2DRectShadow";
			case GL_INT_SAMPLER_1D: return "isampler1D";
			case GL_INT_SAMPLER_2D: return "isampler2D";
			case GL_INT_SAMPLER_3D: return "isampler3D";
			case GL_INT_SAMPLER_CUBE: return "isamplerCube";
			case GL_INT_SAMPLER_1D_ARRAY: return "isampler1DArray";
			case GL_INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
			case GL_INT_SAMPLER_2D_MULTISAMPLE: return "isampler2DMS";
			case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "isampler2DMSArray";
			case GL_INT_SAMPLER_BUFFER: return "isamplerBuffer";
			case GL_INT_SAMPLER_2D_RECT: return "isampler2DRect";
			case GL_UNSIGNED_INT_SAMPLER_1D: return "usampler1D";
			case GL_UNSIGNED_INT_SAMPLER_2D: return "usampler2D";
			case GL_UNSIGNED_INT_SAMPLER_3D: return "usampler3D";
			case GL_UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube";
			case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler2DArray";
			case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return "usampler2DMS";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "usampler2DMSArray";
			case GL_UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer";
			case GL_UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect";
			default: return "unknown";
			}
		}

		static const uint32 shaderLogBufferSize = 1024 * 8;
	}

	void shaderClass::source(uint32 type, void *data, uint32 length)
	{
		shaderImpl *impl = (shaderImpl*)this;

		string typeName;
		switch (type)
		{
		case GL_VERTEX_SHADER: typeName = "vertex"; break;
		case GL_TESS_CONTROL_SHADER: typeName = "tess_control"; break;
		case GL_TESS_EVALUATION_SHADER: typeName = "tess_evaluation"; break;
		case GL_GEOMETRY_SHADER: typeName = "geometry"; break;
		case GL_FRAGMENT_SHADER: typeName = "fragment"; break;
		case GL_COMPUTE_SHADER: typeName = "compute"; break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid shader type");
		}

		if (shaderIntrospection)
		{
			holder<fileClass> f = newFile(string() + "shaderIntrospection/" + impl->id + "/" + typeName + ".glsl", fileMode(false, true, true));
			f->write(data, length);
		}

		GLuint shader = glCreateShader(type);
		CAGE_CHECK_GL_ERROR_DEBUG();

		GLint len = length;
		const GLchar *tmp = (const GLchar *)data;
		glShaderSource(shader, 1, &tmp, &len);
		checkGlErrorDeleteShader(shader);

		glCompileShader(shader);
		checkGlErrorDeleteShader(shader);

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		checkGlErrorDeleteShader(shader);
		if (len > 5)
		{
			char buf[shaderLogBufferSize];
			glGetShaderInfoLog(shader, shaderLogBufferSize - 1, &len, buf);
			checkGlErrorDeleteShader(shader);
			CAGE_LOG(severityEnum::Warning, "shader", string() + "shader compilation log (id: " + impl->id + ", stage: " + typeName + "):");
			lineReaderBuffer lrb(buf, len);
			for (string line; lrb.readLine(line);)
				CAGE_LOG_CONTINUE(severityEnum::Note, "shader", line);
		}

		glGetShaderiv(shader, GL_COMPILE_STATUS, &len);
		checkGlErrorDeleteShader(shader);
		if (len != GL_TRUE)
		{
			glDeleteShader(shader);
			CAGE_THROW_ERROR(graphicsException, "shader compilation failed", len);
		}

		glAttachShader(impl->id, shader);
		checkGlErrorDeleteShader(shader);

		glDeleteShader(shader);
		CAGE_CHECK_GL_ERROR_DEBUG();

		impl->sources.push_back(shader);
	}

	void shaderClass::relink()
	{
		shaderImpl *impl = (shaderImpl*)this;

		impl->subroutinesVertex.clear();
		impl->subroutinesTessControl.clear();
		impl->subroutinesTessEvaluation.clear();
		impl->subroutinesGeometry.clear();
		impl->subroutinesFragment.clear();
		impl->subroutinesCompute.clear();

		glLinkProgram(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();

		for (auto it = impl->sources.begin(), et = impl->sources.end(); it != et; it++)
			glDetachShader(impl->id, *it);
		impl->sources.clear();
		CAGE_CHECK_GL_ERROR_DEBUG();

		{ GLint count; glGetProgramStageiv(impl->id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesVertex.resize(count, -1); }
		{ GLint count; glGetProgramStageiv(impl->id, GL_TESS_CONTROL_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesTessControl.resize(count, -1); }
		{ GLint count; glGetProgramStageiv(impl->id, GL_TESS_EVALUATION_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesTessEvaluation.resize(count, -1); }
		{ GLint count; glGetProgramStageiv(impl->id, GL_GEOMETRY_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesGeometry.resize(count, -1); }
		{ GLint count; glGetProgramStageiv(impl->id, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesFragment.resize(count, -1); }
		{ GLint count; glGetProgramStageiv(impl->id, GL_COMPUTE_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count); impl->subroutinesCompute.resize(count, -1); }
		CAGE_CHECK_GL_ERROR_DEBUG();

		GLint len;
		glGetProgramiv(impl->id, GL_INFO_LOG_LENGTH, &len);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (len > 5)
		{
			char buf[shaderLogBufferSize];
			glGetProgramInfoLog(impl->id, shaderLogBufferSize - 1, &len, buf);
			CAGE_CHECK_GL_ERROR_DEBUG();
			CAGE_LOG(severityEnum::Warning, "shader", string() + "shader linking log (id: " + impl->id + "):");
			lineReaderBuffer lrb(buf, len);
			for (string line; lrb.readLine(line);)
				CAGE_LOG_CONTINUE(severityEnum::Note, "shader", line);
		}

		glGetProgramiv(impl->id, GL_LINK_STATUS, &len);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (len != GL_TRUE)
			CAGE_THROW_ERROR(graphicsException, "shader linking failed", len);

		if (shaderIntrospection)
		{
			{ // uniforms
				holder<fileClass> f = newFile(string() + "shaderIntrospection/" + impl->id + "/uniforms.txt", fileMode(false, true, true));
				f->writeLine("<location> <type> <name>");
				GLint numUniforms = 0;
				glGetProgramInterfaceiv(impl->id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);
				for (GLint unif = 0; unif < numUniforms; unif++)
				{
					const GLenum props[] = { GL_BLOCK_INDEX, GL_LOCATION, GL_TYPE, GL_ARRAY_SIZE };
					GLint values[4];
					glGetProgramResourceiv(impl->id, GL_UNIFORM, unif, 4, props, 4, nullptr, values);
					if (values[0] != -1)
						continue; // skip variables that are inside any block
					GLchar name[100];
					GLint nameLen = 0;
					glGetProgramResourceName(impl->id, GL_UNIFORM, unif, 99, &nameLen, name);
					f->writeLine(string() + values[1] + "\t" + glTypeToName(values[2]) + (values[3] > 1 ? string() + "[" + values[3] + "]" : "") + " " + string(name, nameLen));
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			{ // blocks
				holder<fileClass> f = newFile(string() + "shaderIntrospection/" + impl->id + "/blocks.txt", fileMode(false, true, true));
				f->writeLine("<binding> <name> <variables> <size>");
				GLint numBlocks = 0;
				glGetProgramInterfaceiv(impl->id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
				for (GLint block = 0; block < numBlocks; block++)
				{
					const GLenum props[] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
					GLint values[3];
					glGetProgramResourceiv(impl->id, GL_UNIFORM_BLOCK, block, 3, props, 3, nullptr, values);
					GLchar name[100];
					GLint nameLen = 0;
					glGetProgramResourceName(impl->id, GL_UNIFORM_BLOCK, block, 99, &nameLen, name);
					f->writeLine(string() + values[0] + "\t" + string(name, nameLen) + " " + values[2] + " " + values[1]);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			{ // subroutines
				holder<fileClass> f = newFile(string() + "shaderIntrospection/" + impl->id + "/routines.txt", fileMode(false, true, true));
				f->writeLine("<stage> <location> <name> <compatibles>");
				const GLint stages[] = { GL_VERTEX_SUBROUTINE_UNIFORM, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, GL_GEOMETRY_SUBROUTINE_UNIFORM, GL_FRAGMENT_SUBROUTINE_UNIFORM, GL_COMPUTE_SUBROUTINE_UNIFORM };
				const GLint stages2[] = { GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER };
				for (GLint stageIndex = 0; stageIndex < 6; stageIndex++)
				{
					GLint stage = stages[stageIndex];
					GLint stage2 = stages2[stageIndex];
					GLint numRoutines = 0;
					glGetProgramInterfaceiv(impl->id, stage, GL_ACTIVE_RESOURCES, &numRoutines);
					for (GLint routine = 0; routine < numRoutines; routine++)
					{
						const GLenum props[] = { GL_LOCATION, GL_NUM_COMPATIBLE_SUBROUTINES };
						GLint values[2];
						glGetProgramResourceiv(impl->id, stage, routine, 2, props, 2, nullptr, values);
						GLchar name[100];
						GLint nameLen = 0;
						glGetProgramResourceName(impl->id, stage, routine, 99, &nameLen, name);
						f->writeLine(string() + stageIndex + "\t" + values[0] + "\t" + string(name, nameLen) + " " + values[1]);
						// enumerate compatible subroutines
						const GLenum props2[] = { GL_COMPATIBLE_SUBROUTINES };
						GLint values2[100];
						GLint numCompatibles = -1;
						glGetProgramResourceiv(impl->id, stage, routine, 1, props2, 99, &numCompatibles, values2);
						for (GLint compat = 0; compat < numCompatibles; compat++)
						{
							glGetActiveSubroutineName(impl->id, stage2, values2[compat], 99, &nameLen, name);
							f->writeLine(string() + "\t\t\t" + values2[compat] + "\t" + string(name, nameLen));
						}
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			{ // inouts
				holder<fileClass> f = newFile(string() + "shaderIntrospection/" + impl->id + "/inout.txt", fileMode(false, true, true));
				f->writeLine("<inout> <location> <index> <type> <name>");
				const GLint stages[] = { GL_PROGRAM_INPUT, GL_PROGRAM_OUTPUT };
				const string stageNames[] = { "in", "out" };
				for (GLint stageIndex = 0; stageIndex < 2; stageIndex++)
				{
					GLint stage = stages[stageIndex];
					GLint numVars = 0;
					glGetProgramInterfaceiv(impl->id, stage, GL_ACTIVE_RESOURCES, &numVars);
					for (GLint var = 0; var < numVars; var++)
					{
						const GLenum props[] = { GL_LOCATION, GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION_INDEX };
						GLint values[4];
						values[3] = -1;
						glGetProgramResourceiv(impl->id, stage, var, 3 + stageIndex, props, 4, nullptr, values);
						GLchar name[100];
						GLint nameLen = 0;
						glGetProgramResourceName(impl->id, stage, var, 99, &nameLen, name);
						f->writeLine(string() + stageNames[stageIndex] + "\t" + values[0] + "\t" + values[3] + "\t" + glTypeToName(values[1]) + (values[2] > 1 ? string() + "[" + values[2] + "]" : "") + " " + string(name, nameLen));
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		}
	}

	void shaderClass::validate()
	{
		shaderImpl *impl = (shaderImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<shaderClass>() == impl->id);

		glValidateProgram(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();

		GLint len;
		glGetProgramiv(impl->id, GL_INFO_LOG_LENGTH, &len);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (len > 5)
		{
			char buf[shaderLogBufferSize];
			glGetProgramInfoLog(impl->id, shaderLogBufferSize - 1, &len, buf);
			CAGE_CHECK_GL_ERROR_DEBUG();
			CAGE_LOG(severityEnum::Warning, "shader", string() + "shader validation log (id: " + impl->id + "):");
			lineReaderBuffer lrb(buf, len);
			for (string line; lrb.readLine(line);)
				CAGE_LOG_CONTINUE(severityEnum::Note, "shader", line);
		}

		glGetProgramiv(impl->id, GL_VALIDATE_STATUS, &len);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (len != GL_TRUE)
			CAGE_THROW_ERROR(graphicsException, "shader validation failed", len);
	}

	holder<shaderClass> newShader(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext() == context);
		return detail::systemArena().createImpl <shaderClass, shaderImpl>();
	}
}