#ifndef guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa
#define guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa

#include <iostream>

namespace cage
{
	class CAGE_API BufferIStream : protected std::streambuf, public std::istream
	{
	public:
		explicit BufferIStream(const void *data, uintPtr size);
		explicit BufferIStream(const MemoryBuffer &buffer);
		uintPtr position() const;
	};

	class CAGE_API BufferOStream : protected std::streambuf, public std::ostream
	{
	public:
		explicit BufferOStream(MemoryBuffer &buffer);

	protected:
		using std::streambuf::traits_type;
		typedef traits_type::int_type int_type;
		std::streamsize xsputn(const char* s, std::streamsize n) override;
		int_type overflow(int_type ch) override;
		MemoryBuffer &buffer;
	};
}

#endif
