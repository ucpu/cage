#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/color.h>
#include <cage-core/utility/png.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/gui.h>
#include <cage-client/opengl.h>
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/cameraController.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;
static const uint32 assetsName = hashString("cage/tests/bottle/bottle.pack");

bool windowClose()
{
	engineStop();
	return false;
}

vec3 getGuiColor(uint32 id)
{
	entityClass *e = gui()->entities()->getEntity(id);
	GUI_GET_COMPONENT(colorPicker, c, e);
	return c.color;
}

vec3 getGuiOrientation(uint32 id)
{
	vec3 result;
	entityManagerClass *ents = gui()->entities();
	for (uint32 i = 0; i < 2; i++)
	{
		entityClass *e = ents->getEntity(id + i);
		GUI_GET_COMPONENT(sliderBar, c, e);
		result[i] = c.value;
	}
	return result;
}

bool update()
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
		t.orientation = quat(degs(o[0] - 90), degs(o[1]), degs());
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
	entityClass *e = gui()->entities()->newEntity(id);
	GUI_GET_COMPONENT(parent, p, e);
	p.parent = parentId;
	p.order = 5;
	GUI_GET_COMPONENT(colorPicker, c, e);
	c.color = convertHsvToRgb(hsv);
	c.collapsible = true;
}

void initializeGui()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *layout = ents->newEntity(1);
	{ // layout
		GUI_GET_COMPONENT(layoutLine, l, layout);
		l.vertical = true;
		l.expandToSameWidth = true;
	}

	{ // ambient
		entityClass *panel = ents->newEntity(20);
		{
			GUI_GET_COMPONENT(parent, p, panel);
			p.parent = layout->getName();
			p.order = 1;
			GUI_GET_COMPONENT(groupBox, c, panel);
			c.type = groupBoxTypeEnum::Spoiler;
			GUI_GET_COMPONENT(text, t, panel);
			t.value = "Ambient";
			GUI_GET_COMPONENT(layoutLine, l, panel);
			l.vertical = true;
		}
		initializeGuiColors(panel->getName(), 27, vec3(0, 0, 0.5));
	}

	vec3 colors[3] = {
		vec3(0.00, 1, 0.4),
		vec3(0.33, 1, 0.4),
		vec3(0.66, 1, 0.4)
	};
	real pitches[3] = { 0.25 * 90, 0.3 * 90, 0.55 * 90 };
	real yaws[3] = { 0.1 * 360, 0.2 * 360, 0.6 * 360 };
	for (uint32 i = 0; i < 3; i++)
	{ // spot lights
		entityClass *panel = ents->newEntity(30 + i * 10);
		{
			GUI_GET_COMPONENT(parent, p, panel);
			p.parent = layout->getName();
			p.order = 5 + i;
			GUI_GET_COMPONENT(groupBox, c, panel);
			c.type = groupBoxTypeEnum::Spoiler;
			GUI_GET_COMPONENT(text, t, panel);
			t.value = string() + "Spot light [" + i + "]:";
			GUI_GET_COMPONENT(layoutLine, l, panel);
			l.vertical = true;
		}
		{
			entityClass *e = ents->newEntity(33 + i * 10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = panel->getName();
			p.order = 2;
			GUI_GET_COMPONENT(sliderBar, c, e);
			c.value = pitches[i].value;
			c.min = 0;
			c.max = 90;
		}
		{
			entityClass *e = ents->newEntity(34 + i * 10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = panel->getName();
			p.order = 3;
			GUI_GET_COMPONENT(sliderBar, c, e);
			c.value = yaws[i].value;
			c.min = 0;
			c.max = 360;
		}
		initializeGuiColors(panel->getName(), 37 + i * 10, colors[i]);
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
		GCHL_GENERATE((), windowClose, window()->events.windowClose);
		GCHL_GENERATE((), update, controlThread::update);
#undef GCHL_GENERATE

		// window
		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test engine lights (bottle)");
		initializeGui();
		//gui()->setZoom(5);
		//detail::guiSkinTemplateExport(skinConfigStruct(), 1024)->encodeFile("guiSkinLayout.png");

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
