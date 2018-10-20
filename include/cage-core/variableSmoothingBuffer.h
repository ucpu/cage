#ifndef guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
#define guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F

namespace cage
{
	template<class T, uint32 N = 16>
	struct variableSmoothingBufferStruct
	{
		variableSmoothingBufferStruct() : index(0), sum(T())
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = T();
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

		T last() const
		{
			return buffer[index];
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
}

#endif // guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F