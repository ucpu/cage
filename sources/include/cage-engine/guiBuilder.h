#ifndef guard_guiBuilder_sxrdk487
#define guard_guiBuilder_sxrdk487

#include <cage-core/entities.h>
#include "guiComponents.h"

namespace cage
{
	class EntityManager;
	class Entity;

	namespace guiBuilder
	{
		class GuiBuilder;

		struct CAGE_ENGINE_API BuilderItem
		{
			BuilderItem(GuiBuilder *g, uint32 entityName = 0);
			BuilderItem(BuilderItem &&) noexcept;
			BuilderItem(BuilderItem &) noexcept;
			BuilderItem &operator = (BuilderItem &) noexcept;
			BuilderItem &operator = (BuilderItem &&) noexcept;
			~BuilderItem();

			Entity *entity() const;
			Entity *operator -> () const;
			Entity &operator * () const;

			BuilderItem text(const GuiTextComponent &txt);
			BuilderItem text(const String &txt);
			BuilderItem format(const GuiTextFormatComponent &textFormat);
			BuilderItem image(const GuiImageComponent &img);
			BuilderItem image(uint32 textureName);
			BuilderItem format(const GuiImageFormatComponent &imageFormat);
			BuilderItem scrollbars(const GuiScrollbarsComponent &sc);
			BuilderItem scrollbars(Vec2 alignment);
			BuilderItem size(Vec2 size);
			BuilderItem skin(uint32 index = m);
			BuilderItem disabled(bool disable = true);
			BuilderItem event(Delegate<bool(uint32)> ev);

			template<privat::GuiStringLiteral Text, uint32 AssetName = 0, uint32 TextName = 0>
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
			[[nodiscard]] BuilderItem row();
			[[nodiscard]] BuilderItem column();
			[[nodiscard]] BuilderItem horizontalTable(uint32 rows = GuiLayoutTableComponent().sections, bool grid = GuiLayoutTableComponent().grid);
			[[nodiscard]] BuilderItem verticalTable(uint32 columns = GuiLayoutTableComponent().sections, bool grid = GuiLayoutTableComponent().grid);
			[[nodiscard]] BuilderItem leftRight();
			[[nodiscard]] BuilderItem rightLeft();
			[[nodiscard]] BuilderItem topBottom();
			[[nodiscard]] BuilderItem bottomTop();
			[[nodiscard]] BuilderItem panel();
			[[nodiscard]] BuilderItem spoiler(bool collapsed = GuiSpoilerComponent().collapsed, bool collapsesSiblings = GuiSpoilerComponent().collapsesSiblings);

			BuilderItem empty();
			BuilderItem empty(uint32 entityName);
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
		};
	}

	using guiBuilder::GuiBuilder;

	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(EntityManager *gui);
	CAGE_ENGINE_API Holder<GuiBuilder> newGuiBuilder(Entity *root);
}

#endif // guard_guiBuilder_sxrdk487
