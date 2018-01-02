namespace cage
{
	template struct CAGE_API eventDispatcher<bool(uint32, memoryBuffer&)>;
	template struct CAGE_API eventDispatcher<bool(uint32, string&)>;

	class CAGE_API assetManagerClass
	{
	public:
		template<class T> void defineScheme(uint32 index, const assetSchemeStruct &value) { zScheme(index, value, sizeof(T)); }

		void add(uint32 assetName);
		void fabricate(uint32 scheme, uint32 assetName, const string &textName = "");
		void remove(uint32 assetName);
		void reload(uint32 assetName, bool recursive = false);
		assetStateEnum state(uint32 assetName) const;
		bool ready(uint32 assetName) const;
		uint32 scheme(uint32 assetName) const;

		template<uint32 Scheme, class T> bool get(uint32 assetName, T *value) const
		{
			value = get<Scheme, T>(assetName);
			return !!value;
		}
		template<uint32 Scheme, class T> T *get(uint32 assetName) const
		{
			T *res = (T*)zGet(assetName);
			if (res)
			{
				CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
				CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			}
			return res;
		}
		template<uint32 Scheme> void *get(uint32 assetName) const
		{
			void *res = zGet(assetName);
			if (res)
			{
				CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
				CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == -1, zGetTypeSize(Scheme), assetName);
			}
			return res;
		}

		template<uint32 Scheme, class T> void set(uint32 assetName, T *value)
		{
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			zSet(assetName, value);
		}
		template<uint32 Scheme> void set(uint32 assetName, void *value)
		{
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == -1, zGetTypeSize(Scheme), assetName);
			zSet(assetName, value);
		}

		uint32 dependenciesCount(uint32 assetName) const;
		uint32 dependencyName(uint32 assetName, uint32 index) const;
		pointerRange<const uint32> dependencies(uint32 assetName) const;

		bool processCustomThread(uint32 threadIndex);
		bool processControlThread();

		uint32 countTotal() const;
		uint32 countProcessing() const;

		void listen(const string &address, uint16 port);

		eventDispatcher<bool(uint32, memoryBuffer&)> findAssetBuffer; // this event is called from the loading thread
		eventDispatcher<bool(uint32, string&)> findAssetPath; // this event is called from the loading thread

	private:
		void zScheme(uint32 index, const assetSchemeStruct &value, uintPtr typeSize);
		uintPtr zGetTypeSize(uint32 scheme) const;
		void *zGet(uint32 assetName) const;
		void zSet(uint32 assetName, void *value);
	};

	template<> inline void assetManagerClass::defineScheme<void>(uint32 index, const assetSchemeStruct &value) { zScheme(index, value, -1); }

	struct CAGE_API assetManagerCreateConfig
	{
		string path;
		uint32 threadMaxCount;
		uint32 schemeMaxCount;
		assetManagerCreateConfig();
	};

	CAGE_API holder<assetManagerClass> newAssetManager(const assetManagerCreateConfig &config);

	CAGE_API assetHeaderStruct initializeAssetHeader(const string &name, uint16 schemeIndex);
}
