#include <cage-core/math.h>
#include <cage-core/networkTcp.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/config.h>
#include <cage-core/hashString.h>
#include <cage-core/serialization.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/assetManager.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>
#include <cage-core/profiling.h>
#include <cage-core/tasks.h>
#include <cage-core/logger.h>
#include <cage-core/stdHash.h>

#include <vector>
#include <atomic>
#include <algorithm>

#include <unordered_dense.h>

namespace cage
{
	const ConfigUint32 logLevel("cage/assets/logLevel", 1);
#define ASS_LOG(LEVEL, ASS, MSG) \
	{ \
		if (logLevel >= (LEVEL)) \
		{ \
			if (logLevel > 1) \
			{ \
				CAGE_LOG(SeverityEnum::Info, "assetManager", Stringizer() + "asset '" + (ASS)->textName + "' (" + (ASS)->realName + " / " + (ASS)->aliasName + "): " + MSG); \
			} \
			else \
			{ \
				CAGE_LOG(SeverityEnum::Info, "assetManager", Stringizer() + "asset '" + (ASS)->textName + "': " + MSG); \
			} \
		} \
	}

	namespace
	{
		class AssetManagerImpl;

		constexpr uint32 CurrentAssetVersion = 1;

		// one particular version of an asset
		struct Asset : private Immovable
		{
			detail::StringBase<64> textName;
			std::vector<uint32> dependencies;
			Holder<void> assetHolder;
			Holder<void> ref; // the application receives shares of the ref, its destructor will call a method in the AssetManager to schedule the asset for unloading
			AssetManagerImpl *const impl = nullptr;
			const uint32 realName = 0;
			uint32 aliasName = 0;
			uint32 scheme = m;
			bool failed = false;

			explicit Asset(AssetManagerImpl *impl, uint32 realName);
			~Asset();
		};

		// container for all versions of a single asset name
		struct Collection
		{
			std::vector<Holder<Asset>> versions; // todo replace with a small vector
			sint32 references = 0; // how many times was the asset added in the api
			bool fabricated = false; // once a fabricated asset is added to the collection, all future requests for reload are ignored

			void reload(AssetManagerImpl *impl, uint32 realName);
		};

		struct CustomProcessing : private Immovable
		{
			AssetManagerImpl *const impl = nullptr;
			Holder<Asset> asset;

			explicit CustomProcessing(AssetManagerImpl *impl);
			~CustomProcessing();
			virtual void process() = 0;
		};

		struct Loading : public CustomProcessing
		{
			Holder<PointerRange<char>> origData, compData;

			explicit Loading(Holder<Asset> &&asset);
			~Loading();
			void diskload();
			void decompress();
			void process() override;
			void operator()(uint32) { process(); }
		};

		struct TskDecompress
		{
			Holder<Loading> loading;
			void operator()(uint32);
		};

		struct Waiting : private Immovable
		{
			Holder<Asset> asset;

			explicit Waiting(Holder<Asset> &&asset);
			~Waiting();
		};

		struct Unloading : public CustomProcessing
		{
			explicit Unloading(Holder<Asset> &&asset);
			~Unloading();
			void process() override;
			void operator()(uint32) { process(); }
		};

		struct CommandBase : private Immovable
		{
			AssetManagerImpl *const impl = nullptr;

			explicit CommandBase(AssetManagerImpl *impl);
			~CommandBase();

			virtual void perform() = 0;
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

		class AssetListener : private Immovable
		{};

		class AssetManagerImpl : public AssetManager
		{
		public:
			const String path;
			const sint32 tasksPriority = 0;
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
			ConcurrentQueue<Holder<CommandBase>> commandsQueue;
			ConcurrentQueue<Holder<Loading>> diskLoadingQueue;
			std::vector<Holder<ConcurrentQueue<Holder<CustomProcessing>>>> customProcessingQueues;
			std::vector<Holder<Thread>> diskLoadingThreads;
			Holder<Thread> commandsThread;
			Holder<AssetListener> listener;

			static String findAssetsFolderPath(const AssetManagerCreateConfig &config)
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

