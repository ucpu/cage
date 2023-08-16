#ifndef guard_variableInterpolatingBuffer_h_f4h56fr4h564rdf5h56ea4rz4jwa
#define guard_variableInterpolatingBuffer_h_f4h56fr4h564rdf5h56ea4rz4jwa

#include <cage-core/math.h>

namespace cage
{
	namespace privat
	{
		template<class T>
		struct DefaultInterpolator
		{
			T operator()(const T &a, const T &b, float p) const { return cage::interpolate(a, b, p); }
		};
	}

	template<class T, class F = privat::DefaultInterpolator<T>>
	struct VariableInterpolatingBuffer
	{
		explicit VariableInterpolatingBuffer(F fnc = F()) : fnc(fnc) {}

		void clear()
		{
			for (int i = 0; i < 3; i++)
				times[i] = 0;
		}

		void set(const T &a, uint64 time, uint64 current)
		{
			CAGE_ASSERT(time > 0);
			if (current == time || times[0] == 0)
			{
				times[0] = time;
				data[0] = a;
				if (current == time)
					consolidate();
				return;
			}
			data[0] = (*this)(current);
			times[0] = current;
			consolidate();
			if (current > time)
				return;
			if (times[2] == 0)
			{
				if (times[1] == 0 || times[1] == time)
				{
					data[1] = a;
					times[1] = time;
					return;
				}
			}
			else if (times[2] > time)
				return;
			data[2] = a;
			times[2] = time;
			if (times[2] < times[1])
			{
				T tmpT = data[1];
				data[1] = data[2];
				data[2] = tmpT;
				uint64 tmp64 = times[1];
				times[1] = times[2];
				times[2] = tmp64;
			}
		}

		T operator()(uint64 current) const
		{
			CAGE_ASSERT(current > 0);
			if (times[0] == 0)
				CAGE_THROW_ERROR(Exception, "interpolator has no data");
			if (current <= times[0] || times[1] == 0)
				return data[0];
			if (current <= times[1] || times[2] == 0)
				return interp(0, current);
			return interp(1, current);
		}

		explicit operator bool() const { return times[0] != 0; }

	private:
		T data[3] = {};
		uint64 times[3] = {};
		F fnc;

		T interp(uint32 i, uint64 current) const
		{
			CAGE_ASSERT(i < 2);
			uint64 t1 = times[i];
			uint64 t2 = times[i + 1];
			CAGE_ASSERT(t2 > t1);
			float p = (current - t1) / (float)(t2 - t1);
			return fnc(data[i], data[i + 1], p);
		}

		void consolidate()
		{
			if (times[1] <= times[0])
			{
				if (times[2] <= times[0])
				{
					times[1] = 0;
				}
				else
				{
					times[1] = times[2];
					data[1] = data[2];
				}
				times[2] = 0;
			}
		}
	};
}

#endif // guard_variableInterpolatingBuffer_h_f4h56fr4h564rdf5h56ea4rz4jwa
