#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/engine.h>
#include <cage-client/assets.h>

using namespace cage;

const uint32 assetsName = hashString("cage/tests/logo/logo.ogg");

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
		c.cameraType = cameraTypeEnum::Orthographic;
		c.orthographicSize = vec2(50, 50);
		c.near = -5;
		c.far = 5;
	}

	{ // listener
		entityClass *e = ents->newEntity(2);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.orientation = quat(degs(90), degs(), degs());
		ENGINE_GET_COMPONENT(render, r, e);
		r.object = 1;
		ENGINE_GET_COMPONENT(listener, l, e);
		l.volumeAttenuationByDistance = vec3(0, 0.1, 0.005);
	}

	static const vec3 boxPositions[] = { vec3(-1, -1, 0), vec3(1, -1, 0), vec3(-1, 1, 0), vec3(1, 1, 0) };
	for (uint32 i = 0; i < sizeof(boxPositions) / sizeof(boxPositions[0]); i++)
	{ // box
		entityClass *e = ents->newEntity(10 + i);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.position = boxPositions[i] * 30;
		ENGINE_GET_COMPONENT(render, r, e);
		r.object = 2;
		ENGINE_GET_COMPONENT(voice, s, e);
		s.sound = assetsName;
	}
}

bool update(uint64 time)
{
	entityManagerClass *ents = entities();
	{ // listener
		entityClass *e = ents->getEntity(2);
		ENGINE_GET_COMPONENT(transform, t, e);
		pointStruct cursor = window()->mousePosition();
		pointStruct resolution = window()->resolution();
		vec3 cur = vec3((real)cursor.x / (real)resolution.x, (real)1 - (real)cursor.y / (real)resolution.y, 0);
		t.position = 50 * (cur * 2 - 1) * vec3(1, 1, 0);
	}
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
#undef GCHL_GENERATE
		eventListener<bool()> applicationQuitListener;
		eventListener<bool(windowClass *)> windowCloseListener;
		applicationQuitListener.bind<&applicationQuit>();
		windowCloseListener.bind<&windowClose>();
		applicationQuitListener.attach(window()->events.applicationQuit);
		windowCloseListener.attach(window()->events.windowClose);

		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(600, 600));
		window()->title("cage test surround sound");
		controlInit();

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
