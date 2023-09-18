#include <vector>

#include <cage-core/entities.h>
#include <cage-engine/guiBuilder.h>

namespace cage
{
	namespace guiBuilder
	{
		namespace
		{
			struct StackItem
			{
				Entity *e = nullptr;
				sint32 order = 0;
			};

			class GuiBuilderImpl : public GuiBuilder
			{
			public:
				std::vector<StackItem> stack;
				EntityManager *man = nullptr;
				uint32 nextName = 0;

				GuiBuilderImpl(Entity *root)
				{
					CAGE_ASSERT(root);
					man = root->manager();
					stack.push_back(StackItem{ root, 0 });
				}
			};
		}

		BuilderItem::BuilderItem(GuiBuilder *g) : g(g)
		{
			GuiBuilderImpl *impl = (GuiBuilderImpl *)g;
			e = impl->nextName ? impl->man->getOrCreate(impl->nextName) : impl->man->createUnique();
			impl->nextName = 0;
			if (!impl->stack.empty())
			{
				e->value<GuiParentComponent>().parent = impl->stack.back().e->name();
				e->value<GuiParentComponent>().order = impl->stack.back().order++;
			}
			impl->stack.push_back({ e });
		}

		BuilderItem::BuilderItem(BuilderItem &&other) noexcept
		{
			std::swap(g, other.g);
			std::swap(e, other.e);
		}

		BuilderItem::BuilderItem(BuilderItem &other) noexcept
		{
			std::swap(g, other.g);
			std::swap(e, other.e);
		}

		BuilderItem &BuilderItem::operator=(BuilderItem &other) noexcept
		{
			CAGE_ASSERT(this != &other);
			std::swap(g, other.g);
			std::swap(e, other.e);
			return *this;
		}

		BuilderItem &BuilderItem::operator=(BuilderItem &&other) noexcept
		{
			CAGE_ASSERT(this != &other);
			std::swap(g, other.g);
			std::swap(e, other.e);
			return *this;
		}

		BuilderItem::~BuilderItem()
		{
			if (!g)
				return; // moved-from
			GuiBuilderImpl *impl = (GuiBuilderImpl *)g;
			CAGE_ASSERT(!impl->stack.empty());
			CAGE_ASSERT(impl->stack.back().e == e);
			impl->stack.pop_back();
		}

		Entity *BuilderItem::entity() const
		{
			return e;
		}

		Entity *BuilderItem::operator->() const
		{
			return entity();
		}

		Entity &BuilderItem::operator*() const
		{
			return *entity();
		}

		BuilderItem BuilderItem::text(const GuiTextComponent &txt)
		{
			(*this)->value<GuiTextComponent>() = txt;
			return *this;
		}

		BuilderItem BuilderItem::text(const String &txt)
		{
			(*this)->value<GuiTextComponent>().value = txt;
			return *this;
		}

		BuilderItem BuilderItem::text(uint32 assetName, uint32 textName, const String &parameters)
		{
			return text(GuiTextComponent{ parameters, assetName, textName });
		}

		BuilderItem BuilderItem::textFormat(const GuiTextFormatComponent &textFormat)
		{
			(*this)->value<GuiTextFormatComponent>() = textFormat;
			return *this;
		}

		BuilderItem BuilderItem::textColor(Vec3 color)
		{
			(*this)->value<GuiTextFormatComponent>().color = color;
			return *this;
		}

		BuilderItem BuilderItem::textSize(Real size)
		{
			(*this)->value<GuiTextFormatComponent>().size = size;
			return *this;
		}

		BuilderItem BuilderItem::textAlign(TextAlignEnum align)
		{
			(*this)->value<GuiTextFormatComponent>().align = align;
			return *this;
		}

		BuilderItem BuilderItem::image(const GuiImageComponent &img)
		{
			(*this)->value<GuiImageComponent>() = img;
			return *this;
		}

		BuilderItem BuilderItem::image(uint32 textureName)
		{
			(*this)->value<GuiImageComponent>().textureName = textureName;
			return *this;
		}

		BuilderItem BuilderItem::imageFormat(const GuiImageFormatComponent &imageFormat)
		{
			(*this)->value<GuiImageFormatComponent>() = imageFormat;
			return *this;
		}

		BuilderItem BuilderItem::size(Vec2 size)
		{
			(*this)->value<GuiExplicitSizeComponent>().size = size;
			return *this;
		}

		BuilderItem BuilderItem::skin(uint32 index)
		{
			(*this)->value<GuiWidgetStateComponent>().skinIndex = index;
			return *this;
		}

