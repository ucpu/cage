#include <algorithm>
#include <atomic>
#include <cstdlib> // std::getenv
#include <functional>
#include <vector>

#include <svector.h>
#include <unordered_dense.h>

#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetsManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/memoryCompression.h>
#include <cage-core/networkTcp.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/serialization.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>
#include <cage-core/tasks.h>

namespace cage
{
	namespace detail
	{
		CAGE_API_EXPORT bool assetsListenDefault()
		{
			const char *env = std::getenv("CAGE_ASSETS_LISTEN");
			if (!env)
				return false;
			try
			{
				const bool r = toBool(trim(String(env)));
				CAGE_LOG(SeverityEnum::Info, "assetsManager", Stringizer() + "using environment variable CAGE_ASSETS_LISTEN, value: " + r);
				return r;
			}
			catch (const Exception &)
			{
				CAGE_LOG(SeverityEnum::Warning, "assetsManager", "failed parsing environment variable CAGE_ASSETS_LISTEN");
			}
			return false;
		}
	}

	namespace
	{
		class AssetsManagerImpl;

		// one particular version of an asset
		struct Asset : public AssetContext
		{
			Holder<void> ref; // the application receives shares of the ref, its destructor will call a method in the AssetsManager to schedule the asset for unloading
			bool failed = false;
			bool unloading = false;

			explicit Asset(AssetsManagerImpl *impl, uint32 assetId);
			explicit Asset(AssetsManagerImpl *impl, uint32 scheme, uint32 assetId, const String &textId, Holder<void> &&value);
			explicit Asset(AssetsManagerImpl *impl, uint32 scheme, uint32 assetId, const String &textId, const AssetsScheme &customScheme, Holder<void> &&customData);
			~Asset();

			AssetsManagerImpl *impl() const { return (AssetsManagerImpl *)assetsManager; };
		};

		// container for all versions of a single asset name
		struct Collection : private Noncopyable
		{
			ankerl::svector<Holder<Asset>, 1> versions;
			sint32 references = 0; // how many times was the asset added in the api
			bool fabricated = false; // once a fabricated asset is added to the collection, all future requests for reload are ignored
		};

		struct Work : private Noncopyable
		{
			Holder<Asset> asset;

			Work() = default;
			Work(Holder<Asset> asset);
			Work(Work &&other) { std::swap(asset, other.asset); }
			Work &operator=(Work &&other)
			{
				std::swap(asset, other.asset);
				return *this;
			}
			~Work();

			void doFetch();
			void doDecompress();
			void doLoad();
			void doPublish();
			void doUnload();
			void doRemove();
			void finish();
		};
		static_assert(std::movable<Work>);

		struct Waiting : private Immovable
		{
			Holder<Asset> asset;

			Waiting(Holder<Asset> &&asset);
			~Waiting();
		};

		class KeepOpen : private Immovable
		{
			Holder<Mutex> mut = newMutex();
			ankerl::unordered_dense::map<String, Holder<void>> items;

		public:
			void add(const String &path_)
			{
				const String path = pathJoin(path_, "..");
				ScopeLock l(mut);
				if (items.count(path))
					return;
				items[path] = detail::pathKeepOpen(path);
			}

			void clear()
			{
				ScopeLock l(mut);
				items.clear();
			}
		};

		String findAssetsFolderPath(const AssetManagerCreateConfig &config)
		{
			try
			{
				if (pathIsAbs(config.assetsFolderName))
				{
					if (pathIsDirectory(config.assetsFolderName))
						return config.assetsFolderName;
				}
				try
				{
					detail::OverrideException oe;
					return pathSearchTowardsRoot(config.assetsFolderName, PathTypeFlags::Directory | PathTypeFlags::Archive);
				}
				catch (const Exception &)
				{
					return pathSearchTowardsRoot(config.assetsFolderName, pathExtractDirectory(detail::executableFullPath()), PathTypeFlags::Directory | PathTypeFlags::Archive);
				}
			}
			catch (const Exception &)
			{
				CAGE_LOG(SeverityEnum::Error, "assetsManager", "failed to find the directory with assets");
				throw;
			}
		}

