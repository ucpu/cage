#ifndef guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
#define guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_

#include "core.h"

#define GCHL_DIMENSION(TYPE) (sizeof(TYPE::data) / sizeof(TYPE::data[0]))

namespace cage
{
	struct CAGE_CORE_API real
	{
		using value_type = float;

		value_type value = 0;

		CAGE_FORCE_INLINE constexpr real() noexcept {}
#define GCHL_GENERATE(TYPE) CAGE_FORCE_INLINE constexpr real (TYPE other) noexcept : value((value_type)other) {}
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
#undef GCHL_GENERATE
		explicit constexpr real(rads other) noexcept;
		explicit constexpr real(degs other) noexcept;

		static real parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx == 0); return *this; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx == 0); return *this; }
		bool valid() const noexcept;
		bool finite() const noexcept;
		static constexpr real Pi() noexcept { return (real)3.141592653589793238; }
		static constexpr real E() noexcept { return (real)2.718281828459045235; }
		static constexpr real Log2() noexcept { return (real)0.693147180559945309; }
		static constexpr real Log10() noexcept { return (real)2.302585092994045684; }
		static constexpr real Infinity() noexcept { return std::numeric_limits<value_type>::infinity(); }
		static constexpr real Nan() noexcept { return std::numeric_limits<value_type>::quiet_NaN(); }
	};
}

namespace std
{
	template<>
	struct numeric_limits<cage::real> : public std::numeric_limits<cage::real::value_type>
	{};
}

namespace cage
{
	template<class To>
	CAGE_FORCE_INLINE constexpr To numeric_cast(real from)
	{
		return numeric_cast<To>(from.value);
	}

	struct CAGE_CORE_API rads
	{
		real value;

		CAGE_FORCE_INLINE constexpr rads() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr rads(real value) noexcept : value(value) {}
		constexpr rads(degs other) noexcept;

		static rads parse(const string &str);
		CAGE_FORCE_INLINE bool valid() const noexcept { return value.valid(); }
		CAGE_FORCE_INLINE static constexpr rads Full() noexcept { return rads(real::Pi().value * 2); }
		CAGE_FORCE_INLINE static constexpr rads Nan() noexcept { return rads(real::Nan()); }
		friend struct real;
		friend struct degs;
	};

	struct CAGE_CORE_API degs
	{
		real value;

		CAGE_FORCE_INLINE constexpr degs() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr degs(real value) noexcept : value(value) {}
		constexpr degs(rads other) noexcept;

		static degs parse(const string &str);
		CAGE_FORCE_INLINE bool valid() const noexcept { return value.valid(); }
		CAGE_FORCE_INLINE static constexpr degs Full() noexcept { return degs(360); }
		CAGE_FORCE_INLINE static constexpr degs Nan() noexcept { return degs(real::Nan()); }
		friend struct real;
		friend struct rads;
	};

	struct CAGE_CORE_API vec2
	{
		real data[2];

		CAGE_FORCE_INLINE constexpr vec2() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr vec2(real value) noexcept : data{ value, value } {}
		CAGE_FORCE_INLINE explicit constexpr vec2(real x, real y) noexcept : data{ x, y } {}
		CAGE_FORCE_INLINE explicit constexpr vec2(const vec3 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr vec2(const vec4 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr vec2(const ivec2 &v) noexcept;

		static vec2 parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 2); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 2); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr vec2 Nan() noexcept { return vec2(real::Nan()); }
	};

	struct CAGE_CORE_API ivec2
	{
		sint32 data[2] = {};

