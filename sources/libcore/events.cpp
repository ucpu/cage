#define CAGE_EXPORT
#include <cage-core/core.h>

namespace cage
{
	namespace privat
	{
		eventLinker::eventLinker() : p(nullptr), n(nullptr), order(detail::numeric_limits<sint32>::min())
		{}

		eventLinker::eventLinker(eventLinker &other) : eventLinker()
		{
			attach(&other, other.order);
		}

		eventLinker::~eventLinker()
		{
			detach();
		}

		void eventLinker::attach(eventLinker *d, sint32 o)
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
			CAGE_ASSERT(valid(), "multiple events with same order are prohibited (unless zero)");
		}

		void eventLinker::detach()
		{
			unlink();
		}

		void eventLinker::unlink()
		{
			if (p)
				p->n = n;
			if (n)
				n->p = p;
			p = n = nullptr;
		}

		bool eventLinker::valid() const
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
