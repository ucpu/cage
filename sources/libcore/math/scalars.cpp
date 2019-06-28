#include <cmath>
#include <limits>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include "math.h"

namespace cage
{
	real real::parse(const string &str)
	{
		return detail::tryRemoveParentheses(str).toFloat();
	}

	real sin(rads value)
	{
		return ::sin(real(value).value);
	}

	real cos(rads value)
	{
		return ::cos(real(value).value);
	}

	real tan(rads value)
	{
		return ::tan(real(value).value);
	}

	rads aSin(real value)
	{
		return (rads) ::asin(value.value);
	}

	rads aCos(real value)
	{
		return (rads) ::acos(value.value);
	}

	rads aTan(real value)
	{
		return (rads) ::atan(value.value);
	}

	rads aTan2(real x, real y)
	{
		if (x > 0) return aTan(y / x);
		if (x < 0)
		{
			if (y < 0) return aTan(y / x) - rads(real::Pi());
			return aTan(y / x) + rads(real::Pi());
		}
		if (y < 0) return rads(-real::Pi() / 2);
		if (y > 0) return rads(real::Pi() / 2);
		return rads::Nan();
	}

	bool real::valid() const
	{
		return !std::isnan(value);
	}

	real real::sqrt() const
	{
		return ::sqrt(value);
	}

	real real::pow(real power) const
	{
		return ::pow(value, power.value);
	}

	real real::ln() const
	{
		if (*this <= (real)0)
			CAGE_THROW_ERROR(exception, "invalid value");
		return ::log(value);
	}

	real real::round() const
	{
		return ::round(value);
	}

	real real::floor() const
	{
		return ::floor(value);
	}

	real real::ceil() const
	{
		return ::ceil(value);
	}

	real real::Pi() { return 3.14159265358979323846264338327950288; }
	real real::E() { return 2.718281828459045235360287471352; }
	real real::Ln2() { return 0.69314718055994530942; }
	real real::Ln10() { return 2.302585092994045684; }
	real real::Infinity() { return std::numeric_limits<float>::infinity(); }
	real real::Nan() { return std::numeric_limits<float>::quiet_NaN(); }

	rads rads::Stright() { return rads(real::Pi()); }
	rads rads::Right() { return rads::Stright() / 2; }
	rads rads::Full() { return rads::Stright() * 2; }
	rads rads::Nan() { return rads(real::Nan()); }
}
