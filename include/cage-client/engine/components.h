namespace cage
{
	struct CAGE_API transformComponent : public transform
	{
		static componentClass *component;
		static componentClass *componentHistory;
		transformComponent(const transform &t = transform());
	};

	struct CAGE_API renderComponent
	{
		static componentClass *component;
		vec3 color;
		uint32 object;
		uint32 renderMask;
		renderComponent();
	};

	struct CAGE_API animatedTextureComponent
	{
		static componentClass *component;
		uint64 startTime;
		real speed;
		real offset;
		animatedTextureComponent();
	};

	struct CAGE_API animatedSkeletonComponent
	{
		static componentClass *component;
		uint64 startTime;
		uint32 name;
		real speed;
		real offset;
		animatedSkeletonComponent();
	};

	struct CAGE_API configuredSkeletonComponent
	{
		static componentClass *component;
		static componentClass *componentHistory;
		mat4 configuration[MaxBonesCount];
		configuredSkeletonComponent();
	};

	struct CAGE_API lightComponent
	{
		static componentClass *component;
		vec3 color;
		vec3 attenuation; // constant, linear, quadratic
		rads spotAngle;
		real spotExponent;
		lightTypeEnum lightType;
		lightComponent();
	};

	struct CAGE_API shadowmapComponent
	{
		static componentClass *component;
		vec3 worldRadius;
		uint32 resolution;
		shadowmapComponent();
	};

	struct CAGE_API cameraComponent
	{
		static componentClass *component;
		vec3 ambientLight;
		vec2 viewportOrigin;
		vec2 viewportSize;
		vec2 orthographicSize;
		textureClass *target;
		rads perspectiveFov;
		real near, far;
		real zeroParallaxDistance;
		real eyeSeparation;
		sint32 cameraOrder;
		uint32 renderMask;
		cameraClearFlags clear;
		cameraTypeEnum cameraType;
		cameraComponent();
	};

	struct CAGE_API voiceComponent
	{
		static componentClass *component;
		uint64 startTime;
		busClass *input;
		uint32 name;
		uint32 renderMask;
		voiceComponent();
	};

	struct CAGE_API listenerComponent
	{
		static componentClass *component;
		vec3 attenuation; // constant, linear, quadratic
		busClass *output;
		uint32 renderMask;
		real speedOfSound;
		bool dopplerEffect;
		listenerComponent();
	};
}
