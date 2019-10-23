#include <set>
#include <map>
#include <vector>
#include <atomic>
#include <robin_hood.h>

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

#include <optick.h>

namespace cage
{
	configUint32 logLevel("cage-core.assetManager.logLevel", 0);
#define ASS_LOG(LEVEL, ASS, MSG) { if (logLevel >= (LEVEL)) { CAGE_LOG(severityEnum::Info, "assetManager", string() + "asset '" + (ASS)->textName + "' (" + (ASS)->realName + " / " + (ASS)->internationalName + "): " + MSG); } }

	namespace
	{
		static const uint32 currentAssetVersion = 1;

		void defaultDecompress(const assetContext *context)
		{
			if (context->compressedSize == 0)
				return;
			uintPtr res = detail::decompress(context->compressedData, numeric_cast<uintPtr>(context->compressedSize), context->originalData, numeric_cast<uintPtr>(context->originalSize));
			CAGE_ASSERT(res == context->originalSize, res, context->originalSize);
		}

		void defaultDone(const assetContext *context)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}

		struct assetContextPrivateStruct : public assetContext
		{
			std::vector<uint32> dependencies;
			std::vector<uint32> dependenciesNew;
			uint32 internationalizedPrevious;
			uint32 scheme;
			uint32 references;
			std::atomic<bool> processing, ready, error, dependenciesDoneFlag, fabricated;
			assetContextPrivateStruct() : internationalizedPrevious(0), scheme(m), references(0), processing(false), ready(false), error(true), dependenciesDoneFlag(false), fabricated(false) {}
		};

		struct assetSchemePrivateStruct : public assetScheme
		{
			uintPtr typeSize;
			assetSchemePrivateStruct() : typeSize(0) {}
		};

		void destroy(assetContextPrivateStruct *ptr)
		{
			// these pointers are owned by the hash map, not by the concurrent queues
			ptr->error = true;
		}

		class assetManagerImpl : public assetManager
		{
		public:
			std::map<uint32, std::set<assetContextPrivateStruct*>> interNames;
			std::vector<assetSchemePrivateStruct> schemes;
			robin_hood::unordered_map<uint32, assetContextPrivateStruct*> index;
			holder<tcpConnection> listener;
			holder<threadHandle> loadingThread;
			holder<threadHandle> decompressionThread;

			std::vector<holder<concurrentQueue<assetContextPrivateStruct*>>> queueCustomLoad, queueCustomDone;
			holder<concurrentQueue<assetContextPrivateStruct*>> queueLoadFile, queueDecompression;
			holder<concurrentQueue<assetContextPrivateStruct*>> queueAddDependencies, queueWaitDependencies, queueRemoveDependencies;

			string path;
			uint32 countTotal, countProcessing;
			uint32 hackQueueWaitCounter; // it is desirable to process as many items from the queue in single tick, however, since the items are being pushed back to the same queue, some form of termination mechanism must be provided
			uint32 generateName;
			std::atomic<bool> destroying;

			holder<concurrentQueue<assetContextPrivateStruct*>> createQueue()
			{
				return newConcurrentQueue<assetContextPrivateStruct*>({}, delegate<void(assetContextPrivateStruct*)>().bind<&destroy>());
			}

			assetManagerImpl(const assetManagerCreateConfig &config) : countTotal(0), countProcessing(0), hackQueueWaitCounter(0), generateName(0), destroying(false)
			{
				try
				{
					try
					{
						detail::overrideException oe;
						path = pathSearchTowardsRoot(config.assetsFolderName, pathWorkingDir(), pathTypeFlags::Directory | pathTypeFlags::Archive);
					}
					catch(const exception &)
					{
						path = pathSearchTowardsRoot(config.assetsFolderName, pathExtractPath(detail::getExecutableFullPath()), pathTypeFlags::Directory | pathTypeFlags::Archive);
					}
				}
				catch (const exception &)
				{
					CAGE_LOG(severityEnum::Error, "assetManager", "failed to find the directory with assets");
					throw;
				}
				CAGE_LOG(severityEnum::Info, "assetManager", string() + "using asset path: '" + path + "'");
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
				loadingThread = newThread(delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::loadingThreadEntry>(this), "asset disk io");
				decompressionThread = newThread(delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::decompressionThreadEntry>(this), "asset decompression");
			}

			~assetManagerImpl()
			{
				CAGE_ASSERT(countTotal == 0, countTotal);
				CAGE_ASSERT(countProcessing == 0, countProcessing);
				CAGE_ASSERT(index.size() == 0, index.size());
				CAGE_ASSERT(interNames.size() == 0, interNames.size());
				destroying = true;
				queueLoadFile->push(nullptr);
				queueDecompression->push(nullptr);
				loadingThread->wait();
				decompressionThread->wait();
			}

