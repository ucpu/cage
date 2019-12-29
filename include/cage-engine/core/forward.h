namespace cage
{
	struct GraphicsError;
	struct WindowEventListeners;
	class Window;
	class SkeletalAnimation;
	class Font;
	class FrameBuffer;
	class Mesh;
	class RenderObject;
	class Texture;
	class ScreenDevice;
	class ScreenList;
	class ShaderProgram;
	class SkeletonRig;
	class UniformBuffer;

	struct SoundError;
	struct SoundContextCreateConfig;
	struct SpeakerCreateConfig;
	struct MixingFilterApi;
	struct SoundInterleavedBuffer;
	struct SoundDataBuffer;
	class SoundContext;
	class Speaker;
	class MixingBus;
	class MixingFilter;
	class VolumeFilter;
	class SoundSource;
	class SpeakerDevice;
	class SpeakerList;

	struct GuiCreateConfig;
	class Gui;
	struct ParentComponent;
	struct ImageComponent;
	struct ImageFormatComponent;
	struct TextComponent;
	struct TextFormatComponent;
	struct SelectionComponent;
	struct TooltipComponent;
	struct WidgetStateComponent;
	struct SelectedItemComponent;
	struct ScrollbarsComponent;
	struct ExplicitSizeComponent;
	struct EventComponent;
	struct LayoutLineComponent;
	struct LayoutTableComponent;
	struct LayoutSplitterComponent;
	struct LabelComponent;
	struct ButtonComponent;
	struct InputComponent;
	struct TextAreaComponent;
	struct CheckBoxComponent;
	struct RadioBoxComponent;
	struct ComboBoxComponent;
	struct ListBoxComponent;
	struct ProgressBarComponent;
	struct SliderBarComponent;
	struct ColorPickerComponent;
	struct PanelComponent;
	struct SpoilerComponent;

	struct EngineCreateConfig;
	struct TransformComponent;
	// todo MatrixComponent -> allow for skew, non-uniform scale, etc.
	struct RenderComponent;
	struct TextureAnimationComponent;
	struct SkeletalAnimationComponent;
	// todo SkeletonPoseComponent;
	struct LightComponent;
	struct ShadowmapComponent;
	struct RenderTextComponent;
	struct CameraComponent;
	struct SoundComponent;
	struct ListenerComponent;

	class EngineProfiling;
	class FpsCamera;
	struct FullscreenSwitcherCreateConfig;
	class FullscreenSwitcher;
}
