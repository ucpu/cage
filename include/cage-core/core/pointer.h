namespace cage
{
	struct CAGE_API pointer
	{
		union
		{
			uintPtr decView;
			void *asVoid;
			char *asChar;
			void **asPtr;
			uintPtr *asUintPtr;
			uint64 *asUint64;
			uint32 *asUint32;
			uint16 *asUint16;
			uint8  *asUint8;
			sint64 *asSint64;
			sint32 *asSint32;
			sint16 *asSint16;
			sint8  *asSint8;
			float  *asFloat;
		};

		pointer() : asVoid(nullptr)
		{}

		pointer(void *other) : asVoid(other)
		{}

		explicit pointer(uintPtr other) : decView(other)
		{}

		pointer &operator = (pointer other)
		{
			asVoid = other.asVoid;
			return *this;
		}

		pointer &operator = (uintPtr other)
		{
			decView = other;
			return *this;
		}

		pointer &operator += (uintPtr other)
		{
			decView += other;
			return *this;
		}

		pointer &operator -= (uintPtr other)
		{
			CAGE_ASSERT_RUNTIME(other <= decView);
			decView -= other;
			return *this;
		}

		pointer operator + (uintPtr other) const
		{
			return pointer(decView + other);
		}

		pointer operator - (uintPtr other) const
		{
			CAGE_ASSERT_RUNTIME(other <= decView);
			return pointer(decView - other);
		}

		uintPtr operator - (pointer other) const
		{
			CAGE_ASSERT_RUNTIME(other.decView <= decView);
			return decView - other.decView;
		}

#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (pointer other) const { return decView OPERATOR other.decView; }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

		operator void * () const
		{
			return asVoid;
		}

		operator uintPtr() const
		{
			return decView;
		}
	};
}
