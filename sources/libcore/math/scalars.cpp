#include <cmath>

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
			if (y < 0) return aTan(y / x) - rads(real::Pi);
			return aTan(y / x) + rads(real::Pi);
		}
		if (y < 0) return rads(-real::HalfPi);
		if (y > 0) return rads(real::HalfPi);
		return rads::Nan;
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
}
