namespace cage
{
	struct CAGE_API OutOfMemory : public Exception
	{
		explicit OutOfMemory(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uintPtr memory) noexcept;
		virtual void log();
		uintPtr memory;
	};

	namespace detail
	{
		inline bool isPowerOf2(uintPtr x)
		{
			return x && !(x & (x - 1));
		}

		inline uintPtr addToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - (ptr % alignment)) % alignment;
		}

		inline uintPtr subtractToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - addToAlign(ptr, alignment)) % alignment;
		}

		inline uintPtr roundDownTo(uintPtr val, uintPtr size)
		{
			return (val / size) * size;
		}

		inline uintPtr roundUpTo(uintPtr val, uintPtr size)
		{
			return roundDownTo(val + size - 1, size);
		}

		CAGE_API uintPtr compressionBound(uintPtr size);
		CAGE_API uintPtr compress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize);
		CAGE_API uintPtr decompress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize);
	}

	namespace templates
	{
		template<class T>
		struct PoolAllocatorAtomSize
		{
			static const uintPtr result = sizeof(T) + alignof(T);
		};

		template<class T>
		struct AllocatorSizeList
		{
			void *a;
			void *b;
			T t;
		};

		template<class T>
		struct AllocatorSizeSet
		{
#ifdef CAGE_SYSTEM_WINDOWS
			void *a;
			void *b;
			void *c;
			char d;
			char e;
			T t;
#else
			char d;
			void *a;
			void *b;
			void *c;
			T t;
#endif
		};

		template<class K, class V>
		struct AllocatorSizeMap
		{
			struct pair
			{
				K k;
				V v;
			};
			AllocatorSizeSet<pair> s;
		};
	}

	class CAGE_API VirtualMemory
	{
	public:
		void *reserve(uintPtr pages); // reserve address space
		void free(); // give up whole address space
		void increase(uintPtr pages); // allocate pages
		void decrease(uintPtr pages); // give up pages
		uintPtr pages() const; // currently allocated pages
	};

	CAGE_API Holder<VirtualMemory> newVirtualMemory();

	namespace detail
	{
		CAGE_API uintPtr memoryPageSize();
	}
}
