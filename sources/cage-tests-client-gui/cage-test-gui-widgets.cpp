#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/color.h>
#include <cage-core/utility/png.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;

bool windowClose()
{
	engineStop();
	return false;
}

bool update()
{
	entityManagerClass *ents = gui()->entities();

	{ // update disabled state
		entityClass *ae = ents->getEntity(100);
		GUI_GET_COMPONENT(checkBox, chb, ae);
		entityClass *be = ents->getEntity(3);
		GUI_GET_COMPONENT(widgetState, ws, be);
		ws.disabled = chb.state == checkBoxStateEnum::Checked;
	}

	return false;
}

bool guiEvent(uint32 name)
{
	CAGE_LOG(severityEnum::Info, "gui event", string() + "gui event on entity: " + name);

	entityManagerClass *ents = gui()->entities();

	{
		entityClass *e = ents->getEntity(name);

		if (e->hasComponent(gui()->components().checkBox))
		{
			GUI_GET_COMPONENT(checkBox, c, e);
			CAGE_LOG(severityEnum::Info, "gui event", string() + "check box state: " + (uint32)c.state);
		}

		if (e->hasComponent(gui()->components().colorPicker))
		{
			GUI_GET_COMPONENT(colorPicker, c, e);
			CAGE_LOG(severityEnum::Info, "gui event", string() + "color picker: " + c.color);
		}

		if (e->hasComponent(gui()->components().comboBox))
		{
			GUI_GET_COMPONENT(comboBox, c, e);
			CAGE_LOG(severityEnum::Info, "gui event", string() + "combo box selected: " + c.selected);
		}

		if (e->hasComponent(gui()->components().inputBox))
		{
			GUI_GET_COMPONENT(inputBox, c, e);
			CAGE_LOG(severityEnum::Info, "gui event", string() + "input box valid: " + c.valid + ", value: " + c.value);
		}

		if (e->hasComponent(gui()->components().sliderBar))
		{
			GUI_GET_COMPONENT(sliderBar, c, e);
			CAGE_LOG(severityEnum::Info, "gui event", string() + "slider bar value: " + c.value);
		}
	}

	switch (name)
	{
	case 110: // smaller
		gui()->setZoom(gui()->getZoom() / 1.1);
		break;
	case 111: // larger
		gui()->setZoom(gui()->getZoom() * 1.1);
		break;
	}

	return false;
}

void guiLabel(uint32 parentName, uint32 &index, const string &name)
{
	entityManagerClass *ents = gui()->entities();
	entityClass *e = ents->newEntity(ents->generateUniqueName());
	GUI_GET_COMPONENT(parent, p, e);
	p.parent = parentName;
	p.order = index++;
	GUI_GET_COMPONENT(label, l, e);
	GUI_GET_COMPONENT(text, t, e);
	t.value = name;
}

uint32 guiPanel(uint32 parentName, uint32 &index, const string &name)
{
	entityManagerClass *ents = gui()->entities();
	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	GUI_GET_COMPONENT(parent, p, panel);
	p.parent = parentName;
	p.order = index++;
	GUI_GET_COMPONENT(groupBox, g, panel);
	g.type = groupBoxTypeEnum::Spoiler;
	g.spoilerCollapsed = true;
	g.spoilerCollapsesSiblings = true;
	GUI_GET_COMPONENT(text, t, panel);
	t.value = name;
	GUI_GET_COMPONENT(layoutTable, lt, panel);
	lt.expandToSameWidth = false;
	lt.expandToSameHeight = false;
	lt.cellsAnchor = vec2();
	return panel->getName();
}

void guiButtons(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // with text
		guiLabel(parentName, index, "with text");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(button, b, e);
		GUI_GET_COMPONENT(text, t, e);
		t.value = "text";
	}
	{ // horizontal
		guiLabel(parentName, index, "horizontal");
		entityClass *layout = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, layout);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(layoutLine, l, layout);
		for (uint32 i = 0; i < 4; i++)
		{
			entityClass *e = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.order = index++;
			GUI_GET_COMPONENT(button, b, e);
			b.allowMerging = true;
			GUI_GET_COMPONENT(text, t, e);
			t.value = i;
		}
	}
	{ // vertical
		guiLabel(parentName, index, "vertical");
		entityClass *layout = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, layout);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(layoutLine, l, layout);
		l.vertical = true;
		for (uint32 i = 0; i < 4; i++)
		{
			entityClass *e = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.order = index++;
			GUI_GET_COMPONENT(button, b, e);
			b.allowMerging = true;
			GUI_GET_COMPONENT(text, t, e);
			t.value = i;
		}
	}
	{ // with image
		guiLabel(parentName, index, "with image");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(button, b, e);
		GUI_GET_COMPONENT(image, m, e);
		m.textureName = hashString("cage/texture/helper.jpg");
		m.textureUvOffset = vec2(5 / 8.f, 2 / 8.f);
		m.textureUvSize = vec2(1 / 8.f, 1 / 8.f);
	}
}

