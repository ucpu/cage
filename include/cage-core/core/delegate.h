namespace cage
{
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

		explicit operator bool() const
		{
			return stub.fnc != nullptr;
		}

		void clear()
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
}
