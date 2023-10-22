#include <cage-core/geometry.h>

namespace cage
{
	Line Line::operator*(Mat4 other) const
	{
		CAGE_ASSERT(normalized());

		const auto &tr = [&](Vec3 p)
		{
			Vec4 t = other * Vec4(p, 1);
			return Vec3(t) / t[3];
		};

		if (isPoint())
			return Line(tr(a()), Vec3(1, 0, 0), 0, 0);
		if (isLine())
			return makeLine(tr(origin), tr(origin + direction));
		if (isRay())
			return makeRay(tr(origin), tr(origin + direction));
		if (isSegment())
			return makeSegment(tr(a()), tr(b()));
		CAGE_THROW_CRITICAL(Exception, "geometry");
	}

	bool Line::normalized() const
	{
		if (isPoint())
		{
			if (minimum != 0)
				return false;
			if (direction != Vec3(1, 0, 0))
				return false;
		}
		else if (isLine())
		{
			if (minimum != -Real::Infinity() || maximum != Real::Infinity())
				return false;
		}
		else if (isRay())
		{
			if (minimum != 0 || maximum != Real::Infinity())
				return false;
		}
		else if (isSegment())
		{
			if (minimum != 0 || maximum <= 0)
				return false;
		}
		else
			return false;
		return abs(lengthSquared(direction) - 1) < 1e-5;
	}

	Line Line::normalize() const
	{
		Line r = *this;
		if (r.isPoint() || lengthSquared(r.direction) < 1e-5)
		{
			r.origin = r.a();
			r.direction = Vec3(1, 0, 0);
			r.minimum = r.maximum = 0;
		}
		else
		{
			if (r.minimum > r.maximum)
				std::swap(r.minimum, r.maximum);
			r.direction = cage::normalize(r.direction);
			Real l = length(direction);
			r.minimum *= l;
			r.maximum *= l;
			if (r.minimum.finite())
			{
				if (r.minimum != 0)
				{
					r.origin = r.a();
					r.maximum -= r.minimum;
					r.minimum = 0;
				}
			}
			else if (r.maximum.finite())
				r = Line(r.b(), -r.direction, 0, Real::Infinity());
			if (r.isLine())
			{
				//real d = distance(vec3(), r);
				// todo
			}
		}
		CAGE_ASSERT(r.normalized());
		return r;
	}

	Line makeSegment(Vec3 a, Vec3 b)
	{
		return Line(a, b - a, 0, 1).normalize();
	}

	Line makeRay(Vec3 a, Vec3 b)
	{
		return Line(a, b - a, 0, Real::Infinity()).normalize();
	}

	Line makeLine(Vec3 a, Vec3 b)
	{
		return Line(a, b - a, -Real::Infinity(), Real::Infinity()).normalize();
	}

	Triangle Triangle::operator*(Mat4 other) const
	{
		Triangle r = *this;
		for (uint32 i = 0; i < 3; i++)
		{
			Vec4 t = other * Vec4(r[i], 1);
			r[i] = Vec3(t) / t[3];
		}
		return r;
	}

	bool Triangle::degenerated() const
	{
		return area() < 1e-5;
	}

	Triangle Triangle::flip() const
	{
		Triangle r = *this;
		std::swap(r[1], r[2]);
		return r;
	}

	Plane::Plane(Vec3 point, Vec3 normal) : normal(normal), d(-dot(point, normal)) {}

	Plane::Plane(Vec3 a, Vec3 b, Vec3 c) : Plane(Triangle(a, b, c)) {}

	Plane::Plane(Triangle other) : Plane(other[0], other.normal()) {}

	Plane::Plane(Line a, Vec3 b) : Plane(a.origin, a.origin + a.direction, b) {}

	Plane Plane::operator*(Mat4 other) const
	{
		Vec3 p3 = normal * d;
		Vec4 p4 = Vec4(p3, 1) * other;
		p3 = Vec3(p4) / p4[3];
		return Plane(p3, normal * Mat3(other));
	}

	bool Plane::normalized() const
	{
		return abs(lengthSquared(normal) - 1) < 1e-5;
	}

	Plane Plane::normalize() const
	{
		const Real l = length(normal);
		return Plane(normal / l, d * l); // d times or divided by l ?
	}

	Sphere::Sphere(Line other)
	{
		if (!other.valid())
			*this = Sphere();
		else if (other.isPoint())
			*this = Sphere(other.a(), 0);
		else if (other.isSegment())
		{
			const Vec3 a = other.a();
			const Vec3 b = other.b();
			*this = Sphere((a + b) * 0.5, distance(a, b) * 0.5);
		}
		else
			*this = Sphere(Vec3(), Real::Infinity());
	}

	Sphere::Sphere(Triangle other)
	{
		Vec3 a = other[0];
		Vec3 b = other[1];
		Vec3 c = other[2];
		Real dotABAB = dot(b - a, b - a);
		Real dotABAC = dot(b - a, c - a);
		Real dotACAC = dot(c - a, c - a);
		Real d = 2.0f * (dotABAB * dotACAC - dotABAC * dotABAC);
		Vec3 referencePt = a;
		if (abs(d) <= 1e-5)
		{
			Aabb bbox = Aabb(other);
			center = bbox.center();
			referencePt = bbox.a;
		}
		else
		{
			Real s = (dotABAB * dotACAC - dotACAC * dotABAC) / d;
			Real t = (dotACAC * dotABAB - dotABAB * dotABAC) / d;
			if (s <= 0.0f)
				center = 0.5f * (a + c);
			else if (t <= 0.0f)
				center = 0.5f * (a + b);
			else if (s + t >= 1.0f)
			{
				center = 0.5f * (b + c);
				referencePt = b;
			}
			else
				center = a + s * (b - a) + t * (c - a);
		}
		radius = distance(center, referencePt);
	}

