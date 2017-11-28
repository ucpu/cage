#include <set>
#include <map>
#include <vector>
#include <atomic>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashTable.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/threadSafeQueue.h>

namespace cage
{
	assetContextStruct::assetContextStruct() : compressedSize(0), originalSize(0), assetPointer(nullptr), returnData(nullptr), compressedData(nullptr), originalData(nullptr), realName(0), internationalizedName(0), assetFlags(0) {}
	assetSchemeStruct::assetSchemeStruct() : schemePointer(nullptr), threadIndex(0) {}

	namespace
	{
		static const uint32 currentAssetVersion = 1;

		struct assetContextPrivateStruct : public assetContextStruct
		{
			std::vector<uint32> dependencies;
			std::vector<uint32> dependenciesNew;
			uint32 internationalizedPrevious;
			uint32 scheme;
			uint32 references;
			std::atomic<bool> processing, ready, error, dependenciesDoneFlag, fabricated;
			assetContextPrivateStruct() : internationalizedPrevious(0), scheme(-1), references(0), processing(false), ready(false), error(true), dependenciesDoneFlag(false), fabricated(false) {}
		};

		struct assetSchemePrivateStruct : public assetSchemeStruct
		{
			uintPtr typeSize;
			assetSchemePrivateStruct() : typeSize(0) {}
		};

		class assetManagerImpl : public assetManagerClass
		{
		public:
			std::map<uint32, std::set<assetContextPrivateStruct*>> interNames;
			std::vector<assetSchemePrivateStruct> schemes;
			holder<hashTableClass<assetContextPrivateStruct>> index;
			holder<tcpConnectionClass> listener;
			holder<threadClass> loadingThread;
			holder<threadClass> decompressionThread;

			std::vector<holder<threadSafeQueueClass>> queueCustomLoad, queueCustomDone;
			holder<threadSafeQueueClass> queueLoadFile, queueDecompression;
			holder<threadSafeQueueClass> queueAddDependencies, queueWaitDependencies, queueRemoveDependencies;

			string path;
			uint32 countTotal, countProcessing;
			volatile bool destroying;

			assetManagerImpl(const assetManagerCreateConfig &config) : countTotal(0), countProcessing(0), destroying(false)
			{
				path = pathFind(config.path);
				CAGE_LOG(severityEnum::Info, "assetManager", string() + "using asset path: '" + path + "'");
				index = newHashTable<assetContextPrivateStruct>(1000, 1000000);
				schemes.resize(config.schemeMaxCount);
				queueCustomLoad.reserve(config.threadMaxCount);
				queueCustomDone.reserve(config.threadMaxCount);
				for (uint32 i = 0; i < config.threadMaxCount; i++)
				{
					queueCustomLoad.push_back(newThreadSafeQueue(config.queuesMemory));
					queueCustomDone.push_back(newThreadSafeQueue(config.queuesMemory));
				}
				queueLoadFile = newThreadSafeQueue(config.queuesMemory);
				queueDecompression = newThreadSafeQueue(config.queuesMemory);
				queueAddDependencies = newThreadSafeQueue(config.queuesMemory);
				queueWaitDependencies = newThreadSafeQueue(config.queuesMemory);
				queueRemoveDependencies = newThreadSafeQueue(config.queuesMemory);
				loadingThread = newThread(delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::loadingThreadEntry>(this), "asset disk io");
				decompressionThread = newThread(delegate<void()>().bind<assetManagerImpl, &assetManagerImpl::decompressionThreadEntry>(this), "asset decompression");
			}

			~assetManagerImpl()
			{
				CAGE_ASSERT_RUNTIME(countTotal == 0, countTotal);
				CAGE_ASSERT_RUNTIME(countProcessing == 0, countProcessing);
				CAGE_ASSERT_RUNTIME(index->count() == 0, index->count());
				CAGE_ASSERT_RUNTIME(interNames.size() == 0, interNames.size());
				destroying = true;
				loadingThread->wait();
			}

