#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

#include "core.h"

namespace cage
{
	// split first line from the buffer
	// correctly handles both CRLF and LF

	namespace detail
	{
		// lfOnly == true -> returns false when no new line is found
		// lfOnly == false -> the rest of the buffer is treated as last line
		// set lfOnly to true when you may expand the buffer with more data sometime later
		CAGE_CORE_API [[deprecated]] bool readLine(string &output, const char *&buffer, uintPtr &size, bool lfOnly);
		CAGE_CORE_API bool readLine(string &output, PointerRange<const char> &buffer, bool lfOnly);
	}

	class CAGE_CORE_API LineReader : private Immovable
	{
	public:
		bool readLine(string &line);
		uintPtr remaining() const; // bytes
	};

	CAGE_CORE_API Holder<LineReader> newLineReader(PointerRange<const char> buffer); // the buffer must outlive the reader
	CAGE_CORE_API Holder<LineReader> newLineReader(MemoryBuffer &&buffer); // the reader takes over the buffer
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
