#ifndef guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
#define guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_

namespace cage
{
	// scalars

	struct CAGE_API real
	{
		float value;

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

		// allow to treat it as a one-dimensional vector
		real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx == 0, "index out of range", idx); return *this; }
		real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx == 0, "index out of range", idx); return *this; }

		// comparison operators
#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (real other) const { return value OPERATOR other.value; };
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, != , <, >, <= , >= , == ));
#undef GCHL_GENERATE

		// conversion operators
		operator string() const { return string(value); }

		// methods
		real sqr() const { return value * value; }
		real powE() const { return pow(E()); }
		real pow2() const { return pow(2); }
		real pow10() const { return pow(10); }
		real log(real base) const { return ln() / base.ln(); }
		real log2() const { return ln() / Ln2(); }
		real log10() const { return ln() / Ln10(); }
		real min(real other) const { return *this < other ? *this : other; }
		real max(real other) const { return *this > other ? *this : other; }
		real clamp() const { return clamp(0, 1); }
		real clamp(real minimal, real maximal) const { CAGE_ASSERT_RUNTIME(minimal <= maximal, value, minimal.value, maximal.value); return value < minimal.value ? minimal : value > maximal.value ? maximal : value; }
		real abs() const { if (value < 0) return -value; return value; }
		sint32 sign() const { if (*this > 0) return 1; else if (*this < 0) return -1; else return 0; }
		bool valid() const { return value == value; }
		bool finite() const { return valid() && *this != real::Infinity() && *this != -real::Infinity(); }
		real sqrt() const;
		real pow(real power) const;
		real ln() const;
		real round() const;
		real floor() const;
		real ceil() const;
		real smoothstep() { return value * value * (3 - 2 * value); }
		real smootherstep() { return value * value * value * (value * (value * 6 - 15) + 10); }
		static real parse(const string &str);

		// constants
		static real Pi();
		static real E(); // Euler's number
		static real Ln2(); // natural logarithm of 2
		static real Ln10(); // natural logarithm of 10
		static real Infinity();
		static real Nan();
		static const uint32 Dimension = 1;
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
		rads normalize() const { if (value < 0) return Full() - rads(-value % (real)Full()); else return rads(value % (real)Full()); }
		rads min(rads other) const { return rads(value.min(other.value)); }
		rads max(rads other) const { return rads(value.max(other.value)); }
		rads clamp() const { return clamp(rads(), Full()); }
		rads clamp(rads min, rads max) const { return rads(value.clamp(min.value, max.value)); }
		rads abs() const { return rads(value.abs()); }
		sint32 sign() const { return value.sign(); }
		bool valid() const { return value.valid(); }

		// constants
		static rads Right();
		static rads Stright();
		static rads Full();
		static rads Nan();

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
		operator string() const { return string() + value + " degs"; }

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
		template<>
		struct numeric_limits<real> : public numeric_limits<decltype(real::value)> {};
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
	inline rads::rads(degs other) : value(other.value * real::Pi() / (real)180) {}
	inline degs::degs(rads other) : value(other.value * (real)180 / real::Pi()) {}

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

	CAGE_API real randomChance(); // 0 to 1; including 0, excluding 1
	CAGE_API rads randomAngle();
#define GCHL_GENERATE(TYPE) CAGE_API TYPE randomRange(TYPE min, TYPE max);
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, real, rads, float, double));
#undef GCHL_GENERATE

	namespace detail
	{
		inline uint32 rotl32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value << shift) | (value >> (32 - shift)); }
		inline uint32 rotr32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value >> shift) | (value << (32 - shift)); }
	}

	// vectors

