namespace cage
{
	template <class ArenaPolicy, uintPtr AllocSize>
	struct memoryArenaIndexed
	{
		template<class... Ts>
		memoryArenaIndexed(Ts... vs) : arena(templates::forward<Ts>(vs)...), start(nullptr), stride(0)
		{
			void *a = arena.allocate(AllocSize);
			void *b = arena.allocate(AllocSize);
			CAGE_ASSERT_RUNTIME(!!a && !!b);
			CAGE_ASSERT_RUNTIME(a < b);
			start = a;
			stride = (uintPtr)b - (uintPtr)a;
			flush();
		}

		void *allocate(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size <= AllocSize);
			void *res = arena.allocate(size);
			CAGE_ASSERT_RUNTIME(((uintPtr)res - (uintPtr)start) % stride == 0);
			return res;
		}

		void deallocate(void *ptr)
		{
			arena.deallocate(ptr);
		}

		void flush()
		{
			arena.flush();
		}

		uint32 p2i(void *ptr) const
		{
			CAGE_ASSERT_RUNTIME(((uintPtr)ptr - (uintPtr)start) % stride == 0);
			return numeric_cast<uint32>(((uintPtr)ptr - (uintPtr)start) / stride);
		}

		void *i2p(uint32 i) const
		{
			void *res = (void*)((uintPtr)start + i * stride);
			CAGE_ASSERT_RUNTIME((uintPtr)res - (uintPtr)getOrigin() <= getCurrentSize());
			return res;
		}

		const void *getOrigin() const { return arena.getOrigin(); }
		uintPtr getCurrentSize() const { return arena.getCurrentSize(); }
		uintPtr getReservedSize() const { return arena.getReservedSize(); }

	private:
		ArenaPolicy arena;
		void *start;
		uintPtr stride;
	};
}
