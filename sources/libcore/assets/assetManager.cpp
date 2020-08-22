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
		class AssetManagerImpl;

		constexpr uint32 CurrentAssetVersion = 1;

		struct Scheme : public AssetScheme
		{
			uintPtr typeId = 0;
		};

		// one particular version of an asset
		struct Asset
		{
			detail::StringBase<64> textName;
			std::vector<uint32> dependencies;
			Holder<void> assetHolder;
			Holder<void> ref; // the application receives shares of the ref, its destructor will call a method in the AssetManager to schedule the asset for unloading
			AssetManagerImpl *const impl = nullptr;
			const uint32 guid = 0;
			const uint32 realName = 0;
			uint32 aliasName = 0;
			uint32 scheme = m;
			bool failed = false;

			explicit Asset(uint32 realName_, AssetManagerImpl *impl);
			~Asset();
		};

		// container for all versions of a single asset name
		struct Collection
		{
			std::vector<Holder<Asset>> versions; // todo replace with a small vector
			sint32 references = 0; // how many times was the asset added in the api
			bool fabricated = false; // once a fabricated asset is added to the collection, all future requests for reload are ignored
		};

		struct WorkingAsset : private Immovable
		{
			Holder<Asset> asset;
			std::atomic<sint32> *const workingCounter = nullptr;

			explicit WorkingAsset(std::atomic<sint32> *workingCounter) : workingCounter(workingCounter)
			{
				(*workingCounter)++;
			}

			virtual ~WorkingAsset()
			{
				(*workingCounter)--;
			}

			virtual void process(AssetManagerImpl *impl) = 0;
			virtual void maintain(AssetManagerImpl *impl) = 0;
		};

		struct Loading : public WorkingAsset
		{
			MemoryBuffer origData, compData;

			explicit Loading(std::atomic<sint32> *workingCounter) : WorkingAsset(workingCounter) {}
			void diskLoad(AssetManagerImpl *impl);
			void decompress(AssetManagerImpl *impl);
			void process(AssetManagerImpl *impl) override;
			void maintain(AssetManagerImpl *impl) override;
		};

		struct Waiting : public WorkingAsset
		{
			explicit Waiting(std::atomic<sint32> *workingCounter) : WorkingAsset(workingCounter) {}
			~Waiting();
			void process(AssetManagerImpl *impl) override;
			void maintain(AssetManagerImpl *impl) override;
		};

		struct Unloading : public WorkingAsset
		{
			void *ptr = nullptr;

			explicit Unloading(std::atomic<sint32> *workingCounter) : WorkingAsset(workingCounter) {}
			void process(AssetManagerImpl *impl) override;
			void maintain(AssetManagerImpl *impl) override;
		};

		enum class CommandEnum
		{
			None,
			Add,
			Remove,
			Reload,
			Fabricate,
		};

		struct Command : public WorkingAsset
		{
			uint32 realName = 0;
			CommandEnum type = CommandEnum::None;

			explicit Command(std::atomic<sint32> *workingCounter) : WorkingAsset(workingCounter) {}
			void process(AssetManagerImpl *impl) override;
			void maintain(AssetManagerImpl *impl) override;
		};

		struct FabricateCommand : public Command
		{
			detail::StringBase<64> textName;
			Holder<void> holder;
			uint32 scheme = m;

			explicit FabricateCommand(std::atomic<sint32> *workingCounter) : Command(workingCounter) {}
			void maintain(AssetManagerImpl *impl) override;
		};

		class AssetManagerImpl : public AssetManager
		{
		public:
			typedef ConcurrentQueue<Holder<WorkingAsset>> Queue;

			std::vector<Scheme> schemes;
			const string path;
			const uint64 listenerPeriod;
			uint32 generateName = 0;
			uint32 assetGuid = 1;
			std::atomic<sint32> workingCounter{0};
			std::atomic<sint32> existsCounter{0};
			std::atomic<bool> stopping{false};

			Holder<RwMutex> publicMutex; // protects publicIndex
			Holder<Mutex> privateMutex; // protects privateIndex, waitingIndex, generateName, assetGuid
			std::unordered_map<uint32, Holder<Asset>> publicIndex;
			std::unordered_map<uint32, Collection> privateIndex;
			std::unordered_map<uint32, std::vector<Holder<Waiting>>> waitingIndex;

			Queue maintenanceQueue;
			Queue diskLoadingQueue;
			Queue decompressionQueue;
			Queue defaultProcessingQueue;
			std::vector<Holder<Queue>> customProcessingQueues;

			Holder<TcpConnection> listener;
			string listenerAddress;
			uint16 listenerPort = 0;

			// threads have to be declared at the very end to ensure that they are first to destroy
			Holder<Thread> maintenanceThread;
			Holder<Thread> diskLoadingThread;
			Holder<Thread> decompressionThread;
			Holder<Thread> defaultProcessingThread;
			Holder<Thread> listenerThread;

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

			AssetManagerImpl(const AssetManagerCreateConfig &config) : path(findAssetsFolderPath(config)), listenerPeriod(config.listenerPeriod)
			{
				CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "using asset path: '" + path + "'");
				privateMutex = newMutex();
				publicMutex = newRwMutex();
				customProcessingQueues.resize(config.threadsMaxCount);
				for (auto &it : customProcessingQueues)
					it = detail::systemArena().createHolder<Queue>();
				schemes.resize(config.schemesMaxCount);
				maintenanceThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::maintenanceEntry>(this), "asset maintenance");
				diskLoadingThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::diskLoadingEntry>(this), "asset disk loading");
				decompressionThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::decompressionEntry>(this), "asset decompression");
				defaultProcessingThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::defaultProcessingEntry>(this), "asset processing");
			}

			~AssetManagerImpl()
			{
				stopping = true;
				diskLoadingQueue.terminate();
				decompressionQueue.terminate();
				defaultProcessingQueue.terminate();
				for (auto &it : customProcessingQueues)
					it->terminate();
				maintenanceQueue.terminate();
				CAGE_ASSERT(privateIndex.empty());
			}

			// internal

			void enqueueForProcessing(Holder<WorkingAsset> &&ass)
			{
				CAGE_ASSERT(ass->asset);
				if (ass->asset->failed)
				{
					maintenanceQueue.push(templates::move(ass));
					return;
				}
				const uint32 scheme = ass->asset->scheme;
				CAGE_ASSERT(scheme < schemes.size());
				if (!schemes[scheme].load)
				{
					maintenanceQueue.push(templates::move(ass));
					return;
				}
				const uint32 thr = schemes[scheme].threadIndex;
				if (thr == m)
					defaultProcessingQueue.push(templates::move(ass));
				else
					customProcessingQueues[thr]->push(templates::move(ass));
			}

			void enqueueForDeprocessing(Holder<WorkingAsset> &&ass)
			{
				CAGE_ASSERT(ass->asset);
				if (ass->asset->failed)
				{
					defaultProcessingQueue.push(templates::move(ass));
					return;
				}
				const uint32 scheme = ass->asset->scheme;
				CAGE_ASSERT(scheme < schemes.size());
				const uint32 thr = schemes[scheme].threadIndex;
				if (thr == m)
					defaultProcessingQueue.push(templates::move(ass));
				else
					customProcessingQueues[thr]->push(templates::move(ass));
			}

			void dereference(void *ptr)
			{
				Holder<Unloading> u = detail::systemArena().createHolder<Unloading>(&workingCounter);
				u->ptr = ptr;
				maintenanceQueue.push(templates::move(u).cast<WorkingAsset>());
			}

			void unpublish(uint32 realName)
			{
				auto it = publicIndex.find(realName);
				if (it != publicIndex.end())
				{
					const uint32 alias = it->second->aliasName;
					publicIndex.erase(it);
					if (alias)
					{
						CAGE_ASSERT(publicIndex.count(alias) == 1);
						publicIndex.erase(alias);
					}
				}
			}

			void updatePublicIndex(Holder<Asset> &ass)
			{
				auto &c = privateIndex[ass->realName];

				if (c.references == 0)
				{
					Holder<Unloading> u = detail::systemArena().createHolder<Unloading>(&workingCounter);
					u->asset = ass.share();
					enqueueForDeprocessing(templates::move(u).cast<WorkingAsset>());
				}
				else
				{
					ass->ref = Holder<void>(ass->assetHolder ? ass->assetHolder.get() : (void*)1, ass.get(), Delegate<void(void *)>().bind<AssetManagerImpl, &AssetManagerImpl::dereference>(this)).makeShareable();
					{
						ScopeLock<RwMutex> lock(publicMutex, WriteLock());
						bool found = false;
						for (const auto &v : c.versions)
						{
							if (found)
								v->ref.clear();
							else if (v->ref || v->failed)
							{
								unpublish(ass->realName);
								publicIndex[ass->realName] = v.share();
								if (ass->aliasName)
								{
									CAGE_ASSERT(publicIndex.count(ass->aliasName) == 0);
									publicIndex[ass->aliasName] = v.share();
								}
								found = true;
							}
						}
						CAGE_ASSERT(found);
					}
				}

				waitingIndex.erase(ass->realName);
			}

			// thread entry points

			void maintenanceEntry()
			{
				try
				{
					while (true)
					{
						Holder<WorkingAsset> ass;
						maintenanceQueue.pop(ass);
						{
							ScopeLock<Mutex> lock(privateMutex);
							ass->maintain(this);
						}
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void diskLoadingEntry()
			{
				try
				{
					while (true)
					{
						Holder<WorkingAsset> ass1;
						diskLoadingQueue.pop(ass1);
						Holder<Loading> ass2 = templates::move(ass1).cast<Loading>();
						ass2->diskLoad(this);
						if (ass2->compData.size() > 0)
							decompressionQueue.push(templates::move(ass2).cast<WorkingAsset>());
						else
							enqueueForProcessing(templates::move(ass2).cast<WorkingAsset>());
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void decompressionEntry()
			{
				try
				{
					while (true)
					{
						Holder<WorkingAsset> ass1;
						decompressionQueue.pop(ass1);
						Holder<Loading> ass2 = templates::move(ass1).cast<Loading>();
						ass2->decompress(this);
						enqueueForProcessing(templates::move(ass2).cast<WorkingAsset>());
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void defaultProcessingEntry()
			{
				try
				{
					while (true)
					{
						Holder<WorkingAsset> ass;
						defaultProcessingQueue.pop(ass);
						ass->process(this);
						maintenanceQueue.push(templates::move(ass));
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void listenerEntry()
			{
				try
				{
					detail::OverrideBreakpoint overrideBreakpoint;
					detail::OverrideException overrideException;
					listener = newTcpConnection(listenerAddress, listenerPort);
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed to connect to the database");
					listener.clear();
					return;
				}
				try
				{
					while (!stopping)
					{
						threadSleep(listenerPeriod);
						OPTICK_EVENT("assets network notifications");
						string line;
						while (listener->readLine(line))
						{
							const uint32 name = HashString(line.c_str());
							CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "assets network notifications: reloading asset '" + line + "' (" + name + ")");
							reload(name);
						}
					}
				}
				catch (const Disconnected &)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener disconnected");
					listener.clear();
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed");
					listener.clear();
				}
			}

			// api methods

			void defineScheme(uint32 scheme, uintPtr typeId, const AssetScheme &value)
			{
				CAGE_ASSERT(typeId != 0);
				CAGE_ASSERT(scheme < schemes.size());
				CAGE_ASSERT(schemes[scheme].typeId == 0);
				CAGE_ASSERT(value.threadIndex == m || value.threadIndex < customProcessingQueues.size());
				(AssetScheme &)schemes[scheme] = value;
				schemes[scheme].typeId = typeId;
			}

			void add(uint32 assetName)
			{
				Holder<Command> cmd = detail::systemArena().createHolder<Command>(&workingCounter);
				cmd->realName = assetName;
				cmd->type = CommandEnum::Add;
				maintenanceQueue.push(templates::move(cmd).cast<WorkingAsset>());
			}

			void remove(uint32 assetName)
			{
				Holder<Command> cmd = detail::systemArena().createHolder<Command>(&workingCounter);
				cmd->realName = assetName;
				cmd->type = CommandEnum::Remove;
				maintenanceQueue.push(templates::move(cmd).cast<WorkingAsset>());
			}

			void reload(uint32 assetName)
			{
				Holder<Command> cmd = detail::systemArena().createHolder<Command>(&workingCounter);
				cmd->realName = assetName;
				cmd->type = CommandEnum::Reload;
				maintenanceQueue.push(templates::move(cmd).cast<WorkingAsset>());
			}

			uint32 generateUniqueName()
			{
				static constexpr uint32 a = (uint32)1 << 28;
				static constexpr uint32 b = (uint32)1 << 30;
				ScopeLock<Mutex> lock(privateMutex);
				if (generateName < a || generateName > b)
					generateName = a;
				while (privateIndex.find(generateName) != privateIndex.end())
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}

			void fabricate(uint32 assetName, const string &textName, uint32 scheme, uintPtr typeId, Holder<void> &&value)
			{
				CAGE_ASSERT(typeId != 0);
				CAGE_ASSERT(scheme < schemes.size());
				CAGE_ASSERT(schemes[scheme].typeId == typeId);
				Holder<FabricateCommand> cmd = detail::systemArena().createHolder<FabricateCommand>(&workingCounter);
				cmd->realName = assetName;
				cmd->textName = textName;
				cmd->scheme = scheme;
				cmd->holder = templates::move(value);
				cmd->type = CommandEnum::Fabricate;
				maintenanceQueue.push(templates::move(cmd).cast<WorkingAsset>());
			}

			Holder<void> get(uint32 assetName, uint32 scheme, uintPtr typeId, bool throwOnInvalidScheme) const
			{
				CAGE_ASSERT(typeId != 0);
				CAGE_ASSERT(scheme < schemes.size());
				CAGE_ASSERT(schemes[scheme].typeId == typeId);
				CAGE_ASSERT(schemes[scheme].load);
				ScopeLock<RwMutex> lock(publicMutex, ReadLock());
				auto it = publicIndex.find(assetName);
				if (it == publicIndex.end())
					return {}; // not found
				const auto &a = it->second;
				if (a->failed)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset real name: " + assetName);
					CAGE_THROW_ERROR(Exception, "asset failed to load");
				}
				CAGE_ASSERT(a->ref);
				if (a->scheme != scheme)
				{
					if (throwOnInvalidScheme)
					{
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset real name: " + assetName);
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset loaded scheme: " + a->scheme + ", accessing with: " + scheme);
						CAGE_THROW_ERROR(Exception, "accessing asset with different scheme");
					}
					return {}; // invalid scheme
				}
				CAGE_ASSERT(a->ref.get() != (void *)1);
				return a->ref.share();
			}

			bool processCustomThread(uint32 threadIndex)
			{
				CAGE_ASSERT(threadIndex < customProcessingQueues.size());
				Holder<WorkingAsset> ass;
				if (customProcessingQueues[threadIndex]->tryPop(ass))
				{
					ass->process(this);
					maintenanceQueue.push(templates::move(ass));
					return true;
				}
				return false;
			}

			void listen(const string &address, uint16 port)
			{
				CAGE_ASSERT(!listenerThread);
				listenerAddress = address;
				listenerPort = port;
				listenerThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::listenerEntry>(this), "assets listener");
			}
		};

		Asset::Asset(uint32 realName_, AssetManagerImpl *impl) : textName(stringizer() + "<" + realName_ + ">"), impl(impl), guid(impl->assetGuid++), realName(realName_)
		{
			ASS_LOG(3, this, "constructed");
			impl->existsCounter++;
		}

		Asset::~Asset()
		{
			ASS_LOG(3, this, "destructed");
			impl->existsCounter--;
		}

		void Loading::diskLoad(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("loading disk load");
			OPTICK_TAG("realName", asset->realName);
			ASS_LOG(2, asset, "loading disk load");

			CAGE_ASSERT(asset);
			CAGE_ASSERT(!asset->assetHolder);
			CAGE_ASSERT(asset->scheme == m);
			CAGE_ASSERT(asset->aliasName == 0);

			try
			{
				detail::OverrideBreakpoint OverrideBreakpoint;

				Holder<File> file;
				if (!impl->findAsset.dispatch(asset->realName, file))
					file = readFile(pathJoin(impl->path, stringizer() + asset->realName));
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
				OPTICK_TAG("textName", asset->textName.c_str());
				if (h.scheme >= impl->schemes.size())
					CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
				asset->scheme = h.scheme;
				asset->aliasName = h.aliasName;

				if (!impl->schemes[asset->scheme].load)
				{
					ASS_LOG(1, asset, "no loading procedure");
					return;
				}

				asset->dependencies.resize(h.dependenciesCount);
				file->read(bufferCast<char, uint32>(asset->dependencies));

				if (h.compressedSize)
					compData.allocate(h.compressedSize);
				if (h.originalSize)
					origData.allocate(h.originalSize);
				if (h.compressedSize || h.originalSize)
				{
					MemoryBuffer &t = h.compressedSize ? compData : origData;
					file->read(t);
				}

				CAGE_ASSERT(file->tell() == file->size());
			}
			catch (const Exception &)
			{
				ASS_LOG(1, asset, "disk load failed");
				asset->dependencies.clear();
				asset->failed = true;
			}
			for (uint32 n : asset->dependencies)
				impl->add(n);
		}

		void Loading::decompress(AssetManagerImpl *impl)
		{
			if (asset->failed)
				return;

			OPTICK_EVENT("loading decompress");
			OPTICK_TAG("realName", asset->realName);
			OPTICK_TAG("textName", asset->textName.c_str());
			ASS_LOG(2, asset, "loading decompress");

			CAGE_ASSERT(!asset->assetHolder);
			CAGE_ASSERT(asset->scheme < impl->schemes.size());

			try
			{
				AssetContext ac(asset->textName, compData, origData, asset->assetHolder, asset->realName);
				impl->schemes[asset->scheme].decompress(&ac);
			}
			catch (const Exception &)
			{
				ASS_LOG(1, asset, "decompression failed");
				asset->failed = true;
			}
		}

		void Loading::process(AssetManagerImpl *impl)
		{
			if (asset->failed)
				return;

			OPTICK_EVENT("loading process");
			OPTICK_TAG("realName", asset->realName);
			OPTICK_TAG("textName", asset->textName.c_str());
			ASS_LOG(2, asset, "loading processing");

			CAGE_ASSERT(asset->scheme < impl->schemes.size());

			try
			{
				AssetContext ac(asset->textName, compData, origData, asset->assetHolder, asset->realName);
				impl->schemes[asset->scheme].load(&ac);
			}
			catch (const Exception &)
			{
				ASS_LOG(1, asset, "processing failed");
				asset->assetHolder.clear();
				asset->failed = true;
			}
		}

		void Loading::maintain(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("loading maintain");
			OPTICK_TAG("realName", asset->realName);
			OPTICK_TAG("textName", asset->textName.c_str());
			ASS_LOG(2, asset, "loading maintain");

			if (!asset->dependencies.empty())
			{
				Holder<Waiting> w = detail::systemArena().createHolder<Waiting>(&impl->workingCounter).makeShareable();
				w->asset = asset.share();
				ScopeLock<RwMutex> lock(impl->publicMutex, ReadLock());
				for (uint32 d : asset->dependencies)
				{
					if (impl->publicIndex.count(d) == 0)
						impl->waitingIndex[d].push_back(w.share());
				}
			}
			else
				impl->updatePublicIndex(asset);
		}

		Waiting::~Waiting()
		{
			asset->impl->updatePublicIndex(asset);
		}

		void Waiting::process(AssetManagerImpl *impl)
		{
			CAGE_ASSERT(false);
		}

		void Waiting::maintain(AssetManagerImpl *impl)
		{
			CAGE_ASSERT(false);
		}

		void Unloading::process(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("unloading process");
			OPTICK_TAG("realName", asset->realName);
			OPTICK_TAG("textName", asset->textName.c_str());
			ASS_LOG(2, asset, "unloading process");
			
			asset->assetHolder.clear();
			for (uint32 n : asset->dependencies)
				impl->remove(n);
		}

		void Unloading::maintain(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("unloading maintain");
			if (ptr)
			{
				auto &c = impl->privateIndex[((Asset *)ptr)->realName];
				Holder<Unloading> u = detail::systemArena().createHolder<Unloading>(&impl->workingCounter);
				for (Holder<Asset> &a : c.versions)
					if (a.get() == ptr)
						u->asset = a.share();
				CAGE_ASSERT(u->asset);
				impl->enqueueForDeprocessing(templates::move(u).cast<WorkingAsset>());
			}
			else
			{
				OPTICK_TAG("realName", asset->realName);
				OPTICK_TAG("textName", asset->textName.c_str());
				ASS_LOG(2, asset, "unloading maintain");

				auto &c = impl->privateIndex[asset->realName];
				c.versions.erase(std::remove_if(c.versions.begin(), c.versions.end(), [&](const Holder<Asset> &it) {
						return it->guid == asset->guid;
					}), c.versions.end());
				if (c.versions.empty())
					impl->privateIndex.erase(asset->realName);
			}
		}

		void Command::process(AssetManagerImpl *impl)
		{
			CAGE_ASSERT(false);
		}

		void Command::maintain(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("command");
			OPTICK_TAG("realName", realName);
			OPTICK_TAG("type", (uint32)type);

			switch (type)
			{
			case CommandEnum::Add:
			{
				auto &c = impl->privateIndex[realName];
				if (c.references++ == 0)
				{
					type = CommandEnum::Reload;
					maintain(impl);
					return;
				}
			} break;
			case CommandEnum::Remove:
			{
				auto &c = impl->privateIndex[realName];
				if (c.references-- == 1)
				{
					{
						ScopeLock<RwMutex> lock(impl->publicMutex, WriteLock());
						impl->unpublish(realName);
					}
					for (auto &a : c.versions)
						a->ref.clear();
				}
			} break;
			case CommandEnum::Reload:
			{
				auto &c = impl->privateIndex[realName];
				if (c.fabricated)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", stringizer() + "cannot reload asset " + realName + ", it is fabricated");
					return; // if an existing asset was replaced with a fabricated one, it may not be reloaded with an asset from disk
				}
				auto l = detail::systemArena().createHolder<Loading>(&impl->workingCounter);
				l->asset = detail::systemArena().createHolder<Asset>(realName, impl).makeShareable();
				c.versions.insert(c.versions.begin(), l->asset.share());
				impl->diskLoadingQueue.push(templates::move(l).cast<WorkingAsset>());
			} break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid command type enum");
			}
		}

		void FabricateCommand::maintain(AssetManagerImpl *impl)
		{
			OPTICK_EVENT("command");
			OPTICK_TAG("realName", realName);
			OPTICK_TAG("type", (uint32)type);

			switch (type)
			{
			case CommandEnum::Fabricate:
			{
				auto &c = impl->privateIndex[realName];
				c.references++;
				c.fabricated = true;
				auto l = detail::systemArena().createHolder<Loading>(&impl->workingCounter);
				l->asset = detail::systemArena().createHolder<Asset>(realName, impl).makeShareable();
				l->asset->textName = textName;
				l->asset->scheme = scheme;
				l->asset->assetHolder = templates::move(holder);
				c.versions.insert(c.versions.begin(), l->asset.share());
				impl->maintenanceQueue.push(templates::move(l).cast<WorkingAsset>());
			} break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid command type enum");
			}
		}
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
		CAGE_ASSERT(impl->workingCounter >= 0);
		return impl->workingCounter > 0;
	}

	void AssetManager::unloadCustomThread(uint32 threadIndex)
	{
		while (!unloaded())
		{
			while (processCustomThread(threadIndex));
			threadSleep(2000);
		}
	}

	void AssetManager::unloadWait()
	{
		while (!unloaded())
			threadSleep(2000);
	}

	bool AssetManager::unloaded() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		ScopeLock<Mutex> lock(impl->privateMutex);
		return impl->workingCounter == 0 && impl->existsCounter == 0;
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
}
