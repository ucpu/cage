#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/concurrent.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/textPack.h>

#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/assets.h>
#include <cage-client/gui.h>
#include <cage-client/opengl.h>
#include <cage-client/utility/highPerformanceGpuHint.h>


using namespace cage;

#define GUI_GET_COMPONENT(T,N,E) ::cage::CAGE_JOIN(T, Component) &N = E->value<::cage::CAGE_JOIN(T, Component)>(gui->components.T);

eventListener<bool(uint32)> guiEvent;
bool closing = false;
uint32 autoReload = 0;
const uint32 assetsName = hashString("cage/tests/gui/gui.pack");
holder<windowClass> window;
holder<guiClass> gui;

void clearScreen();
uint32 screenSwitches();
void generateScreen(uint32 screen);

bool guiFunction(uint32 en)
{
	CAGE_ASSERT_RUNTIME(en > 0);
	entityClass *e = gui->entities()->getEntity(en);
	if (e->hasComponent(gui->components.control))
	{
		GUI_GET_COMPONENT(control, c, e);
		CAGE_LOG(severityEnum::Info, "event", string() + "generic event, entity: " + en + ", control: " + (uint32)c.controlType + ", ival: " + c.ival + ", fval: " + c.fval);
	}
	if (e->hasComponent(gui->components.selection))
	{
		GUI_GET_COMPONENT(selection, s, e);
		CAGE_LOG(severityEnum::Info, "event", string() + "generic event, entity: " + en + ", selection start: " + s.selectionStart + ", length: " + s.selectionLength);
	}
	if (en < 100)
	{
		clearScreen();
		screenSwitches();
		generateScreen(en);
	}
	return true;
}

void clearScreen()
{
	gui->genericEvent.detach();
	entityManagerClass *ents = gui->entities();
	ents->getAllEntities()->destroyAllEntities();

	entityClass *screen = ents->newEntity(200);
	{
		GUI_GET_COMPONENT(layout, layout, screen);
		layout.layoutMode = layoutModeEnum::Row;
	}
}

uint32 screenSwitches()
{
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui->genericEvent);

	entityManagerClass *ents = gui->entities();

	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(control, control, panel);
		control.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(parent, parent, panel);
		parent.ordering = 1;
		parent.parent = 200;
	}

	entityClass *column = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, parent, column);
		parent.parent = panel->getName();
		GUI_GET_COMPONENT(layout, layout, column);
		layout.layoutMode = layoutModeEnum::Column;
	}

	static const string names[] = { "basic", "buttons", "text boxes", "combos", "lists", "sliders", "checkboxes", "radios", "ordering", "utf 8 test" };
	for (uint32 i = 0; i < sizeof(names) / sizeof(names[0]); i++)
	{
		entityClass *but = ents->newEntity(i + 1);
		GUI_GET_COMPONENT(parent, parent, but);
		parent.parent = column->getName();
		parent.ordering = i;
		GUI_GET_COMPONENT(control, control, but);
		control.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(text, txt, but);
		txt.value = names[i];
		GUI_GET_COMPONENT(format, f, but);
		f.align = textAlignEnum::Center;
	}

	return column->getName();
}

entityClass *prepareEntity(entityClass *column, uint32 index, const string &label)
{
	entityClass *row = gui->entities()->newEntity(gui->entities()->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, p, row);
		p.parent = column->getName();
		p.ordering = index;
		GUI_GET_COMPONENT(layout, l, row);
		l.layoutMode = layoutModeEnum::Row;
	}
	entityClass *lab = gui->entities()->newEntity(gui->entities()->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, p, lab);
		p.parent = row->getName();
		p.ordering = 1;
		GUI_GET_COMPONENT(control, c, lab);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, lab);
		t.value = label;
		GUI_GET_COMPONENT(position, q, lab);
		q.w = 200;
		q.wUnit = unitsModeEnum::Pixels;
	}
	entityClass *res = gui->entities()->newEntity(gui->entities()->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, p, res);
		p.parent = row->getName();
		p.ordering = 2;
	}
	return res;
}

