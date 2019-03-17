namespace cage
{
	namespace privat
	{
		template<class R, class S>
		struct holderCaster
		{
			R *operator () (S *p) const
			{
				return dynamic_cast<R*>(p);
			}
		};

		template<class R>
		struct holderCaster<R, void>
		{
			R *operator () (void *p) const
			{
				return (R*)p;
			}
		};

		template<class S>
		struct holderCaster<void, S>
		{
			void *operator () (S *p) const
			{
				return (void*)p;
			}
		};

		template<>
		struct holderCaster<void, void>
		{
			void *operator () (void *p) const
			{
				return p;
			}
		};

		template<class T>
		struct holderDereference
		{
			typedef T &type;
		};

		template<>
		struct holderDereference<void>
		{
			typedef void type;
		};
	}

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
		typename privat::holderDereference<T>::type operator * ()
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return *data;
		}

		const typename privat::holderDereference<T>::type operator * () const
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
				holder<M> tmp(privat::holderCaster<M, T>()(data), deleter);
				if (tmp)
				{
					data = nullptr;
					deleter.clear();
					return tmp;
				}
			}
			return holder<M>();
		}

	private:
		delegate<void(void *)> deleter;
		T *data;
	};
}

