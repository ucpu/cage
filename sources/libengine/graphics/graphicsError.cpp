#include <cage-core/concurrent.h>
#include <cage-core/lineReader.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>

#include <cage-engine/uniformBuffer.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h>

#include <map>
#include <cstdlib>
#include <cstring>

namespace cage
{
	namespace
	{
		const ConfigBool confDetailedInfo("cage/graphics/printExtensions", false);
		const ConfigBool confLogSynchronous("cage/graphics/synchronousCallback",
#ifdef CAGE_DEBUG
			true
#else
			false
#endif
			);
	}

	void checkGlError()
	{
		GLenum err = glGetError();
		switch (err)
		{
		case GL_NO_ERROR: return;
		case GL_INVALID_ENUM: CAGE_THROW_ERROR(GraphicsError, "gl_invalid_enum", err); break;
		case GL_INVALID_VALUE: CAGE_THROW_ERROR(GraphicsError, "gl_invalid_value", err); break;
		case GL_INVALID_OPERATION: CAGE_THROW_ERROR(GraphicsError, "gl_invalid_operation", err); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: CAGE_THROW_ERROR(GraphicsError, "gl_invalid_framebuffer_operation", err); break;
		case GL_OUT_OF_MEMORY: CAGE_THROW_ERROR(GraphicsError, "gl_out_of_memory", err); break;
		default: CAGE_THROW_CRITICAL(GraphicsError, "gl_unknown_error", err); break;
		}
	}

