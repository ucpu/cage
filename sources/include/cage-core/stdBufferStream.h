#ifndef guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa
#define guard_bufferStream_h_sadgDS_gfrdskjhgidsajgoa

#include "core.h"

#include <iostream>

namespace cage
{
	struct MemoryBuffer;

	class CAGE_CORE_API BufferIStream : protected std::streambuf, public std::istream
	{
	public:
		explicit BufferIStream(PointerRange<const char> buffer);

	protected:
		using std::streambuf::traits_type;
		using int_type = traits_type::int_type;
		using pos_type = traits_type::pos_type;
		using off_type = traits_type::off_type;
		pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = ios_base::in | ios_base::out) override;
	};

	class CAGE_CORE_API BufferOStream : protected std::streambuf, public std::ostream
	{
	public:
		explicit BufferOStream(MemoryBuffer &buffer);

	protected:
		using std::streambuf::traits_type;
		using int_type = traits_type::int_type;
		using pos_type = traits_type::pos_type;
		using off_type = traits_type::off_type;
		std::streamsize xsputn(const char *s, std::streamsize n) override;
		int_type overflow(int_type ch) override;
		pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = ios_base::in | ios_base::out) override;

		MemoryBuffer &buffer;
		uintPtr writePos = 0;
	};
}

#endif
