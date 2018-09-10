#include <map>
#include <cstdlib>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/lineReader.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include <cage-client/window.h>
#include "private.h"

namespace cage
{
	void checkGlError()
	{
		GLenum err = glGetError();
		switch (err)
		{
		case GL_NO_ERROR: return;
		case GL_INVALID_ENUM: CAGE_THROW_ERROR(graphicsException, "gl_invalid_enum", err); break;
		case GL_INVALID_VALUE: CAGE_THROW_ERROR(graphicsException, "gl_invalid_value", err); break;
		case GL_INVALID_OPERATION: CAGE_THROW_ERROR(graphicsException, "gl_invalid_operation", err); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: CAGE_THROW_ERROR(graphicsException, "gl_invalid_framebuffer_operation", err); break;
		case GL_OUT_OF_MEMORY: CAGE_THROW_ERROR(graphicsException, "gl_out_of_memory", err); break;
		default: CAGE_THROW_CRITICAL(graphicsException, "gl_unknown_error", err); break;
		}
	}

	graphicsException::graphicsException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept : codeException(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER, code)
	{}

	graphicsException &graphicsException::log()
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

			windowClass *ctx = (windowClass*)userParam;
			CAGE_ASSERT_RUNTIME(ctx, "missing context");
			if (ctx->debugOpenglErrorCallback)
				return ctx->debugOpenglErrorCallback(source, type, id, severity, message);

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
			case GL_DEBUG_TYPE_PERFORMANCE: tp = "performance"; break;
			case GL_DEBUG_TYPE_OTHER: tp = "other"; break;
			default: tp = "unknown type";
			}

			severityEnum cageSevr = severityEnum::Error;
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
			lineReaderBuffer lrb((char*)message, detail::strlen(message));
			for (string line; lrb.readLine(line);)
				CAGE_LOG_CONTINUE(cageSevr, "graphics", line);
			CAGE_LOG_CONTINUE(cageSevr, "graphics", string("source: ") + string(src));
			CAGE_LOG_CONTINUE(cageSevr, "graphics", string("type: ") + string(tp));
			CAGE_LOG_CONTINUE(cageSevr, "graphics", string("severity: ") + string(sevr));
			CAGE_LOG_CONTINUE(cageSevr, "graphics", string("id: ") + string(id));

			if (id == 131218 && severity == GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_PERFORMANCE)
				return; // do not break on messages that shader is beeing recompiled based on opengl state

			if (cageSevr > severityEnum::Info)
				detail::debugBreakpoint();
		}
	}

	namespace graphicsPrivat
	{
		void openglContextInitializeGeneral(windowClass *w)
		{
			// initialize debug messages
			if (GLAD_GL_KHR_debug)
				glDebugMessageCallback(&openglErrorCallbackImpl, w);

			{ // query context info
				GLint major = 0, minor = 0;
				glGetIntegerv(GL_MAJOR_VERSION, &major);
				glGetIntegerv(GL_MINOR_VERSION, &minor);
				CAGE_CHECK_GL_ERROR_DEBUG();
				const GLubyte *vendor = nullptr, *renderer = nullptr;
				vendor = glGetString(GL_VENDOR);
				renderer = glGetString(GL_RENDERER);
				CAGE_CHECK_GL_ERROR_DEBUG();
				CAGE_LOG_CONTINUE(severityEnum::Note, "graphics", string() + "opengl version: " + major + "." + minor);
				CAGE_LOG_CONTINUE(severityEnum::Note, "graphics", string() + "device vendor: '" + (char*)vendor + "'");
				CAGE_LOG_CONTINUE(severityEnum::Note, "graphics", string() + "device renderer: '" + (char*)renderer + "'");
			}

			{ // pack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		}

#ifdef GCHL_ENABLE_CONTEXT_BINDING_CHECKS

		holder<mutexClass> assertContextMutex;
		class assertContextMutexInitializer
		{
		public:
			assertContextMutexInitializer()
			{
				assertContextMutex = newMutex();
			}
			~assertContextMutexInitializer()
			{
				assertContextMutex.clear();
			}
		} assertContextMutexInitializerInstance;

		std::map<windowClass*, std::map<uint32, uint32>> assertContextCurrentObjects;
		std::map<uint64, windowClass*> assertThreadCurrentContext;

		void setCurrentContext(windowClass *ctx)
		{
			scopeLock<mutexClass> lock(assertContextMutex);
			if (ctx)
				assertThreadCurrentContext[threadId()] = ctx;
			else
				assertThreadCurrentContext.erase(threadId());
		}

		windowClass *getCurrentContext()
		{
			scopeLock<mutexClass> lock(assertContextMutex);
			std::map<uint64, windowClass*>::iterator it = assertThreadCurrentContext.find(threadId());
			if (it == assertThreadCurrentContext.end())
				return nullptr;
			CAGE_ASSERT_RUNTIME(it->second);
			return it->second;
		}

		uint32 contextTypeIndexInitializer()
		{
			static uint32 index = 0;
			return index++;
		}

		void contextSetCurrentObjectType(uint32 typeIndex, uint32 id)
		{
			auto cc = getCurrentContext();
			scopeLock<mutexClass> lock(assertContextMutex);
			assertContextCurrentObjects[cc][typeIndex] = id;
		}

		uint32 contextGetCurrentObjectType(uint32 typeIndex)
		{
			auto cc = getCurrentContext();
			scopeLock<mutexClass> lock(assertContextMutex);
			std::map<uint32, uint32> &m = assertContextCurrentObjects[cc];
			std::map<uint32, uint32>::iterator it = m.find(typeIndex);
			if (it == m.end())
				return -1;
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
