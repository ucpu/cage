#include <cage-core/events.h>
#include <cage-core/string.h>

namespace cage
{
	namespace privat
	{
		EventLinker::EventLinker(const std::source_location location) : location(location), order(std::numeric_limits<sint32>::min())
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

		Stringizer &operator + (Stringizer &s, const std::source_location &l)
		{
			String n = l.file_name();
#ifdef CAGE_SYSTEM_WINDOWS
			n = replace(n, "\\", "/");
#endif // CAGE_SYSTEM_WINDOWS

			// extract just one folder and file name
			n = reverse(n);
			String a = split(n, "/");
			if (!n.empty())
				a += String() + "/" + split(n, "/");
			a = reverse(a);

			return s + a + ":" + l.line();
		}

		void EventLinker::logAllNames()
		{
			EventLinker *l = this;
			while (l->p)
				l = l->p;
			CAGE_ASSERT(l);
			CAGE_LOG(SeverityEnum::Info, "event-listener", Stringizer() + "(event): " + l->location);
			l = l->n;
			while (l)
			{
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "event-listener", Stringizer() + "(" + l->order + "): " + l->location);
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
