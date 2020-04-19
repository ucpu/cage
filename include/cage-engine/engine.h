#ifndef guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
#define guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D

#include <cage-core/events.h>

#include "core.h"

namespace cage
{
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

	struct CAGE_ENGINE_API CameraSsao
	{
		real worldRadius = 0.5;
		real bias = 0.03;
		real power = 1.3;
		real strength = 3;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount = 16;
		uint32 blurPasses = 3;
	};

	struct CAGE_ENGINE_API CameraMotionBlur
	{
		// todo
	};

	struct CAGE_ENGINE_API CameraBloom
	{
		uint32 blurPasses = 5;
		real threshold = 1;
	};

	struct CAGE_ENGINE_API CameraEyeAdaptation
	{
		real key = 0.5;
		real strength = 1.0;
		real darkerSpeed = 0.1;
		real lighterSpeed = 1;
		// darker should take at least 5 times longer
	};

	struct CAGE_ENGINE_API CameraTonemap
	{
		real shoulderStrength = 0.22;
		real linearStrength = 0.3;
		real linearAngle = 0.1;
		real toeStrength = 0.2;
		real toeNumerator = 0.01;
		real toeDenominator = 0.3;
		real white = 11.2;
	};

	struct CAGE_ENGINE_API CameraEffects
	{
		CameraSsao ssao;
		CameraMotionBlur motionBlur;
		CameraBloom bloom;
		CameraEyeAdaptation eyeAdaptation;
		CameraTonemap tonemap;
		real gamma = 2.2;
		CameraEffectsFlags effects = CameraEffectsFlags::None;
	};

	struct CAGE_ENGINE_API TransformComponent : public transform
	{
		static EntityComponent *component;
		static EntityComponent *componentHistory;
		using transform::transform;
	};

	struct CAGE_ENGINE_API RenderComponent
	{
		static EntityComponent *component;
		vec3 color = vec3::Nan();
		real intensity = real::Nan();
		real opacity = real::Nan();
		uint32 object = 0;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API TextureAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime = 0;
		real speed = real::Nan();
		real offset = real::Nan();
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		static EntityComponent *component;
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
		static EntityComponent *component;
		vec3 color = vec3(1);
		vec3 attenuation = vec3(0, 0, 1); // constant, linear, quadratic
		real intensity = 1;
		rads spotAngle = degs(40);
		real spotExponent = 80;
		uint32 sceneMask = 1;
		LightTypeEnum lightType = LightTypeEnum::Point;
	};

	struct CAGE_ENGINE_API ShadowmapComponent
	{
		static EntityComponent *component;
		// directional: width, height, depth
		// spot: near, far, unused
		vec3 worldSize = vec3();
		uint32 resolution = 256;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API TextComponent
	{
		static EntityComponent *component;
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		vec3 color = vec3(1);
		// real opacity; // todo
		real intensity = 1;
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
		static EntityComponent *component;
		vec3 ambientColor = vec3();
		vec3 ambientDirectionalColor = vec3(); // fake forward light affected by ssao
		vec2 viewportOrigin = vec2(0);
		vec2 viewportSize = vec2(1);
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
		static EntityComponent *component;
		uint64 startTime = 0;
		MixingBus *input = nullptr;
		uint32 name = 0;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		static EntityComponent *component;
		vec3 attenuation = vec3(); // constant, linear, quadratic
		MixingBus *output = nullptr;
		uint32 sceneMask = 1;
		real speedOfSound = 343.3;
		bool dopplerEffect = false;
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
		StereoModeEnum stereoMode;
		static constexpr uint32 threadIndex = 3;
		EngineGraphicsPrepareThread();
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
		EntityManagerCreateConfig *entities = nullptr;
		AssetManagerCreateConfig *assets = nullptr;
		GuiCreateConfig *gui = nullptr;
		SoundContextCreateConfig *soundContext = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
	};

	CAGE_ENGINE_API void engineInitialize(const EngineCreateConfig &config);
	CAGE_ENGINE_API void engineStart();
	CAGE_ENGINE_API void engineStop();
	CAGE_ENGINE_API void engineFinalize();

	CAGE_ENGINE_API SoundContext *engineSound();
	CAGE_ENGINE_API AssetManager *engineAssets();
	CAGE_ENGINE_API EntityManager *engineEntities();
	CAGE_ENGINE_API Window *engineWindow();
	CAGE_ENGINE_API Gui *engineGui();
	CAGE_ENGINE_API Speaker *engineSpeaker();
	CAGE_ENGINE_API MixingBus *engineMasterMixer();
	CAGE_ENGINE_API MixingBus *engineMusicMixer();
	CAGE_ENGINE_API MixingBus *engineEffectsMixer();
	CAGE_ENGINE_API MixingBus *engineGuiMixer();
	CAGE_ENGINE_API uint64 engineControlTime();
}

#define CAGE_COMPONENT_ENGINE(T,C,E) ::cage::T##Component &C = (E)->value<::cage::T##Component>(::cage::T##Component::component);
#define CAGE_COMPONENT_GUI(T,C,E) ::cage::Gui##T##Component &C = (E)->value<::cage::Gui##T##Component>(::cage::engineGui()->components().T);

#endif // guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
