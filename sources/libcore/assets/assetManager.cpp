#include <set>
#include <map>
#include <vector>
#include <atomic>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/assetStructs.h>
#include <cage-core/assetManager.h>
#include <cage-core/config.h>
#include <cage-core/hashString.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/ctl/unordered_map.h>

#include <optick.h>

namespace cage
{
	ConfigUint32 logLevel("cage/assets/logLevel", 0);
#define ASS_LOG(LEVEL, ASS, MSG) { if (logLevel >= (LEVEL)) { CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "asset '" + (ASS)->textName + "' (" + (ASS)->realName + " / " + (ASS)->aliasName + "): " + MSG); } }

	namespace
	{
		static const uint32 currentAssetVersion = 1;

		void defaultDecompress(const AssetContext *context)
		{
			if (context->compressedSize == 0)
				return;
			uintPtr res = detail::decompress(context->compressedData, numeric_cast<uintPtr>(context->compressedSize), context->originalData, numeric_cast<uintPtr>(context->originalSize));
			CAGE_ASSERT(res == context->originalSize, res, context->originalSize);
		}

		void defaultDone(const AssetContext *context)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}

		struct assetContextPrivateStruct : public AssetContext
		{
			std::vector<uint32> dependencies;
			std::vector<uint32> dependenciesNew;
			uint32 aliasPrevious;
			uint32 scheme;
			uint32 references;
			std::atomic<bool> processing, ready, error, dependenciesDoneFlag, fabricated;
			assetContextPrivateStruct() : aliasPrevious(0), scheme(m), references(0), processing(false), ready(false), error(true), dependenciesDoneFlag(false), fabricated(false) {}
		};

		struct assetSchemePrivateStruct : public AssetScheme
		{
			uintPtr typeSize;
			assetSchemePrivateStruct() : typeSize(0) {}
		};

		void destroy(assetContextPrivateStruct *ptr)
		{
			// these pointers are owned by the hash map, not by the concurrent queues
			ptr->error = true;
		}

		class assetManagerImpl : public AssetManager
		{
		public:
			std::map<uint32, std::set<assetContextPrivateStruct*>> aliasNames;
			std::vector<assetSchemePrivateStruct> schemes;
			cage::unordered_map<uint32, assetContextPrivateStruct*> index;
			Holder<TcpConnection> listener;
			Holder<Thread> loadingThread;
			Holder<Thread> decompressionThread;

			std::vector<Holder<ConcurrentQueue<assetContextPrivateStruct*>>> queueCustomLoad, queueCustomDone;
			Holder<ConcurrentQueue<assetContextPrivateStruct*>> queueLoadFile, queueDecompression;
			Holder<ConcurrentQueue<assetContextPrivateStruct*>> queueAddDependencies, queueWaitDependencies, queueRemoveDependencies;

			string path;
			uint32 countTotal, countProcessing;
			uint32 hackQueueWaitCounter; // it is desirable to process as many items from the queue in single tick, however, since the items are being pushed back to the same queue, some form of termination mechanism must be provided
			uint32 generateName;
			std::atomic<bool> destroying;

			Holder<ConcurrentQueue<assetContextPrivateStruct*>> createQueue()
			{
				return newConcurrentQueue<assetContextPrivateStruct*>({}, Delegate<void(assetContextPrivateStruct*)>().bind<&destroy>());
			}

			assetManagerImpl(const AssetManagerCreateConfig &config) : countTotal(0), countProcessing(0), hackQueueWaitCounter(0), generateName(0), destroying(false)
			{
				try
				{
					try
					{
						detail::OverrideException oe;
						path = pathSearchTowardsRoot(config.assetsFolderName, pathWorkingDir(), PathTypeFlags::Directory | PathTypeFlags::Archive);
					}
					catch(const Exception &)
					{
						path = pathSearchTowardsRoot(config.assetsFolderName, pathExtractPath(detail::getExecutableFullPath()), PathTypeFlags::Directory | PathTypeFlags::Archive);
					}
				}
				catch (const Exception &)
				{
					CAGE_LOG(SeverityEnum::Error, "assetManager", "failed to find the directory with assets");
					throw;
				}
				CAGE_LOG(SeverityEnum::Info, "assetManager", stringizer() + "using asset path: '" + path + "'");
				schemes.resize(config.schemeMaxCount);
				queueCustomLoad.reserve(config.threadMaxCount);
				queueCustomDone.reserve(config.threadMaxCount);
				for (uint32 i = 0; i < config.threadMaxCount; i++)
				{
					queueCustomLoad.push_back(createQueue());
					queueCustomDone.push_back(createQueue());
				}
				queueLoadFile = createQueue();
				queueDecompression = createQueue();
				queueAddDependencies = createQueue();
				queueWaitDependencies = createQueue();
				queueRemoveDependencies = createQueue();
				loadingThread = newThread(Delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::loadingThreadEntry>(this), "asset disk io");
				decompressionThread = newThread(Delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::decompressionThreadEntry>(this), "asset decompression");
			}

