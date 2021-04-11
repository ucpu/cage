#include <cage-core/geometry.h>

#include <utility>

namespace cage
{
	Line Line::operator * (const mat4 &other) const
	{
		CAGE_ASSERT(normalized());

		const auto &tr = [&](const vec3 &p)
		{
			vec4 t = other * vec4(p, 1);
			return vec3(t) / t[3];
		};

		if (isPoint())
			return Line(tr(a()), vec3(1, 0, 0), 0, 0);
		if (isLine())
			return makeLine(tr(origin), tr(origin + direction));
		if (isRay())
			return makeRay(tr(origin), tr(origin + direction));
		if (isSegment())
			return makeSegment(tr(a()), tr(b()));
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	bool Line::normalized() const
	{
		if (isPoint())
		{
			if (minimum != 0)
				return false;
			if (direction != vec3(1, 0, 0))
				return false;
		}
		else if (isLine())
		{
			if (minimum != -real::Infinity() || maximum != real::Infinity())
				return false;
		}
		else if (isRay())
		{
			if (minimum != 0 || maximum != real::Infinity())
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
			r.direction = vec3(1, 0, 0);
			r.minimum = r.maximum = 0;
		}
		else
		{
			if (r.minimum > r.maximum)
				std::swap(r.minimum, r.maximum);
			r.direction = cage::normalize(r.direction);
			real l = length(direction);
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
				r = Line(r.b(), -r.direction, 0, real::Infinity());
			if (r.isLine())
			{
				//real d = distance(vec3(), r);
				// todo
			}
		}
		CAGE_ASSERT(r.normalized());
		return r;
	}

	Line makeSegment(const vec3 &a, const vec3 &b)
	{
		return Line(a, b - a, 0, 1).normalize();
	}

	Line makeRay(const vec3 &a, const vec3 &b)
	{
		return Line(a, b - a, 0, real::Infinity()).normalize();
	}

	Line makeLine(const vec3 &a, const vec3 &b)
	{
		return Line(a, b - a, -real::Infinity(), real::Infinity()).normalize();
	}

	Triangle Triangle::operator * (const mat4 &other) const
	{
		Triangle r = *this;
		for (uint32 i = 0; i < 3; i++)
		{
			vec4 t = other * vec4(r[i], 1);
			r[i] = vec3(t) / t[3];
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

	Plane::Plane(const vec3 &point, const vec3 &normal) : normal(normal), d(-dot(point, normal))
	{}

	Plane::Plane(const vec3 &a, const vec3 &b, const vec3 &c) : Plane(Triangle(a, b, c))
	{}

	Plane::Plane(const Triangle &other) : Plane(other[0], other.normal())
	{}

	Plane::Plane(const Line &a, const vec3 &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	Plane Plane::operator * (const mat4 &other) const
	{
		vec3 p3 = normal * d;
		vec4 p4 = vec4(p3, 1) * other;
		p3 = vec3(p4) / p4[3];
		return Plane(p3, normal * mat3(other));
	}

	bool Plane::normalized() const
	{
		return abs(lengthSquared(normal) - 1) < 1e-5;
	}

	Plane Plane::normalize() const
	{
		real l = length(normal);
		return Plane(normal / l, d * l); // d times or divided by l ?
	}

	Sphere::Sphere(const Triangle &other)
	{
		vec3 a = other[0];
		vec3 b = other[1];
		vec3 c = other[2];
		real dotABAB = dot(b - a, b - a);
		real dotABAC = dot(b - a, c - a);
		real dotACAC = dot(c - a, c - a);
		real d = 2.0f*(dotABAB*dotACAC - dotABAC*dotABAC);
		vec3 referencePt = a;
		if (abs(d) <= 1e-5)
		{
			Aabb bbox = Aabb(other);
			center = bbox.center();
			referencePt = bbox.a;
		}
		else
		{
			real s = (dotABAB*dotACAC - dotACAC*dotABAC) / d;
			real t = (dotACAC*dotABAB - dotABAB*dotABAC) / d;
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
				center = a + s*(b - a) + t*(c - a);
		}
		radius = distance(center, referencePt);
	}

	Sphere::Sphere(const Line &other)
	{
		if (!other.valid())
			*this = Sphere();
		else if (other.isPoint())
			*this = Sphere(other.a(), 0);
		else if (other.isSegment())
		{
			const vec3 a = other.a();
			const vec3 b = other.b();
			*this = Sphere((a + b) * 0.5, distance(a, b) * 0.5);
		}
		else
			*this = Sphere(vec3(), real::Infinity());
	}

	Sphere Sphere::operator * (const mat4 &other) const
	{
		real sx2 = lengthSquared(vec3(other[0], other[1], other[2]));
		real sy2 = lengthSquared(vec3(other[4], other[5], other[6]));
		real sz2 = lengthSquared(vec3(other[8], other[9], other[10]));
		real s = sqrt(max(max(sx2, sy2), sz2));
		vec4 p4 = vec4(center, 1) * other;
		return Sphere(vec3(p4) / p4[3], radius * s);
	}

	Aabb::Aabb(const Line &other)
	{
		if (!other.valid())
			*this = Aabb();
		else if (other.isSegment() || other.isPoint())
		{
			vec3 a = other.a();
			vec3 b = other.b();
			*this = Aabb(a, b);
		}
		else
			*this = Aabb::Universe();
	}

	Aabb::Aabb(const Plane &other)
	{
		CAGE_ASSERT(other.normalized());
		*this = Aabb::Universe();
		const vec3 o = other.origin();
		for (uint32 a = 0; a < 3; a++)
		{
			if (abs(abs(other.normal[a]) - 1) < 1e-5)
				this->a[a] = this->b[a] = o[a];
		}
	}

	Aabb Aabb::operator + (const Aabb &other) const
	{
		if (other.empty())
			return *this;
		if (empty())
			return other;
		return Aabb(min(a, other.a), max(b, other.b));
	}

	Aabb Aabb::operator * (const mat4 &other) const
	{
		vec3 tmp[8];
		tmp[0] = vec3(b.data[0], a.data[1], a.data[2]);
		tmp[1] = vec3(a.data[0], b.data[1], a.data[2]);
		tmp[2] = vec3(a.data[0], a.data[1], b.data[2]);
		tmp[3] = vec3(a.data[0], b.data[1], b.data[2]);
		tmp[4] = vec3(b.data[0], a.data[1], b.data[2]);
		tmp[5] = vec3(b.data[0], b.data[1], a.data[2]);
		tmp[6] = a;
		tmp[7] = b;
		Aabb res;
		for (uint32 i = 0; i < 8; i++)
		{
			vec4 r = other * vec4(tmp[i], 1);
			res += Aabb(vec3(r) * (1.0 / r.data[3]));
		}
		return res;
	}

	real Aabb::volume() const
	{
		return empty() ? 0 : (b.data[0] - a.data[0]) * (b.data[1] - a.data[1]) * (b.data[2] - a.data[2]);
	}

	real Aabb::surface() const
	{
		real wx = b[0] - a[0];
		real wy = b[1] - a[1];
		real wz = b[2] - a[2];
		return (wx*wy + wx*wz + wy*wz) * 2;
	}

	Frustum::Frustum(const transform &camera, const mat4 &proj) : Frustum(mat4(inverse(camera)), proj)
	{}

	Frustum::Frustum(const mat4 &view, const mat4 &proj)
	{
		const mat4 mvp = proj * view;
		const auto &column = [&](uint32 index)
		{
			return vec4(mvp[index], mvp[index + 4], mvp[index + 8], mvp[index + 12]);
		};
		planes[0] = column(3) + column(0);
		planes[1] = column(3) - column(0);
		planes[2] = column(3) + column(1);
		planes[3] = column(3) - column(1);
		planes[4] = column(3) + column(2);
		planes[5] = column(3) - column(2);
	}
}
