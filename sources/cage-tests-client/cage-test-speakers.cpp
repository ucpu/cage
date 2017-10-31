#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/timer.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

using namespace cage;

void testDevice(const string &deviceId, uint32 sampleRate, bool raw)
{
	holder<soundContextClass> sndContext = newSoundContext(soundContextCreateConfig(), "testAudio:Context");
	holder<busClass> sndBus = newBus(sndContext.get());
	holder<sourceClass> sndSource = newSource(sndContext.get());
	sndSource->addOutput(sndBus.get());
	sndSource->setDataTone();
	speakerCreateConfig cnf;
	cnf.deviceId = deviceId;
	cnf.sampleRate = sampleRate;
	cnf.deviceRaw = raw;
	holder<speakerClass> sndSpeaker = newSpeaker(sndContext.get(), cnf, "testAudio:Speaker");
	sndSpeaker->setInput(sndBus.get());

	CAGE_LOG(severityEnum::Info, "speaker", string() + "stream: '" + sndSpeaker->getStreamName() + "'");
	CAGE_LOG(severityEnum::Info, "speaker", string() + "id: '" + sndSpeaker->getDeviceId() + "'");
	CAGE_LOG(severityEnum::Info, "speaker", string() + "name: '" + sndSpeaker->getDeviceName() + "'");
	CAGE_LOG(severityEnum::Info, "speaker", string() + "raw: " + sndSpeaker->getDeviceRaw());
	CAGE_LOG(severityEnum::Info, "speaker", string() + "layout: '" + sndSpeaker->getLayoutName() + "'");
	CAGE_LOG(severityEnum::Info, "speaker", string() + "channels: " + sndSpeaker->getChannelsCount());
	CAGE_LOG(severityEnum::Info, "speaker", string() + "sample rate: " + sndSpeaker->getOutputSampleRate());

	CAGE_LOG(severityEnum::Info, "speaker", "play start");
	holder<timerClass> tmr = newTimer();
	while (true)
	{
		uint64 t = tmr->microsSinceStart();
		if (t > 3 * 1000 * 1000)
			break;
		sndSpeaker->update(t);
		threadSleep(10000);
	}
	CAGE_LOG(severityEnum::Info, "speaker", "play stop");
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

		holder<speakerListClass> list = newSpeakerList();
		uint32 dc = list->deviceCount(), dd = list->deviceDefault();
		for (uint32 d = 0; d < dc; d++)
		{
			CAGE_LOG(severityEnum::Info, "listing", string() + "device" + (list->deviceRaw(d) ? ", raw" : "") + (dd == d ? ", default" : ""));
			CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "id: '" + list->deviceId(d) + "'");
			CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "name: '" + list->deviceName(d) + "'");
			{
				uint32 lc = list->layoutCount(d), ld = list->layoutCurrent(d);
				for (uint32 l = 0; l < lc; l++)
					CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "layout name: '" + list->layoutName(d, l) + "', channels: " + list->layoutChannels(d, l) + (l == ld ? ", current" : ""));
			}
			{
				uint32 sc = list->samplerateCount(d), sd = list->samplerateCurrent(d);
				for (uint32 s = 0; s < sc; s++)
					CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "samplerate min: " + list->samplerateMin(d, s) + ", max: " + list->samplerateMax(d, s));
				CAGE_LOG_CONTINUE(severityEnum::Info, "listing", string() + "samplerate current: " + sd);
			}
			if (list->deviceRaw(d))
				continue;
			testDevice(list->deviceId(d), 32000, false);
			testDevice(list->deviceId(d), 44100, false);
			testDevice(list->deviceId(d), 48000, false);
		}

		return 0;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "test", "caught exception");
		return 1;
	}
}
