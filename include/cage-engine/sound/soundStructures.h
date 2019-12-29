namespace cage
{
	struct CAGE_API SoundInterleavedBuffer
	{
		float *buffer;
		uint32 frames;
		uint32 channels;

		SoundInterleavedBuffer();
		SoundInterleavedBuffer(const SoundInterleavedBuffer &other); // creates a reference of the original buffer
		SoundInterleavedBuffer(SoundInterleavedBuffer &&) = delete;
		~SoundInterleavedBuffer();
		SoundInterleavedBuffer &operator = (const SoundInterleavedBuffer &other); // creates a reference of the original buffer
		SoundInterleavedBuffer &operator = (SoundInterleavedBuffer &&) = delete;

		void resize(uint32 channels, uint32 frames);
		void clear();

	private:
		uint32 allocated;
	};

	struct CAGE_API SoundDataBuffer : public SoundInterleavedBuffer
	{
		sint64 time;
		uint32 sampleRate;
		SoundDataBuffer();
	};
}
