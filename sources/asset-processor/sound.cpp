#include <utility> // swap

#include "processor.h"

#include <cage-core/audioAlgorithms.h>
#include <cage-core/audioChannelsConverter.h>

void processSound()
{
	processor->writeLine(String("use=") + processor->inputFile);

	Holder<Audio> audio = newAudio();
	audio->importFile(processor->inputFileName);

	{
		const bool mono = toBool(processor->property("mono"));
		if (mono && audio->channels() != 1)
		{
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converting channels from " + audio->channels() + " to mono");
			audioConvertFormat(+audio, AudioFormatEnum::Float);
			Holder<Audio> tmp = newAudio();
			tmp->initialize(audio->frames(), 1, audio->sampleRate());
			PointerRange<float> dst = { (float *)tmp->rawViewFloat().begin(), (float *)tmp->rawViewFloat().end() };
			newAudioChannelsConverter()->convert(audio->rawViewFloat(), dst, audio->channels(), 1);
			std::swap(tmp, audio);
		}
	}

	{
		const uint32 sr = toUint32(processor->property("sampleRate"));
		if (sr != 0 && audio->sampleRate() != sr)
		{
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converting sample rate from " + audio->sampleRate() + " to " + sr);
			audioConvertSampleRate(+audio, sr);
		}
	}

	{
		const Real gain = toFloat(processor->property("gain"));
		if (abs(gain - 1) > 1e-5)
		{
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "applying gain " + gain);
			audioGain(+audio, gain);
		}
	}

	SoundSourceHeader sds = {};
	sds.channels = audio->channels();
	sds.frames = audio->frames();
	sds.sampleRate = audio->sampleRate();
	const uint64 rawSize = sds.frames * sds.channels * sizeof(float);

	if (rawSize < toUint32(processor->property("compressThreshold")))
		sds.soundType = SoundCompressionEnum::RawRaw;
	else if (toBool(processor->property("stream")))
		sds.soundType = SoundCompressionEnum::CompressedCompressed;
	else
		sds.soundType = SoundCompressionEnum::CompressedRaw;

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "frames: " + sds.frames);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "channels: " + sds.channels);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "raw size: " + rawSize + " bytes");
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "sample rate: " + sds.sampleRate);
	switch (sds.soundType)
	{
		case SoundCompressionEnum::RawRaw:
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "compression method: raw file, raw play");
			break;
		case SoundCompressionEnum::CompressedRaw:
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "compression method: compressed file, raw play");
			break;
		case SoundCompressionEnum::CompressedCompressed:
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "compression method: compressed file, compressed play");
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid sound compression method");
	}

	switch (sds.soundType)
	{
		case SoundCompressionEnum::RawRaw:
		{
			audioConvertFormat(+audio, AudioFormatEnum::Float);
			Holder<File> f = writeFile(processor->outputFileName);
			AssetHeader h = processor->initializeAssetHeader();
			h.originalSize = sizeof(SoundSourceHeader) + rawSize;
			f->write(bufferView(h));
			f->write(bufferView(sds));
			PointerRange<const char> buff = bufferCast<const char, const float>(audio->rawViewFloat());
			CAGE_ASSERT(buff.size() == rawSize);
			f->write(buff);
			f->close();
			break;
		}
		case SoundCompressionEnum::CompressedRaw:
		case SoundCompressionEnum::CompressedCompressed:
		{
			Holder<PointerRange<char>> buff = audio->exportBuffer(".ogg");
			const uint64 oggSize = buff.size();
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "compressed size: " + oggSize + " bytes");
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "compression ratio: " + (oggSize / (float)rawSize));
			AssetHeader h = processor->initializeAssetHeader();
			switch (sds.soundType)
			{
				case SoundCompressionEnum::CompressedRaw:
					h.compressedSize = sizeof(SoundSourceHeader) + oggSize;
					h.originalSize = sizeof(SoundSourceHeader) + rawSize; // same as rawraw, so that loading can use same code
					break;
				case SoundCompressionEnum::CompressedCompressed:
					h.compressedSize = sizeof(SoundSourceHeader) + oggSize;
					h.originalSize = 0; // the sound will not be decoded on asset load, so do not allocate space for it
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid sound compression method");
			}
			Holder<File> f = writeFile(processor->outputFileName);
			f->write(bufferView(h));
			f->write(bufferView(sds));
			f->write(buff);
			f->close();
			break;
		}
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid sound compression method");
	}

	// preview sound
	if (configGetBool("cage-asset-processor/sound/preview"))
	{
		const String dbgName = pathJoin(configGetString("cage-asset-processor/sound/path", "asset-preview"), Stringizer() + pathReplaceInvalidCharacters(processor->inputName) + ".ogg");
		audio->exportFile(dbgName);
	}
}

void analyzeSound()
{
	try
	{
		Holder<Audio> audio = newAudio();
		audio->importFile(processor->inputFileName);
		processor->writeLine("cage-begin");
		processor->writeLine("scheme=sound");
		processor->writeLine(String() + "asset=" + processor->inputFile);
		processor->writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
