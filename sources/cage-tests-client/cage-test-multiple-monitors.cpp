#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/timer.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/opengl.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;

void testScreen(const string &screenId, const pointStruct &resolution, uint32 frequency)
{
	CAGE_LOG(severityEnum::Info, "test", string() + "monitor: '" + screenId + "', resolution: " + resolution.x + " * " + resolution.y + ", frequency: " + frequency);
	{
		holder<windowClass> w = newWindow();
		w->modeSetFullscreen(resolution, frequency, screenId);
		w->title("cage test monitors");
		w->processEvents();
		detail::initializeOpengl();
		w->swapBuffers();
		w->processEvents();
		glViewport(0, 0, resolution.x, resolution.y);
		glClearColor(random().value, random().value, random().value, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		w->swapBuffers();
		w->processEvents();
		threadSleep(2 * 1000 * 1000);
	}
	threadSleep(500 * 1000);
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

		holder<screenListClass> list = newScreenList();
		for (uint32 i = 0; i < list->devicesCount(); i++)
		{
			CAGE_LOG(severityEnum::Info, "listing", string() + "monitor" + (list->primaryDevice() == i ? ", primary" : ""));
			const screenDeviceClass &d = list->device(i);
			CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "monitor id: '" + d.id() + "'");
			CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "monitor name: '" + d.name() + "'");

			screenMode testmode;

			for (uint32 j = 0; j < d.modesCount(); j++)
			{
				const screenMode &m = d.mode(j);
				CAGE_LOG(severityEnum::Info, "listing", string() + "video mode " + j + (d.currentMode() == j ? ", current" : ""));
				pointStruct r = m.resolution;
				CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "resolution: " + r.x + "*" + r.y);
				CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "frequency: " + m.frequency);
				if (d.currentMode() == j)
					testmode = m;
			}

			testScreen(d.id(), testmode.resolution, testmode.frequency);
		}

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
