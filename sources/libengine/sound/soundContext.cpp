#include <cage-core/memory.h>

#include "private.h"

#include <cubeb/cubeb.h>
#include <cstdarg>
#ifdef CAGE_SYSTEM_WINDOWS
#include <Objbase.h>
#endif // CAGE_SYSTEM_WINDOWS

namespace cage
{
	namespace
	{
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
				//checkSoundIoError(cubeb_set_log_callback(CUBEB_LOG_VERBOSE, &soundLogCallback));
			}
		} soundLogInit;

		class SoundContextImpl : public SoundContext
		{
		public:
			string name;
			cubeb *soundio = nullptr;
			MemoryArenaGrowing<MemoryAllocatorPolicyPool<sizeof(templates::AllocatorSizeSet<void*>)>, MemoryConcurrentPolicyNone> linksMemory;

			SoundContextImpl(const SoundContextCreateConfig &config, const string &name) : name(name.replace(":", "_")), linksMemory(config.linksMemory)
			{
#ifdef CAGE_SYSTEM_WINDOWS
				CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif // CAGE_SYSTEM_WINDOWS
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "creating sound context, name: '" + name + "'");
				checkSoundIoError(cubeb_init(&soundio, name.c_str(), nullptr));
				CAGE_ASSERT(soundio);
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
