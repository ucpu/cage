#include <cage-core/events.h>

namespace cage
{
	namespace privat
	{
		EventLinker::EventLinker(const string &name) : order(detail::numeric_limits<sint32>::min()), name(name)
		{}

		EventLinker::~EventLinker()
		{
			detach();
		}

		void EventLinker::attach(EventLinker *d, sint32 o)
		{
			unlink();
			order = o;
			// find rightmost node
			while (d->n)
				d = d->n;
			// find the node after which we belong
			while (d->p && d->p->order > o)
				d = d->p;
			if (d->order > o)
			{ // attach before d
				p = d->p;
				n = d;
				if (p)
					p->n = this;
				n->p = this;
			}
			else
			{ // attach after d
				n = d->n;
				p = d;
				if (n)
					n->p = this;
				p->n = this;
			}
			CAGE_ASSERT(valid());
		}

		void EventLinker::detach()
		{
			unlink();
		}

		void EventLinker::logAllNames()
		{
			EventLinker *l = this;
			while (l->p)
				l = l->p;
			while (l)
			{
				CAGE_LOG(SeverityEnum::Info, "event-listener", stringizer() + l->name + " (" + l->order + ")");
				l = l->n;
			}
		}

		void EventLinker::unlink()
		{
			if (p)
				p->n = n;
			if (n)
				n->p = p;
			p = n = nullptr;
		}

		bool EventLinker::valid() const
		{
			if (order == 0)
				return true;
			if (p && p->order >= order)
				return false;
			if (n && n->order <= order)
				return false;
			return true;
		}
	}
}
