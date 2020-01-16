namespace cage
{
	// holder
	template<class T> struct Holder;

	namespace privat
	{
		template<class R, class S>
		struct HolderCaster
		{
			R *operator () (S *p) const
			{
				if (!p)
					return nullptr;
				R *r = dynamic_cast<R*>(p);
				if (!r)
					CAGE_THROW_ERROR(Exception, "bad cast");
				return r;
			}
		};

		template<class R>
		struct HolderCaster<R, void>
		{
			R *operator () (void *p) const
			{
				return (R*)p;
			}
		};

		template<class S>
		struct HolderCaster<void, S>
		{
			void *operator () (S *p) const
			{
				return (void*)p;
			}
		};

		template<>
		struct HolderCaster<void, void>
		{
			void *operator () (void *p) const
			{
				return p;
			}
		};

		template<class T>
		struct HolderDereference
		{
			typedef T &type;
		};

		template<>
		struct HolderDereference<void>
		{
			typedef void type;
		};

		CAGE_API bool isHolderShareable(const Delegate<void(void *)> &deleter);
		CAGE_API void incHolderShareable(void *ptr, const Delegate<void(void *)> &deleter);
		CAGE_API void makeHolderShareable(void *&ptr, Delegate<void(void *)> &deleter);

		template<class T>
		struct HolderBase
		{
			HolderBase() : ptr_(nullptr), data_(nullptr) {}
			explicit HolderBase(T *data, void *ptr, Delegate<void(void*)> deleter) : deleter_(deleter), ptr_(ptr), data_(data) {}

			HolderBase(const HolderBase &) = delete;
			HolderBase(HolderBase &&other) noexcept
			{
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
			}

			HolderBase &operator = (const HolderBase &) = delete;
			HolderBase &operator = (HolderBase &&other) noexcept
			{
				if (this == &other)
					return *this;
				if (deleter_)
					deleter_(ptr_);
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
				return *this;
			}

			~HolderBase()
			{
				data_ = nullptr;
				void *tmpPtr = ptr_;
				Delegate<void(void *)> tmpDeleter = deleter_;
				ptr_ = nullptr;
				deleter_.clear();
				// calling the deleter is purposefully deferred to until this holder is cleared first
				if (tmpDeleter)
					tmpDeleter(tmpPtr);
			}

			explicit operator bool() const noexcept
			{
				return !!data_;
			}

			T *operator -> () const
			{
				CAGE_ASSERT(data_, "data is null");
				return data_;
			}

			typename HolderDereference<T>::type operator * () const
			{
				CAGE_ASSERT(data_, "data is null");
				return *data_;
			}

			T *get() const
			{
				return data_;
			}

			void clear()
			{
				if (deleter_)
					deleter_(ptr_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
			}

			bool isShareable() const
			{
				return isHolderShareable(deleter_);
			}

			Holder<T> share() const
			{
				incHolderShareable(ptr_, deleter_);
				return Holder<T>(data_, ptr_, deleter_);
			}

			Holder<T> makeShareable() &&
			{
				Holder<T> tmp(data_, ptr_, deleter_);
				makeHolderShareable(tmp.ptr_, tmp.deleter_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
				return tmp;
			}

		protected:
			Delegate<void(void *)> deleter_;
			void *ptr_; // pointer to deallocate
			T *data_; // pointer to the object (may differ in case of classes with inheritance)
		};
	}

	template<class T>
	struct Holder : public privat::HolderBase<T>
	{
		using privat::HolderBase<T>::HolderBase;

		template<class M>
		Holder<M> cast() &&
		{
			Holder<M> tmp(privat::HolderCaster<M, T>()(this->data_), this->ptr_, this->deleter_);
			this->deleter_.clear();
			this->ptr_ = nullptr;
			this->data_ = nullptr;
			return tmp;
		}
	};

	// pointer range

	template<class T>
	struct PointerRange
	{
	private:
		T *begin_;
		T *end_;

	public:
		typedef uintPtr size_type;

		PointerRange() : begin_(nullptr), end_(nullptr) {}
		template<class U>
		PointerRange(U *begin, U *end) : begin_(begin), end_(end) {}
		template<class U>
		PointerRange(const PointerRange<U> &other) : begin_(other.begin()), end_(other.end()) {}
		template<class U>
		PointerRange(U &&other) : begin_(other.data()), end_(other.data() + other.size()) {}

		T *begin() const { return begin_; }
		T *end() const { return end_; }
		T *data() const { return begin_; }
		size_type size() const { return end_ - begin_; }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size(), idx, size()); return begin_[idx]; }
	};

	template<class T>
	struct Holder<PointerRange<T>> : public privat::HolderBase<PointerRange<T>>
	{
		using privat::HolderBase<PointerRange<T>>::HolderBase;
		typedef typename PointerRange<T>::size_type size_type;

		operator Holder<PointerRange<const T>> () &&
		{
			Holder<PointerRange<const T>> tmp((PointerRange<const T>*)this->data_, this->ptr_, this->deleter_);
			this->deleter_.clear();
			this->ptr_ = nullptr;
			this->data_ = nullptr;
			return tmp;
		}

		T *begin() const { return this->data_->begin(); }
		T *end() const { return this->data_->end(); }
		T *data() const { return begin(); }
		size_type size() const { return end() - begin(); }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size(), idx, size()); return begin()[idx]; }
	};

	// memory arena

	namespace privat
	{
		struct CageNewTrait
		{};
	}
}

inline void *operator new(cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void *operator new[](cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void operator delete(void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}
inline void operator delete[](void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}

namespace cage
{
	struct CAGE_API MemoryArena
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
		MemoryArena() noexcept;

		template<class A>
		explicit MemoryArena(A *a) noexcept
		{
			static Stub stb = Stub::init<A>();
			this->stub = &stb;
			inst = a;
		}

		MemoryArena(const MemoryArena &) = default;
		MemoryArena(MemoryArena &&) = default;
		MemoryArena &operator = (const MemoryArena &) = default;
		MemoryArena &operator = (MemoryArena &&) = default;

		void *allocate(uintPtr size, uintPtr alignment);
		void deallocate(void *ptr) noexcept;
		void flush() noexcept;

		template<class T, class... Ts>
		T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new(ptr, privat::CageNewTrait()) T(templates::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		Holder<T> createHolder(Ts... vs)
		{
			Delegate<void(void*)> d;
			d.bind<MemoryArena, &MemoryArena::destroy<T>>(this);
			T *p = createObject<T>(templates::forward<Ts>(vs)...);
			return Holder<T>(p, p, d);
		};

		template<class Interface, class Impl, class... Ts>
		Holder<Interface> createImpl(Ts... vs)
		{
			return createHolder<Impl>(templates::forward<Ts>(vs)...).template cast<Interface>();
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
				CAGE_ASSERT(false, "exception thrown in destructor");
				detail::terminate();
			}
			deallocate(ptr);
		}

		bool operator == (const MemoryArena &other) const noexcept;
		bool operator != (const MemoryArena &other) const noexcept;
	};

	namespace detail
	{
		CAGE_API MemoryArena &systemArena();
	}
}
