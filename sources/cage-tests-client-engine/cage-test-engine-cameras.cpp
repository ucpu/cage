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
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;
static const uint32 assetsName = hashString("cage/tests/room/room.pack");
static const uint32 screenName = hashString("cage/tests/room/tvscreen.jpg");
static const uint32 roomName = hashString("cage/tests/room/room.object");

holder<textureClass> fabScreenTex;

bool windowClose()
{
	engineStop();
	return false;
}

bool graphicInitialize()
{
	fabScreenTex = newTexture(window());
	fabScreenTex->image2d(400, 250, GL_RGB8, GL_RGB, GL_BYTE, nullptr);
	fabScreenTex->filters(GL_LINEAR, GL_LINEAR, 0);
	assets()->set<assetSchemeIndexTexture>(screenName, fabScreenTex.get());
	{
		entityClass *e = entities()->getEntity(4);
		ENGINE_GET_COMPONENT(camera, c, e);
		c.target = fabScreenTex.get();
	}
	return false;
}

bool graphicFinalize()
{
	assets()->set<assetSchemeIndexTexture>(screenName, (textureClass*)nullptr);
	fabScreenTex.clear();
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
		GCHL_GENERATE((), windowClose, window()->events.windowClose);
		GCHL_GENERATE((), graphicInitialize, graphicDispatchThread::initialize);
		GCHL_GENERATE((), graphicFinalize, graphicDispatchThread::finalize);
#undef GCHL_GENERATE

		// window
		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test render to texture (room)");

		// screen
		assets()->fabricate(assetSchemeIndexTexture, screenName, "fab tv screen");

		// entities
		entityManagerClass *ents = entities();
		{ // room
			entityClass *e = ents->newEntity(10);
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = roomName;
			r.renderMask = 3;
			ENGINE_GET_COMPONENT(transform, t, e);
            (void)t;
		}
		entityClass *eye = nullptr;
		{ // eye
			entityClass *e = eye = ents->newEntity(1);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(0, 1.7, 0);
			ENGINE_GET_COMPONENT(camera, c, e);
			c.ambientLight = vec3(1, 1, 1);
			c.near = 0.2;
			c.far = 100;
			c.cameraOrder = 3;
			c.renderMask = 1;
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/room/eye.obj");
			r.renderMask = 2;
		}
		{ // camera stand
			entityClass *e = ents->newEntity(2);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(2.5, 2.5, -1.3);
			t.orientation = quat(degs(), degs(100), degs());
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/room/camera.obj?camera_stand");
			r.renderMask = 1;
		}
		{ // camera lens
			entityClass *e = ents->newEntity(3);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(2.5, 2.5, -1.3);
			t.orientation = quat(degs(-25), degs(100), degs());
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/room/camera.obj?lens");
			r.renderMask = 1;
		}
		{ // camera body
			entityClass *e = ents->newEntity(4);
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = vec3(2.5, 2.5, -1.3);
			t.orientation = quat(degs(-25), degs(100), degs());
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("cage/tests/room/camera.obj?camera");
			r.renderMask = 1;
			ENGINE_GET_COMPONENT(camera, c, e);
			c.ambientLight = vec3(1, 1, 1) * 0.7;
			c.near = 0.2;
			c.far = 100;
			c.cameraOrder = 2;
			c.renderMask = 2;
		}
		holder<cameraControllerClass> cameraController = newCameraController(eye);
		cameraController->mouseButton = mouseButtonsFlags::Left;
		cameraController->movementSpeed = 0.1;
		holder<engineProfilingClass> engineProfiling = newEngineProfiling();

		assets()->add(assetsName);
		engineStart();
		assets()->remove(assetsName);
		assets()->remove(screenName);
		engineFinalize();

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