	Sphere::Sphere(Cone other)
	{
		// https://bartwronski.com/2017/04/13/cull-that-cone/ modified
		if (other.halfAngle > Rads(Real::Pi() / 4))
		{
			*this = Sphere(other.origin + cos(other.halfAngle) * other.length * other.direction, sin(other.halfAngle) * other.length);
		}
		else
		{
			const Real ca2 = 1 / (2 * cos(other.halfAngle));
			*this = Sphere(other.origin + other.length * ca2 * other.direction, other.length * ca2);
		}
	}

	Sphere Sphere::operator*(Mat4 other) const
	{
		Real sx2 = lengthSquared(Vec3(other[0], other[1], other[2]));
		Real sy2 = lengthSquared(Vec3(other[4], other[5], other[6]));
		Real sz2 = lengthSquared(Vec3(other[8], other[9], other[10]));
		Real s = sqrt(max(max(sx2, sy2), sz2));
		Vec4 p4 = Vec4(center, 1) * other;
		return Sphere(Vec3(p4) / p4[3], radius * s);
	}

	Aabb::Aabb(Line other)
	{
		if (!other.valid())
			*this = Aabb();
		else if (other.isSegment() || other.isPoint())
		{
			Vec3 a = other.a();
			Vec3 b = other.b();
			*this = Aabb(a, b);
		}
		else
			*this = Aabb::Universe();
	}

	Aabb::Aabb(Plane other)
	{
		CAGE_ASSERT(other.normalized());
		*this = Aabb::Universe();
		const Vec3 o = other.origin();
		for (uint32 a = 0; a < 3; a++)
		{
			if (abs(abs(other.normal[a]) - 1) < 1e-5)
				this->a[a] = this->b[a] = o[a];
		}
	}

	// todo optimize
	Aabb::Aabb(Cone other) : Aabb(Sphere(other)) {}

	Aabb::Aabb(const Frustum &other)
	{
		Aabb box;
		Frustum::Corners corners = other.corners();
		for (Vec3 v : corners.data)
			box += Aabb(v);
		*this = box;
	}

	Aabb Aabb::operator+(Aabb other) const
	{
		if (other.empty())
			return *this;
		if (empty())
			return other;
		return Aabb(min(a, other.a), max(b, other.b));
	}

	Aabb Aabb::operator*(Mat4 other) const
	{
		Vec3 tmp[8];
		tmp[0] = Vec3(b.data[0], a.data[1], a.data[2]);
		tmp[1] = Vec3(a.data[0], b.data[1], a.data[2]);
		tmp[2] = Vec3(a.data[0], a.data[1], b.data[2]);
		tmp[3] = Vec3(a.data[0], b.data[1], b.data[2]);
		tmp[4] = Vec3(b.data[0], a.data[1], b.data[2]);
		tmp[5] = Vec3(b.data[0], b.data[1], a.data[2]);
		tmp[6] = a;
		tmp[7] = b;
		Aabb res;
		for (uint32 i = 0; i < 8; i++)
		{
			Vec4 r = other * Vec4(tmp[i], 1);
			res += Aabb(Vec3(r) * (1.0 / r.data[3]));
		}
		return res;
	}

	Real Aabb::volume() const
	{
		return empty() ? 0 : (b.data[0] - a.data[0]) * (b.data[1] - a.data[1]) * (b.data[2] - a.data[2]);
	}

	Real Aabb::surface() const
	{
		Real wx = b[0] - a[0];
		Real wy = b[1] - a[1];
		Real wz = b[2] - a[2];
		return (wx * wy + wx * wz + wy * wz) * 2;
	}

	Cone Cone::operator*(Mat4 other) const
	{
		CAGE_THROW_CRITICAL(Exception, "geometry");
	}

	Frustum::Frustum(Transform camera, Mat4 proj) : Frustum(proj * Mat4(inverse(camera))) {}

	Frustum::Frustum(Mat4 viewProj) : viewProj(viewProj)
	{
		const auto &column = [&](uint32 index) { return Vec4(viewProj[index], viewProj[index + 4], viewProj[index + 8], viewProj[index + 12]); };
		const Vec4 c0 = column(0);
		const Vec4 c1 = column(1);
		const Vec4 c2 = column(2);
		const Vec4 c3 = column(3);
		planes[0] = c3 + c0;
		planes[1] = c3 - c0;
		planes[2] = c3 + c1;
		planes[3] = c3 - c1;
		planes[4] = c3 + c2;
		planes[5] = c3 - c2;
	}

	Frustum Frustum::operator*(Mat4 other) const
	{
		return Frustum(viewProj * other);
	}

	Frustum::Corners Frustum::corners() const
	{
		const Mat4 invVP = inverse(viewProj);
		static constexpr const Vec3 clipCorners[8] = {
			Vec3(-1, -1, -1),
			Vec3(-1, -1, +1),
			Vec3(-1, +1, -1),
			Vec3(-1, +1, +1),
			Vec3(+1, -1, -1),
			Vec3(+1, -1, +1),
			Vec3(+1, +1, -1),
			Vec3(+1, +1, +1),
		};
		Frustum::Corners res;
		int i = 0;
		for (Vec3 v : clipCorners)
		{
			const Vec4 p = invVP * Vec4(v, 1);
			res.data[i++] = Vec3(p) / p[3];
		}
		return res;
	}
}