			AssetManagerImpl(const AssetManagerCreateConfig &config) : path(findAssetsFolderPath(config)), tasksPriority(config.tasksPriority)
			{
				CAGE_LOG(SeverityEnum::Info, "assetManager", Stringizer() + "using assets path: '" + path + "'");
				schemes.resize(config.schemesMaxCount);
				customProcessingQueues.resize(config.customProcessingThreads);
				for (auto &it : customProcessingQueues)
					it = systemMemory().createHolder<ConcurrentQueue<Holder<CustomProcessing>>>();
				diskLoadingThreads.reserve(config.diskLoadingThreads);
				for (uint32 i = 0; i < config.diskLoadingThreads; i++)
					diskLoadingThreads.push_back(newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::diskLoadingEntry>(this), Stringizer() + "asset disk loading " + i));
				commandsThread = newThread(Delegate<void()>().bind<AssetManagerImpl, &AssetManagerImpl::commandsEntry>(this), "asset commands");
			}

			~AssetManagerImpl()
			{
				listener.clear();
				for (auto &it : customProcessingQueues)
					it->terminate();
				diskLoadingQueue.terminate();
				commandsQueue.terminate();
				commandsThread.clear();
				diskLoadingThreads.clear();
				CAGE_ASSERT(privateIndex.empty());
			}

			void commandsEntry()
			{
				try
				{
					while (true)
					{
						Holder<CommandBase> cmd;
						commandsQueue.pop(cmd);
						cmd->perform();
					}
				}
				catch (const ConcurrentQueueTerminated &)
				{
					// do nothing
				}
			}

			void enqueueDecompress(Holder<Loading> &&loading)
			{
				if (loading->asset->failed)
					return;
				Holder<TskDecompress> tsk = systemMemory().createHolder<TskDecompress>();
				tsk->loading = std::move(loading);
				tasksRunAsync<TskDecompress>("asset decompress", std::move(tsk), 1, tasksPriority);
			}

			void enqueueProcess(Holder<Loading> &&loading)
			{
				if (loading->asset->failed)
					return;
				const uint32 scheme = loading->asset->scheme;
				CAGE_ASSERT(scheme < schemes.size());
				if (!schemes[scheme].load)
					return;
				const uint32 thr = schemes[scheme].threadIndex;
				if (thr == m)
					tasksRunAsync<Loading>("asset loading process", std::move(loading), 1, tasksPriority);
				else
					customProcessingQueues[thr]->push(std::move(loading).cast<CustomProcessing>());
			}

			void enqueueProcess(Holder<Unloading> &&unloading)
			{
				if (unloading->asset->failed)
					return;
				const uint32 thr = schemes[unloading->asset->scheme].threadIndex;
				if (thr == m)
					tasksRunAsync<Unloading>("asset unloading process", std::move(unloading), 1, tasksPriority);
				else
					customProcessingQueues[thr]->push(std::move(unloading).cast<CustomProcessing>());
			}

