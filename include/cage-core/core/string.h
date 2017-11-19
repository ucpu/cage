namespace cage
{
	namespace privat
	{
		CAGE_API void encodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_API void decodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen);
		CAGE_API void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right);
		CAGE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen);
		CAGE_API uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset);
		CAGE_API void stringOrder(char *data, uint32 &current);
		CAGE_API bool stringContains(const char *data, uint32 current, char what);
	}

	namespace detail
	{
		template<uint32 N>
		struct stringBase
		{
			// constructors
			stringBase() : current(0)
			{
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			stringBase(const stringBase &other) noexcept : current(other.current)
			{
				detail::memcpy(data, other.data, current);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			template<uint32 M>
			stringBase(const stringBase<M> &other)
			{
				if (other.current > N)
					CAGE_THROW_ERROR(exception, "string truncation");
				current = other.current;
				detail::memcpy(data, other.data, current);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			stringBase(bool other)
			{
				CAGE_ASSERT_COMPILE(N >= 6, string_too_short);
				*this = (other ? "true" : "false");
			}

#ifdef CAGE_DEBUG
#define GCHL_GENERATE(TYPE) \
			stringBase(TYPE other)\
			{\
				CAGE_ASSERT_COMPILE(N >= 20, string_too_short);\
				current = privat::sprint1(data, other);\
				data[current] = 0;\
			}
#else
#define GCHL_GENERATE(TYPE) \
			stringBase(TYPE other)\
			{\
				CAGE_ASSERT_COMPILE(N >= 20, string_too_short);\
				current = privat::sprint1(data, other);\
			}
#endif
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE

			explicit stringBase(void *other)
			{
				CAGE_ASSERT_COMPILE(N >= 20, string_too_short);
				current = privat::sprint1(data, other);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			stringBase(const char *pos, uint32 len)
			{
				if (len > N)
					CAGE_THROW_ERROR(exception, "string truncation");
				current = len;
				detail::memcpy(data, pos, len);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			stringBase(const char *other)
			{
				uintPtr len = detail::strlen(other);
				if (len > N)
					CAGE_THROW_ERROR(exception, "string truncation");
				current = numeric_cast<uint32>(len);
				detail::memcpy(data, other, current);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
			}

			// assignment operators
			stringBase &operator = (const stringBase &other) noexcept
			{
				if (this == &other)
					return *this;
				current = other.current;
				detail::memcpy(data, other.data, current);
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
				return *this;
			}

			// compound operators
			stringBase &operator += (const stringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(exception, "string truncation");
				detail::memcpy(data + current, other.data, other.current);
				current += other.current;
#ifdef CAGE_DEBUG
				data[current] = 0;
#endif
				return *this;
			}

			// binary operators
			stringBase operator + (const stringBase &other) const
			{
				return stringBase(*this) += other;
			}

			char &operator [] (uint32 idx)
			{
				CAGE_ASSERT_RUNTIME(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			char operator [] (uint32 idx) const
			{
				CAGE_ASSERT_RUNTIME(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			// comparison operators
#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (const stringBase &other) const { return detail::strcmp (c_str (), other.c_str ()) OPERATOR 0; }
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

			bool compareFast(const stringBase &other) const
			{
				if (current == other.current)
					return *this < other;
				return current < other.current;
			}

			// methods
			const char *c_str() const
			{
				const_cast<char*>(data)[current] = 0;
				return data;
			}

			stringBase reverse() const
			{
				stringBase ret(*this);
				for (uint32 i = 0; i < current; i++)
					ret[current - i - 1] = data[i];
				return ret;
			}

			stringBase subString(uint32 start, uint32 length) const
			{
				if (start >= current)
					return "";
				uint32 len = length;
				if (length == -1 || start + length > current)
					len = current - start;
				return stringBase(data + start, len);
			}

			stringBase replace(const stringBase &what, const stringBase &with) const
			{
				stringBase ret(*this);
				privat::stringReplace(ret.data, ret.current, N, what.data, what.current, with.data, with.current);
				return ret;
			}

			stringBase replace(uint32 start, uint32 length, const stringBase &with) const
			{
				return this->subString(0, start) + with + this->subString(start + length, -1);
			}

			stringBase remove(uint32 start, uint32 length) const
			{
				return subString(0, start) + subString(start + length, -1);
			}

			stringBase insert(uint32 start, const stringBase &what) const
			{
				return subString(0, start) + what + subString(start, -1);
			}

			stringBase trim(bool left = true, bool right = true, const stringBase &trimChars = "\t\n ") const
			{
				stringBase ret(*this);
				privat::stringTrim(ret.data, ret.current, trimChars.data, trimChars.current, left, right);
				return ret;
			}

			stringBase split(const stringBase &splitChars = "\t\n ")
			{
				stringBase ret;
				privat::stringSplit(data, current, ret.data, ret.current, splitChars.data, splitChars.current);
				return ret;
			}

			stringBase fill(uint32 size, char c = ' ') const
			{
				stringBase res = *this;
				while (res.length() < size)
					res += stringBase(&c, 1);
				return res;
			}

			stringBase toUpper() const
			{
				stringBase ret(*this);
				for (uint32 i = 0; i < current; i++)
					ret.data[i] = detail::toupper(data[i]);
				return ret;
			}

			stringBase toLower() const
			{
				stringBase ret(*this);
				for (uint32 i = 0; i < current; i++)
					ret.data[i] = detail::tolower(data[i]);
				return ret;
			}

			float toFloat() const
			{
				float i;
				privat::sscan1(c_str(), i);
				return i;
			}

			double toDouble() const
			{
				double i;
				privat::sscan1(c_str(), i);
				return i;
			}

			sint32 toSint32() const
			{
				sint32 i;
				privat::sscan1(c_str(), i);
				return i;
			}

			uint32 toUint32() const
			{
				uint32 i;
				privat::sscan1(c_str(), i);
				return i;
			}

			sint64 toSint64() const
			{
				sint64 i;
				privat::sscan1(c_str(), i);
				return i;
			}

			uint64 toUint64() const
			{
				uint64 i;
				privat::sscan1(c_str(), i);
				return i;
			}

			bool toBool() const
			{
				stringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off" || l == "0")
					return false;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on" || l == "1")
					return true;
				CAGE_THROW_ERROR(exception, "invalid value");
			}

			uint32 find(const stringBase &other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, other.data, other.current, offset);
			}

			uint32 find(char other, uint32 offset = 0) const
			{
				for (uint32 i = offset; i < current; i++)
					if (data[i] == other)
						return i;
				return -1;
			}

			uint32 length() const
			{
				return current;
			}

			stringBase order() const // best used with contains
			{
				stringBase ret(*this);
				privat::stringOrder(ret.data, ret.current);
				return ret;
			}

			bool contains(char other) const // requires to be ordered
			{
				return privat::stringContains(data, current, other);
			}

			bool pattern(const stringBase &prefix, const stringBase &infix, const stringBase &suffix) const
			{
				if (current < prefix.current + infix.current + suffix.current)
					return false;
				if (subString(0, prefix.current) != prefix)
					return false;
				if (subString(current - suffix.current, suffix.current) != suffix)
					return false;
				if (infix.empty())
					return true;
				uint32 pos = find(infix, prefix.current);
				return pos != (uint32)-1 && pos <= current - infix.current - suffix.current;
			}

			bool empty() const
			{
				return current == 0;
			}

			bool isDigitsOnly() const
			{
				for (uint32 i = 0; i < current; i++)
				{
					if (data[i] < '0' || data[i] > '9')
						return false;
				}
				return true;
			}

			bool isInteger(bool allowSign) const
			{
				if (empty())
					return false;
				if (allowSign && (data[0] == '-' || data[0] == '+'))
					return subString(1, current - 1).isInteger(false);
				else
					return isDigitsOnly();
			}

			bool isReal(bool allowSign) const
			{
				if (empty())
					return false;
				uint32 d = find('.');
				if (d == -1)
					return isInteger(allowSign);
				stringBase whole = subString(0, d);
				stringBase part = subString(d + 1, -1);
				return whole.isInteger(allowSign) && part.isInteger(false);
			}

			bool isBool() const
			{
				stringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off")
					return true;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on")
					return true;
				return false;
			}

			stringBase encodeUrl() const
			{
				stringBase res;
				privat::encodeUrl(data, current, res.data, res.current, N);
				return res;
			}

			stringBase decodeUrl() const
			{
				stringBase res;
				privat::decodeUrl(data, current, res.data, res.current, N);
				return res;
			}

			static const uint32 MaxLength = N;

		private:
			char data[N + 1];
			uint32 current;

			template<uint32 M>
			friend struct stringBase;
		};

		template<uint32 N>
		inline bool stringCompareFast(const stringBase<N> &a, const stringBase<N> &b)
		{
			return a.compareFast(b);
		}

		template<uint32 N>
		struct stringComparatorFastBase
		{
			bool operator () (const stringBase<N> &a, const stringBase<N> &b) const
			{
				return stringCompareFast(a, b);
			}
		};

		template struct CAGE_API stringBase<1000>;
		template struct CAGE_API stringComparatorFastBase<1000>;
	}

	typedef detail::stringBase<1000> string;
	typedef detail::stringComparatorFastBase<1000> stringComparatorFast;
}