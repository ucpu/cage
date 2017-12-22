#include <atomic>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/noise.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/utility/cameraController.h>
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;

holder<cameraControllerClass> cameraController;
real boxesCount;
real cameraRange;
bool shadowEnabled;
std::atomic<bool> regenerate;

bool windowClose(windowClass *)
{
	engineStop();
	return false;
}

bool keyRelease(windowClass *, uint32 a, uint32 b, modifiersFlags modifiers)
{
	return false;
}

bool guiUpdate();

bool update()
{
	uint64 time = currentControlTime();
	entityManagerClass *ents = entities();

	if (regenerate)
	{
		regenerate = false;
		ents->getAllEntities()->destroyAllEntities();

		{ // camera
			entityClass *e = ents->newEntity(1);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.orientation = quat(degs(-30), degs(), degs());
			ENGINE_GET_COMPONENT(camera, c, e);
			c.ambientLight = vec3(1, 1, 1) * 0.1;
			cameraController->setEntity(e);
		}

		{ // light
			entityClass *e = ents->newEntity(2);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.orientation = quat(degs(-30), degs(-110), degs());
			ENGINE_GET_COMPONENT(light, l, e);
			l.lightType = lightTypeEnum::Directional;
			l.color = vec3(1, 1, 1) * 0.9;
			if (shadowEnabled)
			{
				ENGINE_GET_COMPONENT(shadowmap, s, e);
				s.worldRadius = vec3(20, 20, 20);
				s.resolution = 1024;
			}
		}

		// boxes
		uint32 side = numeric_cast<uint32>(sqrt(boxesCount * 100000));
#ifdef CAGE_DEBUG
		side /= 3;
#endif // CAGE_DEBUG

		for (uint32 y = 0; y < side; y++)
		{
			for (uint32 x = 0; x < side; x++)
			{
				entityClass *e = ents->newEntity();
				ENGINE_GET_COMPONENT(transform, t, e);
				t.scale = 0.15 * 0.49;
				t.position = vec3((x - side * 0.5) * 0.15, 0, (y - side * 0.5) * 0.15);
				ENGINE_GET_COMPONENT(render, r, e);
				r.object = 1;
			}
		}
	}

	{ // camera
		entityClass *e = ents->getEntity(1);
		ENGINE_GET_COMPONENT(camera, c, e);
		c.far = c.near + cameraRange * 25 + 1;
	}

	{ // update boxes
		uint32 bc = entities()->getAllEntities()->entitiesCount();
		entityClass *const *bs = entities()->getAllEntities()->entitiesArray();
		for (uint32 i = 0; i < bc; i++)
		{
			if (!bs[i]->hasComponent(renderComponent::component))
				continue;
			ENGINE_GET_COMPONENT(transform, t, bs[i]);
			t.position[1] = noiseClouds(1, vec3(vec2(t.position[0], t.position[2]) * 0.15, time * 5e-8)) * 2 - 3;
		}
	}

	guiUpdate();

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

	{ // boxes count
		entityClass *lay = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lay);
			child.parent = layout->getName();
			child.ordering = 1;
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
			t.value = "boxes: ";
		}
		entityClass *con = g->entities()->newEntity(1);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = lay->getName();
			child.ordering = 2;
			GUI_GET_COMPONENT(control, c, con);
			c.controlType = controlTypeEnum::Slider;
			c.fval = .2f;
			GUI_GET_COMPONENT(position, p, con);
			p.w = 200;
			p.wUnit = unitsModeEnum::Pixels;
		}
	}

	{ // camera range
		entityClass *lay = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lay);
			child.parent = layout->getName();
			child.ordering = 2;
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
			t.value = "camera range: ";
		}
		entityClass *con = g->entities()->newEntity(2);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = lay->getName();
			child.ordering = 2;
			GUI_GET_COMPONENT(control, c, con);
			c.controlType = controlTypeEnum::Slider;
			c.fval = 1.f;
			GUI_GET_COMPONENT(position, p, con);
			p.w = 200;
			p.wUnit = unitsModeEnum::Pixels;
		}
	}

	{ // shadow
		entityClass *lay = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lay);
			child.parent = layout->getName();
			child.ordering = 4;
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
			t.value = "enable shadow: ";
		}
		entityClass *con = g->entities()->newEntity(4);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = lay->getName();
			child.ordering = 2;
			GUI_GET_COMPONENT(control, c, con);
			c.controlType = controlTypeEnum::Checkbox;
			c.ival = 1;
		}
	}

	return false;
}

bool guiUpdate()
{
	{ // update boxes count
		entityClass *e = cage::gui()->entities()->getEntity(1);
		GUI_GET_COMPONENT(control, c, e);
		if (c.fval != boxesCount)
		{
			boxesCount = c.fval;
			regenerate = true;
		}
	}

	{ // update camera range
		entityClass *e = cage::gui()->entities()->getEntity(2);
		GUI_GET_COMPONENT(control, c, e);
		cameraRange = c.fval;
	}

	{ // update enable shadow
		entityClass *e = cage::gui()->entities()->getEntity(4);
		GUI_GET_COMPONENT(control, c, e);
		if (!!c.ival != shadowEnabled)
		{
			shadowEnabled = !!c.ival;
			regenerate = true;
		}
	}

	return false;
}

int main(int argc, char *args[])
{
	try
	{
		// log to console
		holder <loggerClass> log1 = newLogger();
		log1->filter.bind<logFilterPolicyPass>();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		configSetBool("cage-client.engine.debugRenderMissingMeshes", true);
		engineInitialize(engineCreateConfig());

		// events
#define GCHL_GENERATE(TYPE, FUNC, EVENT) eventListener<bool TYPE> CAGE_JOIN(FUNC, Listener); CAGE_JOIN(FUNC, Listener).bind<&FUNC>(); CAGE_JOIN(FUNC, Listener).attach(EVENT);
		GCHL_GENERATE((), update, controlThread::update);
		GCHL_GENERATE((), guiInit, controlThread::initialize);
#undef GCHL_GENERATE
		eventListener<bool(windowClass *)> windowCloseListener;
		eventListener<bool(windowClass *, uint32 key, uint32, modifiersFlags modifiers)> keyReleaseListener;
		windowCloseListener.bind<&windowClose>();
		keyReleaseListener.bind<&keyRelease>();
		windowCloseListener.attach(window()->events.windowClose);
		keyReleaseListener.attach(window()->events.keyRelease);

		window()->modeSetWindowed(windowFlags::Border | windowFlags::Resizeable);
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test cpu performance (wave)");
		regenerate = true;

		cameraController = newCameraController();
		cameraController->movementSpeed = 0.1;
		cameraController->mouseButton = mouseButtonsFlags::Middle;

		holder<engineProfilingClass> engineProfiling = newEngineProfiling();

		engineStart();
		engineFinalize();

		cameraController.clear();

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
