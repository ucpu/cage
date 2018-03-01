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
			uint32 cursor;
			uint32 count;
			sint16 posX, posY;
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

	struct updatePositionStruct
	{
		vec2 position, size;
		updatePositionStruct();
	};

	struct guiItemStruct
	{
		guiImpl *const impl;
		entityClass *const entity;

		/*
		_|-------------------------|_ envelope size
		_|--|...................|--|_ margin
		____|-------------------|____ outer size
		____|--|.............|--|____ border
		_______|-------------|_______ inner size
		_______|--|.......|--|_______ padding
		__________|-------|__________ content size
		*/
		vec2 requestedSize; // size (points) as seen by parent (envelope)
		vec2 position, size; // size (points) as seen by parent (envelope)
		vec2 contentPosition, contentSize; // size (points) as seen by children (content)

		guiItemStruct *parent;
		guiItemStruct *prevSibling, *nextSibling;
		guiItemStruct *firstChild, *lastChild;
		sint32 order;

		struct widgetBaseStruct *widget;
		struct layoutBaseStruct *layout;
		struct textItemStruct *text;
		struct imageItemStruct *image;

		guiItemStruct(guiImpl *impl, entityClass *entity);

		void initialize();
		void updateRequestedSize();
		void updateFinalPosition(const updatePositionStruct &update);
		void moveToWindow(bool horizontal, bool vertical);

		void childrenEmit() const;

		void explicitPosition(vec2 &position, vec2 &size) const;
		void explicitPosition(vec2 &size) const;

		void detachParent();
		void attachParent(guiItemStruct *newParent);
	};

	struct widgetBaseStruct
	{
		guiItemStruct *const base;

		widgetStateComponent widgetState;

		widgetBaseStruct(guiItemStruct *base);

		const skinDataStruct &skin() const;

		bool hasFocus() const;
		void makeFocused();

		virtual void initialize() = 0;
		virtual void updateRequestedSize() = 0;
		virtual void updateFinalPosition(const updatePositionStruct &update) = 0;
		virtual void emit() const = 0;

		renderableElementStruct *emitElement(elementTypeEnum element, uint32 mode, const vec4 &margin) const;
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
		virtual void updateRequestedSize() = 0;
		virtual void updateFinalPosition(const updatePositionStruct &update) = 0;
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

		void updateRequestedSize(vec2 &size);

		renderableTextStruct *emit() const;
		renderableTextStruct *emit(vec2 position, vec2 size) const;

		void updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor);
	};

	struct imageItemStruct
	{
		guiItemStruct *const base;

		imageItemStruct(guiItemStruct *base);

		void initialize();
		void updateRequestedSize(vec2 &size);
		renderableImageStruct *emit() const;
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
		uint32 focusName, hoverName;

		memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> itemsArena;
		memoryArena itemsMemory;
		guiItemStruct *root;

		struct graphicDataStruct
		{
			shaderClass *guiShader;
			shaderClass *fontShader;
			shaderClass *imageAnimatedShader;
			shaderClass *imageStaticShader;
			shaderClass *colorPickerShader[3];
			meshClass *guiMesh;
			meshClass *fontMesh;
			meshClass *imageMesh;
			graphicDataStruct();
		} graphicData;

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
		std::atomic<uint32> emitIndexControl;
		std::atomic<uint32> emitIndexDispatch;

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

		void graphicDispatch();
	};

	void positionOffset(vec2 &position, const vec4 &offset);
	void sizeOffset(vec2 &size, const vec4 &offset);
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
