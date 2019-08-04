#ifndef guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
#define guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_

#ifndef inline
#ifdef _MSC_VER
#define inline __forceinline
#define GCHL_UNDEF_INLINE
#endif // _MSC_VER
#endif

namespace cage
{
	struct CAGE_API real
	{
		float value;

		inline real() : value(0) {}
#define GCHL_GENERATE(TYPE) inline real (TYPE other) : value((float)other) {}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE
		explicit real(rads other);
		explicit real(degs other);

		static real parse(const string &str);
		inline operator string() const { return string(value); }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx == 0, "index out of range", idx); return *this; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx == 0, "index out of range", idx); return *this; }
		bool valid() const;
		inline bool finite() const { return valid() && value != real::Infinity().value && value != -real::Infinity().value; }
		inline static real Pi() { return 3.141592653589793238; }
		inline static real E() { return 2.718281828459045235; }
		inline static real Log2() { return 0.693147180559945309; }
		inline static real Log10() { return 2.302585092994045684; }
		static real Infinity();
		static real Nan();
		static const uint32 Dimension = 1;
	};

	struct CAGE_API rads
	{
		real value;

		inline explicit rads() {}
		inline explicit rads(real value) : value(value) {}
		rads(degs other);

		static rads parse(const string &str);
		inline operator string() const { return string() + value + " rads"; }
		inline bool valid() const { return value.valid(); }
		inline static rads Full() { return rads(real::Pi().value * 2); };
		inline static rads Nan() { return rads(real::Nan()); };
		friend struct real;
		friend struct degs;
	};

	struct CAGE_API degs
	{
		real value;

		inline degs() {}
		inline explicit degs(real value) : value(value) {}
		degs(rads other);

		static degs parse(const string &str);
		inline operator string() const { return string() + value + " degs"; }
		inline bool valid() const { return value.valid(); }
		inline static degs Full() { return degs(360); };
		inline static degs Nan() { return degs(real::Nan()); };
		friend struct real;
		friend struct rads;
	};

	struct CAGE_API vec2
	{
		real data[2];

		inline vec2() : vec2(0) {}
		inline explicit vec2(real value) : data{ value, value} {}
		inline explicit vec2(real x, real y) : data{ x, y } {}
		inline explicit vec2(const vec3 &v);
		inline explicit vec2(const vec4 &v);

		static vec2 parse(const string &str);
		inline operator string() const { return string("(") + data[0] + "," + data[1] + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 2, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 2, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static vec2 Nan() { return vec2(real::Nan()); }
		static const uint32 Dimension = 2;
	};

	struct CAGE_API vec3
	{
		real data[3];

		inline vec3() : vec3(0) {}
		inline explicit vec3(real value) : data{ value, value, value } {}
		inline explicit vec3(real x, real y, real z) : data{ x, y, z } {}
		inline explicit vec3(const vec2 &v, real z) : data{ v[0], v[1], z } {}
		inline explicit vec3(const vec4 &v);

		static vec3 parse(const string &str);
		inline operator string() const { return string("(") + data[0] + "," + data[1] + "," + data[2] + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 3, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 3, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static vec3 Nan() { return vec3(real::Nan()); }
		static const uint32 Dimension = 3;
	};

	struct CAGE_API vec4
	{
		real data[4];

		inline vec4() : vec4(0) {}
		inline explicit vec4(real value) : data{ value, value, value, value } {}
		inline explicit vec4(real x, real y, real z, real w) : data{ x, y, z, w } {}
		inline explicit vec4(const vec2 &v, real z, real w) : data{ v[0], v[1], z, w } {}
		inline explicit vec4(const vec2 &v, const vec2 &w) : data{ v[0], v[1], w[0], w[1] } {}
		inline explicit vec4(const vec3 &v, real w) : data{ v[0], v[1], v[2], w } {}

		static vec4 parse(const string &str);
		inline operator string() const { return string("(") + data[0] + "," + data[1] + "," + data[2] + "," + data[3] + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 4, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 4, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static vec4 Nan() { return vec4(real::Nan()); }
		static const uint32 Dimension = 4;
	};

	struct CAGE_API quat
	{
		real data[4]; // x, y, z, w

		inline quat() : quat(0, 0, 0, 1) {}
		inline explicit quat(real x, real y, real z, real w) : data{ x, y, z, w } {}
		explicit quat(rads pitch, rads yaw, rads roll);
		explicit quat(const vec3 &axis, rads angle);
		explicit quat(const mat3 &rot);
		explicit quat(const vec3 &forward, const vec3 &up, bool keepUp = false);

		static quat parse(const string &str);
		inline operator string() const { return string("(") + data[0] + "," + data[1] + "," + data[2] + "," + data[3] + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 4, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 4, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static quat Nan() { return quat(real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
		static const uint32 Dimension = 4;
	};

	struct CAGE_API mat3
	{
		real data[9];

		inline mat3() : mat3(1, 0, 0, 0, 1, 0, 0, 0, 1) {}
		inline explicit mat3(real a, real b, real c, real d, real e, real f, real g, real h, real i) : data{ a, b, c, d, e, f, g, h, i } {}
		explicit mat3(const vec3 &forward, const vec3 &up, bool keepUp = false);
		explicit mat3(const quat &other);
		explicit mat3(const mat4 &other);

		static mat3 parse(const string &str);
		inline operator string() const { string res = string() + "(" + data[0]; for (uint32 i = 1; i < 9; i++) res += string(",") + data[i]; return res + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static mat3 Zero() { return mat3(0, 0, 0, 0, 0, 0, 0, 0, 0); }
		inline static mat3 Nan() { return mat3(real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	};

	struct CAGE_API mat4
	{
		real data[16]; // SRR0 RSR0 RRS0 TTT1

		inline mat4() : data{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } {}
		inline explicit mat4(real a, real b, real c, real d, real e, real f, real g, real h, real i, real j, real k, real l, real m, real n, real o, real p) : data{ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p } {}
		inline explicit mat4(const mat3 &other) : data{ other[0], other[1], other[2], 0, other[3], other[4], other[5], 0, other[6], other[7], other[8], 0, 0, 0, 0, 1 } {}
		inline explicit mat4(const vec3 &position) : data{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, position[0], position[1], position[2], 1 } {}
		explicit mat4(const vec3 &position, const quat &orientation, const vec3 &scale = vec3(1));
		inline explicit mat4(const quat &orientation) : mat4(mat3(orientation)) {}
		explicit mat4(const transform &other);

		inline static mat4 scale(real scl) { return scale(vec3(scl)); };
		inline static mat4 scale(const vec3 &scl) { return mat4(scl[0], 0, 0, 0, 0, scl[1], 0, 0, 0, 0, scl[2], 0, 0, 0, 0, 1); }
		static mat4 parse(const string &str);
		inline operator string() const { string res = string() + "(" + data[0]; for (uint32 i = 1; i < 16; i++) res += string(",") + data[i]; return res + ")"; }
		inline real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }
		inline real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }
		inline bool valid() const { for (real d : data) if (!d.valid()) return false; return true; }
		inline static mat4 Zero() { return mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); }
		inline static mat4 Nan() { return mat4(real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	};

	struct CAGE_API transform
	{
		quat orientation;
		vec3 position;
		real scale;

		inline transform() : scale(1) {}
		inline explicit transform(const vec3 &position, const quat &orientation = quat(), real scale = 1) : orientation(orientation), position(position), scale(scale) {}

		static transform parse(const string &str);
		inline operator string() const { return string() + "(" + position + "," + orientation + "," + scale + ")"; }
		inline bool valid() const { return orientation.valid() && position.valid() && scale.valid(); }
	};

#define GCHL_GENERATE(OPERATOR) \
	inline bool operator OPERATOR (const real &l, const real &r) { return l.value OPERATOR r.value; }; \
	inline bool operator OPERATOR (const rads &l, const rads &r) { return l.value OPERATOR r.value; }; \
	inline bool operator OPERATOR (const degs &l, const degs &r) { return l.value OPERATOR r.value; }; \
	inline bool operator OPERATOR (const rads &l, const degs &r) { return l OPERATOR rads(r); }; \
	inline bool operator OPERATOR (const degs &l, const rads &r) { return rads(l) OPERATOR r; }; \
	inline bool operator OPERATOR (const vec2 &l, const vec2 &r) { return l.data[0] OPERATOR r.data[0] && l.data[1] OPERATOR r.data[1]; }; \
	inline bool operator OPERATOR (const vec3 &l, const vec3 &r) { return l.data[0] OPERATOR r.data[0] && l.data[1] OPERATOR r.data[1] && l.data[2] OPERATOR r.data[2]; }; \
	inline bool operator OPERATOR (const vec4 &l, const vec4 &r) { return l.data[0] OPERATOR r.data[0] && l.data[1] OPERATOR r.data[1] && l.data[2] OPERATOR r.data[2] && l.data[3] OPERATOR r.data[3]; };
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, ==, <, >, <=, >= ));
#undef GCHL_GENERATE
	inline bool operator == (const quat &l, const quat &r) { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2] && l.data[3] == r.data[3]; };
	inline bool operator == (const mat3 &l, const mat3 &r) { for (uint32 i = 0; i < 9; i++) if (!(l[i] == r[i])) return false; return true; };
	inline bool operator == (const mat4 &l, const mat4 &r) { for (uint32 i = 0; i < 16; i++) if (!(l[i] == r[i])) return false; return true; };
	inline bool operator == (const transform &l, const transform &r) { return l.orientation == r.orientation && l.position == r.position && l.scale == r.scale; };
#define GCHL_GENERATE(TYPE) \
	inline bool operator != (const TYPE &l, const TYPE &r) { return !(l == r); };
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, real, rads, degs, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline real operator OPERATOR (const real &r) { return OPERATOR r.value; } \
	inline rads operator OPERATOR (const rads &r) { return rads(OPERATOR r.value); } \
	inline degs operator OPERATOR (const degs &r) { return degs(OPERATOR r.value); } \
	inline vec2 operator OPERATOR (const vec2 &r) { return vec2(OPERATOR r[0], OPERATOR r[1]); } \
	inline vec3 operator OPERATOR (const vec3 &r) { return vec3(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2]); } \
	inline vec4 operator OPERATOR (const vec4 &r) { return vec4(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); } \
	inline quat operator OPERATOR (const quat &r) { return quat(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, - ));
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline real operator OPERATOR (const real &l, const real &r) { return l.value OPERATOR r.value; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / ));
#undef GCHL_GENERATE
	inline real operator % (const real &l, const real &r) { CAGE_ASSERT_RUNTIME(r.value != 0); return l - (r * (sint32)(l / r).value); }
#define GCHL_GENERATE(OPERATOR) \
	inline rads operator OPERATOR (const rads &l, const rads &r) { return rads(l.value OPERATOR r.value); } \
	inline rads operator OPERATOR (const rads &l, const real &r) { return rads(l.value OPERATOR r); } \
	inline rads operator OPERATOR (const real &l, const rads &r) { return rads(l OPERATOR r.value); } \
	inline degs operator OPERATOR (const degs &l, const degs &r) { return degs(l.value OPERATOR r.value); } \
	inline degs operator OPERATOR (const degs &l, const real &r) { return degs(l.value OPERATOR r); } \
	inline degs operator OPERATOR (const real &l, const degs &r) { return degs(l OPERATOR r.value); } \
	inline rads operator OPERATOR (const rads &l, const degs &r) { return l OPERATOR rads(r); } \
	inline rads operator OPERATOR (const degs &l, const rads &r) { return rads(l) OPERATOR r; } \
	inline vec2 operator OPERATOR (const vec2 &l, const vec2 &r) { return vec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); } \
	inline vec2 operator OPERATOR (const vec2 &l, const real &r) { return vec2(l[0] OPERATOR r, l[1] OPERATOR r); } \
	inline vec2 operator OPERATOR (const real &l, const vec2 &r) { return vec2(l OPERATOR r[0], l OPERATOR r[1]); } \
	inline vec3 operator OPERATOR (const vec3 &l, const vec3 &r) { return vec3(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2]); } \
	inline vec3 operator OPERATOR (const vec3 &l, const real &r) { return vec3(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r); } \
	inline vec3 operator OPERATOR (const real &l, const vec3 &r) { return vec3(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2]); } \
	inline vec4 operator OPERATOR (const vec4 &l, const vec4 &r) { return vec4(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); } \
	inline vec4 operator OPERATOR (const vec4 &l, const real &r) { return vec4(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); } \
	inline vec4 operator OPERATOR (const real &l, const vec4 &r) { return vec4(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, /, % ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	inline quat operator OPERATOR (const quat &l, const quat &r) { return quat(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); } \
	inline quat operator OPERATOR (const real &l, const quat &r) { return quat(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); } \
	inline quat operator OPERATOR (const quat &l, const real &r) { return quat(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, / ));
#undef GCHL_GENERATE
	CAGE_API quat operator * (const quat &l, const quat &r);
	inline quat operator * (const real &l, const quat &r) { return quat(l * r[0], l * r[1], l * r[2], l * r[3]); }
	inline quat operator * (const quat &l, const real &r) { return quat(l[0] * r, l[1] * r, l[2] * r, l[3] * r); }
#define GCHL_GENERATE(OPERATOR) \
	CAGE_API mat3 operator OPERATOR (const mat3 &l, const mat3 &r); \
	inline mat3 operator OPERATOR (const mat3 &l, const real &r) { mat3 res; for (uint32 i = 0; i < 9; i++) res[i] = l[i] OPERATOR r; return res; } \
	inline mat3 operator OPERATOR (const real &l, const mat3 &r) { mat3 res; for (uint32 i = 0; i < 9; i++) res[i] = l OPERATOR r[i]; return res; } \
	CAGE_API mat4 operator OPERATOR (const mat4 &l, const mat4 &r); \
	inline mat4 operator OPERATOR (const mat4 &l, const real &r) { mat4 res; for (uint32 i = 0; i < 16; i++) res[i] = l[i] OPERATOR r; return res; } \
	inline mat4 operator OPERATOR (const real &l, const mat4 &r) { mat4 res; for (uint32 i = 0; i < 16; i++) res[i] = l OPERATOR r[i]; return res; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, * ));
#undef GCHL_GENERATE
	CAGE_API vec3 operator * (const transform &l, const vec3 &r);
	CAGE_API vec3 operator * (const vec3 &l, const transform &r);
	CAGE_API vec3 operator * (const vec3 &l, const quat &r);
	CAGE_API vec3 operator * (const quat &l, const vec3 &r);
	CAGE_API vec3 operator * (const vec3 &l, const mat3 &r);
	CAGE_API vec3 operator * (const mat3 &l, const vec3 &r);
	CAGE_API vec4 operator * (const vec4 &l, const mat4 &r);
	CAGE_API vec4 operator * (const mat4 &l, const vec4 &r);
	CAGE_API transform operator * (const transform &l, const transform &r);
	CAGE_API transform operator * (const transform &l, const quat &r);
	CAGE_API transform operator * (const quat &l, const transform &r);
	CAGE_API transform operator * (const transform &l, const real &r);
	CAGE_API transform operator * (const real &l, const transform &r);
	CAGE_API transform operator + (const transform &l, const vec3 &r);
	CAGE_API transform operator + (const vec3 &l, const transform &r);

#define GCHL_GENERATE(OPERATOR) \
	inline real &operator OPERATOR##= (real &l, const real &r) { return l = l OPERATOR r; } \
	inline rads &operator OPERATOR##= (rads &l, const rads &r) { return l = l OPERATOR r; } \
	inline rads &operator OPERATOR##= (rads &l, const real &r) { return l = l OPERATOR r; } \
	inline degs &operator OPERATOR##= (degs &l, const degs &r) { return l = l OPERATOR r; } \
	inline degs &operator OPERATOR##= (degs &l, const real &r) { return l = l OPERATOR r; } \
	inline vec2 &operator OPERATOR##= (vec2 &l, const vec2 &r) { return l = l OPERATOR r; } \
	inline vec2 &operator OPERATOR##= (vec2 &l, const real &r) { return l = l OPERATOR r; } \
	inline vec3 &operator OPERATOR##= (vec3 &l, const vec3 &r) { return l = l OPERATOR r; } \
	inline vec3 &operator OPERATOR##= (vec3 &l, const real &r) { return l = l OPERATOR r; } \
	inline vec4 &operator OPERATOR##= (vec4 &l, const vec4 &r) { return l = l OPERATOR r; } \
	inline vec4 &operator OPERATOR##= (vec4 &l, const real &r) { return l = l OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, /, % ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	inline quat &operator OPERATOR##= (quat &l, const quat &r) { return l = l OPERATOR r; } \
	inline quat &operator OPERATOR##= (quat &l, const real &r) { return l = l OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, -, *, / ));
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	inline mat3 &operator OPERATOR##= (mat3 &l, const mat3 &r) { return l = l OPERATOR r; } \
	inline mat3 &operator OPERATOR##= (mat3 &l, const real &r) { return l = l OPERATOR r; } \
	inline mat4 &operator OPERATOR##= (mat4 &l, const mat4 &r) { return l = l OPERATOR r; } \
	inline mat4 &operator OPERATOR##= (mat4 &l, const real &r) { return l = l OPERATOR r; }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, +, * ));
#undef GCHL_GENERATE
	inline vec3 &operator *= (vec3 &l, const mat3 &r) { return l = l *= r; }
	inline vec3 &operator *= (vec3 &l, const quat &r) { return l = l *= r; }
	inline vec3 &operator *= (vec3 &l, const transform &r) { return l = l * r; }
	inline vec4 &operator *= (vec4 &l, const mat4 &r) { return l = l *= r; }
	inline transform &operator *= (transform &l, const transform &r) { return l = l * r; }
	inline transform &operator *= (transform &l, const quat &r) { return l = l * r; }
	inline transform &operator *= (transform &l, const real &r) { return l = l * r; }
	inline transform &operator += (transform &l, const vec3 &r) { return l = l + r; }

	inline real::real(rads other) : value(other.value.value) {}
	inline real::real(degs other) : value(other.value.value) {}
	inline rads::rads(degs other) : value(other.value * real::Pi() / 180) {}
	inline degs::degs(rads other) : value(other.value * 180 / real::Pi()) {}
	inline vec2::vec2(const vec3 &other) : data{ other.data[0], other.data[1] } {}
	inline vec2::vec2(const vec4 &other) : data{ other.data[0], other.data[1] } {}
	inline vec3::vec3(const vec4 &other) : data{ other.data[0], other.data[1], other.data[2] } {}
	inline mat3::mat3(const mat4 &other) : data{ other[0], other[1], other[2], other[4], other[5], other[6], other[8], other[9], other[10] } {}
	inline mat4::mat4(const transform &other) : mat4(other.position, other.orientation, vec3(other.scale)) {}

	CAGE_API bool valid(float a);
	CAGE_API bool valid(double a);
#define GCHL_GENERATE(TYPE) \
	inline bool valid(const TYPE &a) { return a.valid(); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, real, rads, degs, vec2, vec3, vec4, quat, mat3, mat4));
