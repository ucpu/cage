#ifndef guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
#define guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_

namespace cage
{
	struct CAGE_API line
	{
		// data
		vec3 origin;
		vec3 direction;
		real minimum;
		real maximum;

		// constructors
		line() : origin(vec3::Nan()), direction(vec3::Nan()), minimum(real::Nan()), maximum(real::Nan()) {};
		line(vec3 origin, vec3 direction, real minimum, real maximum) : origin(origin), direction(direction), minimum(minimum), maximum(maximum) {};

		// compound operators
		line &operator *= (const mat4 &other) { return *this = *this * other; }
		line &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		line operator * (const mat4 &other) const;
		line operator * (const transform &other) const { return *this * mat4(other); };

		// comparison operators
		bool operator == (const line &other) const { return origin == other.origin && direction == other.direction && minimum == other.minimum && maximum == other.maximum; }
		bool operator != (const line &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + origin + ", " + direction + ", " + minimum + ", " + maximum + ")"; }

		// methods
		bool valid() const { return origin.valid() && direction.valid() && minimum.valid() && maximum.valid(); };
		bool isPoint() const { return valid() && minimum == maximum; }
		bool isLine() const { return valid() && !minimum.finite() && !maximum.finite(); }
		bool isRay() const { return valid() && (minimum.finite() != maximum.finite()); }
		bool isSegment() const { return valid() && minimum.finite() && maximum.finite(); }
		bool normalized() const;
		line normalize() const;
		vec3 a() const { return origin + direction * minimum; }
		vec3 b() const { return origin + direction * maximum; }
	};

	struct CAGE_API triangle
	{
		// data
		vec3 vertices[3];

		// constructors
		triangle() : vertices{ vec3::Nan(), vec3::Nan(), vec3::Nan() } {};
		triangle(const vec3 vertices[3]) : vertices{ vertices[0], vertices[1], vertices[2] } {};
		triangle(const vec3 &a, const vec3 &b, const vec3 &c) : vertices{ a, b, c } {};
		triangle(const real coords[9]) : vertices{ vec3(coords[0], coords[1], coords[2]), vec3(coords[3], coords[4], coords[5]), vec3(coords[6], coords[7], coords[8]) } {};

		// compound operators
		triangle &operator *= (const mat4 &other) { return *this = *this * other; }
		triangle &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		triangle operator * (const mat4 &other) const;
		triangle operator * (const transform &other) const { return *this * mat4(other); };