#define GCHL_GEN_QUATERNION_BASE(NAME,N) \
NAME operator - () const { return inverse(); } \
NAME operator + () const { return *this; } \
real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
bool operator == (const NAME &other) const { for (uint32 i = 0; i < N; i++) if (data[i] != other.data[i]) return false; return true; } \
bool operator != (const NAME &other) const { return !(*this == other); } \
operator string() const { string r = "("; for (uint32 i = 0; i < N - 1; i++) r += string(data[i]) + ", "; return r + string(data[N - 1]) + ")"; } \
NAME inverse() const { return *this * -1; } \
NAME normalize() const { return *this / length(); } \
real length() const { return squaredLength().sqrt(); } \
real squaredLength() const { return dot(*this); } \
real dot(const NAME &other) const { real sum = 0; for (uint32 i = 0; i < N; i++) sum += data[i] * other.data[i]; return sum; } \
bool valid() const { for (uint32 i = 0; i < N; i++) if (!data[i].valid()) return false; return true; } \
NAME &operator += (const NAME &other) { return *this = *this + other; } \
NAME &operator += (real other) { return *this = *this + other; } \
NAME &operator -= (const NAME &other) { return *this = *this - other; } \
NAME &operator -= (real other) { return *this = *this - other; } \
NAME &operator *= (real other) { return *this = *this * other; } \
NAME &operator /= (real other) { return *this = *this / other; } \
NAME operator + (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] + other.data[i]; return r; } \
NAME operator + (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] + other; return r; } \
NAME operator - (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] - other.data[i]; return r; } \
NAME operator - (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] - other; return r; } \
NAME operator * (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * other; return r; } \
NAME operator / (real other) const { NAME r; real d = real(1) / other; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * d; return r; } \
static NAME parse(const string &str); \
static NAME Nan();

#define GCHL_GEN_VECTOR_BASE(NAME, N) \
GCHL_GEN_QUATERNION_BASE(NAME, N) \
NAME() : NAME(0) {} \
explicit NAME(real value) { for (uint32 i = 0; i < N; i++) data[i] = value; } \
NAME min(const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other.data[i] ? data[i] : other.data[i]; return r; } \
NAME max(const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other.data[i] ? data[i] : other.data[i]; return r; } \
NAME min(real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other ? data[i] : other; return r; } \
NAME max(real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other ? data[i] : other; return r; } \
NAME abs() const { return this->max(-*this); } \
NAME clamp(const NAME &minimum, const NAME &maximum) const { CAGE_ASSERT_RUNTIME(minimum.min(maximum) == minimum); return min(maximum).max(minimum); } \
NAME clamp(real minimum, real maximum) const { CAGE_ASSERT_RUNTIME (minimum <= maximum, minimum, maximum); return min(maximum).max(minimum); } \
real distance(const NAME &other) const { return squaredDistance(other).sqrt(); } \
real squaredDistance(const NAME &other) const { return (*this - other).squaredLength(); } \
NAME &operator *= (const NAME &other) { return *this = *this * other; } \
NAME &operator /= (const NAME &other) { return *this = *this / other; } \
NAME operator * (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * other.data[i]; return r; } \
NAME operator / (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] / other.data[i]; return r; } \
static const uint32 Dimension = N;

#define GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME inverse(const NAME &th) { return th.inverse(); } \
inline NAME normalize(const NAME &th) { return th.normalize(); } \
inline real length(const NAME &th) { return th.length(); } \
inline real squaredLength(const NAME &th) { return th.squaredLength(); } \
inline real dot(const NAME &th, const NAME &other) { return th.dot(other); } \
inline bool valid(const NAME &th) { return th.valid(); } \
inline NAME operator + (real th, const NAME &other) { return other + th; } \
inline NAME operator - (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th - other.data[i]; return r; } \
inline NAME operator * (real th, const NAME &other) { return other * th; } \
inline NAME operator / (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th / other.data[i]; return r; }

