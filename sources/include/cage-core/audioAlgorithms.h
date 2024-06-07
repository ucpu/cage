#ifndef guard_audioAlgorithms_h_sd5rf4g6t5r
#define guard_audioAlgorithms_h_sd5rf4g6t5r

#include <cage-core/audio.h>

namespace cage
{
	CAGE_CORE_API void audioSetSampleRate(Audio *snd, uint32 sampleRate); // preserve number of frames and change duration
	CAGE_CORE_API void audioConvertSampleRate(Audio *snd, uint32 sampleRate, uint32 quality = 4); // preserve duration and change number of frames
	CAGE_CORE_API void audioConvertFrames(Audio *snd, uintPtr frames, uint32 quality = 4); // preserve duration and change sample rate
	CAGE_CORE_API void audioConvertFormat(Audio *snd, AudioFormatEnum format);
	CAGE_CORE_API void audioGain(Audio *snd, Real gain);

	// copies parts of an audio into another audio
	// if the target and source audios are the same instance, the source range and target range cannot overlap
	// if the target audio is not initialized and the targetFrameOffset is zero, it will be initialized with channels and format of the source audio
	// if the audios have different format, transferred samples will be converted to the target format
	// both audios must have same number of channels
	// sample rate is ignored (except when initializing new audio)
	CAGE_CORE_API void audioBlit(const Audio *source, Audio *target, uintPtr sourceFrameOffset, uintPtr targetFrameOffset, uintPtr frames);
}

#endif // guard_audioAlgorithms_h_sd5rf4g6t5r