		BuilderItem BuilderItem::disabled(bool disable)
		{
			(*this)->value<GuiWidgetStateComponent>().disabled = disable;
			return *this;
		}

		BuilderItem BuilderItem::event(Delegate<bool(Entity *)> ev)
		{
			(*this)->value<GuiEventComponent>().event = ev;
			return *this;
		}

		BuilderItem BuilderItem::tooltip(const GuiTooltipComponent &t)
		{
			(*this)->value<GuiTooltipComponent>() = t;
			return *this;
		}

		GuiBuilder &GuiBuilder::setNextName(uint32 name)
		{
			CAGE_ASSERT(name != m);
			GuiBuilderImpl *impl = (GuiBuilderImpl *)this;
			impl->nextName = name;
			return *this;
		}

		BuilderItem GuiBuilder::row(bool spaced, Real verticalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = verticalAlign;
			l.first = l.last = spaced ? LineEdgeModeEnum::Spaced : LineEdgeModeEnum::None;
			return c;
		}

		BuilderItem GuiBuilder::leftRow(bool flexibleRight, Real verticalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = verticalAlign;
			l.last = flexibleRight ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			return c;
		}

		BuilderItem GuiBuilder::rightRow(bool flexibleLeft, Real verticalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = verticalAlign;
			l.first = flexibleLeft ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			return c;
		}

		BuilderItem GuiBuilder::centerRow(bool flexibleEdges, Real verticalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = verticalAlign;
			l.first = l.last = flexibleEdges ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			return c;
		}

