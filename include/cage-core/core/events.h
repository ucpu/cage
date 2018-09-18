namespace cage
{
	template<class> struct eventListener;
	template<class> struct eventDispatcher;

	namespace privat
	{
		struct CAGE_API eventLinker
		{
			eventLinker();
			eventLinker(eventLinker &other);
			virtual ~eventLinker();
			eventLinker &operator = (eventLinker &other);
			void attach(eventLinker *d, sint32 order);
			void detach();
			eventLinker *p, *n;
		private:
			void unlink();
			sint32 order;
		};

		template<class... Ts>
		struct eventInvoker : public eventLinker
		{
			void attach(eventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0)
			{
				eventLinker::attach(&dispatcher, order);
			}

		protected:
			virtual bool invoke(Ts... vs) const = 0;

			friend struct eventDispatcher<bool(Ts...)>;
		};
	}

	template<class... Ts>
	struct eventListener<bool(Ts...)> : private privat::eventInvoker<Ts...>, public delegate<bool(Ts...)>
	{
		using privat::eventInvoker<Ts...>::attach;
		using privat::eventInvoker<Ts...>::detach;

	private:
		virtual bool invoke(Ts... vs) const override
		{
			if (*this)
				return (*this)(templates::forward<Ts>(vs)...);
			return false;
		}

		friend struct eventDispatcher<bool(Ts...)>;
	};

	template<class... Ts>
	struct eventListener<void(Ts...)> : private privat::eventInvoker<Ts...>, public delegate<void(Ts...)>
	{
		using privat::eventInvoker<Ts...>::attach;
		using privat::eventInvoker<Ts...>::detach;

	private:
		virtual bool invoke(Ts... vs) const override
		{
			if (*this)
				(*this)(templates::forward<Ts>(vs)...);
			return false;
		}

		friend struct eventDispatcher<bool(Ts...)>;
	};
	
	template<class... Ts>
	struct eventDispatcher<bool(Ts...)> : private privat::eventInvoker<Ts...>
	{
		void attach(eventListener<bool(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		void attach(eventListener<void(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT_RUNTIME(!p);
			const privat::eventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const privat::eventInvoker<Ts...>*>(l)->invoke(templates::forward<Ts>(vs)...))
					return true;
				l = l->n;
			}
			return false;
		}
		
		using privat::eventInvoker<Ts...>::detach;

	private:
		virtual bool invoke(Ts... vs) const override
		{
			return false;
		}

		friend struct privat::eventInvoker<Ts...>;
	};
}