void generateEmpties(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "empty with default format text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "default format text";
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with red text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "red text";
		GUI_GET_COMPONENT(format, f, control);
		f.color = vec3(1, 0, 0);
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with right aligned text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "right aligned text";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Right;
		GUI_GET_COMPONENT(position, p, control);
		p.w = 500;
		p.wUnit = unitsModeEnum::Pixels;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with center aligned text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "center aligned text";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Center;
		GUI_GET_COMPONENT(position, p, control);
		p.w = 500;
		p.wUnit = unitsModeEnum::Pixels;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with large font text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "large font text";
		GUI_GET_COMPONENT(format, f, control);
		f.fontName = hashString("cage/font/roboto.ttf?20");
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with different font");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "different font with more decorations";
		GUI_GET_COMPONENT(format, f, control);
		f.fontName = hashString("cage/tests/gui/immortal.ttf?14");
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with multiline text");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "first line\nmultiline\ntext\n...\nlast line";
	}
	{
		entityClass *control = prepareEntity(layout, index++, "empty with image");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(image, m, control);
		m.textureName = hashString("cage/texture/helper.jpg");
		GUI_GET_COMPONENT(position, p, control);
		p.w = 80;
		p.h = 80;
		p.wUnit = p.hUnit = unitsModeEnum::Pixels;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "hidden panel");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Panel;
		c.state = controlStateEnum::Hidden;
		GUI_GET_COMPONENT(position, p, control);
		p.w = 80;
		p.h = 80;
		p.wUnit = p.hUnit = unitsModeEnum::Pixels;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "panel with image (whole)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(image, m, control);
		m.textureName = hashString("cage/texture/helper.jpg");
		GUI_GET_COMPONENT(position, p, control);
		p.w = 80;
		p.h = 80;
		p.wUnit = p.hUnit = unitsModeEnum::Pixels;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "panel with image (blue 4)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(image, m, control);
		m.textureName = hashString("cage/texture/helper.jpg");
		m.tx = 3 / 8.f;
		m.ty = 7 / 8.f;
		m.tw = m.th = 1 / 8.f;
		GUI_GET_COMPONENT(position, p, control);
		p.w = 80;
		p.h = 80;
		p.wUnit = p.hUnit = unitsModeEnum::Pixels;
	}
}

void generateButtons(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "button 1 (text only)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 1";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Center;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "button 2 (text only)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 2";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Center;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "button 3 (text only)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 3";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Center;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "button 4 (image, green 6)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(image, m, control);
		m.textureName = hashString("cage/texture/helper.jpg");
		m.tx = 5 / 8.f;
		m.ty = 2 / 8.f;
		m.tw = m.th = 1 / 8.f;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "button 5 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Button;
		c.state = controlStateEnum::Disabled;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text";
		GUI_GET_COMPONENT(format, f, control);
		f.align = textAlignEnum::Center;
	}
}

void generateTexts(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "text box 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Textbox;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 1";
	}
	{
		entityClass *control = prepareEntity(layout, index++, "text box 2");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Textbox;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 2";
	}
	{
		entityClass *control = prepareEntity(layout, index++, "text box 3 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Textbox;
		c.state = controlStateEnum::Disabled;
		GUI_GET_COMPONENT(text, t, control);
		t.value = "text 3";
	}
	{
		entityClass *control = prepareEntity(layout, index++, "text area 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Textarea;
		for (uint32 i = 1; i < 4; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "line " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "text area 2 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Textarea;
		c.state = controlStateEnum::Disabled;
		for (uint32 i = 1; i < 4; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "line " + i;
		}
	}
}

void generateCombos(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "combo box 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Combobox;
		c.ival = 3;
		for (uint32 i = 1; i < 11; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "item " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "combo box 2");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Combobox;
		c.ival = 4;
		for (uint32 i = 1; i < 6; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "item " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "combo box 3 (unselected)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Combobox;
		c.ival = -1;
		for (uint32 i = 1; i < 3; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "item " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "combo box 4 (empty)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Combobox;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "combo box 5 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Combobox;
		c.state = controlStateEnum::Disabled;
		c.ival = 2;
		for (uint32 i = 1; i < 3; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "item " + i;
		}
	}
}

void generateLists(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "list box 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Listbox;
		for (uint32 i = 1; i < 6; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "line " + i;
			if (i == 3 || i == 5)
			{
				GUI_GET_COMPONENT(selection, s, line);
				s.selectionLength = 1;
			}
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "list box 2");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Listbox;
		for (uint32 i = 1; i < 4; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "line " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "list box 3 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Listbox;
		c.state = controlStateEnum::Disabled;
		for (uint32 i = 1; i < 3; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			t.value = string() + "line " + i;
		}
	}
	{
		entityClass *control = prepareEntity(layout, index++, "list box 4 (empty)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Listbox;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "list box 5 (long line)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Listbox;
		for (uint32 i = 1; i < 4; i++)
		{
			entityClass *line = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, line);
			p.parent = control->getName();
			p.ordering = i;
			GUI_GET_COMPONENT(text, t, line);
			if (i == 2)
				t.value = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Etiam ligula pede, sagittis quis, interdum ultricies, scelerisque eu. In enim a arcu imperdiet malesuada.";
			else
				t.value = string() + "line " + i;
		}
	}
}

void generateSliders(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "slider 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Slider;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "slider 2 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Slider;
		c.state = controlStateEnum::Disabled;
		c.fval = .4f;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "slider 3 (vertical)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Slider;
		GUI_GET_COMPONENT(position, p, control);
		p.w = 32;
		p.wUnit = unitsModeEnum::Pixels;
		p.h = 200;
		p.hUnit = unitsModeEnum::Pixels;
	}
}

void generateCheckboxes(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *control = prepareEntity(layout, index++, "checkbox 1");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Checkbox;
		c.ival = 1;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "checkbox 2");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Checkbox;
		c.ival = 0;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "checkbox 3");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Checkbox;
		c.ival = 2;
	}
	{
		entityClass *control = prepareEntity(layout, index++, "checkbox 4 (disabled)");
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Checkbox;
		c.state = controlStateEnum::Disabled;
	}
}