			memoryBuffer findAsset(uint32 name)
			{
				memoryBuffer buff;
				if (findAssetBuffer.dispatch(name, buff))
					return buff;
				string pth;
				if (!findAssetPath.dispatch(name, pth))
					pth = pathJoin(path, name);
				holder<fileClass> f = newFile(pth, fileMode(true, false));
				auto s = f->size();
				buff.reallocate(s);
				f->read(buff.data(), s);
				return buff;
			}

			const bool processLoadingThread()
			{
				assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueLoadFile->pull();
				if (ass)
				{
					CAGE_ASSERT_RUNTIME(ass->processing);
					CAGE_ASSERT_RUNTIME(!ass->fabricated);
					CAGE_ASSERT_RUNTIME(ass->originalData == nullptr);
					CAGE_ASSERT_RUNTIME(ass->compressedData == nullptr);
					CAGE_ASSERT_RUNTIME(ass->internationalizedPrevious == 0);
					ass->internationalizedPrevious = ass->internationalizedName;
					ass->scheme = -1;
					ass->assetFlags = 0;
					ass->compressedSize = ass->originalSize = 0;
					ass->internationalizedName = 0;
					ass->textName = string() + "<" + ass->realName + ">";
					ass->dependenciesNew.clear();
					ass->compressedData = ass->originalData = nullptr;
					try
					{
						detail::overrideBreakpoint overrideBreakpoint;
						memoryBuffer buff = findAsset(ass->realName);
						if (buff.size() < sizeof(assetHeaderStruct))
							CAGE_THROW_ERROR(exception, "asset is missing required header");
						assetHeaderStruct *h = (assetHeaderStruct*)buff.data();
						if (detail::memcmp(h->cageName, "cageAss", 8) != 0)
							CAGE_THROW_ERROR(exception, "file is not a cage asset");
						if (h->version != currentAssetVersion)
							CAGE_THROW_ERROR(exception, "cage asset version mismatch");
						if (h->textName[sizeof(h->textName) - 1] != 0)
							CAGE_THROW_ERROR(exception, "cage asset text name not bounded");
						if (h->scheme >= schemes.size())
							CAGE_THROW_ERROR(exception, "cage asset scheme out of range");
						if (ass->scheme != -1 && ass->scheme != h->scheme)
							CAGE_THROW_ERROR(exception, "cage asset scheme cannot change");
						if (buff.size() < sizeof(assetHeaderStruct) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(exception, "cage asset file dependencies truncated");
						ass->scheme = h->scheme;
						ass->assetFlags = h->flags;
						ass->compressedSize = h->compressedSize;
						ass->originalSize = h->originalSize;
						ass->internationalizedName = h->internationalizedName;
						ass->textName = detail::stringBase<sizeof(h->textName)>(h->textName);
						ass->dependenciesNew.resize(h->dependenciesCount);
						if (h->dependenciesCount)
							detail::memcpy(ass->dependenciesNew.data(), (char*)buff.data() + sizeof(assetHeaderStruct), h->dependenciesCount * sizeof(uint32));
						uintPtr allocSize = ass->compressedSize + ass->originalSize;
						uintPtr readSize = h->compressedSize == 0 ? h->originalSize : h->compressedSize;
						CAGE_ASSERT_RUNTIME(readSize <= allocSize, readSize, allocSize);
						if (buff.size() < readSize + sizeof(assetHeaderStruct) + h->dependenciesCount * sizeof(uint32))
							CAGE_THROW_ERROR(exception, "cage asset file content truncated");
						ass->compressedData = detail::systemArena().allocate(allocSize);
						detail::memcpy(ass->compressedData, (char*)buff.data() + sizeof(assetHeaderStruct) + h->dependenciesCount * sizeof(uint32), readSize);
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
							CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
							queueCustomLoad[schemes[ass->scheme].threadIndex]->push(ass);
						}
						queueAddDependencies->push(ass);
					}
					return true;
				}

				return false;
			}

