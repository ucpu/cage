#ifndef guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
#define guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F

#include <cage-core/math.h>

namespace cage
{
	template<class T, uint32 N = 16>
	struct VariableSmoothingBuffer
	{
		void seed(T value)
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = value;
			sum_ = value * N;
		}

		void add(T value)
		{
			index = (index + 1) % N;
			sum_ += value - buffer[index];
			buffer[index] = value;
		}

		T smooth() const { return sum_ / N; }
		T current() const { return buffer[index]; }
		T oldest() const { return buffer[(index + 1) % N]; }
		T sum() const { return sum_; }

		T max() const
		{
			T res = buffer[0];
			for (uint32 i = 1; i < N; i++)
				res = cage::max(res, buffer[i]);
			return res;
		}

		T min() const
		{
			T res = buffer[0];
			for (uint32 i = 1; i < N; i++)
				res = cage::min(res, buffer[i]);
			return res;
		}

	private:
		uint32 index = 0;
		T buffer[N] = {};
		T sum_ = T();
	};

	namespace privat
	{
		CAGE_CORE_API Quat averageQuaternions(PointerRange<const Quat> quaternions);
	}

	template<uint32 N>
	struct VariableSmoothingBuffer<Quat, N>
	{
		void seed(Quat value)
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = value;
			avg = value;
		}

		void add(Quat value)
		{
			index = (index + 1) % N;
			buffer[index] = value;
			dirty = true;
		}

		Quat smooth() const
		{
			if (dirty)
			{
				avg = privat::averageQuaternions(buffer);
				dirty = false;
			}
			return avg;
		}

		Quat current() const { return buffer[index]; }
		Quat oldest() const { return buffer[(index + 1) % N]; }

	private:
		uint32 index = 0;
		Quat buffer[N];
		mutable Quat avg;
		mutable bool dirty = false;
	};

	template<uint32 N>
	struct VariableSmoothingBuffer<Transform, N>
	{
		void seed(const Transform &value)
		{
			t.seed(value.position);
			r.seed(value.orientation);
			s.seed(value.scale);
		}

		void add(const Transform &value)
		{
			t.add(value.position);
			r.add(value.orientation);
			s.add(value.scale);
		}

		Transform smooth() const { return Transform(t.smooth(), r.smooth(), s.smooth()); }
		Transform current() const { return Transform(t.current(), r.current(), s.current()); }
		Transform oldest() const { return Transform(t.oldest(), r.oldest(), s.oldest()); }

	private:
		VariableSmoothingBuffer<Vec3, N> t;
		VariableSmoothingBuffer<Quat, N> r;
		VariableSmoothingBuffer<Real, N> s;
	};
}

#endif // guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