			void diskLoadingEntry()
			{
				try
				{
					while (true)
					{
						Holder<Loading> loading;
						diskLoadingQueue.pop(loading);
						loading->diskload();
						if (loading->compData.size())
							enqueueDecompress(std::move(loading));
						else
							enqueueProcess(std::move(loading));
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
				Holder<CustomProcessing> proc;
				if (customProcessingQueues[threadIndex]->tryPop(proc))
				{
					proc->process();
					return true;
				}
				return false;
			}

			void dereference(Asset *ass)
			{
				struct DereferencedCommand : public CommandBase
				{
					Asset *ass = nullptr;

					explicit DereferencedCommand(Asset *ass) : CommandBase(ass->impl), ass(ass)
					{}

					void perform() override
					{
						ScopeLock lock(impl->privateMutex);
						auto &c = impl->privateIndex[ass->realName];
						Holder<Asset> aa;
						for (Holder<Asset> &a : c.versions)
							if (+a == ass)
								aa = a.share();
						CAGE_ASSERT(aa);
						impl->enqueueProcess(systemMemory().createHolder<Unloading>(std::move(aa)));
					}
				};

				ASS_LOG(2, ass, "dereferenced");
				commandsQueue.push(systemMemory().createImpl<CommandBase, DereferencedCommand>(ass));
			}

			void unpublish(uint32 realName)
			{
				// public mutex already locked

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

			void publish(Holder<Asset> &&ass)
			{
				// private mutex already locked

				auto &c = privateIndex[ass->realName];
				if (c.references == 0)
				{
					enqueueProcess(systemMemory().createHolder<Unloading>(std::move(ass)));
					return;
				}

				ASS_LOG(2, ass, "publishing");

				{
					CAGE_ASSERT(!ass->ref);
					struct PublicAssetReference
					{
						Asset *ass = nullptr;
						AssetManagerImpl *impl = nullptr;
						~PublicAssetReference()
						{
							impl->dereference(ass);
						}
					};
					Holder<PublicAssetReference> ctrl = systemMemory().createHolder<PublicAssetReference>();
					ctrl->ass = +ass;
					ctrl->impl = this;
					ass->ref = Holder<void>(ass->assetHolder ? +ass->assetHolder : (void *)1, std::move(ctrl));
				}

				{
					ScopeLock lock(publicMutex, WriteLockTag());
					bool found = false;
					for (const auto &v : c.versions)
					{
						if (found)
							v->ref.clear();
						else if (v->ref || v->failed)
						{
							unpublish(v->realName);
							publicIndex[v->realName] = v.share();
							if (v->aliasName)
							{
								CAGE_ASSERT(publicIndex.count(v->aliasName) == 0);
								publicIndex[v->aliasName] = v.share();
							}
							found = true;
						}
					}
					CAGE_ASSERT(found);
					waitingIndex.erase(ass->realName);
				}
			}

			void decremenWorking()
			{
				if (--workingCounter == 0)
					keepOpen.clear();
			}
		};

		struct PublishCommand : public CommandBase
		{
			Holder<Asset> asset;

			explicit PublishCommand(Holder<Asset> &&asset) : CommandBase(asset->impl), asset(std::move(asset))
			{}

			void perform() override
			{
				ScopeLock lock(impl->privateMutex);
				impl->publish(std::move(asset));
			}
		};

		Asset::Asset(AssetManagerImpl *impl, uint32 realName_) : textName(Stringizer() + "<" + realName_ + ">"), impl(impl), realName(realName_)
		{
			ASS_LOG(3, this, "constructed");
			impl->existsCounter++;
		}

		Asset::~Asset()
		{
			ASS_LOG(3, this, "destructed");
			impl->existsCounter--;
		}

		void Collection::reload(AssetManagerImpl *impl, uint32 realName)
		{
			if (fabricated)
			{
				CAGE_LOG(SeverityEnum::Warning, "assetManager", Stringizer() + "cannot reload fabricated asset " + realName);
				return; // if an existing asset was replaced with a fabricated one, it may not be reloaded with an asset from disk
			}
			auto l = systemMemory().createHolder<Loading>(systemMemory().createHolder<Asset>(impl, realName));
			versions.insert(versions.begin(), l->asset.share());
			impl->diskLoadingQueue.push(std::move(l));
		}

		CustomProcessing::CustomProcessing(AssetManagerImpl *impl) : impl(impl)
		{
			impl->workingCounter++;
		}

		CustomProcessing::~CustomProcessing()
		{
			impl->decremenWorking();
		}

		Loading::Loading(Holder<Asset> &&asset_) : CustomProcessing(asset_->impl)
		{
			asset = std::move(asset_);
		}

		Loading::~Loading()
		{
			if (asset->dependencies.empty())
				impl->commandsQueue.push(systemMemory().createImpl<CommandBase, PublishCommand>(asset.share()));
			else
			{
				Holder<Waiting> w = systemMemory().createHolder<Waiting>(asset.share());
				ScopeLock lock(impl->publicMutex, WriteLockTag());
				for (uint32 d : asset->dependencies)
					if (impl->publicIndex.count(d) == 0)
						impl->waitingIndex[d].push_back(w.share());
			}
		}

		void Loading::diskload()
		{
			ProfilingScope profiling("loading from disk");
			profiling.set(asset->textName);
			ASS_LOG(2, asset, "loading from disk");

			CAGE_ASSERT(!asset->assetHolder);
			CAGE_ASSERT(asset->scheme == m);
			CAGE_ASSERT(asset->aliasName == 0);

			try
			{
				detail::OverrideBreakpoint OverrideBreakpoint;

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
				if (h.scheme >= asset->impl->schemes.size())
					CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
				asset->scheme = h.scheme;
				asset->aliasName = h.aliasName;

				if (!impl->schemes[asset->scheme].load)
				{
					ASS_LOG(2, asset, "has no loading procedure");
					return;
				}

				asset->dependencies.resize(h.dependenciesCount);
				file->read(bufferCast<char, uint32>(asset->dependencies));

				if (h.compressedSize)
					compData = systemMemory().createBuffer(h.compressedSize);
				if (h.originalSize)
					origData = systemMemory().createBuffer(h.originalSize);
				if (h.compressedSize || h.originalSize)
					file->read(h.compressedSize ? compData : origData);

				CAGE_ASSERT(file->tell() == file->size());
			}
			catch (const Exception &)
			{
				ASS_LOG(1, asset, "loading from disk failed");
				asset->dependencies.clear();
				asset->failed = true;
			}

			for (uint32 n : asset->dependencies)
				impl->add(n);
		}

		void Loading::decompress()
		{
			ProfilingScope profiling("asset decompression");
			profiling.set(asset->textName);
			ASS_LOG(2, asset, "decompression");
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

		void Loading::process()
		{
			ProfilingScope profiling("asset loading process");
			profiling.set(asset->textName);
			ASS_LOG(2, asset, "loading process");
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

		void TskDecompress::operator()(uint32)
		{
			loading->decompress();
			auto *impl = loading->asset->impl;
			impl->enqueueProcess(std::move(loading));
		}

		Waiting::Waiting(Holder<Asset> &&asset_) : asset(std::move(asset_))
		{
			asset->impl->workingCounter++;
			ASS_LOG(2, asset, "waiting for dependencies");
		}

		Waiting::~Waiting()
		{
			asset->impl->commandsQueue.push(systemMemory().createImpl<CommandBase, PublishCommand>(asset.share()));
			asset->impl->workingCounter--;
		}

		Unloading::Unloading(Holder<Asset> &&asset_) : CustomProcessing(asset_->impl)
		{
			asset = std::move(asset_);
		}

		Unloading::~Unloading()
		{
			struct UnloadCommand : public CommandBase
			{
				Holder<Asset> asset;

				explicit UnloadCommand(Holder<Asset> &&asset) : CommandBase(asset->impl), asset(std::move(asset))
				{}

				void perform() override
				{
					for (uint32 n : asset->dependencies)
						impl->remove(n);

					ScopeLock lock(impl->privateMutex);
					auto &c = impl->privateIndex[asset->realName];
					std::erase_if(c.versions, [&](const Holder<Asset> &it) {
						return +it == +asset;
					});
					if (c.versions.empty())
						impl->privateIndex.erase(asset->realName);
				}
			};

			impl->commandsQueue.push(systemMemory().createImpl<CommandBase, UnloadCommand>(std::move(asset)));
		}

		void Unloading::process()
		{
			ProfilingScope profiling("asset unloading process");
			profiling.set(asset->textName);
			ASS_LOG(2, asset, "unloading process");

			asset->assetHolder.clear();
		}

		CommandBase::CommandBase(AssetManagerImpl *impl) : impl(impl)
		{
			impl->workingCounter++;
		}

		CommandBase::~CommandBase()
		{
			impl->decremenWorking();
		}
	}

	void AssetManager::add(uint32 assetName)
	{
		struct AddCommand : public CommandBase
		{
			uint32 name = 0;

			explicit AddCommand(AssetManagerImpl *impl, uint32 name) : CommandBase(impl), name(name)
			{}

			void perform() override
			{
				ScopeLock lock(impl->privateMutex);
				auto &c = impl->privateIndex[name];
				if (c.references++ == 0)
					c.reload(impl, name);
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->commandsQueue.push(systemMemory().createImpl<CommandBase, AddCommand>(impl, assetName));
	}

	void AssetManager::remove(uint32 assetName)
	{
		struct RemoveCommand : public CommandBase
		{
			uint32 name = 0;

			explicit RemoveCommand(AssetManagerImpl *impl, uint32 name) : CommandBase(impl), name(name)
			{}

			void perform() override
			{
				ScopeLock lock(impl->privateMutex);
				auto &c = impl->privateIndex[name];
				if (c.references-- == 1)
				{
					{
						ScopeLock lock(impl->publicMutex, WriteLockTag());
						impl->unpublish(name);
					}
					for (auto &a : c.versions)
						a->ref.clear();
				}
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->commandsQueue.push(systemMemory().createImpl<CommandBase, RemoveCommand>(impl, assetName));
	}

	void AssetManager::reload(uint32 assetName)
	{
		struct ReloadCommand : public CommandBase
		{
			uint32 name = 0;

			explicit ReloadCommand(AssetManagerImpl *impl, uint32 name) : CommandBase(impl), name(name)
			{}

			void perform() override
			{
				ScopeLock lock(impl->privateMutex);
				auto &c = impl->privateIndex[name];
				c.reload(impl, name);
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->commandsQueue.push(systemMemory().createImpl<CommandBase, ReloadCommand>(impl, assetName));
	}

	uint32 AssetManager::generateUniqueName()
	{
		static constexpr uint32 a = (uint32)1 << 28;
		static constexpr uint32 b = (uint32)1 << 30;
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		uint32 &name = impl->generateName;
		ScopeLock lock(impl->privateMutex);
		if (name < a || name > b)
			name = a;
		while (impl->privateIndex.count(name))
			name = name == b ? a : name + 1;
		return name++;
	}

	bool AssetManager::processCustomThread(uint32 threadIndex)
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		return impl->customProcessEntry(threadIndex);
	}

	void AssetManager::unloadCustomThread(uint32 threadIndex)
	{
		while (!unloaded())
		{
			while (processCustomThread(threadIndex));
			threadYield();
		}
	}

	void AssetManager::unloadWait()
	{
		while (!unloaded())
			threadYield();
	}

	bool AssetManager::processing() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		return impl->workingCounter > 0;
	}

	bool AssetManager::unloaded() const
	{
		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		ScopeLock lock(impl->privateMutex);
		return impl->workingCounter == 0 && impl->existsCounter == 0;
	}

	void AssetManager::listen(const String &address, uint16 port, uint64 listenerPeriod)
	{
		class AssetListenerImpl : public AssetListener
		{
		public:
			AssetManager *const man = nullptr;
			String address;
			uint16 port = 0;
			uint64 period = 0;
			std::atomic<bool> stopping = false;
			Holder<TcpConnection> tcp;
			Holder<Thread> thread;

			AssetListenerImpl(AssetManager *man, const String &address, uint32 port, uint64 period) : man(man), address(address), port(port), period(period)
			{
				thread = newThread(Delegate<void()>().bind<AssetListenerImpl, &AssetListenerImpl::thrEntry>(this), "assets listener");
			}

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
					tcp.clear();
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
					tcp.clear();
				}
				catch (...)
				{
					CAGE_LOG(SeverityEnum::Warning, "assetManager", "assets network notifications listener failed with exception:");
					detail::logCurrentCaughtException();
					tcp.clear();
				}
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl*)this;
		impl->listener.clear();
		impl->listener = systemMemory().createImpl<AssetListener, AssetListenerImpl>(this, address, port, listenerPeriod);
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

	void AssetManager::fabricate_(uint32 scheme, uint32 assetName, const String &textName, Holder<void> &&value)
	{
		struct FabricateCommand : public CommandBase
		{
			detail::StringBase<64> textName;
			Holder<void> value;
			uint32 scheme = m;
			uint32 realName = m;

			explicit FabricateCommand(AssetManagerImpl *impl, const detail::StringBase<64> &textName, Holder<void> value, uint32 scheme, uint32 realName) : CommandBase(impl), textName(textName), value(std::move(value)), scheme(scheme), realName(realName)
			{}

			void perform() override
			{
				ScopeLock lock(impl->privateMutex);
				auto &c = impl->privateIndex[realName];
				c.references++;
				c.fabricated = true;
				Holder<Asset> a = systemMemory().createHolder<Asset>(impl, realName);
				a->textName = textName;
				a->scheme = scheme;
				a->assetHolder = std::move(value);
				c.versions.insert(c.versions.begin(), a.share());
				impl->publish(std::move(a));
			}
		};

		AssetManagerImpl *impl = (AssetManagerImpl *)this;
		impl->commandsQueue.push(systemMemory().createImpl<CommandBase, FabricateCommand>(impl, textName, std::move(value), scheme, assetName));
	}

	Holder<void> AssetManager::get_(uint32 scheme, uint32 assetName, bool throwOnInvalidScheme) const
	{
		const AssetManagerImpl *impl = (const AssetManagerImpl *)this;
		const auto &schemes = impl->schemes;
		const auto &publicIndex = impl->publicIndex;
		CAGE_ASSERT(scheme < schemes.size());
		CAGE_ASSERT(schemes[scheme].load);
		ScopeLock lock(impl->publicMutex, ReadLockTag());
		auto it = publicIndex.find(assetName);
		if (it == publicIndex.end())
			return {}; // not found
		const auto &a = it->second;
		if (a->scheme != scheme)
		{
			if (throwOnInvalidScheme)
			{
				CAGE_LOG_THROW(Stringizer() + "asset real name: " + assetName + ", text name: " + a->textName);
				CAGE_LOG_THROW(Stringizer() + "asset loaded scheme: " + a->scheme + ", accessing with: " + scheme);
				CAGE_THROW_ERROR(Exception, "accessing asset with different scheme");
			}
			return {}; // invalid scheme
		}
		if (a->failed)
		{
			CAGE_LOG_THROW(Stringizer() + "asset real name: " + assetName + ", text name: " + a->textName);
			CAGE_THROW_ERROR(Exception, "asset failed to load");
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
