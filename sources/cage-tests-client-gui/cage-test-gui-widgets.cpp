#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
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

bool guiEvent(uint32 e)
{
	CAGE_LOG(severityEnum::Info, "gui event", string() + "gui event on entity: " + e);
	return false;
}

void guiLabel(uint32 &index, const string &name)
{
	entityManagerClass *ents = gui()->entities();
	entityClass *e = ents->newEntity(ents->generateUniqueName());
	GUI_GET_COMPONENT(parent, p, e);
	p.parent = 3;
	p.order = index++;
	GUI_GET_COMPONENT(label, l, e);
	GUI_GET_COMPONENT(text, t, e);
	t.value = name;
}

void guiInputBox(uint32 &index)
{
	entityManagerClass *ents = gui()->entities();

	{ // default input box
		guiLabel(index, "default input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
	}

	{ // integer input box
		guiLabel(index, "integer input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Integer;
		ib.min.i = -13;
		ib.max.i = 42;
		ib.step.i = 5;
	}

	{ // real input box
		guiLabel(index, "real input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Real;
		ib.min.f = -1;
		ib.max.f = 1;
		ib.step.f = 0.1;
	}

	{ // password input box
		guiLabel(index, "password input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Password;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "password";
	}

	{ // email input box
		guiLabel(index, "email input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Email;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "@";
	}

	{ // url input box
		guiLabel(index, "url input box");
		entityClass *e = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 3;
		p.order = index++;
		GUI_GET_COMPONENT(inputBox, ib, e);
		ib.type = inputTypeEnum::Url;
		GUI_GET_COMPONENT(text, t, e);
		t.value = "http://";
	}
}

void guiInitialize()
{
	entityManagerClass *ents = gui()->entities();

	{ // splitter
		entityClass *split = ents->newEntity(1);
		GUI_GET_COMPONENT(layoutSplitter, ls, split);
		ls.vertical = true;
		GUI_GET_COMPONENT(explicitPosition, ep, split);
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
		GUI_GET_COMPONENT(layoutLine, ll, panel);
	}
	{ // bottom panel
		entityClass *panel = ents->newEntity(3);
		GUI_GET_COMPONENT(parent, p, panel);
		p.parent = 1;
		p.order = 2;
		GUI_GET_COMPONENT(groupBox, gp, panel);
		GUI_GET_COMPONENT(layoutTable, lt, panel);
		GUI_GET_COMPONENT(widgetState, ws, panel);
	}

	{ // disable widgets
		entityClass *e = ents->newEntity(100);
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = 2;
		p.order = 1;
		GUI_GET_COMPONENT(checkBox, chb, e);
		GUI_GET_COMPONENT(text, txt, e);
		txt.value = "disable widgets";
	}

	uint32 index = 1;
	guiInputBox(index);
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
