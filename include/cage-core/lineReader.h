#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

namespace cage
{
	namespace detail
	{
		// lfOnly == true -> returns false when no new line is found
		// lfOnly == false -> the rest of the buffer is treated as last line
		// set lfOnly to true when you may expand the buffer with more data sometime later
		CAGE_API bool readLine(string &output, const char *&buffer, uintPtr &size, bool lfOnly);
	}

	class CAGE_API LineReader : private Immovable
	{
	public:
		bool readLine(string &line);
		uintPtr left() const; // bytes
	};

	CAGE_API Holder<LineReader> newLineReader(const char *buffer, uintPtr size); // the buffer must outlive the reader
	CAGE_API Holder<LineReader> newLineReader(const MemoryBuffer &buffer); // the buffer must outlive the reader
	CAGE_API Holder<LineReader> newLineReader(MemoryBuffer &&buffer); // the reader takes over the buffer
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
