#include "main.h"
#include <cage-core/math.h>
#include <cage-core/polytone.h>
#include <cage-core/serialization.h>

#include <initializer_list>
#include <vector>

void test(real, real);

namespace
{
	constexpr double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;

	void generateMono(Polytone *snd, uint32 pitch, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		const double step = twoPi * pitch / (double)sampleRate;
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

	void generateStereo(Polytone *snd, uint32 pitch, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		const double stepTone = twoPi * pitch / (double)sampleRate;
		const double stepDir = twoPi * 0.5 / (double)sampleRate;
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
		CAGE_TESTCASE("mono values");
		Holder<Polytone> snd = newPolytone();
		generateMono(+snd, 1, 8, 4);
		CAGE_TEST(snd->frames() == 8);
		CAGE_TEST(snd->channels() == 1);
		CAGE_TEST(snd->sampleRate() == 4);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
		CAGE_TEST(snd->rawViewFloat().size() == 8);
		test(snd->value(0, 0), 0);
		test(snd->value(1, 0), 1);
		test(snd->value(2, 0), 0);
		test(snd->value(3, 0), -1);
		test(snd->value(4, 0), 0);
		test(snd->value(5, 0), 1);
		test(snd->value(6, 0), 0);
		test(snd->value(7, 0), -1);
	}

	{
		CAGE_TESTCASE("stereo values");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 1, 8, 4);
		CAGE_TEST(snd->frames() == 8);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 4);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
		CAGE_TEST(snd->rawViewFloat().size() == 16);
		// left
		test(snd->value(0, 0), 0);
		test(snd->value(1, 0), 0.85355);
		test(snd->value(2, 0), 0);
		test(snd->value(3, 0), -0.85355);
		test(snd->value(4, 0), 0);
		test(snd->value(5, 0), 0.14644);
		test(snd->value(6, 0), 0);
		test(snd->value(7, 0), -0.14644);
		// right
		test(snd->value(0, 1), 0);
		test(snd->value(1, 1), 0.14644);
		test(snd->value(2, 1), 0);
		test(snd->value(3, 1), -0.14644);
		test(snd->value(4, 1), 0);
		test(snd->value(5, 1), 0.85355);
		test(snd->value(6, 1), 0);
		test(snd->value(7, 1), -0.85355);
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
		CAGE_TESTCASE("format conversions");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
		polytoneConvertFormat(+snd, PolytoneFormatEnum::S16);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::S16);
		polytoneConvertFormat(+snd, PolytoneFormatEnum::Vorbis);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Vorbis);
		polytoneConvertFormat(+snd, PolytoneFormatEnum::Float);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == PolytoneFormatEnum::Float);
		snd->exportFile("sounds/conversions_440_stereo.wav");
	}

	{
		CAGE_TESTCASE("export formats");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
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

	{
		CAGE_TESTCASE("blit same format");
		Holder<Polytone> src = newPolytone();
		generateStereo(+src, 440);
		Holder<Polytone> dst = newPolytone();
		generateStereo(+dst, 220);
		polytoneBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_same_format.wav");
	}

	{
		CAGE_TESTCASE("blit format s16 to float");
		Holder<Polytone> src = newPolytone();
		generateStereo(+src, 440);
		polytoneConvertFormat(+src, PolytoneFormatEnum::S16);
		Holder<Polytone> dst = newPolytone();
		generateStereo(+dst, 220);
		polytoneBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_format_s16_float.wav");
	}

	{
		CAGE_TESTCASE("blit format vorbis to s16");
		Holder<Polytone> src = newPolytone();
		generateStereo(+src, 440);
		polytoneConvertFormat(+src, PolytoneFormatEnum::Vorbis);
		Holder<Polytone> dst = newPolytone();
		generateStereo(+dst, 220);
		polytoneConvertFormat(+dst, PolytoneFormatEnum::S16);
		polytoneBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_format_vorbis_s16.wav");
	}

	{
		CAGE_TESTCASE("blit into vorbis");
		Holder<Polytone> src = newPolytone();
		generateStereo(+src, 440);
		Holder<Polytone> dst = newPolytone();
		generateStereo(+dst, 220);
		polytoneConvertFormat(+dst, PolytoneFormatEnum::Vorbis);
		CAGE_TEST_THROWN(polytoneBlit(+src, +dst, 120000, 120000, 240000));
	}

	{
		CAGE_TESTCASE("sample rate conversion to 44100");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		polytoneConvertSampleRate(+snd, 44100);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 44100);
		snd->exportFile("sounds/sample_rate_44100.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion to 96000");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		polytoneConvertSampleRate(+snd, 96000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 96000);
		snd->exportFile("sounds/sample_rate_96000.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion half frames");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		polytoneConvertFrames(+snd, snd->frames() / 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->frames() == 240000);
		snd->exportFile("sounds/sample_rate_half_frames.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion double frames");
		Holder<Polytone> snd = newPolytone();
		generateStereo(+snd, 440);
		polytoneConvertFrames(+snd, snd->frames() * 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->frames() == 960000);
		snd->exportFile("sounds/sample_rate_double_frames.wav");
	}
}
