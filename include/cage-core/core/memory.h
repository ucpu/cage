namespace cage
{
	// holder

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

	// pointer range

	template<class T>
	struct pointerRange
	{
	private:
		T *begin_;
		T *end_;

	public:
		pointerRange() : begin_(nullptr), end_(nullptr)
		{}

		pointerRange(T *begin, T *end) : begin_(begin), end_(end)
		{}

		template<class U>
		pointerRange(U &other) : begin_(other.data()), end_(other.data() + other.size())
		{}

		T *begin() const { return begin_; }
		T *end() const { return end_; }
		T *data() const { return begin_; }
		uintPtr size() const { return end_ - begin_; }
	};

	template<class T>
	constexpr T *begin(const holder<pointerRange<T>> &h)
	{
		return h->begin();
	}

	template<class T>
	constexpr T *end(const holder<pointerRange<T>> &h)
	{
		return h->end();
	}

	// memory arena

	namespace privat
	{
		struct cageNewSpecifier
		{};
	}

}

inline void *operator new(cage::uintPtr size, void *ptr, cage::privat::cageNewSpecifier) noexcept
{
	return ptr;
}

inline void *operator new[](cage::uintPtr size, void *ptr, cage::privat::cageNewSpecifier) noexcept
{
	return ptr;
}

inline void operator delete(void *ptr, void *ptr2, cage::privat::cageNewSpecifier) noexcept {}
inline void operator delete[](void *ptr, void *ptr2, cage::privat::cageNewSpecifier) noexcept {}

namespace cage
{

	struct CAGE_API memoryArena
	{
	private:
		struct Stub
		{
			void *(*alloc)(void *, uintPtr, uintPtr);
			void(*dealloc)(void *, void *);
			void(*fls)(void *);

			template<class A>
			static void *allocate(void *inst, uintPtr size, uintPtr alignment)
			{
				return ((A*)inst)->allocate(size, alignment);
			}

			template<class A>
			static void deallocate(void *inst, void *ptr) noexcept
			{
				((A*)inst)->deallocate(ptr);
			}

			template<class A>
			static void flush(void *inst) noexcept
			{
				((A*)inst)->flush();
			}

			template<class A>
			static Stub init() noexcept
			{
				Stub s;
				s.alloc = &allocate<A>;
				s.dealloc = &deallocate<A>;
				s.fls = &flush<A>;
				return s;
			}
		} *stub;
		void *inst;

	public:
		memoryArena() noexcept : stub(nullptr), inst(nullptr)
		{}

		template<class A>
		memoryArena(A *a) noexcept
		{
			static Stub stb = Stub::init<A>();
			this->stub = &stb;
			inst = a;
		}

		memoryArena(const memoryArena &a) = default;
		memoryArena(memoryArena &&a) = default;
		memoryArena &operator = (const memoryArena &) = default;
		memoryArena &operator = (memoryArena &&) = default;

		void *allocate(uintPtr size, uintPtr alignment)
		{
			void *res = stub->alloc(inst, size, alignment);
			CAGE_ASSERT_RUNTIME((uintPtr(res) % alignment) == 0, size, alignment, res, uintPtr(res) % alignment);
			return res;
		}

		void deallocate(void *ptr) noexcept
		{
			stub->dealloc(inst, ptr);
		}

		void flush() noexcept
		{
			stub->fls(inst);
		}

		template<class T, class... Ts>
		T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new(ptr, privat::cageNewSpecifier()) T(templates::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		holder<T> createHolder(Ts... vs)
		{
			delegate<void(void*)> d;
			d.bind<memoryArena, &memoryArena::destroy<T>>(this);
			T *p = createObject<T>(templates::forward<Ts>(vs)...);
			return holder<T>(p, p, d);
		};

		template<class Holder, class Impl, class... Ts>
		holder<Holder> createImpl(Ts... vs)
		{
			return createHolder<Impl>(templates::forward<Ts>(vs)...).template cast<Holder>();
		};

		template<class T>
		void destroy(void *ptr) noexcept
		{
			if (!ptr)
				return;
			try
			{
				((T*)ptr)->~T();
			}
			catch (...)
			{
				CAGE_ASSERT_RUNTIME(false, "exception thrown in destructor");
				detail::terminate();
			}
			deallocate(ptr);
		}

		bool operator == (const memoryArena &other) const noexcept
		{
			return inst == other.inst;
		}

		bool operator != (const memoryArena &other) const noexcept
		{
			return !(*this == other);
		}
	};

	namespace detail
	{
		CAGE_API memoryArena &systemArena();
	}
}
