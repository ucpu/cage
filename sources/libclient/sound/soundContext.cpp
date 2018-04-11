#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/memory.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

namespace cage
{
	namespace
	{
		void rtprioWarningCallback()
		{
			static bool fired = false;
			if (fired)
				return;
			CAGE_LOG(severityEnum::Warning, "sound", "failed to set thread priority for audio");
			fired = true;
		}

		class soundContextImpl : public soundContextClass
		{
		public:
			string name;
			SoundIo *soundio;
			memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeSet<void*>)>, memoryConcurrentPolicyNone> linksMemory;

			soundContextImpl(const soundContextCreateConfig &config, const string &name) : name(name.replace(":", "_")), soundio(nullptr), linksMemory(config.linksMemory)
			{
				CAGE_LOG(severityEnum::Info, "sound", string() + "creating sound context, name: '" + name + "'");
				soundio = soundio_create();
				if (!soundio)
					CAGE_THROW_ERROR(soundException, "error create soundio", 0);
				soundio->app_name = this->name.c_str();
				soundio->emit_rtprio_warning = &rtprioWarningCallback;
				checkSoundIoError(soundio_connect(soundio));
				soundio_flush_events(soundio);
			}

			~soundContextImpl()
			{
				soundio_disconnect(soundio);
				soundio_destroy(soundio);
			}
		};
	}

	namespace soundPrivat
	{
		SoundIo *soundioFromContext(soundContextClass *context)
		{
			soundContextImpl *impl = (soundContextImpl*)context;
			return impl->soundio;
		}

		memoryArena linksArenaFromContext(soundContextClass *context)
		{
			soundContextImpl *impl = (soundContextImpl*)context;
			return memoryArena(&impl->linksMemory);
		}
	}

	string soundContextClass::getContextName() const
	{
		soundContextImpl *impl = (soundContextImpl*)this;
		return impl->name;
	}

	soundContextCreateConfig::soundContextCreateConfig() : linksMemory(1024 * 1024)
	{}

	holder<soundContextClass> newSoundContext(const soundContextCreateConfig &config, const string &name)
	{
		return detail::systemArena().createImpl <soundContextClass, soundContextImpl>(config, name);
	}
}