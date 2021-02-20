#ifndef guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
#define guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC

#include "core.h"

namespace cage
{
	struct SoundError : public SystemError
	{
		SoundError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
	};

	class CAGE_ENGINE_API SoundContext : private Immovable
	{
	public:
		string getContextName() const;
		string getBackendName() const;
	};

	struct CAGE_ENGINE_API SoundContextCreateConfig
	{};

	CAGE_ENGINE_API Holder<SoundContext> newSoundContext(const SoundContextCreateConfig &config, const string &name = "");

	struct CAGE_ENGINE_API SoundInterleavedBuffer
	{
		float *buffer = nullptr;
		uint32 frames = 0;
		uint32 channels = 0;

		SoundInterleavedBuffer();
		SoundInterleavedBuffer(const SoundInterleavedBuffer &other); // creates a reference of the original buffer
		SoundInterleavedBuffer(SoundInterleavedBuffer &&) = delete;
		~SoundInterleavedBuffer();
		SoundInterleavedBuffer &operator = (const SoundInterleavedBuffer &other); // creates a reference of the original buffer
		SoundInterleavedBuffer &operator = (SoundInterleavedBuffer &&) = delete;

		void resize(uint32 channels, uint32 frames);
		void clear();

	private:
		uint32 allocated = 0;
	};

	struct CAGE_ENGINE_API SoundDataBuffer : public SoundInterleavedBuffer
	{
		sint64 time = 0;
		uint32 sampleRate = 0;
	};

	class CAGE_ENGINE_API MixingBus : private Immovable
	{
	public:
		void addInput(MixingBus *bus);
		void removeInput(MixingBus *bus);
		void addOutput(MixingBus *bus);
		void removeOutput(MixingBus *bus);
		void clear();
	};

	CAGE_ENGINE_API Holder<MixingBus> newMixingBus();

	struct CAGE_ENGINE_API MixingFilterApi
	{
		SoundDataBuffer output;
		Delegate<void(const SoundDataBuffer &)> input;
	};

	class CAGE_ENGINE_API MixingFilter : private Immovable
	{
	public:
		void setBus(MixingBus *bus);
		Delegate<void(const MixingFilterApi &)> execute;
	};

	class CAGE_ENGINE_API VolumeFilter : private Immovable
	{
	public:
		Holder<MixingFilter> filter;
		real volume;
	};

	CAGE_ENGINE_API Holder<MixingFilter> newMixingFilter();
	CAGE_ENGINE_API Holder<VolumeFilter> newVolumeFilter();

	class CAGE_ENGINE_API SoundSource : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void clear();
		void setData(Holder<Polytone> &&poly);
		void setDataTone(uint32 pitch = 440);
		void setDataNoise();

		uint64 getDuration() const;

		void setDataRepeat(bool repeatBeforeStart, bool repeatAfterEnd);

		void addOutput(MixingBus *bus);
		void removeOutput(MixingBus *bus);
	};

	CAGE_ENGINE_API Holder<SoundSource> newSoundSource();

	CAGE_ENGINE_API AssetScheme genAssetSchemeSoundSource(uint32 threadIndex);
	static constexpr uint32 AssetSchemeIndexSoundSource = 20;

	class CAGE_ENGINE_API Speaker : private Immovable
	{
	public:
		string getStreamName() const;
		string getDeviceId() const;
		uint32 getChannels() const;
		uint32 getSamplerate() const;
		uint32 getLatency() const;

		void setInput(MixingBus *bus);
		void update(uint64 time);
	};

	struct CAGE_ENGINE_API SpeakerCreateConfig
	{
		string deviceId;
		uint32 sampleRate = 0;
	};

	CAGE_ENGINE_API Holder<Speaker> newSpeakerOutput(SoundContext *context, const SpeakerCreateConfig &config, const string &name = "");

	CAGE_ENGINE_API void soundSetSpeakerDirections(uint32 channels, const vec3 *directions);
	CAGE_ENGINE_API void soundGetSpeakerDirections(uint32 channels, vec3 *directions);
	CAGE_ENGINE_API void soundSetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, const real *matrix);
	CAGE_ENGINE_API void soundGetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, real *matrix);
}

#endif // guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