			const bool processDecompressionThread()
			{
				assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueDecompression->pull();
				if (ass)
				{
					CAGE_ASSERT_RUNTIME(ass->processing);
					CAGE_ASSERT_RUNTIME(!ass->fabricated);
					if (!ass->error)
					{
						try
						{
							CAGE_ASSERT_RUNTIME(ass->compressedData);
							CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
							if (schemes[ass->scheme].decompress)
								schemes[ass->scheme].decompress(ass, schemes[ass->scheme].schemePointer);
						}
						catch (...)
						{
							ass->error = true;
						}
					}
					CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
					queueCustomLoad[schemes[ass->scheme].threadIndex]->push(ass);
					return true;
				}

				return false;
			}

			const bool processCustomThread(uint32 threadIndex)
			{
				CAGE_ASSERT_RUNTIME(threadIndex < queueCustomDone.size(), threadIndex, queueCustomDone.size());

				{ // done
					assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueCustomDone[threadIndex]->pull();
					if (ass)
					{
						CAGE_ASSERT_RUNTIME(ass->processing);
						CAGE_ASSERT_RUNTIME(!ass->fabricated);
						try
						{
							CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
							if (schemes[ass->scheme].done)
								schemes[ass->scheme].done(ass, schemes[ass->scheme].schemePointer);
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
					assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueCustomLoad[threadIndex]->pull();
					if (ass)
					{
						CAGE_ASSERT_RUNTIME(ass->processing);
						CAGE_ASSERT_RUNTIME(!ass->fabricated);
						if (!ass->error)
						{
							try
							{
								CAGE_ASSERT_RUNTIME(ass->originalData);
								CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
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

			const bool processControlThread()
			{
				{ // remove
					assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueRemoveDependencies->pull();
					if (ass)
					{
						CAGE_ASSERT_RUNTIME(ass->processing);
						for (auto it = ass->dependencies.begin(), et = ass->dependencies.end(); it != et; it++)
							remove(*it);
						ass->dependencies.clear();
						ass->processing = false;
						CAGE_ASSERT_RUNTIME(countProcessing > 0);
						countProcessing--;
						if (ass->references > 0)
							assetStartLoading(ass);
						else
						{
							interNameClear(ass->internationalizedName, ass);
							index->remove(ass->realName);
							detail::systemArena().destroy<assetContextPrivateStruct>(ass);
							CAGE_ASSERT_RUNTIME(countTotal > 0);
							countTotal--;
						}
						return true;
					}
				}

				{ // add
					assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueAddDependencies->pull();
					if (ass)
					{
						CAGE_ASSERT_RUNTIME(ass->processing);
						CAGE_ASSERT_RUNTIME(!ass->fabricated);
						for (auto it = ass->dependenciesNew.begin(), et = ass->dependenciesNew.end(); it != et; it++)
							add(*it);
						for (auto it = ass->dependencies.begin(), et = ass->dependencies.end(); it != et; it++)
							remove(*it);
						std::vector<uint32>().swap(ass->dependencies);
						std::swap(ass->dependenciesNew, ass->dependencies);
						ass->dependenciesDoneFlag = true;
						return true;
					}
				}

				{ // wait
					assetContextPrivateStruct *ass = (assetContextPrivateStruct*)queueWaitDependencies->pull();
					if (ass)
					{
						CAGE_ASSERT_RUNTIME(ass->processing);
						CAGE_ASSERT_RUNTIME(!ass->fabricated);
						bool wait = !ass->dependenciesDoneFlag;
						if (!wait)
						{
							for (auto it = ass->dependencies.begin(), et = ass->dependencies.end(); it != et; it++)
							{
								switch (state(*it))
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
							return false;
						}
						if (ass->internationalizedName != ass->internationalizedPrevious)
						{
							interNameSet(ass->internationalizedName, ass);
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
						CAGE_ASSERT_RUNTIME(countProcessing > 0);
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
					try
					{
						uint32 cnt = 0;
						string line;
						while (listener->availableLine() && listener->readLine(line))
						{
							uint32 name = hashString(line.c_str());
							CAGE_ASSERT_RUNTIME(name != 0);
							assetContextPrivateStruct *ass = index->get(name, true);
							if (ass && ass->references > 0)
							{
								CAGE_LOG(severityEnum::Info, "asset", string() + "asset '" + line + "' hot-reloading");
								reload(name);
								cnt++;
							}
							else
								CAGE_LOG(severityEnum::Warning, "asset", string() + "asset '" + line + "' will not be hot-reloaded");
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
				while (!destroying)
				{
					while (processLoadingThread());
					threadSleep(5000);
				}
			}

			void decompressionThreadEntry()
			{
				while (!destroying)
				{
					while (processDecompressionThread());
					threadSleep(5000);
				}
			}

			void assetStartLoading(assetContextPrivateStruct *ass)
			{
				if (ass->processing || ass->fabricated)
					return;
				countProcessing++;
				ass->processing = true;
				ass->error = false;
				queueLoadFile->push(ass);
			}

			void assetStartRemoving(assetContextPrivateStruct *ass)
			{
				if (ass->processing)
					return;
				countProcessing++;
				ass->processing = true;
				ass->ready = false;
				if (ass->scheme == -1 || ass->fabricated)
					queueRemoveDependencies->push(ass);
				else
				{
					CAGE_ASSERT_RUNTIME(ass->scheme < schemes.size(), ass->scheme, schemes.size());
					queueCustomDone[schemes[ass->scheme].threadIndex]->push(ass);
				}
			}

			void interNameSet(uint32 interName, assetContextPrivateStruct *ass)
			{
				if (interName == 0)
					return;
				CAGE_ASSERT_RUNTIME(ass);
				auto &s = interNames[interName];
				CAGE_ASSERT_RUNTIME(s.find(ass) == s.end());
				s.insert(ass);
				if (!index->exists(interName))
					index->add(interName, ass);
			}

			void interNameClear(uint32 interName, assetContextPrivateStruct *ass)
			{
				if (interName == 0)
					return;
				CAGE_ASSERT_RUNTIME(ass);
				CAGE_ASSERT_RUNTIME(index->exists(interName));
				CAGE_ASSERT_RUNTIME(interNames.find(interName) != interNames.end());
				auto &s = interNames[interName];
				CAGE_ASSERT_RUNTIME(s.find(ass) != s.end());
				s.erase(ass);
				index->remove(interName);
				if (s.empty())
					interNames.erase(interName);
				else
					index->add(interName, *s.begin());
			}
		};
	}

	void assetManagerClass::listen(const string &address, uint16 port)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		impl->listener = newTcpConnection(address, port);
	}

	uint32 assetManagerClass::dependenciesCount(uint32 assetName) const
	{
		CAGE_ASSERT_RUNTIME(ready(assetName), assetName);
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		return numeric_cast<uint32>(ass->dependencies.size());
	}

	uint32 assetManagerClass::dependencyName(uint32 assetName, uint32 index) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		CAGE_ASSERT_RUNTIME(index < ass->dependencies.size(), index, ass->dependencies.size(), assetName);
		return ass->dependencies[index];
	}

	pointerRange<const uint32> assetManagerClass::dependencies(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		return { ass->dependencies.data(), ass->dependencies.data() + ass->dependencies.size() };
	}

	uint32 assetManagerClass::countTotal() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countTotal;
	}

	uint32 assetManagerClass::countProcessing() const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->countProcessing;
	}

	bool assetManagerClass::processControlThread()
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processControlThread();
	}

	bool assetManagerClass::processCustomThread(uint32 threadIndex)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		return impl->processCustomThread(threadIndex);
	}

	assetStateEnum assetManagerClass::state(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, true);
		if (ass)
		{
			if (ass->error)
				return assetStateEnum::Error;
			if (ass->ready)
				return assetStateEnum::Ready;
			return assetStateEnum::Unknown;
		}
		return assetStateEnum::NotFound;
	}

	bool assetManagerClass::ready(uint32 assetName) const
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
			assetContextPrivateStruct *ass = impl->index->get(assetName, true);
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

	uint32 assetManagerClass::scheme(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, true);
		if (ass)
			return ass->scheme;
		return -1;
	}

	void assetManagerClass::add(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT_RUNTIME(assetName != 0);
		assetContextPrivateStruct *ass = impl->index->get(assetName, true);
		if (!ass)
		{
			ass = detail::systemArena().createObject<assetContextPrivateStruct>();
			ass->realName = assetName;
			ass->textName = string() + "<" + assetName + ">";
			impl->index->add(ass->realName, ass);
			impl->countTotal++;
			impl->assetStartLoading(ass);
		}
		CAGE_ASSERT_RUNTIME(ass->references < -1);
		ass->references++;
	}

	void assetManagerClass::fabricate(uint32 scheme, uint32 assetName, const string &textName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT_RUNTIME(assetName != 0);
		CAGE_ASSERT_RUNTIME(scheme < impl->schemes.size());
		CAGE_ASSERT_RUNTIME(!impl->index->exists(assetName));
		assetContextPrivateStruct *ass = detail::systemArena().createObject<assetContextPrivateStruct>();
		ass->fabricated = true;
		ass->realName = assetName;
		ass->textName = textName.subString(0, ass->textName.MaxLength - 1);
		ass->scheme = scheme;
		ass->error = false;
		impl->index->add(ass->realName, ass);
		impl->countTotal++;
		impl->assetStartLoading(ass);
		ass->references++;
	}

	void assetManagerClass::remove(uint32 assetName)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, true);
		CAGE_ASSERT_RUNTIME(ass);
		CAGE_ASSERT_RUNTIME(ass->references > 0);
		CAGE_ASSERT_RUNTIME(ass->realName == assetName, "assets cannot be removed by their internationalized names");
		ass->references--;
		if (ass->references == 0)
			impl->assetStartRemoving(ass);
	}

	void assetManagerClass::reload(uint32 assetName, bool recursive)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		impl->assetStartLoading(ass);
		if (recursive)
		{
			for (auto it = ass->dependencies.begin(), et = ass->dependencies.end(); it != et; it++)
				reload(*it, true);
		}
	}

