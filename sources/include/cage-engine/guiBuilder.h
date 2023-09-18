#ifndef guard_guiBuilder_sxrdk487
#define guard_guiBuilder_sxrdk487

#include <cage-core/entities.h>
#include <cage-core/stringLiteral.h>
#include <cage-engine/guiComponents.h>

namespace cage
{
	class EntityManager;
	class Entity;

	namespace privat
	{
		template<void (*F)(), bool StopPropagation = true>
		CAGE_FORCE_INLINE bool guiActionWrapper(Entity *)
		{
			(F)();
			return StopPropagation;
		}
	}

	namespace guiBuilder
	{
		class GuiBuilder;

		struct CAGE_ENGINE_API BuilderItem
		{
			BuilderItem(GuiBuilder *g);
			BuilderItem(BuilderItem &&) noexcept;
			BuilderItem(BuilderItem &) noexcept;
			BuilderItem &operator=(BuilderItem &) noexcept;
			BuilderItem &operator=(BuilderItem &&) noexcept;
			~BuilderItem();

			Entity *entity() const;
			Entity *operator->() const;
			Entity &operator*() const;

			BuilderItem text(const GuiTextComponent &txt);
			BuilderItem text(const String &txt);
			BuilderItem text(uint32 assetName, uint32 textName, const String &parameters = "");
			BuilderItem textFormat(const GuiTextFormatComponent &textFormat);
			BuilderItem textColor(Vec3 color);
			BuilderItem textSize(Real size);
			BuilderItem textAlign(TextAlignEnum align);
			BuilderItem image(const GuiImageComponent &img);
			BuilderItem image(uint32 textureName);
			BuilderItem imageFormat(const GuiImageFormatComponent &imageFormat);
			BuilderItem size(Vec2 size);
			BuilderItem skin(uint32 index = m);
			BuilderItem disabled(bool disable = true);

			BuilderItem event(Delegate<bool(Entity *)> ev);
			template<bool (*F)(Entity *)>
			BuilderItem bind()
			{
				return event(Delegate<bool(Entity *)>().bind<F>());
			}
			template<class D, bool (*F)(D, Entity *)>
			BuilderItem bind(D d)
			{
				return event(Delegate<bool(Entity *)>().bind<D, F>(d));
			}
			template<void (*F)(), bool StopPropagation = true>
			BuilderItem bind()
			{
				return event(Delegate<bool(Entity *)>().bind<&privat::guiActionWrapper<F, StopPropagation>>());
			}

			BuilderItem tooltip(const GuiTooltipComponent &t);
			template<StringLiteral Text, uint32 AssetName = 0, uint32 TextName = 0>
			BuilderItem tooltip(uint64 delay = GuiTooltipComponent().delay)
			{
				(*this)->template value<GuiTooltipComponent>().tooltip = detail::guiTooltipText<Text, AssetName, TextName>();
				(*this)->template value<GuiTooltipComponent>().delay = delay;
				return *this;
			}

		protected:
			GuiBuilder *g = nullptr;
			Entity *e = nullptr;
		};

		class CAGE_ENGINE_API GuiBuilder : public Immovable
		{
		public:
			[[nodiscard]] GuiBuilder &setNextName(uint32 name);

			[[nodiscard]] BuilderItem row(bool spaced = false, Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem leftRow(bool flexibleRight = false, Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem rightRow(bool flexibleLeft = false, Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem centerRow(bool flexibleEdges = false, Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem column(bool spaced = false, Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem topColumn(bool flexibleBottom = false, Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem bottomColumn(bool flexibleTop = false, Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem middleColumn(bool flexibleEdges = false, Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem horizontalTable(uint32 rows = GuiLayoutTableComponent().sections, bool grid = GuiLayoutTableComponent().grid);
			[[nodiscard]] BuilderItem verticalTable(uint32 columns = GuiLayoutTableComponent().sections, bool grid = GuiLayoutTableComponent().grid);
			[[nodiscard]] BuilderItem alignment(Vec2 align = GuiLayoutAlignmentComponent().alignment);
			[[nodiscard]] BuilderItem scrollbars(bool alwaysShown = false);
			[[nodiscard]] BuilderItem scrollbars(const GuiLayoutScrollbarsComponent &sc);
			[[nodiscard]] BuilderItem horizontalScrollbar(bool alwaysShown = false);
			[[nodiscard]] BuilderItem verticalScrollbar(bool alwaysShown = false);

			BuilderItem empty();
			BuilderItem label();
			BuilderItem button();
			BuilderItem input(const GuiInputComponent &in);
			BuilderItem input(const String &txt);
			BuilderItem input(Real value, Real min = 0, Real max = 1, Real step = 0.01);
			BuilderItem input(sint32 value, sint32 min = 0, sint32 max = 100, sint32 step = 1);
			BuilderItem textArea(MemoryBuffer *buffer);
			BuilderItem checkBox(bool checked = false);
			BuilderItem radioBox(bool checked = false);
			BuilderItem comboBox(uint32 selected = m);
			BuilderItem progressBar(Real value = 0);
			BuilderItem horizontalSliderBar(Real value = 0, Real min = 0, Real max = 1);
			BuilderItem verticalSliderBar(Real value = 0, Real min = 0, Real max = 1);
			BuilderItem colorPicker(Vec3 color = GuiColorPickerComponent().color, bool collapsible = GuiColorPickerComponent().collapsible);
			BuilderItem panel();
			BuilderItem spoiler(bool collapsed = GuiSpoilerComponent().collapsed, bool collapsesSiblings = GuiSpoilerComponent().collapsesSiblings);
		};
	}

	using guiBuilder::GuiBuilder;

	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(EntityManager *gui);
	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(Entity *root);
}

#endif // guard_guiBuilder_sxrdk487
