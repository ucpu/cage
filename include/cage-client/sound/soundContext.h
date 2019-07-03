namespace cage
{
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
