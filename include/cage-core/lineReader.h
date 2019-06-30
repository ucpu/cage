#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

namespace cage
{
	class CAGE_API lineReaderClass : private immovable
	{
	public:
		bool readLine(string &line);
		uintPtr left() const;
	};

	CAGE_API holder<lineReaderClass> newLineReader(const char *buffer, uintPtr size); // the buffer must outlive the reader
	CAGE_API holder<lineReaderClass> newLineReader(const memoryBuffer &buffer); // the buffer must outlive the reader
	CAGE_API holder<lineReaderClass> newLineReader(memoryBuffer &&buffer); // the reader takes over the buffer
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
