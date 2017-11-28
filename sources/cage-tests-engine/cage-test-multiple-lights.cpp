#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/color.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/assets.h>
#include <cage-client/gui.h>
#include <cage-client/opengl.h>
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/cameraController.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;
static const uint32 assetsName = hashString("cage/tests/bottle/bottle.pack");

bool windowClose(windowClass *)
{
	engineStop();
	return false;
}

vec3 getGuiColor(uint32 id)
{
	vec3 result;
	entityManagerClass *ents = gui()->entities();
	for (uint32 i = 0; i < 3; i++)
	{
		entityClass *e = ents->getEntity(id + i);
		GUI_GET_COMPONENT(control, c, e);
		result[i] = c.fval;
	}
	return convertHsvToRgb(result);
}

vec3 getGuiOrientation(uint32 id)
{
	vec3 result;
	entityManagerClass *ents = gui()->entities();
	for (uint32 i = 0; i < 2; i++)
	{
		entityClass *e = ents->getEntity(id + i);
		GUI_GET_COMPONENT(control, c, e);
		result[i] = c.fval;
	}
	return result;
}

bool update(uint64)
{
	entityManagerClass *ents = entities();

	{ // update ambient light
		entityClass *e = ents->getEntity(10);
		ENGINE_GET_COMPONENT(camera, c, e);
		c.ambientLight = getGuiColor(27);
	}

	for (uint32 i = 0; i < 3; i++)
	{ // update lights
		entityClass *e = ents->getEntity(5 + i);
		ENGINE_GET_COMPONENT(render, r, e);
		ENGINE_GET_COMPONENT(light, l, e);
		r.color = l.color = getGuiColor(37 + i * 10);
		ENGINE_GET_COMPONENT(transform, t, e);
		vec3 o = getGuiOrientation(33 + i * 10);
		t.orientation = quat(degs(o[0] * -90), degs(o[1] * 360), degs());
		t.position = vec3(0, 3, 0) + t.orientation * vec3(0, 0, 10);
	}

	{ // rotate the bottle
		entityClass *e = ents->getEntity(2);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.orientation = quat(degs(), degs(1), degs()) * t.orientation;
	}
	return false;
}

void initializeGuiColors(uint32 parentId, uint32 id, const vec3 &hsv)
{
	entityManagerClass *ents = gui()->entities();
	for (uint32 i = 0; i < 3; i++)
	{
		entityClass *e = ents->newEntity(id + i);
		GUI_GET_COMPONENT(parent, p, e);
		p.parent = parentId;
		p.ordering = 5 + i;
		GUI_GET_COMPONENT(control, c, e);
		c.controlType = controlTypeEnum::Slider;
		c.fval = hsv[i].value;
	}
}

