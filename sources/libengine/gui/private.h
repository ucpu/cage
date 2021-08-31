#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#include <cage-core/entities.h>

#include <cage-engine/gui.h>
#include <cage-engine/guiSkins.h>
#include <cage-engine/font.h> // FontFormat
#include <cage-engine/window.h> // WindowEventListeners
#include <cage-engine/renderQueue.h> // UubRange

#include <vector>

#define GCHL_GUI_COMMON_COMPONENTS Parent, Image, ImageFormat, Text, TextFormat, Selection, Tooltip, WidgetState, SelectedItem, Scrollbars, ExplicitSize, Event
#define GCHL_GUI_WIDGET_COMPONENTS Label, Button, Input, TextArea, CheckBox, RadioBox, ComboBox, ListBox, ProgressBar, SliderBar, ColorPicker, Panel, Spoiler
#define GCHL_GUI_LAYOUT_COMPONENTS LayoutLine, LayoutTable, LayoutSplitter

#define GUI_HAS_COMPONENT(T,E) (E)->has<Gui##T##Component>()
#define GUI_REF_COMPONENT(T) hierarchy->ent->value<Gui##T##Component>()
#define GUI_COMPONENT(T,N,E) Gui##T##Component &N = (E)->value<Gui##T##Component>();

namespace cage
{
	class GuiImpl;
	struct HierarchyItem;
	struct RenderableElement;
	struct RenderableText;
	struct RenderableImage;

	struct SkinData : public GuiSkinConfig
	{
		Holder<Texture> texture;
		UubRange uubRange;

		void bind(RenderQueue *queue) const;
	};

	struct FinalPosition
	{
		Vec2 renderPos = Vec2::Nan(), renderSize = Vec2::Nan();
		Vec2 clipPos = Vec2::Nan(), clipSize = Vec2::Nan();
	};

	struct HierarchyItem : private Immovable
	{
		// size (points) as seen by parent
		Vec2 requestedSize = Vec2::Nan();
		Vec2 renderPos = Vec2::Nan(), renderSize = Vec2::Nan();
		Vec2 clipPos = Vec2::Nan(), clipSize = Vec2::Nan(); // clip pos/size are in same coordinate system as render pos/size, they make a rectangle intersection of parents viewport and our render pos/size

		GuiImpl *const impl = nullptr;
		Entity *const ent = nullptr;

		std::vector<Holder<HierarchyItem>> children;
		sint32 order = 0; // relative ordering of items with same parent

		Holder<struct BaseItem> item;
		Holder<struct TextItem> text;
		Holder<struct ImageItem> image;

		bool subsidedItem = false; // prevent use of explicit position

		HierarchyItem(GuiImpl *impl, Entity *ent);

		// called top->down
		void initialize(); // initialize and validate widget, layout, text and image, initialize children
		void findRequestedSize(); // fills in the requestedSize
		void findFinalPosition(const FinalPosition &update); // given position and available space in the FinalPosition, determine actual position and size
		void childrenEmit();
		void generateEventReceivers() const;

		// helpers
		void moveToWindow(bool horizontal, bool vertical);
		HierarchyItem *findParentOf(HierarchyItem *item);
		void fireWidgetEvent() const;
	};

	struct BaseItem : private Immovable
	{
		HierarchyItem *const hierarchy = nullptr;

		BaseItem(HierarchyItem *hierarchy);
		virtual ~BaseItem() = default;

