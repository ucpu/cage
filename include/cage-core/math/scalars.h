namespace cage
{
	struct CAGE_API real
	{
		float value;
		static float epsilon;

		// constructors
		real() : value(0) {}

#define GCHL_GENERATE(TYPE) real (TYPE other): value ((float)other) {}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE

		explicit real(rads other);
		explicit real(degs other);

		// compound operators
#define GCHL_GENERATE(OPERATOR) real &operator OPERATOR (real other) { value OPERATOR other.value; return *this; }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +=, -=, *=, /=));
#undef GCHL_GENERATE

		// unary operators
		real operator - () const { return -value; }
		real operator + () const { return *this; }

		// binary operators
#define GCHL_GENERATE(OPERATOR) real operator OPERATOR (real other) const { return value OPERATOR other.value; }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / ));
#undef GCHL_GENERATE

		real operator % (real other) const { CAGE_ASSERT_RUNTIME(other != 0); return value - (other.value * (int)(value / other.value)); }
		rads operator * (rads other) const;
		degs operator * (degs other) const;

		// comparison operators
		//bool operator == (real other) const { return (*this - other).abs() <= (abs() < other.abs() ? other.abs() : abs()) * epsilon; }

#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (real other) const { return value OPERATOR other.value; };
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, != , <, >, <= , >= , == ));
#undef GCHL_GENERATE

		// conversion operators
		operator string() const { return string(value); }

		// methods
		real sqr() const { return value * value; }
		real powE() const { return pow(E); }
		real pow2() const { return pow(2); }
		real pow10() const { return pow(10); }
		real log(real base) const { return ln() / base.ln(); }
		real log2() const { return ln() / Ln2; }
		real log10() const { return ln() / Ln10; }
		real min(real other) const { return *this < other ? *this : other; }
		real max(real other) const { return *this > other ? *this : other; }
		real clamp() const { return clamp(0, 1); }
		real clamp(real minimal, real maximal) const { CAGE_ASSERT_RUNTIME(minimal <= maximal, value, minimal.value, maximal.value); return value < minimal.value ? minimal : value > maximal.value ? maximal : value; }
		real abs() const { if (value < 0) return -value; return value; }
		sint32 sign() const { if (*this > 0) return 1; else if (*this < 0) return -1; else return 0; }
		bool valid() const { return value == value; }
		bool finite() const { return valid() && *this != real::PositiveInfinity && *this != real::NegativeInfinity; }
		real sqrt() const;
		real pow(real power) const;
		real ln() const;
		real round() const;
		real floor() const;
		real ceil() const;
		real smoothstep() { return value * value * (3 - 2 * value); }
		real smootherstep() { return value * value * value * (value * (value * 6 - 15) + 10); }

		// constants
		static const real Pi;
		static const real HalfPi;
		static const real TwoPi;
		static const real E; // Euler's number
		static const real Ln2; // natural logarithm of 2
		static const real Ln10; // natural logarithm of 10
		static const real PositiveInfinity;
		static const real NegativeInfinity;
		static const real Nan;
	};

	struct CAGE_API rads
	{
	private:
		real value;

	public:
		// constructors
		explicit rads(const real value = 0) : value(value) {}
		rads(degs other);

		// compound operators
		rads &operator += (rads other) { value += other.value; return *this; };
		rads &operator -= (rads other) { value -= other.value; return *this; };
		rads &operator *= (real other) { value *= other; return *this; };
		rads &operator /= (real other) { value /= other; return *this; };

		// unary operators
		rads operator - () const { return rads(-value); }
		rads operator + () const { return *this; }

		// binary operators
		rads operator + (rads other) const { return rads(value + other.value); };
		rads operator - (rads other) const { return rads(value - other.value); };
		rads operator * (real other) const { return rads(value * other); };
		rads operator / (real other) const { return rads(value / other); };
		real operator / (rads other) const { return value / other.value; }

		// comparison operators
#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (rads other) const { return value OPERATOR other.value; };
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

		// conversion operators
		operator string() const { return string() + value + " rads"; }

		// methods
		rads normalize() const { if (value < 0) return Full - rads(-value % (real)Full); else return rads(value % (real)Full); }
		rads min(rads other) const { return rads(value.min(other.value)); }
		rads max(rads other) const { return rads(value.max(other.value)); }
		rads clamp() const { return clamp(Zero, Full); }
		rads clamp(rads min, rads max) const { return rads(value.clamp(min.value, max.value)); }
		rads abs() const { return rads(value.abs()); }
		sint32 sign() const { return value.sign(); }
		bool valid() const { return value.valid(); }

		// constants
		static const rads Zero;
		static const rads Right;
		static const rads Stright;
		static const rads Full;

		friend struct real;
		friend struct degs;
	};

	struct CAGE_API degs
	{
	private:
		real value;

	public:
		// constructors
		explicit degs(real value = 0) : value(value) {}
		degs(rads other);

		// conversion operators
		operator string() const { return string() + value + " °"; }

		friend CAGE_API struct real;
		friend CAGE_API struct rads;
	};

