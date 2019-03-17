namespace cage
{
	template<class T>
	struct holder
	{
		// constructor
		holder() : data(nullptr) {}
		explicit holder(T *value, delegate<void(void*)> deleter) : deleter(deleter), data(value) {}

		// destructor
		~holder()
		{
			if (deleter)
				deleter(data);
		}

		// move constructor
		holder(holder &&other)
		{
			data = other.data;
			deleter = other.deleter;
			other.data = nullptr;
			other.deleter.clear();
		}

		// move assignment operator
		holder &operator = (holder &&other)
		{
			if (data == other.data)
				return *this;
			if (deleter)
				deleter(data);
			data = other.data;
			deleter = other.deleter;
			other.data = nullptr;
			other.deleter.clear();
			return *this;
		}

		// operator bool
		explicit operator bool() const
		{
			return !!data;
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
			if (deleter)
				deleter(data);
			deleter.clear();
			data = nullptr;
		}

		// method cast
		template<class M>
		holder<M> cast()
		{
			if (*this)
			{
				holder<M> tmp(dynamic_cast<M*>(data), deleter);
				if (tmp)
				{
					data = nullptr;
					deleter.clear();
					return tmp;
				}
			}
			return holder<M>();
		}

		template<>
		holder<void> cast()
		{
			if (*this)
			{
				holder<void> tmp = holder<void>(data, deleter);
				data = nullptr;
				deleter.clear();
				return tmp;
			}
			return holder<void>();
		}

	private:
		delegate<void(void *)> deleter;
		T *data;
	};

	template<>
	struct holder<void>
	{
		// constructor
		holder() : data(nullptr) {}
		explicit holder(void *value, delegate<void(void*)> deleter) : deleter(deleter), data(value) {}

		// destructor
		~holder()
		{
			if (deleter)
				deleter(data);
		}

		// move constructor
		holder(holder &&other)
		{
			data = other.data;
			deleter = other.deleter;
			other.data = nullptr;
			other.deleter.clear();
		}

		// move assignment operator
		holder &operator = (holder &&other)
		{
			if (data == other.data)
				return *this;
			if (deleter)
				deleter(data);
			data = other.data;
			deleter = other.deleter;
			other.data = nullptr;
			other.deleter.clear();
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
			if (deleter)
				deleter(data);
			deleter.clear();
			data = nullptr;
		}

		// method cast
		template<class M>
		holder<M> cast()
		{
			if (*this)
			{
				holder<M> tmp((M*)data, deleter);
				data = nullptr;
				deleter.clear();
				return tmp;
			}
			return holder<M>();
		}

	private:
		delegate<void(void *)> deleter;
		void *data;
	};
}
