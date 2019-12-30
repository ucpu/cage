namespace cage
{
	struct CAGE_API TransformComponent : public transform
	{
		static EntityComponent *component;
		static EntityComponent *componentHistory;
		TransformComponent(const transform &t = transform());
	};

	struct CAGE_API RenderComponent
	{
		static EntityComponent *component;
		vec3 color;
		real opacity;
		uint32 object;
		uint32 sceneMask;
		RenderComponent();
	};

	struct CAGE_API TextureAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		real speed;
		real offset;
		TextureAnimationComponent();
	};

	struct CAGE_API SkeletalAnimationComponent
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

	struct CAGE_API LightComponent
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

	struct CAGE_API ShadowmapComponent
	{
		static EntityComponent *component;
		// directional: width, height, depth
		// spot: near, far, unused
		vec3 worldSize;
		uint32 resolution;
		uint32 sceneMask;
		ShadowmapComponent();
	};

	struct CAGE_API RenderTextComponent
	{
		static EntityComponent *component;
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		vec3 color;
		//real opacity;
		uint32 assetName;
		uint32 textName;
		uint32 font;
		uint32 sceneMask;
		RenderTextComponent();
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

	struct CAGE_API CameraComponent : public CameraEffects
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

	struct CAGE_API SoundComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		MixingBus *input;
		uint32 name;
		uint32 sceneMask;
		SoundComponent();
	};

	struct CAGE_API ListenerComponent
	{
		static EntityComponent *component;
		vec3 attenuation; // constant, linear, quadratic
		MixingBus *output;
		uint32 sceneMask;
		real speedOfSound;
		bool dopplerEffect;
		ListenerComponent();
	};
}
