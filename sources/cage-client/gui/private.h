#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#define GUI_GET_COMPONENT(T,N,E) CAGE_JOIN(T, Component) &N = E->value<CAGE_JOIN(T, Component)>(context->components.T);

namespace cage
{
	class guiImpl;

	struct itemCacheStruct
	{
		union
		{
			struct
			{
				float color[3];
				fontClass *font;
				uint32 *translatedText;
				uint32 translatedLength;
				textAlignEnum align;
				float lineSpacing;
				float size;
			};
			struct
			{
				float uvClip[4];
				textureClass *texture;
				uint64 animationStart;
				float animationSpeed;
				float animationOffset;
			};
		};
		const enum itemTypeEnum
		{
			itText,
			itImage,
		} type;
		const uint32 entity;
		itemCacheStruct *next;

		bool verifyItemEntity(guiImpl *context);

		itemCacheStruct(guiImpl *context, itemTypeEnum type, uint32 entity);
	};

	/*
	_|-------------------------|_ pixels envelope size
	_|--|...................|--|_ margin
	____|-------------------|____ ndc outer size
	____|--|.............|--|____ border
	_______|-------------|_______ ndc inner size
	_______|--|.......|--|_______ padding
	__________|-------|__________ pixels content size
	*/

	struct controlCacheStruct
	{
		guiImpl *const context;
		const uint32 entity;
		enum controlModeEnum
		{
			cmHidden = -1,
			cmNormal = 0,
			cmFocus = 1,
			cmHover = 2,
			cmDisabled = 3,
		} controlMode;

		controlCacheStruct *parent;
		controlCacheStruct *nextSibling, *prevSibling;
		controlCacheStruct *firstChild, *lastChild;
		controlCacheStruct *hScrollbar, *vScrollbar;
		itemCacheStruct *firstItem;

		vec4 ndcOuterPositionSize;
		vec4 ndcInnerPositionSize;
		vec2 pixelsEnvelopePosition, pixelsEnvelopeSize; // size as seen by parent
		vec2 pixelsContentPosition, pixelsContentSize; // size as seen by children
		vec2 pixelsRequestedSize; // envelope
		elementTypeEnum elementType;

		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyPress(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyChar(windowClass *win, uint32 key);

		bool verifyControlEntity();

		virtual void graphicPrepare();

		virtual void updateRequestedSize();
		virtual void updatePositionSize();
		virtual void updateContentAndNdcPositionSize();

		bool isPointInside(vec2 pixelsPoint);

		controlCacheStruct(guiImpl *context, uint32 entity);
	};

	namespace
	{
		struct dispatchCacheStruct;
	}

	class guiImpl : public guiClass
	{
	public:
		memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> arenaCache;
		memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> arenaPrepare;
		memoryArena memoryCache;
		memoryArena memoryPrepare;

		elementLayoutStruct::textureUvStruct textureUvs[(uint32)elementTypeEnum::TotalElements];

		windowClass *openglContext;
		assetManagerClass *assetManager;
		holder<entityManagerClass> entityManager;
		holder<uniformBufferClass> elementsGpuBuffer;

		controlCacheStruct *cacheRoot;
		dispatchCacheStruct *prepareCacheFirst, *prepareCacheLast;
		uint32 focusName, hoverName;
		vec2 windowSize;
		vec2 mousePosition;
		mouseButtonsFlags mouseButtons;
		uint64 prepareTime;

		textureClass *skinTexture;
		shaderClass *guiShader;
		shaderClass *fontShader;
		shaderClass *imageAnimatedShader;
		shaderClass *imageStaticShader;
		meshClass *guiMesh;
		meshClass *fontMesh;
		meshClass *imageMesh;

		windowEventListeners listeners;

		void prepareElement(const vec4 &outer, const vec4 &inner, elementTypeEnum element, controlCacheStruct::controlModeEnum mode);
		void prepareText(const itemCacheStruct *text, sint16 pixelsPosX, sint16 pixelsPosY, uint16 pixelsWidth, uint16 pixelsHeight, uint32 cursor = -1);
		void prepareImage(const itemCacheStruct *image, vec4 ndcPosSize);

		vec4 pixelsToNdc(vec2 pixelPosition, vec2 pixelSize);
		real evaluateUnit(real value, unitsModeEnum unit, real defaul = real::Nan);
		vec2 evaluateUnit(vec2 value, unitsModeEnum unit, real defaul = real::Nan);
		vec4 evaluateUnit(vec4 value, unitsModeEnum unit, real defaul = real::Nan);
		void contentToInner(vec2 &pos, vec2 &size, elementTypeEnum element);
		void contentToOuter(vec2 &pos, vec2 &size, elementTypeEnum element);
		void contentToEnvelope(vec2 &pos, vec2 &size, elementTypeEnum element);
		void envelopeToContent(vec2 &pos, vec2 &size, elementTypeEnum element);

		void performCacheCreate();
		void performCacheUpdate();
		void performGraphicPrepare(uint64 time);
		void performGraphicDispatch();

		guiImpl(const guiCreateConfig &config);
		~guiImpl();
	};
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