		virtual void initialize() = 0;
		virtual void findRequestedSize() = 0;
		virtual void findFinalPosition(const FinalPosition &update) = 0;
		virtual void emit() = 0;
		virtual void generateEventReceivers() = 0;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) = 0;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) = 0;
		virtual bool keyRelease(uint32 key, ModifiersFlags modifiers) = 0;
		virtual bool keyChar(uint32 key) = 0;
	};

	struct WidgetItem : public BaseItem
	{
		GuiWidgetStateComponent widgetState;
		const SkinData *skin = nullptr;

		WidgetItem(HierarchyItem *hierarchy);

		uint32 mode(bool hover = true, uint32 focusParts = 1) const;
		uint32 mode(const Vec2 &pos, const Vec2 &size, uint32 focusParts = 1) const;
		bool hasFocus(uint32 part = 1) const;
		void makeFocused(uint32 part = 1);
		RenderableElement emitElement(GuiElementTypeEnum element, uint32 mode, Vec2 pos, Vec2 size);

		virtual void findFinalPosition(const FinalPosition &update) override;
		virtual void generateEventReceivers() override;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct LayoutItem : public BaseItem
	{
		LayoutItem(HierarchyItem *hierarchy);

		virtual void emit() override;
		virtual void generateEventReceivers() override;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRelease(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
	};

	struct CommonTextData
	{
		Mat4 Transform;
		FontFormat format;
		Holder<Font> font;
		Holder<PointerRange<uint32>> glyphs;
		Vec3 color = Vec3::Nan();
		uint32 cursor = m;
	};

	struct TextItem : private Immovable, public CommonTextData
	{
		HierarchyItem *const hierarchy;
		bool skipInitialize = false;

		TextItem(HierarchyItem *hierarchy);

		void initialize();

		void apply(const GuiTextFormatComponent &f);

		void transcript();
		void transcript(const String &value);
		void transcript(const char *value);

		Vec2 findRequestedSize();

		RenderableText emit(Vec2 position, Vec2 size);

		void updateCursorPosition(Vec2 position, Vec2 size, Vec2 point, uint32 &cursor);
	};

	struct ImageItem : private Immovable
	{
		GuiImageComponent image;
		GuiImageFormatComponent format;
		Holder<Texture> texture;

		HierarchyItem *const hierarchy = nullptr;
		bool skipInitialize = false;

		ImageItem(HierarchyItem *hierarchy);

		void initialize();

		void assign();
		void assign(const GuiImageComponent &value);
		void apply(const GuiImageFormatComponent &f);

		Vec2 findRequestedSize();

		RenderableImage emit(Vec2 position, Vec2 size);
	};

	struct RenderableBase : private Immovable
	{
		GuiImpl *impl = nullptr;
		Vec2 clipPos, clipSize;

		RenderableBase(GuiImpl *impl) : impl(impl)
		{}

		virtual ~RenderableBase() = default;

		void setClip(const HierarchyItem *item);
		bool prepare();
	};

	struct RenderableElement : public RenderableBase
	{
		struct Element
		{
			Vec4 outer;
			Vec4 inner;
			uint32 element = m;
			uint32 mode = m;
		} data;

		const SkinData *skin = nullptr;

		RenderableElement(WidgetItem *item, GuiElementTypeEnum element, uint32 mode, Vec2 pos, Vec2 size);

		virtual ~RenderableElement() override;
	};

	struct RenderableText : public RenderableBase
	{
		CommonTextData data;

		RenderableText(TextItem *text, Vec2 position, Vec2 size);

		virtual ~RenderableText() override;
	};

	struct RenderableImage : public RenderableBase
	{
		struct Image
		{
			Vec4 ndcPos;
			Vec4 uvClip;
			Vec4 aniTexFrames;
			Holder<Texture> texture;
		} data;

		RenderableImage(ImageItem *item, Vec2 position, Vec2 size);

		virtual ~RenderableImage() override;
	};

	struct EventReceiver
	{
		Vec2 pos, size;
		WidgetItem *widget;
		uint32 mask; // bitmask for event classes

		EventReceiver();

		bool pointInside(Vec2 point, uint32 maskRequests = 1) const;
	};

	class GuiImpl : public Gui
	{
	public:
		Holder<MemoryArena> memory; // must be last to destroy

		Holder<EntityManager> entityMgr = newEntityManager();
		AssetManager *assetMgr = nullptr;

		Vec2i inputResolution; // last window resolution (pixels)
		Vec2i inputMouse; // last position of mouse (pixels)
		Vec2i outputResolution; // resolution of output texture or screen (pixels)
		Vec2 outputSize; // (points)
		Vec2 outputMouse; // (points)
		Real zoom = 1, retina = 1, pointsScale = 1; // how many pixels per point (1D)
		uint32 focusName = 0; // focused entity name
		uint32 focusParts = 0; // bitmask of focused parts of the single widget (bits 30 and 31 are reserved for scrollbars)
		WidgetItem *hover = nullptr;
		Holder<HierarchyItem> root;

		struct GraphicsData
		{
			Holder<ShaderProgram> elementShader;
			Holder<ShaderProgram> fontShader;
			Holder<ShaderProgram> imageAnimatedShader;
			Holder<ShaderProgram> imageStaticShader;
			Holder<ShaderProgram> colorPickerShader[3];
			Holder<Model> elementModel;
			Holder<Model> fontModel;
			Holder<Model> imageModel;

			void load(AssetManager *assetMgr);
		} graphicsData;
		RenderQueue *activeQueue = nullptr;

		WindowEventListeners listeners;
		std::vector<EventReceiver> mouseEventReceivers;
		bool eventsEnabled = false;

		std::vector<SkinData> skins;

		explicit GuiImpl(const GuiCreateConfig &config);
		~GuiImpl();
		void scaling();
		Vec4 pointsToNdc(Vec2 position, Vec2 size) const;
		bool eventPoint(const Vec2i &ptIn, Vec2 &ptOut);
		Holder<RenderQueue> emit();
	};

	uint32 entityWidgetsCount(Entity *e);
	uint32 entityLayoutsCount(Entity *e);
	void offsetPosition(Vec2 &position, const Vec4 &offset);
	void offsetSize(Vec2 &size, const Vec4 &offset);
	void offset(Vec2 &position, Vec2 &size, const Vec4 &offset);
	bool pointInside(Vec2 pos, Vec2 size, Vec2 point);
	bool clip(Vec2 &pos, Vec2 &size, Vec2 clipPos, Vec2 clipSize); // return whether the clipped rect has positive area
	HierarchyItem *subsideItem(HierarchyItem *item);
	void ensureItemHasLayout(HierarchyItem *item); // if the item's entity does not have any layout, subside the item and add default layouter to it
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
