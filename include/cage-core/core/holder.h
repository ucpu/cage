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
		holder(const holder &) = delete;
		holder &operator = (const holder &) = delete;

		// constructor
		holder() : ptr(nullptr), data(nullptr) {}
		explicit holder(T *data, void *ptr, delegate<void(void*)> deleter) : deleter(deleter), ptr(ptr), data(data) {}

		// destructor
		~holder()
		{
			if (deleter)
				deleter(ptr);
		}

		// move constructor
		holder(holder &&other) noexcept
		{
			deleter = other.deleter;
			ptr = other.ptr;
			data = other.data;
			other.deleter.clear();
			other.ptr = nullptr;
			other.data = nullptr;
		}

		// move assignment operator
		holder &operator = (holder &&other)
		{
			if (ptr == other.ptr)
				return *this;
			if (deleter)
				deleter(ptr);
			deleter = other.deleter;
			ptr = other.ptr;
			data = other.data;
			other.deleter.clear();
			other.ptr = nullptr;
			other.data = nullptr;
			return *this;
		}

		// operator bool
		explicit operator bool() const
		{
			return !!data;
		}

		// operator ->
		T *operator -> () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return data;
		}

		// operator *
		typename privat::holderDereference<T>::type operator * () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return *data;
		}

		// method get
		T *get() const
		{
			return data;
		}

		// method clear
		void clear()
		{
			if (deleter)
				deleter(ptr);
			deleter.clear();
			ptr = nullptr;
			data = nullptr;
		}

		// method cast
		template<class M>
		holder<M> cast()
		{
			if (*this)
			{
				holder<M> tmp(privat::holderCaster<M, T>()(data), ptr, deleter);
				if (tmp)
				{
					deleter.clear();
					ptr = nullptr;
					data = nullptr;
					return tmp;
				}
			}
			return holder<M>();
		}

	private:
		delegate<void(void *)> deleter;
		void *ptr; // pointer to deallocate
		T *data; // pointer to the object (may differ in case of classes with inheritance)
	};
}