	void assetManagerClass::zScheme(uint32 index, const assetSchemeStruct &value, uintPtr typeSize)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT_RUNTIME(index < impl->schemes.size());
		CAGE_ASSERT_RUNTIME(value.threadIndex < impl->queueCustomLoad.size());
		(assetSchemeStruct&)impl->schemes[index] = value;
		impl->schemes[index].typeSize = typeSize;
	}

	uintPtr assetManagerClass::zGetTypeSize(uint32 scheme) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		CAGE_ASSERT_RUNTIME(scheme < impl->schemes.size(), scheme, impl->schemes.size());
		return impl->schemes[scheme].typeSize;
	}

	void *assetManagerClass::zGet(uint32 assetName) const
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		CAGE_ASSERT_RUNTIME(ass->ready);
		return ass->returnData;
	}

	void assetManagerClass::zSet(uint32 assetName, void *value)
	{
		assetManagerImpl *impl = (assetManagerImpl*)this;
		assetContextPrivateStruct *ass = impl->index->get(assetName, false);
		CAGE_ASSERT_RUNTIME(ass);
		CAGE_ASSERT_RUNTIME(ass->fabricated);
		ass->returnData = value;
		ass->ready = !!value;
	}

	assetManagerCreateConfig::assetManagerCreateConfig() : path("assets"), queuesMemory(1024 * 1024), threadMaxCount(5), schemeMaxCount(50)
	{}

	holder<assetManagerClass> newAssetManager(const assetManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl <assetManagerClass, assetManagerImpl>(config);
	}

	assetHeaderStruct initializeAssetHeader(const string &name_, uint16 schemeIndex)
	{
		assetHeaderStruct a;
		detail::memset(&a, 0, sizeof(assetHeaderStruct));
		detail::memcpy(a.cageName, "cageAss", 7);
		a.version = currentAssetVersion;
		string name = name_;
		static const uint32 maxTexName = sizeof(a.textName);
		if (name.length() >= maxTexName)
			name = string() + ".." + name.subString(name.length() - maxTexName - 3, maxTexName - 3);
		CAGE_ASSERT_RUNTIME(name.length() < sizeof(a.textName), name);
		detail::memcpy(a.textName, name.c_str(), name.length());
		a.scheme = schemeIndex;
		return a;
	}
}