		vec3 operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 3, "index out of range", idx); return vertices[idx]; }
		vec3 &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 3, "index out of range", idx); return vertices[idx]; }

		// comparison operators
		bool operator == (const triangle &other) const { for (uint8 i = 0; i < 3; i++) if (vertices[i] != other.vertices[i]) return false; return true; }
		bool operator != (const triangle &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + vertices[0] + ", " + vertices[1] + ", " + vertices[2] + ")"; }

		// methods
		bool valid() const { return vertices[0].valid() && vertices[1].valid() && vertices[2].valid(); };
		bool degenerated() const { return vertices[0] == vertices[1] || vertices[1] == vertices[2] || vertices[2] == vertices[0]; };
		vec3 normal() const { return normalize(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))); }
		vec3 center() const { return (vertices[0] + vertices[1] + vertices[2]) / 3; }
		real area() const { return length(cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0]))) * 0.5; }
		triangle flip() const;
	};

	struct CAGE_API plane
	{
		// data
		vec3 normal;
		real d;

		// constructors
		plane() : normal(vec3::Nan()), d(real::Nan()) {};
		plane(const vec3 &normal, real d) : normal(normal), d(d) {};
		plane(const vec3 &point, const vec3 &normal);
		plane(const vec3 &a, const vec3 &b, const vec3 &c);
		explicit plane(const triangle &other);
		explicit plane(const line &a, const vec3 &b);

		// compound operators
		plane &operator *= (const mat4 &other) { return *this = *this * other; }
		plane &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		plane operator * (const mat4 &other) const;
		plane operator * (const transform &other) const { return *this * mat4(other); };

		// comparison operators
		bool operator == (const plane &other) const { return d == other.d && normal == other.normal; }
		bool operator != (const plane &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + normal + ", " + d + ")"; }

		// methods
		bool valid() const { return d.valid() && normal.valid(); };
		plane normalize() const;
	};

	struct CAGE_API sphere
	{
		// data
		vec3 center;
		real radius;

		// constructors
		sphere() : center(vec3::Nan()), radius(real::Nan()) {};
		sphere(const vec3 &center, real radius) : center(center), radius(radius) {};
		explicit sphere(const triangle &other);
		explicit sphere(const aabb &other);
		explicit sphere(const line &other);

		// compound operators
		sphere &operator *= (const mat4 &other) { return *this = *this * other; }
		sphere &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		sphere operator * (const mat4 &other) const;
		sphere operator * (const transform &other) const { return *this * mat4(other); };

		// comparison operators
		bool operator == (const sphere &other) const { return (empty() == other.empty()) || (center == other.center && radius == other.radius); }
		bool operator != (const sphere &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + center + ", " + radius + ")"; }

		// methods
		bool valid() const { return center.valid() && radius >= 0; }
		bool empty() const { return !valid(); }
		real volume() const { return empty() ? 0 : 4 * real::Pi() * radius * radius * radius / 3; }
		real surface() const { return empty() ? 0 : 4 * real::Pi() * radius * radius; }
	};

	struct CAGE_API aabb
	{
		// data
		vec3 a, b;

		// constructor
		aabb() : a(vec3::Nan()), b(vec3::Nan()) {}
		aabb(const vec3 &a, const vec3 &b) : a(min(a, b)), b(max(a, b)) {}
		explicit aabb(const vec3 &point) : a(point), b(point) {}
		explicit aabb(const triangle &other) : a(min(min(other[0], other[1]), other[2])), b(max(max(other[0], other[1]), other[2])) {};
		explicit aabb(const sphere &other) : a(other.center - other.radius), b(other.center + other.radius) {};
		explicit aabb(const line &other);
		explicit aabb(const plane &other);

		// compound operators
		aabb &operator += (const aabb &other) { return *this = *this + other; }
		aabb &operator *= (const mat4 &other) { return *this = *this * other; }
		aabb &operator *= (const transform &other) { return *this = *this * other; }

		// binary operators
		aabb operator + (const aabb &other) const;
		aabb operator * (const mat4 &other) const;
		aabb operator * (const transform &other) const { return *this * mat4(other); };

		// comparison operators
		bool operator == (const aabb &other) const { return (empty() == other.empty()) && (empty() || (a == other.a && b == other.b)); }
		bool operator != (const aabb &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + a + "," + b + ")"; }

		// methods
		bool valid() const { return a.valid() && b.valid(); };
		bool empty() const { return !valid(); }
		real volume() const;
		real surface() const;
		vec3 center() const { return empty() ? vec3() : (a + b) * 0.5; }
		vec3 size() const { return empty() ? vec3() : b - a; }
		real diagonal() const { return empty() ? 0 : distance(a, b); }

		// constants
		static aabb Universe();
	};

	CAGE_API line makeSegment(const vec3 &a, const vec3 &b);
	CAGE_API line makeRay(const vec3 &a, const vec3 &b);
	CAGE_API line makeLine(const vec3 &a, const vec3 &b);

	inline sphere::sphere(const aabb &other) : center(other.center()), radius(other.diagonal() * 0.5) {};

	CAGE_API bool parallel(const vec3 &dir1, const vec3 &dir2);
	CAGE_API bool parallel(const line &a, const line &b);
	CAGE_API bool parallel(const line &a, const triangle &b);
	CAGE_API bool parallel(const line &a, const plane &b);
	inline   bool parallel(const triangle &a, const line &b) { return parallel(b, a); }
	inline   bool parallel(const plane &a, const line &b) { return parallel(b, a); }
	CAGE_API bool parallel(const triangle &a, const triangle &b);
	CAGE_API bool parallel(const triangle &a, const plane &b);
	inline   bool parallel(const plane &a, const triangle &b) { return parallel(b, a); }
	CAGE_API bool parallel(const plane &a, const plane &b);

	CAGE_API bool perpendicular(const vec3 &dir1, const vec3 &dir2);
	CAGE_API bool perpendicular(const line &a, const line &b);
	CAGE_API bool perpendicular(const line &a, const triangle &b);
	CAGE_API bool perpendicular(const line &a, const plane &b);
	inline   bool perpendicular(const triangle &a, const line &b) { return perpendicular(b, a); }
	inline   bool perpendicular(const plane &a, const line &b) { return perpendicular(b, a); }
	CAGE_API bool perpendicular(const triangle &a, const triangle &b);
	CAGE_API bool perpendicular(const triangle &a, const plane &b);
	inline   bool perpendicular(const plane &a, const triangle &b) { return perpendicular(b, a); }
	CAGE_API bool perpendicular(const plane &a, const plane &b);

	CAGE_API rads angle(const line &a, const line &b);
	CAGE_API rads angle(const line &a, const triangle &b);
	CAGE_API rads angle(const line &a, const plane &b);
	inline   rads angle(const triangle &a, const line &b) { return angle(b, a); };
	CAGE_API rads angle(const triangle &a, const triangle &b);
	CAGE_API rads angle(const triangle &a, const plane &b);
	inline   rads angle(const plane &a, const line &b) { return angle(b, a); };
	inline   rads angle(const plane &a, const triangle &b) { return angle(b, a); };
	CAGE_API rads angle(const plane &a, const plane &b);

	//CAGE_API real distance(const vec3 &a, const vec3 &b);
	CAGE_API real distance(const vec3 &a, const line &b);
	CAGE_API real distance(const vec3 &a, const triangle &b);
	CAGE_API real distance(const vec3 &a, const plane &b);
	CAGE_API real distance(const vec3 &a, const sphere &b);
	CAGE_API real distance(const vec3 &a, const aabb &b);
	inline   real distance(const line &a, const vec3 &b) { return distance(b, a); };
	CAGE_API real distance(const line &a, const line &b);
	CAGE_API real distance(const line &a, const triangle &b);
	CAGE_API real distance(const line &a, const plane &b);
	CAGE_API real distance(const line &a, const sphere &b);
	CAGE_API real distance(const line &a, const aabb &b);
	inline   real distance(const triangle &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const triangle &a, const line &b) { return distance(b, a); };
	CAGE_API real distance(const triangle &a, const triangle &b);
	CAGE_API real distance(const triangle &a, const plane &b);
	CAGE_API real distance(const triangle &a, const sphere &b);
	CAGE_API real distance(const triangle &a, const aabb &b);
	inline   real distance(const plane &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const plane &a, const line &b) { return distance(b, a); };
	inline   real distance(const plane &a, const triangle &b) { return distance(b, a); };
	CAGE_API real distance(const plane &a, const plane &b);
	CAGE_API real distance(const plane &a, const sphere &b);
	CAGE_API real distance(const plane &a, const aabb &b);
	inline   real distance(const sphere &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const line &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const triangle &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const plane &b) { return distance(b, a); };
	CAGE_API real distance(const sphere &a, const sphere &b);
	CAGE_API real distance(const sphere &a, const aabb &b);
	inline   real distance(const aabb &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const line &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const triangle &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const plane &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const sphere &b) { return distance(b, a); };
	CAGE_API real distance(const aabb &a, const aabb &b);

	CAGE_API bool intersects(const vec3 &a, const vec3 &b);
	CAGE_API bool intersects(const vec3 &a, const line &b);
	CAGE_API bool intersects(const vec3 &a, const triangle &b);
	CAGE_API bool intersects(const vec3 &a, const plane &b);
	CAGE_API bool intersects(const vec3 &a, const sphere &b);
	CAGE_API bool intersects(const vec3 &a, const aabb &b);
	inline   bool intersects(const line &a, const vec3 &b) { return intersects(b, a); };
	CAGE_API bool intersects(const line &a, const line &b);
	CAGE_API bool intersects(const line &a, const triangle &b);
	CAGE_API bool intersects(const line &a, const plane &b);
	CAGE_API bool intersects(const line &a, const sphere &b);
	CAGE_API bool intersects(const line &a, const aabb &b);
	inline   bool intersects(const triangle &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const triangle &a, const line &b) { return intersects(b, a); };
	CAGE_API bool intersects(const triangle &a, const triangle &b);
	CAGE_API bool intersects(const triangle &a, const plane &b);
	CAGE_API bool intersects(const triangle &a, const sphere &b);
	CAGE_API bool intersects(const triangle &a, const aabb &b);
	inline   bool intersects(const plane &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const plane &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const plane &a, const triangle &b) { return intersects(b, a); };
	CAGE_API bool intersects(const plane &a, const plane &b);
	CAGE_API bool intersects(const plane &a, const sphere &b);
	CAGE_API bool intersects(const plane &a, const aabb &b);
	inline   bool intersects(const sphere &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const triangle &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const plane &b) { return intersects(b, a); };
	CAGE_API bool intersects(const sphere &a, const sphere &b);
	CAGE_API bool intersects(const sphere &a, const aabb &b);
	inline   bool intersects(const aabb &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const triangle &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const plane &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const sphere &b) { return intersects(b, a); };
	CAGE_API bool intersects(const aabb &a, const aabb &b);

	CAGE_API vec3 intersection(const line &a, const triangle &b);
	CAGE_API vec3 intersection(const line &a, const plane &b);
	CAGE_API line intersection(const line &a, const sphere &b);
	CAGE_API line intersection(const line &a, const aabb &b);
	CAGE_API aabb intersection(const aabb &a, const aabb &b);
	inline   vec3 intersection(const triangle &a, const line &b) { return intersection(b, a); }
	inline   vec3 intersection(const plane &a, const line &b) { return intersection(b, a); }
	inline   line intersection(const sphere &a, const line &b) { return intersection(b, a); };
	inline   line intersection(const aabb &a, const line &b) { return intersection(b, a); };

	CAGE_API bool frustumCulling(const vec3 &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const line &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const triangle &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const plane &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const sphere &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const aabb &shape, const mat4 &mvp);

	CAGE_API vec3 closestPoint(const triangle &trig, const vec3 &point);
	CAGE_API vec3 closestPoint(const plane &pl, const vec3 &point);
}

#endif // guard_geometry_h_waesfes54hg96r85t4h6rt4h564rzth_