		BuilderItem GuiBuilder::column(bool spaced, Real horizontalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = horizontalAlign;
			l.first = l.last = spaced ? LineEdgeModeEnum::Spaced : LineEdgeModeEnum::None;
			l.vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::topColumn(bool flexibleBottom, Real horizontalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = horizontalAlign;
			l.last = flexibleBottom ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			l.vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::bottomColumn(bool flexibleTop, Real horizontalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = horizontalAlign;
			l.first = flexibleTop ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			l.vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::middleColumn(bool flexibleEdges, Real horizontalAlign)
		{
			BuilderItem c(this);
			GuiLayoutLineComponent &l = c->value<GuiLayoutLineComponent>();
			l.crossAlign = horizontalAlign;
			l.first = l.last = flexibleEdges ? LineEdgeModeEnum::Flexible : LineEdgeModeEnum::Empty;
			l.vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::horizontalTable(uint32 rows, bool grid)
		{
			BuilderItem c(this);
			c->value<GuiLayoutTableComponent>().sections = rows;
			c->value<GuiLayoutTableComponent>().grid = grid;
			c->value<GuiLayoutTableComponent>().vertical = false;
			return c;
		}

		BuilderItem GuiBuilder::verticalTable(uint32 columns, bool grid)
		{
			BuilderItem c(this);
			c->value<GuiLayoutTableComponent>().sections = columns;
			c->value<GuiLayoutTableComponent>().grid = grid;
			c->value<GuiLayoutTableComponent>().vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::alignment(Vec2 align)
		{
			BuilderItem c(this);
			c->value<GuiLayoutAlignmentComponent>().alignment = align;
			return c;
		}

		BuilderItem GuiBuilder::scrollbars(bool alwaysShown)
		{
			BuilderItem c(this);
			c->value<GuiLayoutScrollbarsComponent>().overflow[0] = alwaysShown ? OverflowModeEnum::Always : OverflowModeEnum::Auto;
			c->value<GuiLayoutScrollbarsComponent>().overflow[1] = alwaysShown ? OverflowModeEnum::Always : OverflowModeEnum::Auto;
			return c;
		}

		BuilderItem GuiBuilder::scrollbars(const GuiLayoutScrollbarsComponent &sc)
		{
			BuilderItem c(this);
			c->value<GuiLayoutScrollbarsComponent>() = sc;
			return c;
		}

		BuilderItem GuiBuilder::horizontalScrollbar(bool alwaysShown)
		{
			BuilderItem c(this);
			c->value<GuiLayoutScrollbarsComponent>().overflow[0] = alwaysShown ? OverflowModeEnum::Always : OverflowModeEnum::Auto;
			c->value<GuiLayoutScrollbarsComponent>().overflow[1] = OverflowModeEnum::Never;
			return c;
		}

		BuilderItem GuiBuilder::verticalScrollbar(bool alwaysShown)
		{
			BuilderItem c(this);
			c->value<GuiLayoutScrollbarsComponent>().overflow[0] = OverflowModeEnum::Never;
			c->value<GuiLayoutScrollbarsComponent>().overflow[1] = alwaysShown ? OverflowModeEnum::Always : OverflowModeEnum::Auto;
			return c;
		}

		BuilderItem GuiBuilder::empty()
		{
			return BuilderItem(this);
		}

		BuilderItem GuiBuilder::label()
		{
			BuilderItem c(this);
			c->value<GuiLabelComponent>();
			return c;
		}

		BuilderItem GuiBuilder::button()
		{
			BuilderItem c(this);
			c->value<GuiButtonComponent>();
			return c;
		}

		BuilderItem GuiBuilder::input(const GuiInputComponent &in)
		{
			BuilderItem c(this);
			c->value<GuiInputComponent>() = in;
			return c;
		}

		BuilderItem GuiBuilder::input(const String &txt)
		{
			BuilderItem c(this);
			c->value<GuiInputComponent>().value = txt;
			c->value<GuiInputComponent>().type = InputTypeEnum::Text;
			return c;
		}

		BuilderItem GuiBuilder::input(Real value, Real min, Real max, Real step)
		{
			BuilderItem c(this);
			c->value<GuiInputComponent>().value = Stringizer() + value;
			c->value<GuiInputComponent>().min.f = min;
			c->value<GuiInputComponent>().max.f = max;
			c->value<GuiInputComponent>().step.f = step;
			c->value<GuiInputComponent>().type = InputTypeEnum::Real;
			return c;
		}

		BuilderItem GuiBuilder::input(sint32 value, sint32 min, sint32 max, sint32 step)
		{
			BuilderItem c(this);
			c->value<GuiInputComponent>().value = Stringizer() + value;
			c->value<GuiInputComponent>().min.i = min;
			c->value<GuiInputComponent>().max.i = max;
			c->value<GuiInputComponent>().step.i = step;
			c->value<GuiInputComponent>().type = InputTypeEnum::Integer;
			return c;
		}

		BuilderItem GuiBuilder::textArea(MemoryBuffer *buffer)
		{
			BuilderItem c(this);
			c->value<GuiTextAreaComponent>().buffer = buffer;
			return c;
		}

		BuilderItem GuiBuilder::checkBox(bool checked)
		{
			BuilderItem c(this);
			c->value<GuiCheckBoxComponent>().state = checked ? CheckBoxStateEnum::Checked : CheckBoxStateEnum::Unchecked;
			return c;
		}

		BuilderItem GuiBuilder::radioBox(bool checked)
		{
			BuilderItem c(this);
			c->value<GuiRadioBoxComponent>().state = checked ? CheckBoxStateEnum::Checked : CheckBoxStateEnum::Unchecked;
			return c;
		}

		BuilderItem GuiBuilder::comboBox(uint32 selected)
		{
			BuilderItem c(this);
			c->value<GuiComboBoxComponent>().selected = selected;
			return c;
		}

		BuilderItem GuiBuilder::progressBar(Real value)
		{
			BuilderItem c(this);
			c->value<GuiProgressBarComponent>().progress = value;
			return c;
		}

		BuilderItem GuiBuilder::horizontalSliderBar(Real value, Real min, Real max)
		{
			BuilderItem c(this);
			c->value<GuiSliderBarComponent>().value = value;
			c->value<GuiSliderBarComponent>().min = min;
			c->value<GuiSliderBarComponent>().max = max;
			c->value<GuiSliderBarComponent>().vertical = false;
			return c;
		}

		BuilderItem GuiBuilder::verticalSliderBar(Real value, Real min, Real max)
		{
			BuilderItem c(this);
			c->value<GuiSliderBarComponent>().value = value;
			c->value<GuiSliderBarComponent>().min = min;
			c->value<GuiSliderBarComponent>().max = max;
			c->value<GuiSliderBarComponent>().vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::colorPicker(Vec3 color, bool collapsible)
		{
			BuilderItem c(this);
			c->value<GuiColorPickerComponent>().color = color;
			c->value<GuiColorPickerComponent>().collapsible = collapsible;
			return c;
		}

		BuilderItem GuiBuilder::panel()
		{
			BuilderItem c(this);
			c->value<GuiPanelComponent>();
			return c;
		}

		BuilderItem GuiBuilder::spoiler(bool collapsed, bool collapsesSiblings)
		{
			BuilderItem c(this);
			c->value<GuiSpoilerComponent>().collapsed = collapsed;
			c->value<GuiSpoilerComponent>().collapsesSiblings = collapsesSiblings;
			return c;
		}
	}

	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(EntityManager *gui)
	{
		return newGuiBuilder(gui->createUnique());
	}

	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(Entity *root)
	{
		return systemMemory().createImpl<GuiBuilder, guiBuilder::GuiBuilderImpl>(root);
	}
}
