#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#include <cage-core/entities.h>
#include <cage-core/macros.h>

#include <cage-engine/gui.h>
#include <cage-engine/guiSkins.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>

#include <vector>
#include <atomic>

#define GCHL_GUI_COMMON_COMPONENTS Parent, Image, ImageFormat, Text, TextFormat, Selection, Tooltip, WidgetState, SelectedItem, Scrollbars, ExplicitSize, Event
#define GCHL_GUI_WIDGET_COMPONENTS Label, Button, Input, TextArea, CheckBox, RadioBox, ComboBox, ListBox, ProgressBar, SliderBar, ColorPicker, Panel, Spoiler
#define GCHL_GUI_LAYOUT_COMPONENTS LayoutLine, LayoutTable, LayoutSplitter

#define GUI_HAS_COMPONENT(T,E) (E)->has(impl->components.T)
#define GUI_REF_COMPONENT(T) hierarchy->ent->value<CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(hierarchy->impl->components.T)
#define CAGE_COMPONENT_GUI(T,N,E) CAGE_JOIN(Gui, CAGE_JOIN(T, Component)) &N = (E)->value<CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(impl->components.T);

namespace cage
{
	class GuiImpl;
	struct HierarchyItem;

	struct SkinData : public GuiSkinConfig
	{
		Holder<UniformBuffer> elementsGpuBuffer;
		Texture *texture;
	};

	struct RenderableBase
	{
		RenderableBase *next;
		vec2 clipPos, clipSize;

		RenderableBase();
		virtual ~RenderableBase();

		void setClip(const HierarchyItem *item);

		virtual void render(GuiImpl *impl);
	};

	struct RenderableElement : public RenderableBase
	{
		struct Element
		{
			vec4 outer;
			vec4 inner;
			uint32 element;
			uint32 mode;
			Element();
		} data;
		const UniformBuffer *skinBuffer;
		const Texture *skinTexture;

		RenderableElement();

		virtual void render(GuiImpl *impl) override;
	};

	struct RenderableText : public RenderableBase
	{
		struct Text
		{
			mat4 transform;
			Font::FormatStruct format;
			vec3 color;
			Font *font;
			uint32 *glyphs;
			uint32 cursor;
			uint32 count;
			Text();
			void apply(const GuiTextFormatComponent &f, GuiImpl *impl);
		} data;

		virtual void render(GuiImpl *impl) override;
	};

	struct RenderableImage : public RenderableBase
	{
		struct Image
		{
			vec4 ndcPos;
			vec4 uvClip;
			vec4 aniTexFrames;
			Texture *texture;
			Image();
		} data;

		virtual void render(GuiImpl *impl) override;
	};

	struct FinalPosition
	{
		vec2 renderPos, renderSize;
		vec2 clipPos, clipSize;

		FinalPosition();
	};

	struct HierarchyItem
	{
		// size (points) as seen by parent
		vec2 requestedSize;
		vec2 renderPos, renderSize;
		vec2 clipPos, clipSize; // clip pos/size are in same coordinate system as render pos/size, they make a rectangle intersection of parents viewport and our render pos/size

		GuiImpl *const impl;
		Entity *const ent;

		HierarchyItem *parent;
		HierarchyItem *prevSibling, *nextSibling;
		HierarchyItem *firstChild, *lastChild;
		sint32 order; // relative ordering of items with same parent

		struct BaseItem *item;
		struct TextItem *text;
		struct ImageItem *Image;

		bool subsidedItem; // prevent use of explicit position

		HierarchyItem(GuiImpl *impl, Entity *ent);

		// called top->down
		void initialize(); // initialize and validate widget, layout, text and image, initialize children
		void findRequestedSize(); // fills in the requestedSize
		void findFinalPosition(const FinalPosition &update); // given position and available space in the FinalPosition, determine actual position and size
		void childrenEmit() const;
		void generateEventReceivers() const;

		// helpers
		void moveToWindow(bool horizontal, bool vertical);
		void detachChildren();
		void detachParent();
		void attachParent(HierarchyItem *newParent);

		// debug
		void emitDebug() const;
		void emitDebug(vec2 pos, vec2 size, vec4 color) const;
		void printDebug(uint32 offset) const;

		void fireWidgetEvent() const;
	};

	struct BaseItem
	{
		HierarchyItem *const hierarchy;

		BaseItem(HierarchyItem *hierarchy);
		virtual ~BaseItem();

