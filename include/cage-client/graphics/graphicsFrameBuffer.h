namespace cage
{
	class CAGE_API frameBufferClass
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		uint32 getTarget() const;
		void bind() const;

		void depthTexture(textureClass *tex);
		void colorTexture(uint32 index, textureClass *tex);
		void activeAttachments(uint32 mask);
		void clear();
		void checkStatus();
	};

	CAGE_API holder<frameBufferClass> newDrawFrameBuffer();
	CAGE_API holder<frameBufferClass> newReadFrameBuffer();
}
