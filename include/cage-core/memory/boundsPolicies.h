namespace cage
{
	struct CAGE_API memoryBoundsPolicyNone
	{
		static const uintPtr SizeFront = 0;
		static const uintPtr SizeBack = 0;

		void setFront(void *ptr)
		{}

		void setBack(void *ptr)
		{}

		void checkFront(void *ptr)
		{}

		void checkBack(void *ptr)
		{}
	};

	struct CAGE_API memoryBoundsPolicySimple
	{
		static const uintPtr SizeFront = 4;
		static const uintPtr SizeBack = 4;

		void setFront(void *ptr)
		{
			*(uint32*)ptr = PatternFront;
		}

		void setBack(void *ptr)
		{
			*(uint32*)ptr = PatternBack;
		}

		void checkFront(void *ptr)
		{
			if (*(uint32*)ptr != PatternFront)
				CAGE_THROW_CRITICAL(exception, "memory corruption detected");
		}

		void checkBack(void *ptr)
		{
			if (*(uint32*)ptr != PatternBack)
				CAGE_THROW_CRITICAL(exception, "memory corruption detected");
		}

	private:
		static const uint32 PatternFront = 0xCDCDCDCD;
		static const uint32 PatternBack = 0xDCDCDCDC;
	};
}
