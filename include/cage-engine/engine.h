#ifndef guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
#define guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D

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
		real worldRadius;
		real bias;
		real power;
		real strength;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount;
		uint32 blurPasses;
		CameraSsao();
	};

	struct CAGE_ENGINE_API CameraMotionBlur
	{
		// todo
	};

	struct CAGE_ENGINE_API CameraBloom
	{
		uint32 blurPasses;
		real threshold;
		CameraBloom();
	};

	struct CAGE_ENGINE_API CameraEyeAdaptation
	{
		real key;
		real strength;
		real darkerSpeed;
		real lighterSpeed;
		CameraEyeAdaptation();
	};

	struct CAGE_ENGINE_API CameraTonemap
	{
		real shoulderStrength;
		real linearStrength;
		real linearAngle;
		real toeStrength;
		real toeNumerator;
		real toeDenominator;
		real white;
		CameraTonemap();
	};

	struct CAGE_ENGINE_API CameraEffects
	{
		CameraSsao ssao;
		CameraMotionBlur motionBlur;
		CameraBloom bloom;
		CameraEyeAdaptation eyeAdaptation;
		CameraTonemap tonemap;
		real gamma;
		CameraEffectsFlags effects;
		CameraEffects();
	};

	struct CAGE_ENGINE_API TransformComponent : public transform
	{
		static EntityComponent *component;
		static EntityComponent *componentHistory;
		TransformComponent(const transform &t = transform());
	};

	struct CAGE_ENGINE_API RenderComponent
	{
		static EntityComponent *component;
		vec3 color;
		real opacity;
		uint32 object;
		uint32 sceneMask;
		RenderComponent();
	};

	struct CAGE_ENGINE_API TextureAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		real speed;
		real offset;
		TextureAnimationComponent();
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		uint32 name;
		real speed;
		real offset;
		SkeletalAnimationComponent();
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
		vec3 color;
		vec3 attenuation; // constant, linear, quadratic
		rads spotAngle;
		real spotExponent;
		uint32 sceneMask;
		LightTypeEnum lightType;
		LightComponent();
	};

	struct CAGE_ENGINE_API ShadowmapComponent
	{
		static EntityComponent *component;
		// directional: width, height, depth
		// spot: near, far, unused
		vec3 worldSize;
		uint32 resolution;
		uint32 sceneMask;
		ShadowmapComponent();
	};

	struct CAGE_ENGINE_API TextComponent
	{
		static EntityComponent *component;
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		vec3 color;
		//real opacity;
		uint32 assetName;
		uint32 textName;
		uint32 font;
		uint32 sceneMask;
		TextComponent();
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
		vec3 ambientLight;
		vec3 ambientDirectionalLight; // fake forward light affected by ssao
		vec2 viewportOrigin; // [0..1]
		vec2 viewportSize; // [0..1]
		union CameraUnion
		{
			vec2 orthographicSize;
			rads perspectiveFov;
			CameraUnion();
		} camera;
		Texture *target;
		real near, far;
		real zeroParallaxDistance;
		real eyeSeparation;
		sint32 cameraOrder;
		uint32 sceneMask;
		CameraClearFlags clear;
		CameraTypeEnum cameraType;
		CameraComponent();
	};

	struct CAGE_ENGINE_API SoundComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		MixingBus *input;
		uint32 name;
		uint32 sceneMask;
		SoundComponent();
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		static EntityComponent *component;
		vec3 attenuation; // constant, linear, quadratic
		MixingBus *output;
		uint32 sceneMask;
		real speedOfSound;
		bool dopplerEffect;
		ListenerComponent();
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
		EngineControlThread();
	};
	CAGE_ENGINE_API EngineControlThread &controlThread();

	struct CAGE_ENGINE_API EngineGraphicsDispatchThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> dispatch;
		EventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
		EngineGraphicsDispatchThread();
	};
	CAGE_ENGINE_API EngineGraphicsDispatchThread &graphicsDispatchThread();

	struct CAGE_ENGINE_API EngineGraphicsPrepareThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> prepare;
		StereoModeEnum stereoMode;
		static const uint32 threadIndex = 3;
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
		static const uint32 threadIndex = 4;
		EngineSoundThread();
	};
	CAGE_ENGINE_API EngineSoundThread &soundThread();

	struct CAGE_ENGINE_API EngineCreateConfig
	{
		EntityManagerCreateConfig *entities;
		AssetManagerCreateConfig *assets;
		GuiCreateConfig *gui;
		SoundContextCreateConfig *soundContext;
		SpeakerCreateConfig *speaker;
		EngineCreateConfig();
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

#define CAGE_COMPONENT_ENGINE(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define CAGE_COMPONENT_GUI(T,C,E) ::cage::CAGE_JOIN(Gui, CAGE_JOIN(T, Component)) &C = (E)->value<::cage::CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(::cage::engineGui()->components().T);

#endif // guard_engine_h_73772AB49E6A4E6DB7F99E0D0151468D
