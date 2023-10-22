#ifndef guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
#define guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_

#include <cage-core/core.h>

#define GCHL_DIMENSION(TYPE) array_size(decltype(TYPE::data){})

namespace cage
{
	struct CAGE_CORE_API Real
	{
		using value_type = float;

		value_type value = 0;

		CAGE_FORCE_INLINE constexpr Real() noexcept {}
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr Real(TYPE other) noexcept : value((value_type)other) {}
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
		explicit constexpr Real(Rads other) noexcept;
		explicit constexpr Real(Degs other) noexcept;

		auto operator<=>(const Real &) const noexcept = default;
		bool operator==(const Real &) const noexcept = default;

		static Real parse(const String &str);
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx == 0);
			return *this;
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx == 0);
			return *this;
		}
		bool valid() const noexcept;
		bool finite() const noexcept;
		static constexpr Real Pi() noexcept { return (Real)3.141592653589793238; }
		static constexpr Real E() noexcept { return (Real)2.718281828459045235; }
		static constexpr Real Log2() noexcept { return (Real)0.693147180559945309; }
		static constexpr Real Log10() noexcept { return (Real)2.302585092994045684; }
		static constexpr Real Infinity() noexcept { return std::numeric_limits<value_type>::infinity(); }
		static constexpr Real Nan() noexcept { return std::numeric_limits<value_type>::quiet_NaN(); }
	};
}

namespace std
{
	template<>
	struct numeric_limits<cage::Real> : public std::numeric_limits<cage::Real::value_type>
	{};
}

namespace cage
{
	template<class To>
	CAGE_FORCE_INLINE constexpr To numeric_cast(Real from)
	{
		return numeric_cast<To>(from.value);
	}

	struct CAGE_CORE_API Rads
	{
		Real value;

		CAGE_FORCE_INLINE constexpr Rads() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Rads(Real value) noexcept : value(value) {}
		constexpr Rads(Degs other) noexcept;

		auto operator<=>(const Rads &) const noexcept = default;
		bool operator==(const Rads &) const noexcept = default;

		static Rads parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE static constexpr Rads Full() noexcept { return Rads(Real::Pi().value * 2); }
		CAGE_FORCE_INLINE static constexpr Rads Nan() noexcept { return Rads(Real::Nan()); }
		friend struct Real;
		friend struct Degs;
	};

	struct CAGE_CORE_API Degs
	{
		Real value;

		CAGE_FORCE_INLINE constexpr Degs() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Degs(Real value) noexcept : value(value) {}
		constexpr Degs(Rads other) noexcept;

		auto operator<=>(const Degs &) const noexcept = default;
		bool operator==(const Degs &) const noexcept = default;

		static Degs parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE static constexpr Degs Full() noexcept { return Degs(360); }
		CAGE_FORCE_INLINE static constexpr Degs Nan() noexcept { return Degs(Real::Nan()); }
		friend struct Real;
		friend struct Rads;
	};

	struct CAGE_CORE_API Vec2
	{
		Real data[2];

		CAGE_FORCE_INLINE constexpr Vec2() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec2(Real value) noexcept : data{ value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec2(Real x, Real y) noexcept : data{ x, y } {}
		CAGE_FORCE_INLINE explicit constexpr Vec2(Vec3 v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec2(Vec4 v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec2(Vec2i v) noexcept;

		bool operator==(const Vec2 &) const noexcept = default;

		static Vec2 parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 2);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 2);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Vec2 Nan() noexcept { return Vec2(Real::Nan()); }
	};

	struct CAGE_CORE_API Vec2i
	{
		sint32 data[2] = {};

		CAGE_FORCE_INLINE constexpr Vec2i() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec2i(sint32 value) noexcept : data{ value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec2i(sint32 x, sint32 y) noexcept : data{ x, y } {}
		CAGE_FORCE_INLINE explicit constexpr Vec2i(Vec3i v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec2i(Vec4i v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec2i(Vec2 v) noexcept;

		bool operator==(const Vec2i &) const noexcept = default;

		static Vec2i parse(const String &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 2);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr sint32 operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 2);
			return data[idx];
		}
	};

	struct CAGE_CORE_API Vec3
	{
		Real data[3];

		CAGE_FORCE_INLINE constexpr Vec3() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec3(Real value) noexcept : data{ value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3(Real x, Real y, Real z) noexcept : data{ x, y, z } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3(Vec2 v, Real z) noexcept : data{ v[0], v[1], z } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3(Vec4 v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec3(Vec3i v) noexcept;

		bool operator==(const Vec3 &) const noexcept = default;

		static Vec3 parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 3);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 3);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Vec3 Nan() noexcept { return Vec3(Real::Nan()); }
	};

	struct CAGE_CORE_API Vec3i
	{
		sint32 data[3] = {};