		virtual void initialize() = 0;
		virtual void findRequestedSize() = 0;
		virtual void findFinalPosition(const FinalPosition &update) = 0;
		virtual void emit() const = 0;
		virtual void generateEventReceivers() = 0;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) = 0;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point) = 0;
		virtual bool keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers) = 0;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers) = 0;
		virtual bool keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers) = 0;
		virtual bool keyChar(uint32 key) = 0;
	};

	struct WidgetItem : public BaseItem
	{
		GuiWidgetStateComponent widgetState;
		const SkinData *skin;

		WidgetItem(HierarchyItem *hierarchy);

		uint32 mode(bool hover = true, uint32 focusParts = 1) const;
		uint32 mode(const vec2 &pos, const vec2 &size, uint32 focusParts = 1) const;
		bool hasFocus(uint32 part = 1) const;
		void makeFocused(uint32 part = 1);
		RenderableElement *emitElement(GuiElementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const;

		virtual void findFinalPosition(const FinalPosition &update) override;
		virtual void generateEventReceivers() override;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point) override;
		virtual bool keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct LayoutItem : public BaseItem
	{
		LayoutItem(HierarchyItem *hierarchy);

		virtual void emit() const override;
		virtual void generateEventReceivers() override;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point) override;
		virtual bool keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct TextItem
	{
		HierarchyItem *const hierarchy;
		RenderableText::Text text;
		bool skipInitialize;

		TextItem(HierarchyItem *hierarchy);

		void initialize();

		void transcript();
		void transcript(const string &value);
		void transcript(const char *value);

		vec2 findRequestedSize();

		RenderableText *emit(vec2 position, vec2 size) const;

		void updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor);
	};

	struct ImageItem
	{
		GuiImageComponent Image;
		GuiImageFormatComponent format;
		HierarchyItem *const hierarchy = nullptr;
		Texture *texture = nullptr;
		bool skipInitialize = false;

		ImageItem(HierarchyItem *hierarchy);

		void initialize();

		void assign();
		void assign(const GuiImageComponent &value);
		void apply(const GuiImageFormatComponent &f);

		vec2 findRequestedSize();

		RenderableImage *emit(vec2 position, vec2 size) const;
	};

	struct EventReceiver
	{
		vec2 pos, size;
		WidgetItem *widget;
		uint32 mask; // bitmask for event classes

		EventReceiver();

		bool pointInside(vec2 point, uint32 maskRequests = 1) const;
	};

	class GuiImpl : public Gui
	{
	public:
		Holder<EntityManager> entityMgr;
		privat::GuiComponents components;

		AssetManager *assetMgr;

		ivec2 inputResolution; // last window resolution (pixels)
		ivec2 inputMouse; // last position of mouse (pixels)
		ivec2 outputResolution; // resolution of output texture or screen (pixels)
		vec2 outputSize; // (points)
		vec2 outputMouse; // (points)
		real zoom, retina, pointsScale; // how many pixels per point (1D)
		uint32 focusName; // focused entity name
		uint32 focusParts; // bitmask of focused parts of the single widget (bits 30 and 31 are reserved for scrollbars)
		WidgetItem *hover;

		Holder<MemoryArena> itemsArena;
		MemoryArena itemsMemory;
		HierarchyItem *root;

		struct GraphicsData
		{
			ShaderProgram *debugShader;
			ShaderProgram *elementShader;
			ShaderProgram *fontShader;
			ShaderProgram *imageAnimatedShader;
			ShaderProgram *imageStaticShader;
			ShaderProgram *colorPickerShader[3];
			Mesh *debugMesh;
			Mesh *elementMesh;
			Mesh *fontMesh;
			Mesh *imageMesh;
			GraphicsData();
		} graphicsData;

		struct EmitData
		{
			Holder<MemoryArena> arena;
			MemoryArena memory;
			RenderableBase *first = nullptr, *last = nullptr;

			EmitData(const GuiCreateConfig &config);
			//EmitData(EmitData &&other); // this is not defined but it is required - some stupid weird issue with gcc.
			~EmitData();
			void flush();
		} emitData[3], *emitControl;
		Holder<SwapBufferGuard> emitController;

		WindowEventListeners listeners;
		std::vector<EventReceiver> mouseEventReceivers;
		bool eventsEnabled;

		std::vector<SkinData> skins;

		explicit GuiImpl(const GuiCreateConfig &config);
		~GuiImpl();

		void scaling();

		bool eventPoint(const ivec2 &ptIn, vec2 &ptOut);
		vec4 pointsToNdc(vec2 position, vec2 size);

		void graphicsDispatch();

		uint32 entityWidgetsCount(Entity *e);
		uint32 entityLayoutsCount(Entity *e);
	};

	void offsetPosition(vec2 &position, const vec4 &offset);
	void offsetSize(vec2 &size, const vec4 &offset);
	void offset(vec2 &position, vec2 &size, const vec4 &offset);
	bool pointInside(vec2 pos, vec2 size, vec2 point);
	bool clip(vec2 &pos, vec2 &size, vec2 clipPos, vec2 clipSize); // return whether the clipped rect has positive area

	HierarchyItem *subsideItem(HierarchyItem *item);
	void ensureItemHasLayout(HierarchyItem *item); // if the item's entity does not have any layout, subside the item and add default layouter to it
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
