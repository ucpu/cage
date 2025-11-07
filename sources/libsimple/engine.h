#ifndef header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu
#define header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu

#include <cage-core/concurrent.h>
#include <cage-core/entities.h>
#include <cage-simple/engine.h>

namespace cage
{
	class GuiRender;
	class Texture;

	class EnginePrivateGraphics : private Immovable
	{
	public:
		// control thread
		void emit(uint64 time);
		Holder<Image> screenshot();

		// graphics thread
		void initialize();
		void finalize();
		void dispatch(uint64 time, Holder<GuiRender> guiBundle);

		// statistics
		uint64 gpuTime = 0;
		uint32 drawCalls = 0;
		uint32 drawPrimitives = 0;
		Real dynamicResolution = 1;
	};

	Holder<EnginePrivateGraphics> newEnginePrivateGraphics(const EngineCreateConfig &config);

	class EnginePrivateSound : private Immovable
	{
	public:
		// control thread
		void emit();

		// sound thread
		void initialize();
		void finalize();
		void dispatch(uint64 time);
	};

	Holder<EnginePrivateSound> newEnginePrivateSound(const EngineCreateConfig &config);

	template<class T>
	struct ExclusiveHolder : private Immovable
	{
		void assign(Holder<T> &&value)
		{
			ScopeLock lock(mut);
			data = std::move(value);
		}

		Holder<T> get() const
		{
			ScopeLock lock(mut);
			if (data)
				return data.share();
			return {};
		}

		void clear()
		{
			Holder<T> tmp;
			{
				ScopeLock lock(mut);
				std::swap(tmp, data); // swap under lock
			}
			tmp.clear(); // clear outside lock
		}

	private:
		Holder<T> data;
		Holder<Mutex> mut = newMutex();
	};
}

#endif