			memoryBuffer findAsset(uint32 name)
			{
				memoryBuffer buff;
				if (findAssetBuffer.dispatch(name, buff))
					return buff;
				string pth;
				if (!findAssetPath.dispatch(name, pth))
					pth = pathJoin(path, name);
				holder<fileHandle> f = newFile(pth, fileMode(true, false));
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
					CAGE_ASSERT(ass->internationalizedPrevious == 0);
					OPTICK_TAG("realName", ass->realName);
					ass->internationalizedPrevious = ass->internationalName;
					ass->scheme = m;
					ass->assetFlags = 0;
					ass->compressedSize = ass->originalSize = 0;
					ass->internationalName = 0;
					ass->textName = string() + "<" + ass->realName + ">";
					ass->dependenciesNew.clear();
					ass->compressedData = ass->originalData = nullptr;
					try
					{
						detail::overrideBreakpoint overrideBreakpoint;
						memoryBuffer buff = findAsset(ass->realName);
						if (buff.size() < sizeof(assetHeader))
							CAGE_THROW_ERROR(exception, "asset is missing required header");
						assetHeader *h = (assetHeader*)buff.data();
						if (detail::memcmp(h->cageName, "cageAss", 8) != 0)
							CAGE_THROW_ERROR(exception, "file is not a cage asset");
						if (h->version != currentAssetVersion)
							CAGE_THROW_ERROR(exception, "cage asset version mismatch");
						if (h->textName[sizeof(h->textName) - 1] != 0)
							CAGE_THROW_ERROR(exception, "cage asset text name not bounded");
						if (h->scheme >= schemes.size())
							CAGE_THROW_ERROR(exception, "cage asset scheme out of range");
						if (ass->scheme != m && ass->scheme != h->scheme)
							CAGE_THROW_ERROR(exception, "cage asset scheme cannot change");
						if (buff.size() < sizeof(assetHeader) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(exception, "cage asset file dependencies truncated");
						ass->scheme = h->scheme;
						ass->assetFlags = h->flags;
						ass->compressedSize = h->compressedSize;
						ass->originalSize = h->originalSize;
						ass->internationalName = h->internationalName;
						//ass->textName = detail::stringBase<sizeof(h->textName)>(h->textName);
						ass->textName = h->textName;
						OPTICK_TAG("textName", ass->textName.c_str());
						ass->dependenciesNew.resize(h->dependenciesCount);
						if (h->dependenciesCount)
							detail::memcpy(ass->dependenciesNew.data(), buff.data() + sizeof(assetHeader), h->dependenciesCount * sizeof(uint32));
						uintPtr allocSize = numeric_cast<uintPtr>(ass->compressedSize + ass->originalSize);
						uintPtr readSize = numeric_cast<uintPtr>(h->compressedSize == 0 ? h->originalSize : h->compressedSize);
						CAGE_ASSERT(readSize <= allocSize, readSize, allocSize);
						if (buff.size() < readSize + sizeof(assetHeader) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(exception, "cage asset file content truncated");
						ass->compressedData = detail::systemArena().allocate(allocSize, sizeof(uintPtr));
						detail::memcpy(ass->compressedData, buff.data() + sizeof(assetHeader) + h->dependenciesCount * sizeof(uint32), readSize);
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
							interNameClear(ass->internationalName, ass);
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
								case assetStateEnum::Ready:
									break;
								case assetStateEnum::Error:
									ass->error = true;
									break;
								case assetStateEnum::Unknown:
									wait = true;
									break;
								default:
									CAGE_THROW_CRITICAL(exception, "invalid (or impossible) asset state");
								}
							}
						}
						if (wait)
						{
							queueWaitDependencies->push(ass);
							return (hackQueueWaitCounter++ % queueWaitDependencies->estimatedSize()) != 0;
						}
						if (ass->internationalName != ass->internationalizedPrevious)
						{
							interNameSet(ass->internationalName, ass);
							interNameClear(ass->internationalizedPrevious, ass);
						}
						ass->internationalizedPrevious = 0;
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
							CAGE_LOG(severityEnum::Warning, "assetManager", string() + "asset: '" + ass->textName + "', loading failed");
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
							uint32 name = hashString(line.c_str());
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
				auto &s = interNames[interName];
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
				CAGE_ASSERT(interNames.find(interName) != interNames.end());
				auto &s = interNames[interName];
				CAGE_ASSERT(s.find(ass) != s.end());
				s.erase(ass);
				index.erase(interName);
				if (s.empty())
					interNames.erase(interName);
				else
					index[interName] = *s.begin();
			}

