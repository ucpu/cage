namespace cage
{
	class CAGE_API renderTexture : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint64 animationDuration;
		bool animationLoop;

		uint32 getId() const;
		uint32 getTarget() const;
		void getResolution(uint32 &width, uint32 &height) const;
		void getResolution(uint32 &width, uint32 &height, uint32 &depth) const;
		void bind() const;

		void image2d(uint32 w, uint32 h, uint32 internalFormat);
		void image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data, uintPtr stride = 0);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, const void *data);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();

		static void multiBind(uint32 count, const uint32 tius[], const renderTexture *const texs[]);
	};

	CAGE_API holder<renderTexture> newRenderTexture();
	CAGE_API holder<renderTexture> newRenderTexture(uint32 target);

	namespace detail
	{
		CAGE_API vec4 evalSamplesForTextureAnimation(const renderTexture *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_API assetScheme genAssetSchemeRenderTexture(uint32 threadIndex, windowHandle *memoryContext);
	static const uint32 assetSchemeIndexRenderTexture = 11;
}