			~assetManagerImpl()
			{
				CAGE_ASSERT(countTotal == 0, countTotal);
				CAGE_ASSERT(countProcessing == 0, countProcessing);
				CAGE_ASSERT(index.size() == 0, index.size());
				CAGE_ASSERT(aliasNames.size() == 0, aliasNames.size());
				destroying = true;
				queueLoadFile->push(nullptr);
				queueDecompression->push(nullptr);
				loadingThread->wait();
				decompressionThread->wait();
			}

			MemoryBuffer findAsset(uint32 name)
			{
				MemoryBuffer buff;
				if (findAssetBuffer.dispatch(name, buff))
					return buff;
				string pth;
				if (!findAssetPath.dispatch(name, pth))
					pth = pathJoin(path, string(name));
				Holder<File> f = newFile(pth, FileMode(true, false));
				auto s = f->size();
				buff.allocate(numeric_cast<uintPtr>(s));
				f->read(buff.data(), s);
				return buff;
			}

			bool processLoadingThread()
			{
				assetContextPrivateStruct *ass = nullptr;
				queueLoadFile->pop(ass);
				if (ass)
				{
					OPTICK_EVENT("loading");
					ASS_LOG(2, ass, "disk load");
					CAGE_ASSERT(ass->processing);
					CAGE_ASSERT(!ass->fabricated);
					CAGE_ASSERT(ass->originalData == nullptr);
					CAGE_ASSERT(ass->compressedData == nullptr);
					CAGE_ASSERT(ass->aliasPrevious == 0);
					OPTICK_TAG("realName", ass->realName);
					ass->aliasPrevious = ass->aliasName;
					ass->scheme = m;
					ass->assetFlags = 0;
					ass->compressedSize = ass->originalSize = 0;
					ass->aliasName = 0;
					ass->textName = stringizer() + "<" + ass->realName + ">";
					ass->dependenciesNew.clear();
					ass->compressedData = ass->originalData = nullptr;
					try
					{
						detail::OverrideBreakpoint OverrideBreakpoint;
						MemoryBuffer buff = findAsset(ass->realName);
						if (buff.size() < sizeof(AssetHeader))
							CAGE_THROW_ERROR(Exception, "asset is missing required header");
						AssetHeader *h = (AssetHeader*)buff.data();
						if (detail::memcmp(h->cageName, "cageAss", 8) != 0)
							CAGE_THROW_ERROR(Exception, "file is not a cage asset");
						if (h->version != currentAssetVersion)
							CAGE_THROW_ERROR(Exception, "cage asset version mismatch");
						if (h->textName[sizeof(h->textName) - 1] != 0)
							CAGE_THROW_ERROR(Exception, "cage asset text name not bounded");
						if (h->scheme >= schemes.size())
							CAGE_THROW_ERROR(Exception, "cage asset scheme out of range");
						if (ass->scheme != m && ass->scheme != h->scheme)
							CAGE_THROW_ERROR(Exception, "cage asset scheme cannot change");
						if (buff.size() < sizeof(AssetHeader) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(Exception, "cage asset file dependencies truncated");
						ass->scheme = h->scheme;
						ass->assetFlags = h->flags;
						ass->compressedSize = h->compressedSize;
						ass->originalSize = h->originalSize;
						ass->aliasName = h->aliasName;
						ass->textName = h->textName;
						OPTICK_TAG("textName", ass->textName.c_str());
						ass->dependenciesNew.resize(h->dependenciesCount);
						if (h->dependenciesCount)
							detail::memcpy(ass->dependenciesNew.data(), buff.data() + sizeof(AssetHeader), h->dependenciesCount * sizeof(uint32));
						uintPtr allocSize = numeric_cast<uintPtr>(ass->compressedSize + ass->originalSize);
						uintPtr readSize = numeric_cast<uintPtr>(h->compressedSize == 0 ? h->originalSize : h->compressedSize);
						CAGE_ASSERT(readSize <= allocSize, readSize, allocSize);
						if (buff.size() < readSize + sizeof(AssetHeader) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(Exception, "cage asset file content truncated");
						ass->compressedData = detail::systemArena().allocate(allocSize, sizeof(uintPtr));
						detail::memcpy(ass->compressedData, buff.data() + sizeof(AssetHeader) + h->dependenciesCount * sizeof(uint32), readSize);
						ass->originalData = (char*)ass->compressedData + ass->compressedSize;
					}
					catch (...)
					{
						ass->error = true;
					}
					if (ass->error)
					{ // bypass rest of the loading pipeline
						ass->dependenciesDoneFlag = true;
						queueWaitDependencies->push(ass);
					}
					else
					{
						if (ass->compressedSize)
							queueDecompression->push(ass);
						else
						{
							CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
							queueCustomLoad[schemes[ass->scheme].threadIndex]->push(ass);
						}
						queueAddDependencies->push(ass);
					}
					return true;
				}

				return false;
			}

			bool processDecompressionThread()
			{
				assetContextPrivateStruct *ass = nullptr;
				queueDecompression->pop(ass);
				if (ass)
				{
					OPTICK_EVENT("decompression");
					ASS_LOG(2, ass, "decompression");
					CAGE_ASSERT(ass->processing);
					CAGE_ASSERT(!ass->fabricated);
					if (!ass->error)
					{
						CAGE_ASSERT(ass->compressedData);
						CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
						OPTICK_TAG("realName", ass->realName);
						OPTICK_TAG("textName", ass->textName.c_str());
						OPTICK_TAG("originalSize", ass->originalSize);
						OPTICK_TAG("compressedSize", ass->compressedSize);
						try
						{
							if (schemes[ass->scheme].decompress)
								schemes[ass->scheme].decompress(ass, schemes[ass->scheme].schemePointer);
							else
								defaultDecompress(ass);
						}
						catch (...)
						{
							ass->error = true;
						}
					}
					CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
					queueCustomLoad[schemes[ass->scheme].threadIndex]->push(ass);
					return true;
				}

				return false;
			}

			bool processCustomThread(uint32 threadIndex)
			{
				CAGE_ASSERT(threadIndex < queueCustomDone.size(), threadIndex, queueCustomDone.size());

				{ // done
					assetContextPrivateStruct *ass = nullptr;
					queueCustomDone[threadIndex]->tryPop(ass);
					if (ass)
					{
						OPTICK_EVENT("asset done");
						ASS_LOG(1, ass, "custom done");
						CAGE_ASSERT(ass->processing);
						CAGE_ASSERT(!ass->fabricated);
						CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
						OPTICK_TAG("realName", ass->realName);
						OPTICK_TAG("textName", ass->textName.c_str());
						try
						{
							if (schemes[ass->scheme].done)
								schemes[ass->scheme].done(ass, schemes[ass->scheme].schemePointer);
							else
								defaultDone(ass);
						}
						catch (...)
						{
							ass->error = true;
						}
						queueRemoveDependencies->push(ass);
						return true;
					}
				}

				{ // load
					assetContextPrivateStruct *ass = nullptr;
					queueCustomLoad[threadIndex]->tryPop(ass);
					if (ass)
					{
						OPTICK_EVENT("asset load");
						ASS_LOG(1, ass, "custom load");
						CAGE_ASSERT(ass->processing);
						CAGE_ASSERT(!ass->fabricated);
						if (!ass->error)
						{
							CAGE_ASSERT(ass->originalData);
							CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
							OPTICK_TAG("realName", ass->realName);
							OPTICK_TAG("textName", ass->textName.c_str());
							OPTICK_TAG("scheme", ass->scheme);
							try
							{
								if (schemes[ass->scheme].load)
									schemes[ass->scheme].load(ass, schemes[ass->scheme].schemePointer);
							}
							catch (...)
							{
								ass->error = true;
							}
						}
						queueWaitDependencies->push(ass);
						return true;
					}
				}

				return false;
			}

			bool processControlThread()
			{
				{ // remove
					assetContextPrivateStruct *ass = nullptr;
					queueRemoveDependencies->tryPop(ass);
					if (ass)
					{
						OPTICK_EVENT("asset remove");
						ASS_LOG(2, ass, "remove dependencies");
						CAGE_ASSERT(ass->processing);
						OPTICK_TAG("realName", ass->realName);
						OPTICK_TAG("textName", ass->textName.c_str());
						for (auto it : ass->dependencies)
							remove(it);
						ass->dependencies.clear();
						ass->processing = false;
						CAGE_ASSERT(countProcessing > 0);
						countProcessing--;
						if (ass->references > 0)
							assetStartLoading(ass);
						else
						{
							ASS_LOG(2, ass, "destroy");
							interNameClear(ass->aliasName, ass);
							index.erase(ass->realName);
							detail::systemArena().destroy<assetContextPrivateStruct>(ass);
							CAGE_ASSERT(countTotal > 0);
							countTotal--;
						}
						return true;
					}
				}

				{ // add
					assetContextPrivateStruct *ass = nullptr;
					queueAddDependencies->tryPop(ass);
					if (ass)
					{
						OPTICK_EVENT("asset add dependencies");
						ASS_LOG(2, ass, "add dependencies");
						CAGE_ASSERT(ass->processing);
						CAGE_ASSERT(!ass->fabricated);
						OPTICK_TAG("realName", ass->realName);
						OPTICK_TAG("textName", ass->textName.c_str());
						for (auto it : ass->dependenciesNew)
							add(it);
						for (auto it : ass->dependencies)
							remove(it);
						std::vector<uint32>().swap(ass->dependencies);
						std::swap(ass->dependenciesNew, ass->dependencies);
						ass->dependenciesDoneFlag = true;
						return true;
					}
				}

				{ // wait
					assetContextPrivateStruct *ass = nullptr;
					queueWaitDependencies->tryPop(ass);
					if (ass)
					{
						OPTICK_EVENT("asset check dependencies");
						ASS_LOG(3, ass, "check dependencies");
						CAGE_ASSERT(ass->processing);
						CAGE_ASSERT(!ass->fabricated);
						OPTICK_TAG("realName", ass->realName);
						OPTICK_TAG("textName", ass->textName.c_str());
						bool wait = !ass->dependenciesDoneFlag;
						if (!wait)
						{
							for (auto it : ass->dependencies)
							{
								switch (state(it))
								{
								case AssetStateEnum::Ready:
									break;
								case AssetStateEnum::Error:
									ass->error = true;
									break;
								case AssetStateEnum::Unknown:
									wait = true;
									break;
								default:
									CAGE_THROW_CRITICAL(Exception, "invalid (or impossible) asset state");
								}
							}
						}
						if (wait)
						{
							queueWaitDependencies->push(ass);
							return (hackQueueWaitCounter++ % queueWaitDependencies->estimatedSize()) != 0;
						}
						if (ass->aliasName != ass->aliasPrevious)
						{
							interNameSet(ass->aliasName, ass);
							interNameClear(ass->aliasPrevious, ass);
						}
						ass->aliasPrevious = 0;
						ass->dependenciesDoneFlag = false;
						detail::systemArena().deallocate(ass->compressedData);
						ass->compressedData = nullptr;
						ass->originalData = nullptr;
						ass->compressedSize = 0;
						ass->originalSize = 0;
						ass->processing = false;
						ass->ready = true;
						CAGE_ASSERT(countProcessing > 0);
						countProcessing--;
						if (ass->error)
							CAGE_LOG(SeverityEnum::Warning, "assetManager", stringizer() + "asset: '" + ass->textName + "', loading failed");
						if (ass->references == 0)
							assetStartRemoving(ass);
						return true;
					}
				}

				// control thread - network updates
				if (listener)
				{
					OPTICK_EVENT("network notifications");
					try
					{
						uint32 cnt = 0;
						string line;
						while (listener->readLine(line))
						{
							uint32 name = HashString(line.c_str());
							CAGE_ASSERT(name != 0);
							assetContextPrivateStruct *ass = nullptr;
							{
								auto it = index.find(name);
								if (it != index.end())
									ass = it->second;
							}
							if (ass && ass->references > 0)
							{
								reload(name);
								cnt++;
							}
						}
						return cnt > 0;
					}
					catch (...)
					{
						listener.clear();
					}
				}

				return false;
			}

			void loadingThreadEntry()
			{
				while (processLoadingThread());
			}

			void decompressionThreadEntry()
			{
				while (processDecompressionThread());
			}

			void assetStartLoading(assetContextPrivateStruct *ass)
			{
				if (ass->processing || ass->fabricated)
					return;
				ASS_LOG(2, ass, "start loading");
				countProcessing++;
				ass->processing = true;
				ass->error = false;
				queueLoadFile->push(ass);
			}

			void assetStartRemoving(assetContextPrivateStruct *ass)
			{
				if (ass->processing)
					return;
				ASS_LOG(2, ass, "start removing");
				countProcessing++;
				ass->processing = true;
				ass->ready = false;
				if (ass->scheme == m || ass->fabricated)
					queueRemoveDependencies->push(ass);
				else
				{
					CAGE_ASSERT(ass->scheme < schemes.size(), ass->scheme, schemes.size());
					queueCustomDone[schemes[ass->scheme].threadIndex]->push(ass);
				}
			}

			void interNameSet(uint32 interName, assetContextPrivateStruct *ass)
			{
				if (interName == 0)
					return;
				CAGE_ASSERT(ass);
				auto &s = aliasNames[interName];
				CAGE_ASSERT(s.find(ass) == s.end());
				s.insert(ass);
				if (!index.count(interName))
					index[interName] = ass;
			}

			void interNameClear(uint32 interName, assetContextPrivateStruct *ass)
			{
				if (interName == 0)
					return;
				CAGE_ASSERT(ass);
				CAGE_ASSERT(index.count(interName));
				CAGE_ASSERT(aliasNames.find(interName) != aliasNames.end());
				auto &s = aliasNames[interName];
				CAGE_ASSERT(s.find(ass) != s.end());
				s.erase(ass);
				index.erase(interName);
				if (s.empty())
					aliasNames.erase(interName);
				else
					index[interName] = *s.begin();
			}

			uint32 generateUniqueName()
			{
				static const uint32 a = (uint32)1 << 28;
				static const uint32 b = (uint32)1 << 30;
				if (generateName < a || generateName > b)
					generateName = a;
				while (state(generateName) != AssetStateEnum::NotFound)
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}
		};
	}

