namespace cage
{
	struct SoundError : public SystemError
	{
		SoundError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
	};

	class CAGE_API SoundContext : private Immovable
	{
	public:
		string getContextName() const;
	};

	struct CAGE_API SoundContextCreateConfig
	{
		uintPtr linksMemory;
		SoundContextCreateConfig();
	};

	CAGE_API Holder<SoundContext> newSoundContext(const SoundContextCreateConfig &config, const string &name = "");
}
