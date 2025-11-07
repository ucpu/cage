#ifndef guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
#define guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632

#include <cage-core/enumBits.h>
#include <cage-core/math.h>

namespace cage
{
	// forward declarations

	struct GenericInput;

	enum class TextureFlags : uint32;
	enum class SoundCompressionEnum : uint32;
	enum class ScreenSpaceEffectsFlags : uint32;
	enum class ImageModeEnum : uint32;
	enum class OverflowModeEnum : uint32;
	enum class InputTypeEnum : uint32;
	enum class InputStyleFlags : uint32;
	enum class CheckBoxStateEnum : uint32;
	enum class InputButtonsPlacementModeEnum : uint32;
	enum class GuiElementTypeEnum : uint32;
	enum class WindowFlags : uint32;

	// enum declarations

	enum class TextAlignEnum : uint32
	{
		Left,
		Right,
		Center,
	};
	enum class ModifiersFlags : uint32
	{
		None = 0,
		Shift = 1 << 0,
		Ctrl = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3,
	};
	enum class MouseButtonsFlags : uint32
	{
		None = 0,
		Left = 1 << 0,
		Right = 1 << 1,
		Middle = 1 << 2,
	};
	enum class LightTypeEnum : uint32
	{
		Directional,
		Point,
		Spot,
	};
	enum class LightAttenuationEnum : uint32
	{
		None = 0,
		Linear,
		Logarithmic,
		InverseSquare,
	};
	enum class MeshComponentsFlags : uint32
	{
		None = 0,
		Normals = 1 << 0,
		Bones = 1 << 1,
		Uvs2 = 1 << 2,
		Uvs3 = 1 << 3,
	};
	enum class DepthTestEnum : uint32
	{
		None, // undefined
		Never,
		Less, // default
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};
	enum class BlendingEnum : uint32
	{
		None, // ONE, ZERO
		Additive, // ONE, ONE
		AlphaTransparency, // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
		PremultipliedTransparency, // ONE, ONE_MINUS_SRC_ALPHA
	};
	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};
	enum class SoundAttenuationEnum : uint32
	{
		None = 0,
		Linear,
		Logarithmic,
		InverseSquare,
	};

	// enums flags

	GCHL_ENUM_BITS(MeshComponentsFlags);
	GCHL_ENUM_BITS(ScreenSpaceEffectsFlags);
	GCHL_ENUM_BITS(InputStyleFlags);
	GCHL_ENUM_BITS(TextureFlags);
	GCHL_ENUM_BITS(WindowFlags);
	GCHL_ENUM_BITS(ModifiersFlags);
	GCHL_ENUM_BITS(MouseButtonsFlags);

	// constants

	constexpr uint32 MaxTexturesCountPerMaterial = 4; // albedo, special, normal, custom
}

#endif // guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
