#include <cmath>

#include "math.h"

namespace cage
{
	Real Real::parse(const String &str)
	{
		return toFloat(privat::tryRemoveParentheses(str));
	}

	bool Real::valid() const
	{
		return !std::isnan(value);
	}

	bool Real::finite() const
	{
		return std::isfinite(value);
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

	bool Rads::valid() const
	{
		return value.valid();
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

	bool Degs::valid() const
	{
		return value.valid();
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

	bool valid(float a)
	{
		return !std::isnan(a);
	}

	bool valid(double a)
	{
		return !std::isnan(a);
	}

	Real sqrt(Real x)
	{
		return std::sqrt(x.value);
	}

#define GCHL_GENERATE(TYPE) \
	double sqrt(TYPE x) \
	{ \
		return std::sqrt(x); \
	}
	GCHL_GENERATE(sint8);
	GCHL_GENERATE(sint16);
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(sint64);
	GCHL_GENERATE(uint8);
	GCHL_GENERATE(uint16);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(uint64);
	GCHL_GENERATE(float);
	GCHL_GENERATE(double);
#undef GCHL_GENERATE

	Real pow(Real base, Real exponent)
	{
		return std::pow(base.value, exponent.value);
	}

	Real pow(Real x)
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

	Real log(Real base, Real x)
	{
		return std::log(x.value) / std::log(base.value);
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
