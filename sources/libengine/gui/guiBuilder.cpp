#include <cage-engine/guiBuilder.h>
#include <cage-core/entities.h>

#include <vector>

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

				GuiBuilderImpl(Entity *root)
				{
					CAGE_ASSERT(root);
					man = root->manager();
					stack.push_back(StackItem{ root, 0 });
				}
			};
		}

		BuilderItem::BuilderItem(GuiBuilder *g, uint32 entityName) : g(g)
		{
			GuiBuilderImpl *impl = (GuiBuilderImpl *)g;
			e = entityName ? impl->man->getOrCreate(entityName) : impl->man->createUnique();
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

		BuilderItem &BuilderItem::operator = (BuilderItem &other) noexcept
		{
			CAGE_ASSERT(this != &other);
			std::swap(g, other.g);
			std::swap(e, other.e);
			return *this;
		}

		BuilderItem &BuilderItem::operator = (BuilderItem &&other) noexcept
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

		Entity *BuilderItem::operator -> () const
		{
			return entity();
		}

		Entity &BuilderItem::operator * () const
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

		BuilderItem BuilderItem::scrollbars(const GuiScrollbarsComponent &sc)
		{
			(*this)->value<GuiScrollbarsComponent>() = sc;
			return *this;
		}

		BuilderItem BuilderItem::scrollbars(Vec2 alignment)
		{
			(*this)->value<GuiScrollbarsComponent>().alignment = alignment;
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

		BuilderItem GuiBuilder::row()
		{
			BuilderItem c(this);
			c->value<GuiLayoutLineComponent>();
			return c;
		}

		BuilderItem GuiBuilder::column()
		{
			BuilderItem c(this);
			c->value<GuiLayoutLineComponent>().vertical = true;
			return c;
		}

		BuilderItem GuiBuilder::horizontalTable(uint32 rows, bool grid)
		{
			BuilderItem c(this);
			c->value<GuiLayoutTableComponent>().sections = rows;
			c->value<GuiLayoutTableComponent>().grid = grid;
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

		BuilderItem GuiBuilder::leftRight()
		{
			BuilderItem c(this);
			c->value<GuiLayoutSplitterComponent>().vertical = false;
			c->value<GuiLayoutSplitterComponent>().inverse = false;
			return c;
		}

		BuilderItem GuiBuilder::rightLeft()
		{
			BuilderItem c(this);
			c->value<GuiLayoutSplitterComponent>().vertical = false;
			c->value<GuiLayoutSplitterComponent>().inverse = true;
			return c;
		}

		BuilderItem GuiBuilder::topBottom()
		{
			BuilderItem c(this);
			c->value<GuiLayoutSplitterComponent>().vertical = true;
			c->value<GuiLayoutSplitterComponent>().inverse = false;
			return c;
		}

		BuilderItem GuiBuilder::bottomTop()
		{
			BuilderItem c(this);
			c->value<GuiLayoutSplitterComponent>().vertical = true;
			c->value<GuiLayoutSplitterComponent>().inverse = true;
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

		BuilderItem GuiBuilder::empty()
		{
			return BuilderItem(this);
		}

		BuilderItem GuiBuilder::empty(uint32 entityName)
		{
			return BuilderItem(this, entityName);
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
