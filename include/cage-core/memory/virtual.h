namespace cage
{
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
