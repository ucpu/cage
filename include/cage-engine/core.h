#ifndef guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
#define guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632

#include <cage-core/core.h>
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
	// todo MatrixComponent -> allow for skew, non-uniform scale, etc.
	struct RenderComponent;
	struct TextureAnimationComponent;
	struct SkeletalAnimationComponent;
	// todo SkeletonPoseComponent -> allow the application to compute the pose itself
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

	class Font;
	class FrameBuffer;
	struct GraphicsError;
	class Mesh;
	class RenderObject;
	class ShaderProgram;
	class SkeletalAnimation;
	class SkeletonRig;
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

	class MixingBus;
	struct MixingFilterApi;
	class MixingFilter;
	class VolumeFilter;
	struct SoundError;
	class SoundContext;
	struct SoundContextCreateConfig;
	class SoundSource;
	struct SoundInterleavedBuffer;
	struct SoundDataBuffer;
	class Speaker;
	struct SpeakerCreateConfig;

	enum class TextureFlags : uint32;
	struct TextureHeader;
	enum class MeshDataFlags : uint32;
	struct MeshHeader;
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

	// enum declarations

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
	GCHL_ENUM_BITS(MeshDataFlags);
	GCHL_ENUM_BITS(FontFlags);
	GCHL_ENUM_BITS(SoundFlags);
	GCHL_ENUM_BITS(EngineProfilingStatsFlags);
	GCHL_ENUM_BITS(WindowFlags);
	GCHL_ENUM_BITS(MeshRenderFlags);
	GCHL_ENUM_BITS(ModifiersFlags);
	GCHL_ENUM_BITS(MouseButtonsFlags);

	// constants

	static constexpr uint32 MaxTexturesCountPerMaterial = 4;

	// ivec2

	struct CAGE_ENGINE_API ivec2
	{
		ivec2()
		{}

		explicit ivec2(sint32 x, sint32 y) : x(x), y(y)
		{}

		sint32 operator [] (uint32 index) const
		{
			switch (index)
			{
			case 0: return x;
			case 1: return y;
			default: CAGE_THROW_CRITICAL(Exception, "index out of range");
			}
		}

		sint32 &operator [] (uint32 index)
		{
			switch (index)
			{
			case 0: return x;
			case 1: return y;
			default: CAGE_THROW_CRITICAL(Exception, "index out of range");
			}
		}

		sint32 x = 0;
		sint32 y = 0;
	};

	inline bool operator == (const ivec2 &l, const ivec2 &r) { return l[0] == r[0] && l[1] == r[1]; };
	inline bool operator != (const ivec2 &l, const ivec2 &r) { return !(l == r); };

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 operator OPERATOR (const ivec2 &r) { return ivec2(OPERATOR r[0], OPERATOR r[1]); }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 operator OPERATOR (const ivec2 &l, const ivec2 &r) { return ivec2(l[0] OPERATOR r[0], l[1] OPERATOR r[1]); } \
	inline ivec2 operator OPERATOR (const ivec2 &l, const sint32 &r) { return ivec2(l[0] OPERATOR r, l[1] OPERATOR r); }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE

#define GCHL_GENERATE(OPERATOR) \
	inline ivec2 &operator OPERATOR##= (ivec2 &l, const ivec2 &r) { return l = l OPERATOR r; } \
	inline ivec2 &operator OPERATOR##= (ivec2 &l, const sint32 &r) { return l = l OPERATOR r; }
	GCHL_GENERATE(+);
	GCHL_GENERATE(-);
	GCHL_GENERATE(*);
	GCHL_GENERATE(/);
#undef GCHL_GENERATE
}

#endif // guard_core_h_4F3464CEC4474C118E1CEA1EF9DF7632
