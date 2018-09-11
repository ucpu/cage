#include <limits>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>

namespace cage
{
	float real::epsilon = std::numeric_limits<float>::epsilon() * 10.0f;
	const real real::Pi = 3.14159265358979323846264338327950288;
	const real real::HalfPi = Pi / 2;
	const real real::TwoPi = Pi * 2;
	const real real::E = 2.718281828459045235360287471352;
	const real real::Ln2 = 0.69314718055994530942;
	const real real::Ln10 = 2.302585092994045684;
	const real real::PositiveInfinity = +std::numeric_limits<float>::infinity();
	const real real::NegativeInfinity = -std::numeric_limits<float>::infinity();
	const real real::Nan = std::numeric_limits<float>::quiet_NaN();

	const rads rads::Zero = rads(0);
	const rads rads::Stright = rads(3.14159265358979323846264338327950288);
	const rads rads::Right = rads::Stright / 2;
	const rads rads::Full = rads::Stright * 2;
	const rads rads::Nan = rads(real::Nan);

	const vec2 vec2::Nan = vec2(real::Nan, real::Nan);
	const vec3 vec3::Nan = vec3(real::Nan, real::Nan, real::Nan);
	const vec4 vec4::Nan = vec4(real::Nan, real::Nan, real::Nan, real::Nan);
	const quat quat::Nan = quat(real::Nan, real::Nan, real::Nan, real::Nan);

	const mat3 mat3::Nan = mat3(real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan);
	const mat3 mat3::Zero = mat3(0, 0, 0, 0, 0, 0, 0, 0, 0);
	const mat4 mat4::Nan = mat4(real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan, real::Nan);
	const mat4 mat4::Zero = mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	const aabb aabb::Universe = aabb(vec3(real::NegativeInfinity, real::NegativeInfinity, real::NegativeInfinity), vec3(real::PositiveInfinity, real::PositiveInfinity, real::PositiveInfinity));
}