#define GCHL_GEN_VECTOR_FUNCTIONS(NAME, N) \
GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME min(const NAME &th, const NAME &other) { return th.min(other); } \
inline NAME max(const NAME &th, const NAME &other) { return th.max(other); } \
inline NAME min(const NAME &th, real other) { return th.min(other); } \
inline NAME max(const NAME &th, real other) { return th.max(other); } \
inline NAME min(real other, const NAME &th) { return th.min(other); } \
inline NAME max(real other, const NAME &th) { return th.max(other); } \
inline NAME abs(const NAME &th) { return th.abs(); } \
inline NAME clamp(const NAME &th, const NAME &minimum, const NAME &maximum) { return th.clamp(minimum, maximum); } \
inline NAME clamp(const NAME &th, real minimum, real maximum) { return th.clamp(minimum, maximum); } \
inline real distance(const NAME &th, const NAME &other) { return th.distance(other); } \
inline real squaredDistance(const NAME &th, const NAME &other) { return th.squaredDistance(other); } \
inline NAME CAGE_JOIN(randomChance, N) () { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = randomChance(); return r; } \
inline NAME CAGE_JOIN(randomRange, N) (real a, real b) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = randomRange(a, b); return r; }

	struct CAGE_API vec2
	{
		real data[2];

		GCHL_GEN_VECTOR_BASE(vec2, 2);

		explicit vec2(real x, real y) { data[0] = x; data[1] = y; }
		explicit vec2(const vec3 &other);
		explicit vec2(const vec4 &other);

		real cross(const vec2 &other) const { return data[0] * other.data[1] - data[1] * other.data[0]; }
	};

	struct CAGE_API vec3
	{
		real data[3];

		GCHL_GEN_VECTOR_BASE(vec3, 3);

		explicit vec3(real x, real y, real z) { data[0] = x; data[1] = y; data[2] = z; }
		explicit vec3(const vec2 &other, real z);
		explicit vec3(const vec4 &other);

		vec3 &operator *= (const mat3 &other) { return *this = *this * other; }
		vec3 operator * (const quat &other) const;
		vec3 operator * (const mat3 &other) const;

		vec3 primaryAxis() const;

		vec3 cross(const vec3 &other) const { return vec3(data[1] * other.data[2] - data[2] * other.data[1], data[2] * other.data[0] - data[0] * other.data[2], data[0] * other.data[1] - data[1] * other.data[0]); }
	};

	struct CAGE_API vec4
	{
		real data[4];

		GCHL_GEN_VECTOR_BASE(vec4, 4);

		explicit vec4(real x, real y, real z, real w) { data[0] = x; data[1] = y; data[2] = z; data[3] = w; }
		explicit vec4(const vec2 &v, real z, real w);
		explicit vec4(const vec2 &v, const vec2 &w);
		explicit vec4(const vec3 &v, real w);

		vec4 &operator *= (const mat4 &other) { return *this = *this * other; }
		vec4 operator * (const mat4 &other) const;
	};

	struct CAGE_API quat
	{
		real data[4]; // x, y, z, w

		GCHL_GEN_QUATERNION_BASE(quat, 4);

		quat();
		explicit quat(real x, real y, real z, real w);
		explicit quat(rads pitch, rads yaw, rads roll);
		explicit quat(const vec3 &axis, rads angle);
		explicit quat(const mat3 &rot);
		explicit quat(const vec3 &forward, const vec3 &up, bool keepUp = false);

		quat &operator *= (const quat &other) { return *this = *this * other; }
		vec3 operator * (const vec3 &other) const;
		quat operator * (const quat &other) const;
		transform operator * (const transform &other) const;

		quat conjugate() const { return quat(-data[0], -data[1], -data[2], data[3]); }
		void toAxisAngle(vec3 &axis, rads &angle) const;
	};

	GCHL_GEN_VECTOR_FUNCTIONS(vec2, 2);
	GCHL_GEN_VECTOR_FUNCTIONS(vec3, 3);
	GCHL_GEN_VECTOR_FUNCTIONS(vec4, 4);
	GCHL_GEN_QUATERNION_FUNCTIONS(quat, 4);

	inline real cross(const vec2 &th, const vec2 &other) { return th.cross(other); }
	inline vec3 cross(const vec3 &th, const vec3 &other) { return th.cross(other); }
	inline vec3 primaryAxis(vec3 &th) { return th.primaryAxis(); }
	inline quat conjugate(const quat &th) { return th.conjugate(); }
	CAGE_API quat lerp(const quat &min, const quat &max, real value);
	CAGE_API quat slerp(const quat &min, const quat &max, real value);
	CAGE_API quat rotate(const quat &from, const quat &toward, rads maxTurn);

	inline vec3 vec3::operator * (const quat &other) const { return other * *this; }

