#include <cage-core/core.h>
#include <cage-core/math.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

namespace cage
{
	soundError::soundError(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept : codeException(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER, code)
	{};

	namespace soundPrivat
	{
		void checkSoundIoError(int code)
		{
			switch (code)
			{
			case SoundIoErrorNone:
				return;
			case SoundIoErrorNoMem:
			case SoundIoErrorInitAudioBackend:
			case SoundIoErrorSystemResources:
			case SoundIoErrorOpeningDevice:
			case SoundIoErrorNoSuchDevice:
			case SoundIoErrorInvalid:
			case SoundIoErrorBackendUnavailable:
			case SoundIoErrorStreaming:
			case SoundIoErrorIncompatibleDevice:
			case SoundIoErrorNoSuchClient:
			case SoundIoErrorIncompatibleBackend:
			case SoundIoErrorBackendDisconnected:
			case SoundIoErrorInterrupted:
			case SoundIoErrorUnderflow:
			case SoundIoErrorEncodingString:
				CAGE_THROW_ERROR(soundError, soundio_strerror(code), code);
				break;
			default:
				CAGE_THROW_CRITICAL(soundError, "unknown sound error", code);
			}
		}
	}
}
