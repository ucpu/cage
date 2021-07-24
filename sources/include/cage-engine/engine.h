#ifndef guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
#define guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D

#include <cage-core/events.h>

#include "screenSpaceEffectsProperties.h"

namespace cage
{
	enum class CameraEffectsFlags : uint32
	{
		None = 0,
		AmbientOcclusion = 1 << 0,
		DepthOfField = 1 << 1,
		MotionBlur = 1 << 2,
		Bloom = 1 << 3,
		EyeAdaptation = 1 << 4,
		ToneMapping = 1 << 5,
		GammaCorrection = 1 << 6,
		AntiAliasing = 1 << 7,
		Default = AmbientOcclusion | MotionBlur | Bloom | ToneMapping | GammaCorrection | AntiAliasing,
	};

	struct CAGE_ENGINE_API CameraSsao : public ScreenSpaceAmbientOcclusion
	{};

	struct CAGE_ENGINE_API CameraMotionBlur : public ScreenSpaceMotionBlur
	{};

	struct CAGE_ENGINE_API CameraBloom : public ScreenSpaceBloom
	{};

	struct CAGE_ENGINE_API CameraEyeAdaptation : public ScreenSpaceEyeAdaptation
	{};

	struct CAGE_ENGINE_API CameraTonemap : public ScreenSpaceTonemap
	{};

	struct CAGE_ENGINE_API CameraDepthOfField : public ScreenSpaceDepthOfField
	{};

	struct CAGE_ENGINE_API CameraEffects
	{
		CameraSsao ssao;
		CameraMotionBlur motionBlur;
		CameraBloom bloom;
		CameraEyeAdaptation eyeAdaptation;
		CameraTonemap tonemap;
		CameraDepthOfField depthOfField;
		real gamma = 2.2;
		CameraEffectsFlags effects = CameraEffectsFlags::None;
	};

	struct CAGE_ENGINE_API TransformComponent : public transform
	{
		using transform::transform;
		using transform::operator =;
	};

	struct CAGE_ENGINE_API RenderComponent
	{
		vec3 color = vec3::Nan();
		real intensity = real::Nan();
		real opacity = real::Nan();
		uint32 object = 0;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API TextureAnimationComponent
	{
		uint64 startTime = 0;
		real speed = real::Nan();
		real offset = real::Nan();
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		real speed = real::Nan();
		real offset = real::Nan();
	};

	enum class LightTypeEnum : uint32
	{
		Directional,
		Point,
		Spot,
	};

	struct CAGE_ENGINE_API LightComponent
	{
		vec3 attenuation = vec3(0, 0, 1); // constant, linear, quadratic
		vec3 color = vec3(1);
		real intensity = 1;
		rads spotAngle = degs(40);
		real spotExponent = 80;
		uint32 sceneMask = 1;
		LightTypeEnum lightType = LightTypeEnum::Point;
	};

	struct CAGE_ENGINE_API ShadowmapComponent
	{
		// directional: width, height, depth
		// spot: near, far, unused
		vec3 worldSize = vec3();
		real normalOffsetScale = 0.2;
		uint32 resolution = 256;
	};

	struct CAGE_ENGINE_API TextComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		vec3 color = vec3(1);
		real intensity = 1;
		// real opacity; // todo
		uint32 assetName = 0;
		uint32 textName = 0;
		uint32 font = 0;
		uint32 sceneMask = 1;
	};

	enum class CameraClearFlags : uint32
	{
		None = 0,
		Depth = 1 << 0,
		Color = 1 << 1,
		Stencil = 1 << 2,
	};

	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};

	struct CAGE_ENGINE_API CameraComponent : public CameraEffects
	{
		vec3 ambientColor = vec3();
		vec3 ambientDirectionalColor = vec3(); // fake forward light affected by ssao
		union CameraUnion
		{
			vec2 orthographicSize;
			rads perspectiveFov = degs(60);
			CameraUnion() {}
		} camera;
		Texture *target = nullptr;
		real ambientIntensity = 1;
		real ambientDirectionalIntensity = 1;
		real near = 1, far = 100;
		real zeroParallaxDistance = 10;
		real eyeSeparation = 0.3;
		sint32 cameraOrder = 0;
		uint32 sceneMask = 1;
		CameraClearFlags clear = CameraClearFlags::Depth | CameraClearFlags::Color;
		CameraTypeEnum cameraType = CameraTypeEnum::Perspective;
	};

	struct CAGE_ENGINE_API SoundComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		uint32 sceneMask = 1;
		real gain = 1; // linear amplitude multiplier
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		uint32 sceneMask = 1;
		real rolloffFactor = 1; // distance multiplier
		real gain = 1; // linear amplitude multiplier
	};

	struct CAGE_ENGINE_API EngineControlThread
	{
		EventDispatcher<bool()> initialize; // called once from engineStart()
		EventDispatcher<bool()> finalize; // called once from engineStart()
		EventDispatcher<bool()> update; // periodically called from engineStart()
		EventDispatcher<bool()> unload; // periodically called from engineFinalize()
		Scheduler *scheduler();
		uint64 updatePeriod() const;
		void updatePeriod(uint64 p);
		uint64 inputPeriod() const;
		void inputPeriod(uint64 p);
	};
	CAGE_ENGINE_API EngineControlThread &controlThread();

	struct CAGE_ENGINE_API EngineGraphicsDispatchThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> dispatch;
		EventDispatcher<bool()> swap;
		static constexpr uint32 threadIndex = 1;
	};
	CAGE_ENGINE_API EngineGraphicsDispatchThread &graphicsDispatchThread();

	struct CAGE_ENGINE_API EngineGraphicsPrepareThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> prepare;
		static constexpr uint32 threadIndex = 3;
	};
	CAGE_ENGINE_API EngineGraphicsPrepareThread &graphicsPrepareThread();

	struct CAGE_ENGINE_API EngineSoundThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> sound;
		Scheduler *scheduler();
		uint64 updatePeriod() const;
		void updatePeriod(uint64 p);
		static constexpr uint32 threadIndex = 4;
	};
	CAGE_ENGINE_API EngineSoundThread &soundThread();

	struct CAGE_ENGINE_API EngineCreateConfig
	{
		AssetManagerCreateConfig *assets = nullptr;
		GuiCreateConfig *gui = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
	};

	CAGE_ENGINE_API void engineInitialize(const EngineCreateConfig &config);
	CAGE_ENGINE_API void engineStart();
	CAGE_ENGINE_API void engineStop();
	CAGE_ENGINE_API void engineFinalize();

	CAGE_ENGINE_API AssetManager *engineAssets();
	CAGE_ENGINE_API EntityManager *engineEntities();
	CAGE_ENGINE_API Window *engineWindow();
	CAGE_ENGINE_API Gui *engineGui();
	CAGE_ENGINE_API Speaker *engineSpeaker();
	CAGE_ENGINE_API VoicesMixer *engineMasterMixer();
	CAGE_ENGINE_API VoicesMixer *engineEffectsMixer();
	CAGE_ENGINE_API VoicesMixer *engineGuiMixer();
	CAGE_ENGINE_API uint64 engineControlTime();
}

// deprecated
#define CAGE_COMPONENT_ENGINE(T,C,E) ::cage::T##Component &C = (E)->value<::cage::T##Component>();

// deprecated
#define CAGE_COMPONENT_GUI(T,C,E) ::cage::Gui##T##Component &C = (E)->value<::cage::Gui##T##Component>();

#endif // guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
