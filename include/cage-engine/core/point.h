namespace cage
{
	struct CAGE_API ivec2
	{
		ivec2() : x(0), y(0)
		{}

		explicit ivec2(sint32 x, sint32 y) : x(x), y(y)
		{}

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

	inline bool operator == (const ivec2 &l, const ivec2 &r) { return l[0] == r[0] && l[1] == r[1]; };
	inline bool operator != (const ivec2 &l, const ivec2 &r) { return !(l == r); };

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 operator OPERATOR (const ivec2 &r) { return ivec2(OPERATOR r[0], OPERATOR r[1]); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -));
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 operator OPERATOR (const ivec2 &l, const ivec2 &r) { return ivec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); } \
	inline ivec2 operator OPERATOR (const ivec2 &l, const sint32 &r) { return ivec2(l[0] OPERATOR r, l[1] OPERATOR r); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, /));
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 &operator OPERATOR##= (ivec2 &l, const ivec2 &r) { return l = l OPERATOR r; } \
	inline ivec2 &operator OPERATOR##= (ivec2 &l, const sint32 &r) { return l = l OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, /));
#undef GCHL_GENERATE
}
