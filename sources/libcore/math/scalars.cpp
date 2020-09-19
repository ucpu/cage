#include "math.h"

#include <cmath>

namespace cage
{
	real real::parse(const string &str)
	{
		return toFloat(privat::tryRemoveParentheses(str));
	}

	degs degs::parse(const string &str)
	{
		string s = privat::tryRemoveParentheses(str);
		if (isPattern(s, "", "", "degs"))
			s = subString(s, 0, s.length() - 4);
		else if (isPattern(s, "", "", "deg"))
			s = subString(s, 0, s.length() - 3);
		s = trim(s, true, true, "\t ");
		return degs(toFloat(s));
	}

	rads rads::parse(const string &str)
	{
		string s = privat::tryRemoveParentheses(str);
		if (isPattern(s, "", "", "rads"))
			s = subString(s, 0, s.length() - 4);
		else if (isPattern(s, "", "", "rad"))
			s = subString(s, 0, s.length() - 3);
		s = trim(s, true, true, "\t ");
		return rads(toFloat(s));
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
		return (rads)std::asin(value.value);
	}

	rads acos(real value)
	{
		return (rads)std::acos(value.value);
	}

	rads atan(real value)
	{
		return (rads)std::atan(value.value);
	}

	rads atan2(real x, real y)
	{
		return (rads)std::atan2(y.value, x.value);
	}

	bool real::valid() const noexcept
	{
		return !std::isnan(value);
	}

	bool real::finite() const noexcept
	{
		return std::isfinite(value);
	}

	bool valid(float a) noexcept
	{
		return !std::isnan(a);
	}

	bool valid(double a) noexcept
	{
		return !std::isnan(a);
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

	real distanceWrap(real a, real b)
	{
		a = wrap(a);
		b = wrap(b);
		if (abs(a - b) <= 0.5)
			return abs(a - b);
		if (a < b)
			a += 1;
		else
			b += 1;
		return wrap(abs(a - b));
	}

	real interpolateWrap(real a, real b, real f)
	{
		a = wrap(a);
		b = wrap(b);
		if (abs(a - b) <= 0.5)
			return interpolate(a, b, f);
		if (a < b)
			a += 1;
		else
			b += 1;
		return wrap(interpolate(a, b, f));
	}
}