#define GCHL_GENERATE(TYPE) inline TYPE interpolate (const TYPE &min, const TYPE &max, real value) { return (TYPE)(min + (max - min) * value.value); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, vec2, vec3, vec4));
#undef GCHL_GENERATE
	inline quat interpolate(const quat &min, const quat &max, real value) { return slerp(min, max, value); }

	inline vec2::vec2(const vec3 &other) { data[0] = other.data[0]; data[1] = other.data[1]; }
	inline vec2::vec2(const vec4 &other) { data[0] = other.data[0]; data[1] = other.data[1]; }
	inline vec3::vec3(const vec2 &other, real z) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = z; }
	inline vec3::vec3(const vec4 &other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; }
	inline vec4::vec4(const vec2 &v, real z, real w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = z; data[3] = w; }
	inline vec4::vec4(const vec2 &v, const vec2 &w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = w.data[0]; data[3] = w.data[1]; }
	inline vec4::vec4(const vec3 &v, real w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = v.data[2]; data[3] = w; }

	CAGE_API vec2 randomDirection2();
	CAGE_API vec3 randomDirection3();
	CAGE_API quat randomDirectionQuat();

	template<uint32 N>
	struct vecN
	{};

	template<> struct vecN<1> { typedef real type; };
	template<> struct vecN<2> { typedef vec2 type; };
	template<> struct vecN<3> { typedef vec3 type; };
	template<> struct vecN<4> { typedef vec4 type; };

