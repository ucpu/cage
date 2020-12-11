#include <cage-core/memoryAllocators.h>

#include "private.h"

#include <set>

namespace cage
{
	namespace
	{
		class MixingFilterImpl : public MixingFilter
		{
		public:
			MixingBus *filterBus = nullptr;

			~MixingFilterImpl()
			{
				setBus(nullptr);
			}

			void busDestroyed(MixingBus *bus)
			{
				CAGE_ASSERT(filterBus == bus);
				filterBus = nullptr;
			}
		};

		class MixingBusImpl : public MixingBus, public BusInterface
		{
		public:
			typedef std::set<const BusInterface *> BusInteerfaceSet;
			typedef std::set<MixingFilterImpl *> FilterSet;
			BusInteerfaceSet inputs;
			BusInteerfaceSet outputs;
			FilterSet filters;
			MixingFilterApi filterApi;
			FilterSet::iterator currentFilter;

			explicit MixingBusImpl() : BusInterface(Delegate<void(MixingBus*)>().bind<MixingBusImpl, &MixingBusImpl::busDestroyed>(this), Delegate<void(const SoundDataBuffer&)>().bind<MixingBusImpl, &MixingBusImpl::execute>(this))
			{
				filterApi.input.bind<MixingBusImpl, &MixingBusImpl::filterInput>(this);
			}

			~MixingBusImpl()
			{
				clear();
			}

			void busDestroyed(MixingBus *bus)
			{
				inputs.erase((MixingBusImpl*)bus);
				outputs.erase((MixingBusImpl*)bus);
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
					api.input.bind<MixingBusImpl, &MixingBusImpl::filterInput>(this);
					(*currentFilter)->execute(api);
				}
			}

			void execute(const SoundDataBuffer &buf)
			{
				if (inputs.empty())
					return; // shortcut

				CAGE_ASSERT(buf.channels > 0 && buf.frames > 0 && buf.buffer);
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
		void busAddInput(MixingBus *bus, const BusInterface *interface)
		{
			MixingBusImpl *impl = (MixingBusImpl*)bus;
			impl->inputs.insert(interface);
		}

		void busRemoveInput(MixingBus *bus, const BusInterface *interface)
		{
			MixingBusImpl *impl = (MixingBusImpl*)bus;
			impl->inputs.erase(interface);
		}

		void busAddOutput(MixingBus *bus, const BusInterface *interface)
		{
			MixingBusImpl *impl = (MixingBusImpl*)bus;
			impl->outputs.insert(interface);
		}

		void busRemoveOutput(MixingBus *bus, const BusInterface *interface)
		{
			MixingBusImpl *impl = (MixingBusImpl*)bus;
			impl->outputs.erase(interface);
		}
	}

	void MixingFilter::setBus(MixingBus *bus)
	{
		MixingFilterImpl *impl = (MixingFilterImpl*)this;
		if (impl->filterBus)
		{
			MixingBusImpl *b = (MixingBusImpl*)impl->filterBus;
			b->filters.erase(impl);
			impl->filterBus = nullptr;
		}
		if (bus)
		{
			MixingBusImpl *b = (MixingBusImpl*)bus;
			b->filters.insert(impl);
			impl->filterBus = bus;
		}
	}

	void MixingBus::addInput(MixingBus *bus)
	{
		busAddInput(this, (MixingBusImpl*)bus);
		busAddOutput(bus, (MixingBusImpl*)this);
	}

	void MixingBus::removeInput(MixingBus *bus)
	{
		busRemoveInput(this, (MixingBusImpl*)bus);
		busRemoveOutput(bus, (MixingBusImpl*)this);
	}

	void MixingBus::addOutput(MixingBus *bus)
	{
		busAddOutput(this, (MixingBusImpl*)bus);
		busAddInput(bus, (MixingBusImpl*)this);
	}

	void MixingBus::removeOutput(MixingBus *bus)
	{
		busRemoveOutput(this, (MixingBusImpl*)bus);
		busRemoveInput(bus, (MixingBusImpl*)this);
	}

	void MixingBus::clear()
	{
		MixingBusImpl *impl = (MixingBusImpl*)this;
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

	Holder<MixingFilter> newMixingFilter()
	{
		return detail::systemArena().createImpl<MixingFilter, MixingFilterImpl>();
	}

	Holder<MixingBus> newMixingBus()
	{
		return detail::systemArena().createImpl<MixingBus, MixingBusImpl>();
	}
}
