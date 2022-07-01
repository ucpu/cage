#ifdef CAGE_PROFILING_ENABLED

#include <cage-core/profiling.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/networkWebsocket.h>
#include <cage-core/process.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/math.h>

#include <atomic>
#include <string>

namespace cage
{
	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		constexpr String DefaultBrowser = "\"C:/Program Files/Mozilla Firefox/firefox.exe\" --new-window";
#else
		constexpr String DefaultBrowser = "firefox";
#endif // CAGE_SYSTEM_WINDOWS

		ConfigBool confEnabled("cage/profiling/enabled", false);
		const ConfigBool confAutoStartClient("cage/profiling/autoStartClient", true);
		const ConfigString confBrowser("cage/profiling/browser", DefaultBrowser);

		uint64 timestamp()
		{
			// ensures that all timestamps are unique
			static std::atomic<uint64> atom = 0;
			uint64 newv = applicationTime();
			uint64 oldv = atom.load();
			do
			{
				if (oldv >= newv)
					newv = oldv + 1;
			} while (!atom.compare_exchange_weak(oldv, newv));
			return newv;
		}

		struct QueueItem
		{
			String name;
			StringLiteral category;
			uint64 startTime = 0;
			uint64 endTime = 0;
			uint64 threadId = 0;
		};

		using QueueType = ConcurrentQueue<QueueItem>;

		QueueType &queue()
		{
			static QueueType *q = new QueueType(); // intentional memory leak
			return *q;
		}

		struct Dispatcher
		{
			Holder<WebsocketServer> server;
			Holder<WebsocketConnection> connection;
			Holder<Process> client;
			Holder<Thread> thread = newThread(Delegate<void()>().bind<Dispatcher, &Dispatcher::threadEntry>(this), "profiling dispatcher");

			void eraseQueue()
			{
				QueueItem qi;
				while (queue().tryPop(qi));
			}

			void updateEnabled()
			{
				if (connection)
				{
					std::string str;
					str.reserve(queue().estimatedSize() * 200);
					str += "{ \"data\": [\n";
					QueueItem qi;
					while (queue().tryPop(qi))
					{
						const String s = Stringizer() + "{ \"name\": \"" + qi.name + "\", \"category\": \"" + qi.category + "\", \"startTime\": " + qi.startTime + ", \"endTime\": " + qi.endTime + ", \"threadId\": " + qi.threadId + " },\n";
						str += s.c_str();
					}
					str += "{}\n"; // dummy element at end to satisfy commas
					str += "] }\n";
					connection->write(str);
					server.clear();
				}
				else
				{
					if (server)
					{
						connection = server->accept();
						if (connection)
							CAGE_LOG(SeverityEnum::Info, "profiling", Stringizer() + "profiling client connected: " + connection->address() + ":" + connection->port());
					}
					else
					{
						server = newWebsocketServer(randomRange(10000u, 65000u));
						CAGE_LOG(SeverityEnum::Info, "profiling", Stringizer() + "profiling server listens on port " + server->port());

						if (confAutoStartClient)
						{
							try
							{
								const String baseUrl = pathSearchTowardsRoot("profiling.htm", pathExtractDirectory(detail::executableFullPath()));
								const String url = Stringizer() + "file://" + baseUrl + "?port=" + server->port();
								ProcessCreateConfig cfg(Stringizer() + (String)confBrowser + " " + url);
								cfg.discardStdErr = cfg.discardStdIn = cfg.discardStdOut = true;
								client = newProcess(cfg);
							}
							catch (const cage::Exception &)
							{
								CAGE_LOG(SeverityEnum::Warning, "profiling", "failed to automatically launch profiling client");
							}
						}
					}
					eraseQueue();
				}
			}

			void updateDisabled()
			{
				server.clear();
				connection.clear();
				client.clear();
				eraseQueue();
			}

			void threadEntry()
			{
				while (true)
				{
					if (confEnabled)
					{
						try
						{
							updateEnabled();
						}
						catch (...)
						{
							connection.clear();
							confEnabled = false;
							CAGE_LOG(SeverityEnum::Warning, "profiling", "disabling profiling due to error");
						}
						threadSleep(30000);
					}
					else
					{
						try
						{
							updateDisabled();
						}
						catch (...)
						{
							// nothing
						}
						threadSleep(200000);
					}
				}
			}
		} dispatcher;
	}

	void profilingThreadName(const String &name)
	{
		// todo
	}

	ProfilingEvent profilingEventBegin(const String &name, StringLiteral category) noexcept
	{
		ProfilingEvent ev;
		ev.name = name;
		ev.category = category;
		ev.startTime = timestamp();
		return ev;
	}

	void profilingEventEnd(ProfilingEvent &ev) noexcept
	{
		if (ev.startTime == m)
			return;
		if (!confEnabled)
			return;
		QueueItem qi;
		qi.name = ev.name;
		qi.category = ev.category;
		qi.startTime = ev.startTime;
		qi.endTime = timestamp();
		qi.threadId = currentThreadId();
		try
		{
			queue().push(qi);
		}
		catch (...)
		{
			// nothing
		}
		ev.startTime = m;
	}

	void profilingMarker(const String &name, StringLiteral category) noexcept
	{
		if (!confEnabled)
			return;
		QueueItem qi;
		qi.name = name;
		qi.category = category;
		qi.startTime = qi.endTime = timestamp();
		qi.threadId = currentThreadId();
		try
		{
			queue().push(qi);
		}
		catch (...)
		{
			// nothing
		}
	}

	ProfilingScope::ProfilingScope() noexcept
	{}

	ProfilingScope::ProfilingScope(const String &name, StringLiteral category) noexcept
	{
		event = profilingEventBegin(name, category);
	}

	ProfilingScope::ProfilingScope(ProfilingScope &&other) noexcept
	{
		*this = std::move(other);
	}

	ProfilingScope &ProfilingScope::operator = (ProfilingScope &&other) noexcept
	{
		profilingEventEnd(event);
		event = {};
		std::swap(event, other.event);
		return *this;
	}

	ProfilingScope::~ProfilingScope() noexcept
	{
		profilingEventEnd(event);
	}
}

#endif
