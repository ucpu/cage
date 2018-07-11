namespace cage
{
	class CAGE_API textureClass
	{
	public:
		uint32 animationDuration;
		bool animationLoop;

		uint32 getId() const;
		uint32 getTarget() const;
		void getResolution(uint32 &width, uint32 &height) const;
		void getResolution(uint32 &width, uint32 &height, uint32 &depth) const;
		void bind() const;

		void image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, void *data);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, void *data, uint32 stride = 0);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, void *data);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();

		static void multiBind(uint32 count, const uint32 tius[], const textureClass *const texs[]);
	};

	CAGE_API holder<textureClass> newTexture(windowClass *context);
	CAGE_API holder<textureClass> newTexture(windowClass *context, uint32 target);

	namespace detail
	{
		CAGE_API vec4 evalSamplesForTextureAnimation(textureClass *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}
}
