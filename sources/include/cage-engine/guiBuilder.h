#ifndef guard_guiBuilder_sxrdk487
#define guard_guiBuilder_sxrdk487

#include <cage-core/entities.h>
#include <cage-engine/guiComponents.h>

namespace cage
{
	namespace guiBuilder
	{
		class GuiBuilder;

		struct CAGE_ENGINE_API BuilderItem
		{
			BuilderItem(GuiBuilder *g);
			BuilderItem(BuilderItem &&);
			BuilderItem(BuilderItem &);
			BuilderItem &operator=(BuilderItem &);
			BuilderItem &operator=(BuilderItem &&);
			~BuilderItem();

			Entity *entity() const;
			Entity *operator->() const;
			Entity &operator*() const;

			BuilderItem text(const GuiTextComponent &txt);
			BuilderItem text(const String &txt);
			BuilderItem text(uint32 textId, const String &parameters = "");
			BuilderItem textFormat(const GuiTextFormatComponent &textFormat);
			BuilderItem textColor(Vec3 color);
			BuilderItem textSize(Real size);
			BuilderItem textAlign(TextAlignEnum align);
			BuilderItem image(const GuiImageComponent &img);
			BuilderItem image(uint32 textureName);
			BuilderItem imageFormat(const GuiImageFormatComponent &imageFormat);
			BuilderItem size(Vec2 size);
			BuilderItem accent(Vec4 accent);
			BuilderItem skin(GuiSkinIndex skin);
			BuilderItem disabled(bool disable = true);

			BuilderItem event(Delegate<bool(const GenericInput &)> ev);
			BuilderItem event(void (*fnc)())
			{
				return event(inputFilter([fnc](input::GuiValue) { fnc(); }));
			}

			BuilderItem update(Delegate<void(Entity *)> u);

			BuilderItem tooltip(const GuiTooltipComponent &t);
			BuilderItem tooltip(const String &txt, uint64 delay = GuiTooltipComponent().delay)
			{
				(*this)->value<GuiTooltipComponent>() = GuiTooltipComponent{ .tooltip = privat::guiTooltipText(entity(), txt), .delay = delay };
				return *this;
			}
			template<StringLiteral Text>
			BuilderItem tooltip(uint64 delay = GuiTooltipComponent().delay)
			{
				(*this)->template value<GuiTooltipComponent>() = GuiTooltipComponent{ .tooltip = detail::guiTooltipText<Text>(), .delay = delay };
				return *this;
			}
			template<uint32 TextId>
			BuilderItem tooltip(uint64 delay = GuiTooltipComponent().delay)
			{
				(*this)->template value<GuiTooltipComponent>() = GuiTooltipComponent{ .tooltip = detail::guiTooltipText<TextId>(), .delay = delay };
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

			[[nodiscard]] BuilderItem row(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem spacedRow(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem leftRow(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem leftRowStretchRight(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem rightRow(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem rightRowStretchLeft(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem centerRow(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem centerRowStretchBoth(Real verticalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem column(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem spacedColumn(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem topColumn(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem topColumnStretchBottom(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem bottomColumn(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem bottomColumnStretchTop(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem middleColumn(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem middleColumnStretchBoth(Real horizontalAlign = GuiLayoutLineComponent().crossAlign);
			[[nodiscard]] BuilderItem horizontalSplit(Real verticalAlign = GuiLayoutSplitComponent().crossAlign);
			[[nodiscard]] BuilderItem verticalSplit(Real horizontalAlign = GuiLayoutSplitComponent().crossAlign);
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