#define GCHL_GENERATE(OPERATOR) inline bool operator OPERATOR (float l, real r) { return real(l) OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, != , <, >, <= , >= ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) inline real operator OPERATOR (float l, real r) { return real(l) OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / , %));
#undef GCHL_GENERATE

	inline real smoothstep(real x) { return x.smoothstep(); }
	inline real smootherstep(real x) { return x.smootherstep(); }
	inline real sqr(real th) { return th.sqr(); }
	inline real sqrt(real th) { return th.sqrt(); }
	inline real pow(real th, real power) { return th.pow(power); }
	inline real powE(real th) { return th.powE(); }
	inline real pow2(real th) { return th.pow2(); }
	inline real pow10(real th) { return th.pow10(); } 
	inline real log(real th, real base) { return th.log(base); }
	inline real ln(real th) { return th.ln(); }
	inline real log2(real th) { return th.log2(); }
	inline real log10(real th) { return th.log10(); }
	inline real abs(real th) { return th.abs(); }
	inline real round(real th) { return th.round(); }
	inline real floor(real th) { return th.floor(); }
	inline real ceil(real th) { return th.ceil(); }
	inline sint32 sign(real th) { return th.sign(); }
	inline bool valid(real th) { return th.valid(); }

	namespace detail
	{
		template<> struct numeric_limits<real> : public numeric_limits<decltype(real::value)> {};
	}

	template<class To>
	To numeric_cast(real from)
	{
		return privat::numeric_cast_helper_specialized<detail::numeric_limits<To>::is_specialized>::template cast<To>(from.value);
	};

	inline rads normalize(rads th) { return th.normalize(); }
	inline rads abs(rads th) { return th.abs(); }
	inline sint32 sign(rads th) { return th.sign(); }
	inline bool valid(rads th) { return th.valid(); }

	inline real::real(rads other) : value(other.value.value) {}
	inline real::real(degs other) : value(other.value.value) {}
	inline rads real::operator * (rads other) const { return other * value; }
	inline rads::rads(degs other) : value(other.value * real::Pi / (real)180) {}
	inline degs::degs(rads other) : value(other.value * (real)180 / real::Pi) {}

	CAGE_API real sin(rads value);
	CAGE_API real cos(rads value);
	CAGE_API real tan(rads value);
	CAGE_API rads aSin(real value);
	CAGE_API rads aCos(real value);
	CAGE_API rads aTan(real value);
	CAGE_API rads aTan2(real x, real y);

#define GCHL_GENERATE(TYPE) inline TYPE min(TYPE a, TYPE b) { return a < b ? a : b; } inline TYPE max(TYPE a, TYPE b) { return a > b ? a : b; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) inline TYPE clamp(TYPE value, TYPE minimal, TYPE maximal) { return min(max(value, minimal), maximal); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) inline TYPE interpolate(TYPE min, TYPE max, real value) { return (TYPE)(min + (max - min) * value.value); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE

	CAGE_API real random(); // 0 to 1; including 0, excluding 1
	CAGE_API sint32 random(sint32 min, sint32 max); // including min, excluding max
	CAGE_API real random(real min, real max);
	CAGE_API rads random(rads min, rads max);
	CAGE_API rads randomAngle();
	CAGE_API randomGenerator &currentRandomGenerator();

	namespace detail
	{
		inline uint32 rotl32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value << shift) | (value >> (32 - shift)); }
		inline uint32 rotr32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value >> shift) | (value << (32 - shift)); }
	}
}
