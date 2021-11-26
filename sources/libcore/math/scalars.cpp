#include "math.h"

#include <cmath>

namespace cage
{
	Real Real::parse(const String &str)
	{
		return toFloat(privat::tryRemoveParentheses(str));
	}

	Degs Degs::parse(const String &str)
	{
		String s = privat::tryRemoveParentheses(str);
		if (isPattern(s, "", "", "degs"))
			s = subString(s, 0, s.length() - 4);
		else if (isPattern(s, "", "", "deg"))
			s = subString(s, 0, s.length() - 3);
		s = trim(s, true, true, "\t ");
		return Degs(toFloat(s));
	}

	Rads Rads::parse(const String &str)
	{
		String s = privat::tryRemoveParentheses(str);
		if (isPattern(s, "", "", "rads"))
			s = subString(s, 0, s.length() - 4);
		else if (isPattern(s, "", "", "rad"))
			s = subString(s, 0, s.length() - 3);
		s = trim(s, true, true, "\t ");
		return Rads(toFloat(s));
	}

	Real sin(Rads value)
	{
		return std::sin(Real(value).value);
	}

	Real cos(Rads value)
	{
		return std::cos(Real(value).value);
	}

	Real tan(Rads value)
	{
		return std::tan(Real(value).value);
	}

	Rads asin(Real value)
	{
		return (Rads)std::asin(value.value);
	}

	Rads acos(Real value)
	{
		return (Rads)std::acos(value.value);
	}

	Rads atan(Real value)
	{
		return (Rads)std::atan(value.value);
	}

	Rads atan2(Real x, Real y)
	{
		return (Rads)std::atan2(y.value, x.value);
	}

	bool Real::valid() const noexcept
	{
		return !std::isnan(value);
	}

	bool Real::finite() const noexcept
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

	Real sqrt(Real x)
	{
		return std::sqrt(x.value);
	}

	Real pow(Real base, Real exponent)
	{
		return std::pow(base.value, exponent.value);
	}

	Real powE(Real x)
	{
		return std::exp(x.value);
	}

	Real pow2(Real x)
	{
		return std::exp2(x.value);
	}

	Real pow10(Real x)
	{
		return std::pow(10, x.value);
	}

	Real log(Real x)
	{
		return std::log(x.value);
	}

	Real log2(Real x)
	{
		return std::log2(x.value);
	}

	Real log10(Real x)
	{
		return std::log10(x.value);
	}

	Real round(Real x)
	{
		return std::round(x.value);
	}

	Real floor(Real x)
	{
		return std::floor(x.value);
	}

	Real ceil(Real x)
	{
		return std::ceil(x.value);
	}

	Real distanceWrap(Real a, Real b)
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

	Real interpolateWrap(Real a, Real b, Real f)
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
