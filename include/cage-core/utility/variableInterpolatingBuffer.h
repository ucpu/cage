#ifndef interpolator_h__
#define interpolator_h__

namespace cage
{
	namespace privat
	{
		template<class T>
		struct defaultInterpolatorFunctor
		{
			T operator () (const T &a, const T &b, float p) const
			{
				return cage::interpolate(a, b, p);
			}
		};
	}

	template<class T, class F = privat::defaultInterpolatorFunctor<T>>
	struct variableInterpolatingBufferStruct
	{
		variableInterpolatingBufferStruct(F fnc = F()) : fnc(fnc)
		{
			clear();
		}

		void clear()
		{
			for (int i = 0; i < 3; i++)
				times[i] = 0;
		}

		void set(const T &a, uint64 time, uint64 current)
		{
			CAGE_ASSERT_RUNTIME(time > 0, "zero time is used as special value");
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

		T operator () (uint64 current) const
		{
			CAGE_ASSERT_RUNTIME(current > 0, "zero time is used as special value");
			if (times[0] == 0)
				CAGE_THROW_ERROR(exception, "interpolator has no data");
			if (current <= times[0] || times[1] == 0)
				return data[0];
			if (current <= times[1] || times[2] == 0)
				return interp(0, current);
			return interp(1, current);
		}

		operator bool() const
		{
			return times[0] != 0;
		}

	private:
		T data[3];
		uint64 times[3];
		F fnc;

		T interp(uint32 i, uint64 current) const
		{
			CAGE_ASSERT_RUNTIME(i < 2, "invalid parameters", i);
			uint64 t1 = times[i];
			uint64 t2 = times[i + 1];
			CAGE_ASSERT_RUNTIME(t2 > t1, "invalid times in interpolator", times[0], times[1], times[2], i);
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

#endif // interpolator_h__
