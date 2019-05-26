namespace cage
{
	// delegates

	template<class R>
	struct delegate
	{};

	template<class R, class... Ts>
	struct delegate<R(Ts...)>
	{
	private:
		struct
		{
			void *inst;
			R(*fnc)(void *, Ts...);
		} stub;

		template<R(*F)(Ts...)>
		static R functionStub(void *inst, Ts... vs)
		{
			(void)inst;
			return F(templates::forward<Ts>(vs)...);
		}

		template<class C, R(C::*F)(Ts...)>
		static R methodStub(void *inst, Ts... vs)
		{
			return (((C*)inst)->*F)(templates::forward<Ts>(vs)...);
		}

		template<class C, R(C::*F)(Ts...) const>
		static R constStub(void *inst, Ts... vs)
		{
			return (((const C*)inst)->*F)(templates::forward<Ts>(vs)...);
		}

		template<class C, R(C::*F)(Ts...)>
		struct methodWrapper
		{
			methodWrapper(C *inst) : inst(inst) {}
			C *inst;
		};

		template<class C, R(C::*F)(Ts...) const>
		struct constWrapper
		{
			constWrapper(const C *inst) : inst(inst) {}
			const C *inst;
		};

	public:
		delegate()
		{
			stub.inst = nullptr;
			stub.fnc = nullptr;
		}

		template<R(*F)(Ts...)>
		delegate &bind()
		{
			stub.inst = nullptr;
			stub.fnc = &functionStub<F>;
			return *this;
		}

		template<class C, R(C::*F)(Ts...)>
		delegate &bind(methodWrapper<C, F> w)
		{
			stub.inst = w.inst;
			stub.fnc = &methodStub<C, F>;
			return *this;
		}

		template<class C, R(C::*F)(Ts...) const>
		delegate &bind(constWrapper<C, F> w)
		{
			stub.inst = const_cast<C*>(w.inst);
			stub.fnc = &constStub<C, F>;
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return stub.fnc != nullptr;
		}

		void clear() noexcept
		{
			stub.inst = nullptr;
			stub.fnc = nullptr;
		}

		R operator ()(Ts... vs) const
		{
			CAGE_ASSERT_RUNTIME(!!*this, stub.inst, stub.fnc);
			return stub.fnc(stub.inst, templates::forward<Ts>(vs)...);
		}
	};

	// events

	template<class>
	struct eventListener;
	template<class>
	struct eventDispatcher;

	namespace privat
	{
		struct CAGE_API eventLinker
		{
			eventLinker();
			eventLinker(eventLinker &other);
			eventLinker(eventLinker &&other) = delete;
			eventLinker &operator = (eventLinker &other) = delete;
			eventLinker &operator = (eventLinker &&other) = delete;
			virtual ~eventLinker();
			void attach(eventLinker *d, sint32 order);
			void detach();
			eventLinker *p, *n;
		private:
			void unlink();
			bool valid() const;
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
			CAGE_ASSERT_RUNTIME(!this->p);
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
