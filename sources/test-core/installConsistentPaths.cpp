#include <plf_colony.h>
#include <unordered_dense.h>

#include <cage-core/concurrent.h>
#include <cage-core/math.h>

#ifdef CAGE_ENGINE_API
	#include <cage-engine/inputs.h>
	#include <cage-engine/opengl.h>
	#include <cage-engine/window.h>
#endif // !CAGE_ENGINE_API

using namespace cage;

namespace
{
	bool closing = false;

	void testColony()
	{
		plf::colony<Vec3> colony;
		colony.reserve(100);
		colony.insert(Vec3(13));
		colony.insert(Vec3(42));
	}

	void testRobinHood()
	{
		ankerl::unordered_dense::map<uint32, Vec3> um;
		um[13] = Vec3(42);
	}
}

void testCageInstallConsistentPaths()
{
	testColony();
	testRobinHood();

#ifdef CAGE_ENGINE_API
	CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "creating window");
	Holder<Window> window = newWindow({});
	const auto closeListener = window->events.listen(inputFilter([](input::WindowClose) { closing = true; }));
	window->setWindowed();
	window->windowedSize(Vec2i(400, 300));
	window->title("cage-test-install");
	detail::initializeOpengl();

	CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "starting loop");
	while (!closing)
	{
		window->processEvents();
		Vec2i resolution = window->resolution();
		glViewport(0, 0, resolution[0], resolution[1]);
		Vec3 color = Vec3(1, 0, 0);
		glClearColor(color[0].value, color[1].value, color[2].value, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		window->swapBuffers();
		threadSleep(5000);
	}

	CAGE_LOG(SeverityEnum::Info, "test", Stringizer() + "finishing");
#endif // !CAGE_ENGINE_API
}
