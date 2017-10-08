namespace cage
{
	class CAGE_API frameBufferClass
	{
	public:
		const uint32 getId() const;

		void bind() const;
		void bind(bool draw, bool read) const;

		void depthTexture(textureClass *tex);
		void colorTexture(uint32 index, textureClass *tex);
		void drawAttachments(uint32 mask);
		void checkStatus();
	};

	CAGE_API holder<frameBufferClass> newFrameBuffer(windowClass *context);
}
