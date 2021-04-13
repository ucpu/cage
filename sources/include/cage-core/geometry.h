#ifndef guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
#define guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API Line
	{
		// data
		vec3 origin = vec3::Nan();
		vec3 direction = vec3::Nan();
		real minimum = real::Nan();
		real maximum = real::Nan();

		// constructors
		constexpr Line() noexcept {}
		explicit constexpr Line(vec3 origin, vec3 direction, real minimum, real maximum) noexcept : origin(origin), direction(direction), minimum(minimum), maximum(maximum) {}

		// compound operators
		Line &operator *= (const mat4 &other) { return *this = *this * other; }
		Line &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Line operator * (const mat4 &other) const;
		Line operator * (const transform &other) const { return *this * mat4(other); }

		// comparison operators
		constexpr bool operator == (const Line &other) const noexcept { return origin == other.origin && direction == other.direction && minimum == other.minimum && maximum == other.maximum; }
		constexpr bool operator != (const Line &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return origin.valid() && direction.valid() && minimum.valid() && maximum.valid(); }
		bool isPoint() const noexcept { return valid() && minimum == maximum; }
		bool isLine() const noexcept { return valid() && !minimum.finite() && !maximum.finite(); }
		bool isRay() const noexcept { return valid() && (minimum.finite() != maximum.finite()); }
		bool isSegment() const noexcept { return valid() && minimum.finite() && maximum.finite(); }
		bool normalized() const;
		Line normalize() const;
		vec3 a() const { CAGE_ASSERT(minimum.finite()); return origin + direction * minimum; }
		vec3 b() const { CAGE_ASSERT(maximum.finite()); return origin + direction * maximum; }
	};

	struct CAGE_CORE_API Triangle
	{
		// data
		vec3 vertices[3] { vec3::Nan(), vec3::Nan(), vec3::Nan() };

		// constructors
		constexpr Triangle() noexcept {}
		explicit constexpr Triangle(const vec3 vertices[3]) noexcept : vertices{ vertices[0], vertices[1], vertices[2] } {}
		explicit constexpr Triangle(const vec3 &a, const vec3 &b, const vec3 &c) noexcept : vertices{ a, b, c } {}
		explicit constexpr Triangle(const real coords[9]) noexcept : vertices{ vec3(coords[0], coords[1], coords[2]), vec3(coords[3], coords[4], coords[5]), vec3(coords[6], coords[7], coords[8]) } {}

		// compound operators
		Triangle &operator *= (const mat4 &other) { return *this = *this * other; }
		Triangle &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Triangle operator * (const mat4 &other) const;
		Triangle operator * (const transform &other) const { return *this * mat4(other); }

		constexpr vec3 operator [] (uint32 idx) const { CAGE_ASSERT(idx < 3); return vertices[idx]; }
		constexpr vec3 &operator [] (uint32 idx) { CAGE_ASSERT(idx < 3); return vertices[idx]; }

		// comparison operators
		constexpr bool operator == (const Triangle &other) const noexcept { for (uint8 i = 0; i < 3; i++) if (vertices[i] != other.vertices[i]) return false; return true; }
		constexpr bool operator != (const Triangle &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return vertices[0].valid() && vertices[1].valid() && vertices[2].valid(); };
		bool degenerated() const;
		vec3 normal() const { return normalize(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))); }
		vec3 center() const noexcept { return (vertices[0] + vertices[1] + vertices[2]) / 3; }
		real area() const noexcept { return length(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))) * 0.5; }
		Triangle flip() const;
	};

	struct CAGE_CORE_API Plane
	{
		// data
		vec3 normal = vec3::Nan();
		real d = real::Nan();

		// constructors
		constexpr Plane() noexcept {}
		explicit constexpr Plane(const vec3 &normal, real d) noexcept : normal(normal), d(d) {}
		explicit Plane(const vec3 &point, const vec3 &normal);
		explicit Plane(const vec3 &a, const vec3 &b, const vec3 &c);
		explicit Plane(const Triangle &other);
		explicit Plane(const Line &a, const vec3 &b);

		// compound operators
		Plane &operator *= (const mat4 &other) { return *this = *this * other; }
		Plane &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Plane operator * (const mat4 &other) const;
		Plane operator * (const transform &other) const { return *this * mat4(other); }

		// comparison operators
		constexpr bool operator == (const Plane &other) const noexcept { return d == other.d && normal == other.normal; }
		constexpr bool operator != (const Plane &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return d.valid() && normal.valid(); }
		bool normalized() const;
		Plane normalize() const;
		vec3 origin() const noexcept { return normal * -d; }
	};

	struct CAGE_CORE_API Sphere
	{
		// data
		vec3 center = vec3::Nan();
		real radius = real::Nan();

		// constructors
		constexpr Sphere() noexcept {}
		explicit constexpr Sphere(const vec3 &center, real radius) noexcept : center(center), radius(radius) {}
		explicit Sphere(const Line &other);
		explicit Sphere(const Triangle &other);
		explicit Sphere(const Aabb &other);
		explicit Sphere(const Cone &other);
		explicit Sphere(const Frustum &other);

		// compound operators
		Sphere &operator *= (const mat4 &other) { return *this = *this * other; }
		Sphere &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Sphere operator * (const mat4 &other) const;
		Sphere operator * (const transform &other) const { return *this * mat4(other); }

		// comparison operators
		bool operator == (const Sphere &other) const noexcept { return (empty() == other.empty()) || (center == other.center && radius == other.radius); }
		bool operator != (const Sphere &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return center.valid() && radius >= 0; }
		bool empty() const noexcept { return !valid(); }
		real volume() const noexcept { return empty() ? 0 : 4 * real::Pi() * radius * radius * radius / 3; }
		real surface() const noexcept { return empty() ? 0 : 4 * real::Pi() * radius * radius; }
	};

	struct CAGE_CORE_API Aabb
	{
		// data
		vec3 a = vec3::Nan(), b = vec3::Nan();

		// constructor
		constexpr Aabb() noexcept {}
		explicit constexpr Aabb(const vec3 &point) noexcept : a(point), b(point) {}
		explicit constexpr Aabb(const vec3 &a, const vec3 &b) noexcept  : a(min(a, b)), b(max(a, b)) {}
		explicit constexpr Aabb(const Triangle &other) noexcept : a(min(min(other[0], other[1]), other[2])), b(max(max(other[0], other[1]), other[2])) {}
		explicit constexpr Aabb(const Sphere &other) noexcept : a(other.center - other.radius), b(other.center + other.radius) {}
		explicit Aabb(const Line &other);
		explicit Aabb(const Plane &other);
		explicit Aabb(const Cone &other);
		explicit Aabb(const Frustum &other);

		// compound operators
		Aabb &operator += (const Aabb &other) { return *this = *this + other; }
		Aabb &operator *= (const mat4 &other) { return *this = *this * other; }
		Aabb &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Aabb operator + (const Aabb &other) const;
		Aabb operator * (const mat4 &other) const;
		Aabb operator * (const transform &other) const { return *this * mat4(other); }

		// comparison operators
		bool operator == (const Aabb &other) const noexcept { return (empty() == other.empty()) && (empty() || (a == other.a && b == other.b)); }
		bool operator != (const Aabb &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return a.valid() && b.valid(); }
		bool empty() const noexcept { return !valid(); }
		real volume() const;
		real surface() const;
		vec3 center() const noexcept { return empty() ? vec3() : (a + b) * 0.5; }
		vec3 size() const noexcept { return empty() ? vec3() : b - a; }
		real diagonal() const { return empty() ? 0 : distance(a, b); }

		// constants
		static constexpr Aabb Universe() { return Aabb(vec3(-real::Infinity()), vec3(real::Infinity())); }
	};

	struct CAGE_CORE_API Cone
	{
		// data
		vec3 origin = vec3::Nan();
		vec3 direction = vec3::Nan();
		real length = real::Nan();
		rads halfAngle = rads::Nan();

		// constructor
		constexpr Cone() noexcept {}
		explicit constexpr Cone(const vec3 &origin, const vec3 &direction, real length, rads halfAngle) noexcept : origin(origin), direction(direction), length(length), halfAngle(halfAngle) {}

		// compound operators
		Cone &operator *= (const mat4 &other) { return *this = *this * other; }
		Cone &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Cone operator * (const mat4 &other) const;
		Cone operator * (const transform &other) const { return *this * mat4(other); }

		// comparison operators
		bool operator == (const Cone &other) const noexcept { return origin == other.origin && direction == other.direction && length == other.length && halfAngle == other.halfAngle; }
		bool operator != (const Cone &other) const noexcept { return !(*this == other); }

		// methods
		bool valid() const noexcept { return origin.valid() && direction.valid() && length.valid() && halfAngle.valid(); }
	};

	// note: some intersection tests with frustums are conservative
	struct CAGE_CORE_API Frustum
	{
		// data
		mat4 viewProj;
		vec4 planes[6];

		// compound operators
		Frustum &operator *= (const mat4 &other) { return *this = *this * other; }
		Frustum &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		Frustum operator * (const mat4 &other) const;
		Frustum operator * (const transform &other) const { return *this * mat4(other); }

		// constructor
		constexpr Frustum() noexcept {}
		explicit Frustum(const transform &camera, const mat4 &proj);
		explicit Frustum(const mat4 &viewProj);
	};

	namespace detail
	{
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Line &other) { return str + "(" + other.origin + ", " + other.direction + ", " + other.minimum + ", " + other.maximum + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Triangle &other) { return str + "(" + other.vertices[0] + ", " + other.vertices[1] + ", " + other.vertices[2] + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Plane &other) { return str + "(" + other.normal + ", " + other.d + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Sphere &other) { return str + "(" + other.center + ", " + other.radius + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Aabb &other) { return str + "(" + other.a + "," + other.b + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Cone &other) { return str + "(" + other.origin + "," + other.direction + "," + other.length + "," + other.halfAngle + ")"; }
		template<uint32 N> inline StringizerBase<N> &operator + (StringizerBase<N> &str, const Frustum &other) { return str + other.viewProj; }
	}

	CAGE_CORE_API Line makeSegment(const vec3 &a, const vec3 &b);
	CAGE_CORE_API Line makeRay(const vec3 &a, const vec3 &b);
	CAGE_CORE_API Line makeLine(const vec3 &a, const vec3 &b);

	inline Sphere::Sphere(const Aabb &other) : center(other.center()), radius(other.diagonal() * 0.5) {}

	CAGE_CORE_API rads angle(const Line &a, const Line &b);
	CAGE_CORE_API rads angle(const Line &a, const Triangle &b);
	CAGE_CORE_API rads angle(const Line &a, const Plane &b);
	CAGE_CORE_API rads angle(const Triangle &a, const Triangle &b);
	CAGE_CORE_API rads angle(const Triangle &a, const Plane &b);
	CAGE_CORE_API rads angle(const Plane &a, const Plane &b);

	CAGE_CORE_API real distance(const vec3 &a, const Line &b);
	CAGE_CORE_API real distance(const vec3 &a, const Triangle &b);
	CAGE_CORE_API real distance(const vec3 &a, const Plane &b);
	CAGE_CORE_API real distance(const vec3 &a, const Sphere &b);
	CAGE_CORE_API real distance(const vec3 &a, const Aabb &b);
	CAGE_CORE_API real distance(const vec3 &a, const Cone &b);
	CAGE_CORE_API real distance(const vec3 &a, const Frustum &b);
	CAGE_CORE_API real distance(const Line &a, const Line &b);
	CAGE_CORE_API real distance(const Line &a, const Triangle &b);
	CAGE_CORE_API real distance(const Line &a, const Plane &b);
	CAGE_CORE_API real distance(const Line &a, const Sphere &b);
	CAGE_CORE_API real distance(const Line &a, const Aabb &b);
	CAGE_CORE_API real distance(const Line &a, const Cone &b);
	CAGE_CORE_API real distance(const Line &a, const Frustum &b);
	CAGE_CORE_API real distance(const Triangle &a, const Triangle &b);
	CAGE_CORE_API real distance(const Triangle &a, const Plane &b);
	CAGE_CORE_API real distance(const Triangle &a, const Sphere &b);
	CAGE_CORE_API real distance(const Triangle &a, const Aabb &b);
	CAGE_CORE_API real distance(const Triangle &a, const Cone &b);
	CAGE_CORE_API real distance(const Triangle &a, const Frustum &b);
	CAGE_CORE_API real distance(const Plane &a, const Plane &b);
	CAGE_CORE_API real distance(const Plane &a, const Sphere &b);
	CAGE_CORE_API real distance(const Plane &a, const Aabb &b);
	CAGE_CORE_API real distance(const Plane &a, const Cone &b);
	CAGE_CORE_API real distance(const Plane &a, const Frustum &b);
	CAGE_CORE_API real distance(const Sphere &a, const Sphere &b);
	CAGE_CORE_API real distance(const Sphere &a, const Aabb &b);
	CAGE_CORE_API real distance(const Sphere &a, const Cone &b);
	CAGE_CORE_API real distance(const Sphere &a, const Frustum &b);
	CAGE_CORE_API real distance(const Aabb &a, const Aabb &b);
	CAGE_CORE_API real distance(const Aabb &a, const Cone &b);
	CAGE_CORE_API real distance(const Aabb &a, const Frustum &b);
	CAGE_CORE_API real distance(const Cone &a, const Cone &b);
	CAGE_CORE_API real distance(const Cone &a, const Frustum &b);
	CAGE_CORE_API real distance(const Frustum &a, const Frustum &b);

	CAGE_CORE_API bool intersects(const vec3 &a, const vec3 &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Line &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Triangle &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Plane &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Cone &b);
	CAGE_CORE_API bool intersects(const vec3 &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Line &a, const Line &b);
	CAGE_CORE_API bool intersects(const Line &a, const Triangle &b);
	CAGE_CORE_API bool intersects(const Line &a, const Plane &b);
	CAGE_CORE_API bool intersects(const Line &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const Line &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Line &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Line &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Triangle &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Plane &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Triangle &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Plane &a, const Plane &b);
	CAGE_CORE_API bool intersects(const Plane &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const Plane &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Plane &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Plane &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Sphere &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const Sphere &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Sphere &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Sphere &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Aabb &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Aabb &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Aabb &a, const Frustum &b);
	CAGE_CORE_API bool intersects(const Cone &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Cone &a, const Frustum &b);

	CAGE_CORE_API vec3 intersection(const Line &a, const Triangle &b);
	CAGE_CORE_API vec3 intersection(const Line &a, const Plane &b);
	CAGE_CORE_API Line intersection(const Line &a, const Sphere &b);
	CAGE_CORE_API Line intersection(const Line &a, const Aabb &b);
	CAGE_CORE_API Line intersection(const Line &a, const Cone &b);
	CAGE_CORE_API Line intersection(const Line &a, const Frustum &b);
	CAGE_CORE_API Aabb intersection(const Aabb &a, const Aabb &b);

	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Line &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Triangle &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Plane &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Sphere &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Aabb &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Cone &b);
	CAGE_CORE_API vec3 closestPoint(const vec3 &a, const Frustum &b);

	namespace privat
	{
		template<class T> struct GeometryOrder {};
		template<> struct GeometryOrder<vec3> { static constexpr int order = 1; };
		template<> struct GeometryOrder<Line> { static constexpr int order = 2; };
		template<> struct GeometryOrder<Triangle> { static constexpr int order = 3; };
		template<> struct GeometryOrder<Plane> { static constexpr int order = 4; };
		template<> struct GeometryOrder<Sphere> { static constexpr int order = 5; };
		template<> struct GeometryOrder<Aabb> { static constexpr int order = 6; };
		template<> struct GeometryOrder<Cone> { static constexpr int order = 7; };
		template<> struct GeometryOrder<Frustum> { static constexpr int order = 8; };

		// todo replace sfinae with requires (c++20)
		template<class A, class B, bool enabled = (GeometryOrder<A>::order > GeometryOrder<B>::order)> struct GeometryOrderedType {};
		template<class A, class B> struct GeometryOrderedType<A, B, true> { using type = int; };
		template<class A, class B> using GeometryOrderedEnabled = typename GeometryOrderedType<A, B>::type;
	}

	template<class A, class B, privat::GeometryOrderedEnabled<A, B> = 1>
	CAGE_FORCE_INLINE auto angle(const A &a, const B &b) { return angle(b, a); }
	template<class A, class B, privat::GeometryOrderedEnabled<A, B> = 1>
	CAGE_FORCE_INLINE auto distance(const A &a, const B &b) { return distance(b, a); }
	template<class A, class B, privat::GeometryOrderedEnabled<A, B> = 1>
	CAGE_FORCE_INLINE auto intersects(const A &a, const B &b) { return intersects(b, a); }
	template<class A, class B, privat::GeometryOrderedEnabled<A, B> = 1>
	CAGE_FORCE_INLINE auto intersection(const A &a, const B &b) { return intersection(b, a); }
	template<class A, class B, privat::GeometryOrderedEnabled<A, B> = 1>
	CAGE_FORCE_INLINE auto closestPoint(const A &a, const B &b) { return closestPoint(b, a); }
}

#endif // guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
