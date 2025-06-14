#ifndef guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
#define guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1

#include <vector>

#include <cage-core/entities.h>
#include <cage-engine/font.h> // FontFormat
#include <cage-engine/guiComponents.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/guiSkins.h>
#include <cage-engine/renderQueue.h> // UubRange
#include <cage-engine/soundsQueue.h>
#include <cage-engine/window.h> // WindowEventListeners

#define GUI_HAS_COMPONENT(T, E) (E)->has<Gui##T##Component>()
#define GUI_REF_COMPONENT(T) hierarchy->ent->value<Gui##T##Component>()
#define GUI_COMPONENT(T, N, E) Gui##T##Component &N = (E)->value<Gui##T##Component>();

namespace cage
{
	class AssetsOnDemand;
	class MultiShaderProgram;
	class GuiImpl;
	struct HierarchyItem;
	struct RenderableElement;
	struct RenderableText;
	struct RenderableImage;

	enum class ElementModeEnum
	{
		Default = 0,
		Focus = 1,
		Hover = 2,
		Disabled = 3,
	};

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
		void findRequestedSize(Real maxWidth); // fills in the requestedSize
		void findFinalPosition(const FinalPosition &update); // given position and available space in the FinalPosition, determine actual position and size
		void childrenEmit();
		void generateEventReceivers() const;

