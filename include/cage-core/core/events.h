namespace cage
{
	namespace privat
	{
		struct CAGE_API eventPrivate
		{
			eventPrivate();
			eventPrivate(eventPrivate &other);
			~eventPrivate();
			eventPrivate &operator = (eventPrivate &other);
			void attach(eventPrivate *d);
			void detach();
			eventPrivate *p, *n;
		private:
			void unlink();
		};
	}

	template<class> struct eventListener;
	template<class> struct eventDispatcher;

	template<class... Ts>
	struct eventListener<bool(Ts...)> : private privat::eventPrivate, public delegate<bool(Ts...)>
	{
		void attach(eventDispatcher<bool(Ts...)> &dispatcher)
		{
			privat::eventPrivate::attach((privat::eventPrivate *) &dispatcher);
		}

		using privat::eventPrivate::detach;
		friend struct eventDispatcher<bool(Ts...)>;
	};
	
	template<class... Ts>
	struct eventDispatcher<bool(Ts...)> : private privat::eventPrivate
	{
		typedef eventListener<bool(Ts...)> listenerType;

		void attach(listenerType &listener)
		{
			listener.attach(*this);
		}

		bool dispatch(Ts... vs) const
		{
			for (listenerType *l = (listenerType *)n; l; l = (listenerType *)l->n)
			{
				if (!*l)
					continue;
				if ((*l)(templates::forward<Ts>(vs)...))
					return true;
			}
			return false;
		}
		
		using privat::eventPrivate::detach;
	};
}
