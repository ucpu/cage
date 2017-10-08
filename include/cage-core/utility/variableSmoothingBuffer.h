#ifndef guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
#define guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F

namespace cage
{
	template<class T, uint32 N = 16> struct variableSmoothingBufferStruct
	{
		variableSmoothingBufferStruct() : index(0), sum(0)
		{
			for (uint32 i = 0; i < N; i++)
				buffer[i] = {};
		}

		void add(T value)
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

	private:
		uint32 index;
		T buffer[N];
		T sum;
	};
}

#endif // guard_variableSmoothingBuffer_h_A47863F30E264545882F83741CC64A3F
