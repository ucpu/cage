#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

namespace cage
{
	class CAGE_API lineReader : private immovable
	{
	public:
		bool readLine(string &line);
		uintPtr left() const; // bytes
	};

	CAGE_API holder<lineReader> newLineReader(const char *buffer, uintPtr size); // the buffer must outlive the reader
	CAGE_API holder<lineReader> newLineReader(const memoryBuffer &buffer); // the buffer must outlive the reader
	CAGE_API holder<lineReader> newLineReader(memoryBuffer &&buffer); // the reader takes over the buffer
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