void guiInputBoxes(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // default
		guiLabel(parentName, index, "default");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
	}

	{ // integer
		guiLabel(parentName, index, "integer");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Integer;
		ib.min.i = -13;
		ib.max.i = 42;
		ib.step.i = 5;
	}

	{ // real
		guiLabel(parentName, index, "real");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Real;
		ib.min.f = -1;
		ib.max.f = 1;
		ib.step.f = 0.1;
	}

	{ // password
		guiLabel(parentName, index, "password");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Password;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "password";
	}

	{ // email
		guiLabel(parentName, index, "email");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Email;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "@";
	}

	{ // url
		guiLabel(parentName, index, "url");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Url;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "http://";
	}
}

void guiCheckBoxes(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // default
		guiLabel(parentName, index, "default");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(checkBox, cb, e);
	}

	{ // with label
		guiLabel(parentName, index, "with label");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(checkBox, cb, e);
		GUI_GET_COMPONENT(text, t, e);
		t.value = "label";
	}

	{ // checked
		guiLabel(parentName, index, "checked");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(checkBox, cb, e);
		cb.state = checkBoxStateEnum::Checked;
	}

	{ // indeterminate
		guiLabel(parentName, index, "indeterminate");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(checkBox, cb, e);
		cb.state = checkBoxStateEnum::Indeterminate;
		cb.type = checkBoxTypeEnum::TriStateBox;
	}

	{ // radios
		guiLabel(parentName, index, "radio buttons");
		entityClass *layout = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, layout);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(layoutLine, l, layout);
		l.vertical = true;
		for (uint32 i = 0; i < 4; i++)
		{
			entityClass *e = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.order = index++;
			GUI_GET_COMPONENT(checkBox, b, e);
			b.type = checkBoxTypeEnum::RadioButton;
			GUI_GET_COMPONENT(text, t, e);
			t.value = i;
		}
	}
}

void guiCombos(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // empty
		guiLabel(parentName, index, "empty");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(comboBox, cb, e);
		GUI_GET_COMPONENT(text, t, e);
		t.value = "placeholder";
	}
	{ // items
		guiLabel(parentName, index, "items");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(comboBox, cb, e);
		GUI_GET_COMPONENT(text, t, e);
		t.value = "select one:";
		for (uint32 i = 0; i < 4; i++)
		{
			entityClass *o = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, o);
			p.parent = e->getName();
			p.order = index++;
			GUI_GET_COMPONENT(text, t, o);
			t.value = string("option ") + i;
		}
	}
	{ // items
		guiLabel(parentName, index, "preselected");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(comboBox, cb, e);
		cb.selected = 2;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "select one:";
		for (uint32 i = 0; i < 4; i++)
		{
			entityClass *o = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, p, o);
			p.parent = e->getName();
			p.order = index++;
			GUI_GET_COMPONENT(text, t, o);
			t.value = string("option ") + i;
		}
	}
}

void guiSliders(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // default
		guiLabel(parentName, index, "default");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(sliderBar, s, e);
	}

	{ // vertical
		guiLabel(parentName, index, "vertical");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(sliderBar, s, e);
		s.vertical = true;
	}
}

void guiColorPickers(uint32 parentName)
{
	entityManagerClass *ents = gui()->entities();
	uint32 index = 1;

	{ // small
		guiLabel(parentName, index, "small");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(colorPicker, cp, e);
		cp.collapsible = true;
		cp.color = convertToRainbowColor(random());
	}

	{ // large
		guiLabel(parentName, index, "large");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentName;
		p.order = index++;
		GUI_GET_COMPONENT(colorPicker, cp, e);
		cp.collapsible = false;
		cp.color = convertToRainbowColor(random());
	}
}