#undef GCHL_GENERATE
	inline real smoothstep(real x) { return x * x * (3 - 2 * x); }
	inline real smootherstep(real x) { return x * x * x * (x * (x * 6 - 15) + 10); }
	inline real abs(real x) { return x < 0 ? -x : x; }
	inline sint32 sign(real x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }
	inline rads normalize(rads x) { if (x < rads()) return rads::Full() - -x % rads::Full(); else return x % rads::Full(); }
	inline degs normalize(degs x) { if (x < degs()) return degs::Full() - -x % degs::Full(); else return x % degs::Full(); }
	inline rads abs(rads x) { return rads(abs(x.value)); }
	inline degs abs(degs x) { return degs(abs(x.value)); }
	inline sint32 sign(rads x) { return sign(x.value); }
	inline sint32 sign(degs x) { return sign(x.value); }
	inline real sqr(real x) { return x * x; }
	CAGE_API real sqrt(real x);
	CAGE_API real pow(real base, real exponent); // base ^ exponent
	CAGE_API real powE(real x); // e ^ x
	CAGE_API real pow2(real x); // 2 ^ x
	CAGE_API real pow10(real x); // 10 ^ x
	CAGE_API real log(real x);
	CAGE_API real log2(real x);
	CAGE_API real log10(real x);
	CAGE_API real round(real x);
	CAGE_API real floor(real x);
	CAGE_API real ceil(real x);
	CAGE_API real sin(rads value);
	CAGE_API real cos(rads value);
	CAGE_API real tan(rads value);
	CAGE_API rads asin(real value);
	CAGE_API rads acos(real value);
	CAGE_API rads atan(real value);
	CAGE_API rads atan2(real x, real y);