	void AssetManager::listen(const string &address, uint16 port)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		impl->listener = newTcpConnection(address, port);
	}

	uint32 AssetManager::dependenciesCount(uint32 assetName) const
	{
		CAGE_ASSERT(ready(assetName), assetName);
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		return numeric_cast<uint32>(ass->dependencies.size());
	}

	uint32 AssetManager::dependencyName(uint32 assetName, uint32 index) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(index < ass->dependencies.size(), index, ass->dependencies.size(), assetName);
		return ass->dependencies[index];
	}

	PointerRange<const uint32> AssetManager::dependencies(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		return ass->dependencies;
	}

	uint32 AssetManager::countTotal() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countTotal;
	}

	uint32 AssetManager::countProcessing() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countProcessing;
	}

	bool AssetManager::processControlThread()
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processControlThread();
	}

	bool AssetManager::processCustomThread(uint32 threadIndex)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processCustomThread(threadIndex);
	}

	AssetStateEnum AssetManager::state(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		auto it = impl->index.find(assetName);
		if (it == impl->index.end())
			return AssetStateEnum::NotFound;
		assetContextPrivateStruct *ass = it->second;
		if (ass->error)
			return AssetStateEnum::Error;
		if (ass->ready)
			return AssetStateEnum::Ready;
		return AssetStateEnum::Unknown;
	}

	bool AssetManager::ready(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		switch (state(assetName))
		{
		case AssetStateEnum::Ready:
			return true;
		case AssetStateEnum::NotFound:
			return false;
		case AssetStateEnum::Error:
		{
			assetContextPrivateStruct *ass = impl->index.at(assetName);
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset real name: " + assetName);
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "asset text name: '" + ass->textName + "'");
			CAGE_THROW_ERROR(Exception, "asset has failed to load");
		}
		case AssetStateEnum::Unknown:
			return false;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid asset state enum");
		}
	}

	uint32 AssetManager::scheme(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		auto it = impl->index.find(assetName);
		if (it == impl->index.end())
			return m;
		return it->second->scheme;
	}

	uint32 AssetManager::generateUniqueName()
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->generateUniqueName();
	}

	void AssetManager::add(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(assetName != 0);
		assetContextPrivateStruct *ass = nullptr;
		if (!impl->index.count(assetName))
		{
			ass = detail::systemArena().createObject<assetContextPrivateStruct>();
			ass->realName = assetName;
			ass->textName = stringizer() + "<" + assetName + ">";
			ASS_LOG(2, ass, "created");
			impl->index[ass->realName] = ass;
			impl->countTotal++;
			impl->assetStartLoading(ass);
		}
		else
			ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->references < m);
		ass->references++;
	}

	void AssetManager::fabricate(uint32 scheme, uint32 assetName, const string &textName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(assetName != 0);
		CAGE_ASSERT(scheme < impl->schemes.size());
		CAGE_ASSERT(!impl->index.count(assetName));
		assetContextPrivateStruct *ass = detail::systemArena().createObject<assetContextPrivateStruct>();
		ass->fabricated = true;
		ass->realName = assetName;
		ass->textName = textName.subString(0, ass->textName.MaxLength - 1);
		ass->scheme = scheme;
		ass->error = false;
		ASS_LOG(2, ass, "fabricated");
		impl->index[ass->realName] = ass;
		impl->countTotal++;
		impl->assetStartLoading(ass);
		ass->references++;
	}

	void AssetManager::remove(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->references > 0);
		CAGE_ASSERT(ass->realName == assetName, "assets cannot be removed by their alias names");
		ass->references--;
		if (ass->references == 0)
			impl->assetStartRemoving(ass);
	}

	void AssetManager::reload(uint32 assetName, bool recursive)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		ASS_LOG(2, ass, "reload");
		impl->assetStartLoading(ass);
		if (recursive)
		{
			for (auto it = ass->dependencies.begin(), et = ass->dependencies.end(); it != et; it++)
				reload(*it, true);
		}
	}

	void AssetManager::zScheme(uint32 index, const AssetScheme &value, uintPtr typeSize)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(index < impl->schemes.size());
		CAGE_ASSERT(value.threadIndex < impl->queueCustomLoad.size());
		(AssetScheme&)impl->schemes[index] = value;
		impl->schemes[index].typeSize = typeSize;
	}

	uintPtr AssetManager::zGetTypeSize(uint32 scheme) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(scheme < impl->schemes.size(), scheme, impl->schemes.size());
		return impl->schemes[scheme].typeSize;
	}

	void *AssetManager::zGet(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->ready);
		return ass->returnData;
	}

	void AssetManager::zSet(uint32 assetName, void *value)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->fabricated);
		ass->returnData = value;
		ass->ready = !!value;
	}

	AssetManagerCreateConfig::AssetManagerCreateConfig() : assetsFolderName("assets.zip"), threadMaxCount(5), schemeMaxCount(50)
	{}

	Holder<AssetManager> newAssetManager(const AssetManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<AssetManager, assetManagerImpl>(config);
	}

	AssetHeader initializeAssetHeader(const string &name_, uint16 schemeIndex)
	{
		AssetHeader a;
		detail::memset(&a, 0, sizeof(AssetHeader));
		detail::memcpy(a.cageName, "cageAss", 7);
		a.version = currentAssetVersion;
		string name = name_;
		static const uint32 maxTexName = sizeof(a.textName);
		if (name.length() >= maxTexName)
			name = string() + ".." + name.subString(name.length() - maxTexName - 3, maxTexName - 3);
		CAGE_ASSERT(name.length() < sizeof(a.textName), name);
		detail::memcpy(a.textName, name.c_str(), name.length());
		a.scheme = schemeIndex;
		return a;
	}
}
