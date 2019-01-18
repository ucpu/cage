namespace cage
{
	template<class T> struct holder;

	struct CAGE_API holdev
	{
		// constructor
		holdev() : data(nullptr) {}
		holdev(void *value, delegate<void(void*)> action) : action(action), data(value) {}

		// destructor
		~holdev()
		{
			if (action)
				action(data);
		}

		// move constructor
		holdev(holdev &&other)
		{
			data = other.data;
			action = other.action;
			other.data = nullptr;
			other.action.clear();
		}

		// move assignment operator
		holdev &operator = (holdev &&other)
		{
			if (data == other.data)
				return *this;
			if (action)
				action(data);
			data = other.data;
			action = other.action;
			other.data = nullptr;
			other.action.clear();
			return *this;
		}

		// operator bool
		explicit operator bool() const
		{
			return !!data;
		}

		// method get
		void *get()
		{
			return data;
		}

		const void *get() const
		{
			return data;
		}

		// method clear
		void clear()
		{
			if (action)
				action(data);
			action.clear();
			data = nullptr;
		}

		// method transfer
		template<class M>
		holder<M> transfer();

	private:
		delegate<void(void *)> action;
		void *data;
	};

	template<class T>
	struct holder
	{
		// constructor
		holder() : data(nullptr) {}
		holder(T *value, delegate<void(void*)> action) : action(action), data(value) {}

		// destructor
		~holder()
		{
			if (action)
				action(data);
		}

		// move constructor
		holder(holder &&other)
		{
			data = other.data;
			action = other.action;
			other.data = nullptr;
			other.action.clear();
		}

		// move assignment operator
		holder &operator = (holder &&other)
		{
			if (data == other.data)
				return *this;
			if (action)
				action(data);
			data = other.data;
			action = other.action;
			other.data = nullptr;
			other.action.clear();
			return *this;
		}

		// operator ->
		T *operator -> ()
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return data;
		}

		const T *operator -> () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return data;
		}

		// operator *
		T &operator * ()
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return *data;
		}

		const T &operator * () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return *data;
		}

		// operator bool
		explicit operator bool() const
		{
			return !!data;
		}

		// method get
		T *get()
		{
			return data;
		}

		const T *get() const
		{
			return data;
		}

		// method clear
		void clear()
		{
			if (action)
				action(data);
			action.clear();
			data = nullptr;
		}

		// method transfer
		template<class M>
		holder<M> transfer()
		{
			if (*this)
			{
				holder<M> tmp(dynamic_cast<M*>(data), action);
				data = nullptr;
				action.clear();
				return tmp;
			}
			else
				return holder<M>();
		}

		holdev transfev()
		{
			if (*this)
			{
				holdev tmp = holdev(data, action);
				data = nullptr;
				action.clear();
				return tmp;
			}
			else
				return holdev();
		}

	private:
		delegate<void(void *)> action;
		T *data;
	};

	template<>
	struct holder<void>
	{}; // this specialization is purposefully empty, use holdeV instead

	template<class M>
	holder<M> holdev::transfer()
	{
		if (*this)
		{
			holder<M> tmp((M*)data, action);
			data = nullptr;
			action.clear();
			return tmp;
		}
		else
			return holder<M>();
	}
}
