#include <cage-core/math.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/config.h>
#include <cage-core/hashString.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/assetManager.h>
#include <cage-core/unordered_map.h>
#include <cage-core/debug.h>

#include <optick.h>

#include <unordered_map>
#include <vector>
#include <atomic>
#include <algorithm>

namespace cage
{
	ConfigUint32 logLevel("cage/assets/logLevel", 1);
#define ASS_LOG(LEVEL, ASS, MSG) \
	{ \
		if (logLevel >= (LEVEL)) \
		{ \
			if (logLevel > 1) \
			{ \
				CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "asset '" + (ASS)->textName + "' (" + (ASS)->realName + " / " + (ASS)->aliasName + ") [" + (ASS)->guid + "]: " + MSG); \
			} \
			else \
			{ \
				CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "asset '" + (ASS)->textName + "': " + MSG); \
			} \
		} \
	}

	namespace
	{
		constexpr uint32 CurrentAssetVersion = 1;

		struct Scheme : public AssetScheme
		{
			uintPtr typeId = 0;
		};

		enum class StateEnum
		{
			Initialized,
			DiskLoading,
			Decompressing,
			Processing,
			WaitingForDeps,
			Ready,
			Error,
			Unloading,
			Erasing,
		};

		// one particular version of an asset
		struct Asset : public AssetContext
		{
			MemoryBuffer origData, compData;
			std::vector<uint32> dependencies;
			Holder<void> reference; // the deleter is set to a function that will enqueue this asset version for unloading
			std::atomic<StateEnum> state{ StateEnum::Initialized };
			uint32 scheme = m;
			const uint32 guid;

			Asset(uint32 realName, uint32 guid) : AssetContext(realName), guid(guid)
			{
				ASS_LOG(3, this, "created");
			}

			~Asset()
			{
				ASS_LOG(3, this, "destroyed");
			}

			void clearMemory()
			{
				origData.free();
				compData.free();
			}
		};

		// container for all versions of a single asset name
		struct Versions
		{
			std::vector<Holder<Asset>> versions;
			sint32 refCnt = 0;
			bool fabricated = false;
		};

		// reference of one version of an asset for use in the public indices
		struct Reference
		{
			Holder<void> ref;
			uint32 scheme = m; // m -> error

			Reference() = default;

			explicit Reference(const Holder<Asset> &ass) : scheme(ass->state == StateEnum::Error ? m : ass->scheme)
			{
				if (ass->state == StateEnum::Ready)
					ref = ass->reference.share();
			}
		};

		enum class CommandEnum
		{
			None,
			Add,
			Fabricate,
			Remove,
			Reload,
			WaitDeps,
			Erasing,
		};

		struct Command
		{
			detail::StringBase<64> textName;
			Holder<void> holder;
			uint32 scheme = m;
			uint32 realName = 0;
			CommandEnum type = CommandEnum::None;
		};

		template<class T>
		struct CounterValueWrapper
		{
			CounterValueWrapper() = default;
			explicit CounterValueWrapper(T &v, std::atomic<sint32> *c) : counter(c)
			{
				CAGE_ASSERT(c);
				std::swap(v, value);
				(*counter)++;
			}
			CounterValueWrapper(CounterValueWrapper &&other)
			{
				std::swap(value, other.value);
				std::swap(counter, other.counter);
			}
			CounterValueWrapper &operator = (CounterValueWrapper &&other)
			{
				std::swap(value, other.value);
				std::swap(counter, other.counter);
				return *this;
			}
			~CounterValueWrapper()
			{
				if (counter)
					(*counter)--;
			}
			CounterValueWrapper(const CounterValueWrapper &) = delete;
			CounterValueWrapper &operator = (const CounterValueWrapper &) = delete;

			T value = T();
		private:
			std::atomic<sint32> *counter = nullptr;
		};

		typedef std::unordered_map<uint32, Versions> PrivateIndex;
		typedef std::unordered_map<uint32, Reference> PublicIndex;
		typedef ConcurrentQueue<CounterValueWrapper<Asset *>> Queue;

		struct PublicIndexData
		{
			PublicIndex index;
			mutable Holder<RwMutex> mut = newRwMutex();
		};

		class AssetManagerImpl : public AssetManager
		{
		public:
			Holder<TcpConnection> listener;
			string listenerAddress;
			uint16 listenerPort = 0;

			std::vector<Scheme> schemes;
			Holder<Mutex> mutex;
			PrivateIndex privateIndex; // used for owning and managing the assets
			PublicIndexData publicIndices[2]; // used for accessing the assets from the api
			std::atomic<uint32> publicIndex{0};
			const string path;
			const uint64 maintenancePeriod;
			const uint64 listenerPeriod;
			uint32 generateName = 0;
			uint32 assetGuid = 1;
			std::atomic<sint32> working{0};
			std::atomic<bool> stopping{false}, unloaded{false};

			ConcurrentQueue<CounterValueWrapper<Command>> maintenanceQueue;
			Queue diskLoadingQueue;
			Queue decompressionQueue;
			Queue defaultProcessingQueue;
			std::vector<Holder<Queue>> customProcessingQueues;
			std::vector<CounterValueWrapper<Asset *>> waitingDeps;

			// threads has to be declared at the very end to ensure that they are first to destroy
			Holder<Thread> listenerThread;
			Holder<Thread> maintenanceThread;
			Holder<Thread> diskLoadingThread;
			Holder<Thread> decompressionThread;
			Holder<Thread> defaultProcessingThread;

			static string findAssetsFolderPath(const AssetManagerCreateConfig &config)
			{
				try
				{
					if (pathIsAbs(config.assetsFolderName))
					{
						if (any(pathType(config.assetsFolderName) & (PathTypeFlags::Directory | PathTypeFlags::Archive)))
							return config.assetsFolderName;
					}
					try
					{
						detail::OverrideException oe;
						return pathSearchTowardsRoot(config.assetsFolderName, pathWorkingDir(), PathTypeFlags::Directory | PathTypeFlags::Archive);
					}
					catch (const Exception &)
					{
						return pathSearchTowardsRoot(config.assetsFolderName, pathExtractPath(detail::getExecutableFullPath()), PathTypeFlags::Directory | PathTypeFlags::Archive);
					}
				}
				catch (const Exception &)
				{
					CAGE_LOG(SeverityEnum::Error, "assetManager", "failed to find the directory with assets");
					throw;
				}
			}

			AssetManagerImpl(const AssetManagerCreateConfig &config) : path(findAssetsFolderPath(config)), maintenancePeriod(config.maintenancePeriod), listenerPeriod(config.listenerPeriod)
			{
				CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "using asset path: '" + path + "'");
				mutex = newMutex();
				customProcessingQueues.resize(config.threadsMaxCount);
				for (auto &it : customProcessingQueues)
					it = detail::systemArena().createHolder<ConcurrentQueue<CounterValueWrapper<Asset *>>>();
				schemes.resize(config.schemesMaxCount);
				maintenanceThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::maintenanceEntry>(this), "asset maintenance");
				diskLoadingThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::diskLoadingEntry>(this), "asset disk loading");
				decompressionThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::decompressionEntry>(this), "asset decompression");
				defaultProcessingThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::defaultProcessingEntry>(this), "asset processing");
			}

			~AssetManagerImpl()
			{
				stopping = true;
				maintenanceQueue.terminate();
				diskLoadingQueue.terminate();
				decompressionQueue.terminate();
				defaultProcessingQueue.terminate();
				for (auto &it : customProcessingQueues)
					it->terminate();
			}

			// asset methods

			Holder<File> openAsset(uint32 name)
			{
				Holder<File> file;
				if (!findAsset.dispatch(name, file))
					file = readFile(pathJoin(path, stringizer() + name));
				return file;
			}

			void diskLoadAsset(Asset *ass)
			{
				OPTICK_EVENT("disk load");
				OPTICK_TAG("realName", ass->realName);
				ASS_LOG(2, ass, "disk load");

				CAGE_ASSERT(ass->state == StateEnum::DiskLoading);
				CAGE_ASSERT(!ass->assetHolder);
				CAGE_ASSERT(!ass->reference);
				CAGE_ASSERT(ass->scheme == m);
				CAGE_ASSERT(ass->assetFlags == 0);
				CAGE_ASSERT(ass->aliasName == 0);
				CAGE_ASSERT(working > 0);

				try
				{
					detail::OverrideBreakpoint OverrideBreakpoint;
					Holder<File> file = openAsset(ass->realName);

					AssetHeader h;
					file->read(bufferView<char>(h));
					if (detail::memcmp(h.cageName, "cageAss", 8) != 0)
						CAGE_THROW_ERROR(Exception, "file is not a cage asset");
					if (h.version != CurrentAssetVersion)
						CAGE_THROW_ERROR(Exception, "cage asset version mismatch");
					if (h.textName[sizeof(h.textName) - 1] != 0)
						CAGE_THROW_ERROR(Exception, "cage asset text name not bounded");
					ass->textName = h.textName;
					OPTICK_TAG("textName", ass->textName.c_str());
					if (h.scheme >= schemes.size())
						CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
					ass->scheme = h.scheme;
					ass->assetFlags = h.flags;
					ass->aliasName = h.aliasName;

					if (!schemes[ass->scheme].load)
					{
						ASS_LOG(1, ass, "no loading procedure");
						ass->assetHolder = Holder<void>((void *)1, nullptr, {});
						ass->state = StateEnum::WaitingForDeps;
						createReference(ass);
						return;
					}

					ass->dependencies.resize(h.dependenciesCount);
					file->read(bufferCast<char, uint32>(ass->dependencies));

					if (h.compressedSize)
						ass->compData.allocate(h.compressedSize);
					if (h.originalSize)
						ass->origData.allocate(h.originalSize);
					if (h.compressedSize || h.originalSize)
					{
						MemoryBuffer &t = h.compressedSize ? ass->compData : ass->origData;
						file->read(t);
					}

					CAGE_ASSERT(file->tell() == file->size());
				}
				catch (const Exception &)
				{
					ASS_LOG(1, ass, "disk load failed");
					ass->clearMemory();
					ass->dependencies.clear();
					ass->state = StateEnum::Error;
				}

				if (ass->state != StateEnum::Error)
				{
					for (uint32 n : ass->dependencies)
						add(n);
					if (ass->compData.size())
					{
						ass->state = StateEnum::Decompressing;
						decompressionQueue.push(CounterValueWrapper<Asset *>(ass, &working));
					}
					else
					{
						ass->state = StateEnum::Processing;
						enqueueToSchemeQueue(ass);
					}
				}
			}

			void decompressAsset(Asset *ass)
			{
				OPTICK_EVENT("decompress");
				OPTICK_TAG("realName", ass->realName);
				OPTICK_TAG("textName", ass->textName.c_str());
				ASS_LOG(2, ass, "decompress");

				CAGE_ASSERT(ass->state == StateEnum::Decompressing);
				CAGE_ASSERT(!ass->assetHolder);
				CAGE_ASSERT(!ass->reference);
				CAGE_ASSERT(ass->scheme < schemes.size());
				CAGE_ASSERT(working > 0);

				try
				{
					schemes[ass->scheme].decompress(ass);
				}
				catch (const Exception &)
				{
					ASS_LOG(1, ass, "decompression failed");
					ass->clearMemory();
					ass->state = StateEnum::Error;
				}

				if (ass->state != StateEnum::Error)
				{
					ass->state = StateEnum::Processing;
					enqueueToSchemeQueue(ass);
				}
			}

			void processAsset(Asset *ass)
			{
				if (ass->state == StateEnum::Unloading)
				{
					unloadAsset(ass);
					return;
				}

				OPTICK_EVENT("processing");
				OPTICK_TAG("realName", ass->realName);
				OPTICK_TAG("textName", ass->textName.c_str());
				ASS_LOG(2, ass, "processing");

				CAGE_ASSERT(ass->state == StateEnum::Processing);
				CAGE_ASSERT(ass->scheme < schemes.size());
				CAGE_ASSERT(working > 0);

				try
				{
					schemes[ass->scheme].load(ass);
					CAGE_ASSERT(ass->assetHolder);
				}
				catch (const Exception &)
				{
					ASS_LOG(1, ass, "processing failed");
					ass->assetHolder.clear();
					ass->clearMemory();
					ass->state = StateEnum::Error;
				}

				ass->clearMemory();

				if (ass->state != StateEnum::Error)
				{
					ass->state = StateEnum::WaitingForDeps;
					if (!checkDependencies(ass))
					{
						Command cmd;
						cmd.realName = ass->realName;
						cmd.type = CommandEnum::WaitDeps;
						cmd.holder = Holder<void>(ass, nullptr, {});
						maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
					}
				}
			}

			void createReference(Asset *ass)
			{
				CAGE_ASSERT(ass->state == StateEnum::WaitingForDeps);
				CAGE_ASSERT(ass->assetHolder);
				CAGE_ASSERT(!ass->reference);

				Holder<void> r(ass->assetHolder.get(), ass, Delegate<void(void*)>().bind<AssetManagerImpl, &AssetManagerImpl::enqueueAssetToUnload>(this));
				r = templates::move(r).makeShareable();
				ASS_LOG(2, ass, "ready");
				ass->reference = templates::move(r);
				ass->state = StateEnum::Ready;
			}

			void enqueueToSchemeQueue(Asset *ass)
			{
				ASS_LOG(2, ass, "enqueue to scheme specific queue");
				CAGE_ASSERT(ass->scheme < schemes.size());
				uint32 t = schemes[ass->scheme].threadIndex;
				CAGE_ASSERT(t == m || t < customProcessingQueues.size());
				if (t == m)
					defaultProcessingQueue.push(CounterValueWrapper<Asset *>(ass, &working));
				else
					customProcessingQueues[t]->push(CounterValueWrapper<Asset *>(ass, &working));
			}

			void enqueueAssetToUnload(void *ass_)
			{
				Asset *ass = (Asset *)ass_;
				ASS_LOG(3, ass, "enqueue to unload");
				ass->state = StateEnum::Unloading;
				enqueueToSchemeQueue(ass);
			}

			void unloadAsset(Asset *ass)
			{
				OPTICK_EVENT("unloading");
				OPTICK_TAG("realName", ass->realName);
				OPTICK_TAG("textName", ass->textName.c_str());
				ASS_LOG(2, ass, "unloading");

				CAGE_ASSERT(ass->state == StateEnum::Unloading);
				CAGE_ASSERT(ass->assetHolder);
				CAGE_ASSERT(working > 0);

				ass->assetHolder.clear();

				for (uint32 n : ass->dependencies)
					remove(n);
				ass->dependencies.clear();

				ass->state = StateEnum::Erasing;

				Command cmd;
				cmd.realName = ass->realName;
				cmd.type = CommandEnum::Erasing;
				cmd.holder = Holder<void>(ass, nullptr, {});
				maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
			}

			Holder<Asset> newAsset(uint32 realName)
			{
				return detail::systemArena().createHolder<Asset>(realName, assetGuid++);
			}

			bool checkDependencies(Asset *ass)
			{
				ASS_LOG(3, ass, "check dependencies");
				PublicIndexData &index = publicIndices[publicIndex];
				ScopeLock<RwMutex> lock(index.mut, ReadLock());
				for (uint32 n : ass->dependencies)
				{
					auto it = index.index.find(n);
					if (it == index.index.end())
						return false;
					if (it->second.scheme == m)
					{
						ASS_LOG(2, ass, "dependencies failed");
						ass->state = StateEnum::Error;
						return true;
					}
				}
				createReference(ass);
				return true;
			}

			void maintenanceCommand(Command &&cmd)
			{
				OPTICK_EVENT("command");
				OPTICK_TAG("realName", cmd.realName);
				OPTICK_TAG("type", (uint32)cmd.type);
				switch (cmd.type)
				{
				case CommandEnum::Add:
				{
					auto &a = privateIndex[cmd.realName];
					if (++a.refCnt == 1)
					{
						cmd.type = CommandEnum::Reload;
						maintenanceCommand(templates::move(cmd));
					}
				} break;
				case CommandEnum::Fabricate:
				{
					auto &a = privateIndex[cmd.realName];
					++a.refCnt;
					a.fabricated = true;
					Holder<Asset> ass = newAsset(cmd.realName);
					ass->textName = cmd.textName;
					ass->scheme = cmd.scheme;
					ass->assetHolder = templates::move(cmd.holder);
					ass->state = StateEnum::WaitingForDeps;
					createReference(ass.get());
					a.versions.insert(a.versions.begin(), templates::move(ass));
				} break;
				case CommandEnum::Remove:
				{
					auto &a = privateIndex[cmd.realName];
					--a.refCnt;
					CAGE_ASSERT(a.refCnt >= 0);
				} break;
				case CommandEnum::Reload:
				{
					auto &a = privateIndex[cmd.realName];
					if (a.fabricated)
					{
						CAGE_LOG(SeverityEnum::Warning, "assetManager", stringizer() + "cannot reload asset " + cmd.realName + ", it is fabricated");
						break; // if an existing asset was replaced with a fabricated one, it may not be reloaded with an asset from disk
					}
					Holder<Asset> assh = newAsset(cmd.realName);
					Asset *assp = assh.get();
					a.versions.insert(a.versions.begin(), templates::move(assh));
					assp->state = StateEnum::DiskLoading;
					diskLoadingQueue.push(CounterValueWrapper<Asset *>(assp, &working));
				} break;
				case CommandEnum::WaitDeps:
				{
					Asset *ass = (Asset*)cmd.holder.get();
					CAGE_ASSERT(ass);
					CAGE_ASSERT(ass->state == StateEnum::WaitingForDeps);
					waitingDeps.push_back(CounterValueWrapper<Asset *>(ass, &working));
				} break;
				case CommandEnum::Erasing:
				{
					Asset *ass = (Asset*)cmd.holder.get();
					CAGE_ASSERT(ass);
					CAGE_ASSERT(ass->state == StateEnum::Erasing);
					auto &a = privateIndex[cmd.realName];
					a.versions.erase(std::remove_if(a.versions.begin(), a.versions.end(), [&](const auto &it) {
						if (it.get() == ass)
						{
							CAGE_ASSERT(!it->reference);
							return true;
						}
						return false;
						}), a.versions.end());
				} break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid command type enum");
				}
			}

			void maintenanceUpdatePublicIndex()
			{
				uint32 nextIndex = (publicIndex + 1) % 2;
				PrivateIndex &index = privateIndex;
				PublicIndexData &updating = publicIndices[nextIndex];
				ScopeLock<RwMutex> lock(updating.mut, WriteLock());
				PublicIndex &update = updating.index;
				update.clear();
				update.reserve(index.size() * 2);
				auto it = index.begin();
				while (it != index.end())
				{
					auto &a = it->second;
					CAGE_ASSERT(a.refCnt >= 0);
					if (a.refCnt == 0)
					{
						a.versions.erase(std::remove_if(a.versions.begin(), a.versions.end(), [](const auto &it) {
							it->reference.clear();
							return it->state == StateEnum::Error;
							}), a.versions.end());
						if (a.versions.empty())
						{
							it = index.erase(it);
							continue;
						}
					}
					else
					{
						bool found = false;
						for (const auto &v : a.versions)
						{
							if (found)
								v->reference.clear();
							else if (v->state == StateEnum::Error || v->state == StateEnum::Ready)
							{
								CAGE_ASSERT(update.find(it->first) == update.end());
								update[it->first] = Reference(v);
								if (v->aliasName)
								{
									CAGE_ASSERT(update.find(v->aliasName) == update.end());
									update[v->aliasName] = Reference(v);
								}
								found = true;
							}
						}
					}
					++it;
				}
				publicIndex = nextIndex;
			}

			void maintenanceConclusion()
			{
				OPTICK_EVENT("conclusion");
				OPTICK_TAG("assetsCount", privateIndex.size());

				while (true)
				{
					// cycle to resolve whole chains of dependencies in case they are listed in reverse order
					const auto start = waitingDeps.size();
					waitingDeps.erase(std::remove_if(waitingDeps.begin(), waitingDeps.end(), [&](CounterValueWrapper<Asset *> &ass) {
						return checkDependencies(ass.value);
						}), waitingDeps.end());
					if (waitingDeps.size() == start)
						break;
				}

				maintenanceUpdatePublicIndex();
				unloaded = privateIndex.empty() && working == 0;
			}

			// thread entry points

			void listenerEntry()
			{
				try
				{
					listener = newTcpConnection(listenerAddress, listenerPort);
					while (!stopping)
					{
						threadSleep(listenerPeriod);
						OPTICK_EVENT("assets network notifications");
						string line;
						while (listener->readLine(line))
						{
							uint32 name = HashString(line.c_str());
							CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "assets network notifications: reloading asset '" + line + "' (" + name + ")");
							reload(name);
						}
					}
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed");
					listener.clear();
				}
			}

			void diskLoadingEntry()
			{
				auto &que = diskLoadingQueue;
				try
				{
					while (true)
					{
						CounterValueWrapper<Asset *> ass;
						que.pop(ass);
						diskLoadAsset(ass.value);
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void decompressionEntry()
			{
				auto &que = decompressionQueue;
				try
				{
					while (true)
					{
						CounterValueWrapper<Asset *> ass;
						que.pop(ass);
						decompressAsset(ass.value);
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void defaultProcessingEntry()
			{
				auto &que = defaultProcessingQueue;
				try
				{
					while (true)
					{
						CounterValueWrapper<Asset *> ass;
						que.pop(ass);
						processAsset(ass.value);
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void maintenanceEntry()
			{
				auto &que = maintenanceQueue;
				try
				{
					while (true)
					{
						{
							ScopeLock<Mutex> lock(mutex);
							OPTICK_EVENT("maintenance");
							{
								CounterValueWrapper<Command> cmd;
								while (que.tryPop(cmd))
									maintenanceCommand(templates::move(cmd.value));
							}
							maintenanceConclusion();
						}
						{
							OPTICK_EVENT("sleep");
							threadSleep(maintenancePeriod); // give other threads enough time to stop using the previous publicIndex
						}
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			// api methods

			void defineScheme(uint32 scheme, uintPtr typeId, const AssetScheme &value)
			{
				CAGE_ASSERT(scheme < schemes.size());
				CAGE_ASSERT(schemes[scheme].typeId == 0);
				CAGE_ASSERT(value.threadIndex == m || value.threadIndex < customProcessingQueues.size());
				(AssetScheme&)schemes[scheme] = value;
				schemes[scheme].typeId = typeId;
			}

			void add(uint32 assetName)
			{
				Command cmd;
				cmd.realName = assetName;
				cmd.type = CommandEnum::Add;
				maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
				unloaded = false;
			}

			void remove(uint32 assetName)
			{
				Command cmd;
				cmd.realName = assetName;
				cmd.type = CommandEnum::Remove;
				maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
			}

			void reload(uint32 assetName)
			{
				Command cmd;
				cmd.realName = assetName;
				cmd.type = CommandEnum::Reload;
				maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
			}

			uint32 generateUniqueName()
			{
				static constexpr uint32 a = (uint32)1 << 28;
				static constexpr uint32 b = (uint32)1 << 30;
				ScopeLock<Mutex> lock(mutex);
				if (generateName < a || generateName > b)
					generateName = a;
				while (privateIndex.find(generateName) != privateIndex.end())
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}

			void fabricate(uint32 assetName, const string &textName, uint32 scheme, uintPtr typeId, Holder<void> &&value)
			{
				CAGE_ASSERT(typeId != 0);
				CAGE_ASSERT(schemes[scheme].typeId == typeId);
				Command cmd;
				cmd.realName = assetName;
				cmd.textName = textName;
				cmd.scheme = scheme;
				cmd.holder = templates::move(value);
				cmd.type = CommandEnum::Fabricate;
				maintenanceQueue.push(CounterValueWrapper<Command>(cmd, &working));
				unloaded = false;
			}

			Holder<void> get(uint32 assetName, uint32 scheme, uintPtr typeId, bool throwOnInvalidScheme) const
			{
				CAGE_ASSERT(typeId != 0);
				const PublicIndexData &data = publicIndices[publicIndex];
				ScopeLock<RwMutex> lock(data.mut, ReadLock());
				const PublicIndex &index = data.index;
				auto it = index.find(assetName);
				if (it == index.end())
					return {}; // not found
				const auto &a = it->second;
				if (a.scheme == m)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset real name: " + assetName);
					CAGE_THROW_ERROR(Exception, "asset failed to load");
				}
				if (!a.ref)
					return {}; // not yet ready
				if (a.scheme != scheme)
				{
					if (throwOnInvalidScheme)
					{
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset real name: " + assetName);
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset loaded scheme: " + a.scheme + ", accessing with: " + scheme);
						CAGE_THROW_ERROR(Exception, "accessing asset with different scheme");
					}
					return {}; // invalid scheme
				}
				CAGE_ASSERT(schemes[a.scheme].typeId == typeId);
				return a.ref.share();
			}

			bool processCustomThread(uint32 threadIndex)
			{
				CAGE_ASSERT(threadIndex < customProcessingQueues.size());
				auto &que = customProcessingQueues[threadIndex];
				CounterValueWrapper<Asset *> ass;
				if (que->tryPop(ass))
				{
					processAsset(ass.value);
					return true;
				}
				return false;
			}

			void unloadCustomThread(uint32 threadIndex)
			{
				CAGE_ASSERT(threadIndex < customProcessingQueues.size());
				while (!unloaded)
				{
					while (processCustomThread(threadIndex));
					threadSleep(2000);
				}
			}

			void unloadWait()
			{
				while (!unloaded)
					threadSleep(2000);
			}

			void listen(const string &address, uint16 port)
			{
				CAGE_ASSERT(!listenerThread);
				listenerAddress = address;
				listenerPort = port;
				listenerThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::listenerEntry>(this), "assets listener");
			}
		};
	}

	void AssetManager::defineScheme_(uint32 scheme, uintPtr typeId, const AssetScheme &value)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->defineScheme(scheme, typeId, value);
	}

	void AssetManager::add(uint32 assetName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->add(assetName);
	}

	void AssetManager::remove(uint32 assetName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->remove(assetName);
	}

	void AssetManager::reload(uint32 assetName)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->reload(assetName);
	}

	uint32 AssetManager::generateUniqueName()
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		return impl->generateUniqueName();
	}

	void AssetManager::fabricate_(uint32 assetName, const string &textName, uint32 scheme, uintPtr typeId, Holder<void> &&value)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->fabricate(assetName, textName, scheme, typeId, templates::move(value));
	}

	Holder<void> AssetManager::get_(uint32 assetName, uint32 scheme, uintPtr typeId, bool throwOnInvalidScheme) const
	{
		const AssetManagerImpl *impl = (const AssetManagerImpl*)this;
		return impl->get(assetName, scheme, typeId, throwOnInvalidScheme);
	}

	bool AssetManager::processCustomThread(uint32 threadIndex)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		return impl->processCustomThread(threadIndex);
	}

	bool AssetManager::processing() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		CAGE_ASSERT(impl->working >= 0);
		return impl->working == 0;
	}

	void AssetManager::unloadCustomThread(uint32 threadIndex)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->unloadCustomThread(threadIndex);
	}

	void AssetManager::unloadWait()
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->unloadWait();
	}

	bool AssetManager::unloaded() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		return impl->unloaded;
	}

	void AssetManager::listen(const string &address, uint16 port)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->listen(address, port);
	}

	Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<AssetManager, AssetManagerImpl>(config);
	}

	AssetHeader initializeAssetHeader(const string &name_, uint16 schemeIndex)
	{
		AssetHeader a;
		detail::memset(&a, 0, sizeof(AssetHeader));
		detail::memcpy(a.cageName, "cageAss", 7);
		a.version = CurrentAssetVersion;
		string name = name_;
		constexpr uint32 maxTexName = sizeof(a.textName);
		if (name.length() >= maxTexName)
			name = string() + ".." + name.subString(name.length() - maxTexName - 3, maxTexName - 3);
		CAGE_ASSERT(name.length() < sizeof(a.textName));
		detail::memcpy(a.textName, name.c_str(), name.length());
		a.scheme = schemeIndex;
		return a;
	}

	MemoryBuffer &AssetContext::compressedData() const
	{
		Asset *impl = (Asset *)this;
		return impl->compData;
	}

	MemoryBuffer &AssetContext::originalData() const
	{
		Asset *impl = (Asset *)this;
		return impl->origData;
	}
}
