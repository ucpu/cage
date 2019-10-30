namespace cage
{
	struct soundError : public systemError
	{
		soundError(const char *file, uint32 line, const char *function, severityEnum severity, const char *message, uint32 code) noexcept;
	};

	class CAGE_API soundContext : private immovable
	{
	public:
		string getContextName() const;
	};

	struct CAGE_API soundContextCreateConfig
	{
		uintPtr linksMemory;
		soundContextCreateConfig();
	};

	CAGE_API holder<soundContext> newSoundContext(const soundContextCreateConfig &config, const string &name = "");
}
