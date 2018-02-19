#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/assets.h>
#include <cage-core/utility/timer.h>
#include <cage-core/utility/hashString.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/sound.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

using namespace cage;

volatile bool destroying;
holder<windowClass> window;
holder<soundContextClass> sound;
holder<assetManagerClass> assets;

void glThread()
{
	window->makeCurrent();
	while (!destroying)
	{
		while (assets->processCustomThread(1));
		threadSleep(5000);
	}
	window->makeNotCurrent();
}

void slThread()
{
	while (!destroying)
	{
		while (assets->processCustomThread(2));
		threadSleep(5000);
	}
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

		// contexts
		window = newWindow();
		window->makeNotCurrent();
		sound = newSoundContext(soundContextCreateConfig(), "cage");

		// asset schemes
		assets = newAssetManager(assetManagerCreateConfig());
		assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(0));
		assets->defineScheme<shaderClass>(assetSchemeIndexShader, genAssetSchemeShader(1, window.get()));
		assets->defineScheme<textureClass>(assetSchemeIndexTexture, genAssetSchemeTexture(1, window.get()));
		assets->defineScheme<meshClass>(assetSchemeIndexMesh, genAssetSchemeMesh(1, window.get()));
		assets->defineScheme<fontClass>(assetSchemeIndexFont, genAssetSchemeFont(1, window.get()));
		assets->defineScheme<sourceClass>(assetSchemeIndexSound, genAssetSchemeSound(2, sound.get()));

		// threads
		holder<threadClass> thrGl = newThread(delegate<void()>().bind<&glThread>(), "opengl");
		holder<threadClass> thrSl = newThread(delegate<void()>().bind<&slThread>(), "sound");

		// asset names
		uint32 names[] = {
			hashString("cage/cage.pack"),
			hashString("cage/texture/texture.pack"),
			hashString("cage/sound/sound.pack"),
			hashString("cage/shader/shader.pack"),
			hashString("cage/mesh/mesh.pack"),
			hashString("cage/font/font.pack"),
			hashString("cage/cage.pack"),
			hashString("cage/texture/texture.pack"),
			hashString("cage/sound/sound.pack"),
			hashString("cage/shader/shader.pack"),
			hashString("cage/mesh/mesh.pack"),
			hashString("cage/font/font.pack"),
			42,
		};
		static const uint32 count = sizeof(names) / sizeof(names[0]);
		bool loaded[count];
		for (uint32 i = 0; i < count; i++)
			loaded[i] = false;

		assets->fabricate(assetSchemeIndexTexture, 42, "fabricated texture");

		CAGE_LOG(severityEnum::Info, "test", "starting the test, please wait");

		// loop
		for (uint32 step = 0; step < 30; step++)
		{
			uint64 stepEnd = getApplicationTime() + 1000 * 1000;
			while (getApplicationTime() < stepEnd)
			{
				if (random() < 0.7)
				{
					uint32 i = random(0, count);
					CAGE_ASSERT_RUNTIME(i < count);
					if (loaded[i])
						assets->remove(names[i]);
					else
						assets->add(names[i]);
					loaded[i] = !loaded[i];
				}
				else
				{
					while (assets->processControlThread() || assets->processCustomThread(0));
					threadSleep(1000);
				}
			}
			CAGE_LOG(severityEnum::Info, "test", string() + "step " + (step + 1) + "/30 finished");
		}

		CAGE_LOG(severityEnum::Info, "test", "test finished");

		// clean up
		assets->remove(42);
		for (uint32 i = 0; i < count; i++)
			if (loaded[i])
				assets->remove(names[i]);
		while (assets->countTotal())
		{
			while (assets->processControlThread() || assets->processCustomThread(0));
			threadSleep(5000);
		}
		destroying = true;
		thrGl->wait();
		thrSl->wait();
		sound.clear();
		window.clear();
		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}

