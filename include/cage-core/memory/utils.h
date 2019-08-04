namespace cage
{
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
		struct poolAllocatorAtomSize
		{
			static const uintPtr result = sizeof(T) + alignof(T);
		};

		template<class T>
		struct allocatorSizeList
		{
			void *a;
			void *b;
			T t;
		};

		template<class T>
		struct allocatorSizeSet
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
		struct allocatorSizeMap
		{
			struct pair
			{
				K k;
				V v;
			};
			allocatorSizeSet<pair> s;
		};
	}

	class CAGE_API virtualMemoryClass
	{
	public:
		void *reserve(uintPtr pages); // reserve address space
		void free(); // give up whole address space
		void increase(uintPtr pages); // allocate pages
		void decrease(uintPtr pages); // give up pages
		uintPtr pages() const; // currently allocated pages
	};

	CAGE_API holder<virtualMemoryClass> newVirtualMemory();

	namespace detail
	{
		CAGE_API uintPtr memoryPageSize();
	}
}