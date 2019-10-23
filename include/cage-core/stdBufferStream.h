#ifndef guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa
#define guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa

#include <iostream>

namespace cage
{
	class CAGE_API bufferIStream : protected std::streambuf, public std::istream
	{
	public:
		explicit bufferIStream(const void *data, uintPtr size);
		explicit bufferIStream(const memoryBuffer &buffer);
		uintPtr position() const;
	};

	class CAGE_API bufferOStream : protected std::streambuf, public std::ostream
	{
	public:
		explicit bufferOStream(memoryBuffer &buffer);

	protected:
		using std::streambuf::traits_type;
		typedef traits_type::int_type int_type;
		std::streamsize xsputn(const char* s, std::streamsize n) override;
		int_type overflow(int_type ch) override;
		memoryBuffer &buffer;
	};
}

#endif
