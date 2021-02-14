#include "processor.h"

#include <cage-core/polytone.h>

void processSound()
{
	writeLine(string("use=") + inputFile);

	Holder<Polytone> polytone = newPolytone();
	polytone->importFile(inputFileName);

	SoundSourceHeader sds = {};
	sds.channels = polytone->channels();
	sds.frames = polytone->frames();
	sds.sampleRate = polytone->sampleRate();

	if (toBool(properties("loopBefore")))
		sds.flags = sds.flags | SoundFlags::LoopBeforeStart;
	if (toBool(properties("loopAfter")))
		sds.flags = sds.flags | SoundFlags::LoopAfterEnd;
	if (sds.frames * sds.channels * sizeof(float) < toUint32(properties("compressThreshold")))
		sds.soundType = SoundTypeEnum::RawRaw;
	else if (toBool(properties("stream")))
		sds.soundType = SoundTypeEnum::CompressedCompressed;
	else
		sds.soundType = SoundTypeEnum::CompressedRaw;

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "flags: " + (uint32)sds.flags);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "frames: " + sds.frames);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "channels: " + sds.channels);
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

	AssetHeader h = initializeAssetHeader();
	h.originalSize = sizeof(SoundSourceHeader) + sds.frames * sds.channels * sizeof(float);

	switch (sds.soundType)
	{
	case SoundTypeEnum::RawRaw:
	{
		polytoneConvertFormat(+polytone, PolytoneFormatEnum::Float);
		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(bufferView(sds));
		f->write(bufferCast<const char, const float>(polytone->rawViewFloat()));
		f->close();
	} break;
	case SoundTypeEnum::CompressedRaw:
	case SoundTypeEnum::CompressedCompressed:
	{
		Holder<PointerRange<char>> buff = polytone->exportBuffer(".ogg");
		const uint64 oggSize = buff.size();
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "original size: " + h.originalSize + " bytes");
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compressed size: " + oggSize + " bytes");
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compression ratio: " + (oggSize / (float)h.originalSize));
		switch (sds.soundType)
		{
		case SoundTypeEnum::CompressedRaw:
			h.compressedSize = oggSize + sizeof(SoundSourceHeader);
			break;
		case SoundTypeEnum::CompressedCompressed:
			h.compressedSize = oggSize + sizeof(SoundSourceHeader);
			h.originalSize = 0; // the sound will not be decoded on asset load, so do not allocate space for it
			break;
		case SoundTypeEnum::RawRaw:
			break; // do nothing here
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
		CAGE_THROW_ERROR(NotImplemented, "sound preview is not currently implemented");
	}
}

void analyzeSound()
{
	try
	{
		Holder<Polytone> polytone = newPolytone();
		polytone->importFile(inputFile);
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
