namespace cage
{
	// definition

	static const uint32 MaxTexturesCountPerMaterial = 4;
	static const uint32 MaxRoutines = 10;

	// assets

	enum class meshDataFlags : uint32
	{
		None = 0,
		Normals = 1 << 0,
		Tangents = 1 << 1,
		Bones = 1 << 2,
		Uvs = 1 << 3,
		Aux0 = 1 << 4,
		Aux1 = 1 << 5,
		Aux2 = 1 << 6,
		Aux3 = 1 << 7,
	};
	GCHL_ENUM_BITS(meshDataFlags);

	enum class meshRenderFlags : uint32
	{
		None = 0,
		OpacityTexture = 1 << 1,
		Transparency = 1 << 2, // mutually exclusive with Translucency
		Translucency = 1 << 3, // mutually exclusive with Transparency
		TwoSided = 1 << 4,
		DepthTest = 1 << 5,
		DepthWrite = 1 << 6,
		VelocityWrite = 1 << 7,
		ShadowCast = 1 << 8,
		Lighting = 1 << 9,
	};
	GCHL_ENUM_BITS(meshRenderFlags);

	enum class textureFlags : uint32
	{
		None = 0,
		GenerateMipmaps = 1 << 0,
		AnimationLoop = 1 << 1,
		AllowDownscale = 1 << 2,
	};
	GCHL_ENUM_BITS(textureFlags);

	enum class fontFlags : uint32
	{
		None = 0,
		Kerning = 1 << 0,
	};
	GCHL_ENUM_BITS(fontFlags);

	enum class soundTypeEnum : uint32
	{
		RawRaw, // short sounds
		CompressedRaw, // long sounds, but played many times concurrently
		CompressedCompressed, // long sounds, usually only played once at any given time
	};

	enum class soundFlags : uint32
	{
		None = 0,
		RepeatBeforeStart = 1 << 0,
		RepeatAfterEnd = 1 << 1,
	};
	GCHL_ENUM_BITS(soundFlags);

	// window events

	enum class modifiersFlags : uint32
	{
		None = 0,
		Shift = 1 << 0,
		Ctrl = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3,
	};
	GCHL_ENUM_BITS(modifiersFlags);

	enum class mouseButtonsFlags : uint32
	{
		None = 0,
		Left = 1 << 0,
		Right = 1 << 1,
		Middle = 1 << 2,
	};
	GCHL_ENUM_BITS(mouseButtonsFlags);

	enum class windowFlags : uint32
	{
		None = 0,
		Resizeable = 1 << 0,
		Border = 1 << 1,
	};
	GCHL_ENUM_BITS(windowFlags);

	// font

	enum class textAlignEnum : uint32
	{
		Left,
		Right,
		Center,
	};

	// engine components

	enum class lightTypeEnum : uint32
	{
		Directional,
		Point,
		Spot,
	};

	enum class cameraEffectsFlags : uint32
	{
		None = 0,
		AmbientOcclusion = 1 << 0,
		MotionBlur = 1 << 1,
		Bloom = 1 << 2,
		EyeAdaptation = 1 << 3,
		ToneMapping = 1 << 4,
		GammaCorrection = 1 << 5,
		AntiAliasing = 1 << 6,
		GeometryPass = AmbientOcclusion | MotionBlur,
		ScreenPass = Bloom | ToneMapping | GammaCorrection | AntiAliasing,
		CombinedPass = GeometryPass | ScreenPass,
	};
	GCHL_ENUM_BITS(cameraEffectsFlags);

	enum class cameraClearFlags : uint32
	{
		None = 0,
		Depth = 1 << 0,
		Color = 1 << 1,
		Stencil = 1 << 2,
	};
	GCHL_ENUM_BITS(cameraClearFlags);

	enum class cameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};
}
