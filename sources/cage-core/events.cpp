#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>

namespace cage
{
	namespace
	{
		mutexClass *eventsMutex()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentionall
			return m->get();
		}
	}

	namespace privat
	{
		eventPrivate::eventPrivate() : p(nullptr), n(nullptr)
		{}

		eventPrivate::eventPrivate(eventPrivate &other)
		{
			attach(&other);
		}

		eventPrivate::~eventPrivate()
		{
			detach();
		}

		eventPrivate &eventPrivate::operator = (eventPrivate &other)
		{
			if (&other == this)
				return *this;
			attach(&other);
			return *this;
		}

		void eventPrivate::attach(eventPrivate *d)
		{
			scopeLock<mutexClass> lck(eventsMutex());
			unlink();
			while (d->n)
				d = d->n;
			d->n = this;
			p = d;
		}

		void eventPrivate::detach()
		{
			scopeLock<mutexClass> lck(eventsMutex());
			unlink();
		}

		void eventPrivate::unlink()
		{
			if (p)
				p->n = n;
			if (n)
				n->p = p;
			p = n = nullptr;
		}
	}
}
