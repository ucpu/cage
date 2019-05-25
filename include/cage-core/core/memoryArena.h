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

