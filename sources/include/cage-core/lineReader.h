#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

#include "core.h"

namespace cage
{
	// split first line from the buffer
	// correctly handles both CRLF and LF

	namespace detail
	{
		// streaming == true -> returns false when no new line is found
		// streaming == false -> the rest of the buffer is treated as last line
		// set streaming to true if you may expand the buffer with more data later
		CAGE_CORE_API bool readLine(string &output, PointerRange<const char> &buffer, bool streaming);
	}

	class CAGE_CORE_API LineReader : private Immovable
	{
	public:
		bool readLine(string &line);
		uintPtr remaining() const; // bytes
	};

	CAGE_CORE_API Holder<LineReader> newLineReader(PointerRange<const char> buffer); // the buffer must outlive the reader
	CAGE_CORE_API Holder<LineReader> newLineReader(MemoryBuffer &&buffer); // the reader takes ownership of the buffer
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
