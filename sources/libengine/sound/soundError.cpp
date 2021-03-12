#include "private.h"

#include <cubeb/cubeb.h>

namespace cage
{
	namespace soundPrivat
	{
		void checkSoundIoError(int code)
		{
			switch (code)
			{
			case CUBEB_OK:
				return;
			case CUBEB_ERROR:
				CAGE_THROW_ERROR(SoundError, "generic sound error", code);
				break;
			case CUBEB_ERROR_INVALID_FORMAT:
				CAGE_THROW_ERROR(SoundError, "invalid sound format", code);
				break;
			case CUBEB_ERROR_INVALID_PARAMETER:
				CAGE_THROW_ERROR(SoundError, "invalid sound parameter", code);
				break;
			case CUBEB_ERROR_NOT_SUPPORTED:
				CAGE_THROW_ERROR(SoundError, "sound not supported error", code);
				break;
			case CUBEB_ERROR_DEVICE_UNAVAILABLE:
				CAGE_THROW_ERROR(SoundError, "sound device unavailable", code);
				break;
			default:
				CAGE_THROW_CRITICAL(SoundError, "unknown sound error", code);
			}
		}
	}
}
