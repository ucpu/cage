#include <atomic>
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;

std::atomic<bool> dirty;
std::atomic<uint32> camsLayout;
std::atomic<bool> holes;

bool windowClose()
{
	engineStop();
	return false;
}

bool keyPress(uint32, uint32 b, modifiersFlags modifiers)
{
	switch (b)
	{
	case 65:
		camsLayout = camsLayout == 2 ? 0 : camsLayout + 1;
		dirty = true;
		return true;
	case 66:
		holes = !holes;
		dirty = true;
		return true;
	}
	return false;
}

void box(const vec3 &pos, const quat &rot)
{
	entityClass *e = entities()->newEntity();
	ENGINE_GET_COMPONENT(transform, t, e);
	ENGINE_GET_COMPONENT(render, r, e);
	t.position = pos;
	t.orientation = rot;
	r.object = 1;
	r.renderMask = 0b111 & ~(holes ? (1 << random(0, 3)) : 0);
}

void letter(char c, const vec3 &pos)
{
	switch (c)
	{
	case 'C':
		for (rads ang = degs(30); ang < degs(360 - 30); ang += degs(30))
			box(
				vec3(cos(ang), sin(ang) * 2, 0) * 3 + pos,
				quat(degs(), degs(), degs(ang))
			);
		break;
	case 'A':
		box(vec3(+0.0, +2, 0) * 3 + pos, quat(degs(), degs(), degs(+00)));
		box(vec3(-.25, +1, 0) * 3 + pos, quat(degs(), degs(), degs(-10)));
		box(vec3(+.25, +1, 0) * 3 + pos, quat(degs(), degs(), degs(+10)));
		box(vec3(-.50, +0, 0) * 3 + pos, quat(degs(), degs(), degs(-10)));
		box(vec3(+.50, +0, 0) * 3 + pos, quat(degs(), degs(), degs(+10)));
		box(vec3(-.75, -1, 0) * 3 + pos, quat(degs(), degs(), degs(-10)));
		box(vec3(-0.0, -1, 0) * 3 + pos, quat(degs(), degs(), degs(+00)));
		box(vec3(+.75, -1, 0) * 3 + pos, quat(degs(), degs(), degs(+10)));
		box(vec3(-1.0, -2, 0) * 3 + pos, quat(degs(), degs(), degs(-10)));
		box(vec3(+1.0, -2, 0) * 3 + pos, quat(degs(), degs(), degs(+10)));
		break;
	case 'G':
		for (rads ang = degs(30); ang < degs(360 - 30); ang += degs(30))
			box(
				vec3(cos(ang), sin(ang) * 2, 0) * 3 + pos,
				quat(degs(), degs(), degs(ang))
			);
		box(vec3(0.25, -0.5, 0) * 3 + pos, quat(degs(), degs(), degs(-20)));
		break;
	case 'E':
		box(vec3(-1, +2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+0, +2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+1, +2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(-1, +1, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(-1, +0, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+0, +0, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+1, +0, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(-1, -1, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(-1, -2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+0, -2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		box(vec3(+1, -2, 0) * 3 + pos, quat(degs(), degs(), degs()));
		break;
	}
}

void regenerate()
{
	entityManagerClass *ents = entities();
	ents->getAllEntities()->destroyAllEntities();

	for (uint32 i = 0; i < 3; i++)
	{ // camera
		entityClass *e = ents->newEntity(i + 1);
		ENGINE_GET_COMPONENT(transform, t, e);
        (void)t;
		ENGINE_GET_COMPONENT(camera, c, e);
		c.ambientLight = vec3(1, 1, 1) * 0.1;
		c.renderMask = 1 << i;
		c.cameraOrder = i;
		switch ((uint32)camsLayout)
		{
		case 0:
			c.viewportOrigin = vec2(i / 3.f, 0);
			c.viewportSize = vec2(1 / 3.f, 1);
			break;
		case 1:
			c.viewportOrigin = vec2(0, i / 3.f);
			c.viewportSize = vec2(1, 1 / 3.f);
			break;
		case 2: // rotating
			break;
		}
	}

	letter('C', vec3(-3, 0, +0) * 10);
	letter('A', vec3(-1, 0, -1) * 10);
	letter('G', vec3(+1, 0, -1) * 10);
	letter('E', vec3(+3, 0, +0) * 10);

	{ // light
		entityClass *e = ents->newEntity(10);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.orientation = randomDirectionQuat();
		ENGINE_GET_COMPONENT(light, l, e);
		l.lightType = lightTypeEnum::Directional;
		l.color = vec3(1, 1, 1) * 0.9;
		ENGINE_GET_COMPONENT(shadowmap, s, e);
		s.resolution = 512;
		s.worldRadius = vec3(50, 50, 50);
	}
}

bool update()
{
	uint64 time = currentControlTime();

	if (dirty)
	{
		dirty = false;
		regenerate();
	}

	entityManagerClass *ents = entities();

	if (camsLayout == 2)
	{ // rotating viewports
		for (uint32 i = 0; i < 3; i++)
		{
			entityClass *e = ents->getEntity(i + 1);
			ENGINE_GET_COMPONENT(camera, c, e);
			c.ambientLight = vec3(1, 1, 1) * 0.5;
			c.renderMask = 1 << i;
			c.cameraOrder = i;
			rads ang = degs(20 * time / 1e6 + i * 120);
			c.viewportOrigin = vec2(0.5 + cos(ang) * 0.5, 0.5 + sin(ang) * 0.5) * 0.75;
			c.viewportSize = vec2(0.25, 0.25);
		}
	}

	for (uint32 i = 0; i < 3; i++)
	{ // transformations of cameras
		entityClass *e = ents->getEntity(i + 1);
		ENGINE_GET_COMPONENT(transform, t, e);
		switch (i)
		{
		case 0:
			t.orientation = quat(degs(), degs(20 * time / 1e6), degs());
			t.position = vec3(0, 0, 50) * t.orientation;
			break;
		case 1:
			t.orientation = quat(degs(15 * time / 1e6), degs(), degs());
			t.position = vec3(0, 0, 50) * t.orientation;
			break;
		case 2:
			t.orientation = quat(degs(), degs(), degs(10 * time / 1e6));
			t.position = vec3(0, 0, 50) * t.orientation;
			break;
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
		eventListener<bool()> applicationQuitListener;
		eventListener<bool()> windowCloseListener;
		eventListener<bool(uint32 key, uint32, modifiersFlags modifiers)> keyPressListener;
		eventListener<bool()> updateListener;
		windowCloseListener.bind<&windowClose>();
		keyPressListener.bind<&keyPress>();
		updateListener.bind<&update>();
		windowCloseListener.attach(window()->events.windowClose);
		keyPressListener.attach(window()->events.keyPress);
		updateListener.attach(controlThread::update);

		window()->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));
		window()->windowedSize(pointStruct(800, 600));
		window()->title("cage test multiple viewports");
		dirty = true;
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
