namespace cage
{
	class CAGE_API frameBufferClass
	{
	public:
		uint32 getId() const;
		uint32 getTarget() const;
		void bind() const;

		void depthTexture(textureClass *tex);
		void colorTexture(uint32 index, textureClass *tex);
		void activeAttachments(uint32 mask);
		void checkStatus();
	};

	CAGE_API holder<frameBufferClass> newDrawFrameBuffer(windowClass *context);
	CAGE_API holder<frameBufferClass> newReadFrameBuffer(windowClass *context);
}