void guiInitialize()
{
	entityManagerClass *ents = gui()->entities();

	{ // splitter
		entityClass *split = ents->newEntity(1);
		GUI_GET_COMPONENT(layoutSplitter, ls, split);
		ls.vertical = true;
		GUI_GET_COMPONENT(position, ep, split);
		ep.size.value = vec2(1, 1);
		ep.size.units[0] = unitEnum::ScreenWidth;
		ep.size.units[1] = unitEnum::ScreenHeight;
	}
	{ // top panel
		entityClass *panel = ents->newEntity(2);
		GUI_GET_COMPONENT(parent, p, panel);
		p.parent = 1;
		p.order = 1;
		GUI_GET_COMPONENT(groupBox, gp, panel);
		GUI_GET_COMPONENT(layoutSplitter, ls, panel);

		{ // disable widgets
			entityClass *e = ents->newEntity(100);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = 2;
			p.order = 1;
			GUI_GET_COMPONENT(checkBox, chb, e);
			GUI_GET_COMPONENT(text, txt, e);
			txt.value = "disable widgets";
		}
		{ // right splitter
			entityClass *rightSplitter = ents->newEntity(101);
			GUI_GET_COMPONENT(parent, p, rightSplitter);
			p.parent = 2;
			p.order = 2;
			GUI_GET_COMPONENT(layoutSplitter, ls, rightSplitter);
			ls.inverse = true;
			{ // middle line
				entityClass *middleLine = ents->newEntity(102);
				GUI_GET_COMPONENT(parent, p, middleLine);
				p.parent = 101;
				p.order = 1;
				GUI_GET_COMPONENT(layoutLine, ll, middleLine);
			}
			{ // right line
				entityClass *rightLine = ents->newEntity(103);
				GUI_GET_COMPONENT(parent, p, rightLine);
				p.parent = 101;
				p.order = 2;
				GUI_GET_COMPONENT(layoutLine, ll, rightLine);
				{ // smaller
					entityClass *e = ents->newEntity(110);
					GUI_GET_COMPONENT(parent, p, e);
					p.parent = 103;
					p.order = 1;
					GUI_GET_COMPONENT(button, b, e);
					GUI_GET_COMPONENT(text, t, e);
					t.value = "S";
					GUI_GET_COMPONENT(position, ep, e);
					ep.size.units[0] = ep.size.units[1] = unitEnum::Points;
					ep.size.value = vec2(32, 32);
				}
				{ // larger
					entityClass *e = ents->newEntity(111);
					GUI_GET_COMPONENT(parent, p, e);
					p.parent = 103;
					p.order = 2;
					GUI_GET_COMPONENT(button, b, e);
					GUI_GET_COMPONENT(text, t, e);
					t.value = "L";
					GUI_GET_COMPONENT(position, ep, e);
					ep.size.units[0] = ep.size.units[1] = unitEnum::Points;
					ep.size.value = vec2(32, 32);
				}
			}
		}
	}
	{ // bottom panel
		entityClass *panel = ents->newEntity(3);
		GUI_GET_COMPONENT(parent, p, panel);
		p.parent = 1;
		p.order = 2;
		GUI_GET_COMPONENT(layoutLine, ll, panel);
		ll.vertical = true;
		GUI_GET_COMPONENT(widgetState, ws, panel);
	}

	uint32 index = 1;
	guiButtons(guiPanel(3, index, "buttons"));
	guiInputBoxes(guiPanel(3, index, "input boxes"));
	guiCheckBoxes(guiPanel(3, index, "check boxes"));
	guiCombos(guiPanel(3, index, "combo boxes"));
	guiSliders(guiPanel(3, index, "sliders"));
	guiColorPickers(guiPanel(3, index, "color pickers"));
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

		engineInitialize(engineCreateConfig());
		detail::guiSkinTemplateExport(gui()->skin(), 2048)->encodeFile("guiSkinLayout.png");

		// events
#define GCHL_GENERATE(TYPE, FUNC, EVENT) eventListener<bool TYPE> CAGE_JOIN(FUNC, Listener); CAGE_JOIN(FUNC, Listener).bind<&FUNC>(); CAGE_JOIN(FUNC, Listener).attach(EVENT);
		GCHL_GENERATE((), windowClose, window()->events.windowClose);
		GCHL_GENERATE((), update, controlThread::update);
		GCHL_GENERATE((uint32), guiEvent, gui()->widgetEvent);
#undef GCHL_GENERATE

		// window
		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test gui widgets");

		guiInitialize();
		engineStart();
		engineFinalize();

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