			uint32 generateUniqueName()
			{
				static const uint32 a = (uint32)1 << 28;
				static const uint32 b = (uint32)1 << 30;
				if (generateName < a || generateName > b)
					generateName = a;
				while (state(generateName) != assetStateEnum::NotFound)
					generateName = generateName == b ? a : generateName + 1;
				return generateName++;
			}
		};
	}

	void assetManager::listen(const string &address, uint16 port)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		impl->listener = newTcpConnection(address, port);
	}

	uint32 assetManager::dependenciesCount(uint32 assetName) const
	{
		CAGE_ASSERT(ready(assetName), assetName);
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		return numeric_cast<uint32>(ass->dependencies.size());
	}

	uint32 assetManager::dependencyName(uint32 assetName, uint32 index) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(index < ass->dependencies.size(), index, ass->dependencies.size(), assetName);
		return ass->dependencies[index];
	}

	pointerRange<const uint32> assetManager::dependencies(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		return ass->dependencies;
	}

	uint32 assetManager::countTotal() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countTotal;
	}

	uint32 assetManager::countProcessing() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countProcessing;
	}

	bool assetManager::processControlThread()
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processControlThread();
	}

	bool assetManager::processCustomThread(uint32 threadIndex)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processCustomThread(threadIndex);
	}

	assetStateEnum assetManager::state(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		auto it = impl->index.find(assetName);
		if (it == impl->index.end())
			return assetStateEnum::NotFound;
		assetContextPrivateStruct *ass = it->second;
		if (ass->error)
			return assetStateEnum::Error;
		if (ass->ready)
			return assetStateEnum::Ready;
		return assetStateEnum::Unknown;
	}

	bool assetManager::ready(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		switch (state(assetName))
		{
		case assetStateEnum::Ready:
			return true;
		case assetStateEnum::NotFound:
			return false;
		case assetStateEnum::Error:
		{
			assetContextPrivateStruct *ass = impl->index.at(assetName);
			CAGE_LOG(severityEnum::Note, "exception", string() + "asset real name: " + assetName);
			CAGE_LOG(severityEnum::Note, "exception", string() + "asset text name: '" + ass->textName + "'");
			CAGE_THROW_ERROR(exception, "asset has failed to load");
		}
		case assetStateEnum::Unknown:
			return false;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid asset state enum");
		}
	}

	uint32 assetManager::scheme(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		auto it = impl->index.find(assetName);
		if (it == impl->index.end())
			return m;
		return it->second->scheme;
	}

	uint32 assetManager::generateUniqueName()
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->generateUniqueName();
	}

	void assetManager::add(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(assetName != 0);
		assetContextPrivateStruct *ass = nullptr;
		if (!impl->index.count(assetName))
		{
			ass = detail::systemArena().createObject<assetContextPrivateStruct>();
			ass->realName = assetName;
			ass->textName = string() + "<" + assetName + ">";
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

	void assetManager::fabricate(uint32 scheme, uint32 assetName, const string &textName)
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

	void assetManager::remove(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->references > 0);
		CAGE_ASSERT(ass->realName == assetName, "assets cannot be removed by their internationalized names");
		ass->references--;
		if (ass->references == 0)
			impl->assetStartRemoving(ass);
	}

	void assetManager::reload(uint32 assetName, bool recursive)
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

	void assetManager::zScheme(uint32 index, const assetScheme &value, uintPtr typeSize)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(index < impl->schemes.size());
		CAGE_ASSERT(value.threadIndex < impl->queueCustomLoad.size());
		(assetScheme&)impl->schemes[index] = value;
		impl->schemes[index].typeSize = typeSize;
	}

	uintPtr assetManager::zGetTypeSize(uint32 scheme) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT(scheme < impl->schemes.size(), scheme, impl->schemes.size());
		return impl->schemes[scheme].typeSize;
	}

	void *assetManager::zGet(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->ready);
		return ass->returnData;
	}

	void assetManager::zSet(uint32 assetName, void *value)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index.at(assetName);
		CAGE_ASSERT(ass->fabricated);
		ass->returnData = value;
		ass->ready = !!value;
	}

	assetManagerCreateConfig::assetManagerCreateConfig() : assetsFolderName("assets.zip"), threadMaxCount(5), schemeMaxCount(50)
	{}

	holder<assetManager> newAssetManager(const assetManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<assetManager, assetManagerImpl>(config);
	}

	assetHeader initializeAssetHeader(const string &name_, uint16 schemeIndex)
	{
		assetHeader a;
		detail::memset(&a, 0, sizeof(assetHeader));
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