#undef GCHL_GEN_QUATERNION_BASE
#undef GCHL_GEN_VECTOR_BASE
#undef GCHL_GEN_QUATERNION_FUNCTIONS
#undef GCHL_GEN_VECTOR_FUNCTIONS

	// matrices

	struct CAGE_API mat3
	{
		// data
		real data[9];

		// constructors
		mat3(); // identity
		explicit mat3(real a, real b, real c, real d, real e, real f, real g, real h, real i);
		explicit mat3(const vec3 &forward, const vec3 &up, bool keepUp = false);
		explicit mat3(const quat &other);
		explicit mat3(const mat4 &other);

		// compound operators
		mat3 &operator *= (real other) { return *this = *this * other; }
		mat3 &operator += (const mat3 &other) { return *this = *this + other; }
		mat3 &operator *= (const mat3 &other) { return *this = *this * other; }

		// binary operators
		mat3 operator * (real other) const;
		vec3 operator * (const vec3 &other) const;
		mat3 operator + (const mat3 &other) const;
		mat3 operator * (const mat3 &other) const;

		real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }
		real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }

		// comparison operators
		bool operator == (const mat3 &other) const { for (uint8 i = 0; i < 9; i++) if (data[i] != other.data[i]) return false; return true; }
		bool operator != (const mat3 &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { string res = string() + "(" + data[0]; for (uint8 i = 1; i < 9; i++) res += string() + "," + data[i]; return res + ")"; }

		// methods
		mat3 inverse() const;
		mat3 transpose() const;
		mat3 normalize() const;
		real determinant() const;
		bool valid() const;
		static mat3 parse(const string &str);

		// constants
		static mat3 Zero();
		static mat3 Nan();
	};

	struct CAGE_API mat4
	{
		// data
		real data[16];

		// constructors
		mat4(); // identity
		explicit mat4(real a, real b, real c, real d, real e, real f, real g, real h, real i, real j, real k, real l, real m, real n, real o, real p);
		explicit mat4(const mat3 &other);
		explicit mat4(const vec3 &other); // translation matrix
		explicit mat4(const vec3 &position, const quat &orientation, const vec3 &scale = vec3(1));
		explicit mat4(const quat &other) { *this = mat4(mat3(other)); }
		explicit inline mat4(const transform &other);

		// compound operators
		mat4 &operator *= (real other) { return *this = *this * other; }
		mat4 &operator += (const mat4 &other) { return *this = *this + other; }
		mat4 &operator *= (const mat4 &other) { return *this = *this * other; }

		// binary operators
		mat4 operator * (real other) const;
		vec4 operator * (const vec4 &other) const;
		mat4 operator + (const mat4 &other) const;
		mat4 operator * (const mat4 &other) const;
		real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }
		real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }

		// comparison operators
		bool operator == (const mat4 &other) const { for (uint8 i = 0; i < 16; i++) if (data[i] != other.data[i]) return false; return true; }
		bool operator != (const mat4 &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { string res = string() + "(" + data[0]; for (uint8 i = 1; i < 16; i++) res += string() + "," + data[i]; return res + ")"; }

		// methods
		mat4 inverse() const;
		mat4 transpose() const;
		mat4 normalize() const;
		real determinant() const;
		bool valid() const;
		static mat4 parse(const string &str);
		static mat4 scale(real scl) { return scale(vec3(scl)); };
		static mat4 scale(const vec3 &scl) { return mat4(scl[0], 0, 0, 0, 0, scl[1], 0, 0, 0, 0, scl[2], 0, 0, 0, 0, 1); };

		// constants
		static mat4 Zero();
		static mat4 Nan();
	};

	inline mat3 inverse(const mat3 &th) { return th.inverse(); }
	inline mat3 transpose(const mat3 &th) { return th.transpose(); }
	inline mat3 normalize(const mat3 &th) { return th.normalize(); }
	inline real determinant(const mat3 &th) { return th.determinant(); }
	inline bool valid(const mat3 &th) { return th.valid(); }
	inline mat4 inverse(const mat4 &th) { return th.inverse(); }
	inline mat4 transpose(const mat4 &th) { return th.transpose(); }
	inline mat4 normalize(const mat4 &th) { return th.normalize(); }
	inline real determinant(const mat4 &th) { return th.determinant(); }
	inline bool valid(const mat4 &th) { return th.valid(); }

	CAGE_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation);
	CAGE_API mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far);
	CAGE_API mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far);

	inline vec3 vec3::operator * (const mat3 &other) const { return other * *this; }
	inline vec4 vec4::operator * (const mat4 &other) const { return other * *this; }

	template<uint32 N> struct matN {};
	template<> struct matN<3> { typedef mat3 type; };
	template<> struct matN<4> { typedef mat4 type; };

	// transform

	struct CAGE_API transform
	{
		// data
		quat orientation;
		vec3 position;
		real scale;

		// constructors
		transform();
		explicit transform(const vec3 &position, const quat &orientation = quat(), real scale = 1);

		// compound operators
		transform &operator *= (const transform &other) { return *this = *this * other; }
		transform &operator *= (const quat &other) { return *this = *this * other; }
		transform &operator += (const vec3 &other) { return *this = *this + other; }

		// binary operators
		transform operator * (const transform &other) const;
		transform operator * (const quat &other) const;
		transform operator + (const vec3 &other) const;

		// comparison operators
		bool operator == (const transform &other) const { return orientation == other.orientation && position == other.position && scale == other.scale; }
		bool operator != (const transform &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + position + ", " + orientation + ", " + scale + ")"; }

		// methods
		transform inverse() const;
		bool valid() const { return orientation.valid() && position.valid() && scale.valid(); }
	};

	mat4::mat4(const transform &other)
	{
		*this = mat4(other.position, other.orientation, vec3(other.scale));
	}

	inline transform inverse(const transform &th) { return th.inverse(); }
	inline bool valid(const transform &th) { return th.valid(); }
	CAGE_API transform interpolate(const transform &min, const transform &max, real value);
}

#endif // guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
