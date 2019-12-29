namespace cage
{
	namespace privat
	{
#define GCHL_GENERATE(TYPE) CAGE_API uint32 toString(char *s, TYPE value); CAGE_API void fromString(const char *s, TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE
		CAGE_API uint32 toString(char *dst, uint32 dstLen, const char *src);

		CAGE_API void stringEncodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_API void stringDecodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen);
		CAGE_API void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right);
		CAGE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen);
		CAGE_API uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset);
		CAGE_API bool stringIsPattern(const char *data, uint32 dataLen, const char *prefix, uint32 prefixLen, const char *infix, uint32 infixLen, const char *suffix, uint32 suffixLen);
		CAGE_API int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl);
		CAGE_API uint32 stringToUpper(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_API uint32 stringToLower(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
	}

	namespace detail
	{
		CAGE_API void *memset(void *destination, int value, uintPtr num);
		CAGE_API void *memcpy(void *destination, const void *source, uintPtr num);
		CAGE_API void *memmove(void *destination, const void *source, uintPtr num);
		CAGE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num);

		template<uint32 N>
		struct StringBase
		{
			// constructors
			StringBase() : current(0)
			{
				data[current] = 0;
			}

			StringBase(const StringBase &other) noexcept : current(other.current)
			{
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

			template<uint32 M>
			StringBase(const StringBase<M> &other)
			{
				if (other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

			explicit StringBase(bool other)
			{
				static_assert(N >= 6, "string too short");
				*this = (other ? "true" : "false");
			}

#define GCHL_GENERATE(TYPE) \
			explicit StringBase(TYPE other)\
			{\
				static_assert(N >= 20, "string too short");\
				current = privat::toString(data, other);\
				data[current] = 0;\
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE

			template<class T>
			explicit StringBase(T *other) : StringBase((uintPtr)other)
			{}

			explicit StringBase(const char *pos, uint32 len)
			{
				if (len > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = len;
				detail::memcpy(data, pos, len);
				data[current] = 0;
			}

			StringBase(char *other)
			{
				current = privat::toString(data, N, other);
				data[current] = 0;
			}

			StringBase(const char *other)
			{
				current = privat::toString(data, N, other);
				data[current] = 0;
			}

			// assignment operators
			StringBase &operator = (const StringBase &other) noexcept
			{
				if (this == &other)
					return *this;
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
				return *this;
			}

			// compound operators
			StringBase &operator += (const StringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				detail::memcpy(data + current, other.data, other.current);
				current += other.current;
				data[current] = 0;
				return *this;
			}

			// binary operators
			StringBase operator + (const StringBase &other) const
			{
				return StringBase(*this) += other;
			}

			char &operator [] (uint32 idx)
			{
				CAGE_ASSERT(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			char operator [] (uint32 idx) const
			{
				CAGE_ASSERT(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			// comparison operators
#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (const StringBase &other) const { return privat::stringComparison(data, current, other.data, other.current) OPERATOR 0; }
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

			// methods
			const char *c_str() const
			{
				CAGE_ASSERT(data[current] == 0);
				return data;
			}

			StringBase reverse() const
			{
				StringBase ret(*this);
				for (uint32 i = 0; i < current; i++)
					ret.data[current - i - 1] = data[i];
				return ret;
			}

			StringBase subString(uint32 start, uint32 length) const
			{
				if (start >= current)
					return "";
				uint32 len = length;
				if (length == m || start + length > current)
					len = current - start;
				return StringBase(data + start, len);
			}

			StringBase replace(const StringBase &what, const StringBase &with) const
			{
				StringBase ret(*this);
				privat::stringReplace(ret.data, ret.current, N, what.data, what.current, with.data, with.current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase replace(uint32 start, uint32 length, const StringBase &with) const
			{
				return subString(0, start) + with + subString(start + length, m);
			}

			StringBase remove(uint32 start, uint32 length) const
			{
				return subString(0, start) + subString(start + length, m);
			}

			StringBase insert(uint32 start, const StringBase &what) const
			{
				return subString(0, start) + what + subString(start, m);
			}

			StringBase trim(bool left = true, bool right = true, const StringBase &trimChars = "\t\n ") const
			{
				StringBase ret(*this);
				privat::stringTrim(ret.data, ret.current, trimChars.data, trimChars.current, left, right);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase split(const StringBase &splitChars = "\t\n ")
			{
				StringBase ret;
				privat::stringSplit(data, current, ret.data, ret.current, splitChars.data, splitChars.current);
				data[current] = 0;
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase fill(uint32 size, char c = ' ') const
			{
				StringBase cc(&c, 1);
				StringBase ret = *this;
				while (ret.length() < size)
					ret += cc;
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toUpper() const
			{
				StringBase ret;
				ret.current = privat::stringToUpper(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toLower() const
			{
				StringBase ret;
				ret.current = privat::stringToLower(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			float toFloat() const
			{
				float i;
				privat::fromString(data, i);
				return i;
			}

			double toDouble() const
			{
				double i;
				privat::fromString(data, i);
				return i;
			}

			sint32 toSint32() const
			{
				sint32 i;
				privat::fromString(data, i);
				return i;
			}

			uint32 toUint32() const
			{
				uint32 i;
				privat::fromString(data, i);
				return i;
			}

			sint64 toSint64() const
			{
				sint64 i;
				privat::fromString(data, i);
				return i;
			}

			uint64 toUint64() const
			{
				uint64 i;
				privat::fromString(data, i);
				return i;
			}

			bool toBool() const
			{
				StringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off" || l == "0")
					return false;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on" || l == "1")
					return true;
				CAGE_THROW_ERROR(Exception, "invalid value");
			}

			uint32 find(const StringBase &other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, other.data, other.current, offset);
			}

			uint32 find(char other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, &other, 1, offset);
			}

			bool isPattern(const StringBase &prefix, const StringBase &infix, const StringBase &suffix) const
			{
				return privat::stringIsPattern(data, current, prefix.data, prefix.current, infix.data, infix.current, suffix.data, suffix.current);
			}

			bool isDigitsOnly() const
			{
				for (char c : *this)
				{
					if (c < '0' || c > '9')
						return false;
				}
				return true;
			}

			bool isInteger(bool allowSign) const
			{
				if (empty())
					return false;
				if (allowSign && (data[0] == '-' || data[0] == '+'))
					return subString(1, current - 1).isDigitsOnly();
				else
					return isDigitsOnly();
			}

			bool isReal(bool allowSign) const
			{
				if (empty())
					return false;
				uint32 d = find('.');
				if (d == m)
					return isInteger(allowSign);
				StringBase whole = subString(0, d);
				StringBase part = subString(d + 1, m);
				return whole.isInteger(allowSign) && part.isDigitsOnly();
			}

			bool isBool() const
			{
				StringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off")
					return true;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on")
					return true;
				return false;
			}

			StringBase encodeUrl() const
			{
				StringBase ret;
				privat::stringEncodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase decodeUrl() const
			{
				StringBase ret;
				privat::stringDecodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			bool empty() const
			{
				return current == 0;
			}

			uint32 length() const
			{
				return current;
			}

			uint32 size() const
			{
				return current;
			}

			char *begin()
			{
				return data;
			}

			char *end()
			{
				return data + current;
			}

			const char *begin() const
			{
				return data;
			}

			const char *end() const
			{
				return data + current;
			}

			static const uint32 MaxLength = N;

		private:
			char data[N + 1];
			uint32 current;

			template<uint32 M>
			friend struct StringBase;
		};

		template<uint32 N>
		struct StringizerBase
		{
			StringBase<N> value;

			operator const StringBase<N> & () const
			{
				return value;
			}

			template<uint32 M>
			operator const StringBase<M> () const
			{
				return value;
			}
		};

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringizerBase<M> &other)
		{
			str.value += other.value;
			return str;
		}

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringBase<M> &other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N, class T>
		StringizerBase<N> &operator + (StringizerBase<N> &str, T *other)
		{
			return str + (uintPtr)other;
		}

#define GCHL_GENERATE(TYPE) \
		template<uint32 N> \
		inline StringizerBase<N> &operator + (StringizerBase<N> &str, TYPE other) \
		{ \
			return str + StringBase<20>(other); \
		}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double, bool));
#undef GCHL_GENERATE

		template<uint32 N, class T>
		inline StringizerBase<N> &operator + (StringizerBase<N> &&str, const T &other)
		{
			// allow to use l-value-reference operator overloads with r-value-reference stringizer
			return str + other;
		}

		template<uint32 N>
		struct StringComparatorFastBase
		{
			bool operator () (const StringBase<N> &a, const StringBase<N> &b) const noexcept
			{
				if (a.length() == b.length())
					return detail::memcmp(a.begin(), b.begin(), a.length()) < 0;
				return a.length() < b.length();
			}
		};
	}

	typedef detail::StringBase<1000> string;
	typedef detail::StringizerBase<1000> stringizer;
	typedef detail::StringComparatorFastBase<1000> stringComparatorFast;
}
