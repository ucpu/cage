#include <cage-core/geometry.h>

#include <utility>

namespace cage
{
	line line::operator * (const mat4 &other) const
	{
		vec4 tmp = vec4(origin, 1) * other;
		vec3 o = vec3(tmp) / tmp[3];
		tmp = vec4(direction, 0) * other;
		vec3 d = vec3(tmp);
		return line(o, d, minimum, maximum).normalize();
	}

	bool line::normalized() const
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
		return abs(lengthSquared(direction) - 1) < 1e-6;
	}

	line line::normalize() const
	{
		if (!valid())
			return line();
		line r = *this;
		if (r.isPoint() || lengthSquared(r.direction) < 1e-7)
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
				r = line(r.b(), -r.direction, 0, real::Infinity());
			if (r.isLine())
			{
				//real d = distance(vec3(), r);
				// todo
			}
		}
		CAGE_ASSERT(r.normalized());
		return r;
	}

	line makeSegment(const vec3 &a, const vec3 &b)
	{
		return line(a, b - a, 0, 1).normalize();
	}

	line makeRay(const vec3 &a, const vec3 &b)
	{
		return line(a, b - a, 0, real::Infinity()).normalize();
	}

	line makeLine(const vec3 &a, const vec3 &b)
	{
		return line(a, b - a, -real::Infinity(), real::Infinity()).normalize();
	}

	triangle triangle::operator * (const mat4 &other) const
	{
		triangle r(*this);
		for (uint32 i = 0; i < 3; i++)
		{
			vec4 t = other * vec4(r[i], 1);
			r[i] = vec3(t) / t[3];
		}
		return r;
	}

	triangle triangle::flip() const
	{
		triangle r(*this);
		std::swap(r[1], r[2]);
		return r;
	}

	plane::plane(const vec3 &point, const vec3 &normal) : normal(normal), d(-dot(point, normal))
	{}

	plane::plane(const vec3 &a, const vec3 &b, const vec3 &c) : plane(triangle(a, b, c))
	{}

	plane::plane(const triangle &other) : plane(other[0], other.normal())
	{}

	plane::plane(const line &a, const vec3 &b)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "geometry");
	}

	plane plane::operator * (const mat4 &other) const
	{
		vec3 p3 = normal * d;
		vec4 p4 = vec4(p3, 1) * other;
		p3 = vec3(p4) / p4[3];
		return plane(p3, normal * mat3(other));
	}

	plane plane::normalize() const
	{
		real l = length(normal);
		return plane(normal / l, d * l); // d times or divided by l ?
	}

	sphere::sphere(const triangle &other)
	{
		vec3 a = other[0];
		vec3 b = other[1];
		vec3 c = other[2];
		real dotABAB = dot(b - a, b - a);
		real dotABAC = dot(b - a, c - a);
		real dotACAC = dot(c - a, c - a);
		real d = 2.0f*(dotABAB*dotACAC - dotABAC*dotABAC);
		vec3 referencePt = a;
		if (abs(d) <= 1e-7)
		{
			aabb bbox = aabb(other);
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

	sphere::sphere(const line &other)
	{
		if (other.isSegment())
		{
			vec3 a = other.a();
			vec3 b = other.b();
			*this = sphere((a + b) * 0.5, distance(a, b) * 0.5);
		}
		else
			*this = sphere(vec3(), real::Infinity());
	}

	sphere sphere::operator * (const mat4 &other) const
	{
		real sx2 = lengthSquared(vec3(other[0], other[1], other[2]));
		real sy2 = lengthSquared(vec3(other[4], other[5], other[6]));
		real sz2 = lengthSquared(vec3(other[8], other[9], other[10]));
		real s = sqrt(max(max(sx2, sy2), sz2));
		vec4 p4 = vec4(center, 1) * other;
		return sphere(vec3(p4) / p4[3], radius * s);
	}

	aabb::aabb(const line &other)
	{
		if (other.isSegment())
		{
			vec3 a = other.a();
			vec3 b = other.b();
			*this = aabb(a, b);
		}
		else
			*this = aabb::Universe();
	}

	aabb::aabb(const plane &other)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "plane to aabb");
	}

	aabb aabb::operator + (const aabb &other) const
	{
		if (other.empty())
			return *this;
		if (empty())
			return other;
		return aabb(min(a, other.a), max(b, other.b));
	}

	aabb aabb::operator * (const mat4 &other) const
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
		aabb res;
		for (uint32 i = 0; i < 8; i++)
		{
			vec4 r = other * vec4(tmp[i], 1);
			res += aabb(vec3(r) * (1.0 / r.data[3]));
		}
		return res;
	}

	real aabb::volume() const
	{
		return empty() ? 0 : (b.data[0] - a.data[0]) * (b.data[1] - a.data[1]) * (b.data[2] - a.data[2]);
	}

	real aabb::surface() const
	{
		real wx = b[0] - a[0];
		real wy = b[1] - a[1];
		real wz = b[2] - a[2];
		return (wx*wy + wx*wz + wy*wz) * 2;
	}

	aabb aabb::Universe() { return aabb(vec3(-real::Infinity()), vec3(real::Infinity())); }
}
