namespace cage
{
	struct CAGE_API transformComponent : public transform
	{
		static EntityComponent *component;
		static EntityComponent *componentHistory;
		transformComponent(const transform &t = transform());
	};

	struct CAGE_API renderComponent
	{
		static EntityComponent *component;
		vec3 color;
		real opacity;
		uint32 object;
		uint32 sceneMask;
		renderComponent();
	};

	struct CAGE_API textureAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		real speed;
		real offset;
		textureAnimationComponent();
	};

	struct CAGE_API skeletalAnimationComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		uint32 name;
		real speed;
		real offset;
		skeletalAnimationComponent();
	};

	struct CAGE_API lightComponent
	{
		static EntityComponent *component;
		vec3 color;
		vec3 attenuation; // constant, linear, quadratic
		rads spotAngle;
		real spotExponent;
		uint32 sceneMask;
		lightTypeEnum lightType;
		lightComponent();
	};

	struct CAGE_API shadowmapComponent
	{
		static EntityComponent *component;
		// directional: width, height, depth
		// spot: near, far, unused
		vec3 worldSize;
		uint32 resolution;
		uint32 sceneMask;
		shadowmapComponent();
	};

	struct CAGE_API renderTextComponent
	{
		static EntityComponent *component;
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		vec3 color;
		//real opacity;
		uint32 assetName;
		uint32 textName;
		uint32 font;
		uint32 sceneMask;
		renderTextComponent();
	};

	struct CAGE_API cameraComponent : public cameraEffects
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
		renderTexture *target;
		real near, far;
		real zeroParallaxDistance;
		real eyeSeparation;
		sint32 cameraOrder;
		uint32 sceneMask;
		cameraClearFlags clear;
		cameraTypeEnum cameraType;
		cameraComponent();
	};

	struct CAGE_API voiceComponent
	{
		static EntityComponent *component;
		uint64 startTime;
		mixingBus *input;
		uint32 name;
		uint32 sceneMask;
		voiceComponent();
	};

	struct CAGE_API listenerComponent
	{
		static EntityComponent *component;
		vec3 attenuation; // constant, linear, quadratic
		mixingBus *output;
		uint32 sceneMask;
		real speedOfSound;
		bool dopplerEffect;
		listenerComponent();
	};
}
