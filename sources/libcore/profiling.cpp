#ifdef CAGE_PROFILING_ENABLED

#include <cage-core/profiling.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/config.h>
#include <cage-core/files.h>

#include <atomic>

namespace cage
{
	Holder<File> realNewFile(const String &path, const FileMode &mode);
	void realTryFlushFile(File *f);

	namespace
	{
		ConfigBool cnfEnabled("cage/profiling/enabled", false);
		std::atomic<uint64> nextEventId = 1;

		using EventsQueue = ConcurrentQueue<String>;
		EventsQueue &eventsQueue()
		{
			static EventsQueue *q = new EventsQueue(); // intentional leek
			return *q;
		}

		struct Profiling : private Immovable
		{
			void writerEntry()
			{
				try
				{
					while (!stopping)
					{
						String evt;
						eventsQueue().pop(evt);
						if (evt.empty())
						{
							realTryFlushFile(+file);
						}
						else
						{
							if (!file)
							{
								const String exe = detail::executableFullPathNoExe();
								const String filename = Stringizer() + pathExtractDirectory(exe) + "/profiling/" + pathExtractFilename(exe) + "_" + currentProcessId() + ".json";
								file = realNewFile(filename, FileMode(false, true));
								file->writeLine("[");
							}
							file->writeLine(evt);
						}
					}
				}
				catch (...)
				{
					stopping = true;
				}
			}

			void tickerEntry()
			{
				try
				{
					while (!stopping)
					{
						eventsQueue().push("");
						threadSleep(20000);
					}
				}
				catch (...)
				{
					stopping = true;
				}
			}

			Profiling()
			{
				thrWriter = newThread(Delegate<void()>().bind<Profiling, &Profiling::writerEntry>(this), "profiling writer");
				thrTicker = newThread(Delegate<void()>().bind<Profiling, &Profiling::tickerEntry>(this), "profiling ticker");
			}

			~Profiling()
			{
				stopping = true;
				eventsQueue().push("");
			}

			std::atomic<bool> stopping = false;
			Holder<File> file;
			Holder<Thread> thrWriter;
			Holder<Thread> thrTicker;
		};

		void enqueueEvent(const String &data)
		{
			eventsQueue().push(Stringizer() + "{" + data + "\"pid\":0},");
		}

		void addEvent(const String &data)
		{
			enqueueEvent(data);
			static Profiling profiling; // initialize threads
		}

		CAGE_FORCE_INLINE String quote(StringLiteral s)
		{
			return Stringizer() + "\"" + (s ? String(s) : String()) + "\"";
		}
		CAGE_FORCE_INLINE String quote(const String &s)
		{
			return Stringizer() + "\"" + s + "\"";
		}
		CAGE_FORCE_INLINE String keyval(StringLiteral k, StringLiteral v)
		{
			return Stringizer() + quote(k) + ":" + quote(v) + ", ";
		}
		CAGE_FORCE_INLINE String keyval(StringLiteral k, const String &v)
		{
			return Stringizer() + quote(k) + ":" + quote(v) + ", ";
		}
		CAGE_FORCE_INLINE String keyval(StringLiteral k, uint64 v)
		{
			return Stringizer() + quote(k) + ":" + v + ", ";
		}
		CAGE_FORCE_INLINE String args(const String &json)
		{
			if (json.empty())
				return {};
			return Stringizer() + quote(StringLiteral("args")) + ": {" + json + "}, ";
		}

		uint64 timestamp()
		{
			// ensures that all timestamps within a thread are unique
			thread_local uint64 last = 0;
			const uint64 current = applicationTime();
			last = last >= current ? last + 1 : current;
			return last;
		}
	}

	ProfilingEvent profilingEventBegin(const String &name, StringLiteral category, ProfilingTypeEnum type) noexcept
	{
		if (type == ProfilingTypeEnum::Invalid)
			return {};
		try
		{
			if (!cnfEnabled)
				return {};
			ProfilingEvent ev;
			ev.name = name;
			ev.category = category;
			ev.eventId = nextEventId++;
			ev.type = type;
			Stringizer s;
			s + keyval("name", name);
			s + keyval("cat", category);
			constexpr StringLiteral phs[] = { "", "B", "b", "s", "N" };
			s + keyval("ph", phs[(int)ev.type]);
			s + keyval("ts", timestamp());
			s + keyval("tid", currentThreadId());
			s + keyval("id", ev.eventId);
			addEvent(s);
			return ev;
		}
		catch (...)
		{
			return {};
		}
	}

