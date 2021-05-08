#ifndef guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
#define guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632

#include <cage-core/math.h>

#ifdef CAGE_ENGINE_EXPORT
#define CAGE_ENGINE_API CAGE_API_EXPORT
#else
#define CAGE_ENGINE_API CAGE_API_IMPORT
#endif

namespace cage
{
	// forward declarations

	struct TransformComponent;
	struct RenderComponent;
	struct TextureAnimationComponent;
	struct SkeletalAnimationComponent;
	enum class LightTypeEnum : uint32;
	struct LightComponent;
	struct ShadowmapComponent;
	struct TextComponent;
	enum class CameraClearFlags : uint32;
	enum class CameraTypeEnum : uint32;
	struct CameraComponent;
	struct SoundComponent;
	struct ListenerComponent;
	enum class CameraEffectsFlags : uint32;
	struct CameraSsao;
	struct CameraMotionBlur;
	struct CameraBloom;
	struct CameraEyeAdaptation;
	struct CameraTonemap;
	struct CameraEffects;
	struct EngineControlThread;
	struct EngineGraphicsDispatchThread;
	struct EngineGraphicsPrepareThread;
	struct EngineSoundThread;
	struct EngineCreateConfig;

	struct FontFormat;
	class Font;
	class FrameBuffer;
	struct GraphicsError;
	class Model;
	class RenderObject;
	class ShaderProgram;
	class Texture;
	class UniformBuffer;
	struct UniformBufferCreateConfig;

	struct GuiParentComponent;
	struct GuiImageComponent;
	enum class ImageModeEnum : uint32;
	struct GuiImageFormatComponent;
	struct GuiTextComponent;
	struct GuiTextFormatComponent;
	struct GuiSelectionComponent;
	struct GuiTooltipComponent;
	struct GuiWidgetStateComponent;
	struct GuiSelectedItemComponent;
	enum class OverflowModeEnum : uint32;
	struct GuiScrollbarsComponent;
	struct GuiExplicitSizeComponent;
	struct GuiEventComponent;
	struct GuiSkinConfig;
	class Gui;
	struct GuiCreateConfig;
	struct GuiLayoutLineComponent;
	struct GuiLayoutTableComponent;
	struct GuiLayoutSplitterComponent;
	enum class InputButtonsPlacementModeEnum : uint32;
	struct GuiSkinWidgetDefaults;
	struct GuiSkinElementLayout;
	enum class GuiElementTypeEnum : uint32;
	struct GuiLabelComponent;
	struct GuiButtonComponent;
	enum class InputTypeEnum : uint32;
	enum class InputStyleFlags : uint32;
	struct GuiInputComponent;
	struct GuiTextAreaComponent;
	enum class CheckBoxStateEnum : uint32;
	struct GuiCheckBoxComponent;
	struct GuiRadioBoxComponent;
	struct GuiComboBoxComponent;
	struct GuiListBoxComponent;
	struct GuiProgressBarComponent;
	struct GuiSliderBarComponent;
	struct GuiColorPickerComponent;
	struct GuiPanelComponent;
	struct GuiSpoilerComponent;

	struct SoundCallbackData;
	class Sound;
	class Speaker;
	struct SpeakerCreateConfig;
	struct Voice;
	struct Listener;
	class VoicesMixer;
	struct VoicesMixerCreateConfig;

	enum class TextureFlags : uint32;
	struct TextureHeader;
	struct ModelHeader;
	struct SkeletonRigHeader;
	struct SkeletalAnimationHeader;
	struct RenderObjectHeader;
	enum class FontFlags : uint32;
	struct FontHeader;
	enum class SoundTypeEnum : uint32;
	enum class SoundFlags : uint32;
	struct SoundSourceHeader;

	enum class EngineProfilingStatsFlags;
	enum class EngineProfilingModeEnum;
	enum class EngineProfilingScopeEnum;
	class EngineProfiling;
	class FpsCamera;
	class FullscreenSwitcher;
	struct FullscreenSwitcherCreateConfig;
	struct ScreenMode;
	class ScreenDevice;
	class ScreenList;
	struct SpeakerLayout;
	struct SpeakerSamplerate;
	class SpeakerDevice;
	class SpeakerList;
	enum class WindowFlags : uint32;
	class Window;
	struct WindowEventListeners;

	struct UubRange;
	class RenderQueue;

	// enum declarations

	enum class ModelRenderFlags : uint32
	{
		None = 0,
		Translucent = 1 << 1,
		TwoSided = 1 << 2,
		DepthTest = 1 << 3,
		DepthWrite = 1 << 4,
		VelocityWrite = 1 << 5,
		ShadowCast = 1 << 6,
		Lighting = 1 << 7,
	};
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

	GCHL_ENUM_BITS(CameraClearFlags);
	GCHL_ENUM_BITS(CameraEffectsFlags);
	GCHL_ENUM_BITS(InputStyleFlags);
	GCHL_ENUM_BITS(TextureFlags);
	GCHL_ENUM_BITS(FontFlags);
	GCHL_ENUM_BITS(SoundFlags);
	GCHL_ENUM_BITS(EngineProfilingStatsFlags);
	GCHL_ENUM_BITS(WindowFlags);
	GCHL_ENUM_BITS(ModelRenderFlags);
	GCHL_ENUM_BITS(ModifiersFlags);
	GCHL_ENUM_BITS(MouseButtonsFlags);

	// constants

	constexpr uint32 MaxTexturesCountPerMaterial = 4;
}

#endif // guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
