namespace cage
{
	struct soundError : public codeException
	{
		soundError(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept;
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
