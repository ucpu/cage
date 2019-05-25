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
		holder() : ptr(nullptr), data(nullptr) {}
		explicit holder(T *data, void *ptr, delegate<void(void*)> deleter) : deleter(deleter), ptr(ptr), data(data) {}

		holder(const holder &) = delete;
		holder(holder &&other) noexcept
		{
			deleter = other.deleter;
			ptr = other.ptr;
			data = other.data;
			other.deleter.clear();
			other.ptr = nullptr;
			other.data = nullptr;
		}

		holder &operator = (const holder &) = delete;
		holder &operator = (holder &&other) noexcept
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

		~holder()
		{
			if (deleter)
				deleter(ptr);
		}

		explicit operator bool() const
		{
			return !!data;
		}

		T *operator -> () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return data;
		}

		typename privat::holderDereference<T>::type operator * () const
		{
			CAGE_ASSERT_RUNTIME(data, "data is null");
			return *data;
		}

		T *get() const
		{
			return data;
		}

		void clear()
		{
			if (deleter)
				deleter(ptr);
			deleter.clear();
			ptr = nullptr;
			data = nullptr;
		}

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