		CAGE_FORCE_INLINE constexpr ivec2() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr ivec2(sint32 value) noexcept : data{ value, value } {}
		CAGE_FORCE_INLINE explicit constexpr ivec2(sint32 x, sint32 y) noexcept : data{ x, y } {}
		CAGE_FORCE_INLINE explicit constexpr ivec2(const ivec3 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr ivec2(const ivec4 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr ivec2(const vec2 &v) noexcept;

		static ivec2 parse(const string &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator [] (uint32 idx) { CAGE_ASSERT(idx < 2); return data[idx]; }
		CAGE_FORCE_INLINE constexpr sint32 operator [] (uint32 idx) const { CAGE_ASSERT(idx < 2); return data[idx]; }
	};

	struct CAGE_CORE_API vec3
	{
		real data[3];

		CAGE_FORCE_INLINE constexpr vec3() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr vec3(real value) noexcept : data{ value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr vec3(real x, real y, real z) noexcept : data{ x, y, z } {}
		CAGE_FORCE_INLINE explicit constexpr vec3(const vec2 &v, real z) noexcept : data{ v[0], v[1], z } {}
		CAGE_FORCE_INLINE explicit constexpr vec3(const vec4 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr vec3(const ivec3 &v) noexcept;

		static vec3 parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 3); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 3); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr vec3 Nan() noexcept { return vec3(real::Nan()); }
	};

	struct CAGE_CORE_API ivec3
	{
		sint32 data[3] = {};

		CAGE_FORCE_INLINE constexpr ivec3() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr ivec3(sint32 value) noexcept : data{ value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr ivec3(sint32 x, sint32 y, sint32 z) noexcept : data{ x, y, z } {}
		CAGE_FORCE_INLINE explicit constexpr ivec3(const ivec2 &v, sint32 z) noexcept : data{ v[0], v[1], z } {}
		CAGE_FORCE_INLINE explicit constexpr ivec3(const ivec4 &v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr ivec3(const vec3 &v) noexcept;

		static ivec3 parse(const string &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator [] (uint32 idx) { CAGE_ASSERT(idx < 3); return data[idx]; }
		CAGE_FORCE_INLINE constexpr sint32 operator [] (uint32 idx) const { CAGE_ASSERT(idx < 3); return data[idx]; }
	};

	struct CAGE_CORE_API vec4
	{
		real data[4];

		CAGE_FORCE_INLINE constexpr vec4() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr vec4(real value) noexcept : data{ value, value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr vec4(real x, real y, real z, real w) noexcept : data{ x, y, z, w } {}
		CAGE_FORCE_INLINE explicit constexpr vec4(const vec2 &v, real z, real w) noexcept : data{ v[0], v[1], z, w } {}
		CAGE_FORCE_INLINE explicit constexpr vec4(const vec2 &v, const vec2 &w) noexcept : data{ v[0], v[1], w[0], w[1] } {}
		CAGE_FORCE_INLINE explicit constexpr vec4(const vec3 &v, real w) noexcept : data{ v[0], v[1], v[2], w } {}
		CAGE_FORCE_INLINE explicit constexpr vec4(const ivec4 &v) noexcept;

		static vec4 parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 4); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 4); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr vec4 Nan() noexcept { return vec4(real::Nan()); }
	};

	struct CAGE_CORE_API ivec4
	{
		sint32 data[4] = {};

		CAGE_FORCE_INLINE constexpr ivec4() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(sint32 value) noexcept : data{ value, value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(sint32 x, sint32 y, sint32 z, sint32 w) noexcept : data{ x, y, z, w } {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(const ivec2 &v, sint32 z, sint32 w) noexcept : data{ v[0], v[1], z, w } {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(const ivec2 &v, const ivec2 &w) noexcept : data{ v[0], v[1], w[0], w[1] } {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(const ivec3 &v, sint32 w) noexcept : data{ v[0], v[1], v[2], w } {}
		CAGE_FORCE_INLINE explicit constexpr ivec4(const vec4 &v) noexcept;

		static ivec4 parse(const string &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator [] (uint32 idx) { CAGE_ASSERT(idx < 4); return data[idx]; }
		CAGE_FORCE_INLINE constexpr sint32 operator [] (uint32 idx) const { CAGE_ASSERT(idx < 4); return data[idx]; }
	};

	struct CAGE_CORE_API quat
	{
		real data[4] = { 0, 0, 0, 1 }; // x, y, z, w

		CAGE_FORCE_INLINE constexpr quat() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr quat(real x, real y, real z, real w) noexcept : data{ x, y, z, w } {}
		explicit quat(rads pitch, rads yaw, rads roll);
		explicit quat(const vec3 &axis, rads angle);
		explicit quat(const mat3 &rot);
		explicit quat(const vec3 &forward, const vec3 &up, bool keepUp = false);

		static quat parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 4); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 4); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr quat Nan() noexcept { return quat(real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	};

	struct CAGE_CORE_API mat3
	{
		real data[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

		CAGE_FORCE_INLINE constexpr mat3() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr mat3(real a, real b, real c, real d, real e, real f, real g, real h, real i) noexcept : data{ a, b, c, d, e, f, g, h, i } {}
		explicit mat3(const vec3 &forward, const vec3 &up, bool keepUp = false);
		explicit mat3(const quat &other) noexcept;
		explicit constexpr mat3(const mat4 &other) noexcept;

		static mat3 parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 9); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 9); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr mat3 Zero() noexcept { return mat3(0, 0, 0, 0, 0, 0, 0, 0, 0); }
		CAGE_FORCE_INLINE static constexpr mat3 Nan() noexcept { return mat3(real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	};

	struct CAGE_CORE_API mat4
	{
		real data[16]  = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }; // SRR0 RSR0 RRS0 TTT1

		CAGE_FORCE_INLINE constexpr mat4() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr mat4(real a, real b, real c, real d, real e, real f, real g, real h, real i, real j, real k, real l, real m, real n, real o, real p) noexcept : data{ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p } {}
		CAGE_FORCE_INLINE explicit constexpr mat4(const mat3 &other) noexcept : data{ other[0], other[1], other[2], 0, other[3], other[4], other[5], 0, other[6], other[7], other[8], 0, 0, 0, 0, 1 } {}
		CAGE_FORCE_INLINE explicit constexpr mat4(const vec3 &position) noexcept : data{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, position[0], position[1], position[2], 1 } {}
		explicit mat4(const vec3 &position, const quat &orientation, const vec3 &scale = vec3(1)) noexcept;
		CAGE_FORCE_INLINE explicit mat4(const quat &orientation) noexcept : mat4(mat3(orientation)) {}
		explicit mat4(const transform &other) noexcept;

		CAGE_FORCE_INLINE static constexpr mat4 scale(real scl) noexcept { return scale(vec3(scl)); }
		CAGE_FORCE_INLINE static constexpr mat4 scale(const vec3 &scl) noexcept { return mat4(scl[0], 0, 0, 0, 0, scl[1], 0, 0, 0, 0, scl[2], 0, 0, 0, 0, 1); }
		static mat4 parse(const string &str);
		CAGE_FORCE_INLINE constexpr real &operator [] (uint32 idx) { CAGE_ASSERT(idx < 16); return data[idx]; }
		CAGE_FORCE_INLINE constexpr real operator [] (uint32 idx) const { CAGE_ASSERT(idx < 16); return data[idx]; }
		CAGE_FORCE_INLINE bool valid() const noexcept { for (real d : data) if (!d.valid()) return false; return true; }
		CAGE_FORCE_INLINE static constexpr mat4 Zero() noexcept { return mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); }
		CAGE_FORCE_INLINE static constexpr mat4 Nan() noexcept { return mat4(real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan(), real::Nan()); }
	};

	struct CAGE_CORE_API transform
	{
		quat orientation;
		vec3 position;
		real scale = 1;

		CAGE_FORCE_INLINE constexpr transform() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr transform(const vec3 &position, const quat &orientation = quat(), real scale = 1) noexcept : orientation(orientation), position(position), scale(scale) {}

		static transform parse(const string &str);
		CAGE_FORCE_INLINE bool valid() const noexcept { return orientation.valid() && position.valid() && scale.valid(); }
	};

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr bool operator OPERATOR (const real &l, const real &r) noexcept { return l.value OPERATOR r.value; }; \
	CAGE_FORCE_INLINE constexpr bool operator OPERATOR (const rads &l, const rads &r) noexcept { return l.value OPERATOR r.value; }; \
	CAGE_FORCE_INLINE constexpr bool operator OPERATOR (const degs &l, const degs &r) noexcept { return l.value OPERATOR r.value; }; \
	CAGE_FORCE_INLINE constexpr bool operator OPERATOR (const rads &l, const degs &r) noexcept { return l OPERATOR rads(r); }; \
	CAGE_FORCE_INLINE constexpr bool operator OPERATOR (const degs &l, const rads &r) noexcept { return rads(l) OPERATOR r; };
	GCHL_GENERATE(==);
	GCHL_GENERATE(<=);
	GCHL_GENERATE(>=);
	GCHL_GENERATE(<);
	GCHL_GENERATE(>);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr bool operator == (const vec2 &l, const vec2 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const vec3 &l, const vec3 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const vec4 &l, const vec4 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2] && l.data[3] == r.data[3]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const quat &l, const quat &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2] && l.data[3] == r.data[3]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const mat3 &l, const mat3 &r) noexcept { for (uint32 i = 0; i < 9; i++) if (!(l[i] == r[i])) return false; return true; }
	CAGE_FORCE_INLINE constexpr bool operator == (const mat4 &l, const mat4 &r) noexcept { for (uint32 i = 0; i < 16; i++) if (!(l[i] == r[i])) return false; return true; }
	CAGE_FORCE_INLINE constexpr bool operator == (const transform &l, const transform &r) noexcept { return l.orientation == r.orientation && l.position == r.position && l.scale == r.scale; }
	CAGE_FORCE_INLINE constexpr bool operator == (const ivec2 &l, const ivec2 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const ivec3 &l, const ivec3 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2]; }
	CAGE_FORCE_INLINE constexpr bool operator == (const ivec4 &l, const ivec4 &r) noexcept { return l.data[0] == r.data[0] && l.data[1] == r.data[1] && l.data[2] == r.data[2] && l.data[3] == r.data[3]; }
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr bool operator != (const TYPE &l, const TYPE &r) noexcept { return !(l == r); };
	GCHL_GENERATE(real);
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
	GCHL_GENERATE(quat);
	GCHL_GENERATE(mat3);
	GCHL_GENERATE(mat4);
	GCHL_GENERATE(ivec2);
	GCHL_GENERATE(ivec3);
	GCHL_GENERATE(ivec4);
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr real operator OPERATOR (const real &r) noexcept { return OPERATOR r.value; } \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const rads &r) noexcept { return rads(OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr degs operator OPERATOR (const degs &r) noexcept { return degs(OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr vec2 operator OPERATOR (const vec2 &r) noexcept { return vec2(OPERATOR r[0], OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr vec3 operator OPERATOR (const vec3 &r) noexcept { return vec3(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr vec4 operator OPERATOR (const vec4 &r) noexcept { return vec4(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr quat operator OPERATOR (const quat &r) noexcept { return quat(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr ivec2 operator OPERATOR (const ivec2 &r) noexcept { return ivec2(OPERATOR r[0], OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr ivec3 operator OPERATOR (const ivec3 &r) noexcept { return ivec3(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr ivec4 operator OPERATOR (const ivec4 &r) noexcept { return ivec4(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr real operator OPERATOR (const real &l, const real &r) noexcept { return l.value OPERATOR r.value; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr real operator % (const real &l, const real &r) noexcept { CAGE_ASSERT(r.value != 0); return l - (r * (sint32)(l / r).value); }
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const rads &l, const rads &r) noexcept { return rads(l.value OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const rads &l, const real &r) noexcept { return rads(l.value OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const real &l, const rads &r) noexcept { return rads(l OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr degs operator OPERATOR (const degs &l, const degs &r) noexcept { return degs(l.value OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr degs operator OPERATOR (const degs &l, const real &r) noexcept { return degs(l.value OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr degs operator OPERATOR (const real &l, const degs &r) noexcept { return degs(l OPERATOR r.value); } \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const rads &l, const degs &r) noexcept { return l OPERATOR rads(r); } \
	CAGE_FORCE_INLINE constexpr rads operator OPERATOR (const degs &l, const rads &r) noexcept { return rads(l) OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec2 operator OPERATOR (const vec2 &l, const vec2 &r) noexcept { return vec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr vec2 operator OPERATOR (const vec2 &l, const real &r) noexcept { return vec2(l[0] OPERATOR r, l[1] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr vec2 operator OPERATOR (const real &l, const vec2 &r) noexcept { return vec2(l OPERATOR r[0], l OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr vec3 operator OPERATOR (const vec3 &l, const vec3 &r) noexcept { return vec3(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr vec3 operator OPERATOR (const vec3 &l, const real &r) noexcept { return vec3(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr vec3 operator OPERATOR (const real &l, const vec3 &r) noexcept { return vec3(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr vec4 operator OPERATOR (const vec4 &l, const vec4 &r) noexcept { return vec4(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr vec4 operator OPERATOR (const vec4 &l, const real &r) noexcept { return vec4(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr vec4 operator OPERATOR (const real &l, const vec4 &r) noexcept { return vec4(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr ivec2 operator OPERATOR (const ivec2 &l, const ivec2 &r) noexcept { return ivec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr ivec2 operator OPERATOR (const ivec2 &l, const sint32 &r) noexcept { return ivec2(l[0] OPERATOR r, l[1] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr ivec2 operator OPERATOR (const sint32 &l, const ivec2 &r) noexcept { return ivec2(l OPERATOR r[0], l OPERATOR r[1]); } \
	CAGE_FORCE_INLINE constexpr ivec3 operator OPERATOR (const ivec3 &l, const ivec3 &r) noexcept { return ivec3(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr ivec3 operator OPERATOR (const ivec3 &l, const sint32 &r) noexcept { return ivec3(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr ivec3 operator OPERATOR (const sint32 &l, const ivec3 &r) noexcept { return ivec3(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2]); } \
	CAGE_FORCE_INLINE constexpr ivec4 operator OPERATOR (const ivec4 &l, const ivec4 &r) noexcept { return ivec4(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr ivec4 operator OPERATOR (const ivec4 &l, const sint32 &r) noexcept { return ivec4(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); } \
	CAGE_FORCE_INLINE constexpr ivec4 operator OPERATOR (const sint32 &l, const ivec4 &r) noexcept { return ivec4(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
	GCHL_GENERATE(%);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr quat operator OPERATOR (const quat &l, const quat &r) noexcept { return quat(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr quat operator OPERATOR (const real &l, const quat &r) noexcept { return quat(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); } \
	CAGE_FORCE_INLINE constexpr quat operator OPERATOR (const quat &l, const real &r) noexcept { return quat(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr quat operator * (const quat &l, const quat &r) noexcept { return quat(l[3] * r[0] + l[0] * r[3] + l[1] * r[2] - l[2] * r[1], l[3] * r[1] + l[1] * r[3] + l[2] * r[0] - l[0] * r[2], l[3] * r[2] + l[2] * r[3] + l[0] * r[1] - l[1] * r[0], l[3] * r[3] - l[0] * r[0] - l[1] * r[1] - l[2] * r[2]); }
	CAGE_FORCE_INLINE constexpr quat operator * (const real &l, const quat &r) noexcept { return quat(l * r[0], l * r[1], l * r[2], l * r[3]); }
	CAGE_FORCE_INLINE constexpr quat operator * (const quat &l, const real &r) noexcept { return quat(l[0] * r, l[1] * r, l[2] * r, l[3] * r); }
#define GCHL_GENERATE(OPERATOR) \
	CAGE_CORE_API mat3 operator OPERATOR (const mat3 &l, const mat3 &r) noexcept; \
	CAGE_FORCE_INLINE constexpr mat3 operator OPERATOR (const mat3 &l, const real &r) noexcept { mat3 res; for (uint32 i = 0; i < 9; i++) res[i] = l[i] OPERATOR r; return res; } \
	CAGE_FORCE_INLINE constexpr mat3 operator OPERATOR (const real &l, const mat3 &r) noexcept { mat3 res; for (uint32 i = 0; i < 9; i++) res[i] = l OPERATOR r[i]; return res; } \
	CAGE_CORE_API mat4 operator OPERATOR (const mat4 &l, const mat4 &r) noexcept; \
	CAGE_FORCE_INLINE constexpr mat4 operator OPERATOR (const mat4 &l, const real &r) noexcept { mat4 res; for (uint32 i = 0; i < 16; i++) res[i] = l[i] OPERATOR r; return res; } \
	CAGE_FORCE_INLINE constexpr mat4 operator OPERATOR (const real &l, const mat4 &r) noexcept { mat4 res; for (uint32 i = 0; i < 16; i++) res[i] = l OPERATOR r[i]; return res; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(*);
#undef GCHL_GENERATE
	CAGE_CORE_API vec3 operator * (const transform &l, const vec3 &r) noexcept;
	CAGE_CORE_API vec3 operator * (const vec3 &l, const transform &r) noexcept;
	CAGE_CORE_API vec3 operator * (const vec3 &l, const quat &r) noexcept;
	CAGE_CORE_API vec3 operator * (const quat &l, const vec3 &r) noexcept;
	CAGE_CORE_API vec3 operator * (const vec3 &l, const mat3 &r) noexcept;
	CAGE_CORE_API vec3 operator * (const mat3 &l, const vec3 &r) noexcept;
	CAGE_CORE_API vec4 operator * (const vec4 &l, const mat4 &r) noexcept;
	CAGE_CORE_API vec4 operator * (const mat4 &l, const vec4 &r) noexcept;
	CAGE_CORE_API transform operator * (const transform &l, const transform &r) noexcept;
	CAGE_CORE_API transform operator * (const transform &l, const quat &r) noexcept;
	CAGE_CORE_API transform operator * (const quat &l, const transform &r) noexcept;
	CAGE_CORE_API transform operator * (const transform &l, const real &r) noexcept;
	CAGE_CORE_API transform operator * (const real &l, const transform &r) noexcept;
	CAGE_CORE_API transform operator + (const transform &l, const vec3 &r) noexcept;
	CAGE_CORE_API transform operator + (const vec3 &l, const transform &r) noexcept;

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr real &operator OPERATOR##= (real &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr rads &operator OPERATOR##= (rads &l, const rads &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr rads &operator OPERATOR##= (rads &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr degs &operator OPERATOR##= (degs &l, const degs &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr degs &operator OPERATOR##= (degs &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec2 &operator OPERATOR##= (vec2 &l, const vec2 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec2 &operator OPERATOR##= (vec2 &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec3 &operator OPERATOR##= (vec3 &l, const vec3 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec3 &operator OPERATOR##= (vec3 &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec4 &operator OPERATOR##= (vec4 &l, const vec4 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr vec4 &operator OPERATOR##= (vec4 &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec2 &operator OPERATOR##= (ivec2 &l, const ivec2 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec2 &operator OPERATOR##= (ivec2 &l, const sint32 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec3 &operator OPERATOR##= (ivec3 &l, const ivec3 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec3 &operator OPERATOR##= (ivec3 &l, const sint32 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec4 &operator OPERATOR##= (ivec4 &l, const ivec4 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr ivec4 &operator OPERATOR##= (ivec4 &l, const sint32 &r) noexcept { return l = l OPERATOR r; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
	GCHL_GENERATE(%);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr quat &operator OPERATOR##= (quat &l, const quat &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr quat &operator OPERATOR##= (quat &l, const real &r) noexcept { return l = l OPERATOR r; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE mat3 &operator OPERATOR##= (mat3 &l, const mat3 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE mat4 &operator OPERATOR##= (mat4 &l, const mat4 &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr mat3 &operator OPERATOR##= (mat3 &l, const real &r) noexcept { return l = l OPERATOR r; } \
	CAGE_FORCE_INLINE constexpr mat4 &operator OPERATOR##= (mat4 &l, const real &r) noexcept { return l = l OPERATOR r; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(*);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE vec3 &operator *= (vec3 &l, const mat3 &r) noexcept { return l = l *= r; }
	CAGE_FORCE_INLINE vec3 &operator *= (vec3 &l, const quat &r) noexcept { return l = l *= r; }
	CAGE_FORCE_INLINE vec3 &operator *= (vec3 &l, const transform &r) noexcept { return l = l * r; }
	CAGE_FORCE_INLINE vec4 &operator *= (vec4 &l, const mat4 &r) noexcept { return l = l *= r; }
	CAGE_FORCE_INLINE transform &operator *= (transform &l, const transform &r) noexcept { return l = l * r; }
	CAGE_FORCE_INLINE transform &operator *= (transform &l, const quat &r) noexcept { return l = l * r; }
	CAGE_FORCE_INLINE transform &operator *= (transform &l, const real &r) noexcept { return l = l * r; }
	CAGE_FORCE_INLINE transform &operator += (transform &l, const vec3 &r) noexcept { return l = l + r; }

	CAGE_FORCE_INLINE constexpr real::real(rads other) noexcept : value(other.value.value) {}
	CAGE_FORCE_INLINE constexpr real::real(degs other) noexcept : value(other.value.value) {}
	CAGE_FORCE_INLINE constexpr rads::rads(degs other) noexcept : value(other.value * real::Pi() / 180) {}
	CAGE_FORCE_INLINE constexpr degs::degs(rads other) noexcept : value(other.value * 180 / real::Pi()) {}
	CAGE_FORCE_INLINE constexpr vec2::vec2(const vec3 &other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr vec2::vec2(const vec4 &other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr vec2::vec2(const ivec2 &other) noexcept : data{ numeric_cast<real>(other.data[0]), numeric_cast<real>(other.data[1]) } {}
	CAGE_FORCE_INLINE constexpr ivec2::ivec2(const ivec3 &other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr ivec2::ivec2(const ivec4 &other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr ivec2::ivec2(const vec2 &other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]) } {}
	CAGE_FORCE_INLINE constexpr vec3::vec3(const vec4 &other) noexcept : data{ other.data[0], other.data[1], other.data[2] } {}
	CAGE_FORCE_INLINE constexpr vec3::vec3(const ivec3 &other) noexcept : data{ numeric_cast<real>(other.data[0]), numeric_cast<real>(other.data[1]), numeric_cast<real>(other.data[2]) } {}
	CAGE_FORCE_INLINE constexpr ivec3::ivec3(const ivec4 &other) noexcept : data{ other.data[0], other.data[1], other.data[2] } {}
	CAGE_FORCE_INLINE constexpr ivec3::ivec3(const vec3 &other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]), numeric_cast<sint32>(other.data[2]) } {}
	CAGE_FORCE_INLINE constexpr vec4::vec4(const ivec4 &other) noexcept : data{ numeric_cast<real>(other.data[0]), numeric_cast<real>(other.data[1]), numeric_cast<real>(other.data[2]), numeric_cast<real>(other.data[3]) } {}
	CAGE_FORCE_INLINE constexpr ivec4::ivec4(const vec4 &other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]), numeric_cast<sint32>(other.data[2]), numeric_cast<sint32>(other.data[3]) } {}
	CAGE_FORCE_INLINE constexpr mat3::mat3(const mat4 &other) noexcept : data{ other[0], other[1], other[2], other[4], other[5], other[6], other[8], other[9], other[10] } {}
	inline mat4::mat4(const transform &other) noexcept : mat4(other.position, other.orientation, vec3(other.scale)) {} // gcc has trouble with force inlining this function

	CAGE_FORCE_INLINE constexpr real smoothstep(real x) noexcept { return x * x * (3 - 2 * x); }
	CAGE_FORCE_INLINE constexpr real smootherstep(real x) noexcept { return x * x * x * (x * (x * 6 - 15) + 10); }
	CAGE_FORCE_INLINE constexpr sint32 sign(real x) noexcept { return x < 0 ? -1 : x > 0 ? 1 : 0; }
	CAGE_FORCE_INLINE constexpr real wrap(real x) noexcept { if (x < 0) return 1 - -x % 1; else return x % 1; }
	CAGE_FORCE_INLINE constexpr rads wrap(rads x) noexcept { if (x < rads()) return rads::Full() - -x % rads::Full(); else return x % rads::Full(); }
	CAGE_FORCE_INLINE constexpr degs wrap(degs x) noexcept { if (x < degs()) return degs::Full() - -x % degs::Full(); else return x % degs::Full(); }
	CAGE_FORCE_INLINE constexpr sint32 sign(rads x) noexcept { return sign(x.value); }
	CAGE_FORCE_INLINE constexpr sint32 sign(degs x) noexcept { return sign(x.value); }
	CAGE_FORCE_INLINE constexpr real sqr(real x) noexcept { return x * x; }
	CAGE_CORE_API real sqrt(real x);
	CAGE_CORE_API real pow(real base, real exponent); // base ^ exponent
	CAGE_CORE_API real powE(real x); // e ^ x
	CAGE_CORE_API real pow2(real x); // 2 ^ x
	CAGE_CORE_API real pow10(real x); // 10 ^ x
	CAGE_CORE_API real log(real x);
	CAGE_CORE_API real log2(real x);
	CAGE_CORE_API real log10(real x);
	CAGE_CORE_API real round(real x);
	CAGE_CORE_API real floor(real x);
	CAGE_CORE_API real ceil(real x);
	CAGE_CORE_API real sin(rads value);
	CAGE_CORE_API real cos(rads value);
	CAGE_CORE_API real tan(rads value);
	CAGE_CORE_API rads asin(real value);
	CAGE_CORE_API rads acos(real value);
	CAGE_CORE_API rads atan(real value);
	CAGE_CORE_API rads atan2(real x, real y);
	CAGE_CORE_API real distanceWrap(real a, real b);
	CAGE_FORCE_INLINE rads distanceAngle(rads a, rads b) { return distanceWrap((a / rads::Full()).value, (b / rads::Full()).value) * rads::Full(); }

#define GCHL_GENERATE(TYPE) CAGE_FORCE_INLINE constexpr TYPE abs(TYPE a) noexcept { return a < 0 ? -a : a; }
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(real);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) CAGE_FORCE_INLINE constexpr TYPE abs(TYPE a) noexcept { return a; }
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr rads abs(rads x) noexcept { return rads(abs(x.value)); }
	CAGE_FORCE_INLINE constexpr degs abs(degs x) noexcept { return degs(abs(x.value)); }

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr real dot(const TYPE &l, const TYPE &r) noexcept { TYPE m = l * r; real sum = 0; for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) sum += m[i]; return sum; } \
	CAGE_FORCE_INLINE constexpr real lengthSquared(const TYPE &x) noexcept { return dot(x, x); } \
	CAGE_FORCE_INLINE real length(const TYPE &x) { return sqrt(lengthSquared(x)); } \
	CAGE_FORCE_INLINE constexpr real distanceSquared(const TYPE &l, const TYPE &r) noexcept { return lengthSquared(l - r); } \
	CAGE_FORCE_INLINE real distance(const TYPE &l, const TYPE &r) { return sqrt(distanceSquared(l, r)); } \
	CAGE_FORCE_INLINE TYPE normalize(const TYPE &x) { return x / length(x); }
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr real dot(const quat &l, const quat &r) noexcept { real sum = 0; for (uint32 i = 0; i < 4; i++) sum += l[i] * r[i]; return sum; }
	CAGE_FORCE_INLINE constexpr real lengthSquared(const quat &x) noexcept { return dot(x, x); }
	CAGE_FORCE_INLINE real length(const quat &x) noexcept { return sqrt(lengthSquared(x)); }
	CAGE_FORCE_INLINE quat normalize(const quat &x) noexcept { return x / length(x); }
	CAGE_FORCE_INLINE constexpr quat conjugate(const quat &x) noexcept { return quat(-x[0], -x[1], -x[2], x[3]); }
	CAGE_FORCE_INLINE constexpr real cross(const vec2 &l, const vec2 &r) noexcept { return l[0] * r[1] - l[1] * r[0]; }
	CAGE_FORCE_INLINE constexpr vec3 cross(const vec3 &l, const vec3 &r) noexcept { return vec3(l[1] * r[2] - l[2] * r[1], l[2] * r[0] - l[0] * r[2], l[0] * r[1] - l[1] * r[0]); }
	CAGE_CORE_API vec3 dominantAxis(const vec3 &x);
	CAGE_CORE_API void toAxisAngle(const quat &x, vec3 &axis, rads &angle);
	CAGE_CORE_API quat lerp(const quat &a, const quat &b, real f);
	CAGE_CORE_API quat slerp(const quat &a, const quat &b, real f);
	CAGE_CORE_API quat slerpPrecise(const quat &a, const quat &b, real f);
	CAGE_CORE_API quat rotate(const quat &from, const quat &toward, rads maxTurn);
	CAGE_CORE_API rads angle(const quat &a, const quat &b);
	CAGE_CORE_API rads pitch(const quat &q);
	CAGE_CORE_API rads yaw(const quat &q);
	CAGE_CORE_API rads roll(const quat &q);

	CAGE_CORE_API mat3 inverse(const mat3 &x);
	CAGE_CORE_API mat3 transpose(const mat3 &x) noexcept;
	CAGE_CORE_API mat3 normalize(const mat3 &x);
	CAGE_CORE_API real determinant(const mat3 &x);
	CAGE_CORE_API mat4 inverse(const mat4 &x);
	CAGE_CORE_API mat4 transpose(const mat4 &x) noexcept;
	CAGE_CORE_API mat4 normalize(const mat4 &x);
	CAGE_CORE_API real determinant(const mat4 &x);
	CAGE_CORE_API transform inverse(const transform &x);

	CAGE_CORE_API bool valid(float a) noexcept;
	CAGE_CORE_API bool valid(double a) noexcept;
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE bool valid(const TYPE &a) noexcept { return a.valid(); }
	GCHL_GENERATE(real);
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
	GCHL_GENERATE(quat);
	GCHL_GENERATE(mat3);
	GCHL_GENERATE(mat4);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(TYPE a, TYPE b) noexcept { return a < b ? a : b; } \
	CAGE_FORCE_INLINE constexpr TYPE max(TYPE a, TYPE b) noexcept { return a > b ? a : b; } \
	CAGE_FORCE_INLINE constexpr TYPE clamp(TYPE v, TYPE a, TYPE b) { CAGE_ASSERT(a <= b); return min(max(v, a), b); }
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(real);
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE saturate(TYPE v) noexcept { return clamp(v, TYPE(0), TYPE(1)); }
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(real);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE saturate(TYPE v) noexcept { return clamp(v, TYPE(0), TYPE::Full()); }
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE abs(const TYPE &x) noexcept { TYPE res; for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) res[i] = abs(x[i]); return res; } \
	CAGE_FORCE_INLINE constexpr TYPE min(const TYPE &l, const TYPE &r) noexcept { TYPE res; for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) res[i] = min(l[i], r[i]); return res; } \
	CAGE_FORCE_INLINE constexpr TYPE max(const TYPE &l, const TYPE &r) noexcept { TYPE res; for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) res[i] = max(l[i], r[i]); return res; } \
	CAGE_FORCE_INLINE constexpr TYPE clamp(const TYPE &v, const TYPE &a, const TYPE &b) noexcept { TYPE res; for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) res[i] = clamp(v[i], a[i], b[i]); return res; }
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
	GCHL_GENERATE(ivec2);
	GCHL_GENERATE(ivec3);
	GCHL_GENERATE(ivec4);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(const TYPE &l, real r) noexcept { return min(l, TYPE(r)); } \
	CAGE_FORCE_INLINE constexpr TYPE max(const TYPE &l, real r) noexcept { return max(l, TYPE(r)); } \
	CAGE_FORCE_INLINE constexpr TYPE min(real l, const TYPE &r) noexcept { return min(TYPE(l), r); } \
	CAGE_FORCE_INLINE constexpr TYPE max(real l, const TYPE &r) noexcept { return max(TYPE(l), r); } \
	CAGE_FORCE_INLINE constexpr TYPE clamp(const TYPE &v, real a, real b) noexcept { return clamp(v, TYPE(a), TYPE(b)); } \
	CAGE_FORCE_INLINE constexpr TYPE saturate(const TYPE &v) noexcept { return clamp(v, 0, 1); }
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(const TYPE &l, sint32 r) noexcept { return min(l, TYPE(r)); } \
	CAGE_FORCE_INLINE constexpr TYPE max(const TYPE &l, sint32 r) noexcept { return max(l, TYPE(r)); } \
	CAGE_FORCE_INLINE constexpr TYPE min(sint32 l, const TYPE &r) noexcept { return min(TYPE(l), r); } \
	CAGE_FORCE_INLINE constexpr TYPE max(sint32 l, const TYPE &r) noexcept { return max(TYPE(l), r); } \
	CAGE_FORCE_INLINE constexpr TYPE clamp(const TYPE &v, sint32 a, sint32 b) noexcept { return clamp(v, TYPE(a), TYPE(b)); }
	GCHL_GENERATE(ivec2);
	GCHL_GENERATE(ivec3);
	GCHL_GENERATE(ivec4);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE interpolate(const TYPE &a, const TYPE &b, real f) noexcept { return (TYPE)(a * (1 - f.value) + b * f.value); }
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE interpolate(const TYPE &a, const TYPE &b, real f) noexcept { return (TYPE)(a + (b - a) * f.value); }
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(real);
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE quat interpolate(const quat &a, const quat &b, real f) { return slerp(a, b, f); }
	CAGE_FORCE_INLINE transform interpolate(const transform &a, const transform &b, real f) noexcept { return transform(interpolate(a.position, b.position, f), interpolate(a.orientation, b.orientation, f), interpolate(a.scale, b.scale, f)); }
	CAGE_CORE_API real interpolateWrap(real a, real b, real f);
	CAGE_FORCE_INLINE rads interpolateAngle(rads a, rads b, real f) { return interpolateWrap((a / rads::Full()).value, (b / rads::Full()).value, f) * rads::Full(); }

	CAGE_CORE_API real randomChance(); // 0 to 1; including 0, excluding 1
#define GCHL_GENERATE(TYPE) \
	CAGE_CORE_API TYPE randomRange(TYPE min, TYPE max);
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(real);
	GCHL_GENERATE(rads);
	GCHL_GENERATE(degs);
#undef GCHL_GENERATE
	CAGE_CORE_API rads randomAngle();
	CAGE_CORE_API vec2 randomChance2();
	CAGE_CORE_API vec2 randomRange2(real a, real b);
	CAGE_CORE_API vec2 randomDirection2();
	CAGE_CORE_API ivec2 randomRange2i(sint32 a, sint32 b);
	CAGE_CORE_API vec3 randomChance3();
	CAGE_CORE_API vec3 randomRange3(real a, real b);
	CAGE_CORE_API vec3 randomDirection3();
	CAGE_CORE_API ivec3 randomRange3i(sint32 a, sint32 b);
	CAGE_CORE_API vec4 randomChance4();
	CAGE_CORE_API vec4 randomRange4(real a, real b);
	CAGE_CORE_API quat randomDirectionQuat();
	CAGE_CORE_API ivec4 randomRange4i(sint32 a, sint32 b);

	CAGE_FORCE_INLINE constexpr uint32 hash(uint32 key) noexcept
	{ // integer finalizer hash function
		key ^= key >> 16;
		key *= 0x85ebca6b;
		key ^= key >> 13;
		key *= 0xc2b2ae35;
		key ^= key >> 16;
		return key;
	}

	namespace detail
	{
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const real &other) { return str + other.value; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const degs &other) { return str + other.value.value + "deg"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const rads &other) { return str + other.value.value + "rad"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const vec2 &other) { return str + "(" + other[0].value + "," + other[1].value + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const vec3 &other) { return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const vec4 &other) { return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + "," + other[3].value + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const quat &other) { return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + "," + other[3].value + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const mat3 &other) { str + "(" + other[0].value; for (uint32 i = 1; i < 9; i++) str + "," + other[i].value; return str + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const mat4 &other) { str + "(" + other[0].value; for (uint32 i = 1; i < 16; i++) str + "," + other[i].value; return str + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const transform &other) { return str + "(" + other.position + "," + other.orientation + "," + other.scale + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const ivec2 &other) { return str + "(" + other[0] + "," + other[1] + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const ivec3 &other) { return str + "(" + other[0] + "," + other[1] + "," + other[2] + ")"; }
		template<uint32 N> CAGE_FORCE_INLINE StringizerBase<N> &operator + (StringizerBase<N> &str, const ivec4 &other) { return str + "(" + other[0] + "," + other[1] + "," + other[2] + "," + other[3] + ")"; }
	}
}

#undef GCHL_DIMENSION

#endif // guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
