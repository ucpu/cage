namespace cage
{
	struct CAGE_API pointStruct
	{
		pointStruct() : x(0), y(0)
		{}

		pointStruct(sint32 x, sint32 y) : x(x), y(y)
		{}

		bool operator == (const pointStruct &other) const
		{
			return x == other.x && y == other.y;
		}

		bool operator != (const pointStruct &other) const
		{
			return !(*this == other);
		}

		bool operator < (const pointStruct &other) const
		{
			if (x == other.x)
				return y < other.y;
			return x < other.x;
		}

		sint32 operator [] (uint32 index) const
		{
			switch (index)
			{
			case 0: return x;
			case 1: return y;
			default: CAGE_THROW_CRITICAL(exception, "index out of range");
			}
		}

		sint32 &operator [] (uint32 index)
		{
			switch (index)
			{
			case 0: return x;
			case 1: return y;
			default: CAGE_THROW_CRITICAL(exception, "index out of range");
			}
		}

		sint32 x;
		sint32 y;
	};
}