		CAGE_FORCE_INLINE constexpr Vec3i() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec3i(sint32 value) noexcept : data{ value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3i(sint32 x, sint32 y, sint32 z) noexcept : data{ x, y, z } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3i(Vec2i v, sint32 z) noexcept : data{ v[0], v[1], z } {}
		CAGE_FORCE_INLINE explicit constexpr Vec3i(Vec4i v) noexcept;
		CAGE_FORCE_INLINE explicit constexpr Vec3i(Vec3 v) noexcept;

		bool operator==(const Vec3i &) const noexcept = default;

		static Vec3i parse(const String &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 3);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr sint32 operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 3);
			return data[idx];
		}
	};

	struct CAGE_CORE_API Vec4
	{
		Real data[4];

		CAGE_FORCE_INLINE constexpr Vec4() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Real value) noexcept : data{ value, value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Real x, Real y, Real z, Real w) noexcept : data{ x, y, z, w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Vec2 v, Real z, Real w) noexcept : data{ v[0], v[1], z, w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Vec2 v, Vec2 w) noexcept : data{ v[0], v[1], w[0], w[1] } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Vec3 v, Real w) noexcept : data{ v[0], v[1], v[2], w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4(Vec4i v) noexcept;

		bool operator==(const Vec4 &) const noexcept = default;

		static Vec4 parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Vec4 Nan() noexcept { return Vec4(Real::Nan()); }
	};

	struct CAGE_CORE_API Vec4i
	{
		sint32 data[4] = {};

		CAGE_FORCE_INLINE constexpr Vec4i() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(sint32 value) noexcept : data{ value, value, value, value } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(sint32 x, sint32 y, sint32 z, sint32 w) noexcept : data{ x, y, z, w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(Vec2i v, sint32 z, sint32 w) noexcept : data{ v[0], v[1], z, w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(Vec2i v, Vec2i w) noexcept : data{ v[0], v[1], w[0], w[1] } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(Vec3i v, sint32 w) noexcept : data{ v[0], v[1], v[2], w } {}
		CAGE_FORCE_INLINE explicit constexpr Vec4i(Vec4 v) noexcept;

		bool operator==(const Vec4i &) const noexcept = default;

		static Vec4i parse(const String &str);
		CAGE_FORCE_INLINE constexpr sint32 &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr sint32 operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
	};

	struct CAGE_CORE_API Quat
	{
		Real data[4] = { 0, 0, 0, 1 }; // x, y, z, w

		CAGE_FORCE_INLINE constexpr Quat() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Quat(Real x, Real y, Real z, Real w) noexcept : data{ x, y, z, w } {}
		explicit Quat(Rads pitch, Rads yaw, Rads roll);
		explicit Quat(Vec3 axis, Rads angle);
		explicit Quat(Mat3 rot);
		explicit Quat(Vec3 forward, Vec3 up, bool keepUp = false);

		bool operator==(const Quat &) const noexcept = default;

		std::pair<Vec3, Rads> axisAngle() const;

		static Quat parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 4);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Quat Nan() noexcept { return Quat(Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan()); }
	};

	struct CAGE_CORE_API Mat3
	{
		Real data[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

		CAGE_FORCE_INLINE constexpr Mat3() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Mat3(Real a, Real b, Real c, Real d, Real e, Real f, Real g, Real h, Real i) noexcept : data{ a, b, c, d, e, f, g, h, i } {}
		explicit Mat3(Vec3 forward, Vec3 up, bool keepUp = false);
		explicit Mat3(Quat other) noexcept;
		explicit constexpr Mat3(Mat4 other) noexcept;

		bool operator==(const Mat3 &) const noexcept = default;

		static Mat3 parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 9);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 9);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Mat3 Zero() noexcept { return Mat3(0, 0, 0, 0, 0, 0, 0, 0, 0); }
		CAGE_FORCE_INLINE static constexpr Mat3 Nan() noexcept { return Mat3(Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan()); }
	};

	struct CAGE_CORE_API Mat4
	{
		Real data[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }; // SRR0 RSR0 RRS0 TTT1

		CAGE_FORCE_INLINE constexpr Mat4() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Mat4(Real a, Real b, Real c, Real d, Real e, Real f, Real g, Real h, Real i, Real j, Real k, Real l, Real m, Real n, Real o, Real p) noexcept : data{ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p } {}
		CAGE_FORCE_INLINE explicit constexpr Mat4(Mat3 other) noexcept : data{ other[0], other[1], other[2], 0, other[3], other[4], other[5], 0, other[6], other[7], other[8], 0, 0, 0, 0, 1 } {}
		CAGE_FORCE_INLINE explicit constexpr Mat4(Vec3 position) noexcept : data{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, position[0], position[1], position[2], 1 } {}
		explicit Mat4(Vec3 position, Quat orientation, Vec3 scale = Vec3(1)) noexcept;
		CAGE_FORCE_INLINE explicit Mat4(Quat orientation) noexcept : Mat4(Mat3(orientation)) {}
		explicit Mat4(Transform other) noexcept;

		bool operator==(const Mat4 &) const noexcept = default;

