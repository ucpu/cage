#ifndef guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
#define guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632

#include <cage-core/math.h>

namespace cage
{
	// forward declarations

	enum class TextureFlags : uint32;
	enum class TextureSwizzleEnum : uint8;
	struct TextureHeader;
	struct ModelHeader;
	struct RenderObjectHeader;
	enum class FontFlags : uint32;
	struct FontHeader;
	enum class SoundTypeEnum : uint32;
	enum class SoundFlags : uint32;
	struct SoundSourceHeader;
	enum class CameraEffectsFlags : uint32;
	enum class CameraTypeEnum : uint32;
	struct CameraProperties;
	struct FontFormat;
	class Font;
	class FrameBuffer;
	class Gamepad;
	struct GraphicsError;
	struct GraphicsDebugScope;
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
	struct GuiLayoutLineComponent;
	struct GuiLayoutTableComponent;
	struct GuiLayoutSplitterComponent;
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
	class GuiManager;
	struct GuiManagerCreateConfig;
	enum class InputButtonsPlacementModeEnum : uint32;
	struct GuiSkinWidgetDefaults;
	struct GuiSkinElementLayout;
	enum class GuiElementTypeEnum : uint32;
	struct GuiSkinConfig;
	enum class InputClassEnum : uint32;
	struct InputWindow;
	struct InputWindowValue;
	struct InputMouse;
	struct InputMouseWheel;
	struct InputKey;
	struct InputGamepadState;
	struct InputGamepadKey;
	struct InputGamepadAxis;
	struct InputGuiWidget;
	struct GenericInput;
	struct InputsGeneralizer;
	struct InputsDispatchers;
	struct InputsListeners;
	class Model;
	class ProvisionalUniformBuffer;
	class ProvisionalFrameBuffer;
	class ProvisionalTexture;
	class ProvisionalGraphics;
	// struct UniformBufferHandle;
	// struct FrameBufferHandle;
	// struct TextureHandle;
	class RenderObject;
	struct RenderPipelineCamera;
	struct RenderPipelineDebugVisualization;
	struct RenderPipelineResult;
	class RenderPipeline;
	struct RenderPipelineCreateConfig;
	struct UubRange;
	class RenderQueue;
	struct RenderQueueNamedScope;
	struct TransformComponent;
	struct RenderComponent;
	struct TextureAnimationComponent;
	struct SkeletalAnimationComponent;
	enum class LightTypeEnum : uint32;
	struct LightComponent;
	struct ShadowmapComponent;
	struct TextComponent;
	struct CameraComponent;
	struct SoundComponent;
	struct ListenerComponent;
	struct ScreenMode;
	class ScreenDevice;
	class ScreenList;
	struct ScreenSpaceCommonConfig;
	struct ScreenSpaceAmbientOcclusionConfig;
	struct ScreenSpaceDepthOfFieldConfig;
	struct ScreenSpaceEyeAdaptationConfig;
	struct ScreenSpaceBloomConfig;
	struct ScreenSpaceTonemapConfig;
	struct ScreenSpaceFastApproximateAntiAliasingConfig;
	struct ScreenSpaceAmbientOcclusion;
	struct ScreenSpaceDepthOfField;
	struct ScreenSpaceEyeAdaptation;
	struct ScreenSpaceBloom;
	struct ScreenSpaceTonemap;
	class ShaderProgram;
	class Sound;
	struct SoundCallbackData;
	class Speaker;
	struct SpeakerCreateConfig;
	class SpeakerDevice;
	class SpeakerList;
	class Texture;
	struct BindlessHandle;
	class UniformBuffer;
	struct UniformBufferCreateConfig;
	struct Voice;
	struct Listener;
	class VoicesMixer;
	struct VoicesMixerCreateConfig;
	enum class WindowFlags : uint32;
	class Window;

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

	GCHL_ENUM_BITS(CameraEffectsFlags);
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
