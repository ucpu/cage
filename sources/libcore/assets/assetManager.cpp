#include <algorithm>
#include <atomic>
#include <functional>
#include <vector>

#include <unordered_dense.h>

#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetManager.h>
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
#include <cage-core/serialization.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>
#include <cage-core/tasks.h>

namespace cage
{
	namespace
	{
		class AssetManagerImpl;

		struct ScopeExit : private Immovable
		{
		public:
			ScopeExit(const std::function<void()> &callback) : callback_(callback) {}
			~ScopeExit() { callback_(); }

		private:
			std::function<void()> callback_;
		};

		// one particular version of an asset
		struct Asset : public AssetContext, AssetScheme
		{
			Holder<void> ref; // the application receives shares of the ref, its destructor will call a method in the AssetManager to schedule the asset for unloading
			AssetManagerImpl *const impl = nullptr;
			bool failed = false;
			bool unloading = false;

			explicit Asset(AssetManagerImpl *impl, uint32 realName);
			explicit Asset(AssetManagerImpl *impl, uint32 scheme, uint32 realName, const String &textName, Holder<void> &&value);
			explicit Asset(AssetManagerImpl *impl, uint32 scheme, uint32 realName, const String &textName, const AssetScheme &customScheme, Holder<void> &&customData);
			~Asset();
		};

		// container for all versions of a single asset name
		struct Collection : private Noncopyable
		{
			std::vector<Holder<Asset>> versions; // todo replace with a small vector
			sint32 references = 0; // how many times was the asset added in the api
			bool fabricated = false; // once a fabricated asset is added to the collection, all future requests for reload are ignored
		};

		struct Work : private Noncopyable
		{
			Holder<Asset> asset;

			Work() = default;
			Work(Holder<Asset> asset);
			Work(Work &&other) noexcept { std::swap(asset, other.asset); }
			Work &operator=(Work &&other) noexcept
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
				CAGE_LOG(SeverityEnum::Error, "assetManager", "failed to find the directory with assets");
				throw;
			}
		}

		class AssetManagerImpl : public AssetManager
		{
		public:
			const String path;
			Holder<Mutex> privateMutex = newMutex(); // protects generateName, privateIndex
			Holder<RwMutex> publicMutex = newRwMutex(); // protects publicIndex, waitingIndex
			KeepOpen keepOpen;
			std::vector<AssetScheme> schemes;
			std::atomic<sint32> workingCounter = 0;
			std::atomic<sint32> existsCounter = 0;
			uint32 generateName = 0;
			ankerl::unordered_dense::map<uint32, Collection> privateIndex;
			ankerl::unordered_dense::map<uint32, Holder<Asset>> publicIndex;
			ankerl::unordered_dense::map<uint32, std::vector<Holder<Waiting>>> waitingIndex;
			std::vector<Holder<ConcurrentQueue<Work>>> customProcessingQueues;
			ConcurrentQueue<Holder<AsyncTask>> tasksCleanupQueue;
			ConcurrentQueue<Work> fetchQueue;
			std::vector<Holder<Thread>> fetchThreads;
			Holder<void> listener;

			AssetManagerImpl(const AssetManagerCreateConfig &config) : path(findAssetsFolderPath(config))
			{
				CAGE_LOG(SeverityEnum::Info, "assetManager", Stringizer() + "using assets path: '" + path + "'");
				schemes.resize(config.schemesMaxCount);
				customProcessingQueues.resize(config.customProcessingThreads);
				for (auto &it : customProcessingQueues)
					it = systemMemory().createHolder<ConcurrentQueue<Work>>();
				fetchThreads.reserve(config.diskLoadingThreads);
				for (uint32 i = 0; i < config.diskLoadingThreads; i++)
					fetchThreads.push_back(newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::diskLoadingEntry>(this), Stringizer() + "asset disk loading " + i));
			}