		class AssetsManagerImpl : public AssetsManager
		{
		public:
			const String path;
			Holder<Mutex> privateMutex = newMutex(); // protects generateId, privateIndex
			Holder<RwMutex> publicMutex = newRwMutex(); // protects publicIndex, waitingIndex
			KeepOpen keepOpen;
			std::vector<AssetsScheme> schemes;
			std::atomic<sint32> workingCounter = 0;
			std::atomic<sint32> existsCounter = 0;
			uint32 generateId = 0;
			ankerl::unordered_dense::map<uint32, Collection> privateIndex;
			ankerl::unordered_dense::map<uint32, Holder<Asset>> publicIndex;
			ankerl::unordered_dense::map<uint32, std::vector<Holder<Waiting>>> waitingIndex;
			std::vector<Holder<ConcurrentQueue<Work>>> customProcessingQueues;
			ConcurrentQueue<Holder<AsyncTask>> tasksCleanupQueue;
			ConcurrentQueue<Work> fetchQueue;
			std::vector<Holder<Thread>> fetchThreads;
			Holder<void> listener;

			AssetsManagerImpl(const AssetManagerCreateConfig &config) : path(findAssetsFolderPath(config))
			{
				CAGE_LOG(SeverityEnum::Info, "assetsManager", Stringizer() + "using assets path: " + path);
				schemes.resize(config.schemesMaxCount);
				customProcessingQueues.resize(config.customProcessingThreads);
				for (auto &it : customProcessingQueues)
					it = systemMemory().createHolder<ConcurrentQueue<Work>>();
				fetchThreads.reserve(config.diskLoadingThreads);
				for (uint32 i = 0; i < config.diskLoadingThreads; i++)
					fetchThreads.push_back(newThread(Delegate<void()>().bind<AssetsManagerImpl, &AssetsManagerImpl::diskLoadingEntry>(this), Stringizer() + "asset disk loading " + i));
			}

			~AssetsManagerImpl()
			{
				listener.clear();
				for (auto &it : customProcessingQueues)
					it->terminate();
				fetchQueue.terminate();
				fetchThreads.clear();
				CAGE_ASSERT(privateIndex.empty());
			}

			void diskLoadingEntry()
			{
				try
				{
					while (true)
					{
						while (true)
						{
							Holder<AsyncTask> t;
							if (!tasksCleanupQueue.tryPop(t))
								break;
							if (!t->done())
							{
								tasksCleanupQueue.push(std::move(t));
								break;
							}
						}
						{
							Work w;
							fetchQueue.pop(w);
							w.doFetch();
						}
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			bool customProcessEntry(uint32 threadIndex)
			{
				CAGE_ASSERT(threadIndex < customProcessingQueues.size());
				Work w;
				if (customProcessingQueues[threadIndex]->tryPop(w))
				{
					if (w.asset->unloading)
						w.doUnload();
					else
						w.doLoad();
					return true;
				}
				return false;
			}

			void enqueueDecompress(Holder<Asset> asset)
			{
				CAGE_ASSERT(!asset->failed);
				CAGE_ASSERT(asset->scheme < schemes.size());
				CAGE_ASSERT(asset->threadIndex == m || asset->threadIndex < customProcessingQueues.size());
				CAGE_ASSERT(asset->decompress);
				tasksCleanupQueue.push(tasksRunAsync("asset decompress", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doDecompress(); }), systemMemory().createHolder<Work>(std::move(asset))));
			}

			void enqueueLoad(Holder<Asset> asset)
			{
				CAGE_ASSERT(!asset->failed);
				CAGE_ASSERT(asset->scheme < schemes.size());
				CAGE_ASSERT(asset->threadIndex == m || asset->threadIndex < customProcessingQueues.size());
				CAGE_ASSERT(asset->load);
				if (asset->threadIndex == m)
					tasksCleanupQueue.push(tasksRunAsync("asset load", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doLoad(); }), systemMemory().createHolder<Work>(std::move(asset))));
				else
				{
					const uint32 ti = asset->threadIndex;
					customProcessingQueues[ti]->push(std::move(asset));
				}
			}

