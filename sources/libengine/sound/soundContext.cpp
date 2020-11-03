#include <cage-core/memoryAllocators.h>
#include <cage-core/string.h>

#include "private.h"

#include <cubeb/cubeb.h>
#ifdef CAGE_SYSTEM_WINDOWS
#include <Objbase.h>
#endif // CAGE_SYSTEM_WINDOWS

//#define GCHL_CUBEB_LOGGING
#ifdef GCHL_CUBEB_LOGGING
#include <cstdarg>
#endif

namespace cage
{
	namespace
	{
#ifdef GCHL_CUBEB_LOGGING
		void soundLogCallback(const char *fmt, ...)
		{
			char buffer[512];
			va_list args;
			va_start(args, fmt);
			vsnprintf(buffer, 500, fmt, args);
			va_end(args);
			CAGE_LOG(SeverityEnum::Info, "cubeb", buffer);
		}

		struct SoundLogInit
		{
			SoundLogInit()
			{
				checkSoundIoError(cubeb_set_log_callback(CUBEB_LOG_VERBOSE, &soundLogCallback));
			}
		} soundLogInit;
#endif

		class SoundContextImpl : public SoundContext
		{
		public:
			string name;
			cubeb *soundio = nullptr;
			MemoryArenaGrowing<MemoryAllocatorPolicyPool<sizeof(templates::AllocatorSizeSet<void*>)>, MemoryConcurrentPolicyNone> linksMemory;

			SoundContextImpl(const SoundContextCreateConfig &config, const string &name_) : name(replace(name_, ":", "_")), linksMemory(config.linksMemory)
			{
#ifdef CAGE_SYSTEM_WINDOWS
				CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif // CAGE_SYSTEM_WINDOWS
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "creating sound context, name: '" + name + "'");
				checkSoundIoError(cubeb_init(&soundio, name.c_str(), nullptr));
				CAGE_ASSERT(soundio);
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "using backend: '" + getBackendName() + "'");
			}

			~SoundContextImpl()
			{
				cubeb_destroy(soundio);
#ifdef CAGE_SYSTEM_WINDOWS
				CoUninitialize();
#endif // CAGE_SYSTEM_WINDOWS
			}
		};
	}

	namespace soundPrivat
	{
		cubeb *soundioFromContext(SoundContext *context)
		{
			SoundContextImpl *impl = (SoundContextImpl*)context;
			return impl->soundio;
		}

		MemoryArena linksArenaFromContext(SoundContext *context)
		{
			SoundContextImpl *impl = (SoundContextImpl*)context;
			return MemoryArena(&impl->linksMemory);
		}
	}

	string SoundContext::getContextName() const
	{
		const SoundContextImpl *impl = (const SoundContextImpl*)this;
		return impl->name;
	}

	string SoundContext::getBackendName() const
	{
		const SoundContextImpl *impl = (const SoundContextImpl *)this;
		return cubeb_get_backend_id(impl->soundio);
	}

	Holder<SoundContext> newSoundContext(const SoundContextCreateConfig &config, const string &name)
	{
		return detail::systemArena().createImpl<SoundContext, SoundContextImpl>(config, name);
	}
}
