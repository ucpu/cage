#ifndef guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
#define guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API Line
	{
		// data
		Vec3 origin = Vec3::Nan();
		Vec3 direction = Vec3::Nan();
		Real minimum = Real::Nan();
		Real maximum = Real::Nan();

		// constructors
		constexpr Line() noexcept {}
		explicit constexpr Line(Vec3 origin, Vec3 direction, Real minimum, Real maximum) noexcept : origin(origin), direction(direction), minimum(minimum), maximum(maximum) {}

		// compound operators
		Line &operator *= (const Mat4 &other) { return *this = *this * other; }
		Line &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Line operator * (const Mat4 &other) const;
		Line operator * (const Transform &other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator == (const Line &) const noexcept = default;

		// methods
		bool valid() const noexcept { return origin.valid() && direction.valid() && minimum.valid() && maximum.valid(); }
		bool isPoint() const noexcept { return valid() && minimum == maximum; }
		bool isLine() const noexcept { return valid() && !minimum.finite() && !maximum.finite(); }
		bool isRay() const noexcept { return valid() && (minimum.finite() != maximum.finite()); }
		bool isSegment() const noexcept { return valid() && minimum.finite() && maximum.finite(); }
		bool normalized() const;
		Line normalize() const;
		Vec3 a() const { CAGE_ASSERT(minimum.finite()); return origin + direction * minimum; }
		Vec3 b() const { CAGE_ASSERT(maximum.finite()); return origin + direction * maximum; }
	};

	struct CAGE_CORE_API Triangle
	{
		// data
		Vec3 vertices[3] { Vec3::Nan(), Vec3::Nan(), Vec3::Nan() };

		// constructors
		constexpr Triangle() noexcept {}
		explicit constexpr Triangle(const Vec3 vertices[3]) noexcept : vertices{ vertices[0], vertices[1], vertices[2] } {}
		explicit constexpr Triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c) noexcept : vertices{ a, b, c } {}
		explicit constexpr Triangle(const Real coords[9]) noexcept : vertices{ Vec3(coords[0], coords[1], coords[2]), Vec3(coords[3], coords[4], coords[5]), Vec3(coords[6], coords[7], coords[8]) } {}

		// compound operators
		Triangle &operator *= (const Mat4 &other) { return *this = *this * other; }
		Triangle &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Triangle operator * (const Mat4 &other) const;
		Triangle operator * (const Transform &other) const { return *this * Mat4(other); }

		constexpr Vec3 operator [] (uint32 idx) const { CAGE_ASSERT(idx < 3); return vertices[idx]; }
		constexpr Vec3 &operator [] (uint32 idx) { CAGE_ASSERT(idx < 3); return vertices[idx]; }

		// comparison operators
		bool operator == (const Triangle &) const noexcept = default;

		// methods
		bool valid() const noexcept { return vertices[0].valid() && vertices[1].valid() && vertices[2].valid(); };
		bool degenerated() const;
		Vec3 normal() const { return normalize(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))); }
		Vec3 center() const noexcept { return (vertices[0] + vertices[1] + vertices[2]) / 3; }
		Real area() const noexcept { return length(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))) * 0.5; }
		Triangle flip() const;
	};

	struct CAGE_CORE_API Plane
	{
		// data
		Vec3 normal = Vec3::Nan();
		Real d = Real::Nan();

		// constructors
		constexpr Plane() noexcept {}
		explicit constexpr Plane(const Vec3 &normal, Real d) noexcept : normal(normal), d(d) {}
		explicit Plane(const Vec3 &point, const Vec3 &normal);
		explicit Plane(const Vec3 &a, const Vec3 &b, const Vec3 &c);
		explicit Plane(const Triangle &other);
		explicit Plane(const Line &a, const Vec3 &b);

		// compound operators
		Plane &operator *= (const Mat4 &other) { return *this = *this * other; }
		Plane &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Plane operator * (const Mat4 &other) const;
		Plane operator * (const Transform &other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator == (const Plane &) const noexcept = default;

		// methods
		bool valid() const noexcept { return d.valid() && normal.valid(); }
		bool normalized() const;
		Plane normalize() const;
		Vec3 origin() const noexcept { return normal * -d; }
	};

	struct CAGE_CORE_API Sphere
	{
		// data
		Vec3 center = Vec3::Nan();
		Real radius = Real::Nan();

		// constructors
		constexpr Sphere() noexcept {}
		explicit constexpr Sphere(const Vec3 &center, Real radius) noexcept : center(center), radius(radius) {}
		explicit Sphere(const Line &other);
		explicit Sphere(const Triangle &other);
		explicit Sphere(const Aabb &other);
		explicit Sphere(const Cone &other);
		explicit Sphere(const Frustum &other);

		// compound operators
		Sphere &operator *= (const Mat4 &other) { return *this = *this * other; }
		Sphere &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Sphere operator * (const Mat4 &other) const;
		Sphere operator * (const Transform &other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator == (const Sphere &other) const noexcept { return (empty() && other.empty()) || (center == other.center && radius == other.radius); }

		// methods
		bool valid() const noexcept { return center.valid() && radius >= 0; }
		bool empty() const noexcept { return !valid(); }
		Real volume() const noexcept { return empty() ? 0 : 4 * Real::Pi() * radius * radius * radius / 3; }
		Real surface() const noexcept { return empty() ? 0 : 4 * Real::Pi() * radius * radius; }
	};

	struct CAGE_CORE_API Aabb
	{
		// data
		Vec3 a = Vec3::Nan(), b = Vec3::Nan();

		// constructor
		constexpr Aabb() noexcept {}
		explicit constexpr Aabb(const Vec3 &point) noexcept : a(point), b(point) {}
		explicit constexpr Aabb(const Vec3 &a, const Vec3 &b) noexcept  : a(min(a, b)), b(max(a, b)) {}
		explicit constexpr Aabb(const Triangle &other) noexcept : a(min(min(other[0], other[1]), other[2])), b(max(max(other[0], other[1]), other[2])) {}
		explicit constexpr Aabb(const Sphere &other) noexcept : a(other.center - other.radius), b(other.center + other.radius) {}
		explicit Aabb(const Line &other);
		explicit Aabb(const Plane &other);
		explicit Aabb(const Cone &other);
		explicit Aabb(const Frustum &other);

		// compound operators
		Aabb &operator += (const Aabb &other) { return *this = *this + other; }
		Aabb &operator *= (const Mat4 &other) { return *this = *this * other; }
		Aabb &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Aabb operator + (const Aabb &other) const;
		Aabb operator * (const Mat4 &other) const;
		Aabb operator * (const Transform &other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator == (const Aabb &other) const noexcept { return (empty() && other.empty()) || (a == other.a && b == other.b); }

		// methods
		bool valid() const noexcept { return a.valid() && b.valid(); }
		bool empty() const noexcept { return !valid(); }
		Real volume() const;
		Real surface() const;
		Vec3 center() const noexcept { return empty() ? Vec3() : (a + b) * 0.5; }
		Vec3 size() const noexcept { return empty() ? Vec3() : b - a; }
		Real diagonal() const { return empty() ? 0 : distance(a, b); }

		// constants
		static constexpr Aabb Universe() { return Aabb(Vec3(-Real::Infinity()), Vec3(Real::Infinity())); }
	};

	struct CAGE_CORE_API Cone
	{
		// data
		Vec3 origin = Vec3::Nan();
		Vec3 direction = Vec3::Nan();
		Real length = Real::Nan();
		Rads halfAngle = Rads::Nan();

		// constructor
		constexpr Cone() noexcept {}
		explicit constexpr Cone(const Vec3 &origin, const Vec3 &direction, Real length, Rads halfAngle) noexcept : origin(origin), direction(direction), length(length), halfAngle(halfAngle) {}

		// compound operators
		Cone &operator *= (const Mat4 &other) { return *this = *this * other; }
		Cone &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Cone operator * (const Mat4 &other) const;
		Cone operator * (const Transform &other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator == (const Cone &other) const noexcept { return (empty() && other.empty()) || (origin == other.origin && direction == other.direction && length == other.length && halfAngle == other.halfAngle); }

		// methods
		bool valid() const noexcept { return origin.valid() && direction.valid() && length.valid() && halfAngle.valid(); }
		bool empty() const noexcept { return !valid(); }
	};

	// note: some intersection tests with frustums are conservative
	struct CAGE_CORE_API Frustum
	{
		// data
		Mat4 viewProj;
		Vec4 planes[6];

		// constructor
		constexpr Frustum() noexcept {}
		explicit Frustum(const Transform &camera, const Mat4 &proj);
		explicit Frustum(const Mat4 &viewProj);

		// compound operators
		Frustum &operator *= (const Mat4 &other) { return *this = *this * other; }
		Frustum &operator *= (const Transform &other) { return *this = *this * other; }

		// binary operators
		Frustum operator * (const Mat4 &other) const;
		Frustum operator * (const Transform &other) const { return *this * Mat4(other); }

		// methods
		struct Corners { Vec3 data[8]; };
		Corners corners() const;
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

	CAGE_CORE_API Line makeSegment(const Vec3 &a, const Vec3 &b);
	CAGE_CORE_API Line makeRay(const Vec3 &a, const Vec3 &b);
	CAGE_CORE_API Line makeLine(const Vec3 &a, const Vec3 &b);

	inline Sphere::Sphere(const Aabb &other) : center(other.center()), radius(other.diagonal() * 0.5) {}

	CAGE_CORE_API Sphere makeSphere(PointerRange<const Vec3> points); // minimum bounding sphere

	CAGE_CORE_API Rads angle(const Line &a, const Line &b);
	CAGE_CORE_API Rads angle(const Line &a, const Triangle &b);
	CAGE_CORE_API Rads angle(const Line &a, const Plane &b);
	CAGE_CORE_API Rads angle(const Triangle &a, const Triangle &b);
	CAGE_CORE_API Rads angle(const Triangle &a, const Plane &b);
	CAGE_CORE_API Rads angle(const Plane &a, const Plane &b);

	CAGE_CORE_API Real distance(const Vec3 &a, const Line &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Triangle &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Plane &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Sphere &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Cone &b);
	CAGE_CORE_API Real distance(const Vec3 &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Line &a, const Line &b);
	CAGE_CORE_API Real distance(const Line &a, const Triangle &b);
	CAGE_CORE_API Real distance(const Line &a, const Plane &b);
	CAGE_CORE_API Real distance(const Line &a, const Sphere &b);
	CAGE_CORE_API Real distance(const Line &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Line &a, const Cone &b);
	CAGE_CORE_API Real distance(const Line &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Triangle &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Plane &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Sphere &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Cone &b);
	CAGE_CORE_API Real distance(const Triangle &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Plane &a, const Plane &b);
	CAGE_CORE_API Real distance(const Plane &a, const Sphere &b);
	CAGE_CORE_API Real distance(const Plane &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Plane &a, const Cone &b);
	CAGE_CORE_API Real distance(const Plane &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Sphere &a, const Sphere &b);
	CAGE_CORE_API Real distance(const Sphere &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Sphere &a, const Cone &b);
	CAGE_CORE_API Real distance(const Sphere &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Aabb &a, const Aabb &b);
	CAGE_CORE_API Real distance(const Aabb &a, const Cone &b);
	CAGE_CORE_API Real distance(const Aabb &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Cone &a, const Cone &b);
	CAGE_CORE_API Real distance(const Cone &a, const Frustum &b);
	CAGE_CORE_API Real distance(const Frustum &a, const Frustum &b);

	CAGE_CORE_API bool intersects(const Vec3 &a, const Vec3 &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Line &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Triangle &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Plane &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Sphere &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Aabb &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Cone &b);
	CAGE_CORE_API bool intersects(const Vec3 &a, const Frustum &b);
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

	CAGE_CORE_API Vec3 intersection(const Line &a, const Triangle &b);
	CAGE_CORE_API Vec3 intersection(const Line &a, const Plane &b);
	CAGE_CORE_API Line intersection(const Line &a, const Sphere &b);
	CAGE_CORE_API Line intersection(const Line &a, const Aabb &b);
	CAGE_CORE_API Line intersection(const Line &a, const Cone &b);
	CAGE_CORE_API Line intersection(const Line &a, const Frustum &b);
	CAGE_CORE_API Aabb intersection(const Aabb &a, const Aabb &b);

	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Line &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Triangle &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Plane &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Sphere &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Aabb &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Cone &b);
	CAGE_CORE_API Vec3 closestPoint(const Vec3 &a, const Frustum &b);

	namespace privat
	{
		template<class T> struct GeometryOrder;
		template<> struct GeometryOrder<Vec2> { static constexpr int order = 1; };
		template<> struct GeometryOrder<Vec3> { static constexpr int order = 2; };
		template<> struct GeometryOrder<Vec4> { static constexpr int order = 3; };
		template<> struct GeometryOrder<Line> { static constexpr int order = 4; };
		template<> struct GeometryOrder<Triangle> { static constexpr int order = 5; };
		template<> struct GeometryOrder<Plane> { static constexpr int order = 6; };
		template<> struct GeometryOrder<Sphere> { static constexpr int order = 7; };
		template<> struct GeometryOrder<Aabb> { static constexpr int order = 8; };
		template<> struct GeometryOrder<Cone> { static constexpr int order = 9; };
		template<> struct GeometryOrder<Frustum> { static constexpr int order = 10; };
		template<class A, class B> constexpr bool geometrySwapParameters = GeometryOrder<A>::order > GeometryOrder<B>::order;
	}

	template<class A, class B>
	CAGE_FORCE_INLINE auto angle(const A &a, const B &b) requires(privat::geometrySwapParameters<A, B>) { return angle(b, a); }
	template<class A, class B>
	CAGE_FORCE_INLINE auto distance(const A &a, const B &b) requires(privat::geometrySwapParameters<A, B>) { return distance(b, a); }
	template<class A, class B>
	CAGE_FORCE_INLINE auto intersects(const A &a, const B &b) requires(privat::geometrySwapParameters<A, B>) { return intersects(b, a); }
	template<class A, class B>
	CAGE_FORCE_INLINE auto intersection(const A &a, const B &b) requires(privat::geometrySwapParameters<A, B>) { return intersection(b, a); }
	template<class A, class B>
	CAGE_FORCE_INLINE auto closestPoint(const A &a, const B &b) requires(privat::geometrySwapParameters<A, B>) { return closestPoint(b, a); }
}

#endif // guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
