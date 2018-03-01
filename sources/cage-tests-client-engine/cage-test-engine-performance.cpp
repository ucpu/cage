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
uint32 boxesCount;
real cameraRange;
bool shadowEnabled;
std::atomic<bool> regenerate;

bool windowClose()
{
	engineStop();
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
		uint32 side = numeric_cast<uint32>(sqrt(boxesCount));
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
	{
		GUI_GET_COMPONENT(groupBox, c, panel);
		GUI_GET_COMPONENT(layoutTable, l, panel);
	}

	{ // boxes count
		entityClass *lab = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lab);
			child.parent = panel->getName();
			child.order = 1;
			GUI_GET_COMPONENT(label, c, lab);
			GUI_GET_COMPONENT(text, t, lab);
			t.value = "boxes: ";
		}
		entityClass *con = g->entities()->newEntity(1);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = panel->getName();
			child.order = 2;
			GUI_GET_COMPONENT(inputBox, c, con);
			c.type = inputTypeEnum::Integer;
			c.min.i = 100;
			c.max.i = 100000;
			c.step.i = 100;
			c.value = "5000";
		}
	}

	{ // camera range
		entityClass *lab = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lab);
			child.parent = panel->getName();
			child.order = 3;
			GUI_GET_COMPONENT(label, c, lab);
			GUI_GET_COMPONENT(text, t, lab);
			t.value = "camera range: ";
		}
		entityClass *con = g->entities()->newEntity(2);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = panel->getName();
			child.order = 4;
			GUI_GET_COMPONENT(sliderBar, c, con);
			c.value = 1.f;
		}
	}

	{ // shadow
		entityClass *lab = g->entities()->newEntity(g->entities()->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, child, lab);
			child.parent = panel->getName();
			child.order = 5;
			GUI_GET_COMPONENT(label, c, lab);
			GUI_GET_COMPONENT(text, t, lab);
			t.value = "enable shadow: ";
		}
		entityClass *con = g->entities()->newEntity(4);
		{
			GUI_GET_COMPONENT(parent, child, con);
			child.parent = panel->getName();
			child.order = 6;
			GUI_GET_COMPONENT(checkBox, c, con);
			c.state = checkBoxStateEnum::Checked;
		}
	}

	return false;
}

bool guiUpdate()
{
	{ // update boxes count
		entityClass *e = cage::gui()->entities()->getEntity(1);
		GUI_GET_COMPONENT(inputBox, c, e);
		if (c.valid && c.value.toUint32() != boxesCount)
		{
			boxesCount = c.value.toUint32();
			regenerate = true;
		}
	}

	{ // update camera range
		entityClass *e = cage::gui()->entities()->getEntity(2);
		GUI_GET_COMPONENT(sliderBar, c, e);
		cameraRange = c.value;
	}

	{ // update enable shadow
		entityClass *e = cage::gui()->entities()->getEntity(4);
		GUI_GET_COMPONENT(checkBox, c, e);
		bool checked = c.state == checkBoxStateEnum::Checked;
		if (checked != shadowEnabled)
		{
			shadowEnabled = checked;
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
		eventListener<bool()> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		windowCloseListener.attach(window()->events.windowClose);

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
