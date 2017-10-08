#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/utility/engineProfiling.h>

using namespace cage;

uint64 updateDelay = 0;
uint64 prepareDelay = 0;
uint64 renderDelay = 0;
uint64 soundDelay = 0;

bool applicationQuit()
{
	engineStop();
	return false;
}

bool windowClose(windowClass *)
{
	engineStop();
	return false;
}

void controlInit()
{
	entityManagerClass *ents = entities();
	{ // camera
		entityClass *e = ents->newEntity(1);
		ENGINE_GET_COMPONENT(transform, t, e);
		ENGINE_GET_COMPONENT(camera, c, e);
		c.ambientLight = vec3(1, 1, 1);
	}
	{ // box 1
		entityClass *e = ents->newEntity(2);
		ENGINE_GET_COMPONENT(transform, t, e);
		ENGINE_GET_COMPONENT(render, r, e);
		r.object = 1; // something non-existing
	}
}

bool update(uint64 time)
{
	entityManagerClass *ents = entities();
	{ // box 1
		entityClass *e = ents->getEntity(2);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.position = vec3(sin(rads(time * 1e-6)) * 10, cos(rads(time * 1e-6)) * 10, -20);
	}
	threadSleep(updateDelay);
	return false;
}

bool prepare()
{
	threadSleep(prepareDelay);
	return false;
}

bool render()
{
	threadSleep(renderDelay);
	return false;
}

bool soundUpdate()
{
	threadSleep(soundDelay);
	return false;
}

bool guiInit()
{
	guiClass *g = cage::gui();

	entityClass *panel = g->entities()->newEntity(g->entities()->generateUniqueName());
	entityClass *layout = g->entities()->newEntity(g->entities()->generateUniqueName());
	{
		GUI_GET_COMPONENT(control, c, panel);
		c.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(layout, l, layout);
		l.layoutMode = layoutModeEnum::Column;
		GUI_GET_COMPONENT(parent, child, layout);
		child.parent = panel->getName();
	}

	// controls
	static const string names[] = { "control tick", "sound tick", "control delay", "prepare delay", "dispatch delay", "sound delay" };
	static const uint64 values[] = { 1000 / 30, 1000 / 60, 0, 0, 0, 0 };
	CAGE_ASSERT_COMPILE(sizeof(names) / sizeof(names[0]) == sizeof(values) / sizeof(values[0]), arrays_must_have_same_length);
	for (uint32 i = 0; i < sizeof(names) / sizeof(names[0]); i++)
	{
		entityClass *lay = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lay);
			child.parent = layout->getName();
			child.ordering = i;
			GUI_GET_COMPONENT(layout, l, lay);
			l.layoutMode = layoutModeEnum::Row;
		}
		entityClass *lab = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lab);
			child.parent = lay->getName();
			child.ordering = 1;
			GUI_GET_COMPONENT(control, c, lab);
			c.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(position, p, lab);
			p.w = 200;
			p.wUnit = unitsModeEnum::Pixels;
			GUI_GET_COMPONENT(text, t, lab);
			t.value = names[i];
		}
		entityClass *con = g->entities()->newEntity(20 + i);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = lay->getName();
			child.ordering = 2;
			GUI_GET_COMPONENT(control, c, con);
			c.controlType = controlTypeEnum::Textbox;
			GUI_GET_COMPONENT(position, p, con);
			p.w = 200;
			p.wUnit = unitsModeEnum::Pixels;
			GUI_GET_COMPONENT(text, t, con);
			t.value = values[i];
		}
	}

	return false;
}

namespace
{
	void setIntValue(uint32 index, uint64 &value, bool allowZero)
	{
		entityClass *control = cage::gui()->entities()->getEntity(20 + index);
		GUI_GET_COMPONENT(text, t, control);
		GUI_GET_COMPONENT(format, f, control);
		if (t.value.isDigitsOnly() && !t.value.empty() && (t.value.toUint32() > 0 || allowZero))
		{
			f.color = vec3(0, 0, 0);
			value = t.value.toUint32() * 1000;
		}
		else
		{
			f.color = vec3(1, 0, 0);
		}
	}
}

bool gui(uint64)
{
	setIntValue(0, controlThread::tickTime, false);
	setIntValue(1, soundThread::tickTime, false);
	setIntValue(2, updateDelay, true);
	setIntValue(3, prepareDelay, true);
	setIntValue(4, renderDelay, true);
	setIntValue(5, soundDelay, true);
	return false;
}

int main(int argc, char *args[])
{
	try
	{
		// log to console
		holder <loggerClass> log1 = newLogger();
		log1->filter.bind <logFilterPolicyPass>();
		log1->format.bind <logFormatPolicyConsole>();
		log1->output.bind <logOutputPolicyStdOut>();

		configSetBool("cage-client.engine.debugRenderMissingMeshes", true);
		engineInitialize(engineCreateConfig());

		// events
#define GCHL_GENERATE(TYPE, FUNC, EVENT) eventListener<bool TYPE> CAGE_JOIN(FUNC, Listener); CAGE_JOIN(FUNC, Listener).bind<&FUNC>(); CAGE_JOIN(FUNC, Listener).attach(EVENT);
		GCHL_GENERATE((uint64), update, controlThread::update);
		GCHL_GENERATE((), prepare, graphicPrepareThread::prepare);
		GCHL_GENERATE((), render, graphicDispatchThread::render);
		GCHL_GENERATE((), soundUpdate, soundThread::sound);
		GCHL_GENERATE((), guiInit, controlThread::initialize);
		GCHL_GENERATE((uint64), gui, controlThread::update);
#undef GCHL_GENERATE
		eventListener<bool()> applicationQuitListener;
		eventListener<bool(windowClass *)> windowCloseListener;
		applicationQuitListener.bind<&applicationQuit>();
		windowCloseListener.bind<&windowClose>();
		applicationQuitListener.attach(window()->events.applicationQuit);
		windowCloseListener.attach(window()->events.windowClose);

		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test render interpolation (timing)");
		controlInit();
		holder<engineProfilingClass> engineProfiling = newEngineProfiling();

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

#include <cage-client/highPerformanceGpuHint.h>
