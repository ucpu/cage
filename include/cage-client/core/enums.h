namespace cage
{
	// assets

	enum class meshFlags : uint32
	{
		None = 0,
		Uvs = 1 << 0,
		Normals = 1 << 1,
		Tangents = 1 << 2,
		Bones = 1 << 3,
		OpacityTexture = 1 << 4,
		Transparency = 1 << 5,
		Translucency = 1 << 6,
		TwoSided = 1 << 7,
		DepthTest = 1 << 8,
		DepthWrite = 1 << 9,
		VelocityWrite = 1 << 10,
		Lighting = 1 << 11,
		ShadowCast = 1 << 12,
	};
	GCHL_ENUM_BITS(meshFlags);

	enum class textureFlags : uint32
	{
		None = 0,
		GenerateBitmap = 1 << 0,
		AnimationLoop = 1 << 1,
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

	// stereoscopy

	enum class stereoModeEnum : uint32
	{
		Mono,
		LeftRight,
		TopBottom,
	};

	enum class eyeEnum : uint32
	{
		Mono,
		Left,
		Right,
	};

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
