#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

namespace cage
{
	namespace
	{
		mutexClass *eventsMutex()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentional
			return m->get();
		}
	}

	namespace privat
	{
		eventLinker::eventLinker() : p(nullptr), n(nullptr), order(0)
		{}

		eventLinker::eventLinker(eventLinker &other)
		{
			attach(&other, other.order);
		}

		eventLinker::~eventLinker()
		{
			detach();
		}

		eventLinker &eventLinker::operator = (eventLinker &other)
		{
			if (&other == this)
				return *this;
			attach(&other, other.order);
			return *this;
		}

		void eventLinker::attach(eventLinker *d, sint32 o)
		{
			scopeLock<mutexClass> lck(eventsMutex());
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
		}

		void eventLinker::detach()
		{
			scopeLock<mutexClass> lck(eventsMutex());
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
	}
}
