namespace cage
{
	template<class T>
	struct memoryArenaStd
	{
		typedef T value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef uintPtr size_type;
		typedef sintPtr difference_type;

		template<class U>
		struct rebind
		{
			typedef memoryArenaStd<U> other;
		};

		memoryArenaStd(const memoryArena &other) : a(other) {}
		memoryArenaStd(const memoryArenaStd &other) : a(other.a) {}

		template<class U>
		explicit memoryArenaStd(const memoryArenaStd<U> &other) : a(other.a) {}

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
			return (pointer)a.allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(pointer ptr, size_type)
		{
			a.deallocate(ptr);
		}

		size_type max_size() const noexcept
		{
			return m;
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
		template<class U>
		friend struct memoryArenaStd;
	};
}

