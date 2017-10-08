namespace cage
{
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

inline void  operator delete(void *ptr, void *ptr2, cage::privat::cageNewSpecifier) noexcept
{}

inline void  operator delete[](void *ptr, void *ptr2, cage::privat::cageNewSpecifier) noexcept
{}

namespace cage
{
	struct CAGE_API memoryArena
	{
	private:
		struct Stub
		{
			void *(*alloc)(void *, uintPtr);
			void(*dealloc)(void *, void *);
			void(*fls)(void *);

			template<class A> static void *allocate(void *inst, uintPtr size)
			{
				return((A*)inst)->allocate(size);
			}

			template<class A> static void deallocate(void *inst, void *ptr)
			{
				((A*)inst)->deallocate(ptr);
			}

			template<class A> static void flush(void *inst)
			{
				((A*)inst)->flush();
			}

			template<class A> static Stub init()
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
		memoryArena() : stub(nullptr), inst(nullptr)
		{}

		memoryArena(const memoryArena &a)
		{
			*this = a;
		}

		template<class A> memoryArena(A *a)
		{
			static Stub stb = Stub::init<A>();
			this->stub = &stb;
			inst = a;
		}

		memoryArena &operator = (const memoryArena &a)
		{
			stub = a.stub;
			inst = a.inst;
			return *this;
		}

		void *allocate(uintPtr size)
		{
			return stub->alloc(inst, size);
		}

		void deallocate(void *ptr)
		{
			stub->dealloc(inst, ptr);
		}

		void flush()
		{
			stub->fls(inst);
		}

		template<class T, class... Ts>
		T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T));
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
			return holder<T>(createObject<T>(templates::forward<Ts>(vs)...), d);
		};

		template<class T, class I, class... Ts>
		holder<T> createImpl(Ts... vs)
		{
			return createHolder<I>(templates::forward<Ts>(vs)...).template transfer<T>();
		};

		template<class T> void destroy(void *ptr)
		{
			if (!ptr)
				return;
			try
			{
				((T*)ptr)->~T();
			}
			catch (...)
			{
				CAGE_THROW_CRITICAL(exception, "exception thrown in destructor");
			}
			deallocate(ptr);
		}

		bool operator == (const memoryArena &other) const
		{
			return inst == other.inst;
		}

		bool operator != (const memoryArena &other) const
		{
			return !(*this == other);
		}
	};

	template<class T = void*> struct memoryArenaStd
	{
		typedef T value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef uintPtr size_type;
		typedef sintPtr difference_type;

		template<class U> struct rebind
		{
			typedef memoryArenaStd<U> other;
		};

		memoryArenaStd(const memoryArena &other) : a(other)
		{}

		memoryArenaStd(const memoryArenaStd &other) : a(other.a)
		{}

		template<class U> explicit memoryArenaStd(const memoryArenaStd<U> &other) : a(other.a)
		{}

		pointer address(reference r) const
		{
			return &r;
		}

		const_pointer address(const_reference r) const
		{
			return &r;
		}

		pointer allocate(size_type cnt, void *hint = 0)
		{
			return(pointer)a.allocate(cnt * sizeof(T));
		}

		void deallocate(pointer ptr, size_type)
		{
			a.deallocate(ptr);
		}

		const size_type max_size() const noexcept
		{
			return 0;
		}

		void construct(pointer ptr, const T &t)
		{
			new(ptr, privat::cageNewSpecifier()) T(t);
		}

		void destroy(pointer p)
		{
			p->~T();
		}

		bool operator == (const memoryArenaStd &other) const
		{
			return a == other.a;
		}

		bool operator != (const memoryArenaStd &other) const
		{
			return a != other.a;
		}

	private:
		memoryArena a;
		template<class U> friend struct memoryArenaStd;
	};

	namespace detail
	{
		CAGE_API memoryArena &systemArena();
	}
}
