namespace cage
{
	struct soundError : public SystemError
	{
		soundError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
	};

	class CAGE_API soundContext : private Immovable
	{
	public:
		string getContextName() const;
	};

	struct CAGE_API soundContextCreateConfig
	{
		uintPtr linksMemory;
		soundContextCreateConfig();
	};

	CAGE_API Holder<soundContext> newSoundContext(const soundContextCreateConfig &config, const string &name = "");
}
