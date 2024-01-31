#ifndef guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
#define guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632

#include <cage-core/enumBits.h>
#include <cage-core/math.h>

namespace cage
{
	// forward declarations

	struct GenericInput;

	enum class TextureFlags : uint32;
	enum class TextureSwizzleEnum : uint8;
	enum class FontFlags : uint32;
	enum class SoundTypeEnum : uint32;
	enum class SoundFlags : uint32;
	enum class ScreenSpaceEffectsFlags : uint32;
	enum class CameraTypeEnum : uint32;
	enum class ImageModeEnum : uint32;
	enum class OverflowModeEnum : uint32;
	enum class InputTypeEnum : uint32;
	enum class InputStyleFlags : uint32;
	enum class CheckBoxStateEnum : uint32;
	enum class InputButtonsPlacementModeEnum : uint32;
	enum class GuiElementTypeEnum : uint32;
	enum class LightTypeEnum : uint32;
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

	// enums flags

	GCHL_ENUM_BITS(ScreenSpaceEffectsFlags);
	GCHL_ENUM_BITS(InputStyleFlags);
	GCHL_ENUM_BITS(TextureFlags);
	GCHL_ENUM_BITS(FontFlags);
	GCHL_ENUM_BITS(SoundFlags);
	GCHL_ENUM_BITS(WindowFlags);
	GCHL_ENUM_BITS(ModifiersFlags);
	GCHL_ENUM_BITS(MouseButtonsFlags);

	// constants

	constexpr uint32 MaxTexturesCountPerMaterial = 4;
}

#endif // guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
