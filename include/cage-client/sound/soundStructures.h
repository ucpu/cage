namespace cage
{
	struct CAGE_API soundInterleavedBufferStruct
	{
		float *buffer;
		uint32 frames;
		uint32 channels;

		soundInterleavedBufferStruct();
		soundInterleavedBufferStruct(const soundInterleavedBufferStruct &other);
		~soundInterleavedBufferStruct();
		soundInterleavedBufferStruct &operator = (const soundInterleavedBufferStruct &other);

		void resize(uint32 channels, uint32 frames);
		void clear();

	private:
		uint32 allocated;
	};

	struct CAGE_API soundDataBufferStruct : public soundInterleavedBufferStruct
	{
		sint64 time;
		uint32 sampleRate;
		soundDataBufferStruct();
	};
}
