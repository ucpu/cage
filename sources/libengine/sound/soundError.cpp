#include "private.h"

namespace cage
{
	SoundError::SoundError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept : SystemError(file, line, function, severity, message, code)
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
				CAGE_THROW_ERROR(SoundError, soundio_strerror(code), code);
				break;
			default:
				CAGE_THROW_CRITICAL(SoundError, "unknown sound error", code);
			}
		}
	}
}
