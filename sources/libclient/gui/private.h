#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#include <vector>
#include <atomic>

#define GUI_HAS_COMPONENT(T,E) (E)->hasComponent(impl->components.T)
#define GUI_REF_COMPONENT(T) base->entity->value<CAGE_JOIN(T, Component)>(base->impl->components.T)
#define GUI_GET_COMPONENT(T,N,E) CAGE_JOIN(T, Component) &N = (E)->value<CAGE_JOIN(T, Component)>(impl->components.T);

namespace cage
{
	class guiImpl;

	struct skinDataStruct : public skinConfigStruct
	{
		holder<uniformBufferClass> elementsGpuBuffer;
		textureClass *texture;
	};

	struct renderableBaseStruct
	{
		renderableBaseStruct *next;
		renderableBaseStruct();
		virtual void render(guiImpl *impl);
	};

	struct renderableDebugStruct : public renderableBaseStruct
	{
		struct elementStruct
		{
			vec4 position;
			vec4 color;
		} data;

		virtual void render(guiImpl *impl) override;
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
		vec2 position, size;
		finalPositionStruct();
	};

	struct guiItemStruct
	{
		// size (points) as seen by parent (envelope)
		vec2 requestedSize;
		vec2 position, size;

		guiImpl *const impl;
		entityClass *const entity;

		guiItemStruct *parent;
		guiItemStruct *prevSibling, *nextSibling;
		guiItemStruct *firstChild, *lastChild;

		struct widgetBaseStruct *widget;
		struct layoutBaseStruct *layout;
		struct textItemStruct *text;
		struct imageItemStruct *image;

		sint32 order; // relative ordering of items with same parent

		guiItemStruct(guiImpl *impl, entityClass *entity);

		// called top->down
		void initialize(); // initialize and validate widget, layout, text and image, initialize children
		void findRequestedSize(); // fills int the requestedSize
		void findFinalPosition(const finalPositionStruct &update); // given position and available space in the finalPositionStruct, determine actual position, size, contentPosition and contentSize

		// base helpers for derived classes
		void checkExplicitPosition(vec2 &pos, vec2 &size) const;
		void checkExplicitPosition(vec2 &size) const;
		void moveToWindow(bool horizontal, bool vertical);

		// parenting helpers
		void detachChildren();
		void detachParent();
		void attachParent(guiItemStruct *newParent);

		// called top->down
		void childrenEmit() const;
		void emitDebug() const;
		void emitDebug(vec2 pos, vec2 size, vec4 color) const;
	};

	struct widgetBaseStruct
	{
		guiItemStruct *const base;

		widgetStateComponent widgetState;

		widgetBaseStruct(guiItemStruct *base);

		const skinDataStruct &skin() const;
		uint32 mode(bool hover = true) const;
		uint32 mode(const vec2 &pos, const vec2 &size) const;
		bool hasFocus() const;
		void makeFocused();

		virtual void initialize() = 0;
		virtual void findRequestedSize() = 0;
		virtual void findFinalPosition(const finalPositionStruct &update);
		virtual void emit() const = 0;

		renderableElementStruct *emitElement(elementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const;

		virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point);
		virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point);
		virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point);
		virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point);
		virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point);
		virtual bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		virtual bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		virtual bool keyChar(uint32 key);
	};

	struct layoutBaseStruct
	{
		guiItemStruct *const base;

		layoutBaseStruct(guiItemStruct *base);

		virtual void initialize() = 0;
		virtual void findRequestedSize() = 0;
		virtual void findFinalPosition(const finalPositionStruct &update) = 0;
	};

	struct textItemStruct
	{
		guiItemStruct *const base;
		renderableTextStruct::textStruct text;
		bool skipInitialize;

		textItemStruct(guiItemStruct *base);

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
		guiItemStruct *const base;
		imageComponent image;
		imageFormatComponent format;
		textureClass *texture;
		bool skipInitialize;

		imageItemStruct(guiItemStruct *base);

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
		uint32 focusName;
		widgetBaseStruct *hover;

		memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> itemsArena;
		memoryArena itemsMemory;
		guiItemStruct *root;

		struct graphicDataStruct
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
			graphicDataStruct();
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
		std::vector<widgetBaseStruct*> mouseEventReceivers;
		bool eventsEnabled;

		std::vector<skinDataStruct> skins;

		guiImpl(const guiCreateConfig &config);
		~guiImpl();

		void scaling();

		bool eventPoint(const pointStruct &ptIn, vec2 &ptOut);
		vec4 pointsToNdc(vec2 position, vec2 size);
		real eval(real val, unitEnum unit, real defaul = real::Nan);
		template<uint32 N> typename vecN<N>::type eval(const valuesStruct<N> &v, real defaul = real::Nan)
		{
			typename vecN<N>::type result;
			for (uint32 i = 0; i < N; i++)
				result[i] = eval(v.values[i], v.units[i], defaul);
			return result;
		}
		template<uint32 M, uint32 N> typename vecN<N>::type eval(const valuesStruct<N> &v, typename vecN<M>::type defaul)
		{
			typename vecN<N>::type result;
			for (uint32 i = 0; i < N; i++)
				result[i] = eval(v.values[i], v.units[i], defaul[i % M]);
			return result;
		}

		void graphicsDispatch();
	};

	void offsetPosition(vec2 &position, const vec4 &offset);
	void offsetSize(vec2 &size, const vec4 &offset);
	void offset(vec2 &position, vec2 &size, const vec4 &offset);
	bool pointInside(vec2 pos, vec2 size, vec2 point);
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
