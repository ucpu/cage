#ifndef guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
#define guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t

namespace cage
{
	enum class assetStateEnum : uint32
	{
		Ready,
		NotFound,
		Error,
		Unknown,
	};

	class CAGE_API assetManager : private immovable
	{
	public:
		template<class T>
		void defineScheme(uint32 index, const assetScheme &value)
		{
			zScheme(index, value, sizeof(T));
		}

		void add(uint32 assetName);
		void fabricate(uint32 scheme, uint32 assetName, const string &textName = "");
		void remove(uint32 assetName);
		void reload(uint32 assetName, bool recursive = false);
		assetStateEnum state(uint32 assetName) const;
		bool ready(uint32 assetName) const;
		uint32 scheme(uint32 assetName) const;
		uint32 generateUniqueName();

		template<uint32 Scheme, class T>
		T *tryGet(uint32 assetName) const
		{
			if (!ready(assetName))
				return nullptr;
			return get<Scheme, T>(assetName);
		}
		template<uint32 Scheme>
		void *tryGet(uint32 assetName) const
		{
			if (!ready(assetName))
				return nullptr;
			return get<Scheme>(assetName);
		}

		template<uint32 Scheme, class T>
		T *get(uint32 assetName) const
		{
			CAGE_ASSERT_RUNTIME(ready(assetName));
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			return (T*)zGet(assetName);
		}
		template<uint32 Scheme>
		void *get(uint32 assetName) const
		{
			CAGE_ASSERT_RUNTIME(ready(assetName));
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == m, zGetTypeSize(Scheme), assetName);
			return zGet(assetName);
		}

		template<uint32 Scheme, class T>
		void set(uint32 assetName, T *value)
		{
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == sizeof(T), zGetTypeSize(Scheme), sizeof(T), assetName);
			zSet(assetName, value);
		}
		template<uint32 Scheme>
		void set(uint32 assetName, void *value)
		{
			CAGE_ASSERT_RUNTIME(scheme(assetName) == Scheme, assetName, scheme(assetName), Scheme);
			CAGE_ASSERT_RUNTIME(zGetTypeSize(Scheme) == m, zGetTypeSize(Scheme), assetName);
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

		eventDispatcher<bool(uint32, memoryBuffer&)> findAssetBuffer; // this event is called from the loading threadHandle
		eventDispatcher<bool(uint32, string&)> findAssetPath; // this event is called from the loading threadHandle

	private:
		void zScheme(uint32 index, const assetScheme &value, uintPtr typeSize);
		uintPtr zGetTypeSize(uint32 scheme) const;
		void *zGet(uint32 assetName) const;
		void zSet(uint32 assetName, void *value);
	};

	template<>
	inline void assetManager::defineScheme<void>(uint32 index, const assetScheme &value)
	{
		zScheme(index, value, m);
	}

	struct CAGE_API assetManagerCreateConfig
	{
		string assetsFolderName;
		uint32 threadMaxCount;
		uint32 schemeMaxCount;
		assetManagerCreateConfig();
	};

	CAGE_API holder<assetManager> newAssetManager(const assetManagerCreateConfig &config);

	CAGE_API assetScheme genAssetSchemePack(const uint32 threadIndex);
	static const uint32 assetSchemeIndexPack = 0;

	CAGE_API assetScheme genAssetSchemeRaw(const uint32 threadIndex);
	static const uint32 assetSchemeIndexRaw = 1;
}

#endif // guard_assetsManager_h_s54dhg56sr4ht564fdrsh6t
