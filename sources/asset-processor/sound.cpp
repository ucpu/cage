#include "processor.h"

#include <cage-core/audio.h>

void processSound()
{
	writeLine(string("use=") + inputFile);

	Holder<Audio> audio = newAudio();
	audio->importFile(inputFileName);

	SoundSourceHeader sds = {};
	sds.channels = audio->channels();
	sds.frames = audio->frames();
	sds.sampleRate = audio->sampleRate();
	const uint64 rawSize = sds.frames * sds.channels * sizeof(float);

	if (toBool(properties("loopBefore")))
		sds.flags = sds.flags | SoundFlags::LoopBeforeStart;
	if (toBool(properties("loopAfter")))
		sds.flags = sds.flags | SoundFlags::LoopAfterEnd;
	if (rawSize < toUint32(properties("compressThreshold")))
		sds.soundType = SoundTypeEnum::RawRaw;
	else if (toBool(properties("stream")))
		sds.soundType = SoundTypeEnum::CompressedCompressed;
	else
		sds.soundType = SoundTypeEnum::CompressedRaw;

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "flags: " + (uint32)sds.flags);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "frames: " + sds.frames);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "channels: " + sds.channels);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "raw size: " + rawSize + " bytes");
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "samplerate: " + sds.sampleRate);
	switch (sds.soundType)
	{
	case SoundTypeEnum::RawRaw:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: raw file, raw play");
		break;
	case SoundTypeEnum::CompressedRaw:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: compressed file, raw play");
		break;
	case SoundTypeEnum::CompressedCompressed:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: compressed file, compressed play");
		break;
	default:
		CAGE_THROW_CRITICAL(Exception, "invalid sound type");
	}

	switch (sds.soundType)
	{
	case SoundTypeEnum::RawRaw:
	{
		audioConvertFormat(+audio, AudioFormatEnum::Float);
		Holder<File> f = writeFile(outputFileName);
		AssetHeader h = initializeAssetHeader();
		h.originalSize = sizeof(SoundSourceHeader) + rawSize;
		f->write(bufferView(h));
		f->write(bufferView(sds));
		PointerRange<const char> buff = bufferCast<const char, const float>(audio->rawViewFloat());
		CAGE_ASSERT(buff.size() == rawSize);
		f->write(buff);
		f->close();
	} break;
	case SoundTypeEnum::CompressedRaw:
	case SoundTypeEnum::CompressedCompressed:
	{
		Holder<PointerRange<char>> buff = audio->exportBuffer(".ogg");
		const uint64 oggSize = buff.size();
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compressed size: " + oggSize + " bytes");
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compression ratio: " + (oggSize / (float)rawSize));
		AssetHeader h = initializeAssetHeader();
		switch (sds.soundType)
		{
		case SoundTypeEnum::CompressedRaw:
			h.compressedSize = sizeof(SoundSourceHeader) + oggSize;
			h.originalSize = sizeof(SoundSourceHeader) + rawSize; // same as rawraw, so that loading can use same code
			break;
		case SoundTypeEnum::CompressedCompressed:
			h.compressedSize = sizeof(SoundSourceHeader) + oggSize;
			h.originalSize = 0; // the sound will not be decoded on asset load, so do not allocate space for it
			break;
		}
		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(bufferView(sds));
		f->write(buff);
		f->close();
	} break;
	default:
		CAGE_THROW_CRITICAL(Exception, "invalid sound type");
	}

	// preview sound
	if (configGetBool("cage-asset-processor/sound/preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor/sound/path", "asset-preview"), stringizer() + pathReplaceInvalidCharacters(inputName) + ".ogg");
		audio->exportFile(dbgName);
	}
}

void analyzeSound()
{
	try
	{
		Holder<Audio> audio = newAudio();
		audio->importFile(inputFileName);
		writeLine("cage-begin");
		writeLine("scheme=sound");
		writeLine(string() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