#define GCHL_GENERATE(TYPE) \
	inline TYPE min(TYPE a, TYPE b) { return a < b ? a : b; } \
	inline TYPE max(TYPE a, TYPE b) { return a > b ? a : b; } \
	inline TYPE clamp(TYPE v, TYPE a, TYPE b) { CAGE_ASSERT_RUNTIME(a <= b); return min(max(v, a), b); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double, real, rads, degs));
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	inline TYPE min(const TYPE &l, const TYPE &r) { TYPE res; for (uint32 i = 0; i < TYPE::Dimension; i++) res[i] = min(l[i], r[i]); return res; } \
	inline TYPE max(const TYPE &l, const TYPE &r) { TYPE res; for (uint32 i = 0; i < TYPE::Dimension; i++) res[i] = max(l[i], r[i]); return res; } \
	inline TYPE min(const TYPE &l, real r) { return min(l, TYPE(r)); } \
	inline TYPE max(const TYPE &l, real r) { return max(l, TYPE(r)); } \
	inline TYPE min(real l, const TYPE &r) { return min(TYPE(l), r); } \
	inline TYPE max(real l, const TYPE &r) { return max(TYPE(l), r); } \
	inline TYPE clamp(const TYPE &v, const TYPE &a, const TYPE &b) { TYPE res; for (uint32 i = 0; i < TYPE::Dimension; i++) res[i] = clamp(v[i], a[i], b[i]); return res; } \
	inline TYPE clamp(const TYPE &v, real a, real b) { return clamp(v, TYPE(a), TYPE(b)); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, vec2, vec3, vec4));
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	inline real dot(const TYPE &l, const TYPE &r) { TYPE m = l * r; real sum = 0; for (uint32 i = 0; i < TYPE::Dimension; i++) sum += m[i]; return sum; } \
	inline real squaredLength(const TYPE &x) { return dot(x, x); } \
	inline real squaredDistance(const TYPE &l, const TYPE &r) { return squaredLength(l - r); } \
	inline real length(const TYPE &x) { return sqrt(squaredLength(x)); } \
	inline real distance(const TYPE &l, const TYPE &r) { return sqrt(squaredDistance(l, r)); } \
	inline TYPE normalize(const TYPE &x) { return x / length(x); } \
	inline TYPE abs(const TYPE &x) { return max(x, -x); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, vec2, vec3, vec4));
