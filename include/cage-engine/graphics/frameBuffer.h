namespace cage
{
	class CAGE_API FrameBuffer : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		uint32 getTarget() const;
		void bind() const;

		void depthTexture(Texture *tex);
		void colorTexture(uint32 index, Texture *tex, uint32 mipmapLevel = 0);
		void activeAttachments(uint32 mask);
		void clear();
		void checkStatus();
	};

	CAGE_API Holder<FrameBuffer> newFrameBufferDraw();
	CAGE_API Holder<FrameBuffer> newFrameBufferRead();
}
