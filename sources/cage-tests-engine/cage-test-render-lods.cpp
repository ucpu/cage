#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/assets.h>
#include <cage-client/opengl.h>
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/cameraController.h>

using namespace cage;
static const uint32 assetsName = hashString("cage/tests/lods/lods.pack");

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

bool update(uint64)
{
	return false;
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
		GCHL_GENERATE((), applicationQuit, window()->events.applicationQuit);
		GCHL_GENERATE((windowClass *), windowClose, window()->events.windowClose);
		GCHL_GENERATE((uint64), update, controlThread::update);
#undef GCHL_GENERATE

		// window
		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test render lods (cubes and spheres)");

		// entities
		entityManagerClass *ents = entities();
		{ // cube
			entityClass *e = ents->newEntity(1);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/lods/cube.object");
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(-8, 0, 0);
		}
		{ // sphere
			entityClass *e = ents->newEntity(2);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/lods/sphere.object");
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(8, 0, 0);
		}
		{ // floor
			entityClass *e = ents->newEntity(3);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/lods/floor.object");
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(0, -5, 0);
		}
		{ // sun
			entityClass *e = ents->newEntity(9);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.orientation = quat(degs(-60), degs(20), degs());
			ENGINE_GET_COMPONENT(light, l, e);
			l.lightType = lightTypeEnum::Directional;
			ENGINE_GET_COMPONENT(shadowmap, s, e);
			s.resolution = 1024;
			s.worldRadius = vec3(20, 20, 20);
		}
		{ // camera
			entityClass *e = ents->newEntity(10);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(0, 3, 30);
			t.orientation = quat(degs(-10), degs(), degs());
			ENGINE_GET_COMPONENT(camera, c, e);
			c.ambientLight = vec3(1, 1, 1) * 0.5;
			c.near = 0.1;
			c.far = 200;
		}

		holder<cameraControllerClass> cameraController = newCameraController(ents->getEntity(10));
		cameraController->mouseButton = mouseButtonsFlags::Left;
		cameraController->movementSpeed = 1;
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

#include <cage-client/highPerformanceGpuHint.h>