void initializeGui()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *layout = ents->newEntity(1);
	{ // layout
		GUI_GET_COMPONENT(layout, l, layout);
		l.layoutMode = layoutModeEnum::Column;
	}

	{ // ambient
		entityClass *panel = ents->newEntity(20);
		{
			GUI_GET_COMPONENT(parent, p, panel);
			p.parent = layout->getName();
			p.ordering = 1;
			GUI_GET_COMPONENT(control, c, panel);
			c.controlType = controlTypeEnum::Panel;
		}
		entityClass *layout = ents->newEntity(21);
		{
			GUI_GET_COMPONENT(parent, p, layout);
			p.parent = panel->getName();
			GUI_GET_COMPONENT(layout, l, layout);
			l.layoutMode = layoutModeEnum::Column;
		}
		{
			entityClass *e = ents->newEntity(22);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.ordering = 1;
			GUI_GET_COMPONENT(control, c, e);
			c.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, t, e);
			t.value = "Ambient color:";
		}
		initializeGuiColors(layout->getName(), 27, vec3(0, 0, 0.5));
	}

	vec3 colors[3] = {
		vec3(0.00, 1, 0.4),
		vec3(0.33, 1, 0.4),
		vec3(0.66, 1, 0.4)
	};
	real pitches[3] = { 0.25, 0.3, 0.55 };
	real yaws[3] = { 0.1, 0.2, 0.6 };
	for (uint32 i = 0; i < 3; i++)
	{ // spot lights
		entityClass *panel = ents->newEntity(30 + i * 10);
		{
			GUI_GET_COMPONENT(parent, p, panel);
			p.parent = layout->getName();
			p.ordering = 5 + i;
			GUI_GET_COMPONENT(control, c, panel);
			c.controlType = controlTypeEnum::Panel;
		}
		entityClass *layout = ents->newEntity(31 + i * 10);
		{
			GUI_GET_COMPONENT(parent, p, layout);
			p.parent = panel->getName();
			GUI_GET_COMPONENT(layout, l, layout);
			l.layoutMode = layoutModeEnum::Column;
		}
		{
			entityClass *e = ents->newEntity(32 + i * 10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.ordering = 1;
			GUI_GET_COMPONENT(control, c, e);
			c.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, t, e);
			t.value = string() + "Spot light [" + i + "]:";
		}
		{
			entityClass *e = ents->newEntity(33 + i * 10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.ordering = 2;
			GUI_GET_COMPONENT(control, c, e);
			c.controlType = controlTypeEnum::Slider;
			c.fval = pitches[i].value;
		}
		{
			entityClass *e = ents->newEntity(34 + i * 10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = layout->getName();
			p.ordering = 3;
			GUI_GET_COMPONENT(control, c, e);
			c.controlType = controlTypeEnum::Slider;
			c.fval = yaws[i].value;
		}
		initializeGuiColors(layout->getName(), 37 + i * 10, colors[i]);
	}
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

		configSetBool("cage-client.engine.debugRenderMissingMeshes", true);
		engineInitialize(engineCreateConfig());

		// events
#define GCHL_GENERATE(TYPE, FUNC, EVENT) eventListener<bool TYPE> CAGE_JOIN(FUNC, Listener); CAGE_JOIN(FUNC, Listener).bind<&FUNC>(); CAGE_JOIN(FUNC, Listener).attach(EVENT);
		GCHL_GENERATE((windowClass *), windowClose, window()->events.windowClose);
		GCHL_GENERATE((uint64), update, controlThread::update);
#undef GCHL_GENERATE

		// window
		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test multiple lights (pbr bottle)");
		initializeGui();

		// entities
		entityManagerClass *ents = entities();
		{ // floor
			entityClass *e = ents->newEntity(1);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/bottle/other.obj?plane");
			ENGINE_GET_COMPONENT(transform, t, e);
            (void)t;
		}
		{ // bottle
			entityClass *e = ents->newEntity(2);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/bottle/bottle.obj");
			ENGINE_GET_COMPONENT(transform, t, e);
            (void)t;
		}
		for (uint32 i = 0; i < 3; i++)
		{ // spot lights
			entityClass *e = ents->newEntity(5 + i);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/bottle/other.obj?arrow");
			ENGINE_GET_COMPONENT(light, l, e);
			l.lightType = lightTypeEnum::Spot;
			l.spotAngle = degs(40);
			l.spotExponent = 2;
			ENGINE_GET_COMPONENT(shadowmap, s, e);
			s.resolution = 1024;
			s.worldRadius = vec3(3, 20, 0);
		}
		{ // camera
			entityClass *e = ents->newEntity(10);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(0, 5, 7);
			t.orientation = quat(degs(-10), degs(), degs());
			ENGINE_GET_COMPONENT(camera, c, e);
			c.near = 0.1;
			c.far = 150;
		}

		holder<cameraControllerClass> cameraController = newCameraController(ents->getEntity(10));
		cameraController->mouseButton = mouseButtonsFlags::Left;
		cameraController->movementSpeed = 0.3;
		holder<engineProfilingClass> engineProfiling = newEngineProfiling();

		assets()->add(assetsName);
		engineStart();
		assets()->remove(assetsName);
		engineFinalize();

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
