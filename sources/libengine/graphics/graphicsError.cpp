#include <map>
#include <cstdlib>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/lineReader.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/window.h>
#include "private.h"

namespace cage
{
	namespace
	{
		configBool confDetailedInfo("cage.graphics.logOpenglExtensions", false);
	}

	void checkGlError()
	{
		GLenum err = glGetError();
		switch (err)
		{
		case GL_NO_ERROR: return;
		case GL_INVALID_ENUM: CAGE_THROW_ERROR(graphicsError, "gl_invalid_enum", err); break;
		case GL_INVALID_VALUE: CAGE_THROW_ERROR(graphicsError, "gl_invalid_value", err); break;
		case GL_INVALID_OPERATION: CAGE_THROW_ERROR(graphicsError, "gl_invalid_operation", err); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: CAGE_THROW_ERROR(graphicsError, "gl_invalid_framebuffer_operation", err); break;
		case GL_OUT_OF_MEMORY: CAGE_THROW_ERROR(graphicsError, "gl_out_of_memory", err); break;
		default: CAGE_THROW_CRITICAL(graphicsError, "gl_unknown_error", err); break;
		}
	}

	graphicsError::graphicsError(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept : codeException(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER, code)
	{}

	graphicsError &graphicsError::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		codeException::log();
		return *this;
	}

	namespace
	{
		void APIENTRY openglErrorCallbackImpl(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
		{
			if (id == 131185 && severity == GL_DEBUG_SEVERITY_NOTIFICATION && type == GL_DEBUG_TYPE_OTHER && source == GL_DEBUG_SOURCE_API)
				return; // ignore messages like: Buffer detailed info: Buffer object 3 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_ARB, and GL_UNIFORM_BUFFER_EXT, usage hint is GL_STATIC_DRAW) will use VIDEO memory as the source for buffer object operations.

			windowHandle *ctx = (windowHandle*)userParam;
			CAGE_ASSERT(ctx, "missing context");
			if (ctx->debugOpenglErrorCallback)
				return ctx->debugOpenglErrorCallback(source, type, id, severity, message);

			severityEnum cageSevr = severityEnum::Error;

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
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: tp = "undefined behavior"; break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: tp = "deprecated behavior"; break;
			case GL_DEBUG_TYPE_PERFORMANCE: tp = "performance"; cageSevr = severityEnum::Warning; break;
			case GL_DEBUG_TYPE_OTHER: tp = "other"; break;
			default: tp = "unknown type";
			}

			const char *sevr = nullptr;
			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH: sevr = "high"; break;
			case GL_DEBUG_SEVERITY_MEDIUM: sevr = "medium"; break;
			case GL_DEBUG_SEVERITY_LOW: sevr = "low"; break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: sevr = "notification"; cageSevr = severityEnum::Hint; break;
			default: sevr = "unknown severity";
			}

			CAGE_LOG(cageSevr, "graphics", "debug message:");
			holder<lineReader> lrb = newLineReader((char*)message, detail::strlen(message));
			for (string line; lrb->readLine(line);)
				CAGE_LOG_CONTINUE(cageSevr, "graphics", line);
			CAGE_LOG_CONTINUE(severityEnum::Note, "graphics", string() + "source: " + src + ", type: " + tp + ", severity: " + sevr + ", id: " + id);

			if (id == 131218 && severity == GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_PERFORMANCE)
				return; // do not break on messages that shader is beeing recompiled based on opengl state

			if (cageSevr > severityEnum::Info)
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
			CAGE_LOG(severityEnum::Info, "systemInfo", string() + "opengl version: " + major + "." + minor);
			CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "device vendor: '" + (char*)vendor + "'");
			CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "device renderer: '" + (char*)renderer + "'");
			if (confDetailedInfo)
			{
				CAGE_LOG(severityEnum::Info, "systemInfo", string() + "opengl extensions: ");
				GLint num = 0;
				glGetIntegerv(GL_NUM_EXTENSIONS, &num);
				for (GLint i = 0; i < num; i++)
				{
					const GLubyte *ext = glGetStringi(GL_EXTENSIONS, i);
					CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "extension: '" + (char*)ext + "'");
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			return true;
		}
	}

	namespace graphicsPrivat
	{
		void openglContextInitializeGeneral(windowHandle *w)
		{
			// initialize debug messages
			glDebugMessageCallback(&openglErrorCallbackImpl, w);

			static bool blah = logOpenglInfo(); // log just once

			{ // pack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}

#ifdef GCHL_ENABLE_CONTEXT_BINDING_CHECKS

		struct assertContextStruct
		{
			holder<syncMutex> mutex;
			std::map<windowHandle*, std::map<uint32, uint32>> objects;
			std::map<uint64, windowHandle*> contexts;

			assertContextStruct()
			{
				mutex = newSyncMutex();
			}
		};

		assertContextStruct &assertContext()
		{
			static assertContextStruct s;
			return s;
		}

		void setCurrentContext(windowHandle *ctx)
		{
			scopeLock<syncMutex> lock(assertContext().mutex);
			if (ctx)
				assertContext().contexts[threadId()] = ctx;
			else
				assertContext().contexts.erase(threadId());
		}

		windowHandle *getCurrentContext()
		{
			scopeLock<syncMutex> lock(assertContext().mutex);
			auto it = assertContext().contexts.find(threadId());
			if (it == assertContext().contexts.end())
				return nullptr;
			CAGE_ASSERT(it->second);
			return it->second;
		}

		uint32 contextTypeIndexInitializer()
		{
			scopeLock<syncMutex> lock(assertContext().mutex);
			static uint32 index = 0;
			return index++;
		}

		void contextSetCurrentObjectType(uint32 typeIndex, uint32 id)
		{
			auto cc = getCurrentContext();
			scopeLock<syncMutex> lock(assertContext().mutex);
			assertContext().objects[cc][typeIndex] = id;
		}

		uint32 contextGetCurrentObjectType(uint32 typeIndex)
		{
			auto cc = getCurrentContext();
			scopeLock<syncMutex> lock(assertContext().mutex);
			std::map<uint32, uint32> &m = assertContext().objects[cc];
			auto it = m.find(typeIndex);
			if (it == m.end())
				return cage::m;
			return it->second;
		}

#endif
	}

	namespace detail
	{
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