			void enqueuePublish(Holder<Asset> asset)
			{
				tasksCleanupQueue.push(tasksRunAsync("asset publish", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doPublish(); }), systemMemory().createHolder<Work>(std::move(asset))));
			}

			void enqueueUnload(Holder<Asset> asset)
			{
				CAGE_ASSERT(!asset->unloading);
				asset->unloading = true;
				if (asset->threadIndex == m)
					tasksCleanupQueue.push(tasksRunAsync("asset unload", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doUnload(); }), systemMemory().createHolder<Work>(std::move(asset))));
				else
				{
					CAGE_ASSERT(asset->threadIndex < customProcessingQueues.size());
					const uint32 ti = asset->threadIndex;
					customProcessingQueues[ti]->push(std::move(asset));
				}
			}

			void enqueueRemove(Holder<Asset> asset)
			{
				tasksCleanupQueue.push(tasksRunAsync("asset remove", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doRemove(); }), systemMemory().createHolder<Work>(std::move(asset))));
			}

			void publish(Holder<Asset> asset)
			{
				// private mutex already locked

				auto &c = asset->impl()->privateIndex[asset->assetId];
				if (c.references == 0)
				{
					asset->impl()->enqueueUnload(asset.share());
					return;
				}

				{
					CAGE_ASSERT(!asset->ref);
					struct PublicAssetReference : private cage::Immovable
					{
						Holder<Asset> asset;
						~PublicAssetReference() { asset->impl()->enqueueUnload(std::move(asset)); }
					};
					Holder<PublicAssetReference> ctrl = systemMemory().createHolder<PublicAssetReference>();
					ctrl->asset = asset.share();
					asset->ref = Holder<void>(asset->assetHolder ? +asset->assetHolder : (void *)1, std::move(ctrl));
				}

				{
					ScopeLock lock(asset->impl()->publicMutex, WriteLockTag());
					bool found = false;
					for (const auto &v : c.versions)
					{
						if (found)
							v->ref.clear();
						else if (v->ref || v->failed)
						{
							asset->impl()->unpublish(v->assetId);
							asset->impl()->publicIndex[v->assetId] = v.share();
							found = true;
						}
					}
					CAGE_ASSERT(found);
					asset->impl()->waitingIndex.erase(asset->assetId);
				}
			}

			void unpublish(uint32 assetId)
			{
				// public mutex already locked

				auto it = publicIndex.find(assetId);
				if (it != publicIndex.end())
					publicIndex.erase(it);
			}
		};

		void defaultFetch(AssetContext *asset);

		Asset::Asset(AssetsManagerImpl *impl, uint32 assetId) : AssetContext(impl, assetId)
		{
			textId = Stringizer() + "<" + assetId + ">";
			impl->existsCounter++;
			fetch = defaultFetch;
		}

		Asset::Asset(AssetsManagerImpl *impl, uint32 scheme_, uint32 assetId, const String &textId_, Holder<void> &&value_) : AssetContext(impl, assetId)
		{
			textId = textId_.empty() ? (Stringizer() + "<" + assetId + "> with value") : textId_;
			scheme = scheme_;
			impl->existsCounter++;
			*(AssetsScheme *)this = impl->schemes[scheme];
			assetHolder = std::move(value_);
		}

		Asset::Asset(AssetsManagerImpl *impl, uint32 scheme_, uint32 assetId, const String &textId_, const AssetsScheme &customScheme_, Holder<void> &&customData_) : AssetContext(impl, assetId)
		{
			textId = textId_.empty() ? (Stringizer() + "<" + assetId + "> with custom scheme") : textId_;
			scheme = scheme_;
			impl->existsCounter++;
			*(AssetsScheme *)this = customScheme_;
			threadIndex = impl->schemes[scheme].threadIndex; // prevent thread confusion
			typeHash = impl->schemes[scheme].typeHash; // prevent type confusion
			customData = std::move(customData_);
		}

		Asset::~Asset()
		{
			impl()->existsCounter--;
		}

		Work::Work(Holder<Asset> asset_) : asset(std::move(asset_))
		{
			asset->impl()->workingCounter++;
		}

		Work::~Work()
		{
			finish();
		}

		void Work::doFetch()
		{
			ScopeGuard exit([&] { finish(); });

			ProfilingScope profiling("asset fetching");
			profiling.set(asset->textId);
			CAGE_ASSERT(asset->fetch);

			try
			{
				detail::OverrideBreakpoint OverrideBreakpoint;
				asset->fetch(+asset);
			}
			catch (...)
			{
				asset->dependencies.clear();
				asset->failed = true;
				asset->impl()->enqueuePublish(asset.share());
				return;
			}

			CAGE_ASSERT(asset->scheme < asset->impl()->schemes.size());
			CAGE_ASSERT(asset->threadIndex == m || asset->threadIndex < asset->impl()->customProcessingQueues.size());

			for (uint32 n : asset->dependencies)
				asset->impl()->load(n);

			if (asset->decompress && asset->compressedData.size())
				asset->impl()->enqueueDecompress(asset.share());
			else if (asset->load)
				asset->impl()->enqueueLoad(asset.share());
			else
				asset->impl()->enqueuePublish(asset.share());
		}

		void Work::doDecompress()
		{
			ScopeGuard exit([&] { finish(); });

			ProfilingScope profiling("asset decompression");
			profiling.set(asset->textId);
			CAGE_ASSERT(!asset->failed);
			CAGE_ASSERT(!asset->assetHolder);
			CAGE_ASSERT(asset->decompress);

			try
			{
				asset->decompress(+asset);
			}
			catch (...)
			{
				asset->failed = true;
				asset->impl()->enqueuePublish(asset.share());
				return;
			}

			if (asset->load)
				asset->impl()->enqueueLoad(asset.share());
			else
				asset->impl()->enqueuePublish(asset.share());
		}

		void Work::doLoad()
		{
			ScopeGuard exit([&] { finish(); });

			ProfilingScope profiling("asset loading");
			profiling.set(asset->textId);
			CAGE_ASSERT(!asset->failed);
			CAGE_ASSERT(!asset->assetHolder);
			CAGE_ASSERT(asset->load);

			try
			{
				asset->load(+asset);
			}
			catch (...)
			{
				asset->assetHolder.clear();
				asset->failed = true;
				asset->impl()->enqueuePublish(asset.share());
				return;
			}

			if (asset->dependencies.empty())
				asset->impl()->enqueuePublish(asset.share());
			else
			{
				Holder<Waiting> w = systemMemory().createHolder<Waiting>(asset.share());
				ScopeLock lock(asset->impl()->publicMutex, WriteLockTag());
				for (uint32 d : asset->dependencies)
					if (asset->impl()->publicIndex.count(d) == 0)
						asset->impl()->waitingIndex[d].push_back(w.share());
			}
		}

		void Work::doPublish()
		{
			ScopeGuard exit([&] { finish(); });

			asset->customData.clear();
			asset->compressedData.clear();
			asset->originalData.clear();

			ScopeLock lock(asset->impl()->privateMutex);
			asset->impl()->publish(asset.share());
		}

		void Work::doUnload()
		{
			ScopeGuard exit([&] { finish(); });

			ProfilingScope profiling("asset unloading");
			profiling.set(asset->textId);

			asset->assetHolder.clear();

			asset->impl()->enqueueRemove(asset.share());
		}

		void Work::doRemove()
		{
			ScopeGuard exit([&] { finish(); });

			for (uint32 n : asset->dependencies)
				asset->impl()->unload(n);
			asset->dependencies.clear();

			ScopeLock lock(asset->impl()->privateMutex);
			auto &c = asset->impl()->privateIndex[asset->assetId];
			std::erase_if(c.versions, [&](const Holder<Asset> &it) { return +it == +asset; });
			if (c.versions.empty())
				asset->impl()->privateIndex.erase(asset->assetId);
		}

		void Work::finish()
		{
			if (asset)
			{
				if (asset->impl()->workingCounter-- == 1)
					asset->impl()->keepOpen.clear();
				asset.clear();
			}
		}

		Waiting::Waiting(Holder<Asset> &&asset_) : asset(std::move(asset_))
		{
			asset->impl()->workingCounter++;
		}

		Waiting::~Waiting()
		{
			asset->impl()->enqueuePublish(asset.share());
			if (asset->impl()->workingCounter-- == 1)
				asset->impl()->keepOpen.clear();
		}
	}

	void AssetsManager::load(uint32 assetId)
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[assetId];
		if (c.references++ == 0)
		{
			Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, assetId);
			c.versions.insert(c.versions.begin(), asset.share());
			lock.clear();
			impl->fetchQueue.push(std::move(asset));
		}
	}

	void AssetsManager::unload(uint32 assetId)
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		CAGE_ASSERT(impl->privateIndex.count(assetId) == 1);
		auto &c = impl->privateIndex.at(assetId);
		if (c.references-- == 1)
		{
			{
				ScopeLock lock(impl->publicMutex, WriteLockTag());
				impl->unpublish(assetId);
			}
			for (auto &a : c.versions)
				a->ref.clear();
		}
	}

	void AssetsManager::reload(uint32 assetId)
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		auto it = impl->privateIndex.find(assetId);
		if (it == impl->privateIndex.end())
			return;
		auto &c = it->second;
		if (!c.fabricated)
		{
			Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, assetId);
			c.versions.insert(c.versions.begin(), asset.share());
			lock.clear();
			impl->fetchQueue.push(std::move(asset));
		}
	}

	uint32 AssetsManager::generateUniqueId()
	{
		static constexpr uint32 a = (uint32)1 << 28;
		static constexpr uint32 b = (uint32)1 << 30;
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		uint32 &name = impl->generateId;
		ScopeLock lock(impl->privateMutex);
		if (name < a || name > b)
			name = a;
		while (impl->privateIndex.count(name))
			name = name == b ? a : name + 1;
		return name++;
	}

	bool AssetsManager::check(uint32 assetId) const
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		const AssetsManagerImpl *impl = (const AssetsManagerImpl *)this;
		const auto &publicIndex = impl->publicIndex;
		ScopeLock lock(impl->publicMutex, ReadLockTag());
		auto it = publicIndex.find(assetId);
		if (it == publicIndex.end())
			return false; // not found
		return !it->second->failed;
	}

	void AssetsManager::listen(const String &address, uint16 port, uint64 listenerPeriod)
	{
		class AssetListenerImpl : private Immovable
		{
		public:
			String address;
			AssetsManager *const man = nullptr;
			Holder<TcpConnection> tcp;
			Holder<Thread> thread;
			const uint64 period = 0;
			const uint16 port = 0;
			std::atomic<bool> stopping = false;

			AssetListenerImpl(AssetsManager *man, const String &address, uint64 period, uint32 port) : address(address), man(man), period(period), port(port) { thread = newThread(Delegate<void()>().bind<AssetListenerImpl, &AssetListenerImpl::thrEntry>(this), "assets listener"); }

			~AssetListenerImpl()
			{
				stopping = true;
				thread.clear();
			}

			void thrEntry()
			{
				try
				{
					detail::OverrideBreakpoint overrideBreakpoint;
					detail::OverrideException overrideException;
					tcp = newTcpConnection(address, port);
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetsManager", "assets network notifications listener failed to connect to the database");
					return;
				}
				try
				{
					while (!stopping)
					{
						threadSleep(period);
						String line;
						while (tcp->readLine(line))
						{
							const uint32 name = HashString(line);
							CAGE_LOG(SeverityEnum::Info, "assetsManager", Stringizer() + "reloading asset: " + line + " (" + name + ")");
							man->reload(name);
						}
					}
				}
				catch (const Disconnected &)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetsManager", "assets network notifications listener disconnected");
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetsManager", "assets network notifications listener failed with exception");
					detail::logCurrentCaughtException();
				}
				tcp.clear();
			}
		};

		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		impl->listener.clear();
		impl->listener = systemMemory().createHolder<AssetListenerImpl>(this, address, listenerPeriod, port).template cast<void>();
	}

	bool AssetsManager::processCustomThread(uint32 threadIndex)
	{
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		return impl->customProcessEntry(threadIndex);
	}

	void AssetsManager::unloadCustomThread(uint32 threadIndex)
	{
		while (!empty())
		{
			while (processCustomThread(threadIndex))
				;
			threadYield();
		}
	}

	void AssetsManager::waitTillEmpty()
	{
		while (!empty())
			threadYield();
	}

	bool AssetsManager::processing() const
	{
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		return impl->workingCounter > 0;
	}

	bool AssetsManager::empty() const
	{
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		return impl->workingCounter == 0 && impl->existsCounter == 0;
	}

	void AssetsManager::defineScheme_(uint32 typeHash, uint32 scheme, const AssetsScheme &value, bool allowOverride)
	{
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		CAGE_ASSERT(typeHash != m);
		CAGE_ASSERT(scheme < impl->schemes.size());
		CAGE_ASSERT(value.typeHash == typeHash);
		CAGE_ASSERT(value.threadIndex == m || value.threadIndex < impl->customProcessingQueues.size());
		if (!allowOverride)
		{
			CAGE_ASSERT(impl->schemes[scheme].typeHash == m); // the scheme was not defined previously
		}
		impl->schemes[scheme] = value;
	}

	void AssetsManager::load_(uint32 scheme, uint32 assetId, const String &textId, Holder<void> &&value)
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[assetId];
		c.fabricated = true;
		c.references++;
		Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, scheme, assetId, textId, std::move(value));
		c.versions.insert(c.versions.begin(), asset.share());
		impl->publish(std::move(asset));
	}

	void AssetsManager::load_(uint32 scheme, uint32 assetId, const String &textId, const AssetsScheme &customScheme, Holder<void> &&customData)
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		AssetsManagerImpl *impl = (AssetsManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		CAGE_ASSERT(customScheme.threadIndex == m);
		CAGE_ASSERT(customScheme.typeHash == m);
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[assetId];
		c.fabricated = true;
		c.references++;
		Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, scheme, assetId, textId, customScheme, std::move(customData));
		c.versions.insert(c.versions.begin(), asset.share());
		lock.clear();
		if (asset->fetch)
			impl->fetchQueue.push(std::move(asset));
		else if (asset->decompress)
			impl->enqueueDecompress(std::move(asset));
		else if (asset->load)
			impl->enqueueLoad(std::move(asset));
		else
		{
			asset->failed = true;
			asset->impl()->enqueuePublish(asset.share());
		}
	}

	Holder<void> AssetsManager::get1_(uint32 scheme, uint32 assetId) const
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		const AssetsManagerImpl *impl = (const AssetsManagerImpl *)this;
		const auto &schemes = impl->schemes;
		const auto &publicIndex = impl->publicIndex;
		CAGE_ASSERT(scheme < schemes.size());
		CAGE_ASSERT(schemes[scheme].load);
		ScopeLock lock(impl->publicMutex, ReadLockTag());
		auto it = publicIndex.find(assetId);
		if (it == publicIndex.end())
			return {}; // not found
		const auto &a = it->second;
		if (a->scheme != scheme || a->failed)
			return {}; // different scheme
		CAGE_ASSERT(a->ref);
		CAGE_ASSERT(+a->ref != (void *)1);
		return a->ref.share();
	}

	Holder<void> AssetsManager::get2_(uint32 typeHash, uint32 assetId) const
	{
		CAGE_ASSERT(assetId != 0 && assetId != m);
		const AssetsManagerImpl *impl = (const AssetsManagerImpl *)this;
		const auto &publicIndex = impl->publicIndex;
		ScopeLock lock(impl->publicMutex, ReadLockTag());
		auto it = publicIndex.find(assetId);
		if (it == publicIndex.end())
			return {}; // not found
		const auto &a = it->second;
		if (a->typeHash != typeHash || a->failed)
			return {}; // different type
		CAGE_ASSERT(a->ref);
		CAGE_ASSERT(+a->ref != (void *)1);
		return a->ref.share();
	}

	uint32 AssetsManager::schemeTypeHash_(uint32 scheme) const
	{
		const AssetsManagerImpl *impl = (const AssetsManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		return impl->schemes[scheme].typeHash;
	}

	Holder<AssetsManager> newAssetsManager(const AssetManagerCreateConfig &config)
	{
		return systemMemory().createImpl<AssetsManager, AssetsManagerImpl>(config);
	}

	namespace
	{
		constexpr uint32 CurrentAssetVersion = 3;

		void defaultFetch(AssetContext *asset)
		{
			AssetsManagerImpl *impl = ((Asset *)asset)->impl();

			Holder<File> file;
			{
				String foundName;
				if (impl->findAsset.dispatch(asset->assetId, foundName, file))
				{
					if (!file && !foundName.empty())
						file = readFile(foundName);
				}
				else
				{
					CAGE_ASSERT(!file && foundName.empty());
					foundName = pathJoin(impl->path, Stringizer() + asset->assetId);
					file = readFile(foundName);
				}
				if (!foundName.empty())
					impl->keepOpen.add(foundName);
			}
			CAGE_ASSERT(file);

			AssetHeader h;
			file->read(bufferView<char>(h));
			if (detail::memcmp(h.cageName.data(), "cageAss", 8) != 0)
				CAGE_THROW_ERROR(Exception, "file is not a cage asset");
			if (h.version != CurrentAssetVersion)
				CAGE_THROW_ERROR(Exception, "cage asset version mismatch");
			asset->textId = h.textId.data();
			if (h.scheme >= impl->schemes.size())
				CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
			asset->scheme = h.scheme;

			*((AssetsScheme *)asset) = impl->schemes[asset->scheme];
			if (!impl->schemes[asset->scheme].load)
				return;

			{
				PointerRangeHolder<uint32> deps;
				deps.resize(h.dependenciesCount);
				file->read(bufferCast<char, uint32>(deps));
				asset->dependencies = std::move(deps);
			}

			if (h.compressedSize)
				asset->compressedData = systemMemory().createBuffer(h.compressedSize);
			if (h.originalSize)
				asset->originalData = systemMemory().createBuffer(h.originalSize);
			if (h.compressedSize || h.originalSize)
				file->read(h.compressedSize ? asset->compressedData : asset->originalData);

			CAGE_ASSERT(file->tell() == file->size());

			const auto f = impl->schemes[asset->scheme].fetch;
			if (f && f != AssetDelegate().bind<defaultFetch>())
				impl->schemes[asset->scheme].fetch(asset);
		}

		void defaultDecompress(AssetContext *asset)
		{
			if (asset->compressedData.size() == 0)
				return;
			memoryDecompress(*asset->compressedData, *asset->originalData);
		}
	}

	AssetsScheme::AssetsScheme()
	{
		fetch.bind<defaultFetch>();
		decompress.bind<defaultDecompress>();
	}

	AssetHeader::AssetHeader(const String &name_, uint16 schemeIndex)
	{
		version = CurrentAssetVersion;
		String name = name_;
		static constexpr uint32 MaxTexName = AssetLabel::MaxLength;
		if (name.length() > MaxTexName)
			name = String() + ".." + subString(name, name.length() - MaxTexName + 2, m);
		CAGE_ASSERT(name.length() <= MaxTexName);
		CAGE_ASSERT(name.length() > 0);
		textId = name;
		scheme = schemeIndex;
	}
}
