namespace cage
{
	template<class T>
	struct memoryArenaStd
	{
		typedef T value_type;

		memoryArenaStd(const memoryArena &other) : a(other) {}
		memoryArenaStd(const memoryArenaStd &other) : a(other.a) {}

		template<class U>
		explicit memoryArenaStd(const memoryArenaStd<U> &other) : a(other.a) {}

		T *allocate(uintPtr cnt)
		{
			return (T*)a.allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr) noexcept
		{
			a.deallocate(ptr);
		}

		bool operator == (const memoryArenaStd &other) const noexcept
		{
			return a == other.a;
		}

		bool operator != (const memoryArenaStd &other) const noexcept
		{
			return a != other.a;
		}

	private:
		memoryArena a;
		template<class U>
		friend struct memoryArenaStd;
	};
}

