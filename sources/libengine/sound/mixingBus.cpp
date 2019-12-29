#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>

namespace cage
{
	namespace
	{
		class soundFilterImpl : public MixingFilter
		{
		public:
			MixingBus *filterBus;

			soundFilterImpl(SoundContext *context) : filterBus(nullptr)
			{}

			~soundFilterImpl()
			{
				setBus(nullptr);
			}

			void busDestroyed(MixingBus *bus)
			{
				CAGE_ASSERT(filterBus == bus);
				filterBus = nullptr;
			}
		};

		class soundBusImpl : public MixingBus, public busInterfaceStruct
		{
		public:
		    typedef std::set<const busInterfaceStruct*, std::less<const busInterfaceStruct*>, MemoryArenaStd<const busInterfaceStruct *>> busInteerfaceSet;
		    typedef std::set<soundFilterImpl*, std::less<soundFilterImpl*>, MemoryArenaStd<soundFilterImpl *>> filterSet;
			busInteerfaceSet inputs;
			busInteerfaceSet outputs;
			filterSet filters;
			MixingFilterApi filterApi;
			filterSet::iterator currentFilter;

			soundBusImpl(SoundContext *context) :
				busInterfaceStruct(Delegate<void(MixingBus*)>().bind<soundBusImpl, &soundBusImpl::busDestroyed>(this), Delegate<void(const SoundDataBuffer&)>().bind<soundBusImpl, &soundBusImpl::execute>(this)),
				inputs(linksArenaFromContext(context)),
				outputs(linksArenaFromContext(context)),
				filters(linksArenaFromContext(context))
			{
				filterApi.input.bind<soundBusImpl, &soundBusImpl::filterInput>(this);
			}

			~soundBusImpl()
			{
				clear();
			}

			void busDestroyed(MixingBus *bus)
			{
				inputs.erase((soundBusImpl*)bus);
				outputs.erase((soundBusImpl*)bus);
			}

			void filterInput(const SoundDataBuffer &buf)
			{
				CAGE_ASSERT(currentFilter != filters.end());
				CAGE_ASSERT(buf.channels <= 8);
				currentFilter++;
				if (currentFilter == filters.end())
				{
					for (auto it = inputs.begin(), et = inputs.end(); it != et; it++)
						(*it)->busExecuteDelegate(buf);
				}
				else
				{
					// provide data from subsequent filter
					MixingFilterApi api;
					api.output = buf;
					api.input.bind<soundBusImpl, &soundBusImpl::filterInput>(this);
					(*currentFilter)->execute(api);
				}
			}

			void execute(const SoundDataBuffer &buf)
			{
				if (inputs.empty())
					return; // shortcut

				CAGE_ASSERT(buf.channels > 0 && buf.frames > 0 && buf.buffer, buf.channels, buf.frames, buf.buffer);
				CAGE_ASSERT(buf.channels <= 8);

				if (filters.empty())
				{ // shortcut, bypass all overhead associated with filters
					for (auto it = inputs.begin(), et = inputs.end(); it != et; it++)
						(*it)->busExecuteDelegate(buf);
					return;
				}

				// process first filter
				currentFilter = filters.begin();
				{
					filterApi.output.sampleRate = buf.sampleRate;
					filterApi.output.time = buf.time;
					filterApi.output.resize(buf.channels, buf.frames);
					filterApi.output.clear();
					(*currentFilter)->execute(filterApi);
				}

				// add temporary buffer to output
				for (uint32 i = 0, e = buf.channels * buf.frames; i < e; i++)
					buf.buffer[i] += filterApi.output.buffer[i];
			}
		};
	}

	namespace soundPrivat
	{
		void busAddInput(MixingBus *bus, const busInterfaceStruct *interface)
		{
			soundBusImpl *impl = (soundBusImpl*)bus;
			impl->inputs.insert(interface);
		}

		void busRemoveInput(MixingBus *bus, const busInterfaceStruct *interface)
		{
			soundBusImpl *impl = (soundBusImpl*)bus;
			impl->inputs.erase(interface);
		}

		void busAddOutput(MixingBus *bus, const busInterfaceStruct *interface)
		{
			soundBusImpl *impl = (soundBusImpl*)bus;
			impl->outputs.insert(interface);
		}

		void busRemoveOutput(MixingBus *bus, const busInterfaceStruct *interface)
		{
			soundBusImpl *impl = (soundBusImpl*)bus;
			impl->outputs.erase(interface);
		}
	}

	void MixingFilter::setBus(MixingBus *bus)
	{
		soundFilterImpl *impl = (soundFilterImpl*)this;
		if (impl->filterBus)
		{
			soundBusImpl *b = (soundBusImpl*)impl->filterBus;
			b->filters.erase(impl);
			impl->filterBus = nullptr;
		}
		if (bus)
		{
			soundBusImpl *b = (soundBusImpl*)bus;
			b->filters.insert(impl);
			impl->filterBus = bus;
		}
	}

	void MixingBus::addInput(MixingBus *bus)
	{
		busAddInput(this, (soundBusImpl*)bus);
		busAddOutput(bus, (soundBusImpl*)this);
	}

	void MixingBus::removeInput(MixingBus *bus)
	{
		busRemoveInput(this, (soundBusImpl*)bus);
		busRemoveOutput(bus, (soundBusImpl*)this);
	}

	void MixingBus::addOutput(MixingBus *bus)
	{
		busAddOutput(this, (soundBusImpl*)bus);
		busAddInput(bus, (soundBusImpl*)this);
	}

	void MixingBus::removeOutput(MixingBus *bus)
	{
		busRemoveOutput(this, (soundBusImpl*)bus);
		busRemoveInput(bus, (soundBusImpl*)this);
	}

	void MixingBus::clear()
	{
		soundBusImpl *impl = (soundBusImpl*)this;
		for (auto it = impl->filters.begin(), et = impl->filters.end(); it != et; it++)
			(*it)->busDestroyed(this);
		impl->filters.clear();
		for (auto it = impl->inputs.begin(), et = impl->inputs.end(); it != et; it++)
			(*it)->busDestroyedDelegate(this);
		impl->inputs.clear();
		for (auto it = impl->outputs.begin(), et = impl->outputs.end(); it != et; it++)
			(*it)->busDestroyedDelegate(this);
		impl->outputs.clear();
	}

	Holder<MixingFilter> newMixingFilter(SoundContext *context)
	{
		return detail::systemArena().createImpl<MixingFilter, soundFilterImpl>(context);
	}

	Holder<MixingBus> newMixingBus(SoundContext *context)
	{
		return detail::systemArena().createImpl<MixingBus, soundBusImpl>(context);
	}
}