	GraphicsDebugScope::GraphicsDebugScope(StringPointer name)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
	}

	GraphicsDebugScope::~GraphicsDebugScope()
	{
		glPopDebugGroup();
	}

	namespace
	{
		void APIENTRY openglErrorCallbackImpl(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
		{
			if (id == 0 && severity == GL_DEBUG_SEVERITY_NOTIFICATION && (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP) && source == GL_DEBUG_SOURCE_APPLICATION)
				return; // ignore messages from GraphicsDebugScope

			if (id == 131185 && severity == GL_DEBUG_SEVERITY_NOTIFICATION && type == GL_DEBUG_TYPE_OTHER && source == GL_DEBUG_SOURCE_API)
				return; // ignore messages like: Buffer detailed info: Buffer object 3 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_ARB, and GL_UNIFORM_BUFFER_EXT, usage hint is GL_STATIC_DRAW) will use VIDEO memory as the source for buffer object operations.

			Window *ctx = (Window *)userParam;
			CAGE_ASSERT(ctx);
			if (ctx->debugOpenglErrorCallback)
				return ctx->debugOpenglErrorCallback(source, type, id, severity, message);

			SeverityEnum cageSevr = SeverityEnum::Error;

			const char *src = nullptr;
			switch (source)
			{
			case GL_DEBUG_SOURCE_API: src = "api"; break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM: src = "window system"; break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER: src = "shader compiler"; break;
			case GL_DEBUG_SOURCE_THIRD_PARTY: src = "third party"; break;
			case GL_DEBUG_SOURCE_APPLICATION: src = "application"; break;
			case GL_DEBUG_SOURCE_OTHER: src = "other"; break;
			default: src = "unknown source";
			}

			const char *tp = nullptr;
			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR: tp = "error"; break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: tp = "deprecated behavior"; break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: tp = "undefined behavior"; break;
			case GL_DEBUG_TYPE_PORTABILITY: tp = "portability"; cageSevr = SeverityEnum::Warning; break;
			case GL_DEBUG_TYPE_PERFORMANCE: tp = "performance"; cageSevr = SeverityEnum::Warning; break;
			case GL_DEBUG_TYPE_MARKER: tp = "marker"; cageSevr = SeverityEnum::Info; break;
			case GL_DEBUG_TYPE_PUSH_GROUP: tp = "push group"; cageSevr = SeverityEnum::Info; break;
			case GL_DEBUG_TYPE_POP_GROUP: tp = "pop group"; cageSevr = SeverityEnum::Info; break;
			case GL_DEBUG_TYPE_OTHER: tp = "other"; break;
			default: tp = "unknown type";
			}

			const char *sevr = nullptr;
			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH: sevr = "high"; break;
			case GL_DEBUG_SEVERITY_MEDIUM: sevr = "medium"; break;
			case GL_DEBUG_SEVERITY_LOW: sevr = "low"; break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: sevr = "notification"; cageSevr = SeverityEnum::Hint; break;
			default: sevr = "unknown severity";
			}

			CAGE_LOG(cageSevr, "graphics", "debug message:");
			Holder<LineReader> lrb = newLineReader({ message, message + std::strlen(message) });
			for (String line; lrb->readLine(line);)
				CAGE_LOG_CONTINUE(cageSevr, "graphics", line);
			CAGE_LOG_CONTINUE(SeverityEnum::Note, "graphics", Stringizer() + "source: " + src + ", type: " + tp + ", severity: " + sevr + ", id: " + id);

			if (id == 131218 && severity == GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_PERFORMANCE)
				return; // do not break on messages that shader is being recompiled based on opengl state

			if (id == 0 && source == GL_DEBUG_SOURCE_SHADER_COMPILER && severity == GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_OTHER)
				return; // intel gpu: do not break on messages that barrier call inside control flow

			if (cageSevr > SeverityEnum::Info)
				detail::debugBreakpoint();
		}

		bool logOpenglInfo()
		{
			GLint major = 0, minor = 0;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);
			CAGE_CHECK_GL_ERROR_DEBUG();
			const GLubyte *vendor = nullptr, *renderer = nullptr;
			vendor = glGetString(GL_VENDOR);
			renderer = glGetString(GL_RENDERER);
			CAGE_CHECK_GL_ERROR_DEBUG();
			CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "opengl version: " + major + "." + minor);
			CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "device vendor: '" + (char*)vendor + "'");
			CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "device renderer: '" + (char*)renderer + "'");
			if (confDetailedInfo)
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "opengl extensions: ");
				GLint num = 0;
				glGetIntegerv(GL_NUM_EXTENSIONS, &num);
				for (GLint i = 0; i < num; i++)
				{
					const GLubyte *ext = glGetStringi(GL_EXTENSIONS, i);
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "graphics", Stringizer() + "extension: '" + (char*)ext + "'");
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			return true;
		}
	}

	namespace detail
	{
		void initializeOpengl()
		{
			if (gladLoadGL() != 1)
				CAGE_THROW_ERROR(Exception, "failed to initialize OpenGL loader");
		}

		void configureOpengl(Window *w)
		{
			// initialize debug messages
			glDebugMessageCallback(&openglErrorCallbackImpl, w);

			if (confLogSynchronous)
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			else
				glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			static bool blah = logOpenglInfo(); // log just once

#define checkExtension(NAME) if (!NAME) { CAGE_THROW_ERROR(Exception, "missing required OpenGL extension: " #NAME); }
			checkExtension(GLAD_GL_EXT_texture_compression_s3tc);
			checkExtension(GLAD_GL_EXT_texture_filter_anisotropic);
			checkExtension(GLAD_GL_EXT_texture_sRGB);
#undef checkExtension

			{
				GLint combined = 0, fs = 0;
				glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &combined);
				glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &fs);
				CAGE_CHECK_GL_ERROR_DEBUG();
				if (combined < 32 || fs < 32)
					CAGE_THROW_ERROR(Exception, "insufficient number of texture units");
			}

			{ // pack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
			glDisable(GL_FRAMEBUFFER_SRGB);

			UniformBuffer::alignmentRequirement(); // make sure that the value is retrieved in thread with bound opengl context

			checkGlError();
		}

		void purgeGlShaderCache()
		{
#ifdef CAGE_SYSTEM_WINDOWS

			try
			{
#define GCHL_START "rmdir /S /Q "
#define GCHL_END " 2>nul 1>nul; "
				system(
					GCHL_START "\"%APPDATA%\\AMD\\GLCache\"" GCHL_END
					GCHL_START "\"%APPDATA%\\NVIDIA\\GLCache\"" GCHL_END
					GCHL_START "\"%LOCALAPPDATA%\\AMD\\GLCache\"" GCHL_END
					GCHL_START "\"%LOCALAPPDATA%\\NVIDIA\\GLCache\"" GCHL_END
				);
#undef GCHL_START
#undef GCHL_END
			}
			catch (...)
			{
				// do nothing
			}

#endif // CAGE_SYSTEM_WINDOWS
		}
	}
}