void generateRadios(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	{
		entityClass *groupControl = prepareEntity(layout, index++, "radio group 1");
		{
			GUI_GET_COMPONENT(control, c, groupControl);
			c.controlType = controlTypeEnum::RadioGroup;
		}
		entityClass *groupLayout = gui->entities()->newEntity(gui->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, p, groupLayout);
			p.parent = groupControl->getName();
			GUI_GET_COMPONENT(layout, l, groupLayout);
			l.layoutMode = layoutModeEnum::Row;
		}
		for (uint32 i = 0; i < 7; i++)
		{
			entityClass *control = gui->entities()->newEntity(gui->entities()->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, p, control);
				p.parent = groupLayout->getName();
				p.ordering = i;
				GUI_GET_COMPONENT(control, c, control);
				c.controlType = controlTypeEnum::RadioButton;
				switch (i)
				{
				case 2: c.ival = 1; break;
				case 4: c.state = controlStateEnum::Disabled; break;
				case 5: c.state = controlStateEnum::Hidden; break;
				}
			}
		}
	}
	{
		entityClass *groupControl = prepareEntity(layout, index++, "radio group 2");
		{
			GUI_GET_COMPONENT(control, c, groupControl);
			c.controlType = controlTypeEnum::RadioGroup;
		}
		entityClass *groupLayout = gui->entities()->newEntity(gui->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, p, groupLayout);
			p.parent = groupControl->getName();
			GUI_GET_COMPONENT(layout, l, groupLayout);
			l.layoutMode = layoutModeEnum::Column;
		}
		for (uint32 i = 0; i < 3; i++)
		{
			entityClass *control = gui->entities()->newEntity(gui->entities()->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, p, control);
				p.parent = groupLayout->getName();
				p.ordering = i;
				GUI_GET_COMPONENT(control, c, control);
				c.controlType = controlTypeEnum::RadioButton;
			}
		}
	}
}

void generateOrdering(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	sint32 order[] = { 5, 0, -1, 2, 6, -3, 4, 5, 3 };
	for (uint32 i = 0; i < sizeof(order) / sizeof(order[0]); i++)
	{
		entityClass *control = gui->entities()->newEntity(gui->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, p, control);
			p.parent = layout->getName();
			p.ordering = order[i];
			GUI_GET_COMPONENT(control, c, control);
			c.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, t, control);
			t.value = string() + "display order: " + order[i] + ", creation order: " + i;
		}
	}
}

void generateUtfTest(entityClass *layout, uint32 &index)
{
	entityManagerClass *ents = gui->entities();
	for (uint32 i = 0; i < 50; i++)
	{
		entityClass *control = gui->entities()->newEntity(gui->entities()->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, control);
		p.parent = layout->getName();
		p.ordering = i;
		GUI_GET_COMPONENT(control, c, control);
		c.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, t, control);
		t.assetName = hashString("cage/tests/gui/utf8test.textpack");
		uint32 line = (getApplicationTime() / 1000000 + i) % 291;
		t.textName = hashString((string() + "utf8test/" + line).c_str());
		GUI_GET_COMPONENT(format, f, control);
		f.fontName = hashString("cage/tests/gui/DroidSansMono.ttf?12");
	}
}

