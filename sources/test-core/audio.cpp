#include "main.h"

#include <cage-core/math.h>
#include <cage-core/audio.h>
#include <cage-core/audioAlgorithms.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-core/serialization.h>
#include <initializer_list>
#include <vector>

void test(Real, Real);

namespace
{
	constexpr double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;

	void generateMono(Audio *snd, uint32 pitch, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		const double step = twoPi * pitch / (double)sampleRate;
		std::vector<Real> samples;
		samples.reserve(frames);
		for (uint32 index = 0; index < frames; index++)
		{
			double angle = index * step;
			Real sample = sin(Rads(angle));
			samples.push_back(sample);
		}
		snd->importRaw(bufferCast<const char, Real>(samples), frames, 1, sampleRate, AudioFormatEnum::Float);
	}

	void generateStereo(Audio *snd, uint32 pitch, uint32 frames = 480000, uint32 sampleRate = 48000)
	{
		const double stepTone = twoPi * pitch / (double)sampleRate;
		const double stepDir = twoPi * 0.5 / (double)sampleRate;
		std::vector<Real> samples;
		samples.reserve(frames * 2);
		for (uint32 index = 0; index < frames; index++)
		{
			Real sample = sin(Rads(index * stepTone));
			Real pan = sin(Rads(index * stepDir)) * 0.5 + 0.5;
			Real left = sample * pan;
			Real right = sample * (1 - pan);
			samples.push_back(left);
			samples.push_back(right);
		}
		snd->importRaw(bufferCast<const char, Real>(samples), frames, 2, sampleRate, AudioFormatEnum::Float);
	}
}

void testAudio()
{
	CAGE_TESTCASE("audio");

	{
		CAGE_TESTCASE("pure tones");
		Holder<Audio> snd = newAudio();
		generateMono(+snd, 55, 480000, 48000);
		snd->exportFile("sounds/tone_A1_55_mono.wav");
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 1);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
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
		Holder<Audio> snd = newAudio();
		generateMono(+snd, 1, 8, 4);
		CAGE_TEST(snd->frames() == 8);
		CAGE_TEST(snd->channels() == 1);
		CAGE_TEST(snd->sampleRate() == 4);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
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
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 1, 8, 4);
		CAGE_TEST(snd->frames() == 8);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 4);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
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
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440, 441000, 44100);
		snd->exportFile("sounds/panning_440_stereo_44100.wav");
		CAGE_TEST(snd->frames() == 441000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 44100);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
	}

	{
		CAGE_TESTCASE("panning tone at 48000");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		snd->exportFile("sounds/panning_440_stereo_48000.wav");
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
	}

	{
		CAGE_TESTCASE("format conversions");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
		audioConvertFormat(+snd, AudioFormatEnum::S16);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::S16);
		audioConvertFormat(+snd, AudioFormatEnum::Vorbis);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::Vorbis);
		audioConvertFormat(+snd, AudioFormatEnum::Float);
		CAGE_TEST(snd->frames() == 480000);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 48000);
		CAGE_TEST(snd->format() == AudioFormatEnum::Float);
		snd->exportFile("sounds/conversions_440_stereo.wav");
	}

	{
		CAGE_TESTCASE("export formats");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		// no encoding to mp3 or flac for now
		for (const String &format : { ".ogg", ".wav" })
			snd->exportFile(Stringizer() + "sounds/formats/sample" + format);
	}

	{
		CAGE_TESTCASE("import formats");
		Holder<Audio> snd = newAudio();
		for (const String &format : { ".ogg", ".wav" })
		{
			snd->importFile(Stringizer() + "sounds/formats/sample" + format);
			CAGE_TEST(snd->frames() == 480000);
			CAGE_TEST(snd->channels() == 2);
			CAGE_TEST(snd->sampleRate() == 48000);
			snd->clear();
		}
	}

	{
		CAGE_TESTCASE("blit same format");
		Holder<Audio> src = newAudio();
		generateStereo(+src, 440);
		Holder<Audio> dst = newAudio();
		generateStereo(+dst, 220);
		audioBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_same_format.wav");
	}

	{
		CAGE_TESTCASE("blit format s16 to float");
		Holder<Audio> src = newAudio();
		generateStereo(+src, 440);
		audioConvertFormat(+src, AudioFormatEnum::S16);
		Holder<Audio> dst = newAudio();
		generateStereo(+dst, 220);
		audioBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_format_s16_float.wav");
	}

	{
		CAGE_TESTCASE("blit format vorbis to s16");
		Holder<Audio> src = newAudio();
		generateStereo(+src, 440);
		audioConvertFormat(+src, AudioFormatEnum::Vorbis);
		Holder<Audio> dst = newAudio();
		generateStereo(+dst, 220);
		audioConvertFormat(+dst, AudioFormatEnum::S16);
		audioBlit(+src, +dst, 120000, 120000, 240000);
		dst->exportFile("sounds/blit_format_vorbis_s16.wav");
	}

	{
		CAGE_TESTCASE("blit into vorbis");
		Holder<Audio> src = newAudio();
		generateStereo(+src, 440);
		Holder<Audio> dst = newAudio();
		generateStereo(+dst, 220);
		audioConvertFormat(+dst, AudioFormatEnum::Vorbis);
		CAGE_TEST_THROWN(audioBlit(+src, +dst, 120000, 120000, 240000));
	}

	{
		CAGE_TESTCASE("sample rate conversion to 44100");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		audioConvertSampleRate(+snd, 44100, 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 44100);
		snd->exportFile("sounds/sample_rate_44100.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion to 96000");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		audioConvertSampleRate(+snd, 96000, 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->sampleRate() == 96000);
		snd->exportFile("sounds/sample_rate_96000.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion half frames");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		audioConvertFrames(+snd, snd->frames() / 2, 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->frames() == 240000);
		snd->exportFile("sounds/sample_rate_half_frames.wav");
	}

	{
		CAGE_TESTCASE("sample rate conversion double frames");
		Holder<Audio> snd = newAudio();
		generateStereo(+snd, 440);
		audioConvertFrames(+snd, snd->frames() * 2, 2);
		CAGE_TEST(snd->channels() == 2);
		CAGE_TEST(snd->frames() == 960000);
		snd->exportFile("sounds/sample_rate_double_frames.wav");
	}

	{
		CAGE_TESTCASE("doppler");
		Holder<Audio> src = newAudio();
		generateStereo(+src, 440);
		Holder<Audio> dst = src->copy();
		PointerRange<const float> srcb = src->rawViewFloat();
		PointerRange<float> dstb = { (float *)dst->rawViewFloat().data(), (float *)dst->rawViewFloat().data() + dst->rawViewFloat().size() };
		Holder<SampleRateConverter> cnv = newSampleRateConverter(src->channels());
		cnv->convert(srcb, dstb, 2, 0.5);
		dst->exportFile("sounds/doppler.wav");
	}
}