			~AssetManagerImpl()
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
					customProcessingQueues[asset->threadIndex]->push(asset.share());
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
					customProcessingQueues[asset->threadIndex]->push(asset.share());
				}
			}

			void enqueueRemove(Holder<Asset> asset)
			{
				tasksCleanupQueue.push(tasksRunAsync("asset remove", Delegate<void(Work &, uint32)>([](Work &w, uint32) { w.doRemove(); }), systemMemory().createHolder<Work>(std::move(asset))));
			}

			void publish(Holder<Asset> asset)
			{
				// private mutex already locked

				auto &c = asset->impl->privateIndex[asset->realName];
				if (c.references == 0)
				{
					asset->impl->enqueueUnload(asset.share());
					return;
				}

				{
					CAGE_ASSERT(!asset->ref);
					struct PublicAssetReference : private cage::Immovable
					{
						Holder<Asset> asset;
						~PublicAssetReference() { asset->impl->enqueueUnload(std::move(asset)); }
					};
					Holder<PublicAssetReference> ctrl = systemMemory().createHolder<PublicAssetReference>();
					ctrl->asset = asset.share();
					asset->ref = Holder<void>(asset->assetHolder ? +asset->assetHolder : (void *)1, std::move(ctrl));
				}

				{
					ScopeLock lock(asset->impl->publicMutex, WriteLockTag());
					bool found = false;
					for (const auto &v : c.versions)
					{
						if (found)
							v->ref.clear();
						else if (v->ref || v->failed)
						{
							asset->impl->unpublish(v->realName);
							asset->impl->publicIndex[v->realName] = v.share();
							found = true;
						}
					}
					CAGE_ASSERT(found);
					asset->impl->waitingIndex.erase(asset->realName);
				}
			}

			void unpublish(uint32 realName)
			{
				// public mutex already locked

				auto it = publicIndex.find(realName);
				if (it != publicIndex.end())
					publicIndex.erase(it);
			}
		};

		void defaultFetch(AssetContext *asset);

		Asset::Asset(AssetManagerImpl *impl, uint32 realName) : AssetContext(realName), impl(impl)
		{
			textName = Stringizer() + "<" + realName + ">";
			impl->existsCounter++;
			fetch = defaultFetch;
		}

		Asset::Asset(AssetManagerImpl *impl, uint32 scheme_, uint32 realName, const String &textName_, Holder<void> &&value_) : AssetContext(realName), impl(impl)
		{
			textName = textName_.empty() ? (Stringizer() + "<" + realName + "> with value") : textName_;
			scheme = scheme_;
			impl->existsCounter++;
			assetHolder = std::move(value_);
		}

		Asset::Asset(AssetManagerImpl *impl, uint32 scheme_, uint32 realName, const String &textName_, const AssetScheme &customScheme_, Holder<void> &&customData_) : AssetContext(realName), impl(impl)
		{
			textName = textName_.empty() ? (Stringizer() + "<" + realName + "> with custom scheme") : textName_;
			scheme = scheme_;
			impl->existsCounter++;
			*(AssetScheme *)this = customScheme_;
			threadIndex = impl->schemes[scheme].threadIndex;
			typeHash = impl->schemes[scheme].typeHash;
			customData = std::move(customData_);
		}

		Asset::~Asset()
		{
			impl->existsCounter--;
		}

		Work::Work(Holder<Asset> asset_) : asset(std::move(asset_))
		{
			asset->impl->workingCounter++;
		}

		Work::~Work()
		{
			finish();
		}

		void Work::doFetch()
		{
			ScopeExit exit([&] { finish(); });

			ProfilingScope profiling("asset fetching");
			profiling.set(asset->textName);
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
				asset->impl->enqueuePublish(asset.share());
				return;
			}

			CAGE_ASSERT(asset->scheme < asset->impl->schemes.size());
			CAGE_ASSERT(asset->threadIndex == m || asset->threadIndex < asset->impl->customProcessingQueues.size());

			for (uint32 n : asset->dependencies)
				asset->impl->load(n);

			if (asset->decompress && asset->compressedData.size())
				asset->impl->enqueueDecompress(asset.share());
			else if (asset->load)
				asset->impl->enqueueLoad(asset.share());
			else
				asset->impl->enqueuePublish(asset.share());
		}

		void Work::doDecompress()
		{
			ScopeExit exit([&] { finish(); });

			ProfilingScope profiling("asset decompression");
			profiling.set(asset->textName);
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
				asset->impl->enqueuePublish(asset.share());
				return;
			}

			if (asset->load)
				asset->impl->enqueueLoad(asset.share());
			else
				asset->impl->enqueuePublish(asset.share());
		}

		void Work::doLoad()
		{
			ScopeExit exit([&] { finish(); });

			ProfilingScope profiling("asset loading");
			profiling.set(asset->textName);
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
				asset->impl->enqueuePublish(asset.share());
				return;
			}

			if (asset->dependencies.empty())
				asset->impl->enqueuePublish(asset.share());
			else
			{
				Holder<Waiting> w = systemMemory().createHolder<Waiting>(asset.share());
				ScopeLock lock(asset->impl->publicMutex, WriteLockTag());
				for (uint32 d : asset->dependencies)
					if (asset->impl->publicIndex.count(d) == 0)
						asset->impl->waitingIndex[d].push_back(w.share());
			}
		}

		void Work::doPublish()
		{
			ScopeExit exit([&] { finish(); });

			asset->customData.clear();
			asset->compressedData.clear();
			asset->originalData.clear();

			ScopeLock lock(asset->impl->privateMutex);
			asset->impl->publish(asset.share());
		}

		void Work::doUnload()
		{
			ScopeExit exit([&] { finish(); });

			ProfilingScope profiling("asset unloading");
			profiling.set(asset->textName);

			asset->assetHolder.clear();

			asset->impl->enqueueRemove(asset.share());
		}

		void Work::doRemove()
		{
			ScopeExit exit([&] { finish(); });

			for (uint32 n : asset->dependencies)
				asset->impl->unload(n);
			asset->dependencies.clear();

			ScopeLock lock(asset->impl->privateMutex);
			auto &c = asset->impl->privateIndex[asset->realName];
			std::erase_if(c.versions, [&](const Holder<Asset> &it) { return +it == +asset; });
			if (c.versions.empty())
				asset->impl->privateIndex.erase(asset->realName);
		}

		void Work::finish()
		{
			if (asset)
			{
				if (asset->impl->workingCounter-- == 1)
					asset->impl->keepOpen.clear();
				asset.clear();
			}
		}

		Waiting::Waiting(Holder<Asset> &&asset_) : asset(std::move(asset_))
		{
			asset->impl->workingCounter++;
		}

		Waiting::~Waiting()
		{
			asset->impl->enqueuePublish(asset.share());
			if (asset->impl->workingCounter-- == 1)
				asset->impl->keepOpen.clear();
		}
	}

	void AssetManager::load(uint32 realName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[realName];
		if (c.references++ == 0)
		{
			Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, realName);
			c.versions.insert(c.versions.begin(), asset.share());
			lock.clear();
			impl->fetchQueue.push(std::move(asset));
		}
	}

	void AssetManager::unload(uint32 realName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		CAGE_ASSERT(impl->privateIndex.count(realName) == 1);
		auto &c = impl->privateIndex.at(realName);
		if (c.references-- == 1)
		{
			{
				ScopeLock lock(impl->publicMutex, WriteLockTag());
				impl->unpublish(realName);
			}
			for (auto &a : c.versions)
				a->ref.clear();
		}
	}

	void AssetManager::reload(uint32 realName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		auto it = impl->privateIndex.find(realName);
		if (it == impl->privateIndex.end())
			return;
		auto &c = it->second;
		if (!c.fabricated)
		{
			Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, realName);
			c.versions.insert(c.versions.begin(), asset.share());
			lock.clear();
			impl->fetchQueue.push(std::move(asset));
		}
	}

	uint32 AssetManager::generateUniqueName()
	{
		static constexpr uint32 a = (uint32)1 << 28;
		static constexpr uint32 b = (uint32)1 << 30;
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		uint32 &name = impl->generateName;
		ScopeLock lock(impl->privateMutex);
		if (name < a || name > b)
			name = a;
		while (impl->privateIndex.count(name))
			name = name == b ? a : name + 1;
		return name++;
	}

	void AssetManager::listen(const String &address, uint16 port, uint64 listenerPeriod)
	{
		class AssetListenerImpl
		{
		public:
			AssetManager *const man = nullptr;
			String address;
			uint64 period = 0;
			uint16 port = 0;
			std::atomic<bool> stopping = false;
			Holder<TcpConnection> tcp;
			Holder<Thread> thread;

			AssetListenerImpl(AssetManager *man, const String &address, uint64 period, uint32 port) : man(man), address(address), period(period), port(port) { thread = newThread(Delegate<void()>().bind<AssetListenerImpl, &AssetListenerImpl::thrEntry>(this), "assets listener"); }

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
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed to connect to the database");
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
							CAGE_LOG(SeverityEnum::Info, "assetManager", Stringizer() + "reloading asset: " + line + " (" + name + ")");
							man->reload(name);
						}
					}
				}
				catch (const Disconnected &)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener disconnected");
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed with exception");
					detail::logCurrentCaughtException();
				}
				tcp.clear();
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		impl->listener.clear();
		impl->listener = systemMemory().createHolder<AssetListenerImpl>(this, address, listenerPeriod, port).template cast<void>();
	}

	bool AssetManager::processCustomThread(uint32 threadIndex)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		return impl->customProcessEntry(threadIndex);
	}

	void AssetManager::unloadCustomThread(uint32 threadIndex)
	{
		while (!empty())
		{
			while (processCustomThread(threadIndex))
				;
			threadYield();
		}
	}

	void AssetManager::waitTillEmpty()
	{
		while (!empty())
			threadYield();
	}

	bool AssetManager::processing() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		return impl->workingCounter > 0;
	}

	bool AssetManager::empty() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		ScopeLock lock(impl->privateMutex);
		return impl->workingCounter == 0 && impl->existsCounter == 0;
	}

	void AssetManager::defineScheme_(uint32 typeHash, uint32 scheme, const AssetScheme &value)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		CAGE_ASSERT(typeHash != m);
		CAGE_ASSERT(scheme < impl->schemes.size());
		CAGE_ASSERT(value.typeHash == typeHash);
		CAGE_ASSERT(value.threadIndex == m || value.threadIndex < impl->customProcessingQueues.size());
		CAGE_ASSERT(impl->schemes[scheme].typeHash == m); // the scheme was not defined previously
		impl->schemes[scheme] = value;
	}

	void AssetManager::load_(uint32 scheme, uint32 realName, const String &textName, Holder<void> &&value)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[realName];
		c.fabricated = true;
		c.references++;
		Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, scheme, realName, textName, std::move(value));
		c.versions.insert(c.versions.begin(), asset.share());
		impl->publish(std::move(asset));
	}

	void AssetManager::load_(uint32 scheme, uint32 realName, const String &textName, const AssetScheme &customScheme, Holder<void> &&customData)
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		CAGE_ASSERT(customScheme.threadIndex == m);
		CAGE_ASSERT(customScheme.typeHash == m);
		ScopeLock lock(impl->privateMutex);
		auto &c = impl->privateIndex[realName];
		c.fabricated = true;
		c.references++;
		Holder<Asset> asset = systemMemory().createHolder<Asset>(impl, scheme, realName, textName, customScheme, std::move(customData));
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
			asset->impl->enqueuePublish(asset.share());
		}
	}

	Holder<void> AssetManager::get_(uint32 scheme, uint32 realName) const
	{
		const AssetManagerImpl *impl = (const AssetManagerImpl *)this;
		const auto &schemes = impl->schemes;
		const auto &publicIndex = impl->publicIndex;
		CAGE_ASSERT(scheme < schemes.size());
		CAGE_ASSERT(schemes[scheme].load);
		ScopeLock lock(impl->publicMutex, ReadLockTag());
		auto it = publicIndex.find(realName);
		if (it == publicIndex.end())
			return {}; // not found
		const auto &a = it->second;
		if (a->scheme != scheme)
			return {}; // different scheme
		if (a->failed)
		{
			CAGE_LOG_THROW(Stringizer() + "asset real name: " + realName + ", text name: " + a->textName);
			CAGE_THROW_ERROR(Exception, "accessing asset that failed to load");
		}
		CAGE_ASSERT(a->ref);
		CAGE_ASSERT(+a->ref != (void *)1);
		return a->ref.share();
	}

	uint32 AssetManager::schemeTypeHash_(uint32 scheme) const
	{
		const AssetManagerImpl *impl = (const AssetManagerImpl *)this;
		CAGE_ASSERT(scheme < impl->schemes.size());
		return impl->schemes[scheme].typeHash;
	}

	Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config)
	{
		return systemMemory().createImpl<AssetManager, AssetManagerImpl>(config);
	}

	namespace
	{
		constexpr uint32 CurrentAssetVersion = 1;

		void defaultFetch(AssetContext *asset)
		{
			auto impl = ((Asset *)asset)->impl;

			Holder<File> file;
			{
				String foundName;
				if (impl->findAsset.dispatch(asset->realName, foundName, file))
				{
					if (!file && !foundName.empty())
						file = readFile(foundName);
				}
				else
				{
					CAGE_ASSERT(!file && foundName.empty());
					foundName = pathJoin(impl->path, Stringizer() + asset->realName);
					file = readFile(foundName);
				}
				if (!foundName.empty())
					impl->keepOpen.add(foundName);
			}
			CAGE_ASSERT(file);

			AssetHeader h;
			file->read(bufferView<char>(h));
			if (detail::memcmp(h.cageName, "cageAss", 8) != 0)
				CAGE_THROW_ERROR(Exception, "file is not a cage asset");
			if (h.version != CurrentAssetVersion)
				CAGE_THROW_ERROR(Exception, "cage asset version mismatch");
			if (h.textName[sizeof(h.textName) - 1] != 0)
				CAGE_THROW_ERROR(Exception, "cage asset text name not bounded");
			asset->textName = h.textName;
			if (h.scheme >= impl->schemes.size())
				CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
			asset->scheme = h.scheme;

			*((AssetScheme *)(Asset *)asset) = impl->schemes[asset->scheme];
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
		}

		void defaultDecompress(AssetContext *asset)
		{
			if (asset->compressedData.size() == 0)
				return;
			decompress(*asset->compressedData, *asset->originalData);
		}
	}

	AssetScheme::AssetScheme()
	{
		fetch.bind<defaultFetch>();
		decompress.bind<defaultDecompress>();
	}

	AssetHeader::AssetHeader(const String &name_, uint16 schemeIndex)
	{
		version = CurrentAssetVersion;
		String name = name_;
		static constexpr uint32 MaxTexName = sizeof(textName) - 1;
		if (name.length() > MaxTexName)
			name = String() + ".." + subString(name, name.length() - MaxTexName + 2, m);
		CAGE_ASSERT(name.length() <= MaxTexName);
		CAGE_ASSERT(name.length() > 0);
		detail::memcpy(textName, name.c_str(), name.length());
		scheme = schemeIndex;
	}
}