#undef GCHL_GENERATE
	inline real dot(const quat &l, const quat &r) { return dot((vec4&)l, (vec4&)r); }
	inline real squaredLength(const quat &x) { return dot(x, x); }
	inline real length(const quat &x) { return sqrt(squaredLength(x)); }
	inline quat normalize(const quat &x) { return x / length(x); }
	inline real cross(const vec2 &l, const vec2 &r) { return l[0] * r[1] - l[1] * r[0]; }
	inline vec3 cross(const vec3 &l, const vec3 &r) { return vec3(l[1] * r[2] - l[2] * r[1], l[2] * r[0] - l[0] * r[2], l[0] * r[1] - l[1] * r[0]); }
	inline quat conjugate(const quat &x) { return quat(-x[0], -x[1], -x[2], x[3]); }
	CAGE_API vec3 primaryAxis(vec3 &x);
	CAGE_API void toAxisAngle(const quat &x, vec3 &axis, rads &angle);
	CAGE_API quat lerp(const quat &a, const quat &b, real f);
	CAGE_API quat slerp(const quat &a, const quat &b, real f);
	CAGE_API quat slerpPrecise(const quat &a, const quat &b, real f);
	CAGE_API quat rotate(const quat &from, const quat &toward, rads maxTurn);

	CAGE_API real randomChance(); // 0 to 1; including 0, excluding 1
