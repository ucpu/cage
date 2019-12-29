namespace cage
{
	// definition

	static const uint32 MaxTexturesCountPerMaterial = 4;

	// assets

	enum class MeshDataFlags : uint32
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
	GCHL_ENUM_BITS(MeshDataFlags);

	enum class MeshRenderFlags : uint32
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
	GCHL_ENUM_BITS(MeshRenderFlags);

	enum class TextureFlags : uint32
	{
		None = 0,
		GenerateMipmaps = 1 << 0,
		AnimationLoop = 1 << 1,
		AllowDownscale = 1 << 2,
	};
	GCHL_ENUM_BITS(TextureFlags);

	enum class FontFlags : uint32
	{
		None = 0,
		Kerning = 1 << 0,
	};
	GCHL_ENUM_BITS(FontFlags);

	enum class SoundTypeEnum : uint32
	{
		RawRaw, // short sounds
		CompressedRaw, // long sounds, but played many times concurrently
		CompressedCompressed, // long sounds, usually only played once at any given time
	};

	enum class SoundFlags : uint32
	{
		None = 0,
		LoopBeforeStart = 1 << 0,
		LoopAfterEnd = 1 << 1,
	};
	GCHL_ENUM_BITS(SoundFlags);

	// window events

	enum class ModifiersFlags : uint32
	{
		None = 0,
		Shift = 1 << 0,
		Ctrl = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3,
	};
	GCHL_ENUM_BITS(ModifiersFlags);

	enum class MouseButtonsFlags : uint32
	{
		None = 0,
		Left = 1 << 0,
		Right = 1 << 1,
		Middle = 1 << 2,
	};
	GCHL_ENUM_BITS(MouseButtonsFlags);

	enum class WindowFlags : uint32
	{
		None = 0,
		Resizeable = 1 << 0,
		Border = 1 << 1,
	};
	GCHL_ENUM_BITS(WindowFlags);

	// font

	enum class TextAlignEnum : uint32
	{
		Left,
		Right,
		Center,
	};

	// engine components

	enum class LightTypeEnum : uint32
	{
		Directional,
		Point,
		Spot,
	};

	enum class CameraEffectsFlags : uint32
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
	GCHL_ENUM_BITS(CameraEffectsFlags);

	enum class CameraClearFlags : uint32
	{
		None = 0,
		Depth = 1 << 0,
		Color = 1 << 1,
		Stencil = 1 << 2,
	};
	GCHL_ENUM_BITS(CameraClearFlags);

	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};
}
