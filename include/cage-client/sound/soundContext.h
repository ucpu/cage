namespace cage
{
	class CAGE_API soundContextClass : private immovable
	{
	public:
		string getContextName() const;
	};

	struct CAGE_API soundContextCreateConfig
	{
		uintPtr linksMemory;
		soundContextCreateConfig();
	};

	CAGE_API holder<soundContextClass> newSoundContext(const soundContextCreateConfig &config, const string &name = "");
}
