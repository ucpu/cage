#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/timer.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/sound.h>
#include <cage-client/assets.h>
#include <cage-client/opengl.h>

using namespace cage;

bool closing = false;
const uint32 assetsName = hashString("cage/tests/logo/logo.pack");

bool applicationQuit()
{
	closing = true;
	return true;
}

bool windowClose(windowClass *)
{
	closing = true;
	return true;
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

		// window
		holder<windowClass> window = newWindow();
		eventListener<bool()> applicationQuitListener;
		applicationQuitListener.bind<&applicationQuit>();
		window->events.applicationQuit.add(applicationQuitListener);
		eventListener<bool(windowClass*)> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		window->events.windowClose.add(windowCloseListener);
		window->title("cage test logo");
		detail::initializeOpengl();

		// sound
		holder<soundContextClass> sl = newSoundContext(soundContextCreateConfig(), "cage");

		// assets
		holder<assetManagerClass> assets = newAssetManager(assetManagerCreateConfig());
		assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(0));
		assets->defineScheme<shaderClass>(assetSchemeIndexShader, genAssetSchemeShader(0, window.get()));
		assets->defineScheme<textureClass>(assetSchemeIndexTexture, genAssetSchemeTexture(0, window.get()));
		assets->defineScheme<meshClass>(assetSchemeIndexMesh, genAssetSchemeMesh(0, window.get()));
		assets->defineScheme<fontClass>(assetSchemeIndexFont, genAssetSchemeFont(0, window.get()));
		assets->defineScheme<sourceClass>(assetSchemeIndexSound, genAssetSchemeSound(0, sl.get()));

		// load assets
		assets->add(assetsName);
		while (true)
		{
			if (assets->ready(assetsName))
				break;
			assets->processControlThread();
			assets->processCustomThread(0);
		}

		// fetch assets
		meshClass *mesh = assets->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
		textureClass *texture = assets->get<assetSchemeIndexTexture, textureClass>(hashString("cage/tests/logo/logo.png"));
		shaderClass *shader = assets->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/blitColor.glsl"));
		sourceClass *source = assets->get<assetSchemeIndexSound, sourceClass>(hashString("cage/tests/logo/logo.ogg"));

		//source->setDataTone();

		{
			// initialize graphics
			mesh->bind();
			texture->bind();
			shader->bind();
			shader->uniform(0, vec4(0, 0, 1, 1));

			// initialize sounds
			holder<speakerClass> speaker = newSpeaker(sl.get(), speakerCreateConfig(), "cage");
			holder<busClass> bus = newBus(sl.get());
			speaker->setInput(bus.get());
			source->addOutput(bus.get());

			// show the window
			pointStruct res(600, 600);
			window->windowedSize(res);
			window->modeSetWindowed((windowFlags)(windowFlags::Border | windowFlags::Resizeable));

			// loop
			while (!closing)
			{
				res = window->resolution();
				glViewport(0, 0, res.x, res.y);
				mesh->dispatch();
				speaker->update(getApplicationTime());
				threadSleep(10000);
				window->swapBuffers();
				window->processEvents();
			}
		}

		// unload assets
		assets->remove(assetsName);
		while (assets->countTotal())
		{
			assets->processControlThread();
			assets->processCustomThread(0);
		}

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}

#include <cage-client/highPerformanceGpuHint.h>
