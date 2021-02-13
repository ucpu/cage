#include "main.h"
#include <cage-core/math.h>
#include <cage-core/polytone.h>
#include <cage-core/serialization.h>

#include <initializer_list>
#include <vector>

namespace
{
	void generateMono(Polytone *snd, uint32 pitch = 440, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		constexpr double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;
		double step = twoPi * pitch / (double)sampleRate;
		std::vector<real> samples;
		samples.reserve(frames);
		for (uint32 index = 0; index < frames; index++)
		{
			double angle = index * step;
			real sample = sin(rads(angle));
			samples.push_back(sample);
		}
		snd->importRaw(bufferCast<const char, real>(samples), frames, 1, sampleRate, PolytoneFormatEnum::Float);
	}

	void generateStereo(Polytone *snd, uint32 pitch = 440, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		constexpr double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;
		double stepTone = twoPi * pitch / (double)sampleRate;
		double stepDir = twoPi * 0.5 / (double)sampleRate;
		std::vector<real> samples;
		samples.reserve(frames * 2);
		for (uint32 index = 0; index < frames; index++)
		{
			real sample = sin(rads(index * stepTone));
			real pan = sin(rads(index * stepDir)) * 0.5 + 0.5;
			real left = sample * pan;
			real right = sample * (1 - pan);
			samples.push_back(left);
			samples.push_back(right);
		}
		snd->importRaw(bufferCast<const char, real>(samples), frames, 2, sampleRate, PolytoneFormatEnum::Float);
	}
}

void testPolytone()
{
	CAGE_TESTCASE("polytone");

	{
		CAGE_TESTCASE("pure tones");
		Holder<Polytone> snd = newPolytone();
		generateMono(+snd, 55, 480000, 48000);
		snd->exportFile("sounds/tone_A1_55_mono.wav");
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 1);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
		generateMono(+snd, 110);
		snd->exportFile("sounds/tone_A2_110_mono.wav");
		generateMono(+snd, 220);
		snd->exportFile("sounds/tone_A3_220_mono.wav");
		generateMono(+snd, 440);
		snd->exportFile("sounds/tone_A4_440_mono.wav");
		generateMono(+snd, 880);
		snd->exportFile("sounds/tone_A5_880_mono.wav");
		generateMono(+snd, 1760);
		snd->exportFile("sounds/tone_A6_1760_mono.wav");
		generateMono(+snd, 3520);
		snd->exportFile("sounds/tone_A7_3520_mono.wav");
	}

	{
		CAGE_TESTCASE("panning tone at 44100");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440, 441000, 44100);
		snd->exportFile("sounds/panning_440_stereo_44100.wav");
		CAGE_TEST(snd->frames() == 441000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 44100);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
	}

	{
		CAGE_TESTCASE("panning tone at 48000");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		snd->exportFile("sounds/panning_440_stereo_48000.wav");
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
	}

	{
		CAGE_TESTCASE("export formats");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440, 480000, 48000);
		// no encoding to mp3 or flac for now
		for (const string &format : { ".ogg", ".wav" })
			snd->exportFile(stringizer() + "sounds/formats/sample" + format);
	}

	{
		CAGE_TESTCASE("import formats");
		Holder<Polytone> snd = newPolytone();
		for (const string &format : { ".ogg", ".wav" })
		{
			snd->importFile(stringizer() + "sounds/formats/sample" + format);
			CAGE_TEST(snd->frames() == 480000);
			CAGE_TEST(snd->channels() == 2);
			CAGE_TEST(snd->sampleRate() == 48000);
			snd->clear();
		}
	}
}
