namespace cage
{
	struct graphicsError;
	struct windowEventListeners;
	class windowHandle;
	class skeletalAnimation;
	class fontFace;
	class frameBuffer;
	class renderMesh;
	class renderObject;
	class renderTexture;
	class screenDevice;
	class screenList;
	class shaderProgram;
	class skeletonRig;
	class uniformBuffer;

	struct soundError;
	struct soundContextCreateConfig;
	struct speakerOutputCreateConfig;
	struct mixingFilterApi;
	struct soundInterleavedBufferStruct;
	struct soundDataBufferStruct;
	class soundContext;
	class speakerOutput;
	class mixingBus;
	class mixingFilter;
	class volumeFilter;
	class soundSource;
	class speakerDevice;
	class speakerList;

	struct guiManagerCreateConfig;
	class guiManager;

	struct engineCreateConfig;
	struct transformComponent;
	// todo matrixComponent -> allow for skew, non-uniform scale, etc.
	struct renderComponent;
	struct textureAnimationComponent;
	struct skeletalAnimationComponent;
	// todo skeletonPoseComponent;
	struct lightComponent;
	struct shadowmapComponent;
	struct cameraComponent;
	struct voiceComponent;
	struct listenerComponent;

	class engineProfiling;
	class fpsCamera;
	struct fullscreenSwitcherCreateConfig;
	class fullscreenSwitcher;
}
