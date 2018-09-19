#ifndef guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
#define guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_

namespace cage
{
	struct CAGE_API lineReaderBuffer
	{
		lineReaderBuffer(const char *buffer, uintPtr size) : buffer(buffer), size(size)
		{}

		lineReaderBuffer(const memoryBuffer &buffer);

		bool readLine(string &line);

		uintPtr left() const
		{
			return size;
		}

	private:
		const char *buffer;
		uintPtr size;
	};
}

#endif // guard_lineReader_h_09089ef7_374c_4571_85be_3e7de9acc3aa_