		// helpers
		void moveToWindow(bool horizontal, bool vertical);
		HierarchyItem *findParentOf(HierarchyItem *item);
		void fireWidgetEvent(GenericInput in) const;
	};

	struct BaseItem : private Immovable
	{
		HierarchyItem *const hierarchy = nullptr;

		BaseItem(HierarchyItem *hierarchy);
		virtual ~BaseItem() = default;

		virtual void initialize() = 0;
		virtual void findRequestedSize(Real maxWidth) = 0;
		virtual void findFinalPosition(const FinalPosition &update) = 0;
		virtual void emit() = 0;
		virtual void generateEventReceivers() = 0;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point) = 0;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) = 0;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) = 0;
		virtual bool keyChar(uint32 key) = 0;
		// no release events -> always propagate release events to all listeners
	};

	struct WidgetItem : public BaseItem
	{
		GuiWidgetStateComponent widgetState;
		const SkinData *skin = nullptr;

		WidgetItem(HierarchyItem *hierarchy);

		ElementModeEnum mode(bool hover = true, uint32 focusParts = 1) const;
		ElementModeEnum mode(const Vec2 &pos, const Vec2 &size, uint32 focusParts = 1) const;
		bool hasFocus(uint32 part = 1) const;
		void makeFocused(uint32 part = 1);
		RenderableElement emitElement(GuiElementTypeEnum element, ElementModeEnum mode, Vec2 pos, Vec2 size);

		void play(uint32 soundName);
		void playExclusive(uint32 soundName); // only plays when nothing else is currently playing

		virtual void findFinalPosition(const FinalPosition &update) override;
		virtual void generateEventReceivers() override;
		virtual void playHoverSound();

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
		// no release events -> always propagate release events to all listeners
	};

	struct LayoutItem : public BaseItem
	{
		LayoutItem(HierarchyItem *hierarchy);

		virtual void emit() override;
		virtual void generateEventReceivers() override;

		virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point) override;
		virtual bool keyPress(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyRepeat(uint32 key, ModifiersFlags modifiers) override;
		virtual bool keyChar(uint32 key) override;
		// no release events -> always propagate release events to all listeners
	};

	struct CommonTextData
	{
		FontFormat format;
		Holder<Font> font;
		FontLayoutResult layout;
		Vec3 color = Vec3::Nan();
		Real screenPxRange;
	};

	struct TextItem : private Immovable, private CommonTextData
	{
		HierarchyItem *const hierarchy;
		bool skipInitialize = false;

		TextItem(HierarchyItem *hierarchy);
		void initialize(); // load all data from entity component (text value and format)
		void assign(); // load from entity
		void assign(const String &value);
		void assign(PointerRange<const char> value);
		void apply(const GuiTextFormatComponent &f);
		Vec2 findRequestedSize(Real maxWidth = Real::Infinity());
		RenderableText emit(Vec2 position, Vec2 size, bool disabled);
		void updateCursorPosition(Vec2 position, Vec2 size, Vec2 point, uint32 &cursor);
		void setCursorPosition(uint32 cursor);

	private:
		Holder<PointerRange<uint32>> txt;
		uint32 fontId = 0;
		bool dirty = true;
		void updateFont();
		void updateLayout(); // update cached layout if needed
		void resize(Vec2 size); // update wrap width with new size

		friend struct RenderableText;
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
		RenderableImage emit(Vec2 position, Vec2 size, bool disabled);
	};

	struct RenderableBase : private Immovable
	{
		GuiImpl *impl = nullptr;
		Vec2 clipPos, clipSize;

		RenderableBase(GuiImpl *impl) : impl(impl) {}

		virtual ~RenderableBase() = default;

		void setClip(const HierarchyItem *item);
		bool prepare();
	};

	struct RenderableElement : public RenderableBase
	{
		Vec4 outer;
		Vec4 inner;
		Vec4 accent;
		const SkinData *skin = nullptr;
		uint32 element = m;
		ElementModeEnum mode = m;

		RenderableElement(WidgetItem *item, GuiElementTypeEnum element, ElementModeEnum mode, Vec2 pos, Vec2 size);

		virtual ~RenderableElement() override;
	};

	struct RenderableText : public RenderableBase
	{
		Mat4 transform;
		CommonTextData data;

		RenderableText(TextItem *text, Vec2 position, Vec2 size, bool disabled);

		virtual ~RenderableText() override;
	};

	struct RenderableImage : public RenderableBase
	{
		Holder<Texture> texture;
		Vec4 ndcPos;
		Vec4 uvClip;
		Vec4 animation; // time (seconds), speed, offset (normalized), unused
		bool disabled = false;

		RenderableImage(ImageItem *item, Vec2 position, Vec2 size, bool disabled);

		virtual ~RenderableImage() override;
	};

	enum class GuiEventsTypesFlags : uint32
	{
		Default = 1u << 0,
		Wheel = 1u << 1,
		Tooltips = 1u << 2,
	};
	GCHL_ENUM_BITS(GuiEventsTypesFlags);

	struct EventReceiver
	{
		Vec2 pos, size;
		WidgetItem *widget = nullptr;
		GuiEventsTypesFlags mask = GuiEventsTypesFlags::Default;

		bool pointInside(Vec2 point, GuiEventsTypesFlags maskRequests = GuiEventsTypesFlags::Default) const;
	};

	struct TooltipData : public GuiTooltipConfig
	{
		Entity *rootTooltip = nullptr; // we need to prepend new entity with layouting to facilitate positioning of the application-designed entity
		bool removing = false;
	};

	// this component is added to the root entity of shown tooltip to track its deletion
	struct GuiTooltipMarkerComponent
	{};

	// this component is used to hold data for text-only tooltips
	struct GuiTooltipStringComponent
	{
		String value;
		uint32 textId = 0;
	};

	class GuiImpl : public GuiManager
	{
	public:
		Holder<MemoryArena> memory; // must be last to destroy

		Holder<EntityManager> entityMgr = newEntityManager();
		Holder<AssetsOnDemand> assetOnDemand;
		AssetsManager *assetMgr = nullptr;
		ProvisionalGraphics *provisionalGraphics = nullptr;
		SoundsQueue *soundsQueue = nullptr;

		Vec2i outputResolution; // resolution of output texture or screen (pixels)
		Vec2 inputMouse; // last position of mouse (pixels)
		Vec2 outputSize; // (points)
		Vec2 outputMouse; // (points)
		Real zoom = 1, retina = 1, pointsScale = 1; // how many pixels per point (1D)
		uint32 focusName = 0; // focused entity name
		uint32 focusParts = 0; // bitmask of focused parts of the single widget (bits 30 and 31 are reserved for scrollbars)
		WidgetItem *hover = nullptr;
		uint32 hoverName = 0;
		Holder<HierarchyItem> root;

		struct GraphicsData
		{
			Holder<ShaderProgram> elementShader;
			Holder<ShaderProgram> fontShader;
			Holder<MultiShaderProgram> imageShader;
			Holder<ShaderProgram> colorPickerShader[3]; // H, S, V
			Holder<Model> elementModel;
			Holder<Model> imageModel;

			void load(AssetsManager *assetMgr);
		} graphicsData;
		RenderQueue *activeQueue = nullptr;

		bool mouseMove(input::MouseMove);
		bool mousePress(input::MousePress);
		bool mouseDoublePress(input::MouseDoublePress);
		bool mouseWheel(input::MouseWheel);
		bool keyPress(input::KeyPress);
		bool keyRepeat(input::KeyRepeat);
		bool keyChar(input::Character);
		bool testMouseCovered() const;
		// no release events -> always propagate release events to all listeners

		std::vector<EventReceiver> mouseEventReceivers;
		bool eventsEnabled = false;

		std::vector<SkinData> skins;

		bool ttEnabled = false;
		Vec2 ttLastMousePos; // points
		Real ttMouseTraveledDistance; // points
		uint64 ttTimestampMouseMove = 0; // last time when mouse moved too much
		bool ttHasMovedSinceLast = false;
		sint32 ttNextOrder = 0;
		std::vector<TooltipData> ttData;
		EventListener<bool(Entity *)> ttRemovedListener;
		void ttMouseMove(input::MouseMove in);
		void updateTooltips();
		void clearTooltips();
		void tooltipRemoved(Entity *e);

		explicit GuiImpl(const GuiManagerCreateConfig &config);
		~GuiImpl();
		void scaling(); // update outputSize and outputMouse
		Vec4 pointsToNdc(Vec2 position, Vec2 size) const;
		bool eventPoint(Vec2 ptIn, Vec2 &ptOut);
		Holder<RenderQueue> emit();
		void prepareImplGeneration();
	};

	uint32 entityWidgetsCount(Entity *e);
	uint32 entityLayoutsCount(Entity *e);
	void offsetPosition(Vec2 &position, Vec4 offset);
	void offsetSize(Vec2 &size, Vec4 offset);
	void offset(Vec2 &position, Vec2 &size, Vec4 offset);
	bool pointInside(Vec2 pos, Vec2 size, Vec2 point);
	bool clip(Vec2 &pos, Vec2 &size, Vec2 clipPos, Vec2 clipSize); // clips the rect and returns whether the clipped rect has positive area
	HierarchyItem *subsideItem(HierarchyItem *item);
	void ensureItemHasLayout(HierarchyItem *item); // if the item's entity does not have any layout, subside the item and add default layouter to it
}

#endif // guard_private_h_BEDE53C63BB74919B9BD171B995FD1A1