		CAGE_FORCE_INLINE static constexpr Mat4 scale(Real scl) noexcept { return scale(Vec3(scl)); }
		CAGE_FORCE_INLINE static constexpr Mat4 scale(Vec3 scl) noexcept { return Mat4(scl[0], 0, 0, 0, 0, scl[1], 0, 0, 0, 0, scl[2], 0, 0, 0, 0, 1); }
		static Mat4 parse(const String &str);
		bool valid() const noexcept;
		CAGE_FORCE_INLINE constexpr Real &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 16);
			return data[idx];
		}
		CAGE_FORCE_INLINE constexpr Real operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 16);
			return data[idx];
		}
		CAGE_FORCE_INLINE static constexpr Mat4 Zero() noexcept { return Mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); }
		CAGE_FORCE_INLINE static constexpr Mat4 Nan() noexcept { return Mat4(Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan(), Real::Nan()); }
	};

	struct CAGE_CORE_API Transform
	{
		Quat orientation;
		Vec3 position;
		Real scale = 1;

		CAGE_FORCE_INLINE constexpr Transform() noexcept {}
		CAGE_FORCE_INLINE explicit constexpr Transform(Vec3 position, Quat orientation = Quat(), Real scale = 1) noexcept : orientation(orientation), position(position), scale(scale) {}

		bool operator==(const Transform &) const noexcept = default;

		static Transform parse(const String &str);
		bool valid() const noexcept;
	};

	CAGE_FORCE_INLINE constexpr auto operator<=>(Rads l, Degs r) noexcept
	{
		return l <=> Rads(r);
	};
	CAGE_FORCE_INLINE constexpr auto operator<=>(Degs l, Rads r) noexcept
	{
		return Rads(l) <=> r;
	};

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Real operator OPERATOR(Real r) noexcept \
	{ \
		return OPERATOR r.value; \
	} \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Rads r) noexcept \
	{ \
		return Rads(OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Degs operator OPERATOR(Degs r) noexcept \
	{ \
		return Degs(OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 operator OPERATOR(Vec2 r) noexcept \
	{ \
		return Vec2(OPERATOR r[0], OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 operator OPERATOR(Vec3 r) noexcept \
	{ \
		return Vec3(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 operator OPERATOR(Vec4 r) noexcept \
	{ \
		return Vec4(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Quat operator OPERATOR(Quat r) noexcept \
	{ \
		return Quat(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i operator OPERATOR(Vec2i r) noexcept \
	{ \
		return Vec2i(OPERATOR r[0], OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i operator OPERATOR(Vec3i r) noexcept \
	{ \
		return Vec3i(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i operator OPERATOR(Vec4i r) noexcept \
	{ \
		return Vec4i(OPERATOR r[0], OPERATOR r[1], OPERATOR r[2], OPERATOR r[3]); \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Real operator OPERATOR(Real l, Real r) noexcept \
	{ \
		return l.value OPERATOR r.value; \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr Real operator%(Real l, Real r) noexcept
	{
		CAGE_ASSERT(r.value != 0);
		return l - (r * (sint32)(l / r).value);
	}
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Rads l, Rads r) noexcept \
	{ \
		return Rads(l.value OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Rads l, Real r) noexcept \
	{ \
		return Rads(l.value OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Real l, Rads r) noexcept \
	{ \
		return Rads(l OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Degs operator OPERATOR(Degs l, Degs r) noexcept \
	{ \
		return Degs(l.value OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Degs operator OPERATOR(Degs l, Real r) noexcept \
	{ \
		return Degs(l.value OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Degs operator OPERATOR(Real l, Degs r) noexcept \
	{ \
		return Degs(l OPERATOR r.value); \
	} \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Rads l, Degs r) noexcept \
	{ \
		return l OPERATOR Rads(r); \
	} \
	CAGE_FORCE_INLINE constexpr Rads operator OPERATOR(Degs l, Rads r) noexcept \
	{ \
		return Rads(l) OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 operator OPERATOR(Vec2 l, Vec2 r) noexcept \
	{ \
		return Vec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 operator OPERATOR(Vec2 l, Real r) noexcept \
	{ \
		return Vec2(l[0] OPERATOR r, l[1] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 operator OPERATOR(Real l, Vec2 r) noexcept \
	{ \
		return Vec2(l OPERATOR r[0], l OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 operator OPERATOR(Vec3 l, Vec3 r) noexcept \
	{ \
		return Vec3(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 operator OPERATOR(Vec3 l, Real r) noexcept \
	{ \
		return Vec3(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 operator OPERATOR(Real l, Vec3 r) noexcept \
	{ \
		return Vec3(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 operator OPERATOR(Vec4 l, Vec4 r) noexcept \
	{ \
		return Vec4(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 operator OPERATOR(Vec4 l, Real r) noexcept \
	{ \
		return Vec4(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 operator OPERATOR(Real l, Vec4 r) noexcept \
	{ \
		return Vec4(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i operator OPERATOR(Vec2i l, Vec2i r) noexcept \
	{ \
		return Vec2i(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i operator OPERATOR(Vec2i l, sint32 r) noexcept \
	{ \
		return Vec2i(l[0] OPERATOR r, l[1] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i operator OPERATOR(sint32 l, Vec2i r) noexcept \
	{ \
		return Vec2i(l OPERATOR r[0], l OPERATOR r[1]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i operator OPERATOR(Vec3i l, Vec3i r) noexcept \
	{ \
		return Vec3i(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i operator OPERATOR(Vec3i l, sint32 r) noexcept \
	{ \
		return Vec3i(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i operator OPERATOR(sint32 l, Vec3i r) noexcept \
	{ \
		return Vec3i(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i operator OPERATOR(Vec4i l, Vec4i r) noexcept \
	{ \
		return Vec4i(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i operator OPERATOR(Vec4i l, sint32 r) noexcept \
	{ \
		return Vec4i(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i operator OPERATOR(sint32 l, Vec4i r) noexcept \
	{ \
		return Vec4i(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
	GCHL_GENERATE(%);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Quat operator OPERATOR(Quat l, Quat r) noexcept \
	{ \
		return Quat(l[0] OPERATOR r[0], l[1] OPERATOR r[1], l[2] OPERATOR r[2], l[3] OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Quat operator OPERATOR(Real l, Quat r) noexcept \
	{ \
		return Quat(l OPERATOR r[0], l OPERATOR r[1], l OPERATOR r[2], l OPERATOR r[3]); \
	} \
	CAGE_FORCE_INLINE constexpr Quat operator OPERATOR(Quat l, Real r) noexcept \
	{ \
		return Quat(l[0] OPERATOR r, l[1] OPERATOR r, l[2] OPERATOR r, l[3] OPERATOR r); \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr Quat operator*(Quat l, Quat r) noexcept
	{
		return Quat(l[3] * r[0] + l[0] * r[3] + l[1] * r[2] - l[2] * r[1], l[3] * r[1] + l[1] * r[3] + l[2] * r[0] - l[0] * r[2], l[3] * r[2] + l[2] * r[3] + l[0] * r[1] - l[1] * r[0], l[3] * r[3] - l[0] * r[0] - l[1] * r[1] - l[2] * r[2]);
	}
	CAGE_FORCE_INLINE constexpr Quat operator*(Real l, Quat r) noexcept
	{
		return Quat(l * r[0], l * r[1], l * r[2], l * r[3]);
	}
	CAGE_FORCE_INLINE constexpr Quat operator*(Quat l, Real r) noexcept
	{
		return Quat(l[0] * r, l[1] * r, l[2] * r, l[3] * r);
	}
#define GCHL_GENERATE(OPERATOR) \
	CAGE_CORE_API Mat3 operator OPERATOR(Mat3 l, Mat3 r) noexcept; \
	CAGE_FORCE_INLINE constexpr Mat3 operator OPERATOR(Mat3 l, Real r) noexcept \
	{ \
		Mat3 res; \
		for (uint32 i = 0; i < 9; i++) \
			res[i] = l[i] OPERATOR r; \
		return res; \
	} \
	CAGE_FORCE_INLINE constexpr Mat3 operator OPERATOR(Real l, Mat3 r) noexcept \
	{ \
		Mat3 res; \
		for (uint32 i = 0; i < 9; i++) \
			res[i] = l OPERATOR r[i]; \
		return res; \
	} \
	CAGE_CORE_API Mat4 operator OPERATOR(Mat4 l, Mat4 r) noexcept; \
	CAGE_FORCE_INLINE constexpr Mat4 operator OPERATOR(Mat4 l, Real r) noexcept \
	{ \
		Mat4 res; \
		for (uint32 i = 0; i < 16; i++) \
			res[i] = l[i] OPERATOR r; \
		return res; \
	} \
	CAGE_FORCE_INLINE constexpr Mat4 operator OPERATOR(Real l, Mat4 r) noexcept \
	{ \
		Mat4 res; \
		for (uint32 i = 0; i < 16; i++) \
			res[i] = l OPERATOR r[i]; \
		return res; \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(*);
#undef GCHL_GENERATE
	CAGE_CORE_API Vec3 operator*(Transform l, Vec3 r) noexcept;
	CAGE_CORE_API Vec3 operator*(Vec3 l, Transform r) noexcept;
	CAGE_CORE_API Vec3 operator*(Vec3 l, Quat r) noexcept;
	CAGE_CORE_API Vec3 operator*(Quat l, Vec3 r) noexcept;
	CAGE_CORE_API Vec3 operator*(Vec3 l, Mat3 r) noexcept;
	CAGE_CORE_API Vec3 operator*(Mat3 l, Vec3 r) noexcept;
	CAGE_CORE_API Vec4 operator*(Vec4 l, Mat4 r) noexcept;
	CAGE_CORE_API Vec4 operator*(Mat4 l, Vec4 r) noexcept;
	CAGE_CORE_API Transform operator*(Transform l, Transform r) noexcept;
	CAGE_CORE_API Transform operator*(Transform l, Quat r) noexcept;
	CAGE_CORE_API Transform operator*(Quat l, Transform r) noexcept;
	CAGE_CORE_API Transform operator*(Transform l, Real r) noexcept;
	CAGE_CORE_API Transform operator*(Real l, Transform r) noexcept;
	CAGE_CORE_API Transform operator+(Transform l, Vec3 r) noexcept;
	CAGE_CORE_API Transform operator+(Vec3 l, Transform r) noexcept;

#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Real &operator OPERATOR##=(Real &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Rads &operator OPERATOR##=(Rads &l, Rads r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Rads &operator OPERATOR##=(Rads &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Degs &operator OPERATOR##=(Degs &l, Degs r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Degs &operator OPERATOR##=(Degs &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 &operator OPERATOR##=(Vec2 &l, Vec2 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec2 &operator OPERATOR##=(Vec2 &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 &operator OPERATOR##=(Vec3 &l, Vec3 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec3 &operator OPERATOR##=(Vec3 &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 &operator OPERATOR##=(Vec4 &l, Vec4 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec4 &operator OPERATOR##=(Vec4 &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i &operator OPERATOR##=(Vec2i &l, Vec2i r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec2i &operator OPERATOR##=(Vec2i &l, sint32 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i &operator OPERATOR##=(Vec3i &l, Vec3i r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec3i &operator OPERATOR##=(Vec3i &l, sint32 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i &operator OPERATOR##=(Vec4i &l, Vec4i r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Vec4i &operator OPERATOR##=(Vec4i &l, sint32 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
	GCHL_GENERATE(%);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE constexpr Quat &operator OPERATOR##=(Quat &l, Quat r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Quat &operator OPERATOR##=(Quat &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
#define GCHL_GENERATE(OPERATOR) \
	CAGE_FORCE_INLINE Mat3 &operator OPERATOR##=(Mat3 &l, Mat3 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE Mat4 &operator OPERATOR##=(Mat4 &l, Mat4 r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Mat3 &operator OPERATOR##=(Mat3 &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	} \
	CAGE_FORCE_INLINE constexpr Mat4 &operator OPERATOR##=(Mat4 &l, Real r) noexcept \
	{ \
		return l = l OPERATOR r; \
	}
	GCHL_GENERATE(+);
	GCHL_GENERATE(*);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE Vec3 &operator*=(Vec3 &l, Mat3 r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Vec3 &operator*=(Vec3 &l, Quat r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Vec3 &operator*=(Vec3 &l, Transform r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Vec4 &operator*=(Vec4 &l, Mat4 r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Transform &operator*=(Transform &l, Transform r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Transform &operator*=(Transform &l, Quat r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Transform &operator*=(Transform &l, Real r) noexcept
	{
		return l = l * r;
	}
	CAGE_FORCE_INLINE Transform &operator+=(Transform &l, Vec3 r) noexcept
	{
		return l = l + r;
	}

	CAGE_FORCE_INLINE constexpr Real::Real(Rads other) noexcept : value(other.value.value) {}
	CAGE_FORCE_INLINE constexpr Real::Real(Degs other) noexcept : value(other.value.value) {}
	CAGE_FORCE_INLINE constexpr Rads::Rads(Degs other) noexcept : value(other.value * Real::Pi() / 180) {}
	CAGE_FORCE_INLINE constexpr Degs::Degs(Rads other) noexcept : value(other.value * 180 / Real::Pi()) {}
	CAGE_FORCE_INLINE constexpr Vec2::Vec2(Vec3 other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr Vec2::Vec2(Vec4 other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr Vec2::Vec2(Vec2i other) noexcept : data{ numeric_cast<Real>(other.data[0]), numeric_cast<Real>(other.data[1]) } {}
	CAGE_FORCE_INLINE constexpr Vec2i::Vec2i(Vec3i other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr Vec2i::Vec2i(Vec4i other) noexcept : data{ other.data[0], other.data[1] } {}
	CAGE_FORCE_INLINE constexpr Vec2i::Vec2i(Vec2 other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]) } {}
	CAGE_FORCE_INLINE constexpr Vec3::Vec3(Vec4 other) noexcept : data{ other.data[0], other.data[1], other.data[2] } {}
	CAGE_FORCE_INLINE constexpr Vec3::Vec3(Vec3i other) noexcept : data{ numeric_cast<Real>(other.data[0]), numeric_cast<Real>(other.data[1]), numeric_cast<Real>(other.data[2]) } {}
	CAGE_FORCE_INLINE constexpr Vec3i::Vec3i(Vec4i other) noexcept : data{ other.data[0], other.data[1], other.data[2] } {}
	CAGE_FORCE_INLINE constexpr Vec3i::Vec3i(Vec3 other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]), numeric_cast<sint32>(other.data[2]) } {}
	CAGE_FORCE_INLINE constexpr Vec4::Vec4(Vec4i other) noexcept : data{ numeric_cast<Real>(other.data[0]), numeric_cast<Real>(other.data[1]), numeric_cast<Real>(other.data[2]), numeric_cast<Real>(other.data[3]) } {}
	CAGE_FORCE_INLINE constexpr Vec4i::Vec4i(Vec4 other) noexcept : data{ numeric_cast<sint32>(other.data[0]), numeric_cast<sint32>(other.data[1]), numeric_cast<sint32>(other.data[2]), numeric_cast<sint32>(other.data[3]) } {}
	CAGE_FORCE_INLINE constexpr Mat3::Mat3(Mat4 other) noexcept : data{ other[0], other[1], other[2], other[4], other[5], other[6], other[8], other[9], other[10] } {}
	inline Mat4::Mat4(Transform other) noexcept : Mat4(other.position, other.orientation, Vec3(other.scale)) {} // gcc has trouble with force inlining this function

	CAGE_FORCE_INLINE constexpr Real smoothstep(Real x) noexcept
	{
		return x * x * (3 - 2 * x);
	}
	CAGE_FORCE_INLINE constexpr Real smootherstep(Real x) noexcept
	{
		return x * x * x * (x * (x * 6 - 15) + 10);
	}
	CAGE_FORCE_INLINE constexpr sint32 sign(Real x) noexcept
	{
		return x < 0 ? -1 : x > 0 ? 1 : 0;
	}
	CAGE_FORCE_INLINE constexpr Real wrap(Real x) noexcept
	{
		if (x < 0)
			return 1 - -x % 1;
		else
			return x % 1;
	}
	CAGE_FORCE_INLINE constexpr Rads wrap(Rads x) noexcept
	{
		if (x < Rads())
			return Rads::Full() - -x % Rads::Full();
		else
			return x % Rads::Full();
	}
	CAGE_FORCE_INLINE constexpr Degs wrap(Degs x) noexcept
	{
		if (x < Degs())
			return Degs::Full() - -x % Degs::Full();
		else
			return x % Degs::Full();
	}
	CAGE_FORCE_INLINE constexpr sint32 sign(Rads x) noexcept
	{
		return sign(x.value);
	}
	CAGE_FORCE_INLINE constexpr sint32 sign(Degs x) noexcept
	{
		return sign(x.value);
	}
	CAGE_FORCE_INLINE constexpr Real sqr(Real x) noexcept
	{
		return x * x;
	}
	CAGE_CORE_API Real sqrt(Real x);
	CAGE_CORE_API Real pow(Real base, Real x); // base ^ x
	CAGE_CORE_API Real pow(Real x); // e ^ x
	CAGE_CORE_API Real pow2(Real x); // 2 ^ x
	CAGE_CORE_API Real pow10(Real x); // 10 ^ x
	CAGE_CORE_API Real log(Real base, Real x); // log_base(x)
	CAGE_CORE_API Real log(Real x); // log_e(x)
	CAGE_CORE_API Real log2(Real x); // log_2(x)
	CAGE_CORE_API Real log10(Real x); // log_10(x)
	CAGE_CORE_API Real round(Real x);
	CAGE_CORE_API Real floor(Real x);
	CAGE_CORE_API Real ceil(Real x);
	CAGE_CORE_API Real sin(Rads value);
	CAGE_CORE_API Real cos(Rads value);
	CAGE_CORE_API Real tan(Rads value);
	CAGE_CORE_API Rads asin(Real value);
	CAGE_CORE_API Rads acos(Real value);
	CAGE_CORE_API Rads atan(Real value);
	CAGE_CORE_API Rads atan2(Real x, Real y);
	CAGE_CORE_API Real distanceWrap(Real a, Real b);
	CAGE_FORCE_INLINE Rads distanceAngle(Rads a, Rads b)
	{
		return distanceWrap((a / Rads::Full()).value, (b / Rads::Full()).value) * Rads::Full();
	}

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE sqr(TYPE x) noexcept \
	{ \
		return x * x; \
	} \
	CAGE_CORE_API double sqrt(TYPE x);
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

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE abs(TYPE a) noexcept \
	{ \
		return a < 0 ? -a : a; \
	}
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(Real);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE abs(TYPE a) noexcept \
	{ \
		return a; \
	}
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr Rads abs(Rads x) noexcept
	{
		return Rads(abs(x.value));
	}
	CAGE_FORCE_INLINE constexpr Degs abs(Degs x) noexcept
	{
		return Degs(abs(x.value));
	}

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr Real dot(TYPE l, TYPE r) noexcept \
	{ \
		TYPE m = l * r; \
		Real sum = 0; \
		for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) \
			sum += m[i]; \
		return sum; \
	} \
	CAGE_FORCE_INLINE constexpr Real lengthSquared(TYPE x) noexcept \
	{ \
		return dot(x, x); \
	} \
	CAGE_FORCE_INLINE Real length(TYPE x) \
	{ \
		return sqrt(lengthSquared(x)); \
	} \
	CAGE_FORCE_INLINE constexpr Real distanceSquared(TYPE l, TYPE r) noexcept \
	{ \
		return lengthSquared(l - r); \
	} \
	CAGE_FORCE_INLINE Real distance(TYPE l, TYPE r) \
	{ \
		return sqrt(distanceSquared(l, r)); \
	} \
	CAGE_FORCE_INLINE TYPE normalize(TYPE x) \
	{ \
		return x / length(x); \
	}
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE constexpr Real dot(Quat l, Quat r) noexcept
	{
		Real sum = 0;
		for (uint32 i = 0; i < 4; i++)
			sum += l[i] * r[i];
		return sum;
	}
	CAGE_FORCE_INLINE constexpr Real lengthSquared(Quat x) noexcept
	{
		return dot(x, x);
	}
	CAGE_FORCE_INLINE Real length(Quat x) noexcept
	{
		return sqrt(lengthSquared(x));
	}
	CAGE_FORCE_INLINE Quat normalize(Quat x) noexcept
	{
		return x / length(x);
	}
	CAGE_FORCE_INLINE constexpr Quat conjugate(Quat x) noexcept
	{
		return Quat(-x[0], -x[1], -x[2], x[3]);
	}
	CAGE_FORCE_INLINE constexpr Real cross(Vec2 l, Vec2 r) noexcept
	{
		return l[0] * r[1] - l[1] * r[0];
	}
	CAGE_FORCE_INLINE constexpr Vec3 cross(Vec3 l, Vec3 r) noexcept
	{
		return Vec3(l[1] * r[2] - l[2] * r[1], l[2] * r[0] - l[0] * r[2], l[0] * r[1] - l[1] * r[0]);
	}
	CAGE_CORE_API Vec3 dominantAxis(Vec3 x);
	CAGE_CORE_API Quat lerp(Quat a, Quat b, Real f);
	CAGE_CORE_API Quat slerp(Quat a, Quat b, Real f);
	CAGE_CORE_API Quat slerpPrecise(Quat a, Quat b, Real f);
	CAGE_CORE_API Quat rotate(Quat from, Quat toward, Rads maxTurn);
	CAGE_CORE_API Rads angle(Quat a, Quat b);
	CAGE_CORE_API Rads pitch(Quat q);
	CAGE_CORE_API Rads yaw(Quat q);
	CAGE_CORE_API Rads roll(Quat q);

	CAGE_CORE_API Mat3 inverse(Mat3 x);
	CAGE_CORE_API Mat3 transpose(Mat3 x) noexcept;
	CAGE_CORE_API Mat3 normalize(Mat3 x);
	CAGE_CORE_API Real determinant(Mat3 x);
	CAGE_CORE_API Mat4 inverse(Mat4 x);
	CAGE_CORE_API Mat4 transpose(Mat4 x) noexcept;
	CAGE_CORE_API Mat4 normalize(Mat4 x);
	CAGE_CORE_API Real determinant(Mat4 x);
	CAGE_CORE_API Transform inverse(Transform x);

	CAGE_CORE_API bool valid(float a) noexcept;
	CAGE_CORE_API bool valid(double a) noexcept;
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE bool valid(TYPE a) noexcept \
	{ \
		return a.valid(); \
	}
	GCHL_GENERATE(Real);
	GCHL_GENERATE(Rads);
	GCHL_GENERATE(Degs);
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
	GCHL_GENERATE(Quat);
	GCHL_GENERATE(Mat3);
	GCHL_GENERATE(Mat4);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(TYPE a, TYPE b) noexcept \
	{ \
		return a < b ? a : b; \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(TYPE a, TYPE b) noexcept \
	{ \
		return a > b ? a : b; \
	} \
	CAGE_FORCE_INLINE constexpr TYPE clamp(TYPE v, TYPE a, TYPE b) \
	{ \
		CAGE_ASSERT(a <= b); \
		return min(max(v, a), b); \
	}
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
	GCHL_GENERATE(Real);
	GCHL_GENERATE(Rads);
	GCHL_GENERATE(Degs);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE saturate(TYPE v) noexcept \
	{ \
		return clamp(v, TYPE(0), TYPE(1)); \
	}
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(Real);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE saturate(TYPE v) noexcept \
	{ \
		return clamp(v, TYPE(0), TYPE::Full()); \
	}
	GCHL_GENERATE(Rads);
	GCHL_GENERATE(Degs);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE abs(TYPE x) noexcept \
	{ \
		TYPE res; \
		for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) \
			res[i] = abs(x[i]); \
		return res; \
	} \
	CAGE_FORCE_INLINE constexpr TYPE min(TYPE l, TYPE r) noexcept \
	{ \
		TYPE res; \
		for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) \
			res[i] = min(l[i], r[i]); \
		return res; \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(TYPE l, TYPE r) noexcept \
	{ \
		TYPE res; \
		for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) \
			res[i] = max(l[i], r[i]); \
		return res; \
	} \
	CAGE_FORCE_INLINE constexpr TYPE clamp(TYPE v, TYPE a, TYPE b) noexcept \
	{ \
		TYPE res; \
		for (uint32 i = 0; i < GCHL_DIMENSION(TYPE); i++) \
			res[i] = clamp(v[i], a[i], b[i]); \
		return res; \
	}
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
	GCHL_GENERATE(Vec2i);
	GCHL_GENERATE(Vec3i);
	GCHL_GENERATE(Vec4i);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(TYPE l, Real r) noexcept \
	{ \
		return min(l, TYPE(r)); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(TYPE l, Real r) noexcept \
	{ \
		return max(l, TYPE(r)); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE min(Real l, TYPE r) noexcept \
	{ \
		return min(TYPE(l), r); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(Real l, TYPE r) noexcept \
	{ \
		return max(TYPE(l), r); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE clamp(TYPE v, Real a, Real b) noexcept \
	{ \
		return clamp(v, TYPE(a), TYPE(b)); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE saturate(TYPE v) noexcept \
	{ \
		return clamp(v, 0, 1); \
	}
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
#undef GCHL_GENERATE
#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE min(TYPE l, sint32 r) noexcept \
	{ \
		return min(l, TYPE(r)); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(TYPE l, sint32 r) noexcept \
	{ \
		return max(l, TYPE(r)); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE min(sint32 l, TYPE r) noexcept \
	{ \
		return min(TYPE(l), r); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE max(sint32 l, TYPE r) noexcept \
	{ \
		return max(TYPE(l), r); \
	} \
	CAGE_FORCE_INLINE constexpr TYPE clamp(TYPE v, sint32 a, sint32 b) noexcept \
	{ \
		return clamp(v, TYPE(a), TYPE(b)); \
	}
	GCHL_GENERATE(Vec2i);
	GCHL_GENERATE(Vec3i);
	GCHL_GENERATE(Vec4i);
#undef GCHL_GENERATE

#define GCHL_GENERATE(TYPE) \
	CAGE_FORCE_INLINE constexpr TYPE interpolate(TYPE a, TYPE b, Real f) noexcept \
	{ \
		return numeric_cast<TYPE>(a * (1 - f.value) + b * f.value); \
	}
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
	CAGE_FORCE_INLINE constexpr TYPE interpolate(TYPE a, TYPE b, Real f) noexcept \
	{ \
		return (TYPE)(a + (b - a) * f.value); \
	}
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
	GCHL_GENERATE(Real);
	GCHL_GENERATE(Rads);
	GCHL_GENERATE(Degs);
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
#undef GCHL_GENERATE
	CAGE_FORCE_INLINE Quat interpolate(Quat a, Quat b, Real f)
	{
		return slerp(a, b, f);
	}
	CAGE_FORCE_INLINE Transform interpolate(Transform a, Transform b, Real f) noexcept
	{
		return Transform(interpolate(a.position, b.position, f), interpolate(a.orientation, b.orientation, f), interpolate(a.scale, b.scale, f));
	}
	CAGE_CORE_API Real interpolateWrap(Real a, Real b, Real f);
	CAGE_FORCE_INLINE Rads interpolateAngle(Rads a, Rads b, Real f)
	{
		return interpolateWrap((a / Rads::Full()).value, (b / Rads::Full()).value, f) * Rads::Full();
	}

	CAGE_CORE_API Real randomChance(); // 0 to 1; including 0, excluding 1
	CAGE_CORE_API double randomChanceDouble(); // 0 to 1; including 0, excluding 1
#define GCHL_GENERATE(TYPE) CAGE_CORE_API TYPE randomRange(TYPE min, TYPE max);
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
	GCHL_GENERATE(Real);
	GCHL_GENERATE(Rads);
	GCHL_GENERATE(Degs);
#undef GCHL_GENERATE
	CAGE_CORE_API Rads randomAngle();
	CAGE_CORE_API Vec2 randomChance2();
	CAGE_CORE_API Vec2 randomRange2(Real a, Real b);
	CAGE_CORE_API Vec2 randomDirection2();
	CAGE_CORE_API Vec2i randomRange2i(sint32 a, sint32 b);
	CAGE_CORE_API Vec3 randomChance3();
	CAGE_CORE_API Vec3 randomRange3(Real a, Real b);
	CAGE_CORE_API Vec3 randomDirection3();
	CAGE_CORE_API Vec3i randomRange3i(sint32 a, sint32 b);
	CAGE_CORE_API Vec4 randomChance4();
	CAGE_CORE_API Vec4 randomRange4(Real a, Real b);
	CAGE_CORE_API Quat randomDirectionQuat();
	CAGE_CORE_API Vec4i randomRange4i(sint32 a, sint32 b);

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
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Real other)
		{
			return str + other.value;
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Degs other)
		{
			return str + other.value.value + "deg";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Rads other)
		{
			return str + other.value.value + "rad";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec2 other)
		{
			return str + "(" + other[0].value + "," + other[1].value + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec3 other)
		{
			return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec4 other)
		{
			return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + "," + other[3].value + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Quat other)
		{
			return str + "(" + other[0].value + "," + other[1].value + "," + other[2].value + "," + other[3].value + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Mat3 other)
		{
			str + "(" + other[0].value;
			for (uint32 i = 1; i < 9; i++)
				str + "," + other[i].value;
			return str + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Mat4 other)
		{
			str + "(" + other[0].value;
			for (uint32 i = 1; i < 16; i++)
				str + "," + other[i].value;
			return str + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Transform other)
		{
			return str + "(" + other.position + "," + other.orientation + "," + other.scale + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec2i other)
		{
			return str + "(" + other[0] + "," + other[1] + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec3i other)
		{
			return str + "(" + other[0] + "," + other[1] + "," + other[2] + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Vec4i other)
		{
			return str + "(" + other[0] + "," + other[1] + "," + other[2] + "," + other[3] + ")";
		}
	}
}

#undef GCHL_DIMENSION

#endif // guard_math_h_c0d63c8d_8398_4b39_81b4_99671252b150_
