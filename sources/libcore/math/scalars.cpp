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
		return std::sin(real(value).value);
	}

	real cos(rads value)
	{
		return std::cos(real(value).value);
	}

	real tan(rads value)
	{
		return std::tan(real(value).value);
	}

	rads asin(real value)
	{
		return (rads) std::asin(value.value);
	}

	rads acos(real value)
	{
		return (rads) std::acos(value.value);
	}

	rads atan(real value)
	{
		return (rads) std::atan(value.value);
	}

	rads atan2(real x, real y)
	{
		if (x > 0) return atan(y / x);
		if (x < 0)
		{
			if (y < 0) return atan(y / x) - rads(real::Pi());
			return atan(y / x) + rads(real::Pi());
		}
		if (y < 0) return rads(-real::Pi() / 2);
		if (y > 0) return rads(real::Pi() / 2);
		return rads::Nan();
	}

	bool real::valid() const
	{
		return !std::isnan(value);
	}

	real sqrt(real x)
	{
		return std::sqrt(x.value);
	}

	real pow(real base, real exponent)
	{
		return std::pow(base.value, exponent.value);
	}

	real powE(real x)
	{
		return std::exp(x.value);
	}

	real pow2(real x)
	{
		return std::exp2(x.value);
	}

	real pow10(real x)
	{
		return std::pow(10, x.value);
	}

	real log(real x)
	{
		return std::log(x.value);
	}

	real log2(real x)
	{
		return std::log2(x.value);
	}

	real log10(real x)
	{
		return std::log10(x.value);
	}

	real round(real x)
	{
		return std::round(x.value);
	}

	real floor(real x)
	{
		return std::floor(x.value);
	}

	real ceil(real x)
	{
		return std::ceil(x.value);
	}

	real real::Infinity() { return std::numeric_limits<float>::infinity(); }
	real real::Nan() { return std::numeric_limits<float>::quiet_NaN(); }
}