	void profilingEventSnapshot(const ProfilingEvent &ev, const String &jsonParams) noexcept
	{
		if (ev.type == ProfilingTypeEnum::Invalid)
			return;
		try
		{
			if (!cnfEnabled)
				return;
			Stringizer s;
			s + keyval("name", ev.name);
			s + keyval("cat", ev.category);
			constexpr StringLiteral phs[] = { "", "i", "n", "t", "O" };
			s + keyval("ph", phs[(int)ev.type]);
			s + keyval("ts", timestamp());
			s + keyval("tid", currentThreadId());
			s + keyval("id", ev.eventId);
			s + args(jsonParams);
			addEvent(s);
		}
		catch (...)
		{
			// nothing
		}
	}

	void profilingEventEnd(const ProfilingEvent &ev, const String &jsonParams) noexcept
	{
		if (ev.type == ProfilingTypeEnum::Invalid)
			return;
		try
		{
			Stringizer s;
			s + keyval("name", ev.name);
			s + keyval("cat", ev.category);
			constexpr StringLiteral phs[] = { "", "E", "e", "f", "D" };
			s + keyval("ph", phs[(int)ev.type]);
			s + keyval("ts", timestamp());
			s + keyval("tid", currentThreadId());
			s + keyval("id", ev.eventId);
			s + args(jsonParams);
			addEvent(s);
		}
		catch (...)
		{
			// nothing
		}
	}

	void profilingMarker(const String &name, StringLiteral category, bool global) noexcept
	{
		try
		{
			if (!cnfEnabled)
				return;
			Stringizer s;
			s + keyval("name", name);
			s + keyval("cat", category);
			s + keyval("ph", StringLiteral("i"));
			s + keyval("ts", timestamp());
			s + keyval("tid", currentThreadId());
			if (global)
				s + keyval("s", StringLiteral("g"));
			addEvent(s);
		}
		catch (...)
		{
			// nothing
		}
	}

	void profilingEvent(uint64 duration, const String &name, StringLiteral category, const String &jsonParams) noexcept
	{
		try
		{
			if (!cnfEnabled)
				return;
			Stringizer s;
			s + keyval("name", name);
			s + keyval("cat", category);
			s + keyval("ph", StringLiteral("X"));
			s + keyval("ts", timestamp() - duration);
			s + keyval("tid", currentThreadId());
			s + keyval("dur", duration);
			s + args(jsonParams);
			addEvent(s);
		}
		catch (...)
		{
			// nothing
		}
	}

	ProfilingScope::ProfilingScope() noexcept
	{}

	ProfilingScope::ProfilingScope(const String &name, StringLiteral category, ProfilingTypeEnum type) noexcept
	{
		event = profilingEventBegin(name, category, type);
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

	void ProfilingScope::snapshot(const String &jsonParams) noexcept
	{
		profilingEventSnapshot(event, jsonParams);
	}

	void profilingThreadName(const String &name) noexcept
	{
		try
		{
			Stringizer s;
			s + keyval("name", StringLiteral("thread_name"));
			s + keyval("ph", StringLiteral("M"));
			s + keyval("tid", currentThreadId());
			s + args(Stringizer() + "\"name\": \"" + name + "\"");
			enqueueEvent(s); // insert into the queue without initializing the profiling threads
		}
		catch (...)
		{
			// nothing
		}
	}

	void profilingThreadOrder(sint32 order) noexcept
	{
		try
		{
			Stringizer s;
			s + keyval("name", StringLiteral("thread_sort_index"));
			s + keyval("ph", StringLiteral("M"));
			s + keyval("tid", currentThreadId());
			s + args(Stringizer() + "\"sort_index\": " + order);
			enqueueEvent(s); // insert into the queue without initializing the profiling threads
		}
		catch (...)
		{
			// nothing
		}
	}
}

#endif