void generateScreen(uint32 screen)
{
	if (!autoReload)
		CAGE_LOG(severityEnum::Info, "screen", string() + "generating screen: " + screen);
	entityManagerClass *ents = gui->entities();

	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(control, control, panel);
		control.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(parent, parent, panel);
		parent.ordering = 2;
		parent.parent = 200;
	}

	entityClass *layout = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, p, layout);
		p.parent = panel->getName();
		GUI_GET_COMPONENT(layout, l, layout);
		l.layoutMode = layoutModeEnum::Column;
	}

	autoReload = 0;
	uint32 index = 1;
	switch (screen)
	{
	case 1:
		generateEmpties(layout, index);
		break;
	case 2:
		generateButtons(layout, index);
		break;
	case 3:
		generateTexts(layout, index);
		break;
	case 4:
		generateCombos(layout, index);
		break;
	case 5:
		generateLists(layout, index);
		break;
	case 6:
		generateSliders(layout, index);
		break;
	case 7:
		generateCheckboxes(layout, index);
		break;
	case 8:
		generateRadios(layout, index);
		break;
	case 9:
		generateOrdering(layout, index);
		break;
	case 10:
		generateUtfTest(layout, index);
		autoReload = screen;
		break;
	}
}

bool windowClose(windowClass *)
{
	closing = true;
	return true;
}

int main(int argc, char *args[])
{
	try
	{
		// log to console
		holder<loggerClass> log1 = newLogger();
		log1->filter.bind<logFilterPolicyPass>();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		// window
		window = newWindow();
		eventListener<bool()> applicationQuitListener;
		eventListener<bool(windowClass*)> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		window->events.windowClose.attach(windowCloseListener);
		window->title("cage test gui");

		// opengl context
		detail::initializeOpengl();

		// assets
		holder<assetManagerClass> assets = newAssetManager(assetManagerCreateConfig());
		assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(0));
		assets->defineScheme<shaderClass>(assetSchemeIndexShader, genAssetSchemeShader(0, window.get()));
		assets->defineScheme<textureClass>(assetSchemeIndexTexture, genAssetSchemeTexture(0, window.get()));
		assets->defineScheme<meshClass>(assetSchemeIndexMesh, genAssetSchemeMesh(0, window.get()));
		assets->defineScheme<fontClass>(assetSchemeIndexFont, genAssetSchemeFont(0, window.get()));
		assets->defineScheme<textPackClass>(assetSchemeIndexTextPackage, genAssetSchemeTextPackage(0));

		// load assets
		assets->add(assetsName);
		while (true)
		{
			if (assets->ready(assetsName))
				break;
			assets->processControlThread();
			assets->processCustomThread(0);
		}

		// gui
		{
			guiCreateConfig c;
			c.assetManager = assets.get();
			gui = newGui(c);
		}
		detail::guiElementsLayoutTemplateExport(gui->elements, 2048, "guiElementsLayoutTemplate.png");
		gui->handleWindowEvents(window.get());
		gui->graphicInitialize(window.get());
		clearScreen();
		screenSwitches();

		// show the window
		pointStruct res(1440, 920);
		window->windowedSize(res);
		window->modeSetWindowed(windowFlags::Border | windowFlags::Resizeable);
		window->processEvents();

		// loop
		while (!closing)
		{
			res = window->resolution();
			glViewport(0, 0, res.x, res.y);
			glClear(GL_COLOR_BUFFER_BIT);

			if (autoReload)
			{
				clearScreen();
				screenSwitches();
				generateScreen(autoReload);
			}

			gui->controlEmit();
			gui->graphicPrepare(getApplicationTime());
			gui->graphicDispatch();
			//gui->soundRender();

			threadSleep(20000);
			window->swapBuffers();
			window->processEvents();
		}
		clearScreen();

		// finalize gui
		gui->graphicFinalize();
		gui.clear();

		// unload assets
		assets->remove(assetsName);
		while (assets->countTotal())
		{
			assets->processControlThread();
			assets->processCustomThread(0);
		}

		// finalize window
		window.clear();

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
