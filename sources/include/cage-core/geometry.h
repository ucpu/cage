#ifndef guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
#define guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_

#include <cage-core/math.h>

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
		constexpr Line() {}
		explicit constexpr Line(Vec3 origin, Vec3 direction, Real minimum, Real maximum) : origin(origin), direction(direction), minimum(minimum), maximum(maximum) {}

		// compound operators
		Line &operator*=(Mat4 other) { return *this = *this * other; }
		Line &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Line operator*(Mat4 other) const;
		Line operator*(Transform other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator==(const Line &) const = default;

		// methods
		bool valid() const { return origin.valid() && direction.valid() && minimum.valid() && maximum.valid(); }
		bool isPoint() const { return valid() && minimum == maximum; }
		bool isLine() const { return valid() && !minimum.finite() && !maximum.finite(); }
		bool isRay() const { return valid() && (minimum.finite() != maximum.finite()); }
		bool isSegment() const { return valid() && minimum.finite() && maximum.finite(); }
		bool normalized() const;
		Line normalize() const;
		Vec3 a() const
		{
			CAGE_ASSERT(minimum.finite());
			return origin + direction * minimum;
		}
		Vec3 b() const
		{
			CAGE_ASSERT(maximum.finite());
			return origin + direction * maximum;
		}
	};

	struct CAGE_CORE_API Triangle
	{
		// data
		Vec3 vertices[3] = { Vec3::Nan(), Vec3::Nan(), Vec3::Nan() };

		// constructors
		constexpr Triangle() {}
		explicit constexpr Triangle(const Vec3 vertices[3]) : vertices{ vertices[0], vertices[1], vertices[2] } {}
		explicit constexpr Triangle(Vec3 a, Vec3 b, Vec3 c) : vertices{ a, b, c } {}
		explicit constexpr Triangle(const Real coords[9]) : vertices{ Vec3(coords[0], coords[1], coords[2]), Vec3(coords[3], coords[4], coords[5]), Vec3(coords[6], coords[7], coords[8]) } {}

		// compound operators
		Triangle &operator*=(Mat4 other) { return *this = *this * other; }
		Triangle &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Triangle operator*(Mat4 other) const;
		Triangle operator*(Transform other) const { return *this * Mat4(other); }

		constexpr Vec3 operator[](uint32 idx) const
		{
			CAGE_ASSERT(idx < 3);
			return vertices[idx];
		}
		constexpr Vec3 &operator[](uint32 idx)
		{
			CAGE_ASSERT(idx < 3);
			return vertices[idx];
		}

		// comparison operators
		bool operator==(const Triangle &) const = default;

		// methods
		bool valid() const { return vertices[0].valid() && vertices[1].valid() && vertices[2].valid(); };
		bool degenerated() const;
		Vec3 normal() const { return normalize(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))); }
		Vec3 center() const { return (vertices[0] + vertices[1] + vertices[2]) / 3; }
		Real area() const { return length(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))) * 0.5; }
		Triangle flip() const;
	};

	struct CAGE_CORE_API Plane
	{
		// data
		Vec3 normal = Vec3::Nan();
		Real d = Real::Nan();

		// constructors
		constexpr Plane() {}
		explicit constexpr Plane(Vec3 normal, Real d) : normal(normal), d(d) {}
		explicit Plane(Vec3 point, Vec3 normal);
		explicit Plane(Vec3 a, Vec3 b, Vec3 c);
		explicit Plane(Triangle other);
		explicit Plane(Line a, Vec3 b);

		// compound operators
		Plane &operator*=(Mat4 other) { return *this = *this * other; }
		Plane &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Plane operator*(Mat4 other) const;
		Plane operator*(Transform other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator==(const Plane &) const = default;

		// methods
		bool valid() const { return d.valid() && normal.valid(); }
		bool normalized() const;
		Plane normalize() const;
		Vec3 origin() const { return normal * -d; }
	};

	struct CAGE_CORE_API Sphere
	{
		// data
		Vec3 center = Vec3::Nan();
		Real radius = Real::Nan();

		// constructors
		constexpr Sphere() {}
		explicit constexpr Sphere(Vec3 center, Real radius) : center(center), radius(radius) {}
		explicit Sphere(Line other);
		explicit Sphere(Triangle other);
		explicit Sphere(Aabb other);
		explicit Sphere(Cone other);
		explicit Sphere(const Frustum &other);

		// compound operators
		Sphere &operator*=(Mat4 other) { return *this = *this * other; }
		Sphere &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Sphere operator*(Mat4 other) const;
		Sphere operator*(Transform other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator==(const Sphere &other) const { return (empty() && other.empty()) || (center == other.center && radius == other.radius); }

		// methods
		bool valid() const { return center.valid() && radius >= 0; }
		bool empty() const { return !valid(); }
		Real volume() const { return empty() ? 0 : 4 * Real::Pi() * radius * radius * radius / 3; }
		Real surface() const { return empty() ? 0 : 4 * Real::Pi() * radius * radius; }
	};

	struct CAGE_CORE_API Aabb
	{
		// data
		Vec3 a = Vec3::Nan(), b = Vec3::Nan();

		// constructor
		constexpr Aabb() {}
		explicit constexpr Aabb(Vec3 point) : a(point), b(point) {}
		explicit constexpr Aabb(Vec3 a, Vec3 b) : a(min(a, b)), b(max(a, b)) {}
		explicit constexpr Aabb(Triangle other) : a(min(min(other[0], other[1]), other[2])), b(max(max(other[0], other[1]), other[2])) {}
		explicit constexpr Aabb(Sphere other) : a(other.center - other.radius), b(other.center + other.radius) {}
		explicit Aabb(Line other);
		explicit Aabb(Plane other);
		explicit Aabb(Cone other);
		explicit Aabb(const Frustum &other);

		// compound operators
		Aabb &operator+=(Aabb other) { return *this = *this + other; }
		Aabb &operator*=(Mat4 other) { return *this = *this * other; }
		Aabb &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Aabb operator+(Aabb other) const;
		Aabb operator*(Mat4 other) const;
		Aabb operator*(Transform other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator==(const Aabb &other) const { return (empty() && other.empty()) || (a == other.a && b == other.b); }

		// methods
		bool valid() const { return a.valid() && b.valid(); }
		bool empty() const { return !valid(); }
		Real volume() const;
		Real surface() const;
		Vec3 center() const { return empty() ? Vec3() : (a + b) * 0.5; }
		Vec3 size() const { return empty() ? Vec3() : b - a; }
		Real diagonal() const { return empty() ? 0 : distance(a, b); }

		struct Corners
		{
			Vec3 data[8];
		};
		Corners corners() const;

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
		constexpr Cone() {}
		explicit constexpr Cone(Vec3 origin, Vec3 direction, Real length, Rads halfAngle) : origin(origin), direction(direction), length(length), halfAngle(halfAngle) {}

		// compound operators
		Cone &operator*=(Mat4 other) { return *this = *this * other; }
		Cone &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Cone operator*(Mat4 other) const;
		Cone operator*(Transform other) const { return *this * Mat4(other); }

		// comparison operators
		bool operator==(const Cone &other) const { return (empty() && other.empty()) || (origin == other.origin && direction == other.direction && length == other.length && halfAngle == other.halfAngle); }

		// methods
		bool valid() const { return origin.valid() && direction.valid() && length.valid() && halfAngle.valid(); }
		bool empty() const { return !valid(); }
	};

	// note: some intersection tests with frustums are inexact (conservative)
	struct CAGE_CORE_API Frustum
	{
		// data
		Mat4 viewProj = Mat4::Nan();
		Vec4 planes[6] = { Vec4::Nan(), Vec4::Nan(), Vec4::Nan(), Vec4::Nan(), Vec4::Nan(), Vec4::Nan() };

		// constructor
		constexpr Frustum() {}
		explicit Frustum(Transform camera, Mat4 proj);
		explicit Frustum(Mat4 viewProj);

		// compound operators
		Frustum &operator*=(Mat4 other) { return *this = *this * other; }
		Frustum &operator*=(Transform other) { return *this = *this * other; }

		// binary operators
		Frustum operator*(Mat4 other) const;
		Frustum operator*(Transform other) const { return *this * Mat4(other); }

		// methods
		struct Corners
		{
			Vec3 data[8];
		};
		Corners corners() const;
	};

	namespace detail
	{
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Line other)
		{
			return str + "(" + other.origin + ", " + other.direction + ", " + other.minimum + ", " + other.maximum + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Triangle other)
		{
			return str + "(" + other.vertices[0] + ", " + other.vertices[1] + ", " + other.vertices[2] + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Plane other)
		{
			return str + "(" + other.normal + ", " + other.d + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Sphere other)
		{
			return str + "(" + other.center + ", " + other.radius + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Aabb other)
		{
			return str + "(" + other.a + "," + other.b + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, Cone other)
		{
			return str + "(" + other.origin + "," + other.direction + "," + other.length + "," + other.halfAngle + ")";
		}
		template<uint32 N>
		CAGE_FORCE_INLINE StringizerBase<N> &operator+(StringizerBase<N> &str, const Frustum &other)
		{
			return str + other.viewProj;
		}
	}

	CAGE_CORE_API Line makeSegment(Vec3 a, Vec3 b);
	CAGE_CORE_API Line makeRay(Vec3 a, Vec3 b);
	CAGE_CORE_API Line makeLine(Vec3 a, Vec3 b);

	inline Sphere::Sphere(Aabb other) : center(other.center()), radius(other.diagonal() * 0.5) {}

	CAGE_CORE_API Sphere makeSphere(PointerRange<const Vec3> points); // minimum bounding sphere

	CAGE_CORE_API Rads angle(Line a, Line b);
	CAGE_CORE_API Rads angle(Line a, Triangle b);
	CAGE_CORE_API Rads angle(Line a, Plane b);
	CAGE_CORE_API Rads angle(Triangle a, Triangle b);
	CAGE_CORE_API Rads angle(Triangle a, Plane b);
	CAGE_CORE_API Rads angle(Plane a, Plane b);

	CAGE_CORE_API Real distance(Vec3 a, Line b);
	CAGE_CORE_API Real distance(Vec3 a, Triangle b);
	CAGE_CORE_API Real distance(Vec3 a, Plane b);
	CAGE_CORE_API Real distance(Vec3 a, Sphere b);
	CAGE_CORE_API Real distance(Vec3 a, Aabb b);
	CAGE_CORE_API Real distance(Vec3 a, Cone b);
	CAGE_CORE_API Real distance(Vec3 a, const Frustum &b);
	CAGE_CORE_API Real distance(Line a, Line b);
	CAGE_CORE_API Real distance(Line a, Triangle b);
	CAGE_CORE_API Real distance(Line a, Plane b);
	CAGE_CORE_API Real distance(Line a, Sphere b);
	CAGE_CORE_API Real distance(Line a, Aabb b);
	CAGE_CORE_API Real distance(Line a, Cone b);
	CAGE_CORE_API Real distance(Line a, const Frustum &b);
	CAGE_CORE_API Real distance(Triangle a, Triangle b);
	CAGE_CORE_API Real distance(Triangle a, Plane b);
	CAGE_CORE_API Real distance(Triangle a, Sphere b);
	CAGE_CORE_API Real distance(Triangle a, Aabb b);
	CAGE_CORE_API Real distance(Triangle a, Cone b);
	CAGE_CORE_API Real distance(Triangle a, const Frustum &b);
	CAGE_CORE_API Real distance(Plane a, Plane b);
	CAGE_CORE_API Real distance(Plane a, Sphere b);
	CAGE_CORE_API Real distance(Plane a, Aabb b);
	CAGE_CORE_API Real distance(Plane a, Cone b);
	CAGE_CORE_API Real distance(Plane a, const Frustum &b);
	CAGE_CORE_API Real distance(Sphere a, Sphere b);
	CAGE_CORE_API Real distance(Sphere a, Aabb b);
	CAGE_CORE_API Real distance(Sphere a, Cone b);
	CAGE_CORE_API Real distance(Sphere a, const Frustum &b);
	CAGE_CORE_API Real distance(Aabb a, Aabb b);
	CAGE_CORE_API Real distance(Aabb a, Cone b);
	CAGE_CORE_API Real distance(Aabb a, const Frustum &b);
	CAGE_CORE_API Real distance(Cone a, Cone b);
	CAGE_CORE_API Real distance(Cone a, const Frustum &b);
	CAGE_CORE_API Real distance(const Frustum &a, const Frustum &b);

	CAGE_CORE_API bool intersects(Vec3 a, Vec3 b);
	CAGE_CORE_API bool intersects(Vec3 a, Line b);
	CAGE_CORE_API bool intersects(Vec3 a, Triangle b);
	CAGE_CORE_API bool intersects(Vec3 a, Plane b);
	CAGE_CORE_API bool intersects(Vec3 a, Sphere b);
	CAGE_CORE_API bool intersects(Vec3 a, Aabb b);
	CAGE_CORE_API bool intersects(Vec3 a, Cone b);
	CAGE_CORE_API bool intersects(Vec3 a, const Frustum &b);
	CAGE_CORE_API bool intersects(Line a, Line b);
	CAGE_CORE_API bool intersects(Line a, Triangle b);
	CAGE_CORE_API bool intersects(Line a, Plane b);
	CAGE_CORE_API bool intersects(Line a, Sphere b);
	CAGE_CORE_API bool intersects(Line a, Aabb b);
	CAGE_CORE_API bool intersects(Line a, Cone b);
	CAGE_CORE_API bool intersects(Line a, const Frustum &b);
	CAGE_CORE_API bool intersects(Triangle a, Triangle b);
	CAGE_CORE_API bool intersects(Triangle a, Plane b);
	CAGE_CORE_API bool intersects(Triangle a, Sphere b);
	CAGE_CORE_API bool intersects(Triangle a, Aabb b);
	CAGE_CORE_API bool intersects(Triangle a, Cone b);
	CAGE_CORE_API bool intersects(Triangle a, const Frustum &b);
	CAGE_CORE_API bool intersects(Plane a, Plane b);
	CAGE_CORE_API bool intersects(Plane a, Sphere b);
	CAGE_CORE_API bool intersects(Plane a, Aabb b);
	CAGE_CORE_API bool intersects(Plane a, Cone b);
	CAGE_CORE_API bool intersects(Plane a, const Frustum &b);
	CAGE_CORE_API bool intersects(Sphere a, Sphere b);
	CAGE_CORE_API bool intersects(Sphere a, Aabb b);
	CAGE_CORE_API bool intersects(Sphere a, Cone b);
	CAGE_CORE_API bool intersects(Sphere a, const Frustum &b);
	CAGE_CORE_API bool intersects(Aabb a, Aabb b);
	CAGE_CORE_API bool intersects(Aabb a, Cone b);
	CAGE_CORE_API bool intersects(Aabb a, const Frustum &b);
	CAGE_CORE_API bool intersects(Cone a, Cone b);
	CAGE_CORE_API bool intersects(Cone a, const Frustum &b);

	CAGE_CORE_API Vec3 intersection(Line a, Triangle b);
	CAGE_CORE_API Vec3 intersection(Line a, Plane b);
	CAGE_CORE_API Line intersection(Line a, Sphere b);
	CAGE_CORE_API Line intersection(Line a, Aabb b);
	CAGE_CORE_API Line intersection(Line a, Cone b);
	CAGE_CORE_API Line intersection(Line a, const Frustum &b);
	CAGE_CORE_API Aabb intersection(Aabb a, Aabb b);

	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Line b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Triangle b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Plane b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Sphere b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Aabb b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, Cone b);
	CAGE_CORE_API Vec3 closestPoint(Vec3 a, const Frustum &b);

	namespace privat
	{
		template<class T>
		struct GeometryOrder
		{
			static constexpr int order = 0;
		};
		template<>
		struct GeometryOrder<Vec3>
		{
			static constexpr int order = 1;
		};
		template<>
		struct GeometryOrder<Line>
		{
			static constexpr int order = 2;
		};
		template<>
		struct GeometryOrder<Triangle>
		{
			static constexpr int order = 3;
		};
		template<>
		struct GeometryOrder<Plane>
		{
			static constexpr int order = 4;
		};
		template<>
		struct GeometryOrder<Sphere>
		{
			static constexpr int order = 5;
		};
		template<>
		struct GeometryOrder<Aabb>
		{
			static constexpr int order = 6;
		};
		template<>
		struct GeometryOrder<Cone>
		{
			static constexpr int order = 7;
		};
		template<>
		struct GeometryOrder<Frustum>
		{
			static constexpr int order = 8;
		};
		template<class A, class B>
		static constexpr bool geometrySwapParameters = (GeometryOrder<A>::order > GeometryOrder<B>::order) && (GeometryOrder<B>::order > 0);
	}

	template<class A, class B>
	requires(privat::geometrySwapParameters<A, B>)
	CAGE_FORCE_INLINE auto angle(const A &a, const B &b)
	{
		return angle(b, a);
	}
	template<class A, class B>
	requires(privat::geometrySwapParameters<A, B>)
	CAGE_FORCE_INLINE auto distance(const A &a, const B &b)
	{
		return distance(b, a);
	}
	template<class A, class B>
	requires(privat::geometrySwapParameters<A, B>)
	CAGE_FORCE_INLINE auto intersects(const A &a, const B &b)
	{
		return intersects(b, a);
	}
	template<class A, class B>
	requires(privat::geometrySwapParameters<A, B>)
	CAGE_FORCE_INLINE auto intersection(const A &a, const B &b)
	{
		return intersection(b, a);
	}
	template<class A, class B>
	requires(privat::geometrySwapParameters<A, B>)
	CAGE_FORCE_INLINE auto closestPoint(const A &a, const B &b)
	{
		return closestPoint(b, a);
	}
}

#endif // guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