#define GCHL_GENERATE(TYPE) \
	CAGE_API TYPE randomRange(TYPE min, TYPE max);
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double, real, rads, degs));
#undef GCHL_GENERATE
	CAGE_API rads randomAngle();
	inline vec2 randomChance2() { return vec2(randomChance(), randomChance()); }
	inline vec2 randomRange2(real a, real b) { return vec2(randomRange(a, b), randomRange(a, b)); }
	CAGE_API vec2 randomDirection2();
	inline vec3 randomChance3() { return vec3(randomChance(), randomChance(), randomChance()); }
	inline vec3 randomRange3(real a, real b) { return vec3(randomRange(a, b), randomRange(a, b), randomRange(a, b)); }
	CAGE_API vec3 randomDirection3();
	inline vec4 randomChance4() { return vec4(randomChance(), randomChance(), randomChance(), randomChance()); }
	inline vec4 randomRange4(real a, real b) { return vec4(randomRange(a, b), randomRange(a, b), randomRange(a, b), randomRange(a, b)); }
	CAGE_API quat randomDirectionQuat();

#define GCHL_GENERATE(TYPE) \
	inline TYPE interpolate(TYPE a, TYPE b, real f) { return (TYPE)(a * (1 - f.value) + b * f.value); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64));
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	inline TYPE interpolate(TYPE a, TYPE b, real f) { return (TYPE)(a + (b - a) * f.value); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, float, double, real, rads, degs, vec2, vec3, vec4));
#undef GCHL_GENERATE
	inline quat interpolate(const quat &a, const quat &b, real f) { return slerp(a, b, f); }
	inline transform interpolate(const transform &a, const transform &b, real f) { return transform(interpolate(a.position, b.position, f), interpolate(a.orientation, b.orientation, f), interpolate(a.scale, b.scale, f)); }

	CAGE_API mat3 inverse(const mat3 &x);
	CAGE_API mat3 transpose(const mat3 &x);
	CAGE_API mat3 normalize(const mat3 &x);
	CAGE_API real determinant(const mat3 &x);
	CAGE_API mat4 inverse(const mat4 &x);
	CAGE_API mat4 transpose(const mat4 &x);
	CAGE_API mat4 normalize(const mat4 &x);
	CAGE_API real determinant(const mat4 &x);
	CAGE_API transform inverse(const transform &x);
	CAGE_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation);
	CAGE_API mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far);
	CAGE_API mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far);

	/*
	namespace detail
	{
		inline uint32 rotl32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value << shift) | (value >> (32 - shift)); }
		inline uint32 rotr32(uint32 value, sint8 shift) { CAGE_ASSERT_RUNTIME(shift > -32 && shift < 32, "shift too big", shift); return (value >> shift) | (value << (32 - shift)); }
	}
	*/

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

	/*
	template<uint32 N> struct vecN {};
	template<> struct vecN<1> { typedef real type; };
	template<> struct vecN<2> { typedef vec2 type; };
	template<> struct vecN<3> { typedef vec3 type; };
	template<> struct vecN<4> { typedef vec4 type; };
	template<uint32 N> struct matN {};
	template<> struct matN<3> { typedef mat3 type; };
	template<> struct matN<4> { typedef mat4 type; };
	*/
}

#ifdef GCHL_UNDEF_INLINE
#undef GCHL_UNDEF_INLINE
#undef inline
#endif

#endif // guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
