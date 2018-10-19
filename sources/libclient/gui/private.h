#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#include <vector>
#include <atomic>

#define GUI_HAS_COMPONENT(T,E) (E)->has(impl->components.T)
#define GUI_REF_COMPONENT(T) hierarchy->entity->value<CAGE_JOIN(T, Component)>(hierarchy->impl->components.T)
#define GUI_GET_COMPONENT(T,N,E) CAGE_JOIN(T, Component) &N = (E)->value<CAGE_JOIN(T, Component)>(impl->components.T);

namespace cage
{
	class guiImpl;
	struct hierarchyItemStruct;

	struct skinDataStruct : public skinConfigStruct
	{
		holder<uniformBufferClass> elementsGpuBuffer;
		textureClass *texture;
	};

	struct renderableBaseStruct
	{
		renderableBaseStruct *next;
		vec2 clipPos, clipSize;

		renderableBaseStruct();

		void setClip(const hierarchyItemStruct *item);

		virtual void render(guiImpl *impl);
	};

	struct renderableElementStruct : public renderableBaseStruct
	{
		struct elementStruct
		{
			vec4 outer;
			vec4 inner;
			uint32 element;
			uint32 mode;
			elementStruct();
		} data;
		const uniformBufferClass *skinBuffer;
		const textureClass *skinTexture;

		renderableElementStruct();

		virtual void render(guiImpl *impl) override;
	};

	struct renderableTextStruct : public renderableBaseStruct
	{
		struct textStruct
		{
			uint32 *glyphs;
			fontClass *font;
			fontClass::formatStruct format;
			vec3 color;
			vec2 pos;
			uint32 cursor;
			uint32 count;
			textStruct();
			void apply(const textFormatComponent &f, guiImpl *impl);
		} data;

		virtual void render(guiImpl *impl) override;
	};

	struct renderableImageStruct : public renderableBaseStruct
	{
		struct imageStruct
		{
			vec4 ndcPos;
			vec4 uvClip;
			vec4 aniTexFrames;
			textureClass *texture;
			imageStruct();
		} data;

		virtual void render(guiImpl *impl) override;
	};

	struct finalPositionStruct
	{
		vec2 renderPos, renderSize;
		vec2 clipPos, clipSize;

		void clip(vec2 p, vec2 s);

		finalPositionStruct();
	};

	struct hierarchyItemStruct
	{
		// size (points) as seen by parent
		vec2 requestedSize;
		vec2 renderPos, renderSize;
		vec2 clipPos, clipSize; // clip pos/size are in same coordinate system as render pos/size, they make a rectangle intersection of parents viewport and our render pos/size

		guiImpl *const impl;
		entityClass *const entity;

		hierarchyItemStruct *parent;
		hierarchyItemStruct *prevSibling, *nextSibling;
		hierarchyItemStruct *firstChild, *lastChild;
		sint32 order; // relative ordering of items with same parent

		struct baseItemStruct *item;
		struct textItemStruct *text;
		struct imageItemStruct *image;

		bool subsidedItem; // prevent use of explicit position

		hierarchyItemStruct(guiImpl *impl, entityClass *entity);

		// called top->down
		void initialize(); // initialize and validate widget, layout, text and image, initialize children
		void findRequestedSize(); // fills in the requestedSize
		void findFinalPosition(const finalPositionStruct &update); // given position and available space in the finalPositionStruct, determine actual position and size
		void childrenEmit() const;

		// helpers
		void moveToWindow(bool horizontal, bool vertical);
		void detachChildren();
		void detachParent();
		void attachParent(hierarchyItemStruct *newParent);

		// debug
		void emitDebug() const;
		void emitDebug(vec2 pos, vec2 size, vec4 color) const;
		void printDebug(uint32 offset) const;
	};

	struct baseItemStruct
	{
		hierarchyItemStruct *const hierarchy;

		baseItemStruct(hierarchyItemStruct *hierarchy);

		virtual void initialize() = 0;
		virtual void findRequestedSize() = 0;
		virtual void findFinalPosition(const finalPositionStruct &update) = 0;
		virtual void emit() const = 0;

		virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point) = 0;
		virtual bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers) = 0;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers) = 0;
		virtual bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers) = 0;
		virtual bool keyChar(uint32 key) = 0;
	};

	struct widgetItemStruct : public baseItemStruct
	{
		widgetStateComponent widgetState;
		const skinDataStruct *skin;

		widgetItemStruct(hierarchyItemStruct *hierarchy);

		uint32 mode(bool hover = true, uint32 focusParts = 1) const;
		uint32 mode(const vec2 &pos, const vec2 &size, uint32 focusParts = 1) const;
		bool hasFocus(uint32 part = 1) const;
		void makeFocused(uint32 part = 1);
		renderableElementStruct *emitElement(elementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const;

		virtual void findFinalPosition(const finalPositionStruct &update) override;

		virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point) override;
		virtual bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct layoutItemStruct : public baseItemStruct
	{
		layoutItemStruct(hierarchyItemStruct *hierarchy);

		virtual void emit() const override;

		virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point) override;
		virtual bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct textItemStruct
	{
		hierarchyItemStruct *const hierarchy;
		renderableTextStruct::textStruct text;
		bool skipInitialize;

		textItemStruct(hierarchyItemStruct *hierarchy);

		void initialize();

		void transcript();
		void transcript(const string &value);
		void transcript(const char *value);

		vec2 findRequestedSize();

		renderableTextStruct *emit(vec2 position, vec2 size) const;

		void updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor);
	};

	struct imageItemStruct
	{
		hierarchyItemStruct *const hierarchy;
		imageComponent image;
		imageFormatComponent format;
		textureClass *texture;
		bool skipInitialize;

		imageItemStruct(hierarchyItemStruct *hierarchy);

		void initialize();

		void assign();
		void assign(const imageComponent &value);
		void apply(const imageFormatComponent &f);

		vec2 findRequestedSize();

		renderableImageStruct *emit(vec2 position, vec2 size) const;
	};

	class guiImpl : public guiClass
	{
	public:
		holder<entityManagerClass> entityManager;
		componentsStruct components;

		windowClass *openglContext;
		assetManagerClass *assetManager;

		pointStruct inputResolution; // last window resolution (pixels)
		pointStruct inputMouse; // last position of mouse (pixels)
		pointStruct outputResolution; // resolution of output texture or screen (pixels)
		vec2 outputSize; // (points)
		vec2 outputMouse; // (points)
		real zoom, retina, pointsScale; // how many pixels per point (1D)
		uint32 focusName; // focused entity name
		uint32 focusParts; // bitmask of focused parts of the single widget (bits 30 and 31 are reserved for scrollbars)
		widgetItemStruct *hover;

		memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> itemsArena;
		memoryArena itemsMemory;
		hierarchyItemStruct *root;

		struct graphicsDataStruct
		{
			shaderClass *debugShader;
			shaderClass *elementShader;
			shaderClass *fontShader;
			shaderClass *imageAnimatedShader;
			shaderClass *imageStaticShader;
			shaderClass *colorPickerShader[3];
			meshClass *debugMesh;
			meshClass *elementMesh;
			meshClass *fontMesh;
			meshClass *imageMesh;
			graphicsDataStruct();
		} graphicsData;

		struct emitDataStruct
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> arena;
			memoryArena memory;
			renderableBaseStruct *first, *last;
			emitDataStruct(const guiCreateConfig &config);
			emitDataStruct(emitDataStruct &&other); // this is not defined, is it? but it is required, is it not?
			~emitDataStruct();
			void flush();
		} emitData[3], *emitControl;
		holder<swapBufferControllerClass> emitController;

		windowEventListeners listeners;
		std::vector<widgetItemStruct*> mouseEventReceivers;
		bool eventsEnabled;

		std::vector<skinDataStruct> skins;

		guiImpl(const guiCreateConfig &config);
		~guiImpl();

		void scaling();

		bool eventPoint(const pointStruct &ptIn, vec2 &ptOut);
		vec4 pointsToNdc(vec2 position, vec2 size);

		void graphicsDispatch();

		uint32 entityWidgetsCount(entityClass *e);
		uint32 entityLayoutsCount(entityClass *e);
	};

	void offsetPosition(vec2 &position, const vec4 &offset);
	void offsetSize(vec2 &size, const vec4 &offset);
	void offset(vec2 &position, vec2 &size, const vec4 &offset);
	bool pointInside(vec2 pos, vec2 size, vec2 point);

	hierarchyItemStruct *subsideItem(hierarchyItemStruct *item);
	void ensureItemHasLayout(hierarchyItemStruct *item); // if the item's entity does not have any layout, subside the item and add default layouter to it
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
