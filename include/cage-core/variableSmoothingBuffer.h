#ifndef guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
#define guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F

namespace cage
{
	template<class T, uint32 N>
	struct VariableSmoothingBuffer
	{
		VariableSmoothingBuffer() : index(0), sum(T())
		{
			seed(T());
		}

		void seed(const T &value)
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = value;
			sum = value * N;
		}

		void add(const T &value)
		{
			index = (index + 1) % N;
			sum += value - buffer[index];
			buffer[index] = value;
		}

		T smooth() const
		{
			return sum / N;
		}

		T current() const
		{
			return buffer[index];
		}

		T oldest() const
		{
			return buffer[(index + 1) % N];
		}

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
		uint32 index;
		T buffer[N];
		T sum;
	};

	namespace privat
	{
		CAGE_API quat averageQuaternions(const quat *quaternions, uint32 count);
	}

	template<uint32 N>
	struct VariableSmoothingBuffer<quat, N>
	{
		VariableSmoothingBuffer() : index(0)
		{}

		void seed(const quat &value)
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = value;
			avg = value;
		}

		void add(const quat &value)
		{
			index = (index + 1) % N;
			buffer[index] = value;
			avg = privat::averageQuaternions(buffer, N);
		}

		quat smooth() const
		{
			return avg;
		}

		quat current() const
		{
			return buffer[index];
		}

		quat oldest() const
		{
			return buffer[(index + 1) % N];
		}

	private:
		uint32 index;
		quat buffer[N];
		quat avg;
	};
}

#endif // guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